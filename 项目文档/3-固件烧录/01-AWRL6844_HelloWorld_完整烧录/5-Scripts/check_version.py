#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
版本检查工具 - 确保主APP和子模块版本一致性

使用方法：
    python check_version.py          # 检查版本
    python check_version.py --fix    # 自动修复版本不一致
"""

import re
import sys
from pathlib import Path

def extract_version(file_path, pattern):
    """从文件中提取版本号"""
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
            match = re.search(pattern, content)
            if match:
                return match.group(1)
    except Exception as e:
        print(f"❌ 读取失败 {file_path}: {e}")
    return None

def update_version(file_path, pattern, new_version):
    """更新文件中的版本号"""
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # 替换版本号，保持格式
        updated_content = re.sub(
            r'(tab_firmware_lib\.py - 固件库标签页模块 v)[0-9.]+( \(.*?\))',
            rf'\g<1>{new_version}\g<2>',
            content
        )
        
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(updated_content)
        
        return True
    except Exception as e:
        print(f"❌ 更新失败 {file_path}: {e}")
        return False

def main():
    # 文件路径
    base_dir = Path(__file__).parent
    main_app = base_dir / "flash_tool.py"
    firmware_lib = base_dir / "tabs" / "tab_firmware_lib.py"
    
    # 版本号模式
    main_pattern = r'VERSION\s*=\s*"([0-9.]+)"'
    lib_pattern = r'tab_firmware_lib\.py - 固件库标签页模块 v([0-9.]+) \(.*?\)'
    
    # 提取版本号
    main_version = extract_version(main_app, main_pattern)
    lib_version = extract_version(firmware_lib, lib_pattern)
    
    print("=" * 70)
    print("📋 版本检查报告")
    print("=" * 70)
    print(f"主应用 (flash_tool.py):        v{main_version}")
    print(f"固件库模块 (tab_firmware_lib.py): v{lib_version}")
    print("-" * 70)
    
    if main_version == lib_version:
        print("✅ 版本一致！")
        return 0
    else:
        print("⚠️  版本不一致！")
        
        # 检查是否需要自动修复
        if "--fix" in sys.argv:
            print(f"\n🔧 正在同步版本到 v{main_version}...")
            
            if update_version(firmware_lib, lib_pattern, main_version):
                print(f"✅ 已更新 tab_firmware_lib.py 到 v{main_version}")
                return 0
            else:
                print("❌ 更新失败")
                return 1
        else:
            print("\n💡 提示：运行 `python check_version.py --fix` 自动修复")
            return 1

if __name__ == "__main__":
    sys.exit(main())
