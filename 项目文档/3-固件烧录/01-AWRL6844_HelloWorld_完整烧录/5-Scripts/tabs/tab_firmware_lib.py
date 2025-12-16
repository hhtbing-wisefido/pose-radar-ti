#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
tab_firmware_lib.py - 固件库标签页模块 (极简版)

⚠️ 此模块不能单独运行，必须从 flash_tool.py 主入口启动！
"""

import tkinter as tk
from tkinter import ttk, messagebox, filedialog, scrolledtext
from pathlib import Path
import os
import json
import glob

class FirmwareProject:
    """固件项目数据类（v1.2.0 - 以固件为核心）"""
    def __init__(self):
        self.name = ""                    # 项目名称（从固件文件名提取）
        self.sdk_source = ""              # 来源SDK（如果可识别）
        self.project_path = ""            # 固件所在目录
        
        # 核心文件（必须）
        self.app_firmware = None          # 应用固件路径(.appimage)【必须】
        
        # 配置文件（可选，如果同目录存在则关联）
        self.syscfg_file = None           # .syscfg配置文件【可选】
        self.rtos_cfg_file = None         # RTOS .cfg配置文件【可选】
        
        # SBL推荐列表（可选，可能在其他位置）
        self.recommended_sbl = []         # [{path, source, priority, reason}, ...]
        self.selected_sbl = None          # 用户选择的SBL
        
        # 雷达参数配置（可选，运行时使用）
        self.recommended_radar_cfg = []   # [{path, source, priority, reason}, ...]
        self.selected_radar_cfg = None    # 用户选择的雷达配置
        
        # 元信息
        self.compatibility = ""           # 兼容性说明
        self.compatibility_reason = ""    # 兼容性判断原因
        self.description = ""             # 项目描述

class FirmwareLibTab:
    """固件库标签页类 - 项目级管理"""
    
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
        
        # 扫描路径配置 - 从配置文件加载
        self.config_file = os.path.join(os.path.dirname(__file__), '..', 'config', 'scan_paths.json')
        self.sdk_paths = self._load_scan_paths()
        
        # 项目列表和当前选中项目
        self.projects = []                # FirmwareProject对象列表
        self.current_project = None       # 当前选中的项目
        
        # UI变量
        self.sbl_var = tk.StringVar()              # SBL选择变量
        self.radar_cfg_var = tk.StringVar()        # 雷达配置选择变量
        self.use_default_cfg = tk.BooleanVar(value=True)  # 使用默认配置复选框
        
        # 创建界面
        self.create_ui()
        
        # 自动扫描项目
        self.scan_projects()
    
    def _load_scan_paths(self):
        """从配置文件加载扫描路径"""
        default_paths = [
            r"C:\ti\radar_toolbox_3_30_00_06",
            r"C:\ti\MMWAVE_L_SDK_06_01_00_01",
            r"C:\ti\radar_academy_3_10_00_1",
        ]
        
        try:
            if os.path.exists(self.config_file):
                with open(self.config_file, 'r', encoding='utf-8') as f:
                    data = json.load(f)
                    return data.get('scan_paths', default_paths)
        except Exception as e:
            print(f"加载配置失败: {e}")
        
        return default_paths
    
    def _save_scan_paths(self):
        """保存扫描路径到配置文件"""
        try:
            os.makedirs(os.path.dirname(self.config_file), exist_ok=True)
            with open(self.config_file, 'w', encoding='utf-8') as f:
                json.dump({'scan_paths': self.sdk_paths}, f, ensure_ascii=False, indent=2)
        except Exception as e:
            print(f"保存配置失败: {e}")
    
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
        """创建标签页UI - 项目级管理"""
        # 主容器
        main_container = tk.Frame(self.frame, bg="#ecf0f1")
        main_container.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        # 顶部工具栏
        toolbar = tk.Frame(main_container, bg="#ecf0f1")
        toolbar.pack(fill=tk.X, pady=(0, 10))
        
        tk.Label(
            toolbar,
            text="📚 固件库 - 项目级管理",
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
            text="📂 管理扫描路径",
            font=("Microsoft YaHei UI", 9),
            command=self.manage_scan_paths,
            bg="#16a085",
            fg="white",
            relief=tk.FLAT,
            padx=15,
            pady=5,
            cursor="hand2"
        ).pack(side=tk.RIGHT, padx=5)
        
        # 中间内容区域 - 使用PanedWindow实现50/50可调整分割
        paned_window = tk.PanedWindow(
            main_container, 
            orient=tk.HORIZONTAL,
            sashwidth=6,
            sashrelief=tk.RAISED,
            bg="#bdc3c7",
            showhandle=True
        )
        paned_window.pack(fill=tk.BOTH, expand=True)
        
        # 左侧：项目列表 (50%)
        left_frame = tk.Frame(paned_window, bg="#ecf0f1")
        paned_window.add(left_frame, width=600)
        
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
        
        # 右侧：项目详细信息 (50%)
        right_frame = tk.Frame(paned_window, bg="#ecf0f1")
        paned_window.add(right_frame, width=600)
        
        tk.Label(
            right_frame,
            text="📋 项目详细信息",
            font=("Microsoft YaHei UI", 11, "bold"),
            bg="#ecf0f1",
            fg="#2c3e50"
        ).pack(pady=(0, 5))
        
        # 详细信息容器（直接使用Frame，不用Canvas滚动）
        self.detail_frame = tk.Frame(right_frame, bg="white", relief=tk.GROOVE, bd=2)
        self.detail_frame.pack(fill=tk.BOTH, expand=True)
        
        # 初始提示
        self._show_initial_message()
    
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
    
    def scan_projects(self):
        """扫描SDK目录下的固件项目 - v1.2.0核心功能（以.appimage为标志）"""
        self.projects = []
        self.project_listbox.delete(0, tk.END)
        self._show_initial_message()
        
        # 显示扫描进度
        progress_label = tk.Label(
            self.detail_frame,
            text="🔍 正在扫描SDK目录...\n\n⚙️ 搜索所有.appimage固件文件",
            font=("Microsoft YaHei UI", 11),
            bg="white",
            fg="#3498db",
            justify="center"
        )
        progress_label.pack(expand=True, pady=50)
        self.detail_frame.update()
        
        # 扫描所有配置的SDK路径
        scanned_count = 0
        for sdk_path in self.sdk_paths:
            if not os.path.exists(sdk_path):
                continue
            
            progress_label.config(text=f"🔍 正在扫描: {os.path.basename(sdk_path)}...")
            self.detail_frame.update()
            
            # 扫描关键子目录
            search_dirs = [
                os.path.join(sdk_path, 'source', 'ti', 'examples'),
                os.path.join(sdk_path, 'examples'),
                os.path.join(sdk_path, 'tools'),
                os.path.join(sdk_path, 'applications'),
                os.path.join(sdk_path, 'prebuilt_binaries'),
            ]
            
            for search_dir in search_dirs:
                if os.path.exists(search_dir):
                    count = self._scan_directory_for_firmwares(search_dir, os.path.basename(sdk_path))
                    scanned_count += count
        
        # 注释：固件分组逻辑已在_scan_directory_for_firmwares中完成，不再需要单独分组
        
        # 更新UI
        self._update_project_list()
        
        # 显示结果
        if self.projects:
            result_msg = f"✅ 找到 {len(self.projects)} 个AWRL6844兼容固件\n\n请从左侧列表选择一个固件查看详情"
        else:
            result_msg = "⚠️ 未找到AWRL6844兼容固件\n\n请检查SDK路径是否正确"
        
        for widget in self.detail_frame.winfo_children():
            widget.destroy()
        
        tk.Label(
            self.detail_frame,
            text=result_msg,
            font=("Microsoft YaHei UI", 11),
            bg="white",
            fg="#27ae60" if self.projects else "#e74c3c",
            justify="center",
            pady=50
        ).pack(expand=True)
    
    def _scan_directory_for_firmwares(self, directory, source_name):
        """递归扫描目录查找固件文件（以.appimage为标志）
        
        改进：先按项目分组固件，每个项目只创建一个FirmwareProject对象
        """
        count = 0
        try:
            # 第一阶段：收集所有固件并按项目分组
            firmware_groups = {}  # {项目名称: [固件信息列表]}
            
            for root, dirs, files in os.walk(directory):
                # 查找.appimage固件文件
                firmware_files = [f for f in files 
                                if f.endswith('.appimage')
                                and 'sbl' not in f.lower()]  # 排除SBL
                
                # 为每个固件提取项目名称并分组
                for fw_file in firmware_files:
                    project_name = self._extract_project_name_from_firmware(fw_file)
                    fw_info = {
                        'path': os.path.join(root, fw_file),
                        'name': fw_file,
                        'root': root
                    }
                    
                    if project_name not in firmware_groups:
                        firmware_groups[project_name] = []
                    firmware_groups[project_name].append(fw_info)
            
            # 第二阶段：为每个项目创建一个FirmwareProject对象
            for project_name, firmwares in firmware_groups.items():
                project = self._create_project_from_firmware_group(project_name, firmwares, source_name)
                if project and self._is_compatible_project(project):
                    self.projects.append(project)
                    count += 1
                    
        except Exception as e:
            print(f"扫描目录错误 ({directory}): {e}")
        
        return count
    
    def _create_project_from_firmware_group(self, project_name, firmwares, source_name):
        """从固件组创建项目对象（新方法：支持多固件变体）
        
        Args:
            project_name: 项目名称
            firmwares: 固件信息列表 [{'path': ..., 'name': ..., 'root': ...}, ...]
            source_name: SDK来源名称
        
        Returns:
            FirmwareProject对象
        """
        try:
            project = FirmwareProject()
            project.name = project_name
            project.sdk_source = source_name
            
            # 直接选择第一个固件(简化处理)
            primary_fw = firmwares[0]
            project.app_firmware = primary_fw['path']
            project.project_path = primary_fw['root']
            
            # 查找配置文件（在主固件所在目录）
            root_dir = primary_fw['root']
            files = os.listdir(root_dir)
            
            # 1. 查找.syscfg文件
            for f in files:
                if f.endswith('.syscfg'):
                    project.syscfg_file = os.path.join(root_dir, f)
                    break
            
            # 2. 查找RTOS .cfg文件
            for f in files:
                if f.endswith('.cfg'):
                    cfg_path = os.path.join(root_dir, f)
                    if self._is_rtos_cfg(cfg_path):
                        project.rtos_cfg_file = cfg_path
                        break
                    project.rtos_cfg_file = os.path.join(root_dir, f)
            
            return project
            
        except Exception as e:
            print(f"创建项目失败 ({project_name}): {e}")
            return None
    
    def _extract_project_name_from_firmware(self, firmware_file):
        """从固件文件名提取项目名称（包含父目录信息）"""
        # 移除扩展名
        name = os.path.splitext(firmware_file)[0]
        
        # 移除常见后缀
        suffixes = ['.release', '.debug', '_system', '_mss', '_dss', '_c66x']
        for suffix in suffixes:
            name = name.replace(suffix, '')
        
        # 清理并格式化
        name = name.replace('_', ' ').strip()
        return name.title() if name else firmware_file
    
    def _is_rtos_cfg(self, filepath):
        """判断是否是RTOS .cfg文件（JavaScript语法）"""
        try:
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read(500)  # 只读前500字符
                # RTOS .cfg特征：JavaScript语法（var, xdc.useModule）
                if 'var' in content or 'xdc.useModule' in content or 'xdc.loadPackage' in content:
                    return True
        except:
            pass
        return False
    
    def _is_compatible_project(self, project):
        """检查项目是否兼容AWRL6844EVM - 返回(是否兼容, 判断原因)"""
        if not project.app_firmware:
            return False
        
        # 检查文件名和路径中的兼容性标识
        firmware_name = os.path.basename(project.app_firmware).lower()
        full_path = project.app_firmware.lower()
        
        # 支持的命名模式
        compatible_patterns = {
            '6844': '文件名包含"6844"',
            'l6844': '文件名包含"L6844"',
            '68xx': '文件名包含"68xx"系列标识',
            'xwrl684': '路径包含"xWRL684x"系列标识',
            'awrl684': '路径包含"AWRL684x"系列标识',
        }
        
        # 检查文件名或完整路径
        for pattern, reason in compatible_patterns.items():
            if pattern in firmware_name:
                project.compatibility_reason = f"✅ 兼容 ({reason})"
                return True
            if pattern in full_path:
                project.compatibility_reason = f"✅ 兼容 (项目目录包含{pattern}标识)"
                return True
        
        # 特殊情况：hello_world在radar_toolbox或MMWAVE_L_SDK中默认为6844兼容
        if 'hello_world' in firmware_name:
            if 'radar_toolbox' in full_path or 'mmwave_l_sdk' in full_path:
                # 检查目录中是否有xwrl684x标识
                if 'xwrl684' in full_path or '684' in full_path:
                    project.compatibility_reason = "✅ 兼容 (hello_world项目，目录含684x标识)"
                    return True
                else:
                    project.compatibility_reason = "✅ 兼容 (hello_world项目在官方SDK中)"
                    return True
        
        project.compatibility_reason = "⚠️ 兼容性未知 (文件名和路径均未包含6844标识)"
        return False
    
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
            '3d_people_tracking': '3D人员追踪 - 检测和追踪多个人的3D位置和运动',
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
        
        return 'AWRL6844 固件项目'
    
    def _check_compatibility(self, project):
        """检查兼容性"""
        firmware_name = os.path.basename(project.app_firmware).lower()
        if '6844' in firmware_name or 'l6844' in firmware_name:
            return "✅ 专为 AWRL6844 设计"
        elif '68xx' in firmware_name or 'xwrl68' in firmware_name or 'awrl68' in firmware_name:
            return "✅ 兼容 xWR68xx 系列"
        else:
            return "⚠️ 兼容性未知"
    
    def _update_project_list(self):
        """更新项目列表UI"""
        self.project_listbox.delete(0, tk.END)
        
        for project in self.projects:
            # 提取关键目录信息以区分相同名称的固件
            path_parts = project.project_path.split(os.sep)
            # 获取最后2级目录作为区分标识
            context = ""
            if len(path_parts) >= 2:
                context = f"{path_parts[-2]}/{path_parts[-1]}"
            elif len(path_parts) >= 1:
                context = path_parts[-1]
            
            display_name = f"📁 {project.name} [{context}]"
            self.project_listbox.insert(tk.END, display_name)
    
    def on_project_select(self, event):
        """项目选择事件"""
        selection = self.project_listbox.curselection()
        if not selection:
            return
        
        index = selection[0]
        self.current_project = self.projects[index]
        
        # 显示项目详情
        self.show_project_details()
    
    def show_project_details(self):
        """显示项目详细信息 - 使用标签页组织"""
        if not self.current_project:
            return
        
        # 清空详情区域
        for widget in self.detail_frame.winfo_children():
            widget.destroy()
        
        project = self.current_project
        
        # ===== 项目标题区域（紧凑布局）=====
        header_frame = tk.Frame(self.detail_frame, bg="white")
        header_frame.pack(fill=tk.X, padx=10, pady=(5, 3))
        
        tk.Label(
            header_frame,
            text=f"📦 {project.name}",
            font=("Microsoft YaHei UI", 11, "bold"),
            bg="white",
            fg="#2c3e50"
        ).pack(anchor=tk.W)
        
        tk.Label(
            header_frame,
            text=project.description,
            font=("Microsoft YaHei UI", 9),
            bg="white",
            fg="#7f8c8d"
        ).pack(anchor=tk.W, pady=(2, 0))
        
        # 来源和兼容性
        meta_frame = tk.Frame(header_frame, bg="white")
        meta_frame.pack(fill=tk.X, pady=(3, 0))
        
        tk.Label(
            meta_frame,
            text=f"📚 SDK: {project.sdk_source}",
            font=("Microsoft YaHei UI", 8),
            bg="white",
            fg="#34495e"
        ).pack(anchor=tk.W)
        
        compat_color = "#27ae60" if "✅" in project.compatibility_reason else "#95a5a6"
        tk.Label(
            meta_frame,
            text=project.compatibility_reason,
            font=("Microsoft YaHei UI", 8),
            bg="white",
            fg=compat_color
        ).pack(anchor=tk.W, pady=(2, 0))
        
        # 分隔线
        tk.Frame(self.detail_frame, height=1, bg="#ecf0f1").pack(fill=tk.X, padx=10, pady=(3, 0))
        
        # ===== 创建标签页控件（完全填充）=====
        tab_control = ttk.Notebook(self.detail_frame)
        tab_control.pack(fill=tk.BOTH, expand=True, padx=0, pady=0)
        
        # 创建各个标签页
        self._create_files_tab(tab_control, project)
    
    def _add_file_row(self, parent, label, filepath, required=True, hint=""):
        """添加文件行 - 显示完整路径"""
        row = tk.Frame(parent, bg="white")
        row.pack(fill=tk.X, padx=10, pady=8)
        
        # 标签
        label_text = f"{label} {'(必须)' if required else '(可选)'}"
        tk.Label(
            row,
            text=label_text,
            font=("Microsoft YaHei UI", 9, "bold"),
            bg="white",
            fg="#34495e" if required else "#7f8c8d",
            anchor=tk.W
        ).pack(anchor=tk.W)
        
        # 文件名
        if filepath and os.path.exists(str(filepath)):
            filename = os.path.basename(filepath)
            tk.Label(
                row,
                text=f"  📄 {filename}",
                font=("Microsoft YaHei UI", 9),
                bg="white",
                fg="#2c3e50"
            ).pack(anchor=tk.W, padx=10)
            
            # 完整路径（使用Text控件自动换行）
            path_text = tk.Text(
                row,
                font=("Consolas", 8),
                bg="#f8f9fa",
                fg="#495057",
                relief=tk.FLAT,
                height=3,
                wrap=tk.WORD,
                cursor="xterm"
            )
            path_text.insert(1.0, filepath)
            path_text.config(state='disabled')
            path_text.pack(fill=tk.BOTH, expand=True, padx=10, pady=(2, 0))
        else:
            # 未找到时的提示
            not_found_frame = tk.Frame(row, bg="white")
            not_found_frame.pack(anchor=tk.W, padx=10)
            
            icon = "⚠️" if required else "ℹ️"
            color = "#e74c3c" if required else "#95a5a6"
            
            tk.Label(
                not_found_frame,
                text=f"  {icon} 未找到",
                font=("Microsoft YaHei UI", 9),
                bg="white",
                fg=color
            ).pack(side=tk.LEFT)
            
            # 显示提示信息
            if hint:
                tk.Label(
                    not_found_frame,
                    text=f" - {hint}",
                    font=("Microsoft YaHei UI", 8),
                    bg="white",
                    fg="#7f8c8d"
                ).pack(side=tk.LEFT)
    
    def load_to_basic_tab(self):
        """加载固件到基本烧录页面"""
        if not self.current_project:
            messagebox.showwarning("提示", "请先选择一个项目")
            return
        
        # 设置应用固件路径
        self.app.firmware_file.set(self.current_project.app_firmware)
        
        # 切换到基本烧录页面
        self.app.notebook.select(0)
        
        # 在日志中显示
        if hasattr(self.app, 'basic_tab') and hasattr(self.app.basic_tab, 'log'):
            self.app.basic_tab.log(f"\n✅ 已加载项目: {self.current_project.name}\n", "SUCCESS")
            self.app.basic_tab.log(f"📁 应用固件: {self.current_project.app_firmware}\n")
            
            if self.current_project.selected_sbl:
                self.app.basic_tab.log(f"🔧 SBL: {self.current_project.selected_sbl}\n")
            
            if self.current_project.selected_radar_cfg and not self.use_default_cfg.get():
                self.app.basic_tab.log(f"📡 雷达配置: {self.current_project.selected_radar_cfg}\n")
            
            self.app.basic_tab.log(f"{self.current_project.compatibility}\n\n")
        
        messagebox.showinfo("成功", f"项目已加载到基本烧录页面：\n\n{self.current_project.name}")
    
    def manage_scan_paths(self):
        """管理扫描路径 - 添加/删除自定义固件目录"""
        # 创建对话框
        dialog = tk.Toplevel(self.frame)
        dialog.title("管理扫描路径")
        dialog.geometry("700x500")
        dialog.transient(self.frame)
        dialog.grab_set()
        
        # 居中对话框到主窗口
        dialog.update_idletasks()
        x = self.frame.winfo_rootx() + (self.frame.winfo_width() - 700) // 2
        y = self.frame.winfo_rooty() + (self.frame.winfo_height() - 500) // 2
        dialog.geometry(f"700x500+{x}+{y}")
        
        tk.Label(
            dialog,
            text="📂 固件扫描路径管理",
            font=("Microsoft YaHei UI", 12, "bold"),
            fg="#2c3e50"
        ).pack(pady=10)
        
        tk.Label(
            dialog,
            text="工具将在以下目录中搜索固件文件（.appimage/.bin）",
            font=("Microsoft YaHei UI", 9),
            fg="#7f8c8d"
        ).pack()
        
        # 路径列表
        list_frame = tk.Frame(dialog)
        list_frame.pack(fill=tk.BOTH, expand=True, padx=20, pady=10)
        
        scrollbar = tk.Scrollbar(list_frame)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        path_listbox = tk.Listbox(
            list_frame,
            font=("Consolas", 9),
            yscrollcommand=scrollbar.set,
            selectmode=tk.SINGLE
        )
        path_listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.config(command=path_listbox.yview)
        
        # 填充现有路径
        for path in self.sdk_paths:
            status = "✅" if os.path.exists(path) else "❌"
            path_listbox.insert(tk.END, f"{status} {path}")
        
        # 按钮区
        btn_frame = tk.Frame(dialog)
        btn_frame.pack(pady=10)
        
        def add_path():
            new_path = filedialog.askdirectory(title="选择固件目录")
            if new_path and new_path not in self.sdk_paths:
                self.sdk_paths.append(new_path)
                path_listbox.insert(tk.END, f"✅ {new_path}")
        
        def remove_path():
            selection = path_listbox.curselection()
            if selection:
                idx = selection[0]
                if idx < len(self.sdk_paths):
                    del self.sdk_paths[idx]
                    path_listbox.delete(idx)
        
        def save_and_close():
            self._save_scan_paths()
            messagebox.showinfo("成功", f"已保存 {len(self.sdk_paths)} 个扫描路径")
            dialog.destroy()
        
        tk.Button(
            btn_frame,
            text="➕ 添加目录",
            command=add_path,
            bg="#27ae60",
            fg="white",
            padx=20,
            pady=5
        ).pack(side=tk.LEFT, padx=5)
        
        tk.Button(
            btn_frame,
            text="➖ 删除选中",
            command=remove_path,
            bg="#e74c3c",
            fg="white",
            padx=20,
            pady=5
        ).pack(side=tk.LEFT, padx=5)
        
        tk.Button(
            btn_frame,
            text="💾 保存并关闭",
            command=save_and_close,
            bg="#3498db",
            fg="white",
            padx=20,
            pady=5
        ).pack(side=tk.LEFT, padx=5)
    
    def _create_files_tab(self, tab_control, project):
        """创建文件路径标签页"""
        files_tab = tk.Frame(tab_control, bg="white")
        tab_control.add(files_tab, text="📁 文件路径")
        
        # 创建滚动区域
        canvas = tk.Canvas(files_tab, bg="white", highlightthickness=0)
        scrollbar = ttk.Scrollbar(files_tab, orient="vertical", command=canvas.yview)
        scrollable_frame = tk.Frame(canvas, bg="white")
        
        scrollable_frame.bind(
            "<Configure>",
            lambda e: canvas.configure(scrollregion=canvas.bbox("all"))
        )
        
        canvas_window = canvas.create_window((0, 0), window=scrollable_frame, anchor="nw")
        canvas.configure(yscrollcommand=scrollbar.set)
        
        # 让内容随Canvas宽度自适应
        def on_canvas_configure(event):
            canvas.itemconfig(canvas_window, width=event.width)
        canvas.bind("<Configure>", on_canvas_configure)
        
        canvas.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")
        
        # 应用固件
        self._add_detail_path_row(scrollable_frame, "📦 应用固件", project.app_firmware, required=True)
    
    def _add_detail_path_row(self, parent, label, filepath, required=True):
        """添加详细路径行"""
        frame = tk.LabelFrame(
            parent,
            text=label,
            font=("Microsoft YaHei UI", 10, "bold"),
            bg="white",
            fg="#2c3e50",
            bd=2,
            relief=tk.GROOVE
        )
        frame.pack(fill=tk.X, padx=10, pady=10)
        
        if filepath and os.path.exists(filepath):
            # 文件信息
            file_size = os.path.getsize(filepath)
            size_kb = file_size / 1024
            
            info_frame = tk.Frame(frame, bg="white")
            info_frame.pack(fill=tk.X, padx=10, pady=5)
            
            tk.Label(
                info_frame,
                text=f"文件名: {os.path.basename(filepath)}",
                font=("Microsoft YaHei UI", 9),
                bg="white",
                fg="#34495e"
            ).pack(anchor=tk.W)
            
            tk.Label(
                info_frame,
                text=f"大小: {size_kb:.2f} KB ({file_size:,} 字节)",
                font=("Microsoft YaHei UI", 9),
                bg="white",
                fg="#7f8c8d"
            ).pack(anchor=tk.W, pady=(2, 0))
            
            # 完整路径
            tk.Label(
                frame,
                text="完整路径:",
                font=("Microsoft YaHei UI", 9, "bold"),
                bg="white",
                fg="#34495e"
            ).pack(anchor=tk.W, padx=10, pady=(5, 2))
            
            path_text = tk.Text(
                frame,
                font=("Consolas", 8),
                bg="#f8f9fa",
                fg="#495057",
                relief=tk.FLAT,
                height=2,
                wrap=tk.WORD
            )
            path_text.insert(1.0, filepath)
            path_text.config(state='disabled')
            path_text.pack(fill=tk.X, padx=10, pady=(0, 10))
        else:
            tk.Label(
                frame,
                text="❌ 文件不存在或未配置",
                font=("Microsoft YaHei UI", 9),
                bg="white",
                fg="#e74c3c"
            ).pack(padx=10, pady=10)

# 如果直接运行此文件,显示错误提示
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
