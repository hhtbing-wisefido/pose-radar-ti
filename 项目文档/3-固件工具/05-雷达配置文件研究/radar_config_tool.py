#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
AWRL6844雷达配置专用GUI工具 v1.3.3
集成配置文件读写、分析、数据解析等功能

更新日志 v1.3.3:
- 🐛 修复配置分析功能显示问题
  * 分析完成后自动切换到"配置分析"标签页
  * 日志中显示分析结果摘要（通道、性能参数、帧率）
  * 详细结果显示在配置分析树形控件中
  * 提供完整的分析反馈，不再只显示"完成"
- 构建日期：2025-12-20

更新日志 v1.2.0:
- 🎨 UI布局重大优化
  * 左侧面板添加滚动条支持，完整显示所有控制区域
  * 操作控制区域不再被截断，支持鼠标滚轮滚动
  * 优化Canvas布局，自适应窗口宽度
- 📢 启动提示优化
  * 增强旧进程检测提示信息，显示详细列表
  * 启动流程信息更清晰，带边框分隔
  * 关闭进程后显示成功数量统计

更新日志 v1.1.2:
- 🐛 修复端口下拉框和启动流程问题
  * 端口下拉框宽度增加到50，完整显示带描述的端口信息
  * 移除启动时的弹窗确认，自动关闭旧进程，避免阻塞
  * 优化启动流程，不再需要用户手动确认关闭旧窗口

更新日志 v1.1.0:
- 🎨 UI布局优化
  * 日志标签页移至首位，作为默认显示页面
  * 串口设置区域布局调整：刷新和测试按钮移至连接按钮上方
- 🔍 串口功能增强
  * 刷新功能显示详细端口信息（描述、VID:PID）
  * 自动识别AWRL6844烧录端口和调试端口
  * 新增端口测试功能，逐个测试所有端口连接状态
  * 测试结果弹窗显示并同步记录到日志
- 构建日期：2025-12-20

更新日志 v1.0.3:
- 🎨 配置文件选择区域UI细节优化
  * "配置文件"改为"当前加载配置文件"，更明确
  * "选择"按钮改为"选择并立即加载配置"，功能更清晰
  * 按钮移至绝对路径下方，布局更合理
- 构建日期：2025-12-20

更新日志 v1.0.2:
- 🎨 配置文件选择区域UI优化
  * "浏览"改为"选择"，移到文件名后面
  * 绝对路径改用Label显示，无边框，自适应完全显示
  * "加载选中配置"改为"加载默认配置"
  * 移除"最近使用"功能
- 构建日期：2025-12-20

更新日志 v1.0.1:
- 🎨 配置文件选择区域优化
  * 文件名单独显示在标签中
  * 完整绝对路径显示在下方文本框
  * 新增默认配置下拉框，预设两个常用配置路径
- 🔍 SDK扫描功能增强
  * 新增多SDK路径管理（添加/删除目录）
  * 相对路径改为绝对路径显示
  * 新增模糊搜索功能（搜索文件名、路径、应用、芯片）
  * 新增一键添加配置文件到主界面功能
  * 新增右键菜单（复制文件名/绝对路径/在资源管理器中显示）
- 构建日期：2025-12-20
"""

import tkinter as tk
from tkinter import ttk, filedialog, messagebox, scrolledtext
import serial
import serial.tools.list_ports
import struct
import time
import re
import os
import sys
import subprocess
import psutil
from pathlib import Path
from typing import Dict, List, Optional
import threading
import json
from datetime import datetime
from config_calculator import RadarConfigCalculator
from config_scanner import ConfigScanner

class RadarConfigTool:
    """AWRL6844雷达配置工具主窗口"""
    
    # 现代深色主题配色方案
    COLORS = {
        'bg_dark': '#1e1e1e',           # 深色背景
        'bg_medium': '#2d2d2d',         # 中等背景
        'bg_light': '#3e3e3e',          # 浅色背景
        'accent_blue': '#0078d4',       # 强调蓝色
        'accent_cyan': '#00d9ff',       # 青色高亮
        'accent_green': '#16c60c',      # 成功绿色
        'accent_orange': '#ff8c00',     # 警告橙色
        'accent_red': '#e81123',        # 错误红色
        'text_primary': '#ffffff',      # 主文字
        'text_secondary': '#cccccc',    # 次要文字
        'text_disabled': '#666666',     # 禁用文字
        'border': '#555555',            # 边框
        'hover': '#4e4e4e',             # 悬停
    }
    
    def __init__(self, root):
        self.root = root
        self.root.title("⚡ AWRL6844 雷达配置工具 v1.3.3 | Wisefido")
        self.root.geometry("1500x950")
        
        # 窗口置顶显示
        self.root.attributes('-topmost', True)
        self.root.lift()
        self.root.focus_force()
        # 0.5秒后取消置顶，避免一直遮挡其他窗口
        self.root.after(500, lambda: self.root.attributes('-topmost', False))
        
        # 设置窗口图标
        try:
            icon_path = Path(__file__).parent / "radar_icon.ico"
            if icon_path.exists():
                self.root.iconbitmap(str(icon_path))
        except Exception as e:
            print(f"⚠️ 图标加载失败: {e}")
        
        # 设置深色主题
        self._setup_theme()
        
        # 工具类
        self.calculator = RadarConfigCalculator()
        self.scanner = ConfigScanner()
        
        # 变量
        self.default_config = r"C:\ti\radar_toolbox_3_30_00_06\tools\Adc_Data_Capture_Tool_DCA1000_CLI\chirp_configs\xWRL6844_4T4R_tdm.cfg"
        self.current_config_path = tk.StringVar(value=self.default_config)
        self.selected_port = tk.StringVar(value="COM4")
        self.baudrate = tk.IntVar(value=115200)
        self.write_target = tk.StringVar(value="RAM")  # RAM, Flash
        self.serial_port = None
        self.config_history = []
        self.receiving_data = False
        
        # 创建UI
        self._create_menu()
        self._create_widgets()
        
        # 加载默认配置
        try:
            if Path(self.default_config).exists():
                self._load_config_file(self.default_config)
        except Exception as e:
            self._log(f"⚠️ 默认配置加载失败: {e}", 'warning')
    
    def _setup_theme(self):
        """设置现代深色主题"""
        style = ttk.Style()
        
        # 使用clam作为基础主题（最易定制）
        style.theme_use('clam')
        
        # 配置主题颜色
        style.configure('.',
            background=self.COLORS['bg_medium'],
            foreground=self.COLORS['text_primary'],
            bordercolor=self.COLORS['border'],
            darkcolor=self.COLORS['bg_dark'],
            lightcolor=self.COLORS['bg_light'],
            troughcolor=self.COLORS['bg_dark'],
            focuscolor=self.COLORS['accent_cyan'],
            selectbackground=self.COLORS['accent_blue'],
            selectforeground=self.COLORS['text_primary'],
            fieldbackground=self.COLORS['bg_dark'],
            font=('Segoe UI', 10)
        )
        
        # TFrame样式
        style.configure('TFrame', background=self.COLORS['bg_medium'])
        style.configure('Card.TFrame', background=self.COLORS['bg_light'], relief='flat', borderwidth=1)
        
        # TLabel样式
        style.configure('TLabel',
            background=self.COLORS['bg_medium'],
            foreground=self.COLORS['text_primary'],
            font=('Segoe UI', 10)
        )
        style.configure('Title.TLabel',
            font=('Segoe UI', 12, 'bold'),
            foreground=self.COLORS['accent_cyan']
        )
        style.configure('Subtitle.TLabel',
            font=('Segoe UI', 9),
            foreground=self.COLORS['text_secondary']
        )
        
        # TButton样式
        style.configure('TButton',
            background=self.COLORS['bg_light'],
            foreground=self.COLORS['text_primary'],
            bordercolor=self.COLORS['border'],
            focuscolor=self.COLORS['accent_cyan'],
            font=('Segoe UI', 10),
            padding=(15, 8)
        )
        style.map('TButton',
            background=[('active', self.COLORS['hover']), ('pressed', self.COLORS['bg_dark'])],
            foreground=[('disabled', self.COLORS['text_disabled'])]
        )
        
        # Accent按钮样式
        style.configure('Accent.TButton',
            background=self.COLORS['accent_blue'],
            foreground=self.COLORS['text_primary'],
            font=('Segoe UI', 10, 'bold'),
            padding=(20, 10)
        )
        style.map('Accent.TButton',
            background=[('active', '#1e8ad6'), ('pressed', '#006bb3')]
        )
        
        # TEntry样式
        style.configure('TEntry',
            fieldbackground=self.COLORS['bg_dark'],
            foreground=self.COLORS['text_primary'],
            bordercolor=self.COLORS['border'],
            insertcolor=self.COLORS['text_primary']
        )
        
        # TCombobox样式
        style.configure('TCombobox',
            fieldbackground=self.COLORS['bg_dark'],
            background=self.COLORS['bg_light'],
            foreground=self.COLORS['text_primary'],
            arrowcolor=self.COLORS['text_primary']
        )
        
        # Treeview样式
        style.configure('Treeview',
            background=self.COLORS['bg_dark'],
            foreground=self.COLORS['text_primary'],
            fieldbackground=self.COLORS['bg_dark'],
            bordercolor=self.COLORS['border'],
            font=('Consolas', 9)
        )
        style.configure('Treeview.Heading',
            background=self.COLORS['bg_light'],
            foreground=self.COLORS['accent_cyan'],
            font=('Segoe UI', 10, 'bold')
        )
        style.map('Treeview',
            background=[('selected', self.COLORS['accent_blue'])],
            foreground=[('selected', self.COLORS['text_primary'])]
        )
        
        # Notebook样式
        style.configure('TNotebook',
            background=self.COLORS['bg_medium'],
            bordercolor=self.COLORS['border']
        )
        style.configure('TNotebook.Tab',
            background=self.COLORS['bg_light'],
            foreground=self.COLORS['text_secondary'],
            padding=(20, 10),
            font=('Segoe UI', 10)
        )
        style.map('TNotebook.Tab',
            background=[('selected', self.COLORS['bg_medium'])],
            foreground=[('selected', self.COLORS['accent_cyan'])],
            expand=[('selected', [1, 1, 1, 0])]
        )
        
        # LabelFrame样式
        style.configure('TLabelframe',
            background=self.COLORS['bg_medium'],
            bordercolor=self.COLORS['border'],
            relief='flat'
        )
        style.configure('TLabelframe.Label',
            background=self.COLORS['bg_medium'],
            foreground=self.COLORS['accent_cyan'],
            font=('Segoe UI', 11, 'bold')
        )
        
        # Radiobutton样式
        style.configure('TRadiobutton',
            background=self.COLORS['bg_medium'],
            foreground=self.COLORS['text_primary'],
            font=('Segoe UI', 10)
        )
        
        # 设置根窗口背景
        self.root.configure(bg=self.COLORS['bg_medium'])
    
    def _create_menu(self):
        """创建菜单栏"""
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)
        
        # 文件菜单
        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="文件", menu=file_menu)
        file_menu.add_command(label="打开配置文件...", command=self._open_config_file)
        file_menu.add_command(label="保存分析结果...", command=self._save_analysis)
        file_menu.add_separator()
        file_menu.add_command(label="退出", command=self.root.quit)
        
        # 工具菜单
        tool_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="工具", menu=tool_menu)
        tool_menu.add_command(label="扫描SDK配置文件", command=self._scan_sdk_configs)
        tool_menu.add_command(label="配置历史记录", command=self._show_history)
        tool_menu.add_separator()
        tool_menu.add_command(label="清空日志", command=self._clear_log)
        
        # 帮助菜单
        help_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="帮助", menu=help_menu)
        help_menu.add_command(label="使用说明", command=self._show_help)
        help_menu.add_command(label="关于", command=self._show_about)
    
    def _create_widgets(self):
        """创建主界面组件"""
        # 主容器
        main_paned = ttk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        main_paned.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # 左侧面板 - 添加滚动条支持
        left_container = ttk.Frame(main_paned)
        main_paned.add(left_container, weight=1)
        
        # 创建Canvas和Scrollbar
        canvas = tk.Canvas(left_container, bg=self.COLORS['bg_medium'], highlightthickness=0)
        scrollbar = ttk.Scrollbar(left_container, orient="vertical", command=canvas.yview)
        left_frame = ttk.Frame(canvas)
        
        # 配置Canvas
        canvas.configure(yscrollcommand=scrollbar.set)
        
        # 布局
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        
        # 将left_frame放入Canvas
        canvas_frame = canvas.create_window((0, 0), window=left_frame, anchor="nw")
        
        # 更新滚动区域
        def on_frame_configure(event):
            canvas.configure(scrollregion=canvas.bbox("all"))
            # 同时调整canvas窗口宽度以匹配canvas宽度
            canvas.itemconfig(canvas_frame, width=event.width)
        
        left_frame.bind("<Configure>", on_frame_configure)
        canvas.bind("<Configure>", lambda e: canvas.itemconfig(canvas_frame, width=e.width))
        
        # 鼠标滚轮支持
        def on_mousewheel(event):
            canvas.yview_scroll(int(-1*(event.delta/120)), "units")
        
        canvas.bind_all("<MouseWheel>", on_mousewheel)
        
        # 右侧面板
        right_frame = ttk.Frame(main_paned)
        main_paned.add(right_frame, weight=2)
        
        # === 左侧面板内容 ===
        self._create_config_selector(left_frame)
        self._create_port_settings(left_frame)
        self._create_control_buttons(left_frame)
        
        # === 右侧面板内容 ===
        self._create_notebook(right_frame)
    
    def _create_config_selector(self, parent):
        """配置文件选择区域"""
        frame = ttk.LabelFrame(parent, text="📁 配置文件选择", padding=10)
        frame.pack(fill=tk.X, padx=5, pady=5)
        
        # 当前加载配置文件名显示
        file_label_frame = ttk.Frame(frame)
        file_label_frame.pack(fill=tk.X, pady=(0, 5))
        
        ttk.Label(file_label_frame, text="当前加载配置文件:", 
                 font=('Segoe UI', 10, 'bold')).pack(side=tk.LEFT)
        self.config_filename_label = ttk.Label(file_label_frame, text="未选择", 
                                              foreground=self.COLORS['accent_cyan'],
                                              font=('Segoe UI', 10))
        self.config_filename_label.pack(side=tk.LEFT, padx=5)
        
        # 完整路径显示（自适应，无边框）
        path_label = ttk.Label(frame, textvariable=self.current_config_path, 
                              foreground=self.COLORS['text_secondary'],
                              font=('Segoe UI', 9),
                              wraplength=380,  # 自动换行
                              justify=tk.LEFT)
        path_label.pack(fill=tk.X, pady=(0, 5), anchor=tk.W)
        
        # 选择并立即加载配置按钮
        ttk.Button(frame, text="选择并立即加载配置", 
                  command=self._open_config_file).pack(fill=tk.X, pady=(0, 10))
        
        # 默认配置下拉框
        ttk.Label(frame, text="默认设置:").pack(anchor=tk.W, pady=(5, 2))
        self.default_configs = [
            r"C:\ti\radar_toolbox_3_30_00_06\tools\mmwave_data_recorder\src\cfg\6844_profile_4T4R_tdm.cfg",
            r"C:\ti\radar_toolbox_3_30_00_06\tools\Adc_Data_Capture_Tool_DCA1000_CLI\chirp_configs\xWRL6844_4T4R_tdm.cfg"
        ]
        self.default_config_var = tk.StringVar()
        default_combo = ttk.Combobox(frame, textvariable=self.default_config_var, 
                                    values=[Path(p).name for p in self.default_configs],
                                    state='readonly', width=50)
        default_combo.pack(fill=tk.X, pady=2)
        default_combo.bind('<<ComboboxSelected>>', self._on_default_config_selected)
        
        # 加载默认配置按钮
        ttk.Button(frame, text="加载默认配置", 
                  command=self._load_selected_default).pack(fill=tk.X, pady=(5, 0))
    
    def _create_port_settings(self, parent):
        """串口设置区域"""
        frame = ttk.LabelFrame(parent, text="🔌 串口设置", padding=10)
        frame.pack(fill=tk.X, padx=5, pady=5)
        
        # 端口选择
        port_frame = ttk.Frame(frame)
        port_frame.pack(fill=tk.X, pady=2)
        
        ttk.Label(port_frame, text="端口:").pack(side=tk.LEFT)
        self.port_combo = ttk.Combobox(port_frame, textvariable=self.selected_port, width=50)
        self.port_combo['values'] = self._get_available_ports()
        self.port_combo.pack(side=tk.LEFT, padx=5)
        
        # 波特率
        baud_frame = ttk.Frame(frame)
        baud_frame.pack(fill=tk.X, pady=2)
        
        ttk.Label(baud_frame, text="波特率:").pack(side=tk.LEFT)
        ttk.Combobox(baud_frame, textvariable=self.baudrate, 
                    values=[9600, 115200, 230400, 460800], width=10).pack(side=tk.LEFT, padx=5)
        
        # 连接状态
        self.port_status = ttk.Label(frame, text="● 未连接", 
                                     foreground=self.COLORS['accent_red'],
                                     font=('Segoe UI', 10, 'bold'))
        self.port_status.pack(anchor=tk.W, pady=5)
        
        # 刷新和测试按钮
        refresh_frame = ttk.Frame(frame)
        refresh_frame.pack(fill=tk.X, pady=(0, 5))
        
        ttk.Button(refresh_frame, text="🔄 刷新", command=self._refresh_ports).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=2)
        ttk.Button(refresh_frame, text="🧪 测试", command=self._test_ports).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=2)
        
        # 连接/断开按钮
        btn_frame = ttk.Frame(frame)
        btn_frame.pack(fill=tk.X, pady=5)
        
        self.btn_connect = ttk.Button(btn_frame, text="🔗 连接", command=self._connect_port, style='Accent.TButton')
        self.btn_connect.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=2)
        
        self.btn_disconnect = ttk.Button(btn_frame, text="🔌 断开", command=self._disconnect_port, state='disabled')
        self.btn_disconnect.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=2)
    
    def _create_control_buttons(self, parent):
        """控制按钮区域"""
        frame = ttk.LabelFrame(parent, text="🎮 操作控制", padding=10)
        frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # 写入目标选择
        target_frame = ttk.Frame(frame)
        target_frame.pack(fill=tk.X, pady=5)
        
        ttk.Label(target_frame, text="写入目标:").pack(anchor=tk.W)
        ttk.Radiobutton(target_frame, text="RAM (临时)", variable=self.write_target, 
                       value="RAM").pack(anchor=tk.W)
        ttk.Radiobutton(target_frame, text="Flash (永久)", variable=self.write_target, 
                       value="Flash").pack(anchor=tk.W)
        
        # 功能按钮
        ttk.Separator(frame, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=10)
        
        # 写入配置
        ttk.Button(frame, text="📤 写入配置到雷达", 
                  command=self._write_config, style='Accent.TButton').pack(fill=tk.X, pady=2)
        
        # 读取配置（仅支持Flash的Demo）
        ttk.Button(frame, text="📥 读取雷达配置", 
                  command=self._read_config).pack(fill=tk.X, pady=2)
        
        # 分析配置
        ttk.Button(frame, text="📊 分析配置性能", 
                  command=self._analyze_config).pack(fill=tk.X, pady=2)
        
        # 雷达控制
        ttk.Separator(frame, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=10)
        
        control_frame = ttk.Frame(frame)
        control_frame.pack(fill=tk.X)
        
        ttk.Button(control_frame, text="▶️ 启动", 
                  command=lambda: self._send_command("sensorStart")).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=2)
        ttk.Button(control_frame, text="⏹️ 停止", 
                  command=lambda: self._send_command("sensorStop")).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=2)
        
        # 数据采集
        ttk.Separator(frame, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=10)
        
        ttk.Button(frame, text="🎯 开始接收数据", 
                  command=self._start_receiving_data).pack(fill=tk.X, pady=2)
        ttk.Button(frame, text="🛑 停止接收数据", 
                  command=self._stop_receiving_data).pack(fill=tk.X, pady=2)
        
        # 推断配置
        ttk.Button(frame, text="🔍 从数据推断配置", 
                  command=self._infer_config_from_data).pack(fill=tk.X, pady=2)
    
    def _create_notebook(self, parent):
        """创建标签页"""
        self.notebook = ttk.Notebook(parent)
        self.notebook.pack(fill=tk.BOTH, expand=True)
        
        # 标签页1: 日志输出（默认显示）
        self._create_log_tab()
        
        # 标签页2: 配置文件内容
        self._create_config_content_tab()
        
        # 标签页3: 性能分析
        self._create_analysis_tab()
        
        # 标签页4: 数据接收
        self._create_data_tab()
        
        # 标签页5: SDK扫描
        self._create_scanner_tab()
    
    def _create_config_content_tab(self):
        """配置文件内容标签页"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="📄 配置文件")
        
        # 工具栏
        toolbar = ttk.Frame(frame)
        toolbar.pack(fill=tk.X, padx=5, pady=5)
        
        ttk.Button(toolbar, text="重新加载", command=self._reload_config).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="另存为...", command=self._save_config_as).pack(side=tk.LEFT, padx=2)
        
        # 文本编辑器
        text_frame = ttk.Frame(frame)
        text_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        self.config_text = scrolledtext.ScrolledText(text_frame, wrap=tk.WORD, 
                                                     font=('Consolas', 10),
                                                     bg=self.COLORS['bg_dark'],
                                                     fg=self.COLORS['text_primary'],
                                                     insertbackground=self.COLORS['accent_cyan'],
                                                     selectbackground=self.COLORS['accent_blue'],
                                                     selectforeground=self.COLORS['text_primary'],
                                                     relief='flat',
                                                     borderwidth=2)
        self.config_text.pack(fill=tk.BOTH, expand=True)
        
        # 语法高亮（深色主题）
        self.config_text.tag_config('comment', foreground='#6a9955')
        self.config_text.tag_config('command', foreground='#4ec9b0', font=('Consolas', 10, 'bold'))
        self.config_text.tag_config('number', foreground='#b5cea8')
    
    def _create_analysis_tab(self):
        """性能分析标签页"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="📊 性能分析")
        
        # 分析结果树形视图
        tree_frame = ttk.Frame(frame)
        tree_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # 滚动条
        scrollbar = ttk.Scrollbar(tree_frame)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        # 树形控件
        self.analysis_tree = ttk.Treeview(tree_frame, columns=('value', 'unit', 'note'),
                                         yscrollcommand=scrollbar.set)
        self.analysis_tree.pack(fill=tk.BOTH, expand=True)
        scrollbar.config(command=self.analysis_tree.yview)
        
        # 列配置
        self.analysis_tree.heading('#0', text='参数名称')
        self.analysis_tree.heading('value', text='数值')
        self.analysis_tree.heading('unit', text='单位')
        self.analysis_tree.heading('note', text='说明')
        
        self.analysis_tree.column('#0', width=250)
        self.analysis_tree.column('value', width=150)
        self.analysis_tree.column('unit', width=80)
        self.analysis_tree.column('note', width=300)
        
        # 导出按钮
        btn_frame = ttk.Frame(frame)
        btn_frame.pack(fill=tk.X, padx=5, pady=5)
        
        ttk.Button(btn_frame, text="导出为JSON", command=self._export_analysis_json).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_frame, text="导出为CSV", command=self._export_analysis_csv).pack(side=tk.LEFT, padx=2)
    
    def _create_data_tab(self):
        """数据接收标签页"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="🎯 雷达数据")
        
        # 分割面板
        paned = ttk.PanedWindow(frame, orient=tk.VERTICAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # 上半部分：实时数据
        top_frame = ttk.LabelFrame(paned, text="实时检测数据", padding=5)
        paned.add(top_frame, weight=2)
        
        # 数据显示树
        self.data_tree = ttk.Treeview(top_frame, 
                                      columns=('range', 'velocity', 'angle', 'snr'),
                                      height=15)
        self.data_tree.pack(fill=tk.BOTH, expand=True)
        
        self.data_tree.heading('#0', text='目标ID')
        self.data_tree.heading('range', text='距离(m)')
        self.data_tree.heading('velocity', text='速度(m/s)')
        self.data_tree.heading('angle', text='角度(°)')
        self.data_tree.heading('snr', text='SNR(dB)')
        
        # 下半部分：统计信息
        bottom_frame = ttk.LabelFrame(paned, text="帧统计信息", padding=5)
        paned.add(bottom_frame, weight=1)
        
        self.stats_text = scrolledtext.ScrolledText(bottom_frame, height=10, 
                                                    font=('Consolas', 9),
                                                    bg=self.COLORS['bg_dark'],
                                                    fg=self.COLORS['text_primary'],
                                                    insertbackground=self.COLORS['accent_cyan'],
                                                    selectbackground=self.COLORS['accent_blue'],
                                                    relief='flat')
        self.stats_text.pack(fill=tk.BOTH, expand=True)
    
    def _create_log_tab(self):
        """日志输出标签页"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="📝 日志")
        
        # 工具栏
        toolbar = ttk.Frame(frame)
        toolbar.pack(fill=tk.X, padx=5, pady=5)
        
        ttk.Button(toolbar, text="清空日志", command=self._clear_log).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="保存日志...", command=self._save_log).pack(side=tk.LEFT, padx=2)
        
        # 日志文本框
        self.log_text = scrolledtext.ScrolledText(frame, wrap=tk.WORD, 
                                                  font=('Consolas', 9),
                                                  bg=self.COLORS['bg_dark'],
                                                  fg=self.COLORS['text_primary'],
                                                  insertbackground=self.COLORS['accent_cyan'],
                                                  selectbackground=self.COLORS['accent_blue'],
                                                  relief='flat',
                                                  borderwidth=2)
        self.log_text.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # 日志标签配置（深色主题）
        self.log_text.tag_config('info', foreground='#4fc3f7')
        self.log_text.tag_config('success', foreground='#66bb6a')
        self.log_text.tag_config('warning', foreground='#ffa726')
        self.log_text.tag_config('error', foreground='#ef5350')
        self.log_text.tag_config('debug', foreground='#9ccc65')
    
    def _create_scanner_tab(self):
        """SDK扫描标签页"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="🔍 SDK扫描")
        
        # 扫描控制
        control_frame = ttk.Frame(frame)
        control_frame.pack(fill=tk.X, padx=5, pady=5)
        
        ttk.Label(control_frame, text="SDK路径:").pack(side=tk.LEFT)
        self.sdk_path = tk.StringVar(value=r"C:\ti")
        ttk.Entry(control_frame, textvariable=self.sdk_path, width=40).pack(side=tk.LEFT, padx=5)
        ttk.Button(control_frame, text="浏览...", command=self._browse_sdk).pack(side=tk.LEFT)
        ttk.Button(control_frame, text="➕ 添加目录", command=self._add_sdk_path).pack(side=tk.LEFT, padx=2)
        ttk.Button(control_frame, text="➖ 删除选中", command=self._remove_sdk_path).pack(side=tk.LEFT, padx=2)
        ttk.Button(control_frame, text="开始扫描", command=self._scan_sdk_configs, 
                  style='Accent.TButton').pack(side=tk.LEFT, padx=5)
        
        # 模糊搜索
        search_frame = ttk.Frame(frame)
        search_frame.pack(fill=tk.X, padx=5, pady=5)
        
        ttk.Label(search_frame, text="🔎 搜索:").pack(side=tk.LEFT)
        self.search_var = tk.StringVar()
        self.search_var.trace('w', self._on_search_changed)
        search_entry = ttk.Entry(search_frame, textvariable=self.search_var, width=40)
        search_entry.pack(side=tk.LEFT, padx=5)
        ttk.Button(search_frame, text="清除", command=lambda: self.search_var.set("")).pack(side=tk.LEFT)
        
        # SDK路径列表
        path_list_frame = ttk.LabelFrame(frame, text="📂 已添加的SDK路径", padding=5)
        path_list_frame.pack(fill=tk.X, padx=5, pady=5)
        
        path_scrollbar = ttk.Scrollbar(path_list_frame)
        path_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        self.sdk_path_listbox = tk.Listbox(path_list_frame, height=3,
                                           bg=self.COLORS['bg_dark'],
                                           fg=self.COLORS['text_primary'],
                                           selectbackground=self.COLORS['accent_cyan'],
                                           yscrollcommand=path_scrollbar.set)
        self.sdk_path_listbox.pack(fill=tk.BOTH, expand=True)
        path_scrollbar.config(command=self.sdk_path_listbox.yview)
        
        # 默认添加C:\ti路径
        self.sdk_path_listbox.insert(tk.END, r"C:\ti")
        
        # 扫描结果
        result_frame = ttk.Frame(frame)
        result_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # 滚动条
        scrollbar = ttk.Scrollbar(result_frame)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        # 树形控件 - 修改为绝对路径
        self.scanner_tree = ttk.Treeview(result_frame, 
                                        columns=('path', 'app', 'chip', 'size'),
                                        yscrollcommand=scrollbar.set)
        self.scanner_tree.pack(fill=tk.BOTH, expand=True)
        scrollbar.config(command=self.scanner_tree.yview)
        
        self.scanner_tree.heading('#0', text='文件名')
        self.scanner_tree.heading('path', text='绝对路径')
        self.scanner_tree.heading('app', text='应用')
        self.scanner_tree.heading('chip', text='芯片')
        self.scanner_tree.heading('size', text='大小')
        
        self.scanner_tree.column('#0', width=300)
        self.scanner_tree.column('path', width=500)
        self.scanner_tree.column('app', width=150)
        self.scanner_tree.column('chip', width=100)
        self.scanner_tree.column('size', width=80)
        
        # 操作按钮
        action_frame = ttk.Frame(frame)
        action_frame.pack(fill=tk.X, padx=5, pady=5)
        
        ttk.Button(action_frame, text="📥 添加到配置文件", 
                  command=self._add_selected_to_config).pack(side=tk.LEFT, padx=2)
        ttk.Button(action_frame, text="📋 复制文件名", 
                  command=self._copy_filename).pack(side=tk.LEFT, padx=2)
        ttk.Button(action_frame, text="📋 复制绝对路径", 
                  command=self._copy_absolute_path).pack(side=tk.LEFT, padx=2)
        
        # 右键菜单
        self.scanner_context_menu = tk.Menu(self.scanner_tree, tearoff=0)
        self.scanner_context_menu.add_command(label="📥 添加到配置文件", 
                                             command=self._add_selected_to_config)
        self.scanner_context_menu.add_separator()
        self.scanner_context_menu.add_command(label="📋 复制文件名", 
                                             command=self._copy_filename)
        self.scanner_context_menu.add_command(label="📋 复制绝对路径", 
                                             command=self._copy_absolute_path)
        self.scanner_context_menu.add_separator()
        self.scanner_context_menu.add_command(label="🔍 在资源管理器中显示", 
                                             command=self._show_in_explorer)
        
        # 绑定事件
        self.scanner_tree.bind('<Double-Button-1>', self._load_from_scanner)
        self.scanner_tree.bind('<Button-3>', self._show_scanner_context_menu)
    
    # ========== 事件处理函数 ==========
    
    def _open_config_file(self):
        """打开配置文件"""
        filename = filedialog.askopenfilename(
            title="选择雷达配置文件",
            initialdir=Path(self.default_config).parent,
            filetypes=[("配置文件", "*.cfg"), ("所有文件", "*.*")]
        )
        if filename:
            self._load_config_file(filename)
    
    def _load_config_file(self, filepath):
        """加载配置文件"""
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                content = f.read()
            
            self.current_config_path.set(filepath)
            self.config_filename_label.config(text=Path(filepath).name)
            self.config_text.delete('1.0', tk.END)
            self.config_text.insert('1.0', content)
            
            # 应用语法高亮
            self._apply_syntax_highlighting()
            
            # 自动分析
            self._analyze_config()
            
            # 添加到历史
            self.config_history.append({
                'path': filepath,
                'time': datetime.now().isoformat()
            })
            
            self._log(f"✅ 配置文件已加载: {Path(filepath).name}", 'success')
            
        except Exception as e:
            messagebox.showerror("错误", f"加载配置文件失败:\n{e}")
            self._log(f"❌ 加载配置文件失败: {e}", 'error')
    
    def _apply_syntax_highlighting(self):
        """应用语法高亮"""
        content = self.config_text.get('1.0', tk.END)
        
        # 清除旧标签
        for tag in ['comment', 'command', 'number']:
            self.config_text.tag_remove(tag, '1.0', tk.END)
        
        # 注释高亮
        for match in re.finditer(r'%.*$', content, re.MULTILINE):
            start = self.config_text.search(match.group(), '1.0', tk.END)
            if start:
                end = f"{start}+{len(match.group())}c"
                self.config_text.tag_add('comment', start, end)
        
        # 命令高亮
        commands = ['channelCfg', 'profileCfg', 'frameCfg', 'chirpCfg', 
                   'sensorStart', 'sensorStop', 'flushCfg']
        for cmd in commands:
            start_idx = '1.0'
            while True:
                start_idx = self.config_text.search(cmd, start_idx, tk.END)
                if not start_idx:
                    break
                end_idx = f"{start_idx}+{len(cmd)}c"
                self.config_text.tag_add('command', start_idx, end_idx)
                start_idx = end_idx
    
    def _analyze_config(self):
        """分析配置文件"""
        try:
            config_path = self.current_config_path.get()
            if not config_path or not Path(config_path).exists():
                self._log("⚠️ 请先选择有效的配置文件", 'warning')
                return
            
            self._log("🔍 开始分析配置文件...", 'info')
            
            # 解析配置
            config = self.calculator.parse_config_file(config_path)
            if not config:
                self._log("⚠️ 配置文件解析失败", 'warning')
                return
            
            # 计算性能
            performance = self.calculator.calculate_performance(config)
            
            # 显示结果
            self._display_analysis_results(config, performance)
            
            # 切换到配置分析标签页
            self.notebook.select(1)  # 索引1是配置分析标签页
            
            # 在日志中显示分析摘要
            self._log("", 'info')
            self._log("=" * 50, 'info')
            self._log("📊 配置分析结果摘要", 'success')
            self._log("=" * 50, 'info')
            
            # 通道信息
            rx_count = self.calculator.count_enabled_channels(config.get('rxChannelEn', 0))
            tx_count = self.calculator.count_enabled_channels(config.get('txChannelEn', 0))
            self._log(f"📡 通道: RX={rx_count}个, TX={tx_count}个, 虚拟天线={rx_count*tx_count}个", 'info')
            
            # 性能参数
            if 'range_resolution' in performance:
                self._log(f"📏 距离分辨率: {performance['range_resolution']:.4f} m", 'info')
            if 'max_range' in performance:
                self._log(f"📐 最大检测距离: {performance['max_range']:.2f} m", 'info')
            if 'velocity_resolution' in performance:
                self._log(f"🚀 速度分辨率: {performance['velocity_resolution']:.4f} m/s", 'info')
            if 'max_velocity' in performance:
                self._log(f"⚡ 最大检测速度: {performance['max_velocity']:.2f} m/s", 'info')
            if 'angle_resolution' in performance:
                self._log(f"🎯 角度分辨率: {performance['angle_resolution']:.2f}°", 'info')
            
            # Frame信息
            if 'framePeriodicity' in config:
                frame_rate = 1000 / config.get('framePeriodicity', 1000)
                self._log(f"🎬 帧率: {frame_rate:.2f} FPS", 'info')
            
            self._log("=" * 50, 'info')
            self._log("✅ 配置分析完成！详细结果请查看【配置分析】标签页", 'success')
            self._log("", 'info')
            
        except Exception as e:
            self._log(f"❌ 配置分析失败: {e}", 'error')
            import traceback
            traceback.print_exc()
    
    def _display_analysis_results(self, config: Dict, performance: Dict):
        """显示分析结果"""
        # 清空树
        for item in self.analysis_tree.get_children():
            self.analysis_tree.delete(item)
        
        # 通道配置
        channel_node = self.analysis_tree.insert('', 'end', text='通道配置', open=True)
        rx_count = self.calculator.count_enabled_channels(config.get('rxChannelEn', 0))
        tx_count = self.calculator.count_enabled_channels(config.get('txChannelEn', 0))
        
        self.analysis_tree.insert(channel_node, 'end', text='RX通道', 
                                 values=(rx_count, '个', f"掩码: {config.get('rxChannelEn', 0):04b}"))
        self.analysis_tree.insert(channel_node, 'end', text='TX通道', 
                                 values=(tx_count, '个', f"掩码: {config.get('txChannelEn', 0):04b}"))
        self.analysis_tree.insert(channel_node, 'end', text='虚拟天线', 
                                 values=(rx_count * tx_count, '个', 'RX × TX'))
        
        # Profile配置
        profile_node = self.analysis_tree.insert('', 'end', text='Profile配置', open=True)
        self.analysis_tree.insert(profile_node, 'end', text='起始频率', 
                                 values=(config.get('startFreq', 0), 'GHz', ''))
        self.analysis_tree.insert(profile_node, 'end', text='斜率常数', 
                                 values=(config.get('freqSlopeConst', 0), 'MHz/μs', ''))
        self.analysis_tree.insert(profile_node, 'end', text='采样点数', 
                                 values=(config.get('numAdcSamples', 0), '个', ''))
        self.analysis_tree.insert(profile_node, 'end', text='Chirp时长', 
                                 values=(config.get('rampEndTime', 0), 'μs', ''))
        
        # Frame配置
        frame_node = self.analysis_tree.insert('', 'end', text='Frame配置', open=True)
        self.analysis_tree.insert(frame_node, 'end', text='Chirp数量', 
                                 values=(config.get('numLoops', 0), '个', '每帧'))
        self.analysis_tree.insert(frame_node, 'end', text='帧周期', 
                                 values=(config.get('framePeriodicity', 0), 'ms', ''))
        frame_rate = 1000 / config.get('framePeriodicity', 1000)
        self.analysis_tree.insert(frame_node, 'end', text='帧率', 
                                 values=(f"{frame_rate:.2f}", 'FPS', ''))
        
        # 性能参数
        perf_node = self.analysis_tree.insert('', 'end', text='性能参数', open=True)
        
        if 'range_resolution' in performance:
            self.analysis_tree.insert(perf_node, 'end', text='距离分辨率', 
                                     values=(f"{performance['range_resolution']:.4f}", 'm', 
                                            '越小越精确'))
        
        if 'max_range' in performance:
            self.analysis_tree.insert(perf_node, 'end', text='最大检测距离', 
                                     values=(f"{performance['max_range']:.2f}", 'm', ''))
        
        if 'velocity_resolution' in performance:
            self.analysis_tree.insert(perf_node, 'end', text='速度分辨率', 
                                     values=(f"{performance['velocity_resolution']:.4f}", 'm/s', 
                                            '越小越精确'))
        
        if 'max_velocity' in performance:
            self.analysis_tree.insert(perf_node, 'end', text='最大检测速度', 
                                     values=(f"{performance['max_velocity']:.2f}", 'm/s', ''))
        
        if 'bandwidth' in performance:
            self.analysis_tree.insert(perf_node, 'end', text='带宽', 
                                     values=(f"{performance['bandwidth']:.2f}", 'MHz', ''))
        
        if 'angle_resolution' in performance:
            self.analysis_tree.insert(perf_node, 'end', text='角度分辨率', 
                                     values=(f"{performance['angle_resolution']:.2f}", '°', 
                                            f"{rx_count * tx_count}个虚拟天线"))
    
    def _get_available_ports(self) -> List[str]:
        """获取可用串口列表（包含描述）"""
        ports = serial.tools.list_ports.comports()
        # 返回端口设备名 + 描述
        return [f"{port.device} - {port.description}" for port in ports]
    
    def _refresh_ports(self):
        """刷新串口列表并显示详细信息"""
        try:
            ports = list(serial.tools.list_ports.comports())
            
            if not ports:
                self._log("⚠️ 未找到可用串口", 'warning')
                self.port_combo['values'] = []
                return
            
            # 更新下拉框（包含描述）
            port_items = [f"{port.device} - {port.description}" for port in ports]
            self.port_combo['values'] = port_items
            
            # 显示详细信息
            self._log("✅ 刷新成功！", 'success')
            self._log("", 'info')  # 空行
            
            # 识别AWRL6844设备端口
            for port in ports:
                # 检查VID:PID
                if port.vid and port.pid:
                    vid_pid = f"VID:PID = {port.vid:04X}:{port.pid:04X}"
                    
                    # 识别XDS110设备（AWRL6844的调试器）
                    if port.vid == 0x0451 and port.pid == 0xBEF3:
                        if "Application" in port.description or "User UART" in port.description:
                            self._log(f"🔌 找到烧录端口: {port.device}", 'success')
                            self._log(f"   描述: {port.description}", 'info')
                            self._log(f"   {vid_pid}", 'info')
                            self._log("", 'info')  # 空行
                        elif "Auxiliary" in port.description or "Data Port" in port.description:
                            self._log(f"🔌 找到调试端口: {port.device}", 'success')
                            self._log(f"   描述: {port.description}", 'info')
                            self._log(f"   {vid_pid}", 'info')
                            self._log("", 'info')  # 空行
                        else:
                            self._log(f"🔌 {port.device}", 'info')
                            self._log(f"   描述: {port.description}", 'info')
                            self._log(f"   {vid_pid}", 'info')
                            self._log("", 'info')  # 空行
                    else:
                        self._log(f"🔌 {port.device}", 'info')
                        self._log(f"   描述: {port.description}", 'info')
                        self._log(f"   {vid_pid}", 'info')
                        self._log("", 'info')  # 空行
                else:
                    self._log(f"🔌 {port.device}", 'info')
                    self._log(f"   描述: {port.description}", 'info')
                    self._log("", 'info')  # 空行
            
        except Exception as e:
            self._log(f"❌ 刷新端口失败: {e}", 'error')
    
    def _test_ports(self):
        """测试所有端口连接"""
        try:
            ports = list(serial.tools.list_ports.comports())
            
            if not ports:
                messagebox.showwarning("警告", "未找到可用串口")
                return
            
            # 识别AWRL6844端口
            sbl_port = None
            data_port = None
            
            for port in ports:
                if port.vid == 0x0451 and port.pid == 0xBEF3:
                    if "Application" in port.description or "User UART" in port.description:
                        sbl_port = port.device
                    elif "Auxiliary" in port.description or "Data Port" in port.description:
                        data_port = port.device
            
            # 开始测试
            self._log("=" * 60, 'info')
            self._log("🔍 开始测试所有端口...", 'info')
            self._log("", 'info')
            
            test_results = []
            
            # 测试烧录端口
            if sbl_port:
                self._log(f"📌 测试烧录端口: {sbl_port}", 'info')
                result = self._test_single_port(sbl_port, 115200)
                test_results.append((sbl_port, "烧录端口", result))
            
            # 测试数据输出端口
            if data_port:
                self._log(f"📌 测试数据输出端口: {data_port}", 'info')
                result = self._test_single_port(data_port, 115200)
                test_results.append((data_port, "数据输出端口", result))
            
            # 测试其他端口
            for port in ports:
                if port.device not in [sbl_port, data_port]:
                    self._log(f"📌 测试端口: {port.device}", 'info')
                    result = self._test_single_port(port.device, 115200)
                    test_results.append((port.device, "其他端口", result))
            
            # 显示汇总
            self._log("", 'info')
            self._log("=" * 60, 'info')
            self._log("📊 端口测试结果汇总:", 'info')
            
            success_count = 0
            fail_count = 0
            
            for port, port_type, result in test_results:
                if result:
                    self._log(f"  ✅ {port} ({port_type}): 连接正常", 'success')
                    success_count += 1
                else:
                    self._log(f"  ❌ {port} ({port_type}): 连接失败", 'error')
                    fail_count += 1
            
            self._log("=" * 60, 'info')
            
            # 弹出结果窗口
            result_text = "端口测试结果汇总\n\n"
            for port, port_type, result in test_results:
                status = "✅ 连接正常" if result else "❌ 连接失败"
                result_text += f"{port} ({port_type}): {status}\n"
            
            result_text += f"\n总计: {success_count} 个成功, {fail_count} 个失败"
            
            messagebox.showinfo("端口测试结果", result_text)
            
        except Exception as e:
            self._log(f"❌ 端口测试失败: {e}", 'error')
            messagebox.showerror("错误", f"端口测试失败:\\n{e}")
    
    def _test_single_port(self, port: str, baudrate: int) -> bool:
        """测试单个端口"""
        try:
            test_serial = serial.Serial(port, baudrate, timeout=0.5)
            time.sleep(0.1)  # 短暂延迟
            test_serial.close()
            self._log(f"✅ 端口 {port} 连接正常！", 'success')
            self._log("", 'info')
            return True
        except Exception as e:
            self._log(f"❌ 端口 {port} 连接失败: {e}", 'error')
            self._log("", 'info')
            return False
    
    def _connect_port(self):
        """连接串口"""
        try:
            port_selection = self.selected_port.get()
            # 从选择中提取端口名称（COM3 - 描述 -> COM3）
            port = port_selection.split(' - ')[0] if ' - ' in port_selection else port_selection
            baud = self.baudrate.get()
            
            self.serial_port = serial.Serial(port, baud, timeout=1)
            
            self.port_status.config(text=f"● 已连接 ({port})", foreground="green")
            self.btn_connect.config(state='disabled')
            self.btn_disconnect.config(state='normal')
            
            self._log(f"✅ 串口已连接: {port} @ {baud}", 'success')
            
        except Exception as e:
            messagebox.showerror("错误", f"串口连接失败:\n{e}")
            self._log(f"❌ 串口连接失败: {e}", 'error')
    
    def _disconnect_port(self):
        """断开串口"""
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
            self.serial_port = None
            
            self.port_status.config(text="● 未连接", foreground="red")
            self.btn_connect.config(state='normal')
            self.btn_disconnect.config(state='disabled')
            
            self._log("🔌 串口已断开", 'info')
    
    def _write_config(self):
        """写入配置到雷达"""
        if not self.serial_port or not self.serial_port.is_open:
            messagebox.showwarning("警告", "请先连接串口")
            return
        
        try:
            target = self.write_target.get()
            config_content = self.config_text.get('1.0', tk.END)
            
            self._log(f"📤 开始写入配置 (目标: {target})...", 'info')
            
            # 停止雷达
            self.serial_port.write(b'sensorStop\n')
            time.sleep(0.1)
            
            # 清空配置
            self.serial_port.write(b'flushCfg\n')
            time.sleep(0.1)
            
            # 发送配置
            line_count = 0
            for line in config_content.split('\n'):
                line = line.strip()
                if line and not line.startswith('%') and not line.startswith('#'):
                    self.serial_port.write(line.encode() + b'\n')
                    time.sleep(0.05)
                    line_count += 1
                    self._log(f"  发送: {line}", 'debug')
            
            # Flash保存（如果支持）
            if target == "Flash":
                self.serial_port.write(b'setFlashRecord\n')
                time.sleep(0.5)
                self._log("💾 配置已保存到Flash", 'info')
            
            self._log(f"✅ 配置写入完成 (共{line_count}条命令)", 'success')
            messagebox.showinfo("成功", f"配置已写入到雷达 ({target})")
            
        except Exception as e:
            self._log(f"❌ 配置写入失败: {e}", 'error')
            messagebox.showerror("错误", f"配置写入失败:\n{e}")
    
    def _read_config(self):
        """读取雷达配置"""
        if not self.serial_port or not self.serial_port.is_open:
            messagebox.showwarning("警告", "请先连接串口")
            return
        
        try:
            self._log("📥 尝试读取雷达配置...", 'info')
            
            # 尝试读取Flash配置（仅部分Demo支持）
            self.serial_port.write(b'getFlashRecord\n')
            time.sleep(0.2)
            
            response = self.serial_port.read(self.serial_port.in_waiting)
            
            if response:
                config_text = response.decode('utf-8', errors='ignore')
                self._log("✅ 读取到配置数据:", 'success')
                self._log(config_text, 'debug')
                
                # 显示在对话框
                result_window = tk.Toplevel(self.root)
                result_window.title("读取的配置")
                result_window.geometry("600x400")
                
                text = scrolledtext.ScrolledText(result_window, wrap=tk.WORD)
                text.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
                text.insert('1.0', config_text)
            else:
                self._log("⚠️ 未读取到配置数据（固件可能不支持）", 'warning')
                messagebox.showinfo("提示", 
                    "未读取到配置数据。\n\n"
                    "注意：大多数固件不支持读取配置参数。\n"
                    "只有部分Demo（如停车传感器）支持Flash配置读取。")
            
        except Exception as e:
            self._log(f"❌ 读取配置失败: {e}", 'error')
            messagebox.showerror("错误", f"读取配置失败:\n{e}")
    
    def _send_command(self, command: str):
        """发送命令到雷达"""
        if not self.serial_port or not self.serial_port.is_open:
            messagebox.showwarning("警告", "请先连接串口")
            return
        
        try:
            self.serial_port.write(command.encode() + b'\n')
            self._log(f"📨 命令已发送: {command}", 'info')
        except Exception as e:
            self._log(f"❌ 命令发送失败: {e}", 'error')
    
    def _start_receiving_data(self):
        """开始接收雷达数据"""
        if not self.serial_port or not self.serial_port.is_open:
            messagebox.showwarning("警告", "请先连接串口")
            return
        
        self.receiving_data = True
        self._log("🎯 开始接收雷达数据...", 'info')
        
        # 在后台线程接收数据
        threading.Thread(target=self._receive_data_loop, daemon=True).start()
    
    def _stop_receiving_data(self):
        """停止接收数据"""
        self.receiving_data = False
        self._log("🛑 停止接收数据", 'info')
    
    def _receive_data_loop(self):
        """数据接收循环"""
        frame_count = 0
        
        while self.receiving_data:
            try:
                # 查找Magic Word
                magic_word = self.serial_port.read(8)
                if magic_word == b'\x02\x01\x04\x03\x06\x05\x08\x07':
                    # 读取帧头
                    header_data = self.serial_port.read(32)
                    if len(header_data) == 32:
                        header = struct.unpack('8I', header_data)
                        frame_num = header[3]
                        num_objects = header[5]
                        num_tlvs = header[6]
                        
                        frame_count += 1
                        
                        # 更新统计信息
                        stats = f"帧号: {frame_num}\n检测目标: {num_objects}\nTLV数量: {num_tlvs}\n"
                        self.stats_text.delete('1.0', tk.END)
                        self.stats_text.insert('1.0', stats)
                        
                        # 读取TLV数据
                        objects = []
                        for _ in range(num_tlvs):
                            tlv_header = self.serial_port.read(8)
                            if len(tlv_header) == 8:
                                tlv_type, tlv_length = struct.unpack('II', tlv_header)
                                tlv_data = self.serial_port.read(tlv_length)
                                
                                # 解析目标列表 (TLV Type 1)
                                if tlv_type == 1 and num_objects > 0:
                                    objects = self._parse_detected_objects(tlv_data, num_objects)
                        
                        # 更新数据树
                        self._update_data_tree(objects)
                        
                        if frame_count % 10 == 0:
                            self._log(f"📊 已接收 {frame_count} 帧", 'debug')
                
            except Exception as e:
                if self.receiving_data:
                    self._log(f"⚠️ 数据接收错误: {e}", 'warning')
                break
    
    def _parse_detected_objects(self, data: bytes, num_objects: int) -> List[Dict]:
        """解析检测目标"""
        objects = []
        try:
            # 每个目标16字节 (x, y, z, velocity)
            for i in range(num_objects):
                if len(data) >= (i + 1) * 16:
                    obj_data = struct.unpack('4f', data[i*16:(i+1)*16])
                    objects.append({
                        'x': obj_data[0],
                        'y': obj_data[1],
                        'z': obj_data[2],
                        'velocity': obj_data[3]
                    })
        except Exception as e:
            self._log(f"⚠️ 目标解析错误: {e}", 'warning')
        
        return objects
    
    def _update_data_tree(self, objects: List[Dict]):
        """更新数据显示树"""
        # 清空树
        for item in self.data_tree.get_children():
            self.data_tree.delete(item)
        
        # 添加新数据
        for i, obj in enumerate(objects):
            import math
            range_val = math.sqrt(obj['x']**2 + obj['y']**2 + obj['z']**2)
            angle = math.degrees(math.atan2(obj['y'], obj['x']))
            
            self.data_tree.insert('', 'end', text=f"目标 {i+1}",
                                 values=(f"{range_val:.2f}", 
                                        f"{obj['velocity']:.2f}",
                                        f"{angle:.1f}",
                                        "N/A"))
    
    def _infer_config_from_data(self):
        """从数据推断配置"""
        self._log("🔍 开始从数据推断配置...", 'info')
        
        # TODO: 实现配置推断逻辑
        # 1. 测量帧率
        # 2. 分析虚拟天线数
        # 3. 推断距离范围
        
        messagebox.showinfo("提示", "配置推断功能开发中...")
    
    def _scan_sdk_configs(self):
        """扫描SDK配置文件"""
        # 获取所有SDK路径
        sdk_paths = [self.sdk_path_listbox.get(i) for i in range(self.sdk_path_listbox.size())]
        
        if not sdk_paths:
            messagebox.showwarning("警告", "请先添加SDK路径")
            return
        
        self._log(f"🔍 扫描 {len(sdk_paths)} 个SDK路径", 'info')
        
        # 在后台线程扫描
        threading.Thread(target=self._scan_sdk_thread, args=(sdk_paths,), daemon=True).start()
    
    def _scan_sdk_thread(self, sdk_paths: List[str]):
        """SDK扫描线程"""
        all_configs = []
        
        for sdk_path in sdk_paths:
            if not Path(sdk_path).exists():
                self._log(f"⚠️ 路径不存在: {sdk_path}", 'warning')
                continue
            
            try:
                configs = self.scanner.scan_directory(sdk_path)
                all_configs.extend(configs)
                self._log(f"✅ {sdk_path}: 找到 {len(configs)} 个配置", 'success')
            except Exception as e:
                self._log(f"❌ 扫描 {sdk_path} 失败: {e}", 'error')
        
        # 更新树形控件
        self.root.after(0, self._update_scanner_tree, all_configs)
        self._log(f"✅ 扫描完成，总计 {len(all_configs)} 个配置文件", 'success')
    
    def _update_scanner_tree(self, configs: List[Dict]):
        """更新扫描结果树"""
        # 清空树
        for item in self.scanner_tree.get_children():
            self.scanner_tree.delete(item)
        
        # 保存完整配置数据
        self.scanner_configs = configs
        
        # 添加配置文件 - 使用绝对路径
        for cfg in configs:
            abs_path = str(Path(cfg.get('absolute_path', cfg.get('path', ''))))
            self.scanner_tree.insert('', 'end', text=cfg['name'],
                                    values=(abs_path,
                                           cfg.get('application', 'unknown'),
                                           cfg.get('chip', 'unknown'),
                                           f"{cfg.get('size', 0)} B"),
                                    tags=(abs_path,))  # 将绝对路径存储在tags中
    
    def _on_search_changed(self, *args):
        """搜索框内容改变事件"""
        search_text = self.search_var.get().lower()
        
        if not hasattr(self, 'scanner_configs'):
            return
        
        # 清空树
        for item in self.scanner_tree.get_children():
            self.scanner_tree.delete(item)
        
        # 过滤配置文件
        filtered_configs = []
        for cfg in self.scanner_configs:
            # 搜索文件名、路径、应用名、芯片名
            searchable = f"{cfg['name']} {cfg.get('path', '')} {cfg.get('application', '')} {cfg.get('chip', '')}".lower()
            if search_text in searchable:
                filtered_configs.append(cfg)
        
        # 显示过滤结果
        for cfg in filtered_configs:
            abs_path = str(Path(cfg.get('absolute_path', cfg.get('path', ''))))
            self.scanner_tree.insert('', 'end', text=cfg['name'],
                                    values=(abs_path,
                                           cfg.get('application', 'unknown'),
                                           cfg.get('chip', 'unknown'),
                                           f"{cfg.get('size', 0)} B"),
                                    tags=(abs_path,))
        
        self._log(f"🔍 搜索 '{search_text}': 找到 {len(filtered_configs)} 个结果", 'info')
    
    def _add_sdk_path(self):
        """添加SDK路径"""
        directory = filedialog.askdirectory(title="选择SDK根目录",
                                            initialdir=self.sdk_path.get())
        if directory:
            # 检查是否已存在
            existing_paths = [self.sdk_path_listbox.get(i) for i in range(self.sdk_path_listbox.size())]
            if directory not in existing_paths:
                self.sdk_path_listbox.insert(tk.END, directory)
                self._log(f"✅ 已添加SDK路径: {directory}", 'success')
            else:
                messagebox.showinfo("提示", "该路径已存在")
    
    def _remove_sdk_path(self):
        """删除选中的SDK路径"""
        selection = self.sdk_path_listbox.curselection()
        if selection:
            path = self.sdk_path_listbox.get(selection[0])
            self.sdk_path_listbox.delete(selection[0])
            self._log(f"🗑️ 已删除SDK路径: {path}", 'info')
        else:
            messagebox.showwarning("警告", "请先选择要删除的路径")
    
    def _add_selected_to_config(self):
        """将选中的配置文件添加到配置文件选择"""
        selection = self.scanner_tree.selection()
        if not selection:
            messagebox.showwarning("警告", "请先选择一个配置文件")
            return
        
        item = self.scanner_tree.item(selection[0])
        abs_path = item['tags'][0] if item['tags'] else item['values'][0]
        
        if Path(abs_path).exists():
            self._load_config_file(abs_path)
            messagebox.showinfo("成功", f"已加载配置文件:\n{Path(abs_path).name}")
        else:
            messagebox.showerror("错误", f"文件不存在:\n{abs_path}")
    
    def _copy_filename(self):
        """复制选中文件的文件名"""
        selection = self.scanner_tree.selection()
        if not selection:
            messagebox.showwarning("警告", "请先选择一个文件")
            return
        
        item = self.scanner_tree.item(selection[0])
        filename = item['text']
        
        self.root.clipboard_clear()
        self.root.clipboard_append(filename)
        self._log(f"📋 已复制文件名: {filename}", 'success')
    
    def _copy_absolute_path(self):
        """复制选中文件的绝对路径"""
        selection = self.scanner_tree.selection()
        if not selection:
            messagebox.showwarning("警告", "请先选择一个文件")
            return
        
        item = self.scanner_tree.item(selection[0])
        abs_path = item['tags'][0] if item['tags'] else item['values'][0]
        
        self.root.clipboard_clear()
        self.root.clipboard_append(abs_path)
        self._log(f"📋 已复制绝对路径: {abs_path}", 'success')
    
    def _show_in_explorer(self):
        """在资源管理器中显示文件"""
        selection = self.scanner_tree.selection()
        if not selection:
            return
        
        item = self.scanner_tree.item(selection[0])
        abs_path = item['tags'][0] if item['tags'] else item['values'][0]
        
        if Path(abs_path).exists():
            os.system(f'explorer /select,"{abs_path}"')
        else:
            messagebox.showerror("错误", f"文件不存在:\n{abs_path}")
    
    def _show_scanner_context_menu(self, event):
        """显示右键菜单"""
        # 选中右键点击的项
        item = self.scanner_tree.identify_row(event.y)
        if item:
            self.scanner_tree.selection_set(item)
            self.scanner_context_menu.post(event.x_root, event.y_root)
    
    def _load_from_scanner(self, event):
        """从扫描结果加载配置"""
        selection = self.scanner_tree.selection()
        if selection:
            item = self.scanner_tree.item(selection[0])
            abs_path = item['tags'][0] if item['tags'] else item['values'][0]
            
            if Path(abs_path).exists():
                self._load_config_file(abs_path)
            else:
                messagebox.showerror("错误", f"文件不存在:\n{abs_path}")
    
    def _browse_sdk(self):
        """浏览SDK路径"""
        directory = filedialog.askdirectory(title="选择SDK根目录",
                                            initialdir=self.sdk_path.get())
        if directory:
            self.sdk_path.set(directory)
    
    def _reload_config(self):
        """重新加载配置文件"""
        filepath = self.current_config_path.get()
        if Path(filepath).exists():
            self._load_config_file(filepath)
    
    def _save_config_as(self):
        """另存配置文件"""
        content = self.config_text.get('1.0', tk.END)
        filename = filedialog.asksaveasfilename(
            title="保存配置文件",
            defaultextension=".cfg",
            filetypes=[("配置文件", "*.cfg"), ("所有文件", "*.*")]
        )
        if filename:
            try:
                with open(filename, 'w', encoding='utf-8') as f:
                    f.write(content)
                self._log(f"✅ 配置已保存: {filename}", 'success')
            except Exception as e:
                self._log(f"❌ 保存失败: {e}", 'error')
    
    def _save_analysis(self):
        """保存分析结果"""
        # TODO: 实现
        pass
    
    def _export_analysis_json(self):
        """导出分析结果为JSON"""
        # TODO: 实现
        pass
    
    def _export_analysis_csv(self):
        """导出分析结果为CSV"""
        # TODO: 实现
        pass
    
    def _show_history(self):
        """显示配置历史"""
        if not self.config_history:
            messagebox.showinfo("提示", "暂无历史记录")
            return
        
        # 创建历史窗口
        history_window = tk.Toplevel(self.root)
        history_window.title("配置历史记录")
        history_window.geometry("600x400")
        
        # 列表框
        listbox = tk.Listbox(history_window)
        listbox.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        for record in reversed(self.config_history):
            listbox.insert(tk.END, f"{record['time']} - {record['path']}")
    
    def _show_recent_configs(self):
        """显示最近使用的配置"""
        # TODO: 实现
        pass
    
    def _on_default_config_selected(self, event):
        """默认配置下拉框选择事件"""
        selected_index = self.default_config_var.get()
        for i, path in enumerate(self.default_configs):
            if Path(path).name == selected_index:
                # 显示完整路径
                self.current_config_path.set(path)
                self.config_filename_label.config(text=Path(path).name)
                break
    
    def _load_selected_default(self):
        """加载选中的默认配置"""
        selected = self.default_config_var.get()
        if not selected:
            messagebox.showwarning("警告", "请先选择一个配置文件")
            return
        
        for path in self.default_configs:
            if Path(path).name == selected:
                if Path(path).exists():
                    self._load_config_file(path)
                else:
                    messagebox.showerror("错误", f"配置文件不存在:\n{path}")
                break
    
    def _clear_log(self):
        """清空日志"""
        self.log_text.delete('1.0', tk.END)
        self._log("🗑️ 日志已清空", 'info')
    
    def _save_log(self):
        """保存日志"""
        content = self.log_text.get('1.0', tk.END)
        filename = filedialog.asksaveasfilename(
            title="保存日志",
            defaultextension=".log",
            filetypes=[("日志文件", "*.log"), ("文本文件", "*.txt")]
        )
        if filename:
            try:
                with open(filename, 'w', encoding='utf-8') as f:
                    f.write(content)
                self._log(f"✅ 日志已保存: {filename}", 'success')
            except Exception as e:
                self._log(f"❌ 保存失败: {e}", 'error')
    
    def _show_help(self):
        """显示帮助"""
        help_text = """
        AWRL6844雷达配置工具 - 使用说明
        
        1. 配置文件管理
           - 打开/保存配置文件
           - 默认加载xWRL6844_4T4R_tdm.cfg
           - 语法高亮显示
        
        2. 串口通信
           - 连接COM4端口(115200)
           - 写入配置到RAM或Flash
           - 读取Flash配置（部分Demo支持）
        
        3. 配置分析
           - 自动计算性能参数
           - 显示距离/速度/角度分辨率
           - 导出分析结果
        
        4. 雷达数据
           - 实时接收检测数据
           - 显示目标信息
           - 帧统计信息
        
        5. SDK扫描
           - 扫描SDK中的所有配置文件
           - 按应用/芯片分类
           - 双击加载配置
        
        更多信息请参考README.md文档
        """
        
        help_window = tk.Toplevel(self.root)
        help_window.title("使用说明")
        help_window.geometry("600x500")
        
        text = scrolledtext.ScrolledText(help_window, wrap=tk.WORD)
        text.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        text.insert('1.0', help_text)
        text.config(state='disabled')
    
    def _show_about(self):
        """显示关于"""
        about_text = """
        AWRL6844雷达配置工具
        版本: v1.0.0
        
        功能特性:
        • 配置文件读写与管理
        • 性能参数自动计算
        • 雷达数据实时接收
        • SDK配置文件扫描
        • 配置推断分析
        
        开发: Wisefido
        日期: 2025-12-20
        
        适用硬件: AWRL6844-EVM
        SDK版本: radar_toolbox 3.30.00.06+
        """
        
        messagebox.showinfo("关于", about_text)
    
    def _log(self, message: str, level: str = 'info'):
        """写入日志"""
        timestamp = datetime.now().strftime("%H:%M:%S")
        log_msg = f"[{timestamp}] {message}\n"
        
        self.log_text.insert(tk.END, log_msg, level)
        self.log_text.see(tk.END)
        self.root.update_idletasks()


def check_existing_process():
    """
    检查是否已有radar_config_tool.py进程在运行
    
    Returns:
        list: 已存在的进程列表
    """
    current_pid = os.getpid()
    script_name = "radar_config_tool.py"
    existing_processes = []
    
    try:
        for proc in psutil.process_iter(['pid', 'name', 'cmdline', 'create_time']):
            try:
                # 检查是否是Python进程
                if proc.info['name'] and 'python' in proc.info['name'].lower():
                    cmdline = proc.info['cmdline']
                    if cmdline:
                        # 检查命令行参数中是否包含radar_config_tool.py
                        cmdline_str = ' '.join(cmdline)
                        if script_name in cmdline_str and proc.info['pid'] != current_pid:
                            # 排除父进程（没有--child-process参数的）
                            if '--child-process' in cmdline_str:
                                existing_processes.append({
                                    'pid': proc.info['pid'],
                                    'cmdline': cmdline_str,
                                    'create_time': proc.info['create_time']
                                })
            except (psutil.NoSuchProcess, psutil.AccessDenied):
                continue
    except Exception as e:
        print(f"检查进程时出错: {e}")
    
    return existing_processes


def kill_process(pid):
    """
    终止指定PID的进程
    
    Args:
        pid: 进程ID
        
    Returns:
        bool: 是否成功终止
    """
    try:
        proc = psutil.Process(pid)
        
        # 先尝试优雅终止
        proc.terminate()
        try:
            proc.wait(timeout=2)  # 等待2秒
            return True
        except psutil.TimeoutExpired:
            pass
        
        # 如果还在运行，强制kill
        if proc.is_running():
            proc.kill()
            proc.wait(timeout=2)
        
        return True
    except psutil.NoSuchProcess:
        # 进程已不存在
        return True
    except Exception as e:
        print(f"❌ 终止进程 {pid} 失败: {e}")
        return False


def main():
    """主函数 - 仅在子进程中运行GUI"""
    # 创建主窗口
    root = tk.Tk()
    
    # 设置主题样式
    style = ttk.Style()
    style.theme_use('clam')
    
    # 创建应用
    app = RadarConfigTool(root)
    
    # 运行
    root.mainloop()


if __name__ == '__main__':
    # 检查是否是子进程标记
    if '--child-process' not in sys.argv:
        # ====== 父进程：检测旧进程、处理关闭、启动子进程 ======
        print("🚀 启动 AWRL6844 雷达配置工具...")
        
        # 检查旧进程
        existing_processes = check_existing_process()
        
        if existing_processes:
            # 创建临时窗口显示提示
            temp_root = tk.Tk()
            temp_root.withdraw()
            
            msg = f"""⚠️ 检测到 {len(existing_processes)} 个旧窗口正在运行

是否关闭旧窗口并启动新窗口？

点击"是"：关闭旧窗口，启动新窗口
点击"否"：取消启动，保留旧窗口"""
            
            result = messagebox.askyesno(
                "检测到旧窗口",
                msg,
                icon='warning',
                parent=temp_root
            )
            
            temp_root.destroy()
            
            if not result:
                # 用户选择不关闭
                print("❌ 用户取消启动")
                sys.exit(0)
            
            # 关闭旧进程
            print(f"\n⚠️  检测到 {len(existing_processes)} 个旧窗口")
            print("🔄 正在关闭旧窗口...")
            
            success_count = 0
            for proc in existing_processes:
                if kill_process(proc['pid']):
                    success_count += 1
                    print(f"   ✅ 已关闭进程 PID: {proc['pid']}")
                else:
                    print(f"   ❌ 无法关闭进程 PID: {proc['pid']}")
            
            if success_count > 0:
                time.sleep(0.5)
                print(f"\n✅ 成功关闭 {success_count}/{len(existing_processes)} 个旧进程")
            else:
                print("\n⚠️  未能关闭任何旧进程，但继续启动")
        
        # 启动新的子进程
        script_path = os.path.abspath(__file__)
        python_exe = sys.executable
        
        DETACHED_PROCESS = 0x00000008
        subprocess.Popen(
            [python_exe, script_path, '--child-process'],
            creationflags=DETACHED_PROCESS,
            stdin=subprocess.DEVNULL,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL
        )
        
        print("\n✅ 雷达配置工具已启动！")
        print("💡 命令行已完成，GUI在后台运行")
        sys.exit(0)
    else:
        # ====== 子进程：仅运行GUI ======
        main()
