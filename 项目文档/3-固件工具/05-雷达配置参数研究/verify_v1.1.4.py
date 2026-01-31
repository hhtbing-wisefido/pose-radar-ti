# v1.1.4 功能验证测试脚本

"""
验证v1.1.4的两个关键修复：
1. 分类折叠功能
2. 配置命令区行号显示
"""

print("=" * 60)
print("radar_test_gui.py v1.1.4 功能验证")
print("=" * 60)

import ast
import re

# 读取源文件
with open('radar_test_gui.py', 'r', encoding='utf-8') as f:
    content = f.read()

print("\nOK: 验证1: 版本号更新")
if 'v1.1.4' in content:
    print("   ✓ 版本号已更新为 v1.1.4")
else:
    print("   ✗ 版本号未更新")

print("\nOK: 验证2: 分类折叠功能实现")

# 检查数据结构是否改进
if "'state': collapsed_var" in content and "'frame': cmd_list_frame" in content:
    print("   ✓ 折叠数据结构已改进（存储state, frame, button）")
else:
    print("   ✗ 折叠数据结构未改进")

# 检查toggle_category是否完整实现
toggle_match = re.search(r'def toggle_category\(self, category\):.*?(?=\n    def )', content, re.DOTALL)
if toggle_match:
    toggle_code = toggle_match.group(0)
    if 'pack_forget()' in toggle_code and '.config(text=' in toggle_code:
        print("   ✓ toggle_category完整实现（包含pack_forget和按钮图标切换）")
    else:
        print("   ✗ toggle_category未完整实现")
else:
    print("   ✗ 未找到toggle_category函数")

print("\nOK: 验证3: 行号显示功能")

# 检查line_numbers组件是否创建
if 'self.line_numbers = tk.Text' in content:
    print("   ✓ 行号Text组件已创建")
else:
    print("   ✗ 行号组件未创建")

# 检查update_line_numbers方法
if 'def update_line_numbers(self, event=None):' in content:
    print("   ✓ update_line_numbers方法已添加")
else:
    print("   ✗ update_line_numbers方法未添加")

# 检查_sync_scroll方法
if 'def _sync_scroll(self, *args):' in content:
    print("   ✓ _sync_scroll滚动同步方法已添加")
else:
    print("   ✗ _sync_scroll方法未添加")

# 检查事件绑定
if "<KeyRelease>" in content and "update_line_numbers" in content:
    print("   ✓ 行号更新事件已绑定")
else:
    print("   ✗ 行号更新事件未绑定")

print("\nOK: 验证4: 导出功能行号处理增强")

export_match = re.search(r'def export_config_file\(self\):.*?(?=\n    def )', content, re.DOTALL)
if export_match:
    export_code = export_match.group(0)
    if r'^\s*\d+\s+(.+)$' in export_code:
        print("   ✓ 导出时行号移除逻辑已增强")
    else:
        print("   ✗ 导出行号移除逻辑未优化")
else:
    print("   ✗ 未找到export_config_file函数")

print("\n" + "=" * 60)
print("功能验证完成")
print("=" * 60)

print("\nTIP: 测试建议：")
print("1. 运行GUI程序")
print("2. 测试点击分类折叠按钮（▼ 应变为 ▶）")
print("3. 观察配置命令区左侧是否显示灰色行号")
print("4. 测试导出配置文件，检查是否移除了行号")
