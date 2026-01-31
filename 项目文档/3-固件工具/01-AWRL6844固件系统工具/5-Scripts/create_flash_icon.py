#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
创建固件烧录工具的专业图标
高端大气上档次的设计风格
"""

from PIL import Image, ImageDraw, ImageFont
import math

def create_flash_icon(size=256):
    """创建专业的闪电烧录图标"""
    # 创建透明背景
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    center = size // 2

    # 1. 绘制深色渐变背景圆
    for i in range(20):
        radius = size // 2 - 5 - i * 2
        alpha = 250 - i * 10
        # 深蓝到亮蓝渐变
        blue_val = 30 + i * 8
        color = (0, blue_val, 120 + i * 4, alpha)
        draw.ellipse([center - radius, center - radius,
                     center + radius, center + radius],
                     fill=color, outline=None)

    # 2. 绘制芯片轮廓（方形）
    chip_size = size * 0.6
    chip_left = center - chip_size // 2
    chip_top = center - chip_size // 2
    chip_right = center + chip_size // 2
    chip_bottom = center + chip_size // 2

    # 芯片主体（深灰）
    draw.rectangle([chip_left, chip_top, chip_right, chip_bottom],
                   fill=(30, 30, 40, 255), outline=None)

    # 芯片边框（亮蓝色发光）
    for i in range(3):
        offset = i * 2
        draw.rectangle([chip_left - offset, chip_top - offset,
                       chip_right + offset, chip_bottom + offset],
                      outline=(0, 150 + i * 30, 255, 200 - i * 50), width=2)

    # 3. 绘制芯片引脚（四边）
    pin_length = 15
    pin_width = 8
    pin_spacing = 20
    pin_color = (200, 200, 220, 255)

    # 计算引脚数量
    num_pins = int(chip_size / pin_spacing)

    for i in range(num_pins):
        offset = (i - num_pins // 2) * pin_spacing

        # 顶部引脚
        draw.rectangle([center + offset - pin_width // 2, chip_top - pin_length,
                       center + offset + pin_width // 2, chip_top],
                      fill=pin_color)

        # 底部引脚
        draw.rectangle([center + offset - pin_width // 2, chip_bottom,
                       center + offset + pin_width // 2, chip_bottom + pin_length],
                      fill=pin_color)

        # 左侧引脚
        draw.rectangle([chip_left - pin_length, center + offset - pin_width // 2,
                       chip_left, center + offset + pin_width // 2],
                      fill=pin_color)

        # 右侧引脚
        draw.rectangle([chip_right, center + offset - pin_width // 2,
                       chip_right + pin_length, center + offset + pin_width // 2],
                      fill=pin_color)

    # 4. 绘制芯片表面细节（网格）
    grid_color = (60, 60, 80, 180)
    for i in range(5):
        y_pos = chip_top + (i + 1) * chip_size / 6
        draw.line([chip_left + 10, y_pos, chip_right - 10, y_pos],
                 fill=grid_color, width=1)

    for i in range(5):
        x_pos = chip_left + (i + 1) * chip_size / 6
        draw.line([x_pos, chip_top + 10, x_pos, chip_bottom - 10],
                 fill=grid_color, width=1)

    # 5. 绘制中心闪电（烧录符号）- 最亮眼的部分
    lightning_color = (255, 220, 0, 255)  # 金黄色
    lightning_glow = (255, 150, 0, 200)   # 橙色发光

    # 闪电路径（手工绘制的锯齿形）
    scale = chip_size * 0.5
    lightning_points = [
        (center + 0.15 * scale, chip_top + 0.3 * scale),    # 右上起点
        (center - 0.05 * scale, center - 0.1 * scale),       # 中左
        (center + 0.1 * scale, center - 0.05 * scale),       # 中右小尖
        (center - 0.2 * scale, chip_bottom - 0.25 * scale),  # 左下
        (center - 0.05 * scale, center + 0.1 * scale),       # 中间返回点
        (center - 0.15 * scale, center),                      # 中左返回
        (center + 0.15 * scale, chip_top + 0.3 * scale),     # 闭合
    ]

    # 绘制发光效果（多层）
    for i in range(5):
        glow_points = [(x + (5 - i) * (1 if j % 2 == 0 else -1),
                       y + (5 - i) * (1 if j % 2 == 1 else -1))
                      for j, (x, y) in enumerate(lightning_points)]
        draw.polygon(glow_points, fill=None,
                    outline=(255, 180 - i * 20, 0, 150 - i * 30), width=3)

    # 绘制闪电主体
    draw.polygon(lightning_points, fill=lightning_color, outline=(255, 255, 255, 255), width=2)

    # 6. 添加光斑效果（闪电周围）
    for angle in range(0, 360, 45):
        rad = math.radians(angle)
        x1 = center + math.cos(rad) * scale * 0.3
        y1 = center + math.sin(rad) * scale * 0.3
        x2 = center + math.cos(rad) * scale * 0.45
        y2 = center + math.sin(rad) * scale * 0.45
        draw.line([x1, y1, x2, y2], fill=(255, 255, 100, 150), width=2)

    # 7. 添加"Ti"标识（芯片顶部）
    try:
        # 尝试使用系统字体
        font_size = int(chip_size * 0.15)
        font = ImageFont.truetype("arial.ttf", font_size)
    except:
        font = None

    ti_text = "Ti"
    if font:
        bbox = draw.textbbox((0, 0), ti_text, font=font)
        text_width = bbox[2] - bbox[0]
        text_height = bbox[3] - bbox[1]
    else:
        text_width = 20
        text_height = 15

    text_x = center - text_width // 2
    text_y = chip_top + chip_size * 0.1

    # 文字发光效果
    for offset in range(3, 0, -1):
        draw.text((text_x + offset, text_y + offset), ti_text,
                 fill=(0, 100, 200, 100), font=font)

    # 主文字（亮青色）
    draw.text((text_x, text_y), ti_text, fill=(0, 200, 255, 255), font=font)

    # 8. 添加外圈光环
    for i in range(3):
        radius = size // 2 - 8 - i * 3
        alpha = 100 - i * 30
        draw.ellipse([center - radius, center - radius,
                     center + radius, center + radius],
                    outline=(0, 200, 255, alpha), width=2)

    # 9. 添加四角装饰线（科技感）
    corner_length = 25
    corner_offset = 15
    corner_color = (0, 200, 255, 200)

    # 左上角
    draw.line([corner_offset, corner_offset + corner_length,
              corner_offset, corner_offset], fill=corner_color, width=3)
    draw.line([corner_offset, corner_offset,
              corner_offset + corner_length, corner_offset], fill=corner_color, width=3)

    # 右上角
    draw.line([size - corner_offset - corner_length, corner_offset,
              size - corner_offset, corner_offset], fill=corner_color, width=3)
    draw.line([size - corner_offset, corner_offset,
              size - corner_offset, corner_offset + corner_length], fill=corner_color, width=3)

    # 左下角
    draw.line([corner_offset, size - corner_offset - corner_length,
              corner_offset, size - corner_offset], fill=corner_color, width=3)
    draw.line([corner_offset, size - corner_offset,
              corner_offset + corner_length, size - corner_offset], fill=corner_color, width=3)

    # 右下角
    draw.line([size - corner_offset, size - corner_offset - corner_length,
              size - corner_offset, size - corner_offset], fill=corner_color, width=3)
    draw.line([size - corner_offset - corner_length, size - corner_offset,
              size - corner_offset, size - corner_offset], fill=corner_color, width=3)

    return img


def save_icon_sizes(base_img):
    """保存不同尺寸的图标"""
    sizes = [256, 128, 64, 48, 32, 16]

    # 保存为ICO文件（包含多个尺寸）
    icon_images = []
    for size in sizes:
        resized = base_img.resize((size, size), Image.LANCZOS)
        icon_images.append(resized)

    # 保存为.ico文件
    icon_images[0].save('flash_tool_icon.ico', format='ICO', sizes=[(s, s) for s in sizes])

    # 也保存为PNG（方便查看）
    base_img.save('flash_tool_icon.png', format='PNG')

    print("[OK] 固件烧录工具图标已创建:")
    print("   - flash_tool_icon.ico (多尺寸ICO)")
    print("   - flash_tool_icon.png (PNG预览)")


if __name__ == '__main__':
    print("[START] 开始创建固件烧录工具图标...")
    print("   设计风格：高端大气上档次")
    print("   主题：Ti芯片 + 闪电烧录 + 科技光环")

    # 创建256x256的基础图标
    icon = create_flash_icon(256)

    # 保存不同尺寸
    save_icon_sizes(icon)

    print("[DONE] 图标创建完成！")
