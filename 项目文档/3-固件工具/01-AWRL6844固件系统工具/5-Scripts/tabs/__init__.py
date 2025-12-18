#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
tabs 模块 - TI毫米波雷达烧录工具标签页
版本: v1.3.4

此模块包含所有标签页的实现：
- tab_basic: 基本烧录标签页
- tab_advanced: 高级功能标签页
- tab_firmware_manager: 固件管理标签页 (v1.3.4新增)
- tab_monitor: 串口监视标签页
- tab_ports: 端口管理标签页

⚠️ 注意：这些模块不能单独运行，必须从 flash_tool.py 主入口启动
"""

__version__ = "v1.3.4"
__author__ = "Benson@Wisefido"

# 导入所有标签页类
try:
    from .tab_basic import BasicTab
    from .tab_advanced import AdvancedTab
    from .tab_firmware_manager import FirmwareManagerTab
    from .tab_monitor import MonitorTab
    from .tab_ports import PortsTab
    
    __all__ = ['BasicTab', 'AdvancedTab', 'FirmwareManagerTab', 'MonitorTab', 'PortsTab']
except ImportError as e:
    # 如果直接运行此模块，给出友好提示
    import sys
    print("=" * 70)
    print("⚠️  错误：tabs 模块不能单独运行！")
    print("=" * 70)
    print()
    print("请从主入口启动烧录工具：")
    print()
    print("  cd 5-Scripts")
    print("  python flash_tool.py")
    print()
    print("=" * 70)
    sys.exit(1)
