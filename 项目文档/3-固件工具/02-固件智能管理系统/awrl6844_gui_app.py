"""
AWRL6844EVM 固件管理系统 - GUI主程序
专门为AWRL6844评估板设计的固件扫描、筛选、匹配工具
"""

import sys
import os
from pathlib import Path
from typing import List, Dict, Optional

from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QPushButton, QLabel, QLineEdit, QTextEdit, QTreeWidget, QTreeWidgetItem,
    QTabWidget, QGroupBox, QSplitter, QMessageBox, QFileDialog,
    QComboBox, QCheckBox, QSpinBox, QProgressBar, QTableWidget,
    QTableWidgetItem, QHeaderView, QFrame
)
from PyQt6.QtCore import Qt, QThread, pyqtSignal, QSettings
from PyQt6.QtGui import QFont, QColor, QIcon

from awrl6844_firmware_matcher import (
    AWRL6844FirmwareMatcher, FirmwareInfo, SBLInfo, ConfigInfo,
    FirmwareType
)


class ScanThread(QThread):
    """扫描线程"""
    progress = pyqtSignal(int, str)
    finished = pyqtSignal(dict)
    
    def __init__(self, directories: List[str], matcher: AWRL6844FirmwareMatcher):
        super().__init__()
        self.directories = directories
        self.matcher = matcher
        
    def run(self):
        total_stats = {'application': 0, 'sbl': 0, 'config': 0, 'total_files': 0}
        
        for i, directory in enumerate(self.directories):
            self.progress.emit(int((i / len(self.directories)) * 100), 
                             f"正在扫描: {directory}")
            
            stats = self.matcher.scan_directory(directory, recursive=True)
            for key in total_stats:
                total_stats[key] += stats[key]
        
        self.progress.emit(100, "扫描完成")
        self.finished.emit(total_stats)


class AWRL6844GUI(QMainWindow):
    """AWRL6844固件管理系统主窗口"""
    
    def __init__(self):
        super().__init__()
        self.matcher = AWRL6844FirmwareMatcher()
        self.settings = QSettings('TI', 'AWRL6844FirmwareManager')
        
        # 默认扫描目录
        self.scan_directories = [
            r"C:\ti\MMWAVE_L_SDK_06_01_00_01",
            r"C:\ti\radar_toolbox_3_30_00_06"
        ]
        
        # 加载保存的目录
        saved_dirs = self.settings.value('scan_directories', [])
        if saved_dirs:
            self.scan_directories = saved_dirs
        
        self.init_ui()
        self.apply_styles()
        
    def init_ui(self):
        """初始化UI"""
        self.setWindowTitle("AWRL6844EVM 固件智能管理系统 v1.0")
        self.setGeometry(100, 100, 1600, 900)
        
        # 中央部件
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QVBoxLayout(central_widget)
        
        # 顶部控制区
        control_group = self.create_control_panel()
        main_layout.addWidget(control_group)
        
        # 主内容区（选项卡）
        self.tabs = QTabWidget()
        self.tabs.addTab(self.create_scan_tab(), "📡 扫描与管理")
        self.tabs.addTab(self.create_firmware_tab(), "📦 应用固件")
        self.tabs.addTab(self.create_sbl_tab(), "🔧 SBL固件")
        self.tabs.addTab(self.create_config_tab(), "⚙️ 雷达配置")
        self.tabs.addTab(self.create_match_tab(), "🎯 智能匹配")
        
        main_layout.addWidget(self.tabs)
        
        # 底部状态栏
        self.statusBar().showMessage("就绪 - 专为AWRL6844EVM设计")
        
    def create_control_panel(self) -> QGroupBox:
        """创建控制面板"""
        group = QGroupBox("扫描控制")
        layout = QVBoxLayout()
        
        # 目录管理
        dir_layout = QHBoxLayout()
        dir_layout.addWidget(QLabel("扫描目录:"))
        
        self.dir_list_widget = QComboBox()
        self.dir_list_widget.setEditable(True)
        self.dir_list_widget.addItems(self.scan_directories)
        dir_layout.addWidget(self.dir_list_widget, 1)
        
        btn_add_dir = QPushButton("➕ 添加")
        btn_add_dir.clicked.connect(self.add_directory)
        dir_layout.addWidget(btn_add_dir)
        
        btn_remove_dir = QPushButton("➖ 删除")
        btn_remove_dir.clicked.connect(self.remove_directory)
        dir_layout.addWidget(btn_remove_dir)
        
        layout.addLayout(dir_layout)
        
        # 扫描按钮行
        btn_layout = QHBoxLayout()
        
        self.btn_scan = QPushButton("🔍 开始扫描")
        self.btn_scan.setMinimumHeight(40)
        self.btn_scan.clicked.connect(self.start_scan)
        btn_layout.addWidget(self.btn_scan)
        
        btn_clear = QPushButton("🗑️ 清空结果")
        btn_clear.setMinimumHeight(40)
        btn_clear.clicked.connect(self.clear_results)
        btn_layout.addWidget(btn_clear)
        
        layout.addLayout(btn_layout)
        
        # 进度条
        self.progress_bar = QProgressBar()
        self.progress_bar.setVisible(False)
        layout.addWidget(self.progress_bar)
        
        # 统计信息
        stats_layout = QHBoxLayout()
        self.lbl_app_count = QLabel("应用固件: 0")
        self.lbl_sbl_count = QLabel("SBL固件: 0")
        self.lbl_config_count = QLabel("雷达配置: 0")
        
        stats_layout.addWidget(self.lbl_app_count)
        stats_layout.addWidget(QLabel("|"))
        stats_layout.addWidget(self.lbl_sbl_count)
        stats_layout.addWidget(QLabel("|"))
        stats_layout.addWidget(self.lbl_config_count)
        stats_layout.addStretch()
        
        layout.addLayout(stats_layout)
        
        group.setLayout(layout)
        return group
    
    def create_scan_tab(self) -> QWidget:
        """创建扫描管理标签页"""
        widget = QWidget()
        layout = QVBoxLayout()
        
        # 说明文本
        info_text = QTextEdit()
        info_text.setReadOnly(True)
        info_text.setMaximumHeight(120)
        info_text.setHtml("""
        <h3>📖 使用说明</h3>
        <ul>
            <li><b>步骤1:</b> 添加或修改扫描目录（默认已配置TI SDK路径）</li>
            <li><b>步骤2:</b> 点击"开始扫描"按钮，系统将自动识别AWRL6844固件</li>
            <li><b>步骤3:</b> 切换到各个标签页查看扫描结果</li>
            <li><b>步骤4:</b> 使用"智能匹配"功能获取推荐配置</li>
        </ul>
        """)
        layout.addWidget(info_text)
        
        # 目录列表
        dir_group = QGroupBox("当前扫描目录列表")
        dir_layout = QVBoxLayout()
        
        self.dir_table = QTableWidget()
        self.dir_table.setColumnCount(2)
        self.dir_table.setHorizontalHeaderLabels(["目录路径", "状态"])
        self.dir_table.horizontalHeader().setStretchLastSection(False)
        self.dir_table.horizontalHeader().setSectionResizeMode(0, QHeaderView.ResizeMode.Stretch)
        self.update_directory_table()
        
        dir_layout.addWidget(self.dir_table)
        dir_group.setLayout(dir_layout)
        layout.addWidget(dir_group)
        
        widget.setLayout(layout)
        return widget
    
    def create_firmware_tab(self) -> QWidget:
        """创建应用固件标签页"""
        widget = QWidget()
        layout = QVBoxLayout()
        
        # 筛选区
        filter_group = QGroupBox("筛选条件")
        filter_layout = QHBoxLayout()
        
        filter_layout.addWidget(QLabel("类别:"))
        self.fw_category_combo = QComboBox()
        self.fw_category_combo.addItem("全部")
        self.fw_category_combo.currentTextChanged.connect(self.filter_firmwares)
        filter_layout.addWidget(self.fw_category_combo)
        
        filter_layout.addWidget(QLabel("处理器:"))
        self.fw_processor_combo = QComboBox()
        self.fw_processor_combo.addItem("全部")
        self.fw_processor_combo.currentTextChanged.connect(self.filter_firmwares)
        filter_layout.addWidget(self.fw_processor_combo)
        
        filter_layout.addWidget(QLabel("搜索:"))
        self.fw_search_input = QLineEdit()
        self.fw_search_input.setPlaceholderText("输入文件名或路径关键词...")
        self.fw_search_input.textChanged.connect(self.filter_firmwares)
        filter_layout.addWidget(self.fw_search_input, 1)
        
        filter_group.setLayout(filter_layout)
        layout.addWidget(filter_group)
        
        # 固件列表
        self.firmware_table = QTableWidget()
        self.firmware_table.setColumnCount(6)
        self.firmware_table.setHorizontalHeaderLabels([
            "文件名", "类别", "子类别", "处理器", "版本", "大小(KB)"
        ])
        self.firmware_table.horizontalHeader().setSectionResizeMode(0, QHeaderView.ResizeMode.Stretch)
        self.firmware_table.itemSelectionChanged.connect(self.on_firmware_selected)
        layout.addWidget(self.firmware_table)
        
        # 详细信息
        self.firmware_detail = QTextEdit()
        self.firmware_detail.setReadOnly(True)
        self.firmware_detail.setMaximumHeight(200)
        layout.addWidget(self.firmware_detail)
        
        widget.setLayout(layout)
        return widget
    
    def create_sbl_tab(self) -> QWidget:
        """创建SBL固件标签页"""
        widget = QWidget()
        layout = QVBoxLayout()
        
        # 说明
        info_label = QLabel("""
        <b>🔧 SBL固件说明:</b><br>
        SBL (Secondary Bootloader) 是芯片启动的第一级程序，负责从Flash加载应用固件。<br>
        <b>推荐使用:</b> 标准版SBL，功能完整，适用于绝大多数应用场景。
        """)
        info_label.setWordWrap(True)
        layout.addWidget(info_label)
        
        # SBL列表
        self.sbl_table = QTableWidget()
        self.sbl_table.setColumnCount(5)
        self.sbl_table.setHorizontalHeaderLabels([
            "文件名", "变体", "Flash地址", "大小", "说明"
        ])
        self.sbl_table.horizontalHeader().setSectionResizeMode(4, QHeaderView.ResizeMode.Stretch)
        self.sbl_table.itemSelectionChanged.connect(self.on_sbl_selected)
        layout.addWidget(self.sbl_table)
        
        # 详细信息
        self.sbl_detail = QTextEdit()
        self.sbl_detail.setReadOnly(True)
        self.sbl_detail.setMaximumHeight(150)
        layout.addWidget(self.sbl_detail)
        
        widget.setLayout(layout)
        return widget
    
    def create_config_tab(self) -> QWidget:
        """创建雷达配置标签页"""
        widget = QWidget()
        layout = QVBoxLayout()
        
        # 筛选区
        filter_group = QGroupBox("筛选条件")
        filter_layout = QHBoxLayout()
        
        filter_layout.addWidget(QLabel("应用场景:"))
        self.cfg_app_combo = QComboBox()
        self.cfg_app_combo.addItem("全部")
        self.cfg_app_combo.currentTextChanged.connect(self.filter_configs)
        filter_layout.addWidget(self.cfg_app_combo)
        
        filter_layout.addWidget(QLabel("模式:"))
        self.cfg_mode_combo = QComboBox()
        self.cfg_mode_combo.addItems(["全部", "2D", "3D", "TDM时分复用"])
        self.cfg_mode_combo.currentTextChanged.connect(self.filter_configs)
        filter_layout.addWidget(self.cfg_mode_combo)
        
        filter_layout.addWidget(QLabel("功耗:"))
        self.cfg_power_combo = QComboBox()
        self.cfg_power_combo.addItems(["全部", "低功耗", "标准功耗", "满功率"])
        self.cfg_power_combo.currentTextChanged.connect(self.filter_configs)
        filter_layout.addWidget(self.cfg_power_combo)
        
        filter_layout.addWidget(QLabel("搜索:"))
        self.cfg_search_input = QLineEdit()
        self.cfg_search_input.setPlaceholderText("输入关键词...")
        self.cfg_search_input.textChanged.connect(self.filter_configs)
        filter_layout.addWidget(self.cfg_search_input, 1)
        
        filter_group.setLayout(filter_layout)
        layout.addWidget(filter_group)
        
        # 配置文件列表
        self.config_table = QTableWidget()
        self.config_table.setColumnCount(7)
        self.config_table.setHorizontalHeaderLabels([
            "文件名", "应用", "TX/RX", "距离(m)", "模式", "功耗", "说明"
        ])
        self.config_table.horizontalHeader().setSectionResizeMode(6, QHeaderView.ResizeMode.Stretch)
        self.config_table.itemSelectionChanged.connect(self.on_config_selected)
        layout.addWidget(self.config_table)
        
        # 详细信息
        self.config_detail = QTextEdit()
        self.config_detail.setReadOnly(True)
        self.config_detail.setMaximumHeight(150)
        layout.addWidget(self.config_detail)
        
        widget.setLayout(layout)
        return widget
    
    def create_match_tab(self) -> QWidget:
        """创建智能匹配标签页"""
        widget = QWidget()
        layout = QVBoxLayout()
        
        # 说明
        info_label = QLabel("""
        <b>🎯 智能匹配功能:</b><br>
        选择一个应用固件，系统将自动推荐最匹配的SBL固件和雷达配置文件，并显示匹配度评分。
        """)
        info_label.setWordWrap(True)
        layout.addWidget(info_label)
        
        # 分割器
        splitter = QSplitter(Qt.Orientation.Vertical)
        
        # 固件选择
        fw_group = QGroupBox("步骤1: 选择应用固件")
        fw_layout = QVBoxLayout()
        
        # 添加搜索栏
        search_layout = QHBoxLayout()
        search_layout.addWidget(QLabel("🔍 搜索固件:"))
        self.match_search_input = QLineEdit()
        self.match_search_input.setPlaceholderText("输入文件名、类别或关键词快速筛选...")
        self.match_search_input.textChanged.connect(self.filter_match_firmwares)
        search_layout.addWidget(self.match_search_input, 1)
        
        btn_clear_search = QPushButton("✖ 清空")
        btn_clear_search.setMaximumWidth(80)
        btn_clear_search.clicked.connect(lambda: self.match_search_input.clear())
        search_layout.addWidget(btn_clear_search)
        
        fw_layout.addLayout(search_layout)
        
        # 固件列表
        self.match_fw_list = QTableWidget()
        self.match_fw_list.setColumnCount(3)
        self.match_fw_list.setHorizontalHeaderLabels(["文件名", "类别", "说明"])
        self.match_fw_list.horizontalHeader().setSectionResizeMode(2, QHeaderView.ResizeMode.Stretch)
        self.match_fw_list.itemSelectionChanged.connect(self.on_match_firmware_selected)
        fw_layout.addWidget(self.match_fw_list)
        fw_group.setLayout(fw_layout)
        splitter.addWidget(fw_group)
        
        # 匹配结果
        result_group = QGroupBox("步骤2: 查看推荐配置")
        result_layout = QVBoxLayout()
        
        # SBL推荐
        sbl_label = QLabel("<b>推荐SBL固件 (Top 3):</b>")
        result_layout.addWidget(sbl_label)
        self.match_sbl_table = QTableWidget()
        self.match_sbl_table.setColumnCount(4)
        self.match_sbl_table.setHorizontalHeaderLabels(["文件名", "变体", "匹配度", "路径"])
        self.match_sbl_table.horizontalHeader().setSectionResizeMode(3, QHeaderView.ResizeMode.Stretch)
        result_layout.addWidget(self.match_sbl_table)
        
        # 配置文件推荐
        cfg_label = QLabel("<b>推荐雷达配置文件 (Top 5):</b>")
        result_layout.addWidget(cfg_label)
        self.match_cfg_table = QTableWidget()
        self.match_cfg_table.setColumnCount(5)
        self.match_cfg_table.setHorizontalHeaderLabels(["文件名", "应用", "参数", "匹配度", "路径"])
        self.match_cfg_table.horizontalHeader().setSectionResizeMode(4, QHeaderView.ResizeMode.Stretch)
        result_layout.addWidget(self.match_cfg_table)
        
        result_group.setLayout(result_layout)
        splitter.addWidget(result_group)
        
        layout.addWidget(splitter)
        
        widget.setLayout(layout)
        return widget
    
    def apply_styles(self):
        """应用样式"""
        self.setStyleSheet("""
            QMainWindow {
                background-color: #f5f5f5;
            }
            QGroupBox {
                font-weight: bold;
                border: 2px solid #cccccc;
                border-radius: 5px;
                margin-top: 10px;
                padding-top: 10px;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 10px;
                padding: 0 5px 0 5px;
            }
            QPushButton {
                background-color: #0078d4;
                color: white;
                border: none;
                padding: 8px 16px;
                border-radius: 4px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #106ebe;
            }
            QPushButton:pressed {
                background-color: #005a9e;
            }
            QTableWidget {
                gridline-color: #d0d0d0;
                background-color: white;
                alternate-background-color: #f9f9f9;
            }
            QTableWidget::item:selected {
                background-color: #0078d4;
                color: white;
            }
            QHeaderView::section {
                background-color: #e0e0e0;
                padding: 6px;
                border: 1px solid #c0c0c0;
                font-weight: bold;
            }
            QLineEdit, QComboBox {
                padding: 6px;
                border: 1px solid #cccccc;
                border-radius: 3px;
            }
            QTextEdit {
                border: 1px solid #cccccc;
                border-radius: 3px;
                background-color: white;
            }
        """)
    
    def add_directory(self):
        """添加扫描目录"""
        directory = QFileDialog.getExistingDirectory(self, "选择扫描目录")
        if directory and directory not in self.scan_directories:
            self.scan_directories.append(directory)
            self.dir_list_widget.addItem(directory)
            self.update_directory_table()
            self.settings.setValue('scan_directories', self.scan_directories)
    
    def remove_directory(self):
        """删除扫描目录"""
        current_dir = self.dir_list_widget.currentText()
        if current_dir in self.scan_directories:
            self.scan_directories.remove(current_dir)
            self.dir_list_widget.removeItem(self.dir_list_widget.currentIndex())
            self.update_directory_table()
            self.settings.setValue('scan_directories', self.scan_directories)
    
    def update_directory_table(self):
        """更新目录表格"""
        self.dir_table.setRowCount(len(self.scan_directories))
        for i, directory in enumerate(self.scan_directories):
            self.dir_table.setItem(i, 0, QTableWidgetItem(directory))
            status = "✅ 存在" if os.path.exists(directory) else "❌ 不存在"
            self.dir_table.setItem(i, 1, QTableWidgetItem(status))
    
    def start_scan(self):
        """开始扫描"""
        if not self.scan_directories:
            QMessageBox.warning(self, "警告", "请先添加扫描目录！")
            return
        
        # 清空之前的结果
        self.matcher.clear_results()
        
        # 显示进度条
        self.progress_bar.setVisible(True)
        self.progress_bar.setValue(0)
        self.btn_scan.setEnabled(False)
        
        # 创建并启动扫描线程
        self.scan_thread = ScanThread(self.scan_directories, self.matcher)
        self.scan_thread.progress.connect(self.update_progress)
        self.scan_thread.finished.connect(self.scan_finished)
        self.scan_thread.start()
    
    def update_progress(self, value: int, message: str):
        """更新进度"""
        self.progress_bar.setValue(value)
        self.statusBar().showMessage(message)
    
    def scan_finished(self, stats: Dict):
        """扫描完成"""
        self.progress_bar.setVisible(False)
        self.btn_scan.setEnabled(True)
        
        # 更新统计
        self.lbl_app_count.setText(f"应用固件: {stats['application']}")
        self.lbl_sbl_count.setText(f"SBL固件: {stats['sbl']}")
        self.lbl_config_count.setText(f"雷达配置: {stats['config']}")
        
        # 更新各个表格
        self.update_firmware_table()
        self.update_sbl_table()
        self.update_config_table()
        self.update_match_firmware_list()
        
        # 更新筛选选项
        self.update_filter_options()
        
        self.statusBar().showMessage(
            f"扫描完成 - 找到 {stats['application']} 个应用固件, "
            f"{stats['sbl']} 个SBL固件, {stats['config']} 个配置文件"
        )
        
        QMessageBox.information(self, "扫描完成", 
            f"扫描完成！\n\n"
            f"应用固件: {stats['application']} 个\n"
            f"SBL固件: {stats['sbl']} 个\n"
            f"雷达配置: {stats['config']} 个\n"
            f"总文件数: {stats['total_files']} 个"
        )
    
    def update_firmware_table(self, filter_items: Optional[List[FirmwareInfo]] = None):
        """更新固件表格"""
        items = filter_items if filter_items is not None else self.matcher.application_firmwares
        
        self.firmware_table.setRowCount(len(items))
        for i, fw in enumerate(items):
            self.firmware_table.setItem(i, 0, QTableWidgetItem(fw.filename))
            self.firmware_table.setItem(i, 1, QTableWidgetItem(fw.category))
            self.firmware_table.setItem(i, 2, QTableWidgetItem(fw.subcategory))
            self.firmware_table.setItem(i, 3, QTableWidgetItem(fw.processor))
            self.firmware_table.setItem(i, 4, QTableWidgetItem(fw.version))
            size_kb = fw.size / 1024 if fw.size > 0 else 0
            self.firmware_table.setItem(i, 5, QTableWidgetItem(f"{size_kb:.1f}"))
            
            # 存储完整对象
            self.firmware_table.item(i, 0).setData(Qt.ItemDataRole.UserRole, fw)
    
    def update_sbl_table(self):
        """更新SBL表格"""
        self.sbl_table.setRowCount(len(self.matcher.sbl_firmwares))
        for i, sbl in enumerate(self.matcher.sbl_firmwares):
            self.sbl_table.setItem(i, 0, QTableWidgetItem(sbl.filename))
            self.sbl_table.setItem(i, 1, QTableWidgetItem(sbl.variant))
            self.sbl_table.setItem(i, 2, QTableWidgetItem(sbl.flash_address))
            size_kb = sbl.size / 1024 if sbl.size > 0 else 0
            self.sbl_table.setItem(i, 3, QTableWidgetItem(f"{size_kb:.1f} KB"))
            self.sbl_table.setItem(i, 4, QTableWidgetItem(sbl.description[:50] + "..."))
            
            # 存储完整对象
            self.sbl_table.item(i, 0).setData(Qt.ItemDataRole.UserRole, sbl)
    
    def update_config_table(self, filter_items: Optional[List[ConfigInfo]] = None):
        """更新配置表格"""
        items = filter_items if filter_items is not None else self.matcher.config_files
        
        self.config_table.setRowCount(len(items))
        for i, cfg in enumerate(items):
            self.config_table.setItem(i, 0, QTableWidgetItem(cfg.filename))
            self.config_table.setItem(i, 1, QTableWidgetItem(cfg.application))
            
            tx_rx = f"{cfg.tx_channels}TX/{cfg.rx_channels}RX" if cfg.tx_channels > 0 else ""
            self.config_table.setItem(i, 2, QTableWidgetItem(tx_rx))
            
            range_str = str(cfg.range_m) if cfg.range_m > 0 else ""
            self.config_table.setItem(i, 3, QTableWidgetItem(range_str))
            
            self.config_table.setItem(i, 4, QTableWidgetItem(cfg.mode))
            self.config_table.setItem(i, 5, QTableWidgetItem(cfg.power_mode))
            self.config_table.setItem(i, 6, QTableWidgetItem(cfg.description[:50] + "..."))
            
            # 存储完整对象
            self.config_table.item(i, 0).setData(Qt.ItemDataRole.UserRole, cfg)
    
    def update_match_firmware_list(self, filter_items: Optional[List[FirmwareInfo]] = None):
        """更新匹配固件列表"""
        items = filter_items if filter_items is not None else self.matcher.application_firmwares
        
        self.match_fw_list.setRowCount(len(items))
        for i, fw in enumerate(items):
            self.match_fw_list.setItem(i, 0, QTableWidgetItem(fw.filename))
            self.match_fw_list.setItem(i, 1, QTableWidgetItem(fw.category))
            self.match_fw_list.setItem(i, 2, QTableWidgetItem(fw.description))
            
            # 存储完整对象
            self.match_fw_list.item(i, 0).setData(Qt.ItemDataRole.UserRole, fw)
    
    def update_filter_options(self):
        """更新筛选选项"""
        # 应用固件类别
        categories = set(fw.category for fw in self.matcher.application_firmwares if fw.category)
        self.fw_category_combo.clear()
        self.fw_category_combo.addItem("全部")
        self.fw_category_combo.addItems(sorted(categories))
        
        # 处理器
        processors = set(fw.processor for fw in self.matcher.application_firmwares if fw.processor)
        self.fw_processor_combo.clear()
        self.fw_processor_combo.addItem("全部")
        self.fw_processor_combo.addItems(sorted(processors))
        
        # 配置文件应用场景
        applications = set(cfg.application for cfg in self.matcher.config_files if cfg.application)
        self.cfg_app_combo.clear()
        self.cfg_app_combo.addItem("全部")
        self.cfg_app_combo.addItems(sorted(applications))
    
    def filter_firmwares(self):
        """筛选固件"""
        category = self.fw_category_combo.currentText()
        processor = self.fw_processor_combo.currentText()
        search_text = self.fw_search_input.text().lower()
        
        filtered = []
        for fw in self.matcher.application_firmwares:
            # 类别筛选
            if category != "全部" and fw.category != category:
                continue
            
            # 处理器筛选
            if processor != "全部" and fw.processor != processor:
                continue
            
            # 搜索筛选
            if search_text:
                if (search_text not in fw.filename.lower() and
                    search_text not in fw.path.lower() and
                    search_text not in fw.description.lower()):
                    continue
            
            filtered.append(fw)
        
        self.update_firmware_table(filtered)
        self.statusBar().showMessage(f"筛选结果: {len(filtered)} 个固件")
    
    def filter_configs(self):
        """筛选配置文件"""
        application = self.cfg_app_combo.currentText()
        mode = self.cfg_mode_combo.currentText()
        power = self.cfg_power_combo.currentText()
        search_text = self.cfg_search_input.text().lower()
        
        filtered = []
        for cfg in self.matcher.config_files:
            # 应用场景筛选
            if application != "全部" and cfg.application != application:
                continue
            
            # 模式筛选
            if mode != "全部" and cfg.mode != mode:
                continue
            
            # 功耗筛选
            if power != "全部" and cfg.power_mode != power:
                continue
            
            # 搜索筛选
            if search_text:
                if (search_text not in cfg.filename.lower() and
                    search_text not in cfg.path.lower() and
                    search_text not in cfg.description.lower()):
                    continue
            
            filtered.append(cfg)
        
        self.update_config_table(filtered)
        self.statusBar().showMessage(f"筛选结果: {len(filtered)} 个配置文件")
    
    def filter_match_firmwares(self):
        """筛选匹配标签页的固件列表"""
        search_text = self.match_search_input.text().lower()
        
        if not search_text:
            # 如果搜索框为空，显示所有固件
            self.update_match_firmware_list()
            self.statusBar().showMessage(f"显示全部 {len(self.matcher.application_firmwares)} 个固件")
            return
        
        filtered = []
        for fw in self.matcher.application_firmwares:
            # 搜索文件名、类别、子类别、描述、路径
            if (search_text in fw.filename.lower() or
                search_text in fw.category.lower() or
                search_text in fw.subcategory.lower() or
                search_text in fw.description.lower() or
                search_text in fw.path.lower()):
                filtered.append(fw)
        
        self.update_match_firmware_list(filtered)
        self.statusBar().showMessage(f"搜索结果: {len(filtered)} 个固件")
    
    def on_firmware_selected(self):
        """固件被选中"""
        selected_items = self.firmware_table.selectedItems()
        if not selected_items:
            return
        
        fw = selected_items[0].data(Qt.ItemDataRole.UserRole)
        if fw:
            self.show_firmware_detail(fw)
    
    def on_sbl_selected(self):
        """SBL被选中"""
        selected_items = self.sbl_table.selectedItems()
        if not selected_items:
            return
        
        sbl = selected_items[0].data(Qt.ItemDataRole.UserRole)
        if sbl:
            self.show_sbl_detail(sbl)
    
    def on_config_selected(self):
        """配置文件被选中"""
        selected_items = self.config_table.selectedItems()
        if not selected_items:
            return
        
        cfg = selected_items[0].data(Qt.ItemDataRole.UserRole)
        if cfg:
            self.show_config_detail(cfg)
    
    def on_match_firmware_selected(self):
        """匹配固件被选中"""
        selected_items = self.match_fw_list.selectedItems()
        if not selected_items:
            return
        
        fw = selected_items[0].data(Qt.ItemDataRole.UserRole)
        if fw:
            self.show_match_results(fw)
    
    def show_firmware_detail(self, fw: FirmwareInfo):
        """显示固件详细信息"""
        html = f"""
        <h3>📦 {fw.filename}</h3>
        <table style="width:100%; border-collapse: collapse;">
            <tr><td style="padding:5px;"><b>完整路径:</b></td><td style="padding:5px;">{fw.path}</td></tr>
            <tr><td style="padding:5px;"><b>类别:</b></td><td style="padding:5px;">{fw.category}</td></tr>
            <tr><td style="padding:5px;"><b>子类别:</b></td><td style="padding:5px;">{fw.subcategory}</td></tr>
            <tr><td style="padding:5px;"><b>硬件平台:</b></td><td style="padding:5px;">{fw.platform}</td></tr>
            <tr><td style="padding:5px;"><b>处理器配置:</b></td><td style="padding:5px;">{fw.processor}</td></tr>
            <tr><td style="padding:5px;"><b>编译器:</b></td><td style="padding:5px;">{fw.compiler}</td></tr>
            <tr><td style="padding:5px;"><b>版本:</b></td><td style="padding:5px;">{fw.version}</td></tr>
            <tr><td style="padding:5px;"><b>文件大小:</b></td><td style="padding:5px;">{fw.size / 1024:.1f} KB</td></tr>
            <tr><td style="padding:5px;"><b>说明:</b></td><td style="padding:5px;">{fw.description}</td></tr>
        </table>
        """
        self.firmware_detail.setHtml(html)
    
    def show_sbl_detail(self, sbl: SBLInfo):
        """显示SBL详细信息"""
        html = f"""
        <h3>🔧 {sbl.filename}</h3>
        <table style="width:100%; border-collapse: collapse;">
            <tr><td style="padding:5px;"><b>完整路径:</b></td><td style="padding:5px;">{sbl.path}</td></tr>
            <tr><td style="padding:5px;"><b>变体类型:</b></td><td style="padding:5px;">{sbl.variant}</td></tr>
            <tr><td style="padding:5px;"><b>Flash地址:</b></td><td style="padding:5px;">{sbl.flash_address}</td></tr>
            <tr><td style="padding:5px;"><b>Flash大小:</b></td><td style="padding:5px;">{sbl.flash_size}</td></tr>
            <tr><td style="padding:5px;"><b>文件大小:</b></td><td style="padding:5px;">{sbl.size / 1024:.1f} KB</td></tr>
            <tr><td style="padding:5px;"><b>功能说明:</b></td><td style="padding:5px;">{sbl.description}</td></tr>
        </table>
        """
        self.sbl_detail.setHtml(html)
    
    def show_config_detail(self, cfg: ConfigInfo):
        """显示配置文件详细信息"""
        html = f"""
        <h3>⚙️ {cfg.filename}</h3>
        <table style="width:100%; border-collapse: collapse;">
            <tr><td style="padding:5px;"><b>完整路径:</b></td><td style="padding:5px;">{cfg.path}</td></tr>
            <tr><td style="padding:5px;"><b>应用场景:</b></td><td style="padding:5px;">{cfg.application}</td></tr>
            <tr><td style="padding:5px;"><b>TX通道:</b></td><td style="padding:5px;">{cfg.tx_channels if cfg.tx_channels > 0 else 'N/A'}</td></tr>
            <tr><td style="padding:5px;"><b>RX通道:</b></td><td style="padding:5px;">{cfg.rx_channels if cfg.rx_channels > 0 else 'N/A'}</td></tr>
            <tr><td style="padding:5px;"><b>检测距离:</b></td><td style="padding:5px;">{cfg.range_m}m {' ' if cfg.range_m > 0 else 'N/A'}</td></tr>
            <tr><td style="padding:5px;"><b>工作模式:</b></td><td style="padding:5px;">{cfg.mode}</td></tr>
            <tr><td style="padding:5px;"><b>功耗模式:</b></td><td style="padding:5px;">{cfg.power_mode}</td></tr>
            <tr><td style="padding:5px;"><b>带宽模式:</b></td><td style="padding:5px;">{cfg.bandwidth}</td></tr>
            <tr><td style="padding:5px;"><b>封装类型:</b></td><td style="padding:5px;">{cfg.package_type}</td></tr>
            <tr><td style="padding:5px;"><b>详细说明:</b></td><td style="padding:5px;">{cfg.description}</td></tr>
        </table>
        """
        self.config_detail.setHtml(html)
    
    def show_match_results(self, fw: FirmwareInfo):
        """显示匹配结果"""
        # 匹配SBL
        sbl_matches = self.matcher.match_sbl_for_firmware(fw)
        self.match_sbl_table.setRowCount(min(3, len(sbl_matches)))
        for i, (sbl, score) in enumerate(sbl_matches[:3]):
            self.match_sbl_table.setItem(i, 0, QTableWidgetItem(sbl.filename))
            self.match_sbl_table.setItem(i, 1, QTableWidgetItem(sbl.variant))
            self.match_sbl_table.setItem(i, 2, QTableWidgetItem(f"{score:.0f}%"))
            self.match_sbl_table.setItem(i, 3, QTableWidgetItem(sbl.path))
            
            # 高亮最佳匹配
            if i == 0:
                for j in range(4):
                    self.match_sbl_table.item(i, j).setBackground(QColor(200, 255, 200))
        
        # 匹配配置文件
        cfg_matches = self.matcher.match_configs_for_firmware(fw)
        self.match_cfg_table.setRowCount(min(5, len(cfg_matches)))
        for i, (cfg, score) in enumerate(cfg_matches[:5]):
            self.match_cfg_table.setItem(i, 0, QTableWidgetItem(cfg.filename))
            self.match_cfg_table.setItem(i, 1, QTableWidgetItem(cfg.application))
            
            params = []
            if cfg.tx_channels > 0:
                params.append(f"{cfg.tx_channels}TX/{cfg.rx_channels}RX")
            if cfg.range_m > 0:
                params.append(f"{cfg.range_m}m")
            if cfg.mode:
                params.append(cfg.mode)
            self.match_cfg_table.setItem(i, 2, QTableWidgetItem(" | ".join(params)))
            
            self.match_cfg_table.setItem(i, 3, QTableWidgetItem(f"{score:.0f}%"))
            self.match_cfg_table.setItem(i, 4, QTableWidgetItem(cfg.path))
            
            # 高亮最佳匹配
            if i == 0:
                for j in range(5):
                    self.match_cfg_table.item(i, j).setBackground(QColor(200, 255, 200))
        
        self.statusBar().showMessage(
            f"为 {fw.filename} 找到 {len(sbl_matches)} 个SBL匹配, {len(cfg_matches)} 个配置匹配"
        )
    
    def clear_results(self):
        """清空结果"""
        reply = QMessageBox.question(
            self, '确认', '确定要清空所有扫描结果吗?',
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
        )
        
        if reply == QMessageBox.StandardButton.Yes:
            self.matcher.clear_results()
            self.firmware_table.setRowCount(0)
            self.sbl_table.setRowCount(0)
            self.config_table.setRowCount(0)
            self.match_fw_list.setRowCount(0)
            self.match_sbl_table.setRowCount(0)
            self.match_cfg_table.setRowCount(0)
            
            self.lbl_app_count.setText("应用固件: 0")
            self.lbl_sbl_count.setText("SBL固件: 0")
            self.lbl_config_count.setText("雷达配置: 0")
            
            self.statusBar().showMessage("已清空所有结果")


def main():
    app = QApplication(sys.argv)
    app.setApplicationName("AWRL6844 固件管理系统")
    app.setOrganizationName("TI")
    
    window = AWRL6844GUI()
    window.show()
    
    sys.exit(app.exec())


if __name__ == '__main__':
    main()
