#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
tab_flash.py - 烧录功能标签页（整合版）
版本: v1.5.9
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
        # 使用PanedWindow创建可拖动分隔的两列布局
        paned_window = ttk.PanedWindow(self.frame, orient=tk.HORIZONTAL)
        paned_window.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        # 左列容器（固件选择和控制区）- 30%宽度
        left_col = tk.Frame(paned_window, bg="#ecf0f1")
        paned_window.add(left_col, weight=3)  # weight=3 占30%
        
        # 右列容器（日志显示区）- 70%宽度
        right_col = tk.Frame(paned_window, bg="#ecf0f1")
        paned_window.add(right_col, weight=7)  # weight=7 占70%
        
        # 保存paned_window引用，用于动态调整分隔条位置
        self.paned_window = paned_window
        # 延迟设置分隔条位置（窗口显示后）
        self.frame.after(10, self._adjust_sash_position)
        
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
        
        # 配置grid列权重，使中间列可以自动伸缩
        firmware_frame.columnconfigure(0, weight=0)  # 标签列固定
        firmware_frame.columnconfigure(1, weight=1)  # 内容列自适应
        firmware_frame.columnconfigure(2, weight=0)  # 按钮列固定
        
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
        
        # SBL路径显示（自适应宽度）
        self.app.sbl_path_label = tk.Label(
            firmware_frame,
            text="",
            font=("Microsoft YaHei UI", 8),
            bg="#ecf0f1",
            fg="#7f8c8d",
            anchor="w",
            justify=tk.LEFT
        )
        self.app.sbl_path_label.grid(row=1, column=0, columnspan=2, sticky=tk.EW, pady=(0, 5), padx=(0, 5))
        
        # 绑定配置事件，动态更新wraplength
        self.app.sbl_path_label.bind('<Configure>', lambda e: self.app.sbl_path_label.config(wraplength=max(100, e.width - 10)))
        
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
        
        # 应用固件标签
        tk.Label(
            firmware_frame,
            text="应用固件:",
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
        
        # App路径显示（自适应宽度）
        self.app.app_path_label = tk.Label(
            firmware_frame,
            text="",
            font=("Microsoft YaHei UI", 8),
            bg="#ecf0f1",
            fg="#7f8c8d",
            anchor="w",
            justify=tk.LEFT
        )
        self.app.app_path_label.grid(row=3, column=0, columnspan=2, sticky=tk.EW, pady=(0, 5), padx=(0, 5))
        
        # 绑定配置事件，动态更新wraplength
        self.app.app_path_label.bind('<Configure>', lambda e: self.app.app_path_label.config(wraplength=max(100, e.width - 10)))
        
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
        
        # 创建工具选择容器
        tool_container = tk.Frame(firmware_frame, bg="#ecf0f1")
        tool_container.grid(row=4, column=1, columnspan=2, sticky=tk.EW, pady=(5, 2), padx=(5, 0))
        
        # 工具选择下拉框
        self.app.tool_combo = ttk.Combobox(
            tool_container,
            width=15,
            state="readonly",
            font=("Consolas", 8)
        )
        self.app.tool_combo.pack(side=tk.LEFT, fill=tk.X, expand=True)
        
        # 浏览按钮
        tk.Button(
            tool_container,
            text="选择",
            font=("Microsoft YaHei UI", 8),
            command=self.app.select_flash_tool,
            bg="#3498db",
            fg="white",
            relief=tk.FLAT,
            width=4,
            cursor="hand2"
        ).pack(side=tk.LEFT, padx=(3, 0))
        
        # 工具路径显示（自适应宽度）
        self.app.tool_path_label = tk.Label(
            firmware_frame,
            text="",
            font=("Microsoft YaHei UI", 8),
            bg="#ecf0f1",
            fg="#7f8c8d",
            anchor="w",
            justify=tk.LEFT
        )
        self.app.tool_path_label.grid(row=5, column=0, columnspan=3, sticky=tk.EW, pady=(0, 5))
        
        # 绑定配置事件，动态更新wraplength
        self.app.tool_path_label.bind('<Configure>', lambda e: self.app.tool_path_label.config(wraplength=max(100, e.width - 10)))
        
        # 初始化工具选项（放在界面元素创建之后）
        self._init_tool_options()
        
        # 选择变更时的回调
        self.app.tool_combo.bind('<<ComboboxSelected>>', self._on_tool_selected)
        
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
        
        # 单独烧录按钮（三列：仅SBL、仅应用固件、停止烧录）
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
            text="🔥 仅应用固件",
            font=("Microsoft YaHei UI", 9, "bold"),
            command=self.app.flash_app_only,
            bg="#3498db",
            fg="white",
            relief=tk.FLAT,
            padx=8,
            pady=6,
            cursor="hand2"
        ).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(2, 2))
        
        tk.Button(
            single_flash_frame,
            text="🛑 停止",
            font=("Microsoft YaHei UI", 9, "bold"),
            command=self.app.stop_flash,
            bg="#e74c3c",
            fg="white",
            relief=tk.FLAT,
            padx=8,
            pady=6,
            cursor="hand2"
        ).pack(side=tk.RIGHT, fill=tk.X, expand=True, padx=(2, 0))
        
        # --- 端口管理（整合端口设置、串口监视和端口管理）---
        port_mgmt_frame = tk.LabelFrame(
            left_col,
            text="🔧 端口管理",
            font=("Microsoft YaHei UI", 10, "bold"),
            bg="#ecf0f1",
            fg="#2c3e50",
            padx=10,
            pady=10
        )
        port_mgmt_frame.pack(fill=tk.X)
        
        # 端口配置区
        port_config_frame = tk.Frame(port_mgmt_frame, bg="#ecf0f1")
        port_config_frame.pack(fill=tk.X, pady=(0, 8))
        
        # 烧录端口（COM3 - User UART）
        self.flash_port_label = tk.Label(
            port_config_frame,
            text="烧录端口 - XDS110 Class Application/User UART:",
            font=("Microsoft YaHei UI", 9, "bold"),
            bg="#ecf0f1",
            fg="#2c3e50"
        )
        self.flash_port_label.grid(row=0, column=0, sticky=tk.W, pady=5)
        
        self.app.flash_port_combo = ttk.Combobox(
            port_config_frame,
            width=10,
            state="readonly",
            font=("Consolas", 9)
        )
        self.app.flash_port_combo.grid(row=0, column=1, sticky=tk.W, pady=5, padx=(5, 0))
        self.app.flash_port_combo.set("COM3")
        # 同步到主程序变量（SBL和App都使用此端口）
        try:
            self.app.sbl_port.set(self.app.flash_port_combo.get())
            self.app.app_port.set(self.app.flash_port_combo.get())  # App也使用COM3
        except Exception:
            pass
        # 选择变更时同步到两个变量
        def sync_flash_port(e):
            port = self.app.flash_port_combo.get()
            self.app.sbl_port.set(port)
            self.app.app_port.set(port)  # App也同步
        self.app.flash_port_combo.bind('<<ComboboxSelected>>', sync_flash_port)
        
        # 数据输出端口（COM4 - Auxiliary Data Port）
        self.debug_port_label = tk.Label(
            port_config_frame,
            text="测试数据端口 - XDS110 Class Auxiliary Data Port:",
            font=("Microsoft YaHei UI", 9, "bold"),
            bg="#ecf0f1",
            fg="#2c3e50"
        )
        self.debug_port_label.grid(row=1, column=0, sticky=tk.W, pady=5)
        
        self.app.debug_port_combo = ttk.Combobox(
            port_config_frame,
            width=10,
            state="readonly",
            font=("Consolas", 9)
        )
        self.app.debug_port_combo.grid(row=1, column=1, sticky=tk.W, pady=5, padx=(5, 0))
        self.app.debug_port_combo.set("COM4")
        # 不同步到app_port - 调试口仅用于数据输出，不用于烧录
        
        # 端口操作按钮行（刷新 + 测试）
        port_action_frame = tk.Frame(port_mgmt_frame, bg="#ecf0f1")
        port_action_frame.pack(fill=tk.X, pady=(0, 8))
        
        tk.Button(
            port_action_frame,
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
            port_action_frame,
            text="🔍 测试",
            font=("Microsoft YaHei UI", 8),
            command=self.app.test_all_ports,
            bg="#27ae60",
            fg="white",
            relief=tk.FLAT,
            padx=6,
            pady=4,
            cursor="hand2"
        ).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(2, 0))
        
        # 板载SBL固件存在性检测（单独一行）
        sbl_check_frame = tk.Frame(port_mgmt_frame, bg="#ecf0f1")
        sbl_check_frame.pack(fill=tk.X, pady=(0, 8))
        
        tk.Button(
            sbl_check_frame,
            text="🔎 板载SBL固件存在性检测\n(SOP调整为功能模式非烧录模式并重启)",
            font=("Microsoft YaHei UI", 8),
            command=self.check_sbl,
            bg="#9b59b6",
            fg="white",
            relief=tk.FLAT,
            padx=8,
            pady=8,
            cursor="hand2",
            justify=tk.CENTER
        ).pack(fill=tk.X, expand=True)
        
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
        
        # 进度条显示区域（独立Label，解决Text widget渲染问题）
        progress_frame = tk.Frame(log_frame, bg="#1a1a2e", height=50)
        progress_frame.pack(fill=tk.X, pady=(5, 0))
        progress_frame.pack_propagate(False)
        
        self.app.progress_label = tk.Label(
            progress_frame,
            text="",
            font=("Consolas", 11, "bold"),
            bg="#1a1a2e",
            fg="#00d9ff",
            anchor="w",
            justify=tk.LEFT
        )
        self.app.progress_label.pack(fill=tk.BOTH, expand=True, padx=10, pady=8)
        
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
        
        # 初始化时刷新一次端口，更新Label显示
        self.frame.after(100, self.app.refresh_com_ports)
    
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
                # 更新Label显示实际端口号
                if hasattr(self, 'flash_port_label') and values_sbl:
                    self.flash_port_label.config(text=f"烧录端口 - XDS110 Class Application/User UART ({values_sbl[0]}):")
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
                # 更新Label显示实际端口号
                if hasattr(self, 'debug_port_label') and values_app:
                    self.debug_port_label.config(text=f"测试数据端口 - XDS110 Class Auxiliary Data Port ({values_app[0]}):")
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
    
    def _init_tool_options(self):
        """初始化烧录工具选项"""
        import os
        from pathlib import Path
        
        # 工具选项字典 {显示名称: 完整路径}
        self.tool_options = {}
        
        # 选项1: 项目内工具（动态路径）
        try:
            # 获取当前脚本的父目录（5-Scripts）
            script_dir = Path(__file__).parent.parent
            # 构建相对路径到3-Tools
            project_tool = script_dir / ".." / "3-Tools" / "arprog_cmdline_6844.exe"
            project_tool = project_tool.resolve()
            
            if project_tool.exists():
                self.tool_options["📦 项目内工具 (推荐)"] = str(project_tool)
        except Exception as e:
            print(f"项目内工具路径解析失败: {e}")
        
        # 选项2: SDK工具
        sdk_tool = Path(r"C:\ti\MMWAVE_L_SDK_06_01_00_01\tools\FlashingTool\arprog_cmdline_6844.exe")
        if sdk_tool.exists():
            self.tool_options["🔧 SDK工具"] = str(sdk_tool)
        
        # 选项3: 自定义工具（如果已设置）
        if hasattr(self.app, 'flash_tool_path') and self.app.flash_tool_path:
            custom_path = Path(self.app.flash_tool_path)
            if custom_path.exists() and str(custom_path) not in self.tool_options.values():
                self.tool_options["✨ 自定义工具"] = str(custom_path)
        
        # 更新下拉框
        if self.tool_options:
            self.app.tool_combo['values'] = list(self.tool_options.keys())
            # 默认选择第一个（项目内工具）
            self.app.tool_combo.current(0)
            # 触发选择事件来更新路径显示和主程序变量
            self._on_tool_selected(None)
        else:
            self.app.tool_combo['values'] = ["❌ 未找到可用工具"]
            self.app.tool_combo.current(0)
            self.app.tool_path_label.config(text="未找到烧录工具，请手动选择", fg="red")
    
    def _on_tool_selected(self, event):
        """工具选择变更时的回调"""
        selected_name = self.app.tool_combo.get()
        
        if selected_name in self.tool_options:
            tool_path = self.tool_options[selected_name]
            
            # 更新主程序的工具路径
            self.app.flash_tool_path = tool_path
            
            # 更新路径显示
            self.app.tool_path_label.config(
                text=tool_path,
                fg="#27ae60"  # 绿色表示有效
            )
            
            # 更新日志（如果log_text已创建）
            if hasattr(self.app, 'log_text'):
                self.app.log_text.insert(
                    tk.END,
                    f"[INFO] 已选择烧录工具: {selected_name}\n      路径: {tool_path}\n",
                    "info"
                )
                self.app.log_text.see(tk.END)
    
    def _adjust_sash_position(self):
        """动态调整分隔条位置为3:7比例"""
        try:
            if hasattr(self, 'paned_window'):
                # 获取窗口总宽度
                total_width = self.paned_window.winfo_width()
                if total_width > 1:  # 确保窗口已显示
                    # 设置为30%位置
                    position = int(total_width * 0.3)
                    self.paned_window.sashpos(0, position)
                else:
                    # 窗口未完全显示，再次尝试
                    self.frame.after(50, self._adjust_sash_position)
        except Exception as e:
            # 忽略错误，避免影响程序运行
            pass


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
