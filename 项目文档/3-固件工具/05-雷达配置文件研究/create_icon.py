#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
创建雷达配置工具的图标
使用PIL生成一个现代化的雷达图标
"""

from PIL import Image, ImageDraw, ImageFont
import math

def create_radar_icon(size=256):
    """创建雷达扫描图标"""
    # 创建透明背景
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # 中心点和半径
    center = size // 2
    max_radius = size // 2 - 10
    
    # 背景渐变圆（深蓝到浅蓝）
    for i in range(5):
        radius = max_radius - i * 8
        alpha = 200 - i * 30
        color = (0, 120 + i * 20, 255, alpha)
        draw.ellipse([center - radius, center - radius, 
                     center + radius, center + radius], 
                     fill=color, outline=None)
    
    # 绘制雷达圆环（3个同心圆）
    for i in range(3):
        radius = max_radius * (1 - i * 0.3)
        draw.ellipse([center - radius, center - radius,
                     center + radius, center + radius],
                     outline=(0, 255, 255, 255), width=3)
    
    # 绘制十字准线
    line_color = (0, 255, 255, 200)
    line_width = 2
    
    # 水平线
    draw.line([10, center, size-10, center], fill=line_color, width=line_width)
    # 垂直线
    draw.line([center, 10, center, size-10], fill=line_color, width=line_width)
    
    # 绘制雷达扫描扇形（亮青色）
    angle = 60  # 扫描角度
    
    # 扇形渐变效果
    for i in range(angle):
        current_angle = -90 + i  # 从12点方向开始
        alpha = int(255 * (1 - i / angle))  # 渐变透明度
        
        x1 = center
        y1 = center
        x2 = center + max_radius * math.cos(math.radians(current_angle))
        y2 = center + max_radius * math.sin(math.radians(current_angle))
        
        draw.line([x1, y1, x2, y2], fill=(0, 255, 200, alpha), width=2)
    
    # 绘制扫描线末端亮点
    scan_angle = -90 + angle
    x = center + max_radius * math.cos(math.radians(scan_angle))
    y = center + max_radius * math.sin(math.radians(scan_angle))
    draw.ellipse([x-8, y-8, x+8, y+8], fill=(0, 255, 255, 255))
    
    # 绘制中心点
    center_radius = 12
    draw.ellipse([center - center_radius, center - center_radius,
                 center + center_radius, center + center_radius],
                 fill=(255, 100, 0, 255), outline=(255, 200, 0, 255), width=2)
    
    # 添加一些目标点（模拟检测到的目标）
    targets = [
        (0.4, 45),   # (距离比例, 角度)
        (0.6, -30),
        (0.8, 15),
    ]
    
    for dist_ratio, angle_deg in targets:
        angle_rad = math.radians(angle_deg - 90)
        tx = center + max_radius * dist_ratio * math.cos(angle_rad)
        ty = center + max_radius * dist_ratio * math.sin(angle_rad)
        
        # 绘制目标点（红色闪烁点）
        for r in range(3):
            alpha = 255 - r * 60
            radius = 6 - r * 2
            draw.ellipse([tx - radius, ty - radius, tx + radius, ty + radius],
                        fill=(255, 50, 50, alpha))
    
    # 添加外发光效果
    glow_img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    glow_draw = ImageDraw.Draw(glow_img)
    
    for i in range(5):
        alpha = 40 - i * 8
        glow_draw.ellipse([10 - i*2, 10 - i*2, size - 10 + i*2, size - 10 + i*2],
                         outline=(0, 200, 255, alpha), width=2)
    
    # 合成图像
    result = Image.alpha_composite(glow_img, img)
    
    return result


def save_icon_sizes(base_img):
    """保存不同尺寸的图标"""
    sizes = [256, 128, 64, 48, 32, 16]
    
    # 保存为ICO文件（包含多个尺寸）
    icon_images = []
    for size in sizes:
        resized = base_img.resize((size, size), Image.LANCZOS)
        icon_images.append(resized)
    
    # 保存为.ico文件
    icon_images[0].save('radar_icon.ico', format='ICO', sizes=[(s, s) for s in sizes])
    
    # 也保存为PNG（方便查看）
    base_img.save('radar_icon.png', format='PNG')
    
    print("✅ 图标已创建:")
    print("   📁 radar_icon.ico (多尺寸ICO)")
    print("   📁 radar_icon.png (PNG预览)")


if __name__ == '__main__':
    print("🎨 开始创建雷达图标...")
    
    # 创建256x256的基础图标
    icon = create_radar_icon(256)
    
    # 保存不同尺寸
    save_icon_sizes(icon)
    
    print("✨ 图标创建完成！")
