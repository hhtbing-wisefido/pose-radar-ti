"""
创建雷达图标
"""
from PIL import Image, ImageDraw

# 创建256x256图像，深蓝色背景
img = Image.new('RGB', (256, 256), (0, 30, 60))
draw = ImageDraw.Draw(img)

# 外圆环（蓝色边框）
draw.ellipse([28, 28, 228, 228], fill=(0, 100, 180), outline=(0, 200, 255), width=8)

# 雷达扫描弧（绿色，下半圆）
draw.arc([48, 48, 208, 208], 180, 360, fill=(0, 255, 128), width=12)

# 中心点（红色）
draw.ellipse([118, 118, 138, 138], fill=(255, 50, 50))

# 雷达扫描线（黄色）
draw.line([128, 128, 200, 80], fill=(255, 200, 0), width=6)
draw.line([128, 128, 200, 176], fill=(255, 200, 0), width=6)
draw.line([128, 128, 56, 128], fill=(255, 200, 0), width=6)

# 添加目标点（白色小点）
draw.ellipse([180, 100, 190, 110], fill=(255, 255, 255))
draw.ellipse([160, 150, 170, 160], fill=(255, 255, 255))
draw.ellipse([90, 140, 100, 150], fill=(255, 255, 255))

# 保存PNG
img.save('radar_icon.png')
print('✅ 雷达图标已创建: radar_icon.png')

# 转换为ICO（多尺寸）
try:
    img_ico = Image.new('RGB', (256, 256), (0, 30, 60))
    draw_ico = ImageDraw.Draw(img_ico)
    draw_ico.ellipse([28, 28, 228, 228], fill=(0, 100, 180), outline=(0, 200, 255), width=8)
    draw_ico.arc([48, 48, 208, 208], 180, 360, fill=(0, 255, 128), width=12)
    draw_ico.ellipse([118, 118, 138, 138], fill=(255, 50, 50))
    draw_ico.line([128, 128, 200, 80], fill=(255, 200, 0), width=6)
    draw_ico.line([128, 128, 200, 176], fill=(255, 200, 0), width=6)
    draw_ico.line([128, 128, 56, 128], fill=(255, 200, 0), width=6)
    draw_ico.ellipse([180, 100, 190, 110], fill=(255, 255, 255))
    draw_ico.ellipse([160, 150, 170, 160], fill=(255, 255, 255))
    draw_ico.ellipse([90, 140, 100, 150], fill=(255, 255, 255))
    
    img_ico.save('radar_icon.ico', sizes=[(16,16), (32,32), (48,48), (64,64), (128,128), (256,256)])
    print('✅ ICO图标已创建: radar_icon.ico')
except Exception as e:
    print(f'⚠️ ICO创建失败: {e}')
