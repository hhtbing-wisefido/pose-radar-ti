#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
固件管理标签页 - v1.4.1
AWRL6844EVM 固件智能管理系统（集成版）
功能：扫描、筛选、匹配应用固件、SBL、雷达配置文件


v1.4.1 更新 (2026-01-29):
- 修复扫描后应用固件和SBL固件列表不显示的问题
  * 原因：update_filter_options中SBL/Config筛选器未解绑事件，
    调用current(0)时触发筛选事件导致列表被清空
  * 修复：解绑所有筛选器事件，更新选项后重新绑定
- 统一所有筛选器（应用固件/SBL/雷达配置）的事件解绑和重绑定逻辑

v1.3.7 更新:
- 删除扫描控制框中的重复目录管理功能
- 修复多次扫描文件累积重复的bug

v1.3.8 更新:
- 修复添加目录功能无效的bug
- 修复删除目录功能无效的bug
- 修复清空所有目录后列表依然显示的bug

v1.3.9 更新:
- 使用PanedWindow实现可调整大小的详细信息框
- 所有Treeview支持右键复制功能
- 详细信息框自适应尺寸

v1.4.0 更新:
- 简化复制功能：只复制文件名和完整路径
- 移除不必要的“复制选中行”和“复制所有数据”功能
"""

import tkinter as tk
from tkinter import ttk, messagebox, filedialog, scrolledtext
import os
import sys
import threading
from pathlib import Path


DEBUG = os.environ.get("FLASH_TOOL_DEBUG") == "1"


def _dprint(msg: str) -> None:
    if DEBUG:
        print(msg)

# 导入固件匹配器
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
    raise


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

        # Treeview item -> 文件路径映射（避免把Windows路径放进Tk tags导致转义/插入失败/路径失真）
        self._tree_item_paths = {}

        # 默认扫描目录
        self.scan_directories = [
            r"C:\ti\MMWAVE_L_SDK_06_01_00_01",
            r"C:\ti\radar_toolbox_3_30_00_06"
        ]

        # 扫描状态
        self.is_scanning = False

        # 创建界面
        self.create_widgets()

    def _clear_tree_path_map(self, tree):
        if tree in self._tree_item_paths:
            self._tree_item_paths[tree].clear()

    def _remember_tree_item_path(self, tree, item_id, path):
        if not path:
            return
        self._tree_item_paths.setdefault(tree, {})[item_id] = path

    def _get_tree_item_path(self, tree, item_id):
        # 1) 优先从映射获取（最可靠）
        path_map = self._tree_item_paths.get(tree)
        if path_map:
            path = path_map.get(item_id)
            if path:
                return path

        # 2) 兼容旧逻辑：从tags读取（可能被Tcl转义，尽量不用）
        try:
            tags = tree.item(item_id).get('tags')
            if tags:
                return tags[0]
        except Exception:
            pass

        # 3) 兜底：从values里尝试找“像路径”的字段（用于智能匹配列表）
        try:
            values = tree.item(item_id).get('values')
            if values:
                for v in values:
                    s = str(v)
                    if (':\\' in s) or s.startswith('\\\\'):
                        return s
        except Exception:
            pass

        return ""

    def _install_panedwindow_sash_fix(
        self,
        paned,
        *,
        min_list_height=200,
        min_detail_height=140,
        list_ratio=0.60,
    ):
        """确保PanedWindow在任何窗口大小下，列表区域都不会被挤到不可见。"""

        state = {"pending": False, "last": None}

        def apply():
            state["pending"] = False
            try:
                total_h = paned.winfo_height()
                # 还未完成布局时，尺寸可能为1
                if total_h <= 2:
                    paned.after(50, apply)
                    return

                desired = int(total_h * list_ratio)
                max_list = max(0, total_h - min_detail_height)
                list_h = min(max(desired, min_list_height), max_list if max_list > 0 else desired)
                # 至少保证表头可见
                list_h = max(list_h, 40)

                last = state.get("last")
                if isinstance(last, int) and abs(last - list_h) <= 2:
                    return
                state["last"] = list_h

                paned.sashpos(0, list_h)
            except Exception:
                return

        def schedule(_event=None):
            if state["pending"]:
                return
            state["pending"] = True
            paned.after_idle(apply)

        paned.bind("<Configure>", schedule)
        schedule()

    def create_widgets(self):
        """创建界面组件"""

        icons = getattr(self.main_app, "icons", None)
        self.icons = icons

        # ============ 顶部控制区 ============
        control_frame = ttk.LabelFrame(self.parent, padding=10)
        if icons:
            control_frame.configure(
                labelwidget=icons.make_labelframe_labelwidget(
                    parent=self.parent,
                    icon_key="radar",
                    text="扫描控制",
                    size=20,
                )
            )
        else:
            control_frame.configure(text="扫描控制")
        control_frame.pack(fill=tk.X, padx=10, pady=(10, 5))

        # 扫描按钮行
        btn_row = ttk.Frame(control_frame)
        btn_row.pack(fill=tk.X, pady=5)

        self.btn_scan = ttk.Button(btn_row, text="开始扫描", command=self.start_scan)
        if icons:
            self.btn_scan.configure(image=icons.get("search", 20), compound="left")
        self.btn_scan.pack(side=tk.LEFT, padx=5, fill=tk.X, expand=True)

        btn_clear = ttk.Button(btn_row, text="清空结果", command=self.clear_results)
        if icons:
            btn_clear.configure(image=icons.get("trash", 20), compound="left")
        btn_clear.pack(
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
        self.create_directory_tab()
        self.create_firmware_tab()
        self.create_sbl_tab()
        self.create_config_tab()
        self.create_match_tab()

    def create_directory_tab(self):
        """创建扫描目录管理标签页"""
        frame = ttk.Frame(self.notebook)
        if self.icons:
            self.notebook.add(
                frame,
                text="扫描目录",
                image=self.icons.get("folder_open", 20),
                compound="left",
            )
        else:
            self.notebook.add(frame, text="扫描目录")

        # 说明
        info_frame = ttk.Frame(frame)
        info_frame.pack(fill=tk.X, padx=10, pady=10)

        info_text = (
            "使用说明:\n"
            "• 添加TI SDK安装目录（如: C:\\ti\\MMWAVE_L_SDK_06_01_00_01）\n"
            "• 添加雷达工具箱目录（如: C:\\ti\\radar_toolbox_3_30_00_06）\n"
            "• 点击'开始扫描'按钮，系统将递归扫描所有子目录\n"
            "• 支持多个目录同时扫描"
        )
        ttk.Label(info_frame, text=info_text, justify=tk.LEFT).pack(anchor=tk.W)

        # 目录操作按钮
        btn_frame = ttk.Frame(frame)
        btn_frame.pack(fill=tk.X, padx=10, pady=5)

        btn_add = ttk.Button(btn_frame, text="添加目录", command=self.add_directory_to_list)
        if self.icons:
            btn_add.configure(image=self.icons.get("plus", 16), compound="left")
        btn_add.pack(
            side=tk.LEFT, padx=5
        )
        btn_remove = ttk.Button(btn_frame, text="删除选中", command=self.remove_selected_directory)
        if self.icons:
            btn_remove.configure(image=self.icons.get("minus", 16), compound="left")
        btn_remove.pack(
            side=tk.LEFT, padx=5
        )
        btn_restore = ttk.Button(btn_frame, text="恢复默认", command=self.restore_default_directories)
        if self.icons:
            btn_restore.configure(image=self.icons.get("refresh", 16), compound="left")
        btn_restore.pack(
            side=tk.LEFT, padx=5
        )
        btn_clear_all = ttk.Button(btn_frame, text="清空所有", command=self.clear_all_directories)
        if self.icons:
            btn_clear_all.configure(image=self.icons.get("trash", 16), compound="left")
        btn_clear_all.pack(
            side=tk.LEFT, padx=5
        )

        # 目录列表
        list_frame = ttk.LabelFrame(frame, text="扫描目录列表", padding=10)
        list_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)

        # 滚动条
        scrollbar = ttk.Scrollbar(list_frame)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.dir_tree = ttk.Treeview(
            list_frame,
            columns=('path', 'status'),
            show='headings',
            yscrollcommand=scrollbar.set
        )
        scrollbar.config(command=self.dir_tree.yview)

        self.dir_tree.heading('path', text='目录路径')
        self.dir_tree.heading('status', text='状态')

        self.dir_tree.column('path', width=650)
        self.dir_tree.column('status', width=100)

        self.dir_tree.pack(fill=tk.BOTH, expand=True)

        # 初始化显示
        self.update_directory_list()

    def create_firmware_tab(self):
        """创建应用固件标签页"""
        frame = ttk.Frame(self.notebook)
        if self.icons:
            self.notebook.add(
                frame,
                text="应用固件",
                image=self.icons.get("microchip", 20),
                compound="left",
            )
        else:
            self.notebook.add(frame, text="应用固件")

        # 筛选区
        filter_frame = ttk.LabelFrame(frame, text="筛选条件", padding=5)
        filter_frame.pack(fill=tk.X, padx=5, pady=5)

        # 第一行：类别、子类别、处理器、版本
        filter_row1 = ttk.Frame(filter_frame)
        filter_row1.pack(fill=tk.X, pady=2)

        ttk.Label(filter_row1, text="类别:").pack(side=tk.LEFT, padx=5)
        self.fw_category = ttk.Combobox(filter_row1, values=["全部"], width=15, state='readonly')
        self.fw_category.current(0)
        self.fw_category.bind('<<ComboboxSelected>>', lambda e: self.on_fw_filter_change())
        self.fw_category.pack(side=tk.LEFT, padx=5)

        ttk.Label(filter_row1, text="子类别:").pack(side=tk.LEFT, padx=5)
        self.fw_subcategory = ttk.Combobox(filter_row1, values=["全部"], width=18, state='readonly')
        self.fw_subcategory.current(0)
        self.fw_subcategory.bind('<<ComboboxSelected>>', lambda e: self.on_fw_filter_change())
        self.fw_subcategory.pack(side=tk.LEFT, padx=5)

        ttk.Label(filter_row1, text="处理器:").pack(side=tk.LEFT, padx=5)
        self.fw_processor = ttk.Combobox(filter_row1, values=["全部"], width=20, state='readonly')
        self.fw_processor.current(0)
        self.fw_processor.bind('<<ComboboxSelected>>', lambda e: self.on_fw_filter_change())
        self.fw_processor.pack(side=tk.LEFT, padx=5)

        ttk.Label(filter_row1, text="版本:").pack(side=tk.LEFT, padx=5)
        self.fw_version = ttk.Combobox(filter_row1, values=["全部"], width=12, state='readonly')
        self.fw_version.current(0)
        self.fw_version.bind('<<ComboboxSelected>>', lambda e: self.filter_firmwares())
        self.fw_version.pack(side=tk.LEFT, padx=5)

        # 第二行：文件大小、文件路径
        filter_row2 = ttk.Frame(filter_frame)
        filter_row2.pack(fill=tk.X, pady=2)

        ttk.Label(filter_row2, text="文件大小:").pack(side=tk.LEFT, padx=5)
        self.fw_size = ttk.Combobox(filter_row2, values=["全部"], width=15, state='readonly')
        self.fw_size.current(0)
        self.fw_size.bind('<<ComboboxSelected>>', lambda e: self.filter_firmwares())
        self.fw_size.pack(side=tk.LEFT, padx=5)

        ttk.Label(filter_row2, text="文件路径:").pack(side=tk.LEFT, padx=5)
        self.fw_path = ttk.Combobox(filter_row2, values=["全部"], width=50, state='readonly')
        self.fw_path.current(0)
        self.fw_path.bind('<<ComboboxSelected>>', lambda e: self.filter_firmwares())
        self.fw_path.pack(side=tk.LEFT, padx=5, fill=tk.X, expand=True)

        # 第三行：搜索框
        filter_row3 = ttk.Frame(filter_frame)
        filter_row3.pack(fill=tk.X, pady=2)

        lbl_fw_search = ttk.Label(filter_row3, text="搜索:")
        if self.icons:
            lbl_fw_search.configure(image=self.icons.get("search", 16), compound="left")
        lbl_fw_search.pack(side=tk.LEFT, padx=5)
        self.fw_search = ttk.Entry(filter_row3, width=80)
        self.fw_search.bind('<KeyRelease>', lambda e: self.filter_firmwares())
        self.fw_search.pack(side=tk.LEFT, padx=5, fill=tk.X, expand=True)

        ttk.Label(filter_row3, text="(支持搜索: 文件名/路径/类别/子类别/处理器/版本)",
                 font=('Arial', 8), foreground='gray').pack(side=tk.LEFT, padx=5)

        # 使用PanedWindow实现可调整大小的布局
        paned = ttk.PanedWindow(frame, orient=tk.VERTICAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        # 固件列表（Treeview）
        list_frame = ttk.Frame(paned)
        paned.add(list_frame, weight=3)

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
        self.fw_tree.bind('<Button-3>', lambda e: self.show_copy_menu(e, self.fw_tree))

        # 详细信息
        detail_frame = ttk.LabelFrame(paned, text="详细信息", padding=5)
        paned.add(detail_frame, weight=1)

        # 修复：默认窗口下确保列表区域可见（ttk.PanedWindow不支持minsize参数，需手动设置sash）
        self._install_panedwindow_sash_fix(paned)

        self.fw_detail = scrolledtext.ScrolledText(detail_frame, wrap=tk.WORD)
        self.fw_detail.pack(fill=tk.BOTH, expand=True)

    def create_sbl_tab(self):
        """创建SBL固件标签页"""
        frame = ttk.Frame(self.notebook)
        if self.icons:
            self.notebook.add(
                frame,
                text="SBL固件",
                image=self.icons.get("settings", 20),
                compound="left",
            )
        else:
            self.notebook.add(frame, text="SBL固件")

        # 说明
        info_frame = ttk.Frame(frame)
        info_frame.pack(fill=tk.X, padx=5, pady=5)

        info_text = (
            "SBL固件说明:\n"
            "SBL (Secondary Bootloader) 是芯片启动的第一级程序，负责从Flash加载应用固件。\n"
            "推荐使用: 标准版SBL，功能完整，适用于绝大多数应用场景。"
        )
        ttk.Label(info_frame, text=info_text, justify=tk.LEFT, wraplength=800).pack(anchor=tk.W)

        # 筛选区
        filter_frame = ttk.LabelFrame(frame, text="筛选条件", padding=5)
        filter_frame.pack(fill=tk.X, padx=5, pady=5)

        # 第一行：变体类型、Flash地址
        filter_row1 = ttk.Frame(filter_frame)
        filter_row1.pack(fill=tk.X, pady=2)

        ttk.Label(filter_row1, text="变体类型:").pack(side=tk.LEFT, padx=5)
        self.sbl_variant = ttk.Combobox(filter_row1, values=["全部"], width=20, state='readonly')
        self.sbl_variant.current(0)
        self.sbl_variant.bind('<<ComboboxSelected>>', lambda e: self.on_sbl_filter_change())
        self.sbl_variant.pack(side=tk.LEFT, padx=5)

        ttk.Label(filter_row1, text="Flash地址:").pack(side=tk.LEFT, padx=5)
        self.sbl_flash = ttk.Combobox(filter_row1, values=["全部"], width=20, state='readonly')
        self.sbl_flash.current(0)
        self.sbl_flash.bind('<<ComboboxSelected>>', lambda e: self.filter_sbls())
        self.sbl_flash.pack(side=tk.LEFT, padx=5)

        # 第二行：搜索框
        filter_row2 = ttk.Frame(filter_frame)
        filter_row2.pack(fill=tk.X, pady=2)

        lbl_sbl_search = ttk.Label(filter_row2, text="搜索:")
        if self.icons:
            lbl_sbl_search.configure(image=self.icons.get("search", 16), compound="left")
        lbl_sbl_search.pack(side=tk.LEFT, padx=5)
        self.sbl_search = ttk.Entry(filter_row2, width=60)
        self.sbl_search.bind('<KeyRelease>', lambda e: self.filter_sbls())
        self.sbl_search.pack(side=tk.LEFT, padx=5, fill=tk.X, expand=True)

        ttk.Label(filter_row2, text="(支持搜索: 文件名/路径/变体/说明)",
                 font=('Arial', 8), foreground='gray').pack(side=tk.LEFT, padx=5)

        # 使用PanedWindow实现可调整大小的布局
        paned = ttk.PanedWindow(frame, orient=tk.VERTICAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        # SBL列表
        list_frame = ttk.Frame(paned)
        paned.add(list_frame, weight=3)

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
        self.sbl_tree.bind('<Button-3>', lambda e: self.show_copy_menu(e, self.sbl_tree))

        # 详细信息
        detail_frame = ttk.LabelFrame(paned, text="详细信息", padding=5)
        paned.add(detail_frame, weight=1)

        # 修复：默认窗口下确保列表区域可见（ttk.PanedWindow不支持minsize参数，需手动设置sash）
        self._install_panedwindow_sash_fix(paned)

        self.sbl_detail = scrolledtext.ScrolledText(detail_frame, wrap=tk.WORD)
        self.sbl_detail.pack(fill=tk.BOTH, expand=True)

    def create_config_tab(self):
        """创建雷达配置标签页"""
        frame = ttk.Frame(self.notebook)
        if self.icons:
            self.notebook.add(
                frame,
                text="雷达配置",
                image=self.icons.get("radar", 20),
                compound="left",
            )
        else:
            self.notebook.add(frame, text="雷达配置")

        # 筛选区
        filter_frame = ttk.LabelFrame(frame, text="筛选条件", padding=5)
        filter_frame.pack(fill=tk.X, padx=5, pady=5)

        # 第一行：应用场景、模式、TX/RX通道数、检测距离
        filter_row1 = ttk.Frame(filter_frame)
        filter_row1.pack(fill=tk.X, pady=2)

        ttk.Label(filter_row1, text="应用场景:").pack(side=tk.LEFT, padx=5)
        self.cfg_app = ttk.Combobox(filter_row1, values=["全部"], width=15, state='readonly')
        self.cfg_app.current(0)
        self.cfg_app.bind('<<ComboboxSelected>>', lambda e: self.on_cfg_filter_change())
        self.cfg_app.pack(side=tk.LEFT, padx=5)

        ttk.Label(filter_row1, text="模式:").pack(side=tk.LEFT, padx=5)
        self.cfg_mode = ttk.Combobox(filter_row1, values=["全部", "2D", "3D", "TDM"], width=12, state='readonly')
        self.cfg_mode.current(0)
        self.cfg_mode.bind('<<ComboboxSelected>>', lambda e: self.on_cfg_filter_change())
        self.cfg_mode.pack(side=tk.LEFT, padx=5)

        ttk.Label(filter_row1, text="通道数:").pack(side=tk.LEFT, padx=5)
        self.cfg_channels = ttk.Combobox(filter_row1, values=["全部"], width=12, state='readonly')
        self.cfg_channels.current(0)
        self.cfg_channels.bind('<<ComboboxSelected>>', lambda e: self.on_cfg_filter_change())
        self.cfg_channels.pack(side=tk.LEFT, padx=5)

        ttk.Label(filter_row1, text="检测距离:").pack(side=tk.LEFT, padx=5)
        self.cfg_range = ttk.Combobox(filter_row1, values=["全部"], width=15, state='readonly')
        self.cfg_range.current(0)
        self.cfg_range.bind('<<ComboboxSelected>>', lambda e: self.filter_configs())
        self.cfg_range.pack(side=tk.LEFT, padx=5)

        # 第二行：搜索框
        filter_row2 = ttk.Frame(filter_frame)
        filter_row2.pack(fill=tk.X, pady=2)

        lbl_cfg_search = ttk.Label(filter_row2, text="搜索:")
        if self.icons:
            lbl_cfg_search.configure(image=self.icons.get("search", 16), compound="left")
        lbl_cfg_search.pack(side=tk.LEFT, padx=5)
        self.cfg_search = ttk.Entry(filter_row2, width=70)
        self.cfg_search.bind('<KeyRelease>', lambda e: self.filter_configs())
        self.cfg_search.pack(side=tk.LEFT, padx=5, fill=tk.X, expand=True)

        ttk.Label(filter_row2, text="(支持搜索: 文件名/路径/应用场景/通道数/检测距离/模式)",
                 font=('Arial', 8), foreground='gray').pack(side=tk.LEFT, padx=5)

        # 使用PanedWindow实现可调整大小的布局
        paned = ttk.PanedWindow(frame, orient=tk.VERTICAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        # 配置文件列表
        list_frame = ttk.Frame(paned)
        paned.add(list_frame, weight=3)

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
        self.cfg_tree.bind('<Button-3>', lambda e: self.show_copy_menu(e, self.cfg_tree))

        # 详细信息
        detail_frame = ttk.LabelFrame(paned, text="详细信息", padding=5)
        paned.add(detail_frame, weight=1)

        # 修复：默认窗口下确保列表区域可见（ttk.PanedWindow不支持minsize参数，需手动设置sash）
        self._install_panedwindow_sash_fix(paned)

        self.cfg_detail = scrolledtext.ScrolledText(detail_frame, wrap=tk.WORD)
        self.cfg_detail.pack(fill=tk.BOTH, expand=True)

    def create_match_tab(self):
        """创建智能匹配标签页"""
        frame = ttk.Frame(self.notebook)
        if self.icons:
            self.notebook.add(
                frame,
                text="智能匹配",
                image=self.icons.get("search", 20),
                compound="left",
            )
        else:
            self.notebook.add(frame, text="智能匹配")

        # 说明
        # 顶部操作栏：说明文字 + 一键添加按钮（同行）
        top_frame = ttk.Frame(frame)
        top_frame.pack(fill=tk.X, padx=10, pady=10)

        # 左侧说明文字
        info = ttk.Label(
            top_frame,
            text="选择一个应用固件，系统将自动推荐匹配的SBL固件和雷达配置文件",
            font=('Arial', 10, 'bold')
        )
        info.pack(side=tk.LEFT, padx=(0, 20))

        # 右侧一键添加按钮
        add_btn = tk.Button(
            top_frame,
            text="一键添加到烧录功能",
            font=("Microsoft YaHei UI", 10, "bold"),
            command=self.add_to_basic_flash,
            bg="#27ae60",
            fg="white",
            activebackground="#229954",
            activeforeground="white",
            relief=tk.RAISED,
            bd=3,
            padx=15,
            pady=8,
            cursor="hand2"
        )
        if self.icons:
            add_btn.configure(image=self.icons.get("rocket", 20), compound="left")
        add_btn.pack(side=tk.RIGHT)

        # 按钮下方提示
        tip_label = ttk.Label(
            frame,
            text="↑ 点击按钮将选中固件自动添加到「烧录功能」标签页",
            foreground="#e74c3c",
            font=("Microsoft YaHei UI", 9, "bold")
        )
        tip_label.pack(padx=10, pady=(0, 5))

        # 固件选择
        select_frame = ttk.LabelFrame(frame, text="选择应用固件", padding=10)
        select_frame.pack(fill=tk.X, padx=10, pady=5)

        # 搜索栏
        search_row = ttk.Frame(select_frame)
        search_row.pack(fill=tk.X, pady=(0, 5))

        lbl_match_search = ttk.Label(search_row, text="搜索:")
        if self.icons:
            lbl_match_search.configure(image=self.icons.get("search", 16), compound="left")
        lbl_match_search.pack(side=tk.LEFT, padx=(0, 5))
        self.match_search = ttk.Entry(search_row, width=40)
        self.match_search.bind('<KeyRelease>', lambda e: self.filter_match_firmwares())
        self.match_search.pack(side=tk.LEFT, padx=5, fill=tk.X, expand=True)

        ttk.Button(
            search_row,
            text="清空",
            command=lambda: (self.match_search.delete(0, tk.END), self.filter_match_firmwares()),
            width=8
        ).pack(side=tk.LEFT, padx=5)

        list_frame = ttk.Frame(select_frame)
        list_frame.pack(fill=tk.BOTH, expand=True)

        scrollbar = ttk.Scrollbar(list_frame)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.match_fw_list = ttk.Treeview(
            list_frame,
            columns=('filename', 'size', 'path'),
            show='headings',
            height=5,
            yscrollcommand=scrollbar.set
        )
        scrollbar.config(command=self.match_fw_list.yview)

        # 设置列标题（点击可排序）
        self.match_fw_list.heading('filename', text='固件文件名 ▲', command=lambda: self.sort_match_list('filename'))
        self.match_fw_list.heading('size', text='文件大小 ▲', command=lambda: self.sort_match_list('size'))
        self.match_fw_list.heading('path', text='文件路径 ▲', command=lambda: self.sort_match_list('path'))

        self.match_fw_list.column('filename', width=300)
        self.match_fw_list.column('size', width=100)
        self.match_fw_list.column('path', width=400)

        self.match_fw_list.pack(fill=tk.BOTH, expand=True)
        self.match_fw_list.bind('<<TreeviewSelect>>', self.on_match_firmware_selected)
        self.match_fw_list.bind('<Button-3>', lambda e: self.show_copy_menu(e, self.match_fw_list))

        # 排序状态：列名 -> (reverse, last_sort_column)
        self.match_sort_column = 'filename'
        self.match_sort_reverse = False

        # SBL匹配结果
        sbl_frame = ttk.LabelFrame(frame, text="推荐SBL固件 (Top 5)", padding=10)
        sbl_frame.pack(fill=tk.X, padx=10, pady=5)

        # 添加滚动条
        sbl_scroll = ttk.Scrollbar(sbl_frame)
        sbl_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        self.match_sbl_tree = ttk.Treeview(
            sbl_frame,
            columns=('filename', 'variant', 'score', 'path'),
            show='headings',
            height=5,
            yscrollcommand=sbl_scroll.set
        )
        sbl_scroll.config(command=self.match_sbl_tree.yview)

        self.match_sbl_tree.heading('filename', text='文件名')
        self.match_sbl_tree.heading('variant', text='变体')
        self.match_sbl_tree.heading('score', text='匹配度')
        self.match_sbl_tree.heading('path', text='文件路径')

        # 设置列宽和自适应
        self.match_sbl_tree.column('filename', width=250, minwidth=150, stretch=True)
        self.match_sbl_tree.column('variant', width=80, minwidth=60, stretch=True)
        self.match_sbl_tree.column('score', width=80, minwidth=60, stretch=True)
        self.match_sbl_tree.column('path', width=400, minwidth=200, stretch=True)

        self.match_sbl_tree.pack(fill=tk.BOTH, expand=True)
        self.match_sbl_tree.bind('<Button-3>', lambda e: self.show_copy_menu(e, self.match_sbl_tree))

        # 配置匹配结果
        cfg_frame = ttk.LabelFrame(frame, text="推荐雷达配置 (Top 8)", padding=10)
        cfg_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)

        cfg_scroll = ttk.Scrollbar(cfg_frame)
        cfg_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        self.match_cfg_tree = ttk.Treeview(
            cfg_frame,
            columns=('filename', 'application', 'params', 'score', 'path'),
            show='headings',
            yscrollcommand=cfg_scroll.set
        )
        cfg_scroll.config(command=self.match_cfg_tree.yview)

        self.match_cfg_tree.heading('filename', text='文件名')
        self.match_cfg_tree.heading('application', text='应用场景')
        self.match_cfg_tree.heading('params', text='参数')
        self.match_cfg_tree.heading('score', text='匹配度')
        self.match_cfg_tree.heading('path', text='文件路径')

        # 设置列宽和自适应
        self.match_cfg_tree.column('filename', width=200, minwidth=150, stretch=True)
        self.match_cfg_tree.column('application', width=120, minwidth=80, stretch=True)
        self.match_cfg_tree.column('params', width=150, minwidth=100, stretch=True)
        self.match_cfg_tree.column('score', width=80, minwidth=60, stretch=True)
        self.match_cfg_tree.column('path', width=350, minwidth=200, stretch=True)

        self.match_cfg_tree.pack(fill=tk.BOTH, expand=True)
        self.match_cfg_tree.bind('<Button-3>', lambda e: self.show_copy_menu(e, self.match_cfg_tree))

    # ========== 事件处理 ==========

    def show_copy_menu(self, event, tree):
        """显示右键复制菜单"""
        # 选中右键点击的项
        item = tree.identify_row(event.y)
        if item:
            tree.selection_set(item)

            # 创建右键菜单
            menu = tk.Menu(tree, tearoff=0)
            menu.add_command(label="复制文件名", command=lambda: self.copy_filename(tree))
            menu.add_command(label="复制完整路径", command=lambda: self.copy_path(tree))

            # 判断当前树是哪个类型，添加相应的"添加到烧录"选项
            if tree == self.fw_tree or tree == self.match_fw_list:
                # 应用固件（固件管理标签页或智能匹配标签页）
                menu.add_separator()
                menu.add_command(label="添加到烧录", command=lambda: self.add_to_flash(tree, 'app'))
            elif tree == self.sbl_tree or tree == self.match_sbl_tree:
                # SBL固件（SBL管理标签页或智能匹配推荐）
                menu.add_separator()
                menu.add_command(label="添加到烧录", command=lambda: self.add_to_flash(tree, 'sbl'))

            # 显示菜单
            menu.post(event.x_root, event.y_root)

    def copy_filename(self, tree):
        """复制文件名"""
        selection = tree.selection()
        if not selection:
            return

        # 获取选中项的第一个值（文件名）
        item = selection[0]
        values = tree.item(item)['values']
        filename = str(values[0]) if values else ""

        # 复制到剪贴板
        self.parent.clipboard_clear()
        self.parent.clipboard_append(filename)
        self.parent.update()

        messagebox.showinfo("提示", f"已复制文件名: {filename}")

    def copy_path(self, tree):
        """复制完整路径"""
        selection = tree.selection()
        if not selection:
            return

        item = selection[0]
        path = self._get_tree_item_path(tree, item)
        if not path:
            messagebox.showwarning("警告", "无法获取路径信息")
            return

        # 复制到剪贴板
        self.parent.clipboard_clear()
        self.parent.clipboard_append(path)
        self.parent.update()

        messagebox.showinfo("提示", f"已复制完整路径")

    def add_to_flash(self, tree, firmware_type):
        """添加固件到烧录标签页

        Args:
            tree: Treeview对象
            firmware_type: 固件类型（'app'=应用固件, 'sbl'=SBL固件）
        """
        selection = tree.selection()
        if not selection:
            return

        item = selection[0]
        path = self._get_tree_item_path(tree, item)
        if not path:
            messagebox.showwarning("警告", "无法获取路径信息")
            return

        # 验证文件存在
        if not os.path.exists(path):
            messagebox.showerror("错误", f"文件不存在: {path}")
            return

        # 根据类型设置到主应用的对应变量
        if firmware_type == 'app':
            # 设置应用固件
            self.main_app.app_file.set(path)
            self.main_app.log(f"已添加应用固件到烧录: {os.path.basename(path)}\n", "SUCCESS")

            # 更新界面状态
            if hasattr(self.main_app, 'app_status_label'):
                self.main_app.app_status_label.config(text="已选择", fg="green")
            if hasattr(self.main_app, 'app_path_label'):
                self.main_app.app_path_label.config(text=path)

            messagebox.showinfo("成功", f"已添加应用固件到烧录\n{os.path.basename(path)}")

        elif firmware_type == 'sbl':
            # 设置SBL固件
            self.main_app.sbl_file.set(path)
            self.main_app.log(f"已添加SBL固件到烧录: {os.path.basename(path)}\n", "SUCCESS")

            # 更新界面状态
            if hasattr(self.main_app, 'sbl_status_label'):
                self.main_app.sbl_status_label.config(text="已选择", fg="green")
            if hasattr(self.main_app, 'sbl_path_label'):
                self.main_app.sbl_path_label.config(text=path)

            messagebox.showinfo("成功", f"已添加SBL固件到烧录\n{os.path.basename(path)}")

    def add_directory_to_list(self):
        """添加扫描目录到列表"""
        directory = filedialog.askdirectory(title="选择扫描目录")
        if directory and directory not in self.scan_directories:
            self.scan_directories.append(directory)
            self.update_directory_list()

    def remove_selected_directory(self):
        """删除选中的目录"""
        selection = self.dir_tree.selection()
        if not selection:
            messagebox.showwarning("警告", "请先选择要删除的目录")
            return

        for item in selection:
            path = self.dir_tree.item(item)['values'][0]
            if path in self.scan_directories:
                self.scan_directories.remove(path)

        self.update_directory_list()

    def clear_all_directories(self):
        """清空所有目录"""
        if messagebox.askyesno("确认", "确定要清空所有扫描目录吗？"):
            self.scan_directories.clear()
            self.update_directory_list()

    def restore_default_directories(self):
        """恢复默认扫描目录"""
        print(f"\n[DEBUG] restore_default_directories() 被调用")

        default_dirs = [
            r"C:\ti\MMWAVE_L_SDK_06_01_00_01",
            r"C:\ti\radar_toolbox_3_30_00_06"
        ]

        # 检查默认目录是否存在
        existing_dirs = [d for d in default_dirs if os.path.exists(d)]
        missing_dirs = [d for d in default_dirs if not os.path.exists(d)]

        print(f"[DEBUG] 默认目录: {default_dirs}")
        print(f"[DEBUG] 存在的目录: {existing_dirs}")
        print(f"[DEBUG] 缺失的目录: {missing_dirs}")

        msg = "将恢复以下默认扫描目录：\n\n"
        if existing_dirs:
            msg += "存在的目录：\n"
            for d in existing_dirs:
                msg += f"  {d}\n"
        if missing_dirs:
            msg += "\n不存在的目录（将跳过）：\n"
            for d in missing_dirs:
                msg += f"  {d}\n"
        msg += "\n确定要恢复默认目录吗？"

        if messagebox.askyesno("恢复默认目录", msg):
            self.scan_directories = existing_dirs.copy()
            _dprint(f"[DEBUG] 已设置 scan_directories = {self.scan_directories}")
            self.update_directory_list()
            messagebox.showinfo("完成", f"已恢复 {len(existing_dirs)} 个默认扫描目录")

    def update_directory_list(self):
        """更新目录列表显示"""
        self.dir_tree.delete(*self.dir_tree.get_children())

        for directory in self.scan_directories:
            status = "存在" if os.path.exists(directory) else "不存在"
            self.dir_tree.insert('', 'end', values=(directory, status))

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
            # 清空旧结果，避免累积
            self.matcher.clear_results()

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
        # 调试日志
        _dprint("\n[DEBUG] ======== _scan_completed() 被调用 ========")

        self.is_scanning = False
        self.btn_scan.config(state='normal')
        self.progress.stop()

        # 更新统计
        stats = self.matcher.get_statistics()
        _dprint(f"[DEBUG] 统计信息: {stats}")

        self.lbl_app.config(text=f"应用固件: {stats['application_count']}")
        self.lbl_sbl.config(text=f"SBL固件: {stats['sbl_count']}")
        self.lbl_config.config(text=f"雷达配置: {stats['config_count']}")

        # 更新列表
        _dprint("[DEBUG] 开始调用 update_firmware_list()")
        self.update_firmware_list()

        _dprint("[DEBUG] 开始调用 update_sbl_list()")
        self.update_sbl_list()

        _dprint("[DEBUG] 开始调用 update_config_list()")
        self.update_config_list()

        _dprint("[DEBUG] 开始调用 update_match_list()")
        self.update_match_list()

        # 更新筛选器选项
        _dprint("[DEBUG] 开始调用 update_filter_options()")
        self.update_filter_options()

        # 防止筛选器/事件导致列表被意外清空：再强制刷新一次
        self.update_firmware_list()
        self.update_sbl_list()
        self.update_config_list()

        _dprint("[DEBUG] ======== _scan_completed() 执行完成 ========\n")

        # 关键修复：自动切换到"应用固件"标签页，让用户看到结果
        # 标签页索引：0=扫描目录, 1=应用固件, 2=SBL固件, 3=雷达配置, 4=智能匹配
        self.notebook.select(1)  # 切换到"应用固件"标签页
        _dprint("[DEBUG] 已自动切换到'应用固件'标签页")

        messagebox.showinfo(
            "扫描完成",
            f"扫描完成！\n\n"
            f"应用固件: {stats['application_count']}\n"
            f"SBL固件: {stats['sbl_count']}\n"
            f"雷达配置: {stats['config_count']}"
        )

    def update_firmware_list(self):
        """更新应用固件列表"""
        # 调试日志
        _dprint("[DEBUG] update_firmware_list() 被调用")
        _dprint(f"[DEBUG] matcher.application_firmwares 数量: {len(self.matcher.application_firmwares)}")

        self.fw_tree.delete(*self.fw_tree.get_children())
        self._clear_tree_path_map(self.fw_tree)

        count = 0
        for fw in self.matcher.application_firmwares:
            item_id = self.fw_tree.insert('', 'end', values=(
                fw.filename,
                fw.category,
                fw.subcategory,
                fw.processor,
                fw.version
            ))
            self._remember_tree_item_path(self.fw_tree, item_id, fw.path)
            count += 1

        _dprint(f"[DEBUG] 成功插入 {count} 条记录到 fw_tree")
        _dprint(f"[DEBUG] fw_tree 当前子项数量: {len(self.fw_tree.get_children())}")

        # 关键修复：强制设置列宽度（防止列宽度为0）
        self.fw_tree.column('filename', width=300, minwidth=200)
        self.fw_tree.column('category', width=100, minwidth=80)
        self.fw_tree.column('subcategory', width=150, minwidth=100)
        self.fw_tree.column('processor', width=180, minwidth=120)
        self.fw_tree.column('version', width=100, minwidth=80)
        _dprint("[DEBUG] 已强制设置列宽度")

        # 关键修复：强制刷新TreeView显示
        self.fw_tree.update_idletasks()
        self.fw_tree.update()
        _dprint("[DEBUG] 已强制刷新 fw_tree 显示")

    def update_sbl_list(self):
        """更新SBL固件列表"""
        # 调试日志
        _dprint("[DEBUG] update_sbl_list() 被调用")
        _dprint(f"[DEBUG] matcher.sbl_firmwares 数量: {len(self.matcher.sbl_firmwares)}")

        self.sbl_tree.delete(*self.sbl_tree.get_children())
        self._clear_tree_path_map(self.sbl_tree)

        count = 0
        for sbl in self.matcher.sbl_firmwares:
            item_id = self.sbl_tree.insert('', 'end', values=(
                sbl.filename,
                sbl.variant,
                sbl.flash_address,
                sbl.flash_size,
                sbl.description
            ))
            self._remember_tree_item_path(self.sbl_tree, item_id, sbl.path)
            count += 1

        _dprint(f"[DEBUG] 成功插入 {count} 条记录到 sbl_tree")
        _dprint(f"[DEBUG] sbl_tree 当前子项数量: {len(self.sbl_tree.get_children())}")

        # 关键修复：强制设置列宽度（列名必须与定义时一致）
        self.sbl_tree.column('filename', width=300, minwidth=200)
        self.sbl_tree.column('variant', width=100, minwidth=80)
        self.sbl_tree.column('flash_addr', width=120, minwidth=100)      # 注意：是 flash_addr 不是 flash_address
        self.sbl_tree.column('size', width=100, minwidth=80)              # 注意：是 size 不是 flash_size
        self.sbl_tree.column('description', width=250, minwidth=150)
        _dprint("[DEBUG] 已强制设置 SBL 列宽度")

        # 关键修复：强制刷新TreeView显示
        self.sbl_tree.update_idletasks()
        self.sbl_tree.update()
        _dprint("[DEBUG] 已强制刷新 sbl_tree 显示")

    def update_config_list(self):
        """更新雷达配置列表"""
        self.cfg_tree.delete(*self.cfg_tree.get_children())
        self._clear_tree_path_map(self.cfg_tree)
        for cfg in self.matcher.config_files:
            channels = f"{cfg.tx_channels}TX/{cfg.rx_channels}RX" if cfg.tx_channels > 0 else "N/A"
            item_id = self.cfg_tree.insert('', 'end', values=(
                cfg.filename,
                cfg.application,
                channels,
                cfg.range_m if cfg.range_m > 0 else "N/A",
                cfg.mode
            ))
            self._remember_tree_item_path(self.cfg_tree, item_id, cfg.path)

    def update_match_list(self):
        """更新匹配列表"""
        self.match_fw_list.delete(*self.match_fw_list.get_children())
        self._clear_tree_path_map(self.match_fw_list)
        for fw in self.matcher.application_firmwares:
            # 获取文件大小
            size_str = self._format_file_size(fw.path)
            item_id = self.match_fw_list.insert('', 'end', values=(fw.filename, size_str, fw.path))
            self._remember_tree_item_path(self.match_fw_list, item_id, fw.path)

    def filter_match_firmwares(self):
        """根据搜索关键词过滤智能匹配的固件列表"""
        keyword = self.match_search.get().lower()

        self.match_fw_list.delete(*self.match_fw_list.get_children())
        self._clear_tree_path_map(self.match_fw_list)

        for fw in self.matcher.application_firmwares:
            # 搜索文件名或路径
            if keyword and keyword not in fw.filename.lower() and keyword not in fw.path.lower():
                continue

            # 获取文件大小
            size_str = self._format_file_size(fw.path)
            item_id = self.match_fw_list.insert('', 'end', values=(fw.filename, size_str, fw.path))
            self._remember_tree_item_path(self.match_fw_list, item_id, fw.path)

    def update_filter_options(self):
        """更新筛选器选项"""
        _dprint("[DEBUG] update_filter_options() 被调用")

        # 关键修复v2.4.6：临时解绑所有筛选器事件，防止更新选项时触发筛选导致列表被清空
        # ===== 应用固件筛选器事件解绑 =====
        self.fw_category.unbind('<<ComboboxSelected>>')
        self.fw_subcategory.unbind('<<ComboboxSelected>>')
        self.fw_processor.unbind('<<ComboboxSelected>>')
        self.fw_version.unbind('<<ComboboxSelected>>')
        self.fw_size.unbind('<<ComboboxSelected>>')
        self.fw_path.unbind('<<ComboboxSelected>>')

        # ===== SBL固件筛选器事件解绑 =====
        self.sbl_variant.unbind('<<ComboboxSelected>>')
        self.sbl_flash.unbind('<<ComboboxSelected>>')

        # ===== 雷达配置筛选器事件解绑 =====
        self.cfg_app.unbind('<<ComboboxSelected>>')
        self.cfg_mode.unbind('<<ComboboxSelected>>')
        self.cfg_channels.unbind('<<ComboboxSelected>>')
        self.cfg_range.unbind('<<ComboboxSelected>>')

        _dprint("[DEBUG] 所有筛选器事件已解绑")

        # ===== 应用固件筛选器选项更新 =====
        categories = set(fw.category for fw in self.matcher.application_firmwares if fw.category)
        self.fw_category['values'] = ["全部"] + sorted(categories)
        self.fw_category.current(0)  # 重置为"全部"

        subcategories = set(fw.subcategory for fw in self.matcher.application_firmwares if fw.subcategory)
        self.fw_subcategory['values'] = ["全部"] + sorted(subcategories)
        self.fw_subcategory.current(0)

        processors = set(fw.processor for fw in self.matcher.application_firmwares if fw.processor)
        self.fw_processor['values'] = ["全部"] + sorted(processors)
        self.fw_processor.current(0)

        versions = set(fw.version for fw in self.matcher.application_firmwares if fw.version)
        self.fw_version['values'] = ["全部"] + sorted(versions, reverse=True)
        self.fw_version.current(0)

        # 文件大小范围（按KB/MB分组）
        size_ranges = set(self._format_size(fw.size) for fw in self.matcher.application_firmwares if fw.size > 0)
        self.fw_size['values'] = ["全部"] + sorted(size_ranges)
        self.fw_size.current(0)

        # 文件路径（提取父目录）
        paths = set(str(Path(fw.path).parent) for fw in self.matcher.application_firmwares if fw.path)
        # 只显示包含固件的目录
        common_paths = [p for p in paths if len([fw for fw in self.matcher.application_firmwares if str(Path(fw.path).parent) == p]) >= 1]
        self.fw_path['values'] = ["全部"] + sorted(common_paths)[:10]  # 限制前10个常用目录
        self.fw_path.current(0)

        # ===== SBL固件筛选器选项更新 =====
        variants = set(sbl.variant for sbl in self.matcher.sbl_firmwares if sbl.variant)
        self.sbl_variant['values'] = ["全部"] + sorted(variants)
        self.sbl_variant.current(0)

        flash_addrs = set(sbl.flash_address for sbl in self.matcher.sbl_firmwares if sbl.flash_address)
        self.sbl_flash['values'] = ["全部"] + sorted(flash_addrs)
        self.sbl_flash.current(0)

        # ===== 雷达配置筛选器选项更新 =====
        apps = set(cfg.application for cfg in self.matcher.config_files if cfg.application)
        self.cfg_app['values'] = ["全部"] + sorted(apps)
        self.cfg_app.current(0)  # 重置为"全部"

        # 通道数筛选器
        channels = set(f"{cfg.tx_channels}TX/{cfg.rx_channels}RX" for cfg in self.matcher.config_files if cfg.tx_channels > 0)
        self.cfg_channels['values'] = ["全部"] + sorted(channels)
        self.cfg_channels.current(0)

        # 检测距离筛选器
        ranges = set(f"{cfg.range_m}m" for cfg in self.matcher.config_files if cfg.range_m > 0)
        self.cfg_range['values'] = ["全部"] + sorted(ranges, key=lambda x: float(x.rstrip('m')))
        self.cfg_range.current(0)

        # 关键修复v2.4.6：重新绑定所有筛选器事件
        # ===== 应用固件筛选器事件重新绑定 =====
        self.fw_category.bind('<<ComboboxSelected>>', lambda e: self.on_fw_filter_change())
        self.fw_subcategory.bind('<<ComboboxSelected>>', lambda e: self.on_fw_filter_change())
        self.fw_processor.bind('<<ComboboxSelected>>', lambda e: self.on_fw_filter_change())
        self.fw_version.bind('<<ComboboxSelected>>', lambda e: self.filter_firmwares())
        self.fw_size.bind('<<ComboboxSelected>>', lambda e: self.filter_firmwares())
        self.fw_path.bind('<<ComboboxSelected>>', lambda e: self.filter_firmwares())

        # ===== SBL固件筛选器事件重新绑定 =====
        self.sbl_variant.bind('<<ComboboxSelected>>', lambda e: self.on_sbl_filter_change())
        self.sbl_flash.bind('<<ComboboxSelected>>', lambda e: self.filter_sbls())

        # ===== 雷达配置筛选器事件重新绑定 =====
        self.cfg_app.bind('<<ComboboxSelected>>', lambda e: self.on_cfg_filter_change())
        self.cfg_mode.bind('<<ComboboxSelected>>', lambda e: self.on_cfg_filter_change())
        self.cfg_channels.bind('<<ComboboxSelected>>', lambda e: self.on_cfg_filter_change())
        self.cfg_range.bind('<<ComboboxSelected>>', lambda e: self.filter_configs())

        _dprint("[DEBUG] 所有筛选器事件已重新绑定")

        # 调试：检查 update_filter_options 后 TreeView 的数据量
        _dprint("[DEBUG] update_filter_options() 结束后:")
        _dprint(f"[DEBUG]   fw_tree 数量: {len(self.fw_tree.get_children())}")
        _dprint(f"[DEBUG]   sbl_tree 数量: {len(self.sbl_tree.get_children())}")
        _dprint(f"[DEBUG]   cfg_tree 数量: {len(self.cfg_tree.get_children())}")

    def _format_size(self, size_bytes):
        """格式化文件大小"""
        if size_bytes < 1024:
            return f"{size_bytes}B"
        elif size_bytes < 1024 * 1024:
            return f"{size_bytes // 1024}KB"
        else:
            return f"{size_bytes // (1024 * 1024)}MB"

    def on_fw_filter_change(self):
        """应用固件筛选器级联更新"""
        # 获取当前筛选条件
        category = self.fw_category.get()
        subcategory = self.fw_subcategory.get()
        processor = self.fw_processor.get()

        # 根据当前筛选更新可用选项
        filtered_fws = [fw for fw in self.matcher.application_firmwares]

        # 按类别筛选
        if category != "全部":
            filtered_fws = [fw for fw in filtered_fws if fw.category == category]

        # 按子类别筛选
        if subcategory != "全部":
            filtered_fws = [fw for fw in filtered_fws if fw.subcategory == subcategory]

        # 按处理器筛选
        if processor != "全部":
            filtered_fws = [fw for fw in filtered_fws if fw.processor == processor]

        # 更新子类别选项（基于类别）
        if category != "全部":
            subcats = set(fw.subcategory for fw in self.matcher.application_firmwares
                         if fw.category == category and fw.subcategory)
            self.fw_subcategory['values'] = ["全部"] + sorted(subcats)
        else:
            subcats = set(fw.subcategory for fw in self.matcher.application_firmwares if fw.subcategory)
            self.fw_subcategory['values'] = ["全部"] + sorted(subcats)

        # 更新处理器选项（基于类别和子类别）
        procs = set(fw.processor for fw in filtered_fws if fw.processor)
        self.fw_processor['values'] = ["全部"] + sorted(procs)

        # 更新版本选项（基于类别、子类别、处理器）
        vers = set(fw.version for fw in filtered_fws if fw.version)
        self.fw_version['values'] = ["全部"] + sorted(vers, reverse=True)

        # 执行筛选
        self.filter_firmwares()

    def on_sbl_filter_change(self):
        """筛选器级联更新"""
        variant = self.sbl_variant.get()

        # 根据变体更新Flash地址选项
        if variant != "全部":
            flash_addrs = set(sbl.flash_address for sbl in self.matcher.sbl_firmwares
                            if sbl.variant == variant and sbl.flash_address)
            self.sbl_flash['values'] = ["全部"] + sorted(flash_addrs)
        else:
            flash_addrs = set(sbl.flash_address for sbl in self.matcher.sbl_firmwares if sbl.flash_address)
            self.sbl_flash['values'] = ["全部"] + sorted(flash_addrs)

        # 执行筛选
        self.filter_sbls()

    def on_cfg_filter_change(self):
        """雷达配置筛选器级联更新"""
        app = self.cfg_app.get()
        mode = self.cfg_mode.get()
        channels_filter = self.cfg_channels.get()

        # 根据当前筛选更新可用选项
        filtered_cfgs = [cfg for cfg in self.matcher.config_files]

        # 按应用场景筛选
        if app != "全部":
            filtered_cfgs = [cfg for cfg in filtered_cfgs if cfg.application == app]

        # 按模式筛选
        if mode != "全部":
            filtered_cfgs = [cfg for cfg in filtered_cfgs if mode in cfg.mode]

        # 按通道数筛选
        if channels_filter != "全部":
            filtered_cfgs = [cfg for cfg in filtered_cfgs
                           if f"{cfg.tx_channels}TX/{cfg.rx_channels}RX" == channels_filter]

        # 更新通道数选项（基于应用和模式）
        channels = set(f"{cfg.tx_channels}TX/{cfg.rx_channels}RX"
                      for cfg in filtered_cfgs if cfg.tx_channels > 0)
        self.cfg_channels['values'] = ["全部"] + sorted(channels)

        # 更新检测距离选项（基于应用、模式、通道）
        ranges = set(f"{cfg.range_m}m" for cfg in filtered_cfgs if cfg.range_m > 0)
        self.cfg_range['values'] = ["全部"] + sorted(ranges, key=lambda x: float(x.rstrip('m')))

        # 执行筛选
        self.filter_configs()

    def filter_firmwares(self):
        """筛选应用固件"""
        category = self.fw_category.get()
        subcategory = self.fw_subcategory.get()
        processor = self.fw_processor.get()
        version = self.fw_version.get()
        size_filter = self.fw_size.get()
        path_filter = self.fw_path.get()
        keyword = self.fw_search.get().lower()

        self.fw_tree.delete(*self.fw_tree.get_children())
        self._clear_tree_path_map(self.fw_tree)

        for fw in self.matcher.application_firmwares:
            if category != "全部" and fw.category != category:
                continue
            if subcategory != "全部" and fw.subcategory != subcategory:
                continue
            if processor != "全部" and fw.processor != processor:
                continue
            if version != "全部" and fw.version != version:
                continue
            if size_filter != "全部" and size_filter not in self._format_size(fw.size):
                continue
            if path_filter != "全部" and path_filter not in fw.path:
                continue

            # 扩展搜索：支持文件名/路径/类别/子类别/处理器/版本
            if keyword:
                search_fields = [
                    fw.filename.lower(),
                    fw.path.lower(),
                    fw.category.lower() if fw.category else "",
                    fw.subcategory.lower() if fw.subcategory else "",
                    fw.processor.lower() if fw.processor else "",
                    fw.version.lower() if fw.version else ""
                ]
                if not any(keyword in field for field in search_fields):
                    continue

            item_id = self.fw_tree.insert('', 'end', values=(
                fw.filename,
                fw.category,
                fw.subcategory,
                fw.processor,
                fw.version
            ))
            self._remember_tree_item_path(self.fw_tree, item_id, fw.path)

    def filter_sbls(self):
        """筛选SBL固件"""
        variant_filter = self.sbl_variant.get()
        flash_filter = self.sbl_flash.get()
        keyword = self.sbl_search.get().lower()

        self.sbl_tree.delete(*self.sbl_tree.get_children())
        self._clear_tree_path_map(self.sbl_tree)

        for sbl in self.matcher.sbl_firmwares:
            if variant_filter != "全部" and sbl.variant != variant_filter:
                continue
            if flash_filter != "全部" and sbl.flash_address != flash_filter:
                continue

            # 扩展搜索：支持文件名/路径/变体/说明
            if keyword:
                search_fields = [
                    sbl.filename.lower(),
                    sbl.path.lower(),
                    sbl.variant.lower() if sbl.variant else "",
                    sbl.description.lower() if sbl.description else ""
                ]
                if not any(keyword in field for field in search_fields):
                    continue

            item_id = self.sbl_tree.insert('', 'end', values=(
                sbl.filename,
                sbl.variant,
                sbl.flash_address,
                self._format_size(sbl.size),
                sbl.description
            ))
            self._remember_tree_item_path(self.sbl_tree, item_id, sbl.path)

    def filter_configs(self):
        """筛选雷达配置"""
        app = self.cfg_app.get()
        mode = self.cfg_mode.get()
        channels_filter = self.cfg_channels.get()
        range_filter = self.cfg_range.get()
        keyword = self.cfg_search.get().lower()

        self.cfg_tree.delete(*self.cfg_tree.get_children())
        self._clear_tree_path_map(self.cfg_tree)

        for cfg in self.matcher.config_files:
            if app != "全部" and cfg.application != app:
                continue
            if mode != "全部" and mode not in cfg.mode:
                continue

            # 通道数筛选
            channels_str = f"{cfg.tx_channels}TX/{cfg.rx_channels}RX" if cfg.tx_channels > 0 else "N/A"
            if channels_filter != "全部" and channels_filter != channels_str:
                continue

            # 检测距离筛选
            if range_filter != "全部":
                range_str = f"{cfg.range_m}m" if cfg.range_m > 0 else "N/A"
                if range_filter != range_str:
                    continue

            # 扩展搜索：支持文件名/路径/应用场景/通道数/检测距离/模式
            if keyword:
                range_display = f"{cfg.range_m}m" if cfg.range_m > 0 else "N/A"
                search_fields = [
                    cfg.filename.lower(),
                    cfg.path.lower(),
                    cfg.application.lower() if cfg.application else "",
                    channels_str.lower(),
                    range_display.lower(),
                    cfg.mode.lower() if cfg.mode else ""
                ]
                if not any(keyword in field for field in search_fields):
                    continue

            range_display = f"{cfg.range_m}m" if cfg.range_m > 0 else "N/A"
            item_id = self.cfg_tree.insert('', 'end', values=(
                cfg.filename,
                cfg.application,
                channels_str,
                range_display,
                cfg.mode
            ))
            self._remember_tree_item_path(self.cfg_tree, item_id, cfg.path)

    def on_firmware_selected(self, event):
        """应用固件选中事件"""
        selection = self.fw_tree.selection()
        if not selection:
            return

        path = self._get_tree_item_path(self.fw_tree, selection[0])
        if not path:
            return
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

        path = self._get_tree_item_path(self.sbl_tree, selection[0])
        if not path:
            return
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

        path = self._get_tree_item_path(self.cfg_tree, selection[0])
        if not path:
            return
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

    def add_to_basic_flash(self):
        """将选中的应用固件和SBL固件添加到基本烧录标签页"""
        # 获取选中的应用固件
        fw_selection = self.match_fw_list.selection()
        if not fw_selection:
            messagebox.showwarning("警告", "请先选择一个应用固件")
            return

        app_path = self._get_tree_item_path(self.match_fw_list, fw_selection[0])
        if not app_path:
            messagebox.showwarning("警告", "无法获取应用固件路径")
            return

        # 获取选中的SBL固件（推荐列表中的第一个或用户选中的）
        sbl_selection = self.match_sbl_tree.selection()
        if not sbl_selection:
            # 如果没有手动选择，使用推荐列表的第一个
            sbl_items = self.match_sbl_tree.get_children()
            if not sbl_items:
                messagebox.showwarning("警告", "没有可用的SBL固件推荐\n\n请先选择一个应用固件触发智能匹配")
                return
            sbl_selection = [sbl_items[0]]

        sbl_path = self._get_tree_item_path(self.match_sbl_tree, sbl_selection[0])
        if not sbl_path:
            messagebox.showwarning("警告", "无法获取SBL固件路径")
            return

        # 设置到主应用的固件路径
        try:
            # 关键修复：设置StringVar变量（供analyze_firmware使用）
            if hasattr(self.main_app, 'sbl_file'):
                self.main_app.sbl_file.set(sbl_path)
            if hasattr(self.main_app, 'app_file'):
                self.main_app.app_file.set(app_path)
            if hasattr(self.main_app, 'firmware_file'):
                self.main_app.firmware_file.set(app_path)  # 兼容旧代码

            # 更新SBL路径显示
            if hasattr(self.main_app, 'sbl_path_label'):
                self.main_app.sbl_path_label.config(text=sbl_path)
            if hasattr(self.main_app, 'sbl_status_label'):
                self.main_app.sbl_status_label.config(text="已选择", fg="green")

            # 更新App路径显示
            if hasattr(self.main_app, 'app_path_label'):
                self.main_app.app_path_label.config(text=app_path)
            if hasattr(self.main_app, 'app_status_label'):
                self.main_app.app_status_label.config(text="已选择", fg="green")

            # 切换到基本烧录标签页
            if hasattr(self.main_app, 'notebook'):
                self.main_app.notebook.select(0)  # 切换到第一个标签页（基本烧录）

            messagebox.showinfo(
                "成功",
                f"已添加到基本烧录:\n\n" +
                f"SBL固件: {Path(sbl_path).name}\n" +
                f"App固件: {Path(app_path).name}"
            )

        except Exception as e:
            messagebox.showerror("错误", f"添加失败: {str(e)}")

    def on_match_firmware_selected(self, event):
        """匹配固件选中事件"""
        selection = self.match_fw_list.selection()
        if not selection:
            return

        path = self._get_tree_item_path(self.match_fw_list, selection[0])
        if not path:
            return
        fw = next((f for f in self.matcher.application_firmwares if f.path == path), None)

        if not fw:
            return

        # 清空之前的匹配结果
        self.match_sbl_tree.delete(*self.match_sbl_tree.get_children())
        self.match_cfg_tree.delete(*self.match_cfg_tree.get_children())

        # 匹配SBL
        sbl_matches = self.matcher.match_sbl_for_firmware(fw)
        for i, (sbl, score) in enumerate(sbl_matches[:5]):
            tag_list = []
            if i == 0:  # 高亮最佳匹配
                tag_list.append('best')

            item_id = self.match_sbl_tree.insert('', 'end', values=(
                sbl.filename,
                sbl.variant,
                f"{score:.0f}%",
                sbl.path
            ), tags=tuple(tag_list))
            self._remember_tree_item_path(self.match_sbl_tree, item_id, sbl.path)

        # 匹配配置
        cfg_matches = self.matcher.match_configs_for_firmware(fw)
        for i, (cfg, score, validation) in enumerate(cfg_matches[:8]):
            params = []
            if cfg.tx_channels > 0:
                params.append(f"{cfg.tx_channels}TX/{cfg.rx_channels}RX")
            if cfg.range_m > 0:
                params.append(f"{cfg.range_m}m")
            if cfg.mode:
                params.append(cfg.mode)

            # v4.0: 根据评分设置标签
            tag_list = []
            if score <= -999999:
                score_text = "不可用"
                tag_list.append('unavailable')
            elif score < 0:
                score_text = f"警告 {score:.0f}"
                tag_list.append('warning')
            elif i == 0:
                score_text = f"最佳 {score:.0f}"
                tag_list.append('best')
            else:
                score_text = f"{score:.0f}"

            item_id = self.match_cfg_tree.insert('', 'end', values=(
                cfg.filename,
                cfg.application,
                " | ".join(params),
                score_text,
                cfg.path
            ), tags=tuple(tag_list))
            self._remember_tree_item_path(self.match_cfg_tree, item_id, cfg.path)

        # 配置高亮样式（v4.0更新）
        self.match_sbl_tree.tag_configure('best', background='#c8ffc8')
        self.match_cfg_tree.tag_configure('best', background='#c8ffc8')
        self.match_cfg_tree.tag_configure('warning', background='#fff4c8')  # 黄色警告
        self.match_cfg_tree.tag_configure('unavailable', background='#ffc8c8')  # 红色不可用

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

            self._clear_tree_path_map(self.fw_tree)
            self._clear_tree_path_map(self.sbl_tree)
            self._clear_tree_path_map(self.cfg_tree)
            self._clear_tree_path_map(self.match_fw_list)
            self._clear_tree_path_map(self.match_sbl_tree)
            self._clear_tree_path_map(self.match_cfg_tree)

            self.lbl_app.config(text="应用固件: 0")
            self.lbl_sbl.config(text="SBL固件: 0")
            self.lbl_config.config(text="雷达配置: 0")

            self.fw_detail.delete('1.0', tk.END)
            self.sbl_detail.delete('1.0', tk.END)
            self.cfg_detail.delete('1.0', tk.END)

    def refresh(self):
        """刷新标签页数据"""
        pass

    def _format_file_size(self, file_path):
        """格式化文件大小显示"""
        try:
            size = os.path.getsize(file_path)
            if size < 1024:
                return f"{size} B"
            elif size < 1024 * 1024:
                return f"{size / 1024:.1f} KB"
            else:
                return f"{size / (1024 * 1024):.2f} MB"
        except:
            return "N/A"

    def sort_match_list(self, column):
        """排序智能匹配固件列表"""
        # 切换排序方向
        if self.match_sort_column == column:
            self.match_sort_reverse = not self.match_sort_reverse
        else:
            self.match_sort_column = column
            self.match_sort_reverse = False

        # 更新列标题显示排序方向
        for col in ('filename', 'size', 'path'):
            if col == column:
                direction = '▼' if self.match_sort_reverse else '▲'
                text = {'filename': '固件文件名', 'size': '文件大小', 'path': '文件路径'}[col]
                self.match_fw_list.heading(col, text=f"{text} {direction}")
            else:
                text = {'filename': '固件文件名', 'size': '文件大小', 'path': '文件路径'}[col]
                self.match_fw_list.heading(col, text=text)

        # 获取所有项
        items = [(self.match_fw_list.set(item, column), item) for item in self.match_fw_list.get_children('')]

        # 特殊处理文件大小排序（转换为数字）
        if column == 'size':
            def size_key(item):
                size_str = item[0]
                if 'MB' in size_str:
                    return float(size_str.split()[0]) * 1024 * 1024
                elif 'KB' in size_str:
                    return float(size_str.split()[0]) * 1024
                elif 'B' in size_str:
                    try:
                        return float(size_str.split()[0])
                    except:
                        return 0
                return 0
            items.sort(key=size_key, reverse=self.match_sort_reverse)
        else:
            # 按文本排序
            items.sort(reverse=self.match_sort_reverse)

        # 重新排列项
        for index, (val, item) in enumerate(items):
            self.match_fw_list.move(item, '', index)
