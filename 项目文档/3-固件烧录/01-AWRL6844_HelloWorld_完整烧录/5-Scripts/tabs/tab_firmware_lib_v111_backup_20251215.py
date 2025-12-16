#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
tab_firmware_lib.py - 固件库标签页
版本: v1.2.0
作者: Benson@Wisefido

⚠️ 此模块不能单独运行，必须从 flash_tool.py 主入口启动！

功能：
- 项目级别的固件管理
- 扫描完整项目（应用固件 + 配置文件）
- 智能推荐SBL和雷达参数配置
- 支持.appimage和.bin格式
"""

import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox, filedialog
from pathlib import Path
import os
import json
import glob

class FirmwareProject:
    """固件项目数据类"""
    def __init__(self):
        self.name = ""                    # 项目名称
        self.sdk_source = ""              # 来源SDK
        self.project_path = ""            # 项目根目录
        
        # 核心固件文件（必须）
        self.app_firmware = None          # 应用固件路径(.appimage或.bin)
        self.syscfg_file = None           # .syscfg配置文件
        self.rtos_cfg_file = None         # RTOS .cfg配置文件
        
        # SBL推荐列表（可选）
        self.recommended_sbl = []         # [{path, source, priority, reason}, ...]
        self.selected_sbl = None          # 用户选择的SBL
        
        # 雷达参数配置（可选）
        self.recommended_radar_cfg = []   # [{path, source, priority, reason}, ...]
        self.selected_radar_cfg = None    # 用户选择的雷达配置
        
        # 元信息
        self.compatibility = ""           # 兼容性说明
        self.description = ""             # 项目描述

class FirmwareLibTab:
    """固件库标签页类 - v1.2.0 项目级管理"""
    
    def __init__(self, parent_frame, app):
        """
        初始化固件库标签页
        
        Args:
            parent_frame: 父容器（tk.Frame）
            app: 主应用实例（FlashToolGUI）
        """
        self.frame = parent_frame
        self.app = app
        
        # 检查是否是通过主入口启动
        if not hasattr(app, 'VERSION'):
            self._show_error_and_exit()
        
        # SDK路径配置 - 支持多个SDK目录
        self.sdk_paths = [
            r"C:\ti\radar_toolbox_3_30_00_06",
            r"C:\ti\MMWAVE_L_SDK_06_01_00_01",
            r"C:\ti\radar_academy_3_10_00_1",
        ]
        
        # 项目列表和当前选中项目
        self.projects = []                # FirmwareProject对象列表
        self.current_project = None       # 当前选中的项目
        
        # UI变量
        self.sbl_var = None              # SBL选择变量
        self.radar_cfg_var = None        # 雷达配置选择变量
        self.use_default_cfg = None      # 使用默认配置复选框
        
        # 创建界面
        self.create_ui()
        
        # 自动扫描项目
        self.scan_projects()
    
    def _show_error_and_exit(self):
        """显示错误并退出"""
        import sys
        print("=" * 70)
        print("⚠️  错误：tab_firmware_lib 模块不能单独运行！")
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
        """创建标签页UI - v1.2.0项目级管理"""
        # 主容器
        main_container = tk.Frame(self.frame, bg="#ecf0f1")
        main_container.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        # 顶部工具栏
        toolbar = tk.Frame(main_container, bg="#ecf0f1")
        toolbar.pack(fill=tk.X, pady=(0, 10))
        
        tk.Label(
            toolbar,
            text="📚 固件库 - 项目级管理 (v1.2.0)",
            font=("Microsoft YaHei UI", 14, "bold"),
            bg="#ecf0f1",
            fg="#2c3e50"
        ).pack(side=tk.LEFT)
        
        tk.Button(
            toolbar,
            text="🔄 重新扫描",
            font=("Microsoft YaHei UI", 9),
            command=self.scan_projects,
            bg="#3498db",
            fg="white",
            relief=tk.FLAT,
            padx=15,
            pady=5,
            cursor="hand2"
        ).pack(side=tk.RIGHT, padx=5)
        
        tk.Button(
            toolbar,
            text="📂 打开SDK目录",
            font=("Microsoft YaHei UI", 9),
            command=self.open_sdk_folder,
            bg="#16a085",
            fg="white",
            relief=tk.FLAT,
            padx=15,
            pady=5,
            cursor="hand2"
        ).pack(side=tk.RIGHT, padx=5)
        
        # 中间内容区域 - 左右分栏 (30% / 70%)
        content_container = tk.Frame(main_container, bg="#ecf0f1")
        content_container.pack(fill=tk.BOTH, expand=True)
        
        # 左侧：项目列表 (30%)
        left_frame = tk.Frame(content_container, bg="#ecf0f1", width=300)
        left_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=False, padx=(0, 10))
        left_frame.pack_propagate(False)
        
        tk.Label(
            left_frame,
            text="📁 固件项目列表",
            font=("Microsoft YaHei UI", 11, "bold"),
            bg="#ecf0f1",
            fg="#2c3e50"
        ).pack(pady=(0, 5))
        
        # 项目列表框
        list_frame = tk.Frame(left_frame, bg="white", relief=tk.GROOVE, bd=2)
        list_frame.pack(fill=tk.BOTH, expand=True)
        
        # Listbox + Scrollbar
        scrollbar = tk.Scrollbar(list_frame)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        self.project_listbox = tk.Listbox(
            list_frame,
            font=("Microsoft YaHei UI", 10),
            yscrollcommand=scrollbar.set,
            selectmode=tk.SINGLE,
            bg="white",
            fg="#2c3e50",
            selectbackground="#3498db",
            selectforeground="white"
        )
        self.project_listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.config(command=self.project_listbox.yview)
        
        # 绑定选择事件
        self.project_listbox.bind('<<ListboxSelect>>', self.on_project_select)
        
        # 右侧：项目详细信息 (70%)
        right_frame = tk.Frame(content_container, bg="#ecf0f1")
        right_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)
        
        tk.Label(
            right_frame,
            text="📋 项目详细信息",
            font=("Microsoft YaHei UI", 11, "bold"),
            bg="#ecf0f1",
            fg="#2c3e50"
        ).pack(pady=(0, 5))
        
        # 详细信息滚动容器
        detail_canvas_frame = tk.Frame(right_frame, bg="white", relief=tk.GROOVE, bd=2)
        detail_canvas_frame.pack(fill=tk.BOTH, expand=True)
        
        # 创建Canvas和Scrollbar
        self.detail_canvas = tk.Canvas(detail_canvas_frame, bg="white", highlightthickness=0)
        detail_scrollbar = tk.Scrollbar(detail_canvas_frame, orient="vertical", command=self.detail_canvas.yview)
        self.detail_frame = tk.Frame(self.detail_canvas, bg="white")
        
        self.detail_frame.bind(
            "<Configure>",
            lambda e: self.detail_canvas.configure(scrollregion=self.detail_canvas.bbox("all"))
        )
        
        self.detail_canvas.create_window((0, 0), window=self.detail_frame, anchor="nw")
        self.detail_canvas.configure(yscrollcommand=detail_scrollbar.set)
        
        detail_scrollbar.pack(side="right", fill="y")
        self.detail_canvas.pack(side="left", fill="both", expand=True)
        
        # 鼠标滚轮支持
        self.detail_canvas.bind_all("<MouseWheel>", self._on_mousewheel)
        
        # 初始提示
        self._show_initial_message()
    
    def _on_mousewheel(self, event):
        """鼠标滚轮事件处理"""
        self.detail_canvas.yview_scroll(int(-1*(event.delta/120)), "units")
    
    def _show_initial_message(self):
        """显示初始提示信息"""
        for widget in self.detail_frame.winfo_children():
            widget.destroy()
        
        msg = tk.Label(
            self.detail_frame,
            text="🔍 点击'重新扫描'按钮开始扫描SDK中的固件项目\n\n或从左侧列表选择一个项目查看详情",
            font=("Microsoft YaHei UI", 11),
            bg="white",
            fg="#7f8c8d",
            justify="center",
            pady=50
        )
        msg.pack(expand=True)
            padx=15,
            pady=8,
            cursor="hand2"
        ).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 5))
        
        tk.Button(
            button_frame,
            text="➡️ 加载到基本烧录页面",
            font=("Microsoft YaHei UI", 9),
            command=self.load_to_basic_tab,
            bg="#e67e22",
            fg="white",
            relief=tk.FLAT,
            padx=15,
            pady=8,
            cursor="hand2"
        ).pack(side=tk.RIGHT, fill=tk.X, expand=True, padx=(5, 0))
    
    def scan_firmwares(self):
        """扫描SDK目录下的demo固件"""
        self.firmware_list = []
        self.firmware_listbox.delete(0, tk.END)
        self.detail_text.delete(1.0, tk.END)
        
        self.detail_text.insert(tk.END, "🔍 正在扫描固件...\n\n", "title")
        self.detail_text.insert(tk.END, "⚠️ 仅搜索AWRL6844兼容固件\n\n", "warning")
        self.detail_text.update()
        
        # 扫描所有配置的SDK路径
        scanned_paths = []
        for sdk_path in self.sdk_paths:
            if not os.path.exists(sdk_path):
                continue
            
            scanned_paths.append(sdk_path)
            self.detail_text.insert(tk.END, f"📂 扫描: {os.path.basename(sdk_path)}...\n", "path")
            self.detail_text.update()
            
            # 扫描关键子目录
            search_dirs = [
                os.path.join(sdk_path, 'source', 'ti', 'examples'),
                os.path.join(sdk_path, 'examples'),
                os.path.join(sdk_path, 'tools'),
                os.path.join(sdk_path, 'applications'),
            ]
            
            for search_dir in search_dirs:
                if os.path.exists(search_dir):
                    self._scan_directory(search_dir, os.path.basename(sdk_path))
        
        # 按类型分组排序
        sbl_list = [f for f in self.firmware_list if f['file_type'] == 'SBL引导']
        app_list = [f for f in self.firmware_list if f['file_type'] == '应用固件']
        
        # 更新列表
        if self.firmware_list:
            # 先显示SBL
            if sbl_list:
                self.firmware_listbox.insert(tk.END, "═══ SBL引导固件 ═══")
                for firmware in sbl_list:
                    display_name = f"  🔧 {firmware['name']} [{firmware['source']}]"
                    self.firmware_listbox.insert(tk.END, display_name)
            
            # 再显示应用固件
            if app_list:
                self.firmware_listbox.insert(tk.END, "")
                self.firmware_listbox.insert(tk.END, "═══ 应用固件 ═══")
                for firmware in app_list:
                    display_name = f"  📦 {firmware['name']} [{firmware['source']}]"
                    self.firmware_listbox.insert(tk.END, display_name)
            
            self.detail_text.delete(1.0, tk.END)
            self.detail_text.insert(tk.END, f"✅ 找到 {len(self.firmware_list)} 个AWRL6844兼容固件\n\n", "success")
            self.detail_text.insert(tk.END, "扫描路径:\n", "subtitle")
            for path in scanned_paths:
                self.detail_text.insert(tk.END, f"  • {path}\n", "path")
            self.detail_text.insert(tk.END, "\n请在左侧列表中选择一个固件查看详细信息", "subtitle")
        else:
            self.detail_text.delete(1.0, tk.END)
            self.detail_text.insert(tk.END, "⚠️ 未找到AWRL6844兼容固件\n\n", "warning")
            self.detail_text.insert(tk.END, "扫描路径:\n", "subtitle")
            for path in scanned_paths:
                self.detail_text.insert(tk.END, f"  • {path}\n", "path")
            if not scanned_paths:
                self.detail_text.insert(tk.END, "\n❌ 所有SDK路径都不存在！\n", "warning")
    
    def _scan_directory(self, directory, source_name):
        """递归扫描目录"""
        try:
            for root, dirs, files in os.walk(directory):
                # 查找.appimage或.bin文件
                for file in files:
                    if file.endswith(('.appimage', '.bin')) and self._is_compatible_firmware(file):
                        firmware_path = os.path.join(root, file)
                        
                        # 查找对应的配置文件
                        sysconfig_file = self._find_config_file(root, '.syscfg')
                        cfg_file = self._find_config_file(root, '.cfg')
                        
                        # 提取项目信息
                        project_name = self._extract_project_name(root)
                        file_type = self._classify_firmware_type(file)
                        
                        firmware_info = {
                            'name': project_name,
                            'source': source_name,
                            'firmware_path': firmware_path,
                            'firmware_file': file,
                            'file_type': file_type,
                            'sysconfig_path': sysconfig_file,
                            'cfg_path': cfg_file,
                            'root_dir': root,
                            'description': self._get_description(project_name)
                        }
                        
                        self.firmware_list.append(firmware_info)
        except Exception as e:
            print(f"扫描目录错误: {e}")
    
    def _is_compatible_firmware(self, filename):
        """检查固件是否兼容AWRL6844EVM"""
        filename_lower = filename.lower()
        # 支持的命名模式
        compatible_patterns = [
            '6844',      # 明确的6844标识
            'l6844',     # L6844变体
            '68xx',      # xWR68xx系列
            'xwrl68',    # xWRL68xx系列
            'awrl68',    # AWRL68xx系列
        ]
        return any(pattern in filename_lower for pattern in compatible_patterns)
    
    def _find_config_file(self, directory, extension):
        """查找配置文件"""
        for file in os.listdir(directory):
            if file.endswith(extension):
                return os.path.join(directory, file)
        
        # 向上一级查找
        parent_dir = os.path.dirname(directory)
        if os.path.exists(parent_dir):
            for file in os.listdir(parent_dir):
                if file.endswith(extension):
                    return os.path.join(parent_dir, file)
        
        return None
    
    def _classify_firmware_type(self, filename):
        """分类固件类型"""
        filename_lower = filename.lower()
        if 'sbl' in filename_lower:
            return 'SBL引导'
        elif filename_lower.endswith('.syscfg'):
            return 'SysConfig配置'
        elif filename_lower.endswith('.cfg'):
            return 'RTOS配置'
        else:
            return '应用固件'
    
    def _extract_project_name(self, path):
        """从路径中提取项目名称"""
        parts = path.replace('\\', '/').split('/')
        # 查找有意义的目录名
        for i in range(len(parts) - 1, -1, -1):
            part = parts[i]
            if part and part not in ['Debug', 'Release', 'build', 'out', 'prebuilt_binaries', 'ti-arm-clang']:
                return part
        return "Unknown"
    
    def _get_description(self, project_name):
        """获取项目描述"""
        descriptions = {
            '3D_people_tracking': '3D人员追踪 - 检测和追踪多个人的3D位置和运动',
            'people_counting': '人员计数 - 统计区域内的人员数量',
            'vital_signs': '生命体征检测 - 心率和呼吸率监测',
            'gesture_recognition': '手势识别 - 识别手部动作和手势',
            'level_sensing': '液位检测 - 监测液体或固体物料的高度',
            'industrial_visualizer': '工业可视化 - 工业场景的目标检测和追踪',
            'automotive': '汽车应用 - 车内乘员检测和监测',
            'outdoor_false_detection': '户外虚警检测 - 降低户外场景的误检',
            'ti_demo': 'TI官方Demo - 基础功能演示',
            'hello_world': 'Hello World - 基础启动示例',
        }
        
        name_lower = project_name.lower()
        for key, desc in descriptions.items():
            if key in name_lower:
                return desc
        
        return '未知Demo - 请查看SDK文档了解详情'
    
    def on_firmware_select(self, event):
        """固件选择事件"""
        selection = self.firmware_listbox.curselection()
        if not selection:
            return
        
        selected_text = self.firmware_listbox.get(selection[0])
        # 跳过分隔符
        if '═══' in selected_text or not selected_text.strip():
            return
        
        # 查找对应的固件（通过名称匹配）
        firmware = None
        for fw in self.firmware_list:
            if fw['name'] in selected_text:
                firmware = fw
                break
        
        if not firmware:
            return
        
        # 清空详细信息
        self.detail_text.delete(1.0, tk.END)
        
        # 显示固件信息
        self.detail_text.insert(tk.END, f"📦 {firmware['name']}\n", "title")
        self.detail_text.insert(tk.END, "=" * 60 + "\n\n", "normal")
        
        # 描述
        self.detail_text.insert(tk.END, "📝 功能说明\n", "subtitle")
        self.detail_text.insert(tk.END, f"{firmware['description']}\n", "normal")
        
        self.detail_text.insert(tk.END, "\n" + "="*60 + "\n\n", "normal")
        
        # 来源SDK
        self.detail_text.insert(tk.END, "📚 来源SDK\n", "subtitle")
        self.detail_text.insert(tk.END, f"{firmware['source']}\n\n", "normal")
        
        # 文件类型
        self.detail_text.insert(tk.END, "🏷️ 文件类型\n", "subtitle")
        type_icon = "🔧" if firmware['file_type'] == 'SBL引导' else "📦"
        self.detail_text.insert(tk.END, f"{type_icon} {firmware['file_type']}\n\n", "normal")
        
        # 固件文件
        self.detail_text.insert(tk.END, "📦 固件文件\n", "subtitle")
        self.detail_text.insert(tk.END, f"文件名: {firmware['firmware_file']}\n", "normal")
        self.detail_text.insert(tk.END, f"路径: {firmware['firmware_path']}\n\n", "path")
        
        # 兼容性检查 - 更严格的提示
        self.detail_text.insert(tk.END, "🎯 AWRL6844EVM 兼容性\n", "subtitle")
        filename_lower = firmware['firmware_file'].lower()
        if '6844' in filename_lower or 'l6844' in filename_lower:
            self.detail_text.insert(tk.END, "✅ 专为 AWRL6844 设计\n", "success")
            self.detail_text.insert(tk.END, "此固件可直接用于 AWRL6844EVM 开发板\n\n", "normal")
        elif '68xx' in filename_lower or 'xwrl68' in filename_lower or 'awrl68' in filename_lower:
            self.detail_text.insert(tk.END, "✅ 兼容 xWR68xx 系列\n", "success")
            self.detail_text.insert(tk.END, "此固件兼容AWRL6844EVM（属于68xx系列）\n\n", "normal")
        else:
            self.detail_text.insert(tk.END, "⚠️ 警告：无法确认兼容性\n", "warning")
            self.detail_text.insert(tk.END, "请确认此固件是否适用于 AWRL6844EVM\n\n", "warning")
        
        # 配置文件标题
        self.detail_text.insert(tk.END, "="*60 + "\n", "normal")
        self.detail_text.insert(tk.END, "📄 配置文件信息\n", "title")
        self.detail_text.insert(tk.END, "="*60 + "\n\n", "normal")
        
        # SysConfig配置文件
        self.detail_text.insert(tk.END, "⚙️ SysConfig配置文件 (.syscfg)\n", "subtitle")
        if firmware['sysconfig_path']:
            self.detail_text.insert(tk.END, "✅ 已找到\n", "success")
            self.detail_text.insert(tk.END, "用途: CCS SysConfig图形化配置工具\n", "normal")
            self.detail_text.insert(tk.END, "配置内容: 外设初始化、引脚映射、时钟配置\n", "normal")
            self.detail_text.insert(tk.END, f"绝对路径: {firmware['sysconfig_path']}\n\n", "path")
        else:
            self.detail_text.insert(tk.END, "❌ 未找到\n", "warning")
            self.detail_text.insert(tk.END, "说明: 此demo可能不使用SysConfig配置\n\n", "normal")
        
        # RTOS配置文件
        self.detail_text.insert(tk.END, "⚙️ RTOS配置文件 (.cfg)\n", "subtitle")
        if firmware['cfg_path']:
            self.detail_text.insert(tk.END, "✅ 已找到\n", "success")
            self.detail_text.insert(tk.END, "用途: TI-RTOS系统配置文件\n", "normal")
            self.detail_text.insert(tk.END, "配置内容: 任务优先级、内存分配、中断处理、堆栈大小\n", "normal")
            self.detail_text.insert(tk.END, f"绝对路径: {firmware['cfg_path']}\n\n", "path")
        else:
            self.detail_text.insert(tk.END, "❌ 未找到\n", "warning")
            self.detail_text.insert(tk.END, "说明: 此demo可能使用默认RTOS配置\n\n", "normal")
        
        # 项目目录
        self.detail_text.insert(tk.END, "="*60 + "\n", "normal")
        self.detail_text.insert(tk.END, "📁 项目根目录\n", "subtitle")
        self.detail_text.insert(tk.END, f"{firmware['root_dir']}\n", "path")
        self.detail_text.insert(tk.END, "\n")
        
        # 保存当前选中的固件
        self.detail_text.insert(tk.END, "💡 使用说明\n", "subtitle")
        self.detail_text.insert(tk.END, "1. 点击'复制固件路径'将路径复制到剪贴板\n")
        self.detail_text.insert(tk.END, "2. 或点击'加载到基本烧录页面'自动填充固件路径\n")
        self.detail_text.insert(tk.END, "3. 切换到'基本烧录'标签页进行固件烧录\n")
    
    def copy_firmware_path(self):
        """复制固件路径到剪贴板"""
        selection = self.firmware_listbox.curselection()
        if not selection:
            messagebox.showwarning("提示", "请先选择一个固件")
            return
        
        index = selection[0]
        firmware = self.firmware_list[index]
        
        # 复制到剪贴板
        self.frame.clipboard_clear()
        self.frame.clipboard_append(firmware['firmware_path'])
        
        messagebox.showinfo("成功", f"固件路径已复制到剪贴板：\n\n{firmware['firmware_path']}")
    
    def load_to_basic_tab(self):
        """加载固件到基本烧录页面"""
        selection = self.firmware_listbox.curselection()
        if not selection:
            messagebox.showwarning("提示", "请先选择一个固件")
            return
        
        index = selection[0]
        firmware = self.firmware_list[index]
        
        # 设置固件路径
        self.app.firmware_file.set(firmware['firmware_path'])
        
        # 切换到基本烧录页面
        self.app.notebook.select(0)
        
        # 在日志中显示
        if hasattr(self.app, 'basic_tab') and hasattr(self.app.basic_tab, 'log'):
            self.app.basic_tab.log(f"\n✅ 已加载固件: {firmware['name']}\n", "SUCCESS")
            self.app.basic_tab.log(f"📁 路径: {firmware['firmware_path']}\n")
            self.app.basic_tab.log(f"📝 说明: {firmware['description']}\n")
            
            # 检查匹配性
            if '6844' in firmware['firmware_file'].lower():
                self.app.basic_tab.log("✅ 固件匹配 AWRL6844EVM\n\n", "SUCCESS")
            else:
                self.app.basic_tab.log("⚠️ 警告: 固件可能不匹配 AWRL6844EVM\n\n", "WARN")
        
        messagebox.showinfo("成功", f"固件已加载到基本烧录页面：\n\n{firmware['name']}")
    
    def open_sdk_folder(self):
        """打开第一个存在的SDK目录"""
        for sdk_path in self.sdk_paths:
            if os.path.exists(sdk_path):
                os.startfile(sdk_path)
                return
        messagebox.showerror("错误", "所有SDK路径都不存在！")


# 如果直接运行此文件，显示错误提示
if __name__ == "__main__":
    import sys
    print("=" * 70)
    print("⚠️  错误：tab_firmware_lib.py 不能单独运行！")
    print("=" * 70)
    print()
    print("请从主入口启动烧录工具：")
    print()
    print("  cd 5-Scripts")
    print("  python flash_tool.py")
    print()
    print("=" * 70)
    sys.exit(1)
