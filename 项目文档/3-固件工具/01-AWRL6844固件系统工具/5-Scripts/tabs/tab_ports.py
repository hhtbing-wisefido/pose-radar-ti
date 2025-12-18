#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
tab_ports.py - 端口管理标签页
版本: v1.0.8
作者: Benson@Wisefido

⚠️ 此模块不能单独运行，必须从 flash_tool.py 主入口启动！
"""

import tkinter as tk
from tkinter import ttk

class PortsTab:
    """端口管理标签页类"""
    
    def __init__(self, parent_frame, app):
        """
        初始化端口管理标签页
        
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
        print("⚠️  错误：tab_ports 模块不能单独运行！")
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
        # ttk.Frame不支持bg参数，使用默认主题
        
        # 标题
        tk.Label(
            self.frame,
            text="🔌 端口管理工具",
            font=("Microsoft YaHei UI", 14, "bold"),
            bg="#ecf0f1",
            fg="#2c3e50"
        ).pack(pady=(10, 15))
        
        # 说明
        tk.Label(
            self.frame,
            text="扫描、测试、释放COM端口，解决端口占用问题",
            font=("Microsoft YaHei UI", 9),
            bg="#ecf0f1",
            fg="#7f8c8d"
        ).pack(pady=(0, 20))
        
        # 主容器
        main_container = tk.Frame(self.frame, bg="#ecf0f1")
        main_container.pack(fill=tk.BOTH, expand=True, padx=40, pady=(0, 20))
        
        # ============= 端口扫描 =============
        scan_frame = tk.LabelFrame(
            main_container,
            text="🔍 端口扫描",
            font=("Microsoft YaHei UI", 12, "bold"),
            bg="#ffffff",
            fg="#3498db",
            relief=tk.GROOVE,
            bd=2
        )
        scan_frame.pack(fill=tk.X, pady=(0, 15))
        
        scan_control = tk.Frame(scan_frame, bg="#ffffff")
        scan_control.pack(fill=tk.X, padx=15, pady=15)
        
        tk.Label(
            scan_control,
            text="扫描系统中所有可用的COM端口：",
            font=("Microsoft YaHei UI", 10),
            bg="#ffffff",
            fg="#2c3e50"
        ).pack(side=tk.LEFT, padx=(0, 15))
        
        tk.Button(
            scan_control,
            text="🔄 扫描COM端口",
            font=("Microsoft YaHei UI", 10, "bold"),
            command=self.app.refresh_com_ports,
            bg="#3498db",
            fg="white",
            relief=tk.FLAT,
            padx=20,
            pady=8,
            cursor="hand2"
        ).pack(side=tk.LEFT)
        
        # ============= 端口测试 =============
        test_frame = tk.LabelFrame(
            main_container,
            text="✅ 端口测试",
            font=("Microsoft YaHei UI", 12, "bold"),
            bg="#ffffff",
            fg="#27ae60",
            relief=tk.GROOVE,
            bd=2
        )
        test_frame.pack(fill=tk.X, pady=(0, 15))
        
        test_control = tk.Frame(test_frame, bg="#ffffff")
        test_control.pack(fill=tk.X, padx=15, pady=15)
        
        tk.Label(
            test_control,
            text="选择端口：",
            font=("Microsoft YaHei UI", 10),
            bg="#ffffff",
            fg="#2c3e50"
        ).pack(side=tk.LEFT, padx=(0, 10))
        
        self.test_port_combo = ttk.Combobox(
            test_control,
            values=["COM3", "COM4", "COM5", "COM6"],
            state="readonly",
            width=12,
            font=("Microsoft YaHei UI", 10)
        )
        self.test_port_combo.set("COM3")
        self.test_port_combo.pack(side=tk.LEFT, padx=(0, 15))
        
        tk.Button(
            test_control,
            text="🔍 测试端口",
            font=("Microsoft YaHei UI", 10, "bold"),
            command=lambda: self.app.test_port(self.test_port_combo.get()),
            bg="#27ae60",
            fg="white",
            relief=tk.FLAT,
            padx=20,
            pady=8,
            cursor="hand2"
        ).pack(side=tk.LEFT)
        
        tk.Label(
            test_frame,
            text="💡 测试端口是否可用，检查是否被其他程序占用",
            font=("Microsoft YaHei UI", 9),
            bg="#ffffff",
            fg="#7f8c8d"
        ).pack(padx=15, pady=(0, 15))
        
        # ============= 端口释放 =============
        release_frame = tk.LabelFrame(
            main_container,
            text="🔓 端口释放",
            font=("Microsoft YaHei UI", 12, "bold"),
            bg="#ffffff",
            fg="#e67e22",
            relief=tk.GROOVE,
            bd=2
        )
        release_frame.pack(fill=tk.X, pady=(0, 15))
        
        release_control = tk.Frame(release_frame, bg="#ffffff")
        release_control.pack(fill=tk.X, padx=15, pady=15)
        
        tk.Label(
            release_control,
            text="选择端口：",
            font=("Microsoft YaHei UI", 10),
            bg="#ffffff",
            fg="#2c3e50"
        ).pack(side=tk.LEFT, padx=(0, 10))
        
        self.release_port_combo = ttk.Combobox(
            release_control,
            values=["COM3", "COM4", "COM5", "COM6"],
            state="readonly",
            width=12,
            font=("Microsoft YaHei UI", 10)
        )
        self.release_port_combo.set("COM3")
        self.release_port_combo.pack(side=tk.LEFT, padx=(0, 15))
        
        tk.Button(
            release_control,
            text="🔓 释放端口",
            font=("Microsoft YaHei UI", 10, "bold"),
            command=lambda: self.app.release_port(self.release_port_combo.get()),
            bg="#e67e22",
            fg="white",
            relief=tk.FLAT,
            padx=20,
            pady=8,
            cursor="hand2"
        ).pack(side=tk.LEFT)
        
        tk.Label(
            release_frame,
            text="⚠️ 释放被占用的端口，会终止占用该端口的进程（需要确认）",
            font=("Microsoft YaHei UI", 9),
            bg="#ffffff",
            fg="#7f8c8d"
        ).pack(padx=15, pady=(0, 15))
        
        # ============= 常见问题 =============
        faq_frame = tk.LabelFrame(
            main_container,
            text="❓ 常见问题",
            font=("Microsoft YaHei UI", 12, "bold"),
            bg="#ffffff",
            fg="#9b59b6",
            relief=tk.GROOVE,
            bd=2
        )
        faq_frame.pack(fill=tk.BOTH, expand=True)
        
        faq_text = tk.Text(
            faq_frame,
            font=("Microsoft YaHei UI", 9),
            bg="#f8f9fa",
            fg="#2c3e50",
            wrap=tk.WORD,
            relief=tk.FLAT,
            height=10,
            padx=15,
            pady=10
        )
        faq_text.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        faq_content = """🔴 端口被占用怎么办？
→ 先用"测试端口"确认，再用"释放端口"功能

🔴 找不到COM端口？
→ 检查USB线是否连接，驱动是否安装
→ 点击"扫描COM端口"刷新端口列表

🔴 释放端口后仍然无法使用？
→ 重新插拔USB线
→ 重启设备
→ 检查是否有多个程序同时占用

🔴 烧录时提示端口打开失败？
→ 在"端口管理"标签页释放端口
→ 确保没有其他串口工具打开该端口
→ 关闭串口监视器后再烧录

💡 最佳实践：
• 烧录前先关闭所有串口监视器
• 使用完毕后及时关闭端口
• 定期扫描端口确保设备连接正常
"""
        faq_text.insert(tk.END, faq_content)
        faq_text.config(state=tk.DISABLED)


# 如果直接运行此文件，显示错误提示
if __name__ == "__main__":
    import sys
    print("=" * 70)
    print("⚠️  错误：tab_ports.py 不能单独运行！")
    print("=" * 70)
    print()
    print("请从主入口启动烧录工具：")
    print()
    print("  cd 5-Scripts")
    print("  python flash_tool.py")
    print()
    print("=" * 70)
    sys.exit(1)
