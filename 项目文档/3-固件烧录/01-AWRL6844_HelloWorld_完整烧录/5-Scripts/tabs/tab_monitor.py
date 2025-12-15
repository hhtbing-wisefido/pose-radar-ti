#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
tab_monitor.py - 串口监视标签页
版本: v1.0.8
作者: Benson@Wisefido

⚠️ 此模块不能单独运行，必须从 flash_tool.py 主入口启动！
"""

import tkinter as tk
from tkinter import ttk

class MonitorTab:
    """串口监视标签页类"""
    
    def __init__(self, parent_frame, app):
        """
        初始化串口监视标签页
        
        Args:
            parent_frame: 父容器（tk.Frame）
            app: 主应用实例（FlashToolGUI）
        """
        self.frame = parent_frame
        self.app = app
        
        # 检查是否是通过主入口启动
        if not hasattr(app, 'VERSION'):
            self._show_error_and_exit()
        
        # 创建界面
        self.create_ui()
    
    def _show_error_and_exit(self):
        """显示错误并退出"""
        import sys
        print("=" * 70)
        print("⚠️  错误：tab_monitor 模块不能单独运行！")
        print("=" * 70)
        print()
        print("请从主入口启动烧录工具：")
        print()
        print("  cd 5-Scripts")
        print("  python flash_tool.py")
        print()
        print("=" * 70)
        sys.exit(1)
    
    def create_ui(self):
        """创建标签页UI"""
        self.frame.configure(bg="#ecf0f1")
        
        # 标题
        tk.Label(
            self.frame,
            text="📡 串口监视器",
            font=("Microsoft YaHei UI", 14, "bold"),
            bg="#ecf0f1",
            fg="#2c3e50"
        ).pack(pady=(10, 15))
        
        # 说明
        tk.Label(
            self.frame,
            text="实时查看串口输出，用于调试和监控设备状态",
            font=("Microsoft YaHei UI", 9),
            bg="#ecf0f1",
            fg="#7f8c8d"
        ).pack(pady=(0, 20))
        
        # 主容器 - 两列布局
        monitor_container = tk.Frame(self.frame, bg="#ecf0f1")
        monitor_container.pack(fill=tk.BOTH, expand=True, padx=20, pady=(0, 20))
        
        # ============= COM3监视器（左列） =============
        com3_frame = tk.LabelFrame(
            monitor_container,
            text="📟 COM3 - 烧录/调试端口",
            font=("Microsoft YaHei UI", 11, "bold"),
            bg="#ffffff",
            fg="#27ae60",
            relief=tk.GROOVE,
            bd=2
        )
        com3_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(0, 10))
        
        # COM3控制按钮
        com3_control = tk.Frame(com3_frame, bg="#ffffff")
        com3_control.pack(fill=tk.X, padx=10, pady=10)
        
        tk.Button(
            com3_control,
            text="▶️ 打开COM3监视器",
            font=("Microsoft YaHei UI", 10),
            command=lambda: self.app.open_serial_monitor("COM3"),
            bg="#27ae60",
            fg="white",
            relief=tk.FLAT,
            padx=15,
            pady=8,
            cursor="hand2"
        ).pack(side=tk.LEFT, padx=(0, 5))
        
        tk.Button(
            com3_control,
            text="🔄 测试COM3",
            font=("Microsoft YaHei UI", 10),
            command=lambda: self.app.test_port("COM3"),
            bg="#3498db",
            fg="white",
            relief=tk.FLAT,
            padx=15,
            pady=8,
            cursor="hand2"
        ).pack(side=tk.LEFT)
        
        # COM3说明
        com3_info = tk.Text(
            com3_frame,
            font=("Microsoft YaHei UI", 9),
            bg="#f8f9fa",
            fg="#2c3e50",
            wrap=tk.WORD,
            relief=tk.FLAT,
            height=12,
            padx=10,
            pady=10
        )
        com3_info.pack(fill=tk.BOTH, expand=True, padx=10, pady=(0, 10))
        
        com3_content = """📌 COM3端口说明

用途：
• 固件烧录（主要功能）
• 应用程序调试输出
• 命令行交互（CLI）

特点：
• SBL启动后自动激活
• 用于arprog烧录通信
• 可以看到App的printf输出

监视内容：
• 设备启动日志
• 应用程序调试信息
• 命令行输入输出
• 错误和警告信息
"""
        com3_info.insert(tk.END, com3_content)
        com3_info.config(state=tk.DISABLED)
        
        # ============= COM4监视器（右列） =============
        com4_frame = tk.LabelFrame(
            monitor_container,
            text="📟 COM4 - 数据输出端口",
            font=("Microsoft YaHei UI", 11, "bold"),
            bg="#ffffff",
            fg="#e67e22",
            relief=tk.GROOVE,
            bd=2
        )
        com4_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True, padx=(10, 0))
        
        # COM4控制按钮
        com4_control = tk.Frame(com4_frame, bg="#ffffff")
        com4_control.pack(fill=tk.X, padx=10, pady=10)
        
        tk.Button(
            com4_control,
            text="▶️ 打开COM4监视器",
            font=("Microsoft YaHei UI", 10),
            command=lambda: self.app.open_serial_monitor("COM4"),
            bg="#e67e22",
            fg="white",
            relief=tk.FLAT,
            padx=15,
            pady=8,
            cursor="hand2"
        ).pack(side=tk.LEFT, padx=(0, 5))
        
        tk.Button(
            com4_control,
            text="🔄 测试COM4",
            font=("Microsoft YaHei UI", 10),
            command=lambda: self.app.test_port("COM4"),
            bg="#3498db",
            fg="white",
            relief=tk.FLAT,
            padx=15,
            pady=8,
            cursor="hand2"
        ).pack(side=tk.LEFT)
        
        # COM4说明
        com4_info = tk.Text(
            com4_frame,
            font=("Microsoft YaHei UI", 9),
            bg="#f8f9fa",
            fg="#2c3e50",
            wrap=tk.WORD,
            relief=tk.FLAT,
            height=12,
            padx=10,
            pady=10
        )
        com4_info.pack(fill=tk.BOTH, expand=True, padx=10, pady=(0, 10))
        
        com4_content = """📌 COM4端口说明

用途：
• 数据输出端口
• 日志记录
• 性能监控

特点：
• 独立于烧录端口
• 持续输出运行数据
• 不影响烧录过程

监视内容：
• 实时数据流
• 系统运行日志
• 性能统计信息
• 传感器原始数据
"""
        com4_info.insert(tk.END, com4_content)
        com4_info.config(state=tk.DISABLED)
        
        # 底部提示
        tk.Label(
            self.frame,
            text="💡 提示：监视器在独立窗口打开，可以同时监视多个端口",
            font=("Microsoft YaHei UI", 9),
            bg="#ecf0f1",
            fg="#7f8c8d"
        ).pack(pady=(0, 10))


# 如果直接运行此文件，显示错误提示
if __name__ == "__main__":
    import sys
    print("=" * 70)
    print("⚠️  错误：tab_monitor.py 不能单独运行！")
    print("=" * 70)
    print()
    print("请从主入口启动烧录工具：")
    print()
    print("  cd 5-Scripts")
    print("  python flash_tool.py")
    print()
    print("=" * 70)
    sys.exit(1)
