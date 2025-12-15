#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
tab_firmware_lib.py - 固件库标签页
版本: v1.2.5
作者: Benson@Wisefido

⚠️ 此模块不能单独运行，必须从 flash_tool.py 主入口启动！

v1.2.5 新功能：
- 改进兼容性判断，显示判断原因
- 关联源代码目录，显示.syscfg/.cfg文件
- 合并同一项目的多个固件，添加比较栏
- 三栏布局：项目列表 | 项目详情 | 固件比较
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
        self.app_firmware = None          # 应用固件路径(.appimage或.bin)【必须】
        
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
        self.source_directory = None      # 关联的源代码目录（如果存在）
        self.variants = []                # 同一项目的其他固件变体

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
        """创建标签页UI - v1.2.0项目级管理"""
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
                # 查找.appimage或.bin固件文件
                firmware_files = [f for f in files 
                                if (f.endswith('.appimage') or f.endswith('.bin'))
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
                    # 尝试关联源代码目录
                    self._link_source_directory(project)
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
            
            # 选择主固件（优先选择Release版本或FreeRTOS版本）
            primary_fw = self._select_primary_firmware(firmwares)
            project.app_firmware = primary_fw['path']
            project.project_path = primary_fw['root']
            
            # 存储所有固件变体
            project.variants = []
            for fw in firmwares:
                if fw != primary_fw:
                    variant_project = FirmwareProject()
                    variant_project.name = project_name
                    variant_project.app_firmware = fw['path']
                    variant_project.project_path = fw['root']
                    variant_project.sdk_source = source_name
                    project.variants.append(variant_project)
            
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
            
            # 推荐SBL
            project.recommended_sbl = self._recommend_sbl(project)
            if project.recommended_sbl:
                project.selected_sbl = project.recommended_sbl[0]['path']
            
            # 推荐雷达参数配置
            project.recommended_radar_cfg = self._recommend_radar_cfg(project)
            if project.recommended_radar_cfg:
                project.selected_radar_cfg = project.recommended_radar_cfg[0]['path']
            
            return project
            
        except Exception as e:
            print(f"创建项目失败 ({project_name}): {e}")
            return None
    
    def _select_primary_firmware(self, firmwares):
        """选择主推荐固件
        
        优先级：Release > FreeRTOS > NoRTOS > 其他
        """
        # 优先选择Release版本
        for fw in firmwares:
            if 'release' in fw['name'].lower():
                return fw
        
        # 其次选择FreeRTOS版本
        for fw in firmwares:
            if 'freertos' in fw['name'].lower():
                return fw
        
        # 再选择NoRTOS版本
        for fw in firmwares:
            if 'nortos' in fw['name'].lower():
                return fw
        
        # 默认返回第一个
        return firmwares[0]
    
    def _create_project_from_firmware(self, root_dir, firmware_file, source_name):
        """从固件文件创建项目对象"""
        try:
            project = FirmwareProject()
            project.project_path = root_dir
            project.sdk_source = source_name
            project.app_firmware = os.path.join(root_dir, firmware_file)
            
            # 从固件文件名提取项目名称
            project.name = self._extract_project_name_from_firmware(firmware_file)
            
            # 查找同目录下的可选配置文件
            files = os.listdir(root_dir)
            
            # 1. 查找.syscfg文件（可选）
            for f in files:
                if f.endswith('.syscfg'):
                    project.syscfg_file = os.path.join(root_dir, f)
                    break
            
            # 2. 查找RTOS .cfg文件（可选，JavaScript语法）
            for f in files:
                if f.endswith('.cfg'):
                    cfg_path = os.path.join(root_dir, f)
                    if self._is_rtos_cfg(cfg_path):
                        project.rtos_cfg_file = cfg_path
                        break
                    project.rtos_cfg_file = os.path.join(root_dir, f)
            
            # 推荐SBL
            project.recommended_sbl = self._recommend_sbl(project)
            if project.recommended_sbl:
                project.selected_sbl = project.recommended_sbl[0]['path']  # 默认选择优先级最高的
            
            # 推荐雷达参数配置
            project.recommended_radar_cfg = self._recommend_radar_cfg(project)
            if project.recommended_radar_cfg:
                project.selected_radar_cfg = project.recommended_radar_cfg[0]['path']
            
            # 设置描述和兼容性
            project.description = self._get_description(project.name)
            project.compatibility = self._check_compatibility(project)
            
            return project
            
        except Exception as e:
            print(f"创建项目对象错误 ({root_dir}): {e}")
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
    
    def _link_source_directory(self, project):
        """关联源代码目录（如果存在）"""
        try:
            # 如果已有配置文件，不需要查找
            if project.syscfg_file or project.rtos_cfg_file:
                return
            
            # 从 prebuilt_binaries 向上1级查找源代码目录（修正：从2级改为1级）
            current_dir = project.project_path
            
            # 检查是否在 prebuilt_binaries 目录中
            if 'prebuilt_binaries' not in current_dir.lower():
                return
            
            # 向上1级：prebuilt_binaries -> [项目目录]
            parent_dir = os.path.dirname(current_dir)
            
            if not os.path.exists(parent_dir):
                return
            
            # 检查父目录是否包含源代码文件
            has_source = False
            syscfg_file = None
            rtos_cfg_file = None
            
            for file in os.listdir(parent_dir):
                file_path = os.path.join(parent_dir, file)
                if file.endswith('.syscfg'):
                    syscfg_file = file_path
                    has_source = True
                elif file.endswith('.cfg') and self._is_rtos_cfg(file_path):
                    rtos_cfg_file = file_path
                    has_source = True
                elif file.endswith(('.c', '.cpp', '.h')):
                    has_source = True
            
            if has_source:
                project.source_directory = parent_dir
                if syscfg_file:
                    project.syscfg_file = syscfg_file
                if rtos_cfg_file:
                    project.rtos_cfg_file = rtos_cfg_file
                    
        except Exception as e:
            # 静默失败，不影响主流程
            pass
    
    def _extract_base_name(self, firmware_path):
        """提取固件的基础名称（去除变体后缀）"""
        filename = os.path.basename(firmware_path)
        name = os.path.splitext(filename)[0].lower()
        
        # 移除常见变体标识
        variants = ['_freertos', '_nortos', '_release', '_debug', 
                   '_ti-arm-clang', '_arm-clang', '_gcc', '_ccs',
                   '_system', '_mss', '_dss']
        
        for variant in variants:
            name = name.replace(variant, '')
        
        return name
    
    def _group_firmware_variants(self):
        """将同一项目的多个固件变体分组"""
        try:
            # 按基础名称分组
            groups = {}
            for project in self.projects:
                base_name = self._extract_base_name(project.app_firmware)
                if base_name not in groups:
                    groups[base_name] = []
                groups[base_name].append(project)
            
            # 为每个分组设置variants
            for base_name, projects_list in groups.items():
                if len(projects_list) > 1:
                    # 有多个变体
                    for project in projects_list:
                        # 将其他项目作为此项目的变体
                        project.variants = [p for p in projects_list if p != project]
                        
        except Exception as e:
            print(f"分组固件变体错误: {e}")
    
    def _identify_variant_type(self, filename):
        """识别固件变体类型"""
        filename_lower = filename.lower()
        
        types = []
        if 'freertos' in filename_lower:
            types.append("FreeRTOS")
        elif 'nortos' in filename_lower:
            types.append("NoRTOS")
            
        if 'ti-arm-clang' in filename_lower or 'tiarmclang' in filename_lower:
            types.append("TI-ARM")
        elif 'arm-clang' in filename_lower or 'armclang' in filename_lower:
            types.append("ARM")
        elif 'gcc' in filename_lower:
            types.append("GCC")
            
        if 'debug' in filename_lower:
            types.append("Debug")
        elif 'release' in filename_lower:
            types.append("Release")
        
        return " + ".join(types) if types else "Standard"
    
    def _recommend_sbl(self, project):
        """推荐SBL固件 - 3级优先级"""
        recommendations = []
        
        # Priority 1: 项目本地SBL（与应用固件同目录或父目录）
        search_dirs = [
            project.project_path,
            os.path.dirname(project.project_path),
        ]
        
        for search_dir in search_dirs:
            if os.path.exists(search_dir):
                for root, dirs, files in os.walk(search_dir):
                    for f in files:
                        if 'sbl' in f.lower() and f.endswith(('.appimage', '.bin')):
                            sbl_path = os.path.join(root, f)
                            recommendations.append({
                                'path': sbl_path,
                                'source': '项目本地',
                                'priority': 1,
                                'reason': '与应用固件在同一项目'
                            })
        
        # Priority 2: 同一SDK/SBL标准目录
        for sdk_path in self.sdk_paths:
            if not os.path.exists(sdk_path):
                continue
            
            # 查找SDK中的SBL目录
            sbl_dirs = [
                os.path.join(sdk_path, 'tools', 'sbl'),
                os.path.join(sdk_path, 'source', 'ti', 'examples', 'sbl'),
                os.path.join(sdk_path, 'examples', 'sbl'),
            ]
            
            for sbl_dir in sbl_dirs:
                if os.path.exists(sbl_dir):
                    for root, dirs, files in os.walk(sbl_dir):
                        for f in files:
                            if 'sbl' in f.lower() and f.endswith(('.appimage', '.bin')):
                                # 检查是否兼容6844
                                if '6844' in f.lower() or '68xx' in f.lower() or 'xwrl68' in f.lower():
                                    sbl_path = os.path.join(root, f)
                                    recommendations.append({
                                        'path': sbl_path,
                                        'source': os.path.basename(sdk_path),
                                        'priority': 2,
                                        'reason': '来自同一SDK标准目录'
                                    })
        
        # Priority 3: 其他SDK通用SBL
        for sdk_path in self.sdk_paths:
            if not os.path.exists(sdk_path) or sdk_path == project.sdk_source:
                continue
            
            sbl_dirs = [
                os.path.join(sdk_path, 'tools', 'sbl'),
                os.path.join(sdk_path, 'source', 'ti', 'examples', 'sbl'),
            ]
            
            for sbl_dir in sbl_dirs:
                if os.path.exists(sbl_dir):
                    for root, dirs, files in os.walk(sbl_dir):
                        for f in files:
                            if 'sbl' in f.lower() and f.endswith(('.appimage', '.bin')):
                                if '6844' in f.lower() or '68xx' in f.lower():
                                    sbl_path = os.path.join(root, f)
                                    recommendations.append({
                                        'path': sbl_path,
                                        'source': os.path.basename(sdk_path),
                                        'priority': 3,
                                        'reason': '来自其他SDK（通用SBL）'
                                    })
        
        # 去重和排序
        seen = set()
        unique_recommendations = []
        for rec in recommendations:
            if rec['path'] not in seen:
                seen.add(rec['path'])
                unique_recommendations.append(rec)
        
        unique_recommendations.sort(key=lambda x: x['priority'])
        return unique_recommendations
    
    def _recommend_radar_cfg(self, project):
        """推荐雷达参数配置 - 4级优先级"""
        recommendations = []
        
        # Priority 1: 项目根目录/profile.cfg
        profile_path = os.path.join(project.project_path, 'profile.cfg')
        if os.path.exists(profile_path):
            recommendations.append({
                'path': profile_path,
                'source': '项目默认',
                'priority': 1,
                'reason': '项目标准配置文件'
            })
        
        # Priority 2: 项目/config/目录
        config_dir = os.path.join(project.project_path, 'config')
        if os.path.exists(config_dir):
            for f in os.listdir(config_dir):
                if f.endswith('.cfg') and not self._is_rtos_cfg(os.path.join(config_dir, f)):
                    cfg_path = os.path.join(config_dir, f)
                    recommendations.append({
                        'path': cfg_path,
                        'source': '项目配置目录',
                        'priority': 2,
                        'reason': '项目自定义配置'
                    })
        
        # Priority 3: SDK示例配置
        for sdk_path in self.sdk_paths:
            if not os.path.exists(sdk_path):
                continue
            
            cfg_dirs = [
                os.path.join(sdk_path, 'examples', 'profiles'),
                os.path.join(sdk_path, 'tools', 'profiles'),
            ]
            
            for cfg_dir in cfg_dirs:
                if os.path.exists(cfg_dir):
                    for f in os.listdir(cfg_dir):
                        if f.endswith('.cfg'):
                            cfg_path = os.path.join(cfg_dir, f)
                            recommendations.append({
                                'path': cfg_path,
                                'source': os.path.basename(sdk_path),
                                'priority': 3,
                                'reason': 'SDK标准示例配置'
                            })
        
        # 去重和排序
        seen = set()
        unique_recommendations = []
        for rec in recommendations:
            if rec['path'] not in seen:
                seen.add(rec['path'])
                unique_recommendations.append(rec)
        
        unique_recommendations.sort(key=lambda x: x['priority'])
        return unique_recommendations[:5]  # 最多返回5个推荐
    
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
            
            # 显示固件数量（包括主固件+变体）
            firmware_count = 1 + len(project.variants)
            variant_info = f" ({firmware_count}个固件)" if firmware_count > 1 else ""
            
            display_name = f"📁 {project.name}{variant_info} [{context}]"
            self.project_listbox.insert(tk.END, display_name)
    
    def on_project_select(self, event):
        """项目选择事件"""
        selection = self.project_listbox.curselection()
        if not selection:
            return
        
        index = selection[0]
        self.current_project = self.projects[index]
        
        # 加载配置
        self._load_project_config()
        
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
        self._create_directory_tree_tab(tab_control, project)  # 新增：项目目录树
        self._create_sbl_tab(tab_control, project)
        self._create_firmware_tab(tab_control, project)
        self._create_config_tab(tab_control, project)
        self._create_analysis_tab(tab_control, project)
    
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
    
    def _get_priority_icon(self, priority):
        """获取优先级图标"""
        icons = {1: "⭐⭐⭐", 2: "⭐⭐", 3: "⭐", 4: ""}
        return icons.get(priority, "")
    
    def _on_sbl_change(self):
        """SBL选择变化"""
        if self.current_project:
            self.current_project.selected_sbl = self.sbl_var.get()
    
    def _on_default_cfg_change(self):
        """默认配置复选框变化"""
        # 禁用/启用雷达配置选项
        state = tk.DISABLED if self.use_default_cfg.get() else tk.NORMAL
        # TODO: 更新所有雷达配置RadioButton的状态
        # 重新显示详情
        self.show_project_details()
    
    def _select_custom_sbl(self):
        """选择自定义SBL"""
        filepath = filedialog.askopenfilename(
            title="选择SBL固件",
            filetypes=[("固件文件", "*.appimage *.bin"), ("所有文件", "*.*")]
        )
        if filepath:
            self.current_project.selected_sbl = filepath
            self.sbl_var.set(filepath)
            messagebox.showinfo("成功", f"已选择SBL: {os.path.basename(filepath)}")
    
    def _select_custom_radar_cfg(self):
        """选择自定义雷达配置"""
        filepath = filedialog.askopenfilename(
            title="选择雷达参数配置",
            filetypes=[("配置文件", "*.cfg"), ("所有文件", "*.*")]
        )
        if filepath:
            self.current_project.selected_radar_cfg = filepath
            self.radar_cfg_var.set(filepath)
            messagebox.showinfo("成功", f"已选择配置: {os.path.basename(filepath)}")
    
    def _save_project_config(self):
        """保存项目配置到JSON"""
        if not self.current_project:
            return
        
        config = {
            'selected_sbl': self.current_project.selected_sbl,
            'selected_radar_cfg': self.current_project.selected_radar_cfg,
            'use_default_cfg': self.use_default_cfg.get(),
            'last_used': True
        }
        
        config_file = os.path.join(self.current_project.project_path, '.flash_tool_config.json')
        try:
            with open(config_file, 'w', encoding='utf-8') as f:
                json.dump(config, f, indent=2, ensure_ascii=False)
            messagebox.showinfo("成功", "项目配置已保存")
        except Exception as e:
            messagebox.showerror("错误", f"保存配置失败: {e}")
    
    def _load_project_config(self):
        """加载项目配置"""
        if not self.current_project:
            return
        
        config_file = os.path.join(self.current_project.project_path, '.flash_tool_config.json')
        if not os.path.exists(config_file):
            # 使用默认配置
            if self.current_project.recommended_sbl:
                self.sbl_var.set(self.current_project.recommended_sbl[0]['path'])
            if self.current_project.recommended_radar_cfg:
                self.radar_cfg_var.set(self.current_project.recommended_radar_cfg[0]['path'])
            return
        
        try:
            with open(config_file, 'r', encoding='utf-8') as f:
                config = json.load(f)
            
            self.current_project.selected_sbl = config.get('selected_sbl')
            self.current_project.selected_radar_cfg = config.get('selected_radar_cfg')
            self.use_default_cfg.set(config.get('use_default_cfg', True))
            
            # 更新UI变量
            if self.current_project.selected_sbl:
                self.sbl_var.set(self.current_project.selected_sbl)
            if self.current_project.selected_radar_cfg:
                self.radar_cfg_var.set(self.current_project.selected_radar_cfg)
        except Exception as e:
            print(f"加载配置失败: {e}")
    
    def flash_project(self):
        """一键填充项目文件到基本烧录页"""
        if not self.current_project:
            messagebox.showwarning("提示", "请先选择一个项目")
            return
        
        # TODO: 实现一键填充逻辑
        # 1. 检查SBL和应用固件
        # 2. 调用基本烧录页的烧录功能
        # 3. 显示进度和结果
        
        messagebox.showinfo("开发中", "一键填充功能正在开发中...\n\n请使用'加载到基本烧录页'按钮")
    
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
        
        # SBL固件
        sbl_frame = tk.LabelFrame(
            scrollable_frame,
            text="🔧 SBL引导固件",
            font=("Microsoft YaHei UI", 10, "bold"),
            bg="white",
            fg="#2c3e50",
            bd=2,
            relief=tk.GROOVE
        )
        sbl_frame.pack(fill=tk.X, padx=10, pady=10)
        
        if project.recommended_sbl:
            for i, sbl in enumerate(project.recommended_sbl[:3]):
                sbl_item_frame = tk.Frame(sbl_frame, bg="white")
                sbl_item_frame.pack(fill=tk.X, padx=10, pady=5)
                
                rb = tk.Radiobutton(
                    sbl_item_frame,
                    text=f"{self._get_priority_icon(sbl['priority'])} {os.path.basename(sbl['path'])}",
                    variable=self.sbl_var,
                    value=sbl['path'],
                    font=("Microsoft YaHei UI", 9),
                    bg="white",
                    fg="#34495e",
                    selectcolor="white"
                )
                rb.pack(anchor=tk.W)
                
                # 完整路径
                path_text = tk.Text(
                    sbl_item_frame,
                    font=("Consolas", 8),
                    bg="#f8f9fa",
                    fg="#495057",
                    relief=tk.FLAT,
                    height=3,
                    wrap=tk.WORD
                )
                path_text.insert(1.0, sbl['path'])
                path_text.config(state='disabled')
                path_text.pack(fill=tk.BOTH, expand=True, padx=20, pady=(2, 5))
                
                # 推荐原因
                tk.Label(
                    sbl_item_frame,
                    text=f"💡 {sbl['reason']} (来源: {sbl['source']})",
                    font=("Microsoft YaHei UI", 8),
                    bg="white",
                    fg="#7f8c8d"
                ).pack(anchor=tk.W, padx=20)
            
            tk.Button(
                sbl_frame,
                text="📂 选择其他SBL",
                font=("Microsoft YaHei UI", 9),
                command=self._select_custom_sbl,
                bg="#95a5a6",
                fg="white",
                relief=tk.FLAT,
                padx=10,
                pady=3
            ).pack(padx=10, pady=5, anchor=tk.W)
        else:
            tk.Label(
                sbl_frame,
                text="⚠️ 未找到推荐的SBL，请手动选择",
                font=("Microsoft YaHei UI", 9),
                bg="white",
                fg="#e67e22"
            ).pack(padx=10, pady=5)
        
        # 雷达配置
        radar_frame = tk.LabelFrame(
            scrollable_frame,
            text="📡 雷达参数配置",
            font=("Microsoft YaHei UI", 10, "bold"),
            bg="white",
            fg="#2c3e50",
            bd=2,
            relief=tk.GROOVE
        )
        radar_frame.pack(fill=tk.X, padx=10, pady=10)
        
        if project.recommended_radar_cfg:
            for i, cfg in enumerate(project.recommended_radar_cfg[:3]):
                cfg_item_frame = tk.Frame(radar_frame, bg="white")
                cfg_item_frame.pack(fill=tk.X, padx=10, pady=5)
                
                rb = tk.Radiobutton(
                    cfg_item_frame,
                    text=f"{self._get_priority_icon(cfg['priority'])} {os.path.basename(cfg['path'])}",
                    variable=self.radar_cfg_var,
                    value=cfg['path'],
                    font=("Microsoft YaHei UI", 9),
                    bg="white",
                    fg="#34495e",
                    selectcolor="white"
                )
                rb.pack(anchor=tk.W)
                
                # 完整路径
                path_text = tk.Text(
                    cfg_item_frame,
                    font=("Consolas", 8),
                    bg="#f8f9fa",
                    fg="#495057",
                    relief=tk.FLAT,
                    height=3,
                    wrap=tk.WORD
                )
                path_text.insert(1.0, cfg['path'])
                path_text.config(state='disabled')
                path_text.pack(fill=tk.BOTH, expand=True, padx=20, pady=(2, 5))
                
                # 推荐原因
                tk.Label(
                    cfg_item_frame,
                    text=f"💡 {cfg['reason']} (来源: {cfg['source']})",
                    font=("Microsoft YaHei UI", 8),
                    bg="white",
                    fg="#7f8c8d"
                ).pack(anchor=tk.W, padx=20)
        else:
            # 显示无雷达配置的原因
            reason = self._get_no_radar_cfg_reason(project)
            tk.Label(
                radar_frame,
                text=f"ℹ️ {reason}",
                font=("Microsoft YaHei UI", 9),
                bg="white",
                fg="#3498db",
                wraplength=700,
                justify=tk.LEFT
            ).pack(padx=10, pady=10, anchor=tk.W)
    
    def _create_directory_tree_tab(self, tab_control, project):
        """创建项目目录树标签页"""
        tree_tab = tk.Frame(tab_control, bg="white")
        tab_control.add(tree_tab, text="🌲 项目目录树")
        
        # 使用ScrolledText显示目录树
        import tkinter.scrolledtext as scrolledtext
        tree_text = scrolledtext.ScrolledText(
            tree_tab,
            font=("Consolas", 9),
            bg="#f8f9fa",
            fg="#2c3e50",
            wrap=tk.NONE,
            padx=15,
            pady=15
        )
        tree_text.pack(fill=tk.BOTH, expand=True)
        
        # 生成目录树内容
        self._generate_directory_tree(tree_text, project)
        
        # 配置文本样式
        tree_text.tag_config("title", font=("Microsoft YaHei UI", 11, "bold"), foreground="#2c3e50")
        tree_text.tag_config("subtitle", font=("Microsoft YaHei UI", 10, "bold"), foreground="#3498db")
        tree_text.tag_config("comment", foreground="#7f8c8d", font=("Microsoft YaHei UI", 9))
        tree_text.tag_config("folder", foreground="#e67e22", font=("Consolas", 9, "bold"))
        tree_text.tag_config("file", foreground="#27ae60")
        tree_text.tag_config("important", foreground="#e74c3c", font=("Consolas", 9, "bold"))
        
        tree_text.config(state='disabled')
    
    def _generate_directory_tree(self, text_widget, project):
        """生成项目目录树内容"""
        text_widget.insert(tk.END, "🌲 项目完整目录结构\n\n", "title")
        
        # 基本信息
        text_widget.insert(tk.END, "📂 项目路径\n", "subtitle")
        project_dir = os.path.dirname(project.app_firmware)
        
        # 向上查找到项目根目录（包含多个构建目标的目录）
        while project_dir:
            parent = os.path.dirname(project_dir)
            if not parent or parent == project_dir:
                break
            # 检查父目录是否包含其他构建变体
            if os.path.exists(os.path.join(parent, "r5fss0-0_freertos")) or \
               os.path.exists(os.path.join(parent, "system_freertos")) or \
               "examples" in parent.lower():
                project_dir = parent
            else:
                break
        
        text_widget.insert(tk.END, f"{project_dir}\n\n")
        
        # 递归生成目录树
        text_widget.insert(tk.END, "📊 目录结构\n\n", "subtitle")
        
        def generate_tree(path, prefix="", is_last=True):
            """递归生成目录树"""
            if not os.path.exists(path):
                return
            
            try:
                items = sorted(os.listdir(path))
            except PermissionError:
                return
            
            # 过滤掉隐藏文件和不需要的目录
            items = [item for item in items if not item.startswith('.') and item not in ['__pycache__', 'Debug', 'Release']]
            
            dirs = [item for item in items if os.path.isdir(os.path.join(path, item))]
            files = [item for item in items if os.path.isfile(os.path.join(path, item))]
            
            all_items = dirs + files
            
            for idx, item in enumerate(all_items):
                is_last_item = (idx == len(all_items) - 1)
                connector = "└── " if is_last_item else "├── "
                item_path = os.path.join(path, item)
                
                if os.path.isdir(item_path):
                    # 目录
                    text_widget.insert(tk.END, prefix + connector, "")
                    text_widget.insert(tk.END, f"{item}/", "folder")
                    
                    # 添加注释
                    comment = ""
                    if "freertos" in item.lower():
                        comment = "  ← FreeRTOS操作系统"
                    elif "nortos" in item.lower():
                        comment = "  ← NoRTOS裸机"
                    elif "r5fss0" in item.lower():
                        comment = "  ← R5F核心"
                    elif "c66ss0" in item.lower():
                        comment = "  ← C66x DSP核心"
                    elif "system" in item.lower():
                        comment = "  ← 双核系统固件"
                    elif item.lower() == "config":
                        comment = "  ← 配置文件目录"
                    elif "ti-arm-clang" in item.lower():
                        comment = "  ← TI ARM编译器输出"
                    elif "ti-c6000" in item.lower():
                        comment = "  ← TI C6000编译器输出"
                    
                    if comment:
                        text_widget.insert(tk.END, comment, "comment")
                    text_widget.insert(tk.END, "\n")
                    
                    # 递归（只深入2层避免太长）
                    if prefix.count("│") + prefix.count(" ") < 8:
                        new_prefix = prefix + ("    " if is_last_item else "│   ")
                        generate_tree(item_path, new_prefix, is_last_item)
                else:
                    # 文件
                    text_widget.insert(tk.END, prefix + connector, "")
                    
                    # 根据文件类型使用不同标签
                    if item.endswith(('.appimage', '.out', '.bin')):
                        text_widget.insert(tk.END, f"{item}", "important")
                        # 添加文件大小
                        try:
                            size = os.path.getsize(item_path) / 1024
                            text_widget.insert(tk.END, f"  [{size:.2f} KB]", "comment")
                        except:
                            pass
                    elif item.endswith(('.cfg', '.json', '.xml')):
                        text_widget.insert(tk.END, f"{item}", "file")
                    else:
                        text_widget.insert(tk.END, f"{item}", "")
                    
                    # 添加文件注释
                    comment = ""
                    if item.endswith('.appimage'):
                        if 'system' in item.lower():
                            comment = "  ← ⭐ 双核系统固件"
                        else:
                            comment = "  ← 单核应用固件"
                    elif item.endswith('.syscfg'):
                        comment = "  ← SysConfig配置"
                    elif item == 'main.c':
                        comment = "  ← 主程序源码"
                    elif item == 'linker.cmd':
                        comment = "  ← 链接脚本"
                    elif item == 'makefile':
                        comment = "  ← 构建脚本"
                    elif item.endswith('.projectspec'):
                        comment = "  ← CCS项目配置"
                    elif 'metaimage' in item.lower():
                        comment = "  ← 固件打包配置"
                    elif item == 'system.xml':
                        comment = "  ← 多核系统配置"
                    
                    if comment:
                        text_widget.insert(tk.END, comment, "comment")
                    text_widget.insert(tk.END, "\n")
        
        # 生成树
        generate_tree(project_dir)
        
        # 添加说明
        text_widget.insert(tk.END, "\n\n📝 目录说明\n\n", "subtitle")
        text_widget.insert(tk.END, "📁 ", "folder")
        text_widget.insert(tk.END, "目录/\n", "folder")
        text_widget.insert(tk.END, "📄 ", "important")
        text_widget.insert(tk.END, "重要固件文件\n", "important")
        text_widget.insert(tk.END, "📄 ", "file")
        text_widget.insert(tk.END, "配置文件\n", "file")
        text_widget.insert(tk.END, "📄 普通文件\n\n")
        
        text_widget.insert(tk.END, "💡 提示\n", "subtitle")
        text_widget.insert(tk.END, "• system_freertos/ - 推荐用于完整雷达应用\n")
        text_widget.insert(tk.END, "• r5fss0-0_freertos/ - 单核FreeRTOS应用\n")
        text_widget.insert(tk.END, "• *_nortos/ - 裸机版本，体积更小\n")
        text_widget.insert(tk.END, "• .appimage - TI标准固件格式\n")
    
    def _create_sbl_tab(self, tab_control, project):
        """创建SBL固件标签页 - 引用完整分析中的SBL部分"""
        sbl_tab = tk.Frame(tab_control, bg="white")
        tab_control.add(sbl_tab, text="🔧 SBL固件")
        
        # 使用ScrolledText显示SBL分析
        import tkinter.scrolledtext as scrolledtext
        sbl_text = scrolledtext.ScrolledText(
            sbl_tab,
            font=("Microsoft YaHei UI", 9),
            bg="white",
            wrap=tk.WORD,
            padx=15,
            pady=15
        )
        sbl_text.pack(fill=tk.BOTH, expand=True)
        
        # 生成SBL分析内容（从完整分析中提取）
        self._add_sbl_analysis_section(sbl_text, project)
        
        # 配置文本样式
        sbl_text.tag_config("title", font=("Microsoft YaHei UI", 12, "bold"), foreground="#2c3e50")
        sbl_text.tag_config("subtitle", font=("Microsoft YaHei UI", 10, "bold"), foreground="#3498db")
        sbl_text.tag_config("important", foreground="#e74c3c", font=("Microsoft YaHei UI", 9, "bold"))
        sbl_text.tag_config("note", foreground="#7f8c8d", font=("Microsoft YaHei UI", 9))
        sbl_text.tag_config("success", foreground="#27ae60", font=("Microsoft YaHei UI", 9))
        sbl_text.tag_config("current", foreground="#27ae60", font=("Microsoft YaHei UI", 9, "bold"))
        
        sbl_text.config(state='disabled')
    
    def _create_firmware_tab(self, tab_control, project):
        """创建固件变体标签页 - 引用完整分析中的固件对比部分"""
        firmware_tab = tk.Frame(tab_control, bg="white")
        tab_control.add(firmware_tab, text="🔄 固件变体")
        
        # 使用ScrolledText显示固件分析
        import tkinter.scrolledtext as scrolledtext
        firmware_text = scrolledtext.ScrolledText(
            firmware_tab,
            font=("Microsoft YaHei UI", 9),
            bg="white",
            wrap=tk.WORD,
            padx=15,
            pady=15
        )
        firmware_text.pack(fill=tk.BOTH, expand=True)
        
        # 生成固件分析内容（从完整分析中提取）
        self._add_firmware_analysis_section(firmware_text, project)
        
        # 配置文本样式
        firmware_text.tag_config("title", font=("Microsoft YaHei UI", 12, "bold"), foreground="#2c3e50")
        firmware_text.tag_config("subtitle", font=("Microsoft YaHei UI", 10, "bold"), foreground="#3498db")
        firmware_text.tag_config("current", foreground="#27ae60", font=("Microsoft YaHei UI", 9, "bold"))
        firmware_text.tag_config("variant", foreground="#7f8c8d")
        
        firmware_text.config(state='disabled')
    
    def _create_config_tab(self, tab_control, project):
        """创建雷达配置标签页 - 引用完整分析中的配置部分"""
        config_tab = tk.Frame(tab_control, bg="white")
        tab_control.add(config_tab, text="📡 雷达配置")
        
        # 使用ScrolledText显示配置分析
        import tkinter.scrolledtext as scrolledtext
        config_text = scrolledtext.ScrolledText(
            config_tab,
            font=("Microsoft YaHei UI", 9),
            bg="white",
            wrap=tk.WORD,
            padx=15,
            pady=15
        )
        config_text.pack(fill=tk.BOTH, expand=True)
        
        # 生成配置分析内容（从完整分析中提取）
        self._add_config_analysis_section(config_text, project)
        
        # 配置文本样式
        config_text.tag_config("title", font=("Microsoft YaHei UI", 12, "bold"), foreground="#2c3e50")
        config_text.tag_config("subtitle", font=("Microsoft YaHei UI", 10, "bold"), foreground="#3498db")
        config_text.tag_config("note", foreground="#7f8c8d")
        config_text.tag_config("important", foreground="#e74c3c")
        config_text.tag_config("current", foreground="#27ae60", font=("Microsoft YaHei UI", 9, "bold"))
        
        config_text.config(state='disabled')
    
    def _create_analysis_tab(self, tab_control, project):
        """创建完整分析标签页"""
        analysis_tab = tk.Frame(tab_control, bg="white")
        tab_control.add(analysis_tab, text="📊 完整分析")
        
        import tkinter.scrolledtext as scrolledtext
        info_text = scrolledtext.ScrolledText(
            analysis_tab,
            font=("Microsoft YaHei UI", 9),
            bg="white",
            wrap=tk.WORD,
            padx=15,
            pady=15
        )
        info_text.pack(fill=tk.BOTH, expand=True)
        
        # 根据项目类型生成完整分析
        if 'hello_world' in project.name.lower():
            self._add_hello_world_full_analysis(info_text, project)
        elif 'mmwave_demo' in project.name.lower() or 'mmw_demo' in project.name.lower():
            self._add_mmwave_demo_full_analysis(info_text, project)
        else:
            self._add_generic_full_analysis(info_text, project)
        
        # 配置文本样式
        info_text.tag_config("title", font=("Microsoft YaHei UI", 12, "bold"), foreground="#2c3e50")
        info_text.tag_config("subtitle", font=("Microsoft YaHei UI", 10, "bold"), foreground="#3498db")
        info_text.tag_config("line", foreground="#bdc3c7")
        
        info_text.config(state='disabled')
    
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
    
    def _get_no_radar_cfg_reason(self, project):
        """获取无雷达配置文件的原因"""
        if 'hello_world' in project.name.lower():
            return ("Hello World是基础启动示例项目，主要用于验证硬件和SDK环境，"
                   "不涉及雷达信号处理功能，因此不需要雷达配置文件。\n\n"
                   "如需雷达功能，请使用mmwave_demo等雷达应用项目。")
        elif 'empty' in project.name.lower():
            return "这是空白项目模板，用于创建自定义应用，不包含预配置的雷达参数。"
        elif 'sbl' in project.name.lower() or 'boot' in project.name.lower():
            return "这是引导加载程序(SBL)，只负责启动应用固件，不涉及雷达配置。"
        else:
            return ("该项目未包含雷达配置文件，可能原因：\n"
                   "1. 非雷达应用项目（如驱动示例、内核示例）\n"
                   "2. 使用代码配置而非.cfg文件\n"
                   "3. 配置文件位于其他位置")
    
    def _get_priority_icon(self, priority):
        """获取优先级图标"""
        priority_map = {
            'high': '🔴 高',
            'medium': '🟡 中',
            'low': '🟢 低'
        }
        return priority_map.get(priority, priority)
    
    def _add_hello_world_firmware_analysis(self, text_widget, project):
        """添加HelloWorld固件分析"""
        content = f"""
🎯 Hello World 项目固件变体分析

该项目共有 {1 + len(project.variants)} 个固件版本，提供不同的配置选择：

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

📊 固件变体对比

"""
        text_widget.insert(tk.END, content)
        
        # 创建对比表
        all_firmwares = [project] + project.variants
        for i, fw in enumerate(all_firmwares):
            fw_path = fw.app_firmware
            fw_name = os.path.basename(fw_path)
            fw_size = os.path.getsize(fw_path) if os.path.exists(fw_path) else 0
            
            # 判断类型
            if 'system' in fw_name.lower():
                fw_type = "双核系统固件"
                cores = "R5F + C66x DSP"
                features = "完整雷达处理能力"
            else:
                fw_type = "单核固件"
                cores = "R5F"
                features = "基础功能"
            
            os_type = "FreeRTOS" if 'freertos' in fw_name.lower() else "NoRTOS(裸机)"
            
            marker = "▶" if i == 0 else " "
            text_widget.insert(tk.END, f"""
{marker} 固件 {i+1}: {fw_name}
   类型: {fw_type}
   核心: {cores}
   操作系统: {os_type}
   大小: {fw_size/1024:.2f} KB
   特点: {features}
   
""")
        
        text_widget.insert(tk.END, """
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

💡 选择建议

1️⃣ 单核 R5F 固件:
   - r5fss0-0_freertos: 使用FreeRTOS，支持多任务
   - r5fss0-0_nortos: 裸机运行，最小资源占用
   适用场景: 简单应用、学习入门

2️⃣ 双核 System 固件:
   - system_freertos: R5F+C66x，FreeRTOS管理
   - system_nortos: R5F+C66x，裸机运行
   适用场景: 雷达信号处理、高性能计算

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

🔑 关键差异

单核 vs 双核:
• 单核: 仅R5F，适合控制和通信
• 双核: R5F+C66x，C66x负责信号处理(FFT等)

FreeRTOS vs NoRTOS:
• FreeRTOS: 支持多任务、调度器、信号量等
• NoRTOS: 简单循环，适合固定流程

System固件组成:
• R5F核心固件: 主控制器
• C66x核心固件: DSP信号处理
• RF固件补丁: 雷达射频子系统
""")
    
    def _add_mmwave_demo_firmware_analysis(self, text_widget, project):
        """添加mmwave_demo固件分析"""
        fw_path = project.app_firmware
        fw_size = os.path.getsize(fw_path) if os.path.exists(fw_path) else 0
        
        content = f"""
🎯 mmwave_demo 项目固件分析

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

📦 固件信息

文件名: {os.path.basename(fw_path)}
大小: {fw_size/1024:.2f} KB ({fw_size:,} 字节)
类型: 单核R5F + RF固件
操作系统: FreeRTOS

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

🏗️ 固件架构

该固件包含两个核心组件:

1️⃣ R5F应用固件:
   • 主控制器ARM Cortex-R5F
   • FreeRTOS实时操作系统
   • 完整的雷达应用逻辑
   • 使用HWA(硬件加速器)进行信号处理

2️⃣ RF固件补丁:
   • 毫米波RF子系统固件
   • 60-64 GHz射频控制
   • 发射和接收链路管理

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

⚙️ 处理架构

与HelloWorld的System固件不同:
• mmwave_demo: R5F + HWA + RF
• HelloWorld System: R5F + C66x DSP + RF

mmwave_demo专注于R5F实现:
✅ 所有信号处理在R5F完成
✅ 使用HWA硬件加速(FFT、CFAR)
✅ 无需C66x DSP
✅ 功耗更低、延迟更小

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

📊 资源占用

代码大小: ~374 KB
主要包含:
• 雷达控制逻辑
• 信号处理算法(CFAR、AOA)
• 校准系统
• 数据流管理
• 电源管理
• RF监控系统
"""
        text_widget.insert(tk.END, content)
    
    def _add_generic_firmware_analysis(self, text_widget, project):
        """添加通用固件分析"""
        content = f"""
📦 固件基本信息

当前固件: {os.path.basename(project.app_firmware)}
"""
        if project.variants:
            content += f"发现 {len(project.variants)} 个其他固件变体\n\n"
        
        text_widget.insert(tk.END, content)
    
    def _add_mmwave_demo_config_analysis(self, text_widget, project):
        """添加mmwave_demo配置分析"""
        content = """
📡 mmwave_demo 雷达配置详解

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

🎯 配置文件清单

该项目包含 4 种雷达配置文件:

"""
        text_widget.insert(tk.END, content)
        
        # 列出所有配置
        for i, cfg in enumerate(project.recommended_radar_cfg, 1):
            cfg_name = os.path.basename(cfg['path'])
            
            if '4T4R' in cfg_name or '4t4r' in cfg_name:
                desc = """
1️⃣ profile_4T4R_tdm.cfg ⭐ 推荐 AWRL6844
   天线配置: 4发4收 (TDM时分复用)
   虚拟天线: 16通道
   角度分辨率: ~15°
   FOV: ±60° (方位+俯仰)
   适用场景: 高精度角度测量、3D定位
"""
            elif '3T4R' in cfg_name or '3t4r' in cfg_name:
                desc = """
2️⃣ profile_3T4R_tdm.cfg
   天线配置: 3发4收 (TDM时分复用)
   虚拟天线: 12通道
   角度分辨率: ~20°
   适用场景: 中等性能应用
"""
            elif '2T4R' in cfg_name or '2t4r' in cfg_name or 'bpm' in cfg_name.lower():
                desc = """
3️⃣ profile_2T4R_bpm.cfg
   天线配置: 2发4收 (BPM二进制相位调制)
   虚拟天线: 8通道
   功耗: 最低
   适用场景: 低功耗应用、简单检测
"""
            elif 'monitor' in cfg_name.lower():
                desc = """
4️⃣ monitors.cfg
   类型: RF监控配置
   包含: 完整的RF健康监控参数
   监控器: PLL、发射功率、基带、DC、环回等
"""
            else:
                desc = f"\n{i}️⃣ {cfg_name}\n   配置文件\n"
            
            text_widget.insert(tk.END, desc)
        
        text_widget.insert(tk.END, """

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

🔑 4T4R配置核心参数 (AWRL6844最佳配置)

📻 通道配置:
   channelCfg 153 255 0
   • 153 (0x99) = TX: 1,4,5,8 启用 (4发)
   • 255 (0xFF) = RX: 全部启用 (4收)
   • 结果: 4T4R完整配置

📡 Chirp配置:
   • 频段: 60-64 GHz
   • 带宽: 3 GHz
   • ADC采样: 256点
   • 扫频时间: 13.1 μs

🎯 帧配置:
   • Chirp数/帧: 64个
   • 帧周期: 100 ms
   • 帧率: 10 FPS

🔍 CFAR检测:
   • 距离CFAR: 8单元窗口，9.0 dB门限
   • 多普勒CFAR: 4单元窗口，9.0 dB门限

📐 测量范围:
   • 距离: 0.25 - 9.0 m
   • 速度: ±20.16 m/s
   • 角度: ±60° (方位和俯仰)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

💡 TDM-MIMO技术

4发4收TDM时序:
   Chirp1: TX1发射 → 4RX接收 → 4路数据
   Chirp2: TX4发射 → 4RX接收 → 4路数据
   Chirp3: TX5发射 → 4RX接收 → 4路数据
   Chirp4: TX8发射 → 4RX接收 → 4路数据
   合计: 16路虚拟天线数据

优势:
✅ 高角度分辨率 (~15°)
✅ 无需复杂相位编码
✅ 信号处理简化
⚠️ 帧率受限 (需4倍Chirp时间)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

🛡️ RF监控系统 (monitors.cfg)

包含8种监控器:

1️⃣ PLL监控: 锁相环控制电压
2️⃣ TX功率监控: 8个TX独立监控
3️⃣ 基带功率监控: 4个TX通道
4️⃣ DC偏置监控: 发射DC监控
5️⃣ TX-RX环回: 发射-接收链路测试
6️⃣ RX高通滤波器: 接收通路DC
7️⃣ 时钟监控: 电源和时钟稳定性
8️⃣ 温度监控: 芯片温度

监控启用掩码:
   enableRFmons 0x00000001FEABFEAB

作用:
✅ 确保RF性能稳定
✅ 早期故障检测
✅ 满足法规要求(FCC)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

🎯 配置选择建议

根据应用场景选择:

🏢 车内监控 / 驾驶员监控:
   推荐: profile_4T4R_tdm.cfg
   原因: 高精度角度测量、3D定位

🏭 工业入侵检测:
   推荐: profile_4T4R_tdm.cfg
   原因: 宽视场、多目标检测

🏠 智能家居 / 存在检测:
   推荐: profile_2T4R_bpm.cfg
   原因: 低功耗、简单检测足够

🔧 系统调试 / 性能验证:
   推荐: monitors.cfg
   原因: 完整RF健康检查
""")
    
    def _add_generic_config_analysis(self, text_widget, project):
        """添加通用配置分析"""
        if not project.recommended_radar_cfg:
            text_widget.insert(tk.END, "该项目未包含雷达配置文件\n")
            return
        
        content = "📡 雷达配置文件\n\n"
        for cfg in project.recommended_radar_cfg:
            content += f"• {os.path.basename(cfg['path'])}\n"
            content += f"  {cfg['reason']}\n\n"
        
        text_widget.insert(tk.END, content)
    
    def _add_sbl_analysis_section(self, text_widget, project):
        """SBL分析部分 - 从完整分析中提取"""
        text_widget.insert(tk.END, "🔧 SBL引导固件分析\n\n", "title")
        
        if not project.recommended_sbl:
            text_widget.insert(tk.END, "⚠️ 未找到推荐的SBL固件\n\n", "important")
            text_widget.insert(tk.END, "请手动选择SBL固件文件或检查SDK安装路径。\n\n", "note")
            text_widget.insert(tk.END, "SBL是二级引导加载程序，负责从Flash加载应用固件到RAM并启动。\n", "note")
            return
        
        # SBL说明
        text_widget.insert(tk.END, "📚 什么是SBL？\n\n", "subtitle")
        text_widget.insert(tk.END, "SBL (Secondary Bootloader) 是二级引导加载程序：\n\n")
        text_widget.insert(tk.END, "• 作用: 负责从Flash加载应用固件到RAM并启动\n")
        text_widget.insert(tk.END, "• 位置: 烧录到Flash的0x2000地址（8KB偏移）\n")
        text_widget.insert(tk.END, "• 类型: 独立于应用程序，可单独烧录\n")
        text_widget.insert(tk.END, "• 版本: sbl.release.appimage (完整版) 或 sbl_lite.release.appimage (精简版)\n\n")
        
        # 推荐的SBL列表
        text_widget.insert(tk.END, f"🎯 推荐的SBL固件（共{len(project.recommended_sbl)}个）\n\n", "subtitle")
        
        for idx, sbl in enumerate(project.recommended_sbl, 1):
            marker = "⭐ " if idx == 1 else f"{idx}. "
            text_widget.insert(tk.END, f"{marker}{os.path.basename(sbl['path'])}\n", "current" if idx == 1 else "")
            
            if os.path.exists(sbl['path']):
                size = os.path.getsize(sbl['path']) / 1024
                text_widget.insert(tk.END, f"   大小: {size:.2f} KB\n")
            
            text_widget.insert(tk.END, f"   推荐原因: {sbl['reason']}\n")
            text_widget.insert(tk.END, f"   来源: {sbl['source']}\n")
            text_widget.insert(tk.END, f"   优先级: {self._get_priority_text(sbl['priority'])}\n")
            text_widget.insert(tk.END, f"   完整路径:\n   {sbl['path']}\n\n")
        
        # 使用建议
        text_widget.insert(tk.END, "💡 使用建议\n\n", "subtitle")
        text_widget.insert(tk.END, "✅ 推荐使用标记为⭐的SBL固件\n")
        text_widget.insert(tk.END, "✅ 确保SBL与硬件平台匹配\n")
        text_widget.insert(tk.END, "✅ 使用与SDK版本对应的SBL\n")
        text_widget.insert(tk.END, "✅ 优先使用Release版本（已优化）\n")
    
    def _add_firmware_analysis_section(self, text_widget, project):
        """固件变体分析部分 - 从完整分析中提取"""
        text_widget.insert(tk.END, "🔄 固件变体分析\n\n", "title")
        
        if not project.variants:
            text_widget.insert(tk.END, "该项目只有一个固件版本\n\n", "note")
            text_widget.insert(tk.END, f"当前固件: {os.path.basename(project.app_firmware)}\n", "current")
            if os.path.exists(project.app_firmware):
                size = os.path.getsize(project.app_firmware) / 1024
                text_widget.insert(tk.END, f"大小: {size:.2f} KB\n")
            return
        
        # 固件列表
        text_widget.insert(tk.END, f"📊 该项目共有 {1 + len(project.variants)} 个固件变体\n\n", "subtitle")
        
        # variants是FirmwareProject对象列表，需要提取其app_firmware路径
        all_firmwares = [{'path': project.app_firmware, 'current': True}] + \
                       [{'path': v.app_firmware, 'current': False} for v in project.variants]
        
        for idx, fw in enumerate(all_firmwares, 1):
            if not os.path.exists(fw['path']):
                continue
            
            name = os.path.basename(fw['path'])
            size = os.path.getsize(fw['path']) / 1024
            marker = "⭐ [当前]" if fw['current'] else f"   [{idx}]"
            
            text_widget.insert(tk.END, f"{marker} {name}\n", "current" if fw['current'] else "variant")
            text_widget.insert(tk.END, f"     大小: {size:.2f} KB\n")
            
            # 分析固件类型
            fw_type = []
            fw_features = []
            
            if 'system' in name.lower():
                fw_type.append("双核系统固件")
                fw_features.append("架构: R5F + C66x DSP")
                fw_features.append("用途: 完整雷达信号处理")
                fw_features.append("DSP加速: 是")
            elif 'r5fss0-0' in name.lower() or 'r5f' in name.lower():
                fw_type.append("单核固件")
                fw_features.append("架构: 单核R5F")
                fw_features.append("用途: 基础应用和学习")
                fw_features.append("DSP加速: 否")
            
            if 'freertos' in name.lower():
                fw_type.append("FreeRTOS操作系统")
                fw_features.append("任务调度: 支持")
                fw_features.append("特性: 多任务、信号量、队列")
            elif 'nortos' in name.lower():
                fw_type.append("NoRTOS裸机")
                fw_features.append("任务调度: 无")
                fw_features.append("特性: 最小资源占用")
            
            if fw_type:
                text_widget.insert(tk.END, f"     类型: {', '.join(fw_type)}\n")
            
            for feature in fw_features:
                text_widget.insert(tk.END, f"     • {feature}\n")
            
            text_widget.insert(tk.END, "\n")
        
        # 对比分析
        text_widget.insert(tk.END, "💡 固件对比与选择建议\n\n", "subtitle")
        
        has_system = any('system' in os.path.basename(fw['path']).lower() for fw in all_firmwares)
        has_single = any('r5f' in os.path.basename(fw['path']).lower() and 'system' not in os.path.basename(fw['path']).lower() for fw in all_firmwares)
        
        if has_system and has_single:
            text_widget.insert(tk.END, "🔷 单核 vs 双核:\n")
            text_widget.insert(tk.END, "• 单核固件: 适合简单应用、学习入门、纯控制任务\n")
            text_widget.insert(tk.END, "• 双核固件: 适合雷达应用、信号处理、需要DSP加速的场景\n\n")
        
        has_freertos = any('freertos' in os.path.basename(fw['path']).lower() for fw in all_firmwares)
        has_nortos = any('nortos' in os.path.basename(fw['path']).lower() for fw in all_firmwares)
        
        if has_freertos and has_nortos:
            text_widget.insert(tk.END, "🔷 FreeRTOS vs NoRTOS:\n")
            text_widget.insert(tk.END, "• FreeRTOS: 支持多任务调度、适合复杂应用\n")
            text_widget.insert(tk.END, "• NoRTOS: 裸机运行、体积更小、实时性更高\n\n")
        
        # 推荐
        text_widget.insert(tk.END, "✅ 推荐选择:\n")
        if has_system:
            text_widget.insert(tk.END, "• 雷达应用: 选择 system_freertos 版本\n")
        if has_single:
            text_widget.insert(tk.END, "• 学习入门: 选择 r5fss0-0_freertos 或 r5fss0-0_nortos 版本\n")
    
    def _add_config_analysis_section(self, text_widget, project):
        """雷达配置分析部分 - 从完整分析中提取"""
        text_widget.insert(tk.END, "📡 雷达配置分析\n\n", "title")
        
        if not project.recommended_radar_cfg:
            reason = self._get_no_radar_cfg_reason(project)
            text_widget.insert(tk.END, "ℹ️ 该项目无雷达配置文件\n\n", "note")
            text_widget.insert(tk.END, f"{reason}\n\n")
            
            # 提供更多说明
            if 'hello_world' in project.name.lower():
                text_widget.insert(tk.END, "📝 说明\n\n", "subtitle")
                text_widget.insert(tk.END, "Hello World是基础示例项目，主要用于：\n")
                text_widget.insert(tk.END, "• 验证SDK开发环境\n")
                text_widget.insert(tk.END, "• 学习固件结构和编译流程\n")
                text_widget.insert(tk.END, "• 测试板件通信功能\n\n")
                text_widget.insert(tk.END, "如需雷达功能，请参考以下项目：\n")
                text_widget.insert(tk.END, "• mmwave_demo - 完整雷达演示\n")
                text_widget.insert(tk.END, "• area_scanner - 区域扫描应用\n")
                text_widget.insert(tk.END, "• people_tracking - 人员追踪\n")
            return
        
        # 配置文件列表
        text_widget.insert(tk.END, f"📋 该项目包含 {len(project.recommended_radar_cfg)} 个配置文件\n\n", "subtitle")
        
        for idx, cfg in enumerate(project.recommended_radar_cfg, 1):
            marker = "⭐ " if idx == 1 else f"{idx}. "
            text_widget.insert(tk.END, f"{marker}{os.path.basename(cfg['path'])}\n", "current" if idx == 1 else "")
            text_widget.insert(tk.END, f"   推荐原因: {cfg['reason']}\n")
            text_widget.insert(tk.END, f"   完整路径:\n   {cfg['path']}\n\n")
        
        # 配置说明
        text_widget.insert(tk.END, "💡 配置文件说明\n\n", "subtitle")
        text_widget.insert(tk.END, "雷达配置文件(.cfg)定义了雷达的工作参数：\n\n")
        text_widget.insert(tk.END, "• Chirp配置: 线性调频参数\n")
        text_widget.insert(tk.END, "• Frame配置: 帧参数和循环次数\n")
        text_widget.insert(tk.END, "• Profile配置: TX/RX通道、采样率等\n")
        text_widget.insert(tk.END, "• 算法参数: CFAR、DOA等算法配置\n\n")
        
        text_widget.insert(tk.END, "✅ 使用提示:\n")
        text_widget.insert(tk.END, "• 根据应用场景选择合适的配置\n")
        text_widget.insert(tk.END, "• 可在运行时通过CLI命令动态修改\n")
        text_widget.insert(tk.END, "• 建议先使用推荐配置进行测试\n")
    
    def _get_priority_text(self, priority):
        """获取优先级文本"""
        priority_map = {
            1: "最高 ⭐⭐⭐",
            2: "高 ⭐⭐",
            3: "中 ⭐",
            4: "低"
        }
        return priority_map.get(priority, f"优先级{priority}")
    
    def _add_hello_world_full_analysis(self, text_widget, project):
        """动态生成HelloWorld完整分析"""
        # 标题
        text_widget.insert(tk.END, "📊 Hello World 项目完整分析\n\n", "title")
        text_widget.insert(tk.END, "="*80 + "\n\n", "line")
        
        # 基本信息
        text_widget.insert(tk.END, "🎯 项目基本信息\n\n", "subtitle")
        text_widget.insert(tk.END, f"项目名称: {project.name}\n")
        text_widget.insert(tk.END, f"描述: {project.description}\n")
        text_widget.insert(tk.END, f"SDK来源: {project.sdk_source}\n")
        text_widget.insert(tk.END, f"兼容性: {project.compatibility_reason}\n\n")
        
        # 应用固件信息
        text_widget.insert(tk.END, "📦 应用固件\n\n", "subtitle")
        if project.app_firmware and os.path.exists(project.app_firmware):
            size = os.path.getsize(project.app_firmware) / 1024
            text_widget.insert(tk.END, f"文件名: {os.path.basename(project.app_firmware)}\n")
            text_widget.insert(tk.END, f"大小: {size:.2f} KB\n")
            text_widget.insert(tk.END, f"路径: {project.app_firmware}\n\n")
        
        # 固件变体分析
        if project.variants:
            text_widget.insert(tk.END, f"🔄 固件变体 (共{len(project.variants) + 1}个)\n\n", "subtitle")
            
            # 分析所有固件 - variants是FirmwareProject对象列表
            all_firmwares = [{'path': project.app_firmware, 'current': True}] + \
                           [{'path': v.app_firmware, 'current': False} for v in project.variants]
            
            for idx, fw in enumerate(all_firmwares, 1):
                if not os.path.exists(fw['path']):
                    continue
                    
                name = os.path.basename(fw['path'])
                size = os.path.getsize(fw['path']) / 1024
                marker = "⭐ [当前]" if fw['current'] else f"   [{idx}]"
                
                text_widget.insert(tk.END, f"{marker} {name}\n")
                text_widget.insert(tk.END, f"     大小: {size:.2f} KB\n")
                
                # 分析固件类型
                if 'freertos' in name.lower():
                    text_widget.insert(tk.END, "     类型: FreeRTOS系统\n")
                    text_widget.insert(tk.END, "     特性: 支持多任务调度、信号量、队列\n")
                elif 'nortos' in name.lower():
                    text_widget.insert(tk.END, "     类型: 裸机系统\n")
                    text_widget.insert(tk.END, "     特性: 无OS开销，最小资源占用\n")
                
                if 'system' in name.lower():
                    text_widget.insert(tk.END, "     架构: R5F + C66x DSP 双核\n")
                    text_widget.insert(tk.END, "     用途: 雷达信号处理应用\n")
                elif 'r5fss0-0' in name.lower():
                    text_widget.insert(tk.END, "     架构: 单核R5F\n")
                    text_widget.insert(tk.END, "     用途: 基础应用和学习\n")
                
                text_widget.insert(tk.END, "\n")
        
        # SBL固件
        if project.recommended_sbl:
            text_widget.insert(tk.END, f"🔧 SBL引导固件 (推荐{len(project.recommended_sbl)}个)\n\n", "subtitle")
            for sbl in project.recommended_sbl[:3]:
                if os.path.exists(sbl['path']):
                    name = os.path.basename(sbl['path'])
                    size = os.path.getsize(sbl['path']) / 1024
                    text_widget.insert(tk.END, f"• {name}\n")
                    text_widget.insert(tk.END, f"  大小: {size:.2f} KB\n")
                    text_widget.insert(tk.END, f"  原因: {sbl['reason']}\n")
                    text_widget.insert(tk.END, f"  来源: {sbl['source']}\n\n")
        
        # 雷达配置
        text_widget.insert(tk.END, "📡 雷达配置\n\n", "subtitle")
        if not project.recommended_radar_cfg:
            reason = self._get_no_radar_cfg_reason(project)
            text_widget.insert(tk.END, f"无雷达配置文件\n\n原因:\n{reason}\n\n")
        
        # 使用建议
        text_widget.insert(tk.END, "💡 使用建议\n\n", "subtitle")
        text_widget.insert(tk.END, "学习路径:\n")
        text_widget.insert(tk.END, "1️⃣ 选择 FreeRTOS 版本开始学习\n")
        text_widget.insert(tk.END, "2️⃣ 理解任务创建和串口通信\n")
        text_widget.insert(tk.END, "3️⃣ 尝试 System 双核版本\n")
        text_widget.insert(tk.END, "4️⃣ 进阶到 mmwave_demo 雷达应用\n\n")
        
        text_widget.insert(tk.END, "适用场景:\n")
        text_widget.insert(tk.END, "• 环境验证: 确认开发环境和硬件正常\n")
        text_widget.insert(tk.END, "• 学习入门: 理解TI SDK基本结构\n")
        text_widget.insert(tk.END, "• 项目起点: 作为自定义应用的基础\n\n")
        
        # 标签配置
        text_widget.tag_config("title", font=("Microsoft YaHei UI", 12, "bold"), foreground="#2c3e50")
        text_widget.tag_config("subtitle", font=("Microsoft YaHei UI", 10, "bold"), foreground="#3498db")
        text_widget.tag_config("line", foreground="#95a5a6")
    
    def _add_mmwave_demo_full_analysis(self, text_widget, project):
        """添加mmwave_demo完整分析"""
        content = """
📊 mmwave_demo 项目完整分析

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

🎯 项目定位

mmwave_demo是TI官方最完整的雷达应用参考项目:
✅ 完整的雷达信号处理链
✅ 多种天线配置支持(2T4R、3T4R、4T4R)
✅ 完善的校准和监控机制
✅ 多种数据输出方式(UART、LVDS)
✅ 电源管理优化
✅ 工厂和运行时校准

可以直接用于产品开发！

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

🏗️ 系统架构

核心处理流程:
1. RF配置 → Chirp参数、帧配置
2. ADC采集 → 4通道同时采样
3. HWA加速 → 距离FFT、多普勒FFT
4. CFAR检测 → 目标识别
5. AOA估计 → 角度测量
6. 数据输出 → UART/LVDS流

关键组件:
• R5F: 主控制器，运行FreeRTOS
• HWA: 硬件加速器(FFT、CFAR)
• RF子系统: 60-64 GHz毫米波
• LVDS: 高速数据流接口

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

🔧 功能模块 (6大模块, 24文件)

1️⃣ 校准系统 (calibrations/)
   • factory_cal: 工厂校准流程
   • mmw_flash_cal: Flash校准数据管理
   • range_phase_bias: 相位偏差测量
   
   作用:
   ✅ 补偿硬件差异
   ✅ 提高测量精度
   ✅ 长期稳定性保证

2️⃣ 信号处理 (dpc/)
   • 数据路径控制(DPC)
   • HWA配置和管理
   • 信号处理链编排
   
   处理流程:
   ADC → 距离FFT → 多普勒FFT → 
   CFAR检测 → AOA估计 → 目标跟踪

3️⃣ 数据流 (lvds_streaming/)
   • LVDS高速数据流
   • 原始ADC数据导出
   • 点云数据输出
   
   应用:
   ✅ 连接DCA1000采集卡
   ✅ 高级算法开发
   ✅ 数据记录和回放

4️⃣ 毫米波控制 (mmwave_control/)
   • Chirp和帧配置
   • 触发控制
   • 中断处理
   • RF监控器管理
   
   功能:
   ✅ 雷达参数配置
   ✅ 实时监控RF状态
   ✅ 异常检测和处理

5️⃣ 电源管理 (power_management/)
   • 低功耗模式
   • 帧间睡眠
   • 动态电源调节
   
   效果:
   ✅ 功耗降低30-50%
   ✅ 延长电池寿命
   ✅ 热管理优化

6️⃣ 命令行接口 (CLI)
   • UART命令解析
   • 配置参数接收
   • 结果输出控制
   
   功能:
   ✅ 接收.cfg文件命令
   ✅ 实时参数调整
   ✅ 调试和诊断

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

🎯 关键技术特性

1️⃣ TDM-MIMO技术:
   • 4发4收时分复用
   • 16个虚拟天线
   • 角度分辨率: ~15°
   • FOV: ±60°

2️⃣ 两级CFAR检测:
   • 距离CFAR: 静止目标
   • 多普勒CFAR: 运动目标
   • 自适应门限
   • 虚警率控制

3️⃣ AOA角度估计:
   • FFT-based方法
   • 64点FFT
   • 精度: ±1-2°
   • 范围: ±60°

4️⃣ 校准系统:
   • 工厂校准(Flash存储)
   • 运行时校准(温度补偿)
   • TX相位校准
   • RX增益校准

5️⃣ RF监控:
   • PLL锁定监控
   • 发射功率监控(8TX)
   • 温度监控
   • 环回测试

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

📊 性能参数 (4T4R配置)

测量范围:
• 距离: 0.25 - 9.0 m
• 速度: ±20.16 m/s
• 角度: ±60° (方位+俯仰)

分辨率:
• 距离: ~5 cm (3 GHz带宽)
• 速度: ~0.16 m/s
• 角度: ~15°

检测性能:
• 最大目标数: 64个/帧
• 帧率: 10 FPS
• 虚警率: 可配置(门限控制)

功耗:
• 活动模式: ~1.2 W
• 低功耗模式: ~0.3 W
• 平均功耗: ~0.6 W (10 FPS)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

🎯 适用场景

🚗 汽车雷达:
   ✅ 车内人员检测
   ✅ 驾驶员监控(DMS)
   ✅ 乘客监控(OMS)
   ✅ 生命体征检测

🏭 工业自动化:
   ✅ 区域入侵检测
   ✅ 人员计数
   ✅ 轨迹跟踪
   ✅ 手势识别

🏠 智能家居:
   ✅ 存在检测
   ✅ 跌倒检测
   ✅ 呼吸心率监测
   ✅ 手势控制

🔒 安防监控:
   ✅ 周界防范
   ✅ 入侵报警
   ✅ 人员统计
   ✅ 异常行为检测

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

💡 开发建议

基于mmwave_demo开发的优势:
✅ 完整参考实现，减少开发时间
✅ 经过充分测试，稳定可靠
✅ 模块化设计，易于定制
✅ 详细注释，易于理解

定制建议:
1. 保留核心模块(DPC、校准)
2. 根据需求调整雷达参数
3. 定制数据处理算法
4. 优化功耗和性能平衡

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

🔗 与HelloWorld的对比

| 对比维度 | HelloWorld | mmwave_demo |
|---------|------------|-------------|
| 项目定位 | 基础入门 | 完整应用 |
| 固件大小 | 43-219 KB | 374 KB |
| 雷达功能 | ❌ 无 | ✅ 完整 |
| 信号处理 | ❌ 无 | ✅ 完整链 |
| 配置文件 | ❌ 无 | ✅ 4种 |
| 适用场景 | 学习框架 | 产品开发 |

学习路径:
HelloWorld验证 → mmwave_demo理解 → 定制开发
"""
        text_widget.insert(tk.END, content)
    
    def _add_generic_full_analysis(self, text_widget, project):
        """动态生成通用项目完整分析"""
        # 标题
        text_widget.insert(tk.END, f"📊 {project.name} 项目分析\n\n", "title")
        text_widget.insert(tk.END, "="*80 + "\n\n", "line")
        
        # 基本信息
        text_widget.insert(tk.END, "📦 基本信息\n\n", "subtitle")
        text_widget.insert(tk.END, f"项目名称: {project.name}\n")
        text_widget.insert(tk.END, f"描述: {project.description}\n")
        text_widget.insert(tk.END, f"SDK来源: {project.sdk_source}\n")
        text_widget.insert(tk.END, f"兼容性: {project.compatibility_reason}\n\n")
        
        # 固件信息
        text_widget.insert(tk.END, "📁 固件文件\n\n", "subtitle")
        if project.app_firmware and os.path.exists(project.app_firmware):
            size = os.path.getsize(project.app_firmware) / 1024
            text_widget.insert(tk.END, f"应用固件: {os.path.basename(project.app_firmware)}\n")
            text_widget.insert(tk.END, f"文件大小: {size:.2f} KB ({os.path.getsize(project.app_firmware):,} 字节)\n")
            text_widget.insert(tk.END, f"完整路径: {project.app_firmware}\n\n")
        
        # 固件变体
        if project.variants:
            text_widget.insert(tk.END, f"🔄 固件变体 ({len(project.variants)}个)\n\n", "subtitle")
            for idx, variant in enumerate(project.variants, 1):
                # variant是FirmwareProject对象，需要访问其app_firmware属性
                if os.path.exists(variant.app_firmware):
                    name = os.path.basename(variant.app_firmware)
                    size = os.path.getsize(variant.app_firmware) / 1024
                    text_widget.insert(tk.END, f"{idx}. {name}\n")
                    text_widget.insert(tk.END, f"   大小: {size:.2f} KB\n\n")
        
        # SBL固件
        if project.recommended_sbl:
            text_widget.insert(tk.END, f"🔧 推荐SBL固件 ({len(project.recommended_sbl)}个)\n\n", "subtitle")
            for idx, sbl in enumerate(project.recommended_sbl, 1):
                if os.path.exists(sbl['path']):
                    name = os.path.basename(sbl['path'])
                    size = os.path.getsize(sbl['path']) / 1024
                    text_widget.insert(tk.END, f"{idx}. {name}\n")
                    text_widget.insert(tk.END, f"   大小: {size:.2f} KB\n")
                    text_widget.insert(tk.END, f"   推荐原因: {sbl['reason']}\n")
                    text_widget.insert(tk.END, f"   来源: {sbl['source']}\n\n")
        
        # 雷达配置
        if project.recommended_radar_cfg:
            text_widget.insert(tk.END, f"📡 雷达配置 ({len(project.recommended_radar_cfg)}个)\n\n", "subtitle")
            for idx, cfg in enumerate(project.recommended_radar_cfg, 1):
                if os.path.exists(cfg['path']):
                    name = os.path.basename(cfg['path'])
                    text_widget.insert(tk.END, f"{idx}. {name}\n")
                    text_widget.insert(tk.END, f"   说明: {cfg.get('reason', '雷达参数配置文件')}\n\n")
        else:
            text_widget.insert(tk.END, "📡 雷达配置\n\n", "subtitle")
            reason = self._get_no_radar_cfg_reason(project)
            text_widget.insert(tk.END, f"无雷达配置文件\n\n{reason}\n\n")
        
        # 项目特征分析
        text_widget.insert(tk.END, "💡 项目特征\n\n", "subtitle")
        
        # 根据固件名称分析
        app_name = os.path.basename(project.app_firmware).lower()
        
        if 'demo' in project.name.lower():
            text_widget.insert(tk.END, "• 项目类型: 演示应用\n")
            text_widget.insert(tk.END, "• 用途: 展示特定功能或技术\n")
        elif 'test' in project.name.lower():
            text_widget.insert(tk.END, "• 项目类型: 测试项目\n")
            text_widget.insert(tk.END, "• 用途: 功能验证和测试\n")
        
        if 'system' in app_name:
            text_widget.insert(tk.END, "• 架构: 多核System固件\n")
            text_widget.insert(tk.END, "• 包含: R5F + C66x/HWA 处理单元\n")
        elif 'r5f' in app_name:
            text_widget.insert(tk.END, "• 架构: 单核R5F固件\n")
        
        if 'freertos' in app_name:
            text_widget.insert(tk.END, "• 操作系统: FreeRTOS\n")
            text_widget.insert(tk.END, "• 特性: 支持多任务调度\n")
        elif 'nortos' in app_name:
            text_widget.insert(tk.END, "• 操作系统: 无(裸机)\n")
            text_widget.insert(tk.END, "• 特性: 最小资源占用\n")
        
        # 标签配置
        text_widget.tag_config("title", font=("Microsoft YaHei UI", 12, "bold"), foreground="#2c3e50")
        text_widget.tag_config("subtitle", font=("Microsoft YaHei UI", 10, "bold"), foreground="#3498db")
        text_widget.tag_config("line", foreground="#95a5a6")


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
