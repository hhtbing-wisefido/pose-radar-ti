#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
tab_flash.py - 烧录功能标签页（整合版）
版本: v1.5.9
作者: Benson@Wisefido

整合了原来的基本烧录、高级功能、串口监视、端口管理功能

注意：此模块不能单独运行，必须从 flash_tool.py 主入口启动！
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
        print("错误：tab_flash 模块不能单独运行！")
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
        icons = getattr(self.app, "icons", None)
        if icons is None:
            raise RuntimeError("主程序未初始化 icons（IconManager）")

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
        firmware_label = icons.make_labelframe_labelwidget(
            left_col,
            key="microchip",
            size=16,
            text="固件文件",
            bg="#ecf0f1",
            fg="#2c3e50",
            font=("Microsoft YaHei UI", 10, "bold"),
        )
        firmware_frame = tk.LabelFrame(
            left_col,
            labelwidget=firmware_label,
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
            text="未找到",
            font=("Microsoft YaHei UI", 9),
            bg="#ecf0f1",
            fg="red"
        )
        self.app.sbl_status_label.grid(row=0, column=1, columnspan=2, sticky=tk.W, pady=2, padx=(5, 0))
        self.app.sbl_status_label.config(image=icons.get("error", 16), compound="left")

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
            text="未找到",
            font=("Microsoft YaHei UI", 9),
            bg="#ecf0f1",
            fg="red"
        )
        self.app.app_status_label.grid(row=2, column=1, columnspan=2, sticky=tk.W, pady=2, padx=(5, 0))
        self.app.app_status_label.config(image=icons.get("error", 16), compound="left")

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
            text="分析已选固件",
            font=("Microsoft YaHei UI", 8),
            command=self.app.analyze_firmware,
            bg="#3498db",
            fg="white",
            relief=tk.FLAT,
            padx=8,
            pady=4,
            cursor="hand2"
        ).pack(fill=tk.X, expand=True)
        try:
            btn = button_container.winfo_children()[-1]
            btn.config(image=icons.get("search", 16), compound="left")
        except Exception:
            pass

        # --- Flash偏移量配置区 ---
        offset_label = icons.make_labelframe_labelwidget(
            left_col,
            key="settings",
            size=16,
            text="Flash偏移量配置",
            bg="#ecf0f1",
            fg="#2c3e50",
            font=("Microsoft YaHei UI", 10, "bold"),
        )
        offset_frame = tk.LabelFrame(
            left_col,
            labelwidget=offset_label,
            font=("Microsoft YaHei UI", 10, "bold"),
            bg="#ecf0f1",
            fg="#2c3e50",
            padx=10,
            pady=10
        )
        offset_frame.pack(fill=tk.X, pady=(10, 10))

        # 启用/禁用开关
        enable_offset_frame = tk.Frame(offset_frame, bg="#ecf0f1")
        enable_offset_frame.pack(fill=tk.X, pady=(0, 10))

        self.app.offset_enabled_var = tk.BooleanVar(value=False)  # 默认禁用

        tk.Checkbutton(
            enable_offset_frame,
            text="启用Flash偏移量参数",
            variable=self.app.offset_enabled_var,
            font=("Microsoft YaHei UI", 9, "bold"),
            bg="#ecf0f1",
            activebackground="#ecf0f1",
            fg="#2c3e50",
            selectcolor="#27ae60",
            command=self.toggle_offset_controls
        ).pack(side=tk.LEFT)
        try:
            cb = enable_offset_frame.winfo_children()[-1]
            cb.config(image=icons.get("settings", 16), compound="left")
        except Exception:
            pass

        # 提示信息
        self.offset_hint_label = tk.Label(
            enable_offset_frame,
            text="（启用时烧录命令包含偏移参数）",
            font=("Microsoft YaHei UI", 8),
            bg="#ecf0f1",
            fg="#7f8c8d"
        )
        self.offset_hint_label.pack(side=tk.LEFT, padx=(5, 0))

        # SBL Flash偏移
        sbl_offset_container = tk.Frame(offset_frame, bg="#ecf0f1")
        sbl_offset_container.pack(fill=tk.X, pady=(0, 8))

        self.sbl_offset_label = tk.Label(
            sbl_offset_container,
            text="SBL偏移:",
            font=("Microsoft YaHei UI", 9, "bold"),
            bg="#ecf0f1",
            width=10,
            anchor="w"
        )
        self.sbl_offset_label.pack(side=tk.LEFT)

        # SBL偏移选择变量
        self.app.sbl_offset_var = tk.StringVar(value="0x2000")

        # 预设选项
        sbl_preset_frame = tk.Frame(sbl_offset_container, bg="#ecf0f1")
        sbl_preset_frame.pack(side=tk.LEFT, fill=tk.X, expand=True)

        self.sbl_radio_0 = tk.Radiobutton(
            sbl_preset_frame,
            text="0x0000 (0字节)",
            variable=self.app.sbl_offset_var,
            value="0x0000",
            font=("Consolas", 8),
            bg="#ecf0f1",
            activebackground="#ecf0f1"
        )
        self.sbl_radio_0.pack(side=tk.LEFT, padx=(0, 8))

        self.sbl_radio_2000 = tk.Radiobutton(
            sbl_preset_frame,
            text="0x2000 (8192字节)",
            variable=self.app.sbl_offset_var,
            value="0x2000",
            font=("Consolas", 8),
            bg="#ecf0f1",
            activebackground="#ecf0f1"
        )
        self.sbl_radio_2000.pack(side=tk.LEFT, padx=(0, 8))

        self.sbl_radio_custom = tk.Radiobutton(
            sbl_preset_frame,
            text="自定义",
            variable=self.app.sbl_offset_var,
            value="custom",
            font=("Consolas", 8),
            bg="#ecf0f1",
            activebackground="#ecf0f1",
            command=lambda: self.app.sbl_offset_entry.focus()
        )
        self.sbl_radio_custom.pack(side=tk.LEFT)

        # 自定义输入框
        self.app.sbl_offset_entry = tk.Entry(
            sbl_preset_frame,
            font=("Consolas", 8),
            width=12,
            state="normal"
        )
        self.app.sbl_offset_entry.pack(side=tk.LEFT, padx=(5, 0))
        self.app.sbl_offset_entry.insert(0, "0x")

        # App Flash偏移
        app_offset_container = tk.Frame(offset_frame, bg="#ecf0f1")
        app_offset_container.pack(fill=tk.X)

        self.app_offset_label = tk.Label(
            app_offset_container,
            text="App偏移:",
            font=("Microsoft YaHei UI", 9, "bold"),
            bg="#ecf0f1",
            width=10,
            anchor="w"
        )
        self.app_offset_label.pack(side=tk.LEFT)

        # App偏移选择变量
        self.app.app_offset_var = tk.StringVar(value="0x42000")

        # 预设选项
        app_preset_frame = tk.Frame(app_offset_container, bg="#ecf0f1")
        app_preset_frame.pack(side=tk.LEFT, fill=tk.X, expand=True)

        self.app_radio_0 = tk.Radiobutton(
            app_preset_frame,
            text="0x0000 (0字节)",
            variable=self.app.app_offset_var,
            value="0x0000",
            font=("Consolas", 8),
            bg="#ecf0f1",
            activebackground="#ecf0f1"
        )
        self.app_radio_0.pack(side=tk.LEFT, padx=(0, 8))

        self.app_radio_42000 = tk.Radiobutton(
            app_preset_frame,
            text="0x42000 (270336字节)",
            variable=self.app.app_offset_var,
            value="0x42000",
            font=("Consolas", 8),
            bg="#ecf0f1",
            activebackground="#ecf0f1"
        )
        self.app_radio_42000.pack(side=tk.LEFT, padx=(0, 8))

        self.app_radio_custom = tk.Radiobutton(
            app_preset_frame,
            text="自定义",
            variable=self.app.app_offset_var,
            value="custom",
            font=("Consolas", 8),
            bg="#ecf0f1",
            activebackground="#ecf0f1",
            command=lambda: self.app.app_offset_entry.focus()
        )
        self.app_radio_custom.pack(side=tk.LEFT)

        # 自定义输入框
        self.app.app_offset_entry = tk.Entry(
            app_preset_frame,
            font=("Consolas", 8),
            width=12,
            state="normal"
        )
        self.app.app_offset_entry.pack(side=tk.LEFT, padx=(5, 0))
        self.app.app_offset_entry.insert(0, "0x")

        # 初始化偏移量控件状态（默认禁用，所以设为灰色）
        self.toggle_offset_controls()

        # --- 烧录操作区 ---
        flash_label = icons.make_labelframe_labelwidget(
            left_col,
            key="fire",
            size=16,
            text="烧录操作",
            bg="#ecf0f1",
            fg="#2c3e50",
            font=("Microsoft YaHei UI", 10, "bold"),
        )
        flash_frame = tk.LabelFrame(
            left_col,
            labelwidget=flash_label,
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
            text="完整烧录 (SBL + App)",
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
        try:
            btn = flash_frame.winfo_children()[-1]
            btn.config(image=icons.get("rocket", 16), compound="left")
        except Exception:
            pass

        # 单独烧录按钮（三列：仅SBL、仅应用固件、停止烧录）
        single_flash_frame = tk.Frame(flash_frame, bg="#ecf0f1")
        single_flash_frame.pack(fill=tk.X)

        tk.Button(
            single_flash_frame,
            text="仅SBL",
            font=("Microsoft YaHei UI", 9, "bold"),
            command=self.app.flash_sbl_only,
            bg="#e67e22",
            fg="white",
            relief=tk.FLAT,
            padx=8,
            pady=6,
            cursor="hand2"
        ).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 2))
        try:
            btn = single_flash_frame.winfo_children()[-1]
            btn.config(image=icons.get("fire", 16), compound="left")
        except Exception:
            pass

        tk.Button(
            single_flash_frame,
            text="仅应用固件",
            font=("Microsoft YaHei UI", 9, "bold"),
            command=self.app.flash_app_only,
            bg="#3498db",
            fg="white",
            relief=tk.FLAT,
            padx=8,
            pady=6,
            cursor="hand2"
        ).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(2, 2))
        try:
            btn = single_flash_frame.winfo_children()[-1]
            btn.config(image=icons.get("upload", 16), compound="left")
        except Exception:
            pass

        tk.Button(
            single_flash_frame,
            text="停止",
            font=("Microsoft YaHei UI", 9, "bold"),
            command=self.app.stop_flash,
            bg="#e74c3c",
            fg="white",
            relief=tk.FLAT,
            padx=8,
            pady=6,
            cursor="hand2"
        ).pack(side=tk.RIGHT, fill=tk.X, expand=True, padx=(2, 0))
        try:
            btn = single_flash_frame.winfo_children()[-1]
            btn.config(image=icons.get("stop", 16), compound="left")
        except Exception:
            pass

        # --- 端口管理（整合端口设置、串口监视和端口管理）---
        port_label = icons.make_labelframe_labelwidget(
            left_col,
            key="plug",
            size=16,
            text="端口管理",
            bg="#ecf0f1",
            fg="#2c3e50",
            font=("Microsoft YaHei UI", 10, "bold"),
        )
        port_mgmt_frame = tk.LabelFrame(
            left_col,
            labelwidget=port_label,
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

        # 配置grid列权重
        port_config_frame.columnconfigure(0, weight=0)  # 标签列
        port_config_frame.columnconfigure(1, weight=0)  # 端口选择列
        port_config_frame.columnconfigure(2, weight=0)  # 波特率标签列
        port_config_frame.columnconfigure(3, weight=0)  # 波特率选择列

        # 烧录端口（COM3 - User UART）
        self.flash_port_label = tk.Label(
            port_config_frame,
            text="烧录端口 - XDS110 Class Application/User UART:",
            font=("Microsoft YaHei UI", 9, "bold"),
            bg="#ecf0f1",
            fg="#2c3e50",
            anchor="w"
        )
        self.flash_port_label.grid(row=0, column=0, sticky=tk.W, pady=5, padx=(0, 5))

        self.app.flash_port_combo = ttk.Combobox(
            port_config_frame,
            width=8,
            state="readonly",
            font=("Consolas", 9)
        )
        self.app.flash_port_combo.grid(row=0, column=1, sticky=tk.W, pady=5, padx=(0, 15))
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

        # App波特率
        tk.Label(
            port_config_frame,
            text="App Baudrate:",
            font=("Microsoft YaHei UI", 9, "bold"),
            bg="#ecf0f1",
            fg="#2c3e50",
            anchor="w"
        ).grid(row=0, column=2, sticky=tk.W, pady=5, padx=(0, 5))

        self.app.app_baudrate_combo = ttk.Combobox(
            port_config_frame,
            width=10,
            state="readonly",
            font=("Consolas", 9),
            values=["9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600"]
        )
        self.app.app_baudrate_combo.grid(row=0, column=3, sticky=tk.W, pady=5)
        self.app.app_baudrate_combo.set("115200")  # 默认115200

        # 数据输出端口（COM4 - Auxiliary Data Port）
        self.debug_port_label = tk.Label(
            port_config_frame,
            text="测试数据端口 - XDS110 Class Auxiliary Data Port:",
            font=("Microsoft YaHei UI", 9, "bold"),
            bg="#ecf0f1",
            fg="#2c3e50",
            anchor="w"
        )
        self.debug_port_label.grid(row=1, column=0, sticky=tk.W, pady=5, padx=(0, 5))

        self.app.debug_port_combo = ttk.Combobox(
            port_config_frame,
            width=8,
            state="readonly",
            font=("Consolas", 9)
        )
        self.app.debug_port_combo.grid(row=1, column=1, sticky=tk.W, pady=5, padx=(0, 15))
        self.app.debug_port_combo.set("COM4")
        # 不同步到app_port - 调试口仅用于数据输出，不用于烧录

        # Data波特率
        tk.Label(
            port_config_frame,
            text="Data Baudrate:",
            font=("Microsoft YaHei UI", 9, "bold"),
            bg="#ecf0f1",
            fg="#2c3e50",
            anchor="w"
        ).grid(row=1, column=2, sticky=tk.W, pady=5, padx=(0, 5))

        self.app.data_baudrate_combo = ttk.Combobox(
            port_config_frame,
            width=10,
            state="readonly",
            font=("Consolas", 9),
            values=["9600", "19200", "38400", "57600", "115200", "125000", "230400", "460800", "921600"]
        )
        self.app.data_baudrate_combo.grid(row=1, column=3, sticky=tk.W, pady=5)
        self.app.data_baudrate_combo.set("125000")  # 默认125000

        # 端口操作按钮行（刷新 + 测试）
        port_action_frame = tk.Frame(port_mgmt_frame, bg="#ecf0f1")
        port_action_frame.pack(fill=tk.X, pady=(0, 8))

        tk.Button(
            port_action_frame,
            text="刷新",
            font=("Microsoft YaHei UI", 8),
            command=self.app.refresh_com_ports,
            bg="#3498db",
            fg="white",
            relief=tk.FLAT,
            padx=6,
            pady=4,
            cursor="hand2"
        ).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 2))
        try:
            btn = port_action_frame.winfo_children()[-1]
            btn.config(image=icons.get("refresh", 16), compound="left")
        except Exception:
            pass

        tk.Button(
            port_action_frame,
            text="测试",
            font=("Microsoft YaHei UI", 8),
            command=self.app.test_all_ports,
            bg="#27ae60",
            fg="white",
            relief=tk.FLAT,
            padx=6,
            pady=4,
            cursor="hand2"
        ).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(2, 0))
        try:
            btn = port_action_frame.winfo_children()[-1]
            btn.config(image=icons.get("search", 16), compound="left")
        except Exception:
            pass

        # 板载SBL固件存在性检测（单独一行）
        sbl_check_frame = tk.Frame(port_mgmt_frame, bg="#ecf0f1")
        sbl_check_frame.pack(fill=tk.X, pady=(0, 8))

        tk.Button(
            sbl_check_frame,
            text="板载SBL固件存在性检测\n(SOP调整为功能模式非烧录模式并重启)",
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
        try:
            btn = sbl_check_frame.winfo_children()[-1]
            btn.config(image=icons.get("search", 16), compound="left")
        except Exception:
            pass

        # ============= 右列：日志输出 =============

        # 日志标题
        tk.Label(
            right_col,
            text="烧录日志",
            font=("Microsoft YaHei UI", 12, "bold"),
            bg="#ecf0f1",
            fg="#2c3e50"
        ).pack(pady=(0, 10))
        try:
            lbl = right_col.winfo_children()[-1]
            lbl.config(image=icons.get("clipboard", 20), compound="left")
        except Exception:
            pass

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

        # 创建右键菜单
        self.log_context_menu = tk.Menu(self.app.log_text, tearoff=0)
        self.log_context_menu.add_command(
            label="复制选中内容",
            image=icons.get("clipboard", 16),
            compound="left",
            command=self.copy_selected_log,
        )
        self.log_context_menu.add_command(
            label="复制全部日志",
            image=icons.get("clipboard", 16),
            compound="left",
            command=self.copy_all_log,
        )
        self.log_context_menu.add_separator()
        self.log_context_menu.add_command(
            label="清空日志",
            image=icons.get("trash", 16),
            compound="left",
            command=self.clear_log,
        )

        # 绑定右键菜单
        self.app.log_text.bind("<Button-3>", self.show_log_context_menu)

        # 配置日志颜色标签
        self.app.log_text.tag_config("INFO", foreground="#3498db")
        self.app.log_text.tag_config("SUCCESS", foreground="#27ae60")
        self.app.log_text.tag_config("WARN", foreground="#f39c12")
        self.app.log_text.tag_config("ERROR", foreground="#e74c3c")

        # 进度条和时间显示区域
        progress_container = tk.Frame(log_frame, bg="#1a1a2e")
        progress_container.pack(fill=tk.X, pady=(5, 0))

        # 进度条显示区域（独立Label，解决Text widget渲染问题）- 左侧占70%
        progress_frame = tk.Frame(progress_container, bg="#1a1a2e", height=50)
        progress_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
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

        # 总执行时间实时显示区域 - 右侧占30%
        time_frame = tk.Frame(progress_container, bg="#1a1a2e", height=50, width=200)
        time_frame.pack(side=tk.RIGHT, fill=tk.Y)
        time_frame.pack_propagate(False)

        self.app.total_time_label = tk.Label(
            time_frame,
            text="总时间: 0秒",
            font=("Microsoft YaHei UI", 10, "bold"),
            bg="#1a1a2e",
            fg="#f39c12",
            anchor="center",
            justify=tk.CENTER
        )
        self.app.total_time_label.pack(fill=tk.BOTH, expand=True, padx=5, pady=8)
        self.app.total_time_label.config(image=icons.get("clock", 16), compound="left")

        # 清除日志按钮
        tk.Button(
            log_frame,
            text="清除日志",
            font=("Microsoft YaHei UI", 9),
            command=self.app.clear_log,
            bg="#95a5a6",
            fg="white",
            relief=tk.FLAT,
            padx=10,
            pady=4,
            cursor="hand2"
        ).pack(pady=(5, 0))
        try:
            btn = log_frame.winfo_children()[-1]
            btn.config(image=icons.get("trash", 16), compound="left")
        except Exception:
            pass

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
        if not hasattr(self.app, 'log_text'):
            return

        # 兼容：如果仍有历史emoji流入，这里做一次兜底清洗
        if isinstance(message, str):
            replacements = {
                "\u2705": "[OK]",
                "\u274c": "[X]",
                "\u26a0\ufe0f": "[!]",
                "\u26a0": "[!]",
                "\U0001F50D": "[?]",
                "\U0001F4C1": "",
                "\U0001F4CD": "",
                "\U0001F4E1": "",
                "\U0001F50C": "",
                "\U0001F527": "",
                "\U0001F5D1\ufe0f": "",
                "\U0001F5D1": "",
                "\U0001F4CB": "",
                "\U0001F680": "",
                "\U0001F525": "",
                "\U0001F6D1": "",
                "\u23f1\ufe0f": "",
                "\u23f1": "",
            }
            for old, new in replacements.items():
                message = message.replace(old, new)

        icons = getattr(self.app, "icons", None)
        tag_to_icon_key = {
            "SUCCESS": "ok",
            "WARN": "warning",
            "ERROR": "error",
            "INFO": "info",
        }
        tag_to_strip_prefix = {
            "SUCCESS": ("[OK]", "[DONE]"),
            "WARN": ("[WARN]", "[!]"),
            "ERROR": ("[ERROR]", "[X]"),
            "INFO": ("[INFO]",),
        }

        def _strip_prefix(line: str) -> str:
            if not tag or not isinstance(line, str):
                return line
            prefixes = tag_to_strip_prefix.get(tag, ())
            if not prefixes:
                return line

            # 仅处理行首前缀，保留行首缩进
            i = 0
            while i < len(line) and line[i] in (" ", "\t"):
                i += 1
            leading_ws = line[:i]
            body = line[i:]
            for p in prefixes:
                if body.startswith(p):
                    body = body[len(p):].lstrip()
                    break
            return leading_ws + body

        def _should_prefix_icon(line: str) -> bool:
            stripped = (line or "").strip()
            if not stripped:
                return False
            if all(ch in "=-*_+/\\|.[]()<>" for ch in stripped):
                return False
            return True

        self.app.log_text.config(state=tk.NORMAL)
        try:
            if tag and icons is not None and tag in tag_to_icon_key and isinstance(message, str):
                icon_key = tag_to_icon_key[tag]
                icon_img = icons.get(icon_key, 16)

                for part in message.splitlines(True):
                    if part in ("\n", "\r\n"):
                        self.app.log_text.insert(tk.END, part)
                        continue

                    part = _strip_prefix(part)
                    if icon_img is not None and _should_prefix_icon(part):
                        self.app.log_text.image_create(tk.END, image=icon_img)
                        self.app.log_text.insert(tk.END, " ")

                    self.app.log_text.insert(tk.END, part, tag)
            else:
                if tag:
                    self.app.log_text.insert(tk.END, message, tag)
                else:
                    self.app.log_text.insert(tk.END, message)

            self.app.log_text.see(tk.END)
        finally:
            self.app.log_text.config(state=tk.DISABLED)

    def show_log_context_menu(self, event):
        """显示日志右键菜单"""
        try:
            # 检查是否有选中内容
            if self.app.log_text.tag_ranges(tk.SEL):
                self.log_context_menu.entryconfig(0, state=tk.NORMAL)  # 启用"复制选中内容"
            else:
                self.log_context_menu.entryconfig(0, state=tk.DISABLED)  # 禁用"复制选中内容"

            # 显示菜单
            self.log_context_menu.post(event.x_root, event.y_root)
        except Exception as e:
            print(f"显示右键菜单失败: {e}")

    def copy_selected_log(self):
        """复制选中的日志内容"""
        try:
            # 临时启用文本框以获取选中内容
            self.app.log_text.config(state=tk.NORMAL)

            # 检查是否有选中内容
            if self.app.log_text.tag_ranges(tk.SEL):
                selected_text = self.app.log_text.get(tk.SEL_FIRST, tk.SEL_LAST)
                # 复制到剪贴板
                self.app.log_text.clipboard_clear()
                self.app.log_text.clipboard_append(selected_text)
                self.log("已复制选中日志到剪贴板\n", "SUCCESS")

            self.app.log_text.config(state=tk.DISABLED)
        except Exception as e:
            self.app.log_text.config(state=tk.DISABLED)
            self.log(f"复制失败: {e}\n", "ERROR")

    def copy_all_log(self):
        """复制全部日志内容"""
        try:
            # 临时启用文本框以获取全部内容
            self.app.log_text.config(state=tk.NORMAL)

            all_text = self.app.log_text.get(1.0, tk.END)
            if all_text.strip():
                # 复制到剪贴板
                self.app.log_text.clipboard_clear()
                self.app.log_text.clipboard_append(all_text)
                self.log("已复制全部日志到剪贴板\n", "SUCCESS")
            else:
                self.log("日志为空，无内容可复制\n", "WARN")

            self.app.log_text.config(state=tk.DISABLED)
        except Exception as e:
            self.app.log_text.config(state=tk.DISABLED)
            self.log(f"复制失败: {e}\n", "ERROR")

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

        # 导入 SBLCheckDialog（兼容脚本模式与 PyInstaller EXE 模式）
        try:
            import __main__ as main_mod

            dialog_cls = getattr(main_mod, "SBLCheckDialog", None)
            if dialog_cls is None:
                try:
                    import flash_tool as flash_tool_mod
                    dialog_cls = getattr(flash_tool_mod, "SBLCheckDialog", None)
                except Exception:
                    dialog_cls = None

            if dialog_cls is None:
                raise AttributeError("未找到 SBLCheckDialog（__main__/flash_tool）")

            dialog = dialog_cls(self.app.root, port)
            self.app.root.wait_window(dialog)
        except Exception as e:
            from tkinter import messagebox
            messagebox.showerror("错误", f"无法打开SBL检测对话框：{str(e)}")

    def _init_tool_options(self):
        """初始化烧录工具选项"""
        import os
        import sys
        from pathlib import Path

        # 工具选项字典 {显示名称: 完整路径}
        self.tool_options = {}

        # 选项1: 项目内工具（动态路径）
        try:
            # 获取基础目录 - EXE模式下使用exe所在目录，脚本模式下使用脚本目录的父目录
            if getattr(sys, 'frozen', False):
                # PyInstaller EXE模式：兼容 EXE 位于不同目录（如 6-Distribution 或 项目根 release/）
                exe_dir = Path(sys.executable).resolve().parent
                meipass = getattr(sys, '_MEIPASS', None)
                candidates = [
                    Path(meipass) if meipass else exe_dir,
                    exe_dir,
                    exe_dir.parent,
                    exe_dir.parent.parent,
                    exe_dir.parent.parent / '项目文档' / '3-固件工具' / '01-AWRL6844固件系统工具',
                ]
                base_dir = candidates[0]
                for p in candidates:
                    if (p / '3-Tools').exists():
                        base_dir = p
                        break
            else:
                # 脚本模式：使用tabs目录的父目录(5-Scripts)的父目录
                base_dir = Path(__file__).parent.parent.parent

            # 构建相对路径到3-Tools
            project_tool = base_dir / "3-Tools" / "arprog_cmdline_6844.exe"
            project_tool = project_tool.resolve()

            if project_tool.exists():
                self.tool_options["项目内工具 (推荐)"] = str(project_tool)
        except Exception as e:
            print(f"项目内工具路径解析失败: {e}")

        # 选项2: SDK工具
        sdk_tool = Path(r"C:\ti\MMWAVE_L_SDK_06_01_00_01\tools\FlashingTool\arprog_cmdline_6844.exe")
        if not sdk_tool.exists():
            ti_root = Path(r"C:\ti")
            if ti_root.exists():
                try:
                    for p in ti_root.glob("**/tools/FlashingTool/arprog_cmdline_6844.exe"):
                        sdk_tool = p
                        break
                except Exception:
                    pass
        if sdk_tool.exists():
            self.tool_options["SDK工具"] = str(sdk_tool)

        # 选项3: 自定义工具（如果已设置）
        if hasattr(self.app, 'flash_tool_path') and self.app.flash_tool_path:
            custom_path = Path(self.app.flash_tool_path)
            if custom_path.exists() and str(custom_path) not in self.tool_options.values():
                self.tool_options["自定义工具"] = str(custom_path)

        # 更新下拉框
        if self.tool_options:
            self.app.tool_combo['values'] = list(self.tool_options.keys())
            # 默认选择第一个（项目内工具）
            self.app.tool_combo.current(0)
            # 触发选择事件来更新路径显示和主程序变量
            self._on_tool_selected(None)
        else:
            self.app.tool_combo['values'] = ["未找到可用工具"]
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

    def toggle_offset_controls(self):
        """切换Flash偏移量控件的启用/禁用状态"""
        enabled = self.app.offset_enabled_var.get()

        state = "normal" if enabled else "disabled"
        fg_color = "#2c3e50" if enabled else "#95a5a6"

        # 控制SBL偏移量控件
        self.sbl_offset_label.config(fg=fg_color)
        self.sbl_radio_0.config(state=state)
        self.sbl_radio_2000.config(state=state)
        self.sbl_radio_custom.config(state=state)
        self.app.sbl_offset_entry.config(state=state)

        # 控制App偏移量控件
        self.app_offset_label.config(fg=fg_color)
        self.app_radio_0.config(state=state)
        self.app_radio_42000.config(state=state)
        self.app_radio_custom.config(state=state)
        self.app.app_offset_entry.config(state=state)

        # 更新提示信息
        if enabled:
            self.offset_hint_label.config(
                text="（启用时烧录命令包含偏移参数）",
                fg="#7f8c8d"
            )
        else:
            self.offset_hint_label.config(
                text="（禁用时烧录命令无偏移参数）",
                fg="#e74c3c"
            )

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
    print("错误：tab_flash.py 不能单独运行！")
    print("=" * 70)
    print()
    print("请从主入口启动烧录工具：")
    print()
    print("  cd 5-Scripts")
    print("  python flash_tool.py")
    print()
    print("=" * 70)
    sys.exit(1)
