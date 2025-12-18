#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
固件管理标签页 - v1.3.5
AWRL6844EVM 固件智能管理系统（集成版）
功能：扫描、筛选、匹配应用固件、SBL、雷达配置文件
"""

import tkinter as tk
from tkinter import ttk, messagebox, filedialog, scrolledtext
import os
import sys
import threading
from pathlib import Path

# 导入固件匹配器（从同目录父级导入）
sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
try:
    from awrl6844_firmware_matcher import (
        AWRL6844FirmwareMatcher, FirmwareInfo, SBLInfo, ConfigInfo
    )
except ImportError:
    messagebox.showerror(
        "模块导入错误",
        "无法导入 awrl6844_firmware_matcher 模块\n"
        "请确保 awrl6844_firmware_matcher.py 在 Scripts 目录下"
    )


class FirmwareManagerTab:
    """固件管理标签页 - 集成AWRL6844固件智能管理系统"""
    
    def __init__(self, parent, main_app):
        """
        初始化固件管理标签页
        
        Args:
            parent: 父容器（Frame）
            main_app: 主应用实例
        """
        self.parent = parent
        self.main_app = main_app
        self.matcher = AWRL6844FirmwareMatcher()
        
        # 默认扫描目录
        self.scan_directories = [
            r"C:\ti\MMWAVE_L_SDK_06_01_00_01",
            r"C:\ti\radar_toolbox_3_30_00_06"
        ]
        
        # 扫描状态
        self.is_scanning = False
        
        # 创建界面
        self.create_widgets()
        
    def create_widgets(self):
        """创建界面组件"""
        
        # ============ 顶部控制区 ============
        control_frame = ttk.LabelFrame(self.parent, text="📡 扫描控制", padding=10)
        control_frame.pack(fill=tk.X, padx=10, pady=(10, 5))
        
        # 目录管理行
        dir_row = ttk.Frame(control_frame)
        dir_row.pack(fill=tk.X, pady=(0, 5))
        
        ttk.Label(dir_row, text="扫描目录:", width=10).pack(side=tk.LEFT)
        
        self.dir_combo = ttk.Combobox(dir_row, values=self.scan_directories, width=60)
        self.dir_combo.pack(side=tk.LEFT, padx=5, fill=tk.X, expand=True)
        if self.scan_directories:
            self.dir_combo.current(0)
        
        ttk.Button(dir_row, text="➕ 添加", command=self.add_directory, width=8).pack(side=tk.LEFT, padx=2)
        ttk.Button(dir_row, text="➖ 删除", command=self.remove_directory, width=8).pack(side=tk.LEFT, padx=2)
        
        # 扫描按钮行
        btn_row = ttk.Frame(control_frame)
        btn_row.pack(fill=tk.X, pady=5)
        
        self.btn_scan = ttk.Button(btn_row, text="🔍 开始扫描", command=self.start_scan)
        self.btn_scan.pack(side=tk.LEFT, padx=5, fill=tk.X, expand=True)
        
        ttk.Button(btn_row, text="🗑️ 清空结果", command=self.clear_results).pack(
            side=tk.LEFT, padx=5, fill=tk.X, expand=True
        )
        
        # 进度条
        self.progress = ttk.Progressbar(control_frame, mode='indeterminate')
        self.progress.pack(fill=tk.X, pady=5)
        
        # 统计信息行
        stats_row = ttk.Frame(control_frame)
        stats_row.pack(fill=tk.X, pady=5)
        
        self.lbl_app = ttk.Label(stats_row, text="应用固件: 0", font=('Arial', 9, 'bold'))
        self.lbl_app.pack(side=tk.LEFT, padx=10)
        
        ttk.Label(stats_row, text="|").pack(side=tk.LEFT)
        
        self.lbl_sbl = ttk.Label(stats_row, text="SBL固件: 0", font=('Arial', 9, 'bold'))
        self.lbl_sbl.pack(side=tk.LEFT, padx=10)
        
        ttk.Label(stats_row, text="|").pack(side=tk.LEFT)
        
        self.lbl_config = ttk.Label(stats_row, text="雷达配置: 0", font=('Arial', 9, 'bold'))
        self.lbl_config.pack(side=tk.LEFT, padx=10)
        
        # ============ 主内容区（选项卡） ============
        self.notebook = ttk.Notebook(self.parent)
        self.notebook.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)
        
        # 创建各标签页
        self.create_firmware_tab()
        self.create_sbl_tab()
        self.create_config_tab()
        self.create_match_tab()
        
    def create_firmware_tab(self):
        """创建应用固件标签页"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="📦 应用固件")
        
        # 筛选区
        filter_frame = ttk.LabelFrame(frame, text="筛选条件", padding=5)
        filter_frame.pack(fill=tk.X, padx=5, pady=5)
        
        filter_row = ttk.Frame(filter_frame)
        filter_row.pack(fill=tk.X)
        
        ttk.Label(filter_row, text="类别:").pack(side=tk.LEFT, padx=5)
        self.fw_category = ttk.Combobox(filter_row, values=["全部"], width=15, state='readonly')
        self.fw_category.current(0)
        self.fw_category.bind('<<ComboboxSelected>>', lambda e: self.filter_firmwares())
        self.fw_category.pack(side=tk.LEFT, padx=5)
        
        ttk.Label(filter_row, text="处理器:").pack(side=tk.LEFT, padx=5)
        self.fw_processor = ttk.Combobox(filter_row, values=["全部"], width=20, state='readonly')
        self.fw_processor.current(0)
        self.fw_processor.bind('<<ComboboxSelected>>', lambda e: self.filter_firmwares())
        self.fw_processor.pack(side=tk.LEFT, padx=5)
        
        ttk.Label(filter_row, text="搜索:").pack(side=tk.LEFT, padx=5)
        self.fw_search = ttk.Entry(filter_row, width=30)
        self.fw_search.bind('<KeyRelease>', lambda e: self.filter_firmwares())
        self.fw_search.pack(side=tk.LEFT, padx=5, fill=tk.X, expand=True)
        
        # 固件列表（Treeview）
        list_frame = ttk.Frame(frame)
        list_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # 滚动条
        scrollbar = ttk.Scrollbar(list_frame)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        self.fw_tree = ttk.Treeview(
            list_frame,
            columns=('filename', 'category', 'subcategory', 'processor', 'version'),
            show='headings',
            yscrollcommand=scrollbar.set
        )
        scrollbar.config(command=self.fw_tree.yview)
        
        self.fw_tree.heading('filename', text='文件名')
        self.fw_tree.heading('category', text='类别')
        self.fw_tree.heading('subcategory', text='子类别')
        self.fw_tree.heading('processor', text='处理器')
        self.fw_tree.heading('version', text='版本')
        
        self.fw_tree.column('filename', width=300)
        self.fw_tree.column('category', width=100)
        self.fw_tree.column('subcategory', width=150)
        self.fw_tree.column('processor', width=180)
        self.fw_tree.column('version', width=100)
        
        self.fw_tree.pack(fill=tk.BOTH, expand=True)
        self.fw_tree.bind('<<TreeviewSelect>>', self.on_firmware_selected)
        
        # 详细信息
        detail_frame = ttk.LabelFrame(frame, text="详细信息", padding=5)
        detail_frame.pack(fill=tk.X, padx=5, pady=5)
        
        self.fw_detail = scrolledtext.ScrolledText(detail_frame, height=6, wrap=tk.WORD)
        self.fw_detail.pack(fill=tk.BOTH, expand=True)
        
    def create_sbl_tab(self):
        """创建SBL固件标签页"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="🔧 SBL固件")
        
        # 说明
        info_frame = ttk.Frame(frame)
        info_frame.pack(fill=tk.X, padx=5, pady=5)
        
        info_text = (
            "🔧 SBL固件说明:\n"
            "SBL (Secondary Bootloader) 是芯片启动的第一级程序，负责从Flash加载应用固件。\n"
            "推荐使用: 标准版SBL，功能完整，适用于绝大多数应用场景。"
        )
        ttk.Label(info_frame, text=info_text, justify=tk.LEFT, wraplength=800).pack(anchor=tk.W)
        
        # SBL列表
        list_frame = ttk.Frame(frame)
        list_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        scrollbar = ttk.Scrollbar(list_frame)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        self.sbl_tree = ttk.Treeview(
            list_frame,
            columns=('filename', 'variant', 'flash_addr', 'size', 'description'),
            show='headings',
            yscrollcommand=scrollbar.set
        )
        scrollbar.config(command=self.sbl_tree.yview)
        
        self.sbl_tree.heading('filename', text='文件名')
        self.sbl_tree.heading('variant', text='变体')
        self.sbl_tree.heading('flash_addr', text='Flash地址')
        self.sbl_tree.heading('size', text='大小')
        self.sbl_tree.heading('description', text='说明')
        
        self.sbl_tree.column('filename', width=250)
        self.sbl_tree.column('variant', width=100)
        self.sbl_tree.column('flash_addr', width=120)
        self.sbl_tree.column('size', width=100)
        self.sbl_tree.column('description', width=300)
        
        self.sbl_tree.pack(fill=tk.BOTH, expand=True)
        self.sbl_tree.bind('<<TreeviewSelect>>', self.on_sbl_selected)
        
        # 详细信息
        detail_frame = ttk.LabelFrame(frame, text="详细信息", padding=5)
        detail_frame.pack(fill=tk.X, padx=5, pady=5)
        
        self.sbl_detail = scrolledtext.ScrolledText(detail_frame, height=4, wrap=tk.WORD)
        self.sbl_detail.pack(fill=tk.BOTH, expand=True)
        
    def create_config_tab(self):
        """创建雷达配置标签页"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="⚙️ 雷达配置")
        
        # 筛选区
        filter_frame = ttk.LabelFrame(frame, text="筛选条件", padding=5)
        filter_frame.pack(fill=tk.X, padx=5, pady=5)
        
        filter_row = ttk.Frame(filter_frame)
        filter_row.pack(fill=tk.X)
        
        ttk.Label(filter_row, text="应用场景:").pack(side=tk.LEFT, padx=5)
        self.cfg_app = ttk.Combobox(filter_row, values=["全部"], width=15, state='readonly')
        self.cfg_app.current(0)
        self.cfg_app.bind('<<ComboboxSelected>>', lambda e: self.filter_configs())
        self.cfg_app.pack(side=tk.LEFT, padx=5)
        
        ttk.Label(filter_row, text="模式:").pack(side=tk.LEFT, padx=5)
        self.cfg_mode = ttk.Combobox(filter_row, values=["全部", "2D", "3D", "TDM"], width=12, state='readonly')
        self.cfg_mode.current(0)
        self.cfg_mode.bind('<<ComboboxSelected>>', lambda e: self.filter_configs())
        self.cfg_mode.pack(side=tk.LEFT, padx=5)
        
        ttk.Label(filter_row, text="搜索:").pack(side=tk.LEFT, padx=5)
        self.cfg_search = ttk.Entry(filter_row, width=30)
        self.cfg_search.bind('<KeyRelease>', lambda e: self.filter_configs())
        self.cfg_search.pack(side=tk.LEFT, padx=5, fill=tk.X, expand=True)
        
        # 配置文件列表
        list_frame = ttk.Frame(frame)
        list_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        scrollbar = ttk.Scrollbar(list_frame)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        self.cfg_tree = ttk.Treeview(
            list_frame,
            columns=('filename', 'application', 'channels', 'range', 'mode'),
            show='headings',
            yscrollcommand=scrollbar.set
        )
        scrollbar.config(command=self.cfg_tree.yview)
        
        self.cfg_tree.heading('filename', text='文件名')
        self.cfg_tree.heading('application', text='应用场景')
        self.cfg_tree.heading('channels', text='TX/RX通道')
        self.cfg_tree.heading('range', text='检测距离(m)')
        self.cfg_tree.heading('mode', text='工作模式')
        
        self.cfg_tree.column('filename', width=300)
        self.cfg_tree.column('application', width=150)
        self.cfg_tree.column('channels', width=120)
        self.cfg_tree.column('range', width=120)
        self.cfg_tree.column('mode', width=150)
        
        self.cfg_tree.pack(fill=tk.BOTH, expand=True)
        self.cfg_tree.bind('<<TreeviewSelect>>', self.on_config_selected)
        
        # 详细信息
        detail_frame = ttk.LabelFrame(frame, text="详细信息", padding=5)
        detail_frame.pack(fill=tk.X, padx=5, pady=5)
        
        self.cfg_detail = scrolledtext.ScrolledText(detail_frame, height=6, wrap=tk.WORD)
        self.cfg_detail.pack(fill=tk.BOTH, expand=True)
        
    def create_match_tab(self):
        """创建智能匹配标签页"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="🎯 智能匹配")
        
        # 说明
        info = ttk.Label(
            frame,
            text="选择一个应用固件，系统将自动推荐匹配的SBL固件和雷达配置文件",
            font=('Arial', 10, 'bold')
        )
        info.pack(pady=10)
        
        # 固件选择
        select_frame = ttk.LabelFrame(frame, text="选择应用固件", padding=10)
        select_frame.pack(fill=tk.X, padx=10, pady=5)
        
        list_frame = ttk.Frame(select_frame)
        list_frame.pack(fill=tk.BOTH, expand=True)
        
        scrollbar = ttk.Scrollbar(list_frame)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        self.match_fw_list = ttk.Treeview(
            list_frame,
            columns=('filename',),
            show='headings',
            height=5,
            yscrollcommand=scrollbar.set
        )
        scrollbar.config(command=self.match_fw_list.yview)
        
        self.match_fw_list.heading('filename', text='固件文件名')
        self.match_fw_list.column('filename', width=700)
        self.match_fw_list.pack(fill=tk.BOTH, expand=True)
        self.match_fw_list.bind('<<TreeviewSelect>>', self.on_match_firmware_selected)
        
        # SBL匹配结果
        sbl_frame = ttk.LabelFrame(frame, text="推荐SBL固件 (Top 3)", padding=10)
        sbl_frame.pack(fill=tk.X, padx=10, pady=5)
        
        self.match_sbl_tree = ttk.Treeview(
            sbl_frame,
            columns=('filename', 'variant', 'score'),
            show='headings',
            height=3
        )
        self.match_sbl_tree.heading('filename', text='文件名')
        self.match_sbl_tree.heading('variant', text='变体')
        self.match_sbl_tree.heading('score', text='匹配度')
        
        self.match_sbl_tree.column('filename', width=400)
        self.match_sbl_tree.column('variant', width=100)
        self.match_sbl_tree.column('score', width=100)
        
        self.match_sbl_tree.pack(fill=tk.BOTH, expand=True)
        
        # 配置匹配结果
        cfg_frame = ttk.LabelFrame(frame, text="推荐雷达配置 (Top 5)", padding=10)
        cfg_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)
        
        cfg_scroll = ttk.Scrollbar(cfg_frame)
        cfg_scroll.pack(side=tk.RIGHT, fill=tk.Y)
        
        self.match_cfg_tree = ttk.Treeview(
            cfg_frame,
            columns=('filename', 'application', 'params', 'score'),
            show='headings',
            yscrollcommand=cfg_scroll.set
        )
        cfg_scroll.config(command=self.match_cfg_tree.yview)
        
        self.match_cfg_tree.heading('filename', text='文件名')
        self.match_cfg_tree.heading('application', text='应用场景')
        self.match_cfg_tree.heading('params', text='参数')
        self.match_cfg_tree.heading('score', text='匹配度')
        
        self.match_cfg_tree.column('filename', width=300)
        self.match_cfg_tree.column('application', width=150)
        self.match_cfg_tree.column('params', width=200)
        self.match_cfg_tree.column('score', width=100)
        
        self.match_cfg_tree.pack(fill=tk.BOTH, expand=True)
    
    # ========== 事件处理 ==========
    
    def add_directory(self):
        """添加扫描目录"""
        directory = filedialog.askdirectory(title="选择扫描目录")
        if directory and directory not in self.scan_directories:
            self.scan_directories.append(directory)
            self.dir_combo['values'] = self.scan_directories
            self.dir_combo.set(directory)
            
    def remove_directory(self):
        """删除当前选择的目录"""
        current = self.dir_combo.get()
        if current in self.scan_directories:
            self.scan_directories.remove(current)
            self.dir_combo['values'] = self.scan_directories
            if self.scan_directories:
                self.dir_combo.current(0)
            else:
                self.dir_combo.set('')
    
    def start_scan(self):
        """开始扫描"""
        if self.is_scanning:
            messagebox.showwarning("警告", "正在扫描中，请等待完成")
            return
        
        if not self.scan_directories:
            messagebox.showwarning("警告", "请先添加扫描目录")
            return
        
        self.is_scanning = True
        self.btn_scan.config(state='disabled')
        self.progress.start()
        
        # 在后台线程执行扫描
        threading.Thread(target=self._scan_worker, daemon=True).start()
    
    def _scan_worker(self):
        """扫描工作线程"""
        try:
            for directory in self.scan_directories:
                if os.path.exists(directory):
                    self.matcher.scan_directory(directory, recursive=True)
            
            # 扫描完成，更新UI
            self.parent.after(0, self._scan_completed)
        except Exception as e:
            self.parent.after(0, lambda: messagebox.showerror("扫描错误", str(e)))
            self.parent.after(0, self._scan_completed)
    
    def _scan_completed(self):
        """扫描完成"""
        self.is_scanning = False
        self.btn_scan.config(state='normal')
        self.progress.stop()
        
        # 更新统计
        stats = self.matcher.get_statistics()
        self.lbl_app.config(text=f"应用固件: {stats['application_count']}")
        self.lbl_sbl.config(text=f"SBL固件: {stats['sbl_count']}")
        self.lbl_config.config(text=f"雷达配置: {stats['config_count']}")
        
        # 更新列表
        self.update_firmware_list()
        self.update_sbl_list()
        self.update_config_list()
        self.update_match_list()
        
        # 更新筛选器选项
        self.update_filter_options()
        
        messagebox.showinfo(
            "扫描完成",
            f"扫描完成！\n\n"
            f"应用固件: {stats['application_count']}\n"
            f"SBL固件: {stats['sbl_count']}\n"
            f"雷达配置: {stats['config_count']}"
        )
    
    def update_firmware_list(self):
        """更新应用固件列表"""
        self.fw_tree.delete(*self.fw_tree.get_children())
        for fw in self.matcher.application_firmwares:
            self.fw_tree.insert('', 'end', values=(
                fw.filename,
                fw.category,
                fw.subcategory,
                fw.processor,
                fw.version
            ), tags=(fw.path,))
    
    def update_sbl_list(self):
        """更新SBL固件列表"""
        self.sbl_tree.delete(*self.sbl_tree.get_children())
        for sbl in self.matcher.sbl_firmwares:
            self.sbl_tree.insert('', 'end', values=(
                sbl.filename,
                sbl.variant,
                sbl.flash_address,
                sbl.flash_size,
                sbl.description
            ), tags=(sbl.path,))
    
    def update_config_list(self):
        """更新雷达配置列表"""
        self.cfg_tree.delete(*self.cfg_tree.get_children())
        for cfg in self.matcher.config_files:
            channels = f"{cfg.tx_channels}TX/{cfg.rx_channels}RX" if cfg.tx_channels > 0 else "N/A"
            self.cfg_tree.insert('', 'end', values=(
                cfg.filename,
                cfg.application,
                channels,
                cfg.range_m if cfg.range_m > 0 else "N/A",
                cfg.mode
            ), tags=(cfg.path,))
    
    def update_match_list(self):
        """更新匹配列表"""
        self.match_fw_list.delete(*self.match_fw_list.get_children())
        for fw in self.matcher.application_firmwares:
            self.match_fw_list.insert('', 'end', values=(fw.filename,), tags=(fw.path,))
    
    def update_filter_options(self):
        """更新筛选器选项"""
        # 应用固件筛选器
        categories = set(fw.category for fw in self.matcher.application_firmwares if fw.category)
        self.fw_category['values'] = ["全部"] + sorted(categories)
        
        processors = set(fw.processor for fw in self.matcher.application_firmwares if fw.processor)
        self.fw_processor['values'] = ["全部"] + sorted(processors)
        
        # 配置文件筛选器
        apps = set(cfg.application for cfg in self.matcher.config_files if cfg.application)
        self.cfg_app['values'] = ["全部"] + sorted(apps)
    
    def filter_firmwares(self):
        """筛选应用固件"""
        category = self.fw_category.get()
        processor = self.fw_processor.get()
        keyword = self.fw_search.get().lower()
        
        self.fw_tree.delete(*self.fw_tree.get_children())
        
        for fw in self.matcher.application_firmwares:
            if category != "全部" and fw.category != category:
                continue
            if processor != "全部" and fw.processor != processor:
                continue
            if keyword and keyword not in fw.filename.lower() and keyword not in fw.path.lower():
                continue
            
            self.fw_tree.insert('', 'end', values=(
                fw.filename,
                fw.category,
                fw.subcategory,
                fw.processor,
                fw.version
            ), tags=(fw.path,))
    
    def filter_configs(self):
        """筛选雷达配置"""
        app = self.cfg_app.get()
        mode = self.cfg_mode.get()
        keyword = self.cfg_search.get().lower()
        
        self.cfg_tree.delete(*self.cfg_tree.get_children())
        
        for cfg in self.matcher.config_files:
            if app != "全部" and cfg.application != app:
                continue
            if mode != "全部" and mode not in cfg.mode:
                continue
            if keyword and keyword not in cfg.filename.lower() and keyword not in cfg.path.lower():
                continue
            
            channels = f"{cfg.tx_channels}TX/{cfg.rx_channels}RX" if cfg.tx_channels > 0 else "N/A"
            self.cfg_tree.insert('', 'end', values=(
                cfg.filename,
                cfg.application,
                channels,
                cfg.range_m if cfg.range_m > 0 else "N/A",
                cfg.mode
            ), tags=(cfg.path,))
    
    def on_firmware_selected(self, event):
        """应用固件选中事件"""
        selection = self.fw_tree.selection()
        if not selection:
            return
        
        path = self.fw_tree.item(selection[0])['tags'][0]
        fw = next((f for f in self.matcher.application_firmwares if f.path == path), None)
        
        if fw:
            details = (
                f"文件名: {fw.filename}\n"
                f"完整路径: {fw.path}\n"
                f"类别: {fw.category}\n"
                f"子类别: {fw.subcategory}\n"
                f"处理器: {fw.processor}\n"
                f"版本: {fw.version}\n"
                f"说明: {fw.description}\n"
            )
            self.fw_detail.delete('1.0', tk.END)
            self.fw_detail.insert('1.0', details)
    
    def on_sbl_selected(self, event):
        """SBL固件选中事件"""
        selection = self.sbl_tree.selection()
        if not selection:
            return
        
        path = self.sbl_tree.item(selection[0])['tags'][0]
        sbl = next((s for s in self.matcher.sbl_firmwares if s.path == path), None)
        
        if sbl:
            details = (
                f"文件名: {sbl.filename}\n"
                f"完整路径: {sbl.path}\n"
                f"变体: {sbl.variant}\n"
                f"Flash地址: {sbl.flash_address}\n"
                f"大小: {sbl.flash_size}\n"
                f"说明: {sbl.description}\n"
            )
            self.sbl_detail.delete('1.0', tk.END)
            self.sbl_detail.insert('1.0', details)
    
    def on_config_selected(self, event):
        """雷达配置选中事件"""
        selection = self.cfg_tree.selection()
        if not selection:
            return
        
        path = self.cfg_tree.item(selection[0])['tags'][0]
        cfg = next((c for c in self.matcher.config_files if c.path == path), None)
        
        if cfg:
            details = (
                f"文件名: {cfg.filename}\n"
                f"完整路径: {cfg.path}\n"
                f"应用场景: {cfg.application}\n"
                f"TX通道: {cfg.tx_channels if cfg.tx_channels > 0 else 'N/A'}\n"
                f"RX通道: {cfg.rx_channels if cfg.rx_channels > 0 else 'N/A'}\n"
                f"检测距离: {cfg.range_m}m\n"
                f"工作模式: {cfg.mode}\n"
                f"功耗模式: {cfg.power_mode}\n"
                f"带宽模式: {cfg.bandwidth}\n"
                f"封装类型: {cfg.package_type}\n"
                f"说明: {cfg.description}\n"
            )
            self.cfg_detail.delete('1.0', tk.END)
            self.cfg_detail.insert('1.0', details)
    
    def on_match_firmware_selected(self, event):
        """匹配固件选中事件"""
        selection = self.match_fw_list.selection()
        if not selection:
            return
        
        path = self.match_fw_list.item(selection[0])['tags'][0]
        fw = next((f for f in self.matcher.application_firmwares if f.path == path), None)
        
        if not fw:
            return
        
        # 清空之前的匹配结果
        self.match_sbl_tree.delete(*self.match_sbl_tree.get_children())
        self.match_cfg_tree.delete(*self.match_cfg_tree.get_children())
        
        # 匹配SBL
        sbl_matches = self.matcher.match_sbl_for_firmware(fw)
        for i, (sbl, score) in enumerate(sbl_matches[:3]):
            item = self.match_sbl_tree.insert('', 'end', values=(
                sbl.filename,
                sbl.variant,
                f"{score:.0f}%"
            ))
            if i == 0:  # 高亮最佳匹配
                self.match_sbl_tree.item(item, tags=('best',))
        
        # 匹配配置
        cfg_matches = self.matcher.match_configs_for_firmware(fw)
        for i, (cfg, score) in enumerate(cfg_matches[:5]):
            params = []
            if cfg.tx_channels > 0:
                params.append(f"{cfg.tx_channels}TX/{cfg.rx_channels}RX")
            if cfg.range_m > 0:
                params.append(f"{cfg.range_m}m")
            if cfg.mode:
                params.append(cfg.mode)
            
            item = self.match_cfg_tree.insert('', 'end', values=(
                cfg.filename,
                cfg.application,
                " | ".join(params),
                f"{score:.0f}%"
            ))
            if i == 0:  # 高亮最佳匹配
                self.match_cfg_tree.item(item, tags=('best',))
        
        # 配置高亮样式
        self.match_sbl_tree.tag_configure('best', background='#c8ffc8')
        self.match_cfg_tree.tag_configure('best', background='#c8ffc8')
    
    def clear_results(self):
        """清空结果"""
        if messagebox.askyesno("确认", "确定要清空所有扫描结果吗？"):
            self.matcher.clear_results()
            
            self.fw_tree.delete(*self.fw_tree.get_children())
            self.sbl_tree.delete(*self.sbl_tree.get_children())
            self.cfg_tree.delete(*self.cfg_tree.get_children())
            self.match_fw_list.delete(*self.match_fw_list.get_children())
            self.match_sbl_tree.delete(*self.match_sbl_tree.get_children())
            self.match_cfg_tree.delete(*self.match_cfg_tree.get_children())
            
            self.lbl_app.config(text="应用固件: 0")
            self.lbl_sbl.config(text="SBL固件: 0")
            self.lbl_config.config(text="雷达配置: 0")
            
            self.fw_detail.delete('1.0', tk.END)
            self.sbl_detail.delete('1.0', tk.END)
            self.cfg_detail.delete('1.0', tk.END)
    
    def refresh(self):
        """刷新标签页数据"""
        pass
