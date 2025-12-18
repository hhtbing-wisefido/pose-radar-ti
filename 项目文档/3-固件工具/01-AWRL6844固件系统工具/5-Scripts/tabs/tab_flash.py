#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
tab_flash.py - 烧录功能标签页（整合版）
版本: v1.5.0
作者: Benson@Wisefido

整合了原来的基本烧录、高级功能、串口监视、端口管理功能

⚠️ 此模块不能单独运行，必须从 flash_tool.py 主入口启动！
"""

import tkinter as tk
from tkinter import ttk, scrolledtext, filedialog, messagebox
from pathlib import Path

class FlashTab:
    """烧录功能标签页类（整合版）"""
    
    def __init__(self, parent_frame, app):
        """
        初始化烧录功能标签页
        
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
        print("⚠️  错误：tab_flash 模块不能单独运行！")
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
        # 主容器 - 两列布局
        left_col = tk.Frame(self.frame, bg="#ecf0f1")
        left_col.pack(side=tk.LEFT, fill=tk.BOTH, expand=False, padx=(10, 5), pady=10)
        
        right_col = tk.Frame(self.frame, bg="#ecf0f1")
        right_col.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True, padx=(5, 10), pady=10)
        
        # ============= 左列：所有功能区 =============
        
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
        self.app.tool_status_label.grid(row=4, column=1, sticky=tk.W, pady=(5, 2), padx=(5, 0))
        
        # 选择工具按钮
        tk.Button(
            firmware_frame,
            text="选择",
            font=("Microsoft YaHei UI", 8),
            command=self.app.select_flash_tool,
            bg="#3498db",
            fg="white",
            relief=tk.FLAT,
            padx=5,
            pady=1,
            cursor="hand2"
        ).grid(row=4, column=2, sticky=tk.E, pady=(5, 2))
        
        # 工具路径显示
        self.app.tool_path_label = tk.Label(
            firmware_frame,
            text="",
            font=("Consolas", 7),
            bg="#ecf0f1",
            fg="#7f8c8d",
            wraplength=220,
            justify=tk.LEFT
        )
        self.app.tool_path_label.grid(row=5, column=0, columnspan=3, sticky=tk.W, pady=(0, 5))
        
        # 按钮区域
        button_container = tk.Frame(firmware_frame, bg="#ecf0f1")
        button_container.grid(row=6, column=0, columnspan=3, pady=(10, 0), sticky=tk.EW)
        
        # 分析已选固件按钮
        tk.Button(
            button_container,
            text="🔍 分析已选固件",
            font=("Microsoft YaHei UI", 8),
            command=self.app.analyze_firmware,
            bg="#3498db",
            fg="white",
            relief=tk.FLAT,
            padx=8,
            pady=4,
            cursor="hand2"
        ).pack(fill=tk.X, expand=True)
        
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
        
        # 烧录端口（COM3 - User UART）- 实测验证
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
        # 同步到主程序变量
        try:
            self.app.sbl_port.set(self.app.flash_port_combo.get())
        except Exception:
            pass
        # 选择变更时同步
        self.app.flash_port_combo.bind('<<ComboboxSelected>>', lambda e: self.app.sbl_port.set(self.app.flash_port_combo.get()))
        
        # 数据输出端口（COM4 - Auxiliary Data Port）- 实测验证
        tk.Label(
            port_frame,
            text="数据输出端口(COM4):",
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
        # 同步到主程序变量
        try:
            self.app.app_port.set(self.app.debug_port_combo.get())
        except Exception:
            pass
        # 选择变更时同步
        self.app.debug_port_combo.bind('<<ComboboxSelected>>', lambda e: self.app.app_port.set(self.app.debug_port_combo.get()))
        
        # 刷新按钮 + 测试按钮 + SBL检测按钮
        button_frame = tk.Frame(port_frame, bg="#ecf0f1")
        button_frame.grid(row=2, column=0, columnspan=2, pady=(5, 0), sticky=tk.EW)
        
        tk.Button(
            button_frame,
            text="🔄 刷新",
            font=("Microsoft YaHei UI", 8),
            command=self.app.refresh_com_ports,
            bg="#3498db",
            fg="white",
            relief=tk.FLAT,
            padx=6,
            pady=4,
            cursor="hand2"
        ).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 2))
        
        tk.Button(
            button_frame,
            text="🔍 测试",
            font=("Microsoft YaHei UI", 8),
            command=lambda: self.app.test_port(self.app.flash_port_combo.get()),
            bg="#27ae60",
            fg="white",
            relief=tk.FLAT,
            padx=6,
            pady=4,
            cursor="hand2"
        ).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(2, 2))
        
        tk.Button(
            button_frame,
            text="🔎 SBL检测",
            font=("Microsoft YaHei UI", 8),
            command=self.check_sbl,
            bg="#9b59b6",
            fg="white",
            relief=tk.FLAT,
            padx=6,
            pady=4,
            cursor="hand2"
        ).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(2, 0))
        
        # --- 烧录操作区 ---
        flash_frame = tk.LabelFrame(
            left_col,
            text="🔥 烧录操作",
            font=("Microsoft YaHei UI", 10, "bold"),
            bg="#ecf0f1",
            fg="#2c3e50",
            padx=10,
            pady=10
        )
        flash_frame.pack(fill=tk.X, pady=(0, 10))
        
        # 完整烧录按钮
        tk.Button(
            flash_frame,
            text="🚀 完整烧录 (SBL + App)",
            font=("Microsoft YaHei UI", 11, "bold"),
            command=self.app.flash_firmware,
            bg="#27ae60",
            fg="white",
            relief=tk.FLAT,
            padx=15,
            pady=10,
            cursor="hand2",
            activebackground="#229954"
        ).pack(fill=tk.X, pady=(0, 5))
        
        # 单独烧录按钮（两列）
        single_flash_frame = tk.Frame(flash_frame, bg="#ecf0f1")
        single_flash_frame.pack(fill=tk.X)
        
        tk.Button(
            single_flash_frame,
            text="🔥 仅SBL",
            font=("Microsoft YaHei UI", 9, "bold"),
            command=self.app.flash_sbl_only,
            bg="#e67e22",
            fg="white",
            relief=tk.FLAT,
            padx=8,
            pady=6,
            cursor="hand2"
        ).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 2))
        
        tk.Button(
            single_flash_frame,
            text="🔥 仅App",
            font=("Microsoft YaHei UI", 9, "bold"),
            command=self.app.flash_app_only,
            bg="#3498db",
            fg="white",
            relief=tk.FLAT,
            padx=8,
            pady=6,
            cursor="hand2"
        ).pack(side=tk.RIGHT, fill=tk.X, expand=True, padx=(2, 0))
        
        # --- 串口监视 ---
        monitor_frame = tk.LabelFrame(
            left_col,
            text="📡 串口监视",
            font=("Microsoft YaHei UI", 10, "bold"),
            bg="#ecf0f1",
            fg="#2c3e50",
            padx=10,
            pady=10
        )
        monitor_frame.pack(fill=tk.X, pady=(0, 10))
        
        # 监视按钮（两列）
        monitor_button_frame = tk.Frame(monitor_frame, bg="#ecf0f1")
        monitor_button_frame.pack(fill=tk.X)
        
        tk.Button(
            monitor_button_frame,
            text="📟 监视COM3",
            font=("Microsoft YaHei UI", 9),
            command=lambda: self.app.open_serial_monitor("COM3"),
            bg="#27ae60",
            fg="white",
            relief=tk.FLAT,
            padx=8,
            pady=6,
            cursor="hand2"
        ).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 2))
        
        tk.Button(
            monitor_button_frame,
            text="📟 监视COM4",
            font=("Microsoft YaHei UI", 9),
            command=lambda: self.app.open_serial_monitor("COM4"),
            bg="#e67e22",
            fg="white",
            relief=tk.FLAT,
            padx=8,
            pady=6,
            cursor="hand2"
        ).pack(side=tk.RIGHT, fill=tk.X, expand=True, padx=(2, 0))
        
        # --- 端口管理 ---
        port_mgmt_frame = tk.LabelFrame(
            left_col,
            text="🔧 端口管理",
            font=("Microsoft YaHei UI", 10, "bold"),
            bg="#ecf0f1",
            fg="#2c3e50",
            padx=10,
            pady=10
        )
        port_mgmt_frame.pack(fill=tk.X, pady=(0, 10))
        
        # 端口选择
        port_select_frame = tk.Frame(port_mgmt_frame, bg="#ecf0f1")
        port_select_frame.pack(fill=tk.X, pady=(0, 5))
        
        tk.Label(
            port_select_frame,
            text="端口:",
            font=("Microsoft YaHei UI", 9),
            bg="#ecf0f1"
        ).pack(side=tk.LEFT, padx=(0, 5))
        
        self.port_mgmt_combo = ttk.Combobox(
            port_select_frame,
            values=["COM3", "COM4", "COM5", "COM6"],
            state="readonly",
            width=8,
            font=("Consolas", 9)
        )
        self.port_mgmt_combo.set("COM3")
        self.port_mgmt_combo.pack(side=tk.LEFT, fill=tk.X, expand=True)
        
        # 管理按钮（两列）
        port_mgmt_button_frame = tk.Frame(port_mgmt_frame, bg="#ecf0f1")
        port_mgmt_button_frame.pack(fill=tk.X)
        
        tk.Button(
            port_mgmt_button_frame,
            text="🔍 测试端口",
            font=("Microsoft YaHei UI", 9),
            command=lambda: self.app.test_port(self.port_mgmt_combo.get()),
            bg="#3498db",
            fg="white",
            relief=tk.FLAT,
            padx=8,
            pady=6,
            cursor="hand2"
        ).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 2))
        
        tk.Button(
            port_mgmt_button_frame,
            text="🔓 释放端口",
            font=("Microsoft YaHei UI", 9),
            command=lambda: self.app.release_port(self.port_mgmt_combo.get()),
            bg="#e74c3c",
            fg="white",
            relief=tk.FLAT,
            padx=8,
            pady=6,
            cursor="hand2"
        ).pack(side=tk.RIGHT, fill=tk.X, expand=True, padx=(2, 0))
        
        # --- 高级设置 ---
        advanced_frame = tk.LabelFrame(
            left_col,
            text="⚙️ 高级设置",
            font=("Microsoft YaHei UI", 10, "bold"),
            bg="#ecf0f1",
            fg="#2c3e50",
            padx=10,
            pady=10
        )
        advanced_frame.pack(fill=tk.X)
        
        # 超时设置
        tk.Label(
            advanced_frame,
            text="烧录超时:",
            font=("Microsoft YaHei UI", 9),
            bg="#ecf0f1"
        ).grid(row=0, column=0, sticky=tk.W, pady=5)
        
        timeout_options = ["120秒（标准）", "180秒（推荐）", "300秒（大文件）"]
        self.app.timeout_combo = ttk.Combobox(
            advanced_frame,
            values=timeout_options,
            state="readonly",
            width=15,
            font=("Microsoft YaHei UI", 8)
        )
        self.app.timeout_combo.set(timeout_options[1])  # 默认180秒
        self.app.timeout_combo.grid(row=0, column=1, sticky=tk.W, pady=5, padx=(5, 0))
        
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
        # 更新烧录端口候选
        try:
            current_sbl = self.app.flash_port_combo.get() if hasattr(self.app, 'flash_port_combo') else None
            values_sbl = sbl_ports or []
            if hasattr(self.app, 'flash_port_combo'):
                self.app.flash_port_combo['values'] = values_sbl
                if current_sbl in values_sbl:
                    self.app.flash_port_combo.set(current_sbl)
                elif values_sbl:
                    self.app.flash_port_combo.set(values_sbl[0])
                # 同步变量
                if hasattr(self.app, 'sbl_port'):
                    self.app.sbl_port.set(self.app.flash_port_combo.get())
        except Exception:
            pass
        
        # 更新调试端口候选
        try:
            current_app = self.app.debug_port_combo.get() if hasattr(self.app, 'debug_port_combo') else None
            values_app = app_ports or []
            if hasattr(self.app, 'debug_port_combo'):
                self.app.debug_port_combo['values'] = values_app
                if current_app in values_app:
                    self.app.debug_port_combo.set(current_app)
                elif values_app:
                    self.app.debug_port_combo.set(values_app[0])
                # 同步变量
                if hasattr(self.app, 'app_port'):
                    self.app.app_port.set(self.app.debug_port_combo.get())
        except Exception:
            pass
        
        # 更新端口管理下拉：合并并去重
        try:
            if hasattr(self, 'port_mgmt_combo'):
                all_ports = list(dict.fromkeys((sbl_ports or []) + (app_ports or [])))
                if all_ports:
                    self.port_mgmt_combo['values'] = all_ports
                    if self.port_mgmt_combo.get() not in all_ports:
                        self.port_mgmt_combo.set(all_ports[0])
        except Exception:
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
    
    def check_sbl(self):
        """检测SBL是否存在"""
        port = self.app.flash_port_combo.get()
        
        if not port:
            from tkinter import messagebox
            messagebox.showwarning("警告", "请先选择烧录端口（COM3）")
            return
        
        # 导入SBLCheckDialog
        import sys
        import os
        # 获取flash_tool.py所在目录
        flash_tool_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        if flash_tool_dir not in sys.path:
            sys.path.insert(0, flash_tool_dir)
        
        # 动态导入（因为SBLCheckDialog在flash_tool.py中）
        try:
            import flash_tool
            dialog = flash_tool.SBLCheckDialog(self.app.root, port)
            self.app.root.wait_window(dialog)
        except Exception as e:
            from tkinter import messagebox
            messagebox.showerror("错误", f"无法打开SBL检测对话框：{str(e)}")


# 如果直接运行此文件，显示错误提示
if __name__ == "__main__":
    import sys
    print("=" * 70)
    print("⚠️  错误：tab_flash.py 不能单独运行！")
    print("=" * 70)
    print()
    print("请从主入口启动烧录工具：")
    print()
    print("  cd 5-Scripts")
    print("  python flash_tool.py")
    print()
    print("=" * 70)
    sys.exit(1)
