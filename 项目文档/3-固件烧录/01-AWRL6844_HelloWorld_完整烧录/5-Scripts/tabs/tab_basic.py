#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
tab_basic.py - 基本烧录标签页
版本: v1.0.8
作者: Benson@Wisefido

⚠️ 此模块不能单独运行，必须从 flash_tool.py 主入口启动！
"""

import tkinter as tk
from tkinter import ttk, scrolledtext, filedialog, messagebox
from pathlib import Path

class BasicTab:
    """基本烧录标签页类"""
    
    def __init__(self, parent_frame, app):
        """
        初始化基本烧录标签页
        
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
        print("⚠️  错误：tab_basic 模块不能单独运行！")
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
        
        # 主容器 - 两列布局
        left_col = tk.Frame(self.frame, bg="#ecf0f1")
        left_col.pack(side=tk.LEFT, fill=tk.BOTH, expand=False, padx=(10, 5), pady=10)
        
        right_col = tk.Frame(self.frame, bg="#ecf0f1")
        right_col.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True, padx=(5, 10), pady=10)
        
        # ============= 左列：固件文件 + 端口设置 + 烧录按钮 =============
        
        # --- 固件文件状态 ---
        firmware_frame = tk.LabelFrame(
            left_col,
            text="📦 固件文件",
            font=("Microsoft YaHei UI", 10, "bold"),
            bg="#ecf0f1",
            fg="#2c3e50",
            padx=10,
            pady=10
        )
        firmware_frame.pack(fill=tk.X, pady=(0, 10))
        
        # SBL固件标签
        tk.Label(
            firmware_frame,
            text="SBL固件:",
            font=("Microsoft YaHei UI", 9, "bold"),
            bg="#ecf0f1"
        ).grid(row=0, column=0, sticky=tk.W, pady=2)
        
        self.app.sbl_status_label = tk.Label(
            firmware_frame,
            text="❌ 未找到",
            font=("Microsoft YaHei UI", 9),
            bg="#ecf0f1",
            fg="red"
        )
        self.app.sbl_status_label.grid(row=0, column=1, columnspan=2, sticky=tk.W, pady=2, padx=(5, 0))
        
        # SBL路径显示
        self.app.sbl_path_label = tk.Label(
            firmware_frame,
            text="",
            font=("Consolas", 7),
            bg="#ecf0f1",
            fg="#7f8c8d",
            wraplength=220,
            justify=tk.LEFT
        )
        self.app.sbl_path_label.grid(row=1, column=0, columnspan=2, sticky=tk.W, pady=(0, 5), padx=(0, 5))
        
        tk.Button(
            firmware_frame,
            text="选择",
            font=("Microsoft YaHei UI", 8),
            command=self.app.select_sbl_file,
            bg="#3498db",
            fg="white",
            relief=tk.FLAT,
            padx=5,
            pady=1,
            cursor="hand2"
        ).grid(row=1, column=2, sticky=tk.E, pady=(0, 5))
        
        # App固件标签
        tk.Label(
            firmware_frame,
            text="App固件:",
            font=("Microsoft YaHei UI", 9, "bold"),
            bg="#ecf0f1"
        ).grid(row=2, column=0, sticky=tk.W, pady=2)
        
        self.app.app_status_label = tk.Label(
            firmware_frame,
            text="❌ 未找到",
            font=("Microsoft YaHei UI", 9),
            bg="#ecf0f1",
            fg="red"
        )
        self.app.app_status_label.grid(row=2, column=1, columnspan=2, sticky=tk.W, pady=2, padx=(5, 0))
        
        # App路径显示
        self.app.app_path_label = tk.Label(
            firmware_frame,
            text="",
            font=("Consolas", 7),
            bg="#ecf0f1",
            fg="#7f8c8d",
            wraplength=220,
            justify=tk.LEFT
        )
        self.app.app_path_label.grid(row=3, column=0, columnspan=2, sticky=tk.W, pady=(0, 5), padx=(0, 5))
        
        tk.Button(
            firmware_frame,
            text="选择",
            font=("Microsoft YaHei UI", 8),
            command=self.app.select_app_file,
            bg="#3498db",
            fg="white",
            relief=tk.FLAT,
            padx=5,
            pady=1,
            cursor="hand2"
        ).grid(row=3, column=2, sticky=tk.E, pady=(0, 5))
        
        # 工具标签
        tk.Label(
            firmware_frame,
            text="烧录工具:",
            font=("Microsoft YaHei UI", 9, "bold"),
            bg="#ecf0f1"
        ).grid(row=4, column=0, sticky=tk.W, pady=(5, 2))
        
        self.app.tool_status_label = tk.Label(
            firmware_frame,
            text="❌ 未找到",
            font=("Microsoft YaHei UI", 9),
            bg="#ecf0f1",
            fg="red"
        )
        self.app.tool_status_label.grid(row=4, column=1, columnspan=2, sticky=tk.W, pady=(5, 2), padx=(5, 0))
        
        # 按钮区域
        button_container = tk.Frame(firmware_frame, bg="#ecf0f1")
        button_container.grid(row=5, column=0, columnspan=3, pady=(10, 0), sticky=tk.EW)
        
        # 打开目录按钮
        tk.Button(
            button_container,
            text="📂 打开目录",
            font=("Microsoft YaHei UI", 8),
            command=self.app.open_firmware_folder,
            bg="#16a085",
            fg="white",
            relief=tk.FLAT,
            padx=8,
            pady=4,
            cursor="hand2"
        ).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 3))
        
        # 分析固件按钮
        tk.Button(
            button_container,
            text="🔍 分析",
            font=("Microsoft YaHei UI", 8),
            command=self.app.analyze_firmware,
            bg="#3498db",
            fg="white",
            relief=tk.FLAT,
            padx=8,
            pady=4,
            cursor="hand2"
        ).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(3, 0))
        
        # --- 端口设置 ---
        port_frame = tk.LabelFrame(
            left_col,
            text="🔌 端口设置",
            font=("Microsoft YaHei UI", 10, "bold"),
            bg="#ecf0f1",
            fg="#2c3e50",
            padx=10,
            pady=10
        )
        port_frame.pack(fill=tk.X, pady=(0, 10))
        
        # 烧录端口（COM3）
        tk.Label(
            port_frame,
            text="烧录端口(COM3):",
            font=("Microsoft YaHei UI", 9),
            bg="#ecf0f1"
        ).grid(row=0, column=0, sticky=tk.W, pady=5)
        
        self.app.flash_port_combo = ttk.Combobox(
            port_frame,
            width=10,
            state="readonly",
            font=("Consolas", 9)
        )
        self.app.flash_port_combo.grid(row=0, column=1, sticky=tk.W, pady=5, padx=(5, 0))
        self.app.flash_port_combo.set("COM3")
        
        # 调试端口（COM4）
        tk.Label(
            port_frame,
            text="调试端口(COM4):",
            font=("Microsoft YaHei UI", 9),
            bg="#ecf0f1"
        ).grid(row=1, column=0, sticky=tk.W, pady=5)
        
        self.app.debug_port_combo = ttk.Combobox(
            port_frame,
            width=10,
            state="readonly",
            font=("Consolas", 9)
        )
        self.app.debug_port_combo.grid(row=1, column=1, sticky=tk.W, pady=5, padx=(5, 0))
        self.app.debug_port_combo.set("COM4")
        
        # 刷新按钮 + 测试按钮
        button_frame = tk.Frame(port_frame, bg="#ecf0f1")
        button_frame.grid(row=2, column=0, columnspan=2, pady=(5, 0), sticky=tk.EW)
        
        tk.Button(
            button_frame,
            text="🔄 刷新端口",
            font=("Microsoft YaHei UI", 8),
            command=self.app.refresh_com_ports,
            bg="#3498db",
            fg="white",
            relief=tk.FLAT,
            padx=8,
            pady=4,
            cursor="hand2"
        ).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 3))
        
        tk.Button(
            button_frame,
            text="🔍 测试COM3",
            font=("Microsoft YaHei UI", 8),
            command=lambda: self.app.test_port(self.app.flash_port_combo.get()),
            bg="#27ae60",
            fg="white",
            relief=tk.FLAT,
            padx=8,
            pady=4,
            cursor="hand2"
        ).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(3, 0))
        
        # --- 完整烧录按钮（大按钮） ---
        flash_button_frame = tk.Frame(left_col, bg="#ecf0f1")
        flash_button_frame.pack(fill=tk.X, pady=10)
        
        tk.Button(
            flash_button_frame,
            text="🚀 完整烧录 (SBL + App)",
            font=("Microsoft YaHei UI", 12, "bold"),
            command=self.app.flash_firmware,
            bg="#27ae60",
            fg="white",
            relief=tk.FLAT,
            padx=20,
            pady=15,
            cursor="hand2",
            activebackground="#229954"
        ).pack(fill=tk.X)
        
        # ============= 右列：日志输出 =============
        
        # 日志标题
        tk.Label(
            right_col,
            text="📋 烧录日志",
            font=("Microsoft YaHei UI", 12, "bold"),
            bg="#ecf0f1",
            fg="#2c3e50"
        ).pack(pady=(0, 10))
        
        # 日志框架
        log_frame = tk.Frame(right_col, bg="#ecf0f1")
        log_frame.pack(fill=tk.BOTH, expand=True)
        
        # 日志文本框
        self.app.log_text = scrolledtext.ScrolledText(
            log_frame,
            font=("Consolas", 9),
            bg="#2c3e50",
            fg="#ecf0f1",
            insertbackground="white",
            wrap=tk.WORD,
            state=tk.DISABLED
        )
        self.app.log_text.pack(fill=tk.BOTH, expand=True)
        
        # 配置日志颜色标签
        self.app.log_text.tag_config("INFO", foreground="#3498db")
        self.app.log_text.tag_config("SUCCESS", foreground="#27ae60")
        self.app.log_text.tag_config("WARN", foreground="#f39c12")
        self.app.log_text.tag_config("ERROR", foreground="#e74c3c")
        
        # 清除日志按钮
        tk.Button(
            log_frame,
            text="🗑️ 清除日志",
            font=("Microsoft YaHei UI", 9),
            command=self.app.clear_log,
            bg="#95a5a6",
            fg="white",
            relief=tk.FLAT,
            padx=10,
            pady=4,
            cursor="hand2"
        ).pack(pady=(5, 0))
    
    def update_port_list(self, sbl_ports, app_ports):
        """更新端口列表"""
        # TODO: 更新端口下拉框
        pass
    
    def log(self, message, tag=None):
        """添加日志消息"""
        if hasattr(self.app, 'log_text'):
            self.app.log_text.config(state=tk.NORMAL)
            if tag:
                self.app.log_text.insert(tk.END, message, tag)
            else:
                self.app.log_text.insert(tk.END, message)
            self.app.log_text.see(tk.END)
            self.app.log_text.config(state=tk.DISABLED)
    
    def clear_log(self):
        """清空日志"""
        if hasattr(self.app, 'log_text'):
            self.app.log_text.config(state=tk.NORMAL)
            self.app.log_text.delete(1.0, tk.END)
            self.app.log_text.config(state=tk.DISABLED)


# 如果直接运行此文件，显示错误提示
if __name__ == "__main__":
    import sys
    print("=" * 70)
    print("⚠️  错误：tab_basic.py 不能单独运行！")
    print("=" * 70)
    print()
    print("请从主入口启动烧录工具：")
    print()
    print("  cd 5-Scripts")
    print("  python flash_tool.py")
    print()
    print("=" * 70)
    sys.exit(1)
