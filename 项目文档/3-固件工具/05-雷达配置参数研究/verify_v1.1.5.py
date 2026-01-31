# v1.1.5 编码处理验证脚本

"""
验证v1.1.5的中文注释编码处理功能
"""

print("=" * 60)
print("radar_test_gui.py v1.1.5 编码处理验证")
print("=" * 60)

# 测试编码检测逻辑
test_strings = [
    ("% Configuration: Fall Detection", True, "纯英文"),
    ("% 配置说明：跌倒检测", False, "纯中文"),
    ("% Config: Test 配置", False, "中英混合"),
    ("% Range: 0-6m", True, "英文+数字"),
    ("% 创建时间: 2025-12-24", False, "中文+英文+数字"),
    ("% Scene: Office", True, "纯英文"),
]

print("\nOK: 测试编码检测逻辑")
print("-" * 60)

for text, should_pass, desc in test_strings:
    try:
        text.encode('ascii')
        result = "✓ 可发送"
        passed = should_pass
    except UnicodeEncodeError:
        result = "✗ 跳过"
        passed = not should_pass

    status = "OK" if passed else "FAIL"
    print(f"{status} {desc:15s} | {text:40s} | {result}")

print("\nOK: 验证代码改进")
print("-" * 60)

import re

with open('radar_test_gui.py', 'r', encoding='utf-8') as f:
    content = f.read()

# 检查1: 版本号
if 'v1.1.5' in content:
    print("✓ 版本号已更新为 v1.1.5")
else:
    print("✗ 版本号未更新")

# 检查2: send_command中的嵌套try-except
if 'except UnicodeEncodeError:' in content:
    print("✓ 已添加UnicodeEncodeError处理")
else:
    print("✗ 未添加异常处理")

# 检查3: 跳过计数
if 'skipped_count' in content:
    print("✓ 已添加跳过统计")
else:
    print("✗ 未添加跳过统计")

# 检查4: 警告提示
if 'CLI仅支持ASCII' in content or 'contains non-ASCII' in content:
    print("✓ 已添加编码警告提示")
else:
    print("✗ 未添加警告提示")

# 检查5: 默认模板改为英文
if '% Configuration:' in content and '% 配置说明：' not in content.split('config_comment_text.insert')[1].split('\n')[0]:
    print("✓ 默认模板已改为英文")
else:
    print("✗ 默认模板未改为英文")

print("\n" + "=" * 60)
print("功能验证完成")
print("=" * 60)

print("\nTIP: 测试建议：")
print("1. 运行GUI程序")
print("2. 在注释框输入中文注释")
print("3. 点击'发送配置执行'")
print("4. 观察CLI输出区是否显示跳过提示")
print("5. 验证配置命令是否正常发送")

print("\nINFO: 推荐注释格式：")
print("   % Configuration: [场景描述]")
print("   % Created: [日期]")
print("   % Scene: [应用场景]")
print("   % Author: [作者]")
