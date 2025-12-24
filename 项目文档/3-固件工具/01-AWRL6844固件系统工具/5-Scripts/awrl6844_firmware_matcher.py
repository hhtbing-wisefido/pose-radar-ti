"""
AWRL6844EVM 固件智能匹配器
功能：扫描、筛选、匹配应用固件、SBL、雷达配置文件
"""

import os
import re
from pathlib import Path
from typing import List, Dict, Set, Tuple
from dataclasses import dataclass, field
from enum import Enum


class FirmwareType(Enum):
    """固件类型枚举"""
    APPLICATION = "应用固件"
    SBL = "SBL固件"
    CONFIG = "雷达配置文件"


class ConfigLevel(Enum):
    """配置文件层级"""
    FUNCTION_CATEGORY = "功能类别"
    SPECIFIC_FUNCTION = "具体功能"
    HARDWARE_PLATFORM = "硬件平台"
    PROCESSOR_OS = "处理器_操作系统"
    COMPILER = "编译器"
    DEMO_NAME = "演示名称"
    APPLICATION_SCENE = "应用场景"


@dataclass
class FirmwareInfo:
    """固件信息"""
    path: str
    filename: str
    type: FirmwareType
    chip_series: str = "xWRL68xx"
    board: str = "AWRL6844EVM"
    description: str = ""
    category: str = ""
    subcategory: str = ""
    platform: str = ""
    processor: str = ""
    compiler: str = ""
    version: str = ""
    size: int = 0
    matched_sbl: List[str] = field(default_factory=list)
    matched_configs: List[str] = field(default_factory=list)
    compatibility_score: float = 0.0


@dataclass
class SBLInfo:
    """SBL固件信息"""
    path: str
    filename: str
    variant: str = "标准版"  # 标准版/轻量版/镜像选择
    description: str = ""
    size: int = 0
    flash_address: str = "0x00000000"
    flash_size: str = "264KB"


@dataclass
class ConfigInfo:
    """雷达配置文件信息"""
    path: str
    filename: str
    application: str = ""
    description: str = ""
    tx_channels: int = 0
    rx_channels: int = 0
    range_m: int = 0
    mode: str = ""  # 2D/3D/TDM/DDM
    power_mode: str = ""  # 低功耗/标准/满功率
    bandwidth: str = ""  # 低带宽/标准/全带宽
    package_type: str = ""  # AOP/ISK/ODS
    compatibility_score: float = 0.0


class AWRL6844FirmwareMatcher:
    """AWRL6844固件智能匹配器"""
    
    # AWRL6844匹配规则
    AWRL6844_PATTERNS = {
        'path': [
            r'xwrL684x[-_]evm',  # 官方平台标识
            r'AWRL6844',
            r'6844',
        ],
        'filename': [
            r'xWRL6844',
            r'_6844[_\.]',
            r'L6844',
        ]
    }
    
    # 排除规则
    EXCLUDE_PATTERNS = [
        r'xwrl1432', r'L1432', r'xwrl6432', r'L6432',
        r'awr2944', r'awr2544', r'awr29xx', r'iwrl6432',
        r'1432', r'6432', r'2944', r'2544',
    ]
    
    # SBL识别规则
    SBL_PATTERNS = {
        'path': [r'/boot/sbl', r'/SBL_'],
        'filename': [r'^sbl[_\.]', r'sbl_lite', r'sbl_image']
    }
    
    # 雷达配置文件识别规则
    CONFIG_PATTERNS = {
        'path': [r'chirp_configs', r'config_file'],
        'extension': ['.cfg'],
        'exclude_names': ['syscfg', 'rtos', 'ti_', 'board_']
    }
    
    def __init__(self):
        self.application_firmwares: List[FirmwareInfo] = []
        self.sbl_firmwares: List[SBLInfo] = []
        self.config_files: List[ConfigInfo] = []
        
    def scan_directory(self, directory: str, recursive: bool = True) -> Dict[str, int]:
        """
        扫描目录，识别所有固件和配置文件
        
        Args:
            directory: 要扫描的目录路径
            recursive: 是否递归扫描子目录
            
        Returns:
            扫描统计信息
        """
        stats = {
            'application': 0,
            'sbl': 0,
            'config': 0,
            'total_files': 0
        }
        
        if not os.path.exists(directory):
            print(f"⚠️ 目录不存在: {directory}")
            return stats
        
        directory = Path(directory)
        
        # 扫描固件文件
        pattern = '**/*.appimage' if recursive else '*.appimage'
        for file_path in directory.glob(pattern):
            stats['total_files'] += 1
            
            # 判断文件类型
            if self._is_sbl_firmware(str(file_path)):
                sbl_info = self._parse_sbl_firmware(str(file_path))
                if sbl_info:
                    self.sbl_firmwares.append(sbl_info)
                    stats['sbl'] += 1
            elif self._is_awrl6844_firmware(str(file_path)):
                fw_info = self._parse_application_firmware(str(file_path))
                if fw_info:
                    self.application_firmwares.append(fw_info)
                    stats['application'] += 1
        
        # 扫描配置文件
        for file_path in directory.glob('**/*.cfg' if recursive else '*.cfg'):
            stats['total_files'] += 1
            
            if self._is_radar_config(str(file_path)):
                cfg_info = self._parse_config_file(str(file_path))
                if cfg_info:
                    self.config_files.append(cfg_info)
                    stats['config'] += 1
        
        return stats
    
    def _is_awrl6844_firmware(self, file_path: str) -> bool:
        """判断是否为AWRL6844固件"""
        path_lower = file_path.lower()
        filename = os.path.basename(file_path).lower()
        
        # 排除规则检查
        for pattern in self.EXCLUDE_PATTERNS:
            if re.search(pattern, path_lower, re.IGNORECASE):
                return False
        
        # 路径匹配
        for pattern in self.AWRL6844_PATTERNS['path']:
            if re.search(pattern, path_lower, re.IGNORECASE):
                return True
        
        # 文件名匹配
        for pattern in self.AWRL6844_PATTERNS['filename']:
            if re.search(pattern, filename, re.IGNORECASE):
                return True
        
        return False
    
    def _is_sbl_firmware(self, file_path: str) -> bool:
        """判断是否为SBL固件"""
        path_lower = file_path.lower()
        filename = os.path.basename(file_path).lower()
        
        # 路径匹配
        for pattern in self.SBL_PATTERNS['path']:
            if re.search(pattern, path_lower, re.IGNORECASE):
                return True
        
        # 文件名匹配
        for pattern in self.SBL_PATTERNS['filename']:
            if re.search(pattern, filename, re.IGNORECASE):
                return True
        
        return False
    
    def _is_radar_config(self, file_path: str) -> bool:
        """判断是否为雷达配置文件"""
        path_lower = file_path.lower()
        filename = os.path.basename(file_path).lower()
        
        # 必须是.cfg文件
        if not filename.endswith('.cfg'):
            return False
        
        # 排除系统配置文件
        for exclude_name in self.CONFIG_PATTERNS['exclude_names']:
            if exclude_name in filename:
                return False
        
        # 路径匹配
        for pattern in self.CONFIG_PATTERNS['path']:
            if re.search(pattern, path_lower, re.IGNORECASE):
                return True
        
        # 检查是否为6844或68xx配置
        if re.search(r'(6844|68xx|xwrl68)', filename, re.IGNORECASE):
            return True
        
        return False
    
    def _parse_application_firmware(self, file_path: str) -> FirmwareInfo:
        """解析应用固件信息"""
        filename = os.path.basename(file_path)
        path_parts = Path(file_path).parts
        
        info = FirmwareInfo(
            path=file_path,
            filename=filename,
            type=FirmwareType.APPLICATION
        )
        
        # 从路径提取信息
        info.category = self._extract_category(path_parts)
        info.subcategory = self._extract_subcategory(path_parts)
        info.platform = self._extract_platform(path_parts)
        info.processor = self._extract_processor(path_parts)
        info.compiler = self._extract_compiler(path_parts)
        
        # 从文件名提取版本
        if '.release.' in filename:
            info.version = 'Release'
        elif '.debug.' in filename:
            info.version = 'Debug'
        elif '.Release.' in filename:
            info.version = 'Release'
        elif '.Debug.' in filename:
            info.version = 'Debug'
        
        # 生成描述
        info.description = self._generate_firmware_description(info, path_parts)
        
        # 文件大小
        try:
            info.size = os.path.getsize(file_path)
        except:
            pass
        
        return info
    
    def _parse_sbl_firmware(self, file_path: str) -> SBLInfo:
        """解析SBL固件信息"""
        filename = os.path.basename(file_path)
        
        # 判断变体
        variant = "标准版"
        if 'lite' in filename.lower():
            variant = "轻量版"
        elif 'image_select' in filename.lower():
            variant = "镜像选择"
        
        # 生成描述
        descriptions = {
            "标准版": "完整功能SBL，支持QSPI Flash启动，适用于标准开发和生产环境",
            "轻量版": "精简版SBL，启动速度更快，适用于简单应用",
            "镜像选择": "支持多固件镜像选择和切换的SBL"
        }
        
        info = SBLInfo(
            path=file_path,
            filename=filename,
            variant=variant,
            description=descriptions.get(variant, "")
        )
        
        try:
            info.size = os.path.getsize(file_path)
        except:
            pass
        
        return info
    
    def _parse_config_file(self, file_path: str) -> ConfigInfo:
        """解析雷达配置文件信息"""
        filename = os.path.basename(file_path)
        path_parts = Path(file_path).parts
        
        info = ConfigInfo(
            path=file_path,
            filename=filename
        )
        
        # 从文件名提取信息
        info.application = self._extract_config_application(filename, path_parts)
        info.tx_channels = self._extract_tx_channels(filename)
        info.rx_channels = self._extract_rx_channels(filename)
        info.range_m = self._extract_range(filename)
        info.mode = self._extract_mode(filename)
        info.power_mode = self._extract_power_mode(filename)
        info.bandwidth = self._extract_bandwidth(filename)
        info.package_type = self._extract_package_type(filename)
        
        # 生成描述
        info.description = self._generate_config_description(info)
        
        return info
    
    def _extract_category(self, path_parts: Tuple[str]) -> str:
        """提取功能类别"""
        categories = {
            'control': '雷达控制',
            'datapath': '数据处理',
            'drivers': '硬件驱动',
            'kernel': '操作系统',
            'mmw_demo': '毫米波演示',
            'hello_world': '基础示例',
            'empty': '空白工程',
            'Automotive': '车载应用',
            'Industrial': '工业应用',
            'Fundamentals': '基础功能',
        }
        
        for part in path_parts:
            for key, value in categories.items():
                if key in part:
                    return value
        return "其他"
    
    def _extract_subcategory(self, path_parts: Tuple[str]) -> str:
        """提取子类别"""
        subcategories = {
            'InCabin': '车内监测',
            'People_Tracking': '人员跟踪',
            'Area_Scanner': '区域扫描',
            'Presence': '存在检测',
            'Traffic': '交通监控',
            'Gesture': '手势识别',
            'Vital_Signs': '生命体征',
            'gpio': 'GPIO示例',
            'uart': 'UART串口',
            'i2c': 'I2C通信',
            'spi': 'SPI通信',
            'can': 'CAN总线',
            'hwa': '硬件加速器',
        }
        
        for part in path_parts:
            for key, value in subcategories.items():
                if key in part:
                    return value
        return ""
    
    def _extract_platform(self, path_parts: Tuple[str]) -> str:
        """提取硬件平台"""
        for part in path_parts:
            if 'xwrL684x-evm' in part:
                return 'xWRL684x-EVM'
            elif 'AWRL6844' in part:
                return 'AWRL6844EVM'
        return ""
    
    def _extract_processor(self, path_parts: Tuple[str]) -> str:
        """提取处理器配置"""
        processors = {
            'r5fss0-0_freertos': 'ARM R5F + FreeRTOS',
            'r5fss0-0_nortos': 'ARM R5F 裸机',
            'system_freertos': '多核 + FreeRTOS',
            'system_nortos': '多核裸机',
        }
        
        for part in path_parts:
            for key, value in processors.items():
                if key in part:
                    return value
        return ""
    
    def _extract_compiler(self, path_parts: Tuple[str]) -> str:
        """提取编译器"""
        for part in path_parts:
            if 'ti-arm-clang' in part:
                return 'TI ARM Clang'
            elif 'gcc' in part.lower():
                return 'GCC'
        return ""
    
    def _extract_config_application(self, filename: str, path_parts: Tuple[str]) -> str:
        """提取配置文件应用场景"""
        applications = {
            'vod': '车内乘员检测',
            'incabin': '车内监测',
            'people_tracking': '人员跟踪',
            'presence': '存在检测',
            'area_scanner': '区域扫描',
            'traffic': '交通监控',
            'gesture': '手势识别',
            'vital': '生命体征',
            'level': '液位检测',
            'parking': '停车检测',
        }
        
        filename_lower = filename.lower()
        for key, value in applications.items():
            if key in filename_lower:
                return value
        
        # 从路径提取
        for part in path_parts:
            for key, value in applications.items():
                if key in part.lower():
                    return value
        
        return "通用配置"
    
    def _extract_tx_channels(self, filename: str) -> int:
        """提取TX通道数"""
        match = re.search(r'(\d+)[tT](\d+)[rR]', filename)
        if match:
            return int(match.group(1))
        
        if '6844' in filename or '4T4R' in filename:
            return 4
        elif '6843' in filename or '3T4R' in filename:
            return 3
        elif '6432' in filename or '2T4R' in filename:
            return 2
        
        return 0
    
    def _extract_rx_channels(self, filename: str) -> int:
        """提取RX通道数"""
        match = re.search(r'(\d+)[tT](\d+)[rR]', filename)
        if match:
            return int(match.group(2))
        
        # xWRL68xx系列默认4RX
        if '68' in filename:
            return 4
        
        return 0
    
    def _extract_range(self, filename: str) -> int:
        """提取检测距离"""
        match = re.search(r'(\d+)m', filename)
        if match:
            return int(match.group(1))
        return 0
    
    def _extract_mode(self, filename: str) -> str:
        """提取工作模式"""
        if '3d' in filename.lower():
            return '3D'
        elif '2d' in filename.lower():
            return '2D'
        elif 'tdm' in filename.lower():
            return 'TDM时分复用'
        elif 'ddm' in filename.lower():
            return 'DDM'
        return ""
    
    def _extract_power_mode(self, filename: str) -> str:
        """提取功耗模式"""
        filename_lower = filename.lower()
        if 'low_power' in filename_lower or '_lp' in filename_lower:
            return '低功耗'
        elif 'full_power' in filename_lower:
            return '满功率'
        return '标准功耗'
    
    def _extract_bandwidth(self, filename: str) -> str:
        """提取带宽模式"""
        filename_lower = filename.lower()
        if 'full_bandwidth' in filename_lower:
            return '全带宽'
        elif 'low_bandwidth' in filename_lower or 'low_bw' in filename_lower:
            return '低带宽'
        return '标准带宽'
    
    def _extract_package_type(self, filename: str) -> str:
        """提取封装类型"""
        filename_upper = filename.upper()
        if 'AOP' in filename_upper:
            return 'AOP封装'
        elif 'ISK' in filename_upper:
            return 'ISK封装'
        elif 'ODS' in filename_upper:
            return 'ODS封装'
        return ""
    
    def _generate_firmware_description(self, info: FirmwareInfo, path_parts: Tuple[str]) -> str:
        """生成固件描述"""
        parts = []
        
        if info.category:
            parts.append(info.category)
        if info.subcategory:
            parts.append(info.subcategory)
        if info.processor:
            parts.append(info.processor)
        if info.version:
            parts.append(f"{info.version}版本")
        
        return " - ".join(parts) if parts else "AWRL6844应用固件"
    
    def _generate_config_description(self, info: ConfigInfo) -> str:
        """生成配置文件描述"""
        parts = []
        
        if info.application:
            parts.append(info.application)
        
        if info.tx_channels > 0 and info.rx_channels > 0:
            parts.append(f"{info.tx_channels}TX/{info.rx_channels}RX")
        
        if info.range_m > 0:
            parts.append(f"检测距离{info.range_m}m")
        
        if info.mode:
            parts.append(info.mode)
        
        if info.power_mode and info.power_mode != '标准功耗':
            parts.append(info.power_mode)
        
        if info.bandwidth and info.bandwidth != '标准带宽':
            parts.append(info.bandwidth)
        
        if info.package_type:
            parts.append(info.package_type)
        
        return " | ".join(parts) if parts else "雷达参数配置"
    
    def match_sbl_for_firmware(self, firmware: FirmwareInfo) -> List[Tuple[SBLInfo, float]]:
        """为应用固件匹配SBL固件（改进版v2.2 - 强化SDK路径判断）
        
        评分体系（按重要性排序）：
        
        【核心判断】：
        1. 同一SDK路径：50分（最高优先级，确保版本兼容）
        2. SDK路径特征：
           - ti-arm-clang路径：40分（官方SDK，生产环境）
           - prebuilt_binaries路径：-80分（示例工具箱，不适合生产）
        
        【辅助判断】：
        3. 文件格式检测：
           - Multi-Image格式：30分（可烧录，但只是表象）
           - Single-Image格式：-100分（不可烧录）
        4. 硬件平台匹配（xwrL684x-evm）：20分
        5. SBL版本类型：
           - 标准版：20分
           - 轻量版：10分
        
        总分范围：[-180, 160]
        - 理想情况：同SDK + ti-arm-clang + Multi-Image + 平台匹配 + 标准版 = 160分
        - 最差情况：不同SDK + prebuilt + Single-Image = -180分
        """
        matches = []
        
        for sbl in self.sbl_firmwares:
            score = 0.0
            
            # ========== 1. SDK版本匹配（最高优先级）==========
            # 确保SBL和应用固件来自同一SDK，避免版本不兼容
            if self._is_same_sdk(firmware.path, sbl.path):
                score += 50.0
            
            # ========== 2. SDK路径特征检测（根本判断）==========
            # 路径特征反映了SDK的定位和用途
            
            # ti-arm-clang路径：官方开发SDK，适合生产环境
            if 'ti-arm-clang' in sbl.path.lower():
                score += 40.0  # 高分，推荐使用
            
            # prebuilt_binaries路径：预编译示例，不适合生产
            if 'prebuilt_binaries' in sbl.path.lower():
                score -= 80.0  # 严重惩罚，强烈不推荐
            
            # ========== 3. 文件格式检测（表象验证）==========
            # 格式检测只是验证SDK路径判断的正确性
            image_format = self._check_appimage_format(sbl.path)
            
            if image_format == "Multi-Image":
                score += 30.0  # ✅ 可烧录格式
            elif image_format == "Single-Image":
                score -= 100.0  # ❌ 不可烧录，严重惩罚
            
            # ========== 4. 硬件平台匹配 ==========
            # 确认是xwrL684x平台的SBL
            if 'xwrl684x' in sbl.path.lower():
                score += 20.0
            
            # ========== 5. SBL版本类型 ==========
            # 标准版SBL功能更完整，优先推荐
            if sbl.variant == "标准版":
                score += 20.0
            elif sbl.variant == "轻量版":
                score += 10.0
            
            matches.append((sbl, score))
        
        # 按评分排序，返回最佳匹配
        matches.sort(key=lambda x: x[1], reverse=True)
        return matches
    
    def match_configs_for_firmware(self, firmware: FirmwareInfo) -> List[Tuple[ConfigInfo, float, dict]]:
        """为应用固件匹配雷达配置文件（v4.0.2 - 2025-12-23）
        
        v4.0.2关键改进：解决"所有固件推荐相同配置"的问题
        - ✅ 添加固件名称语义匹配（60分）
        - ✅ 降低SDK路径权重（80分→40分）
        - ✅ 提取固件关键词进行精准匹配
        
        基于实际数据优化的评分体系：
        
        【P0级验证 - 一票否决】：
        1. 必需命令检测：缺少channelCfg/frameCfg/sensorStart → -999999分
        2. 中文字符检测：包含中文 → -500分
        3. 文件编码检测：UTF-8编码 → -200分
        
        【P1级评分 - 核心匹配】（总分230分）:
        1. 固件名称语义匹配：60分（v4.0.2新增，解决关键问题）
           - 固件关键词完全匹配：+30分/个
           - 固件关键词部分匹配：+15分/个
        2. 同SDK路径：40分（v4.0.2降低，从80分）
        3. 核心参数匹配：
           - frameCfg完全匹配：50分
           - runtimeCalibCfg=1：30分
           - lowPowerCfg匹配：20分
        4. Demo目录关联：30分（v4.0.2新增）
        
        【P2级评分 - 辅助参考】（总分75分）:
        1. 应用场景文本：20分
        2. 芯片型号：20分
        3. 检测距离：15分
        4. 天线配置：15分
        5. 功耗模式：5分
        
        返回格式：List[Tuple[ConfigInfo, float, dict]]
        - ConfigInfo: 配置文件信息
        - float: 总分
        - dict: 验证详情
            {
                'p0_encoding': True/False,  # 编码检测
                'p0_antenna': True/False,   # 天线配置
                'p0_comment': True/False,   # 注释格式
                'p1_sdk': score,            # SDK匹配分数
                'p1_params': score,         # 参数匹配分数
                'warnings': [...]           # 警告信息
            }
        """
        matches = []
        
        for config in self.config_files:
            score = 0.0
            validation = {
                'p0_encoding': True,
                'p0_antenna': True,
                'p0_comment': True,
                'p0_required_commands': True,  # v4.0新增
                'p1_sdk': 0,
                'p1_params': 0,
                'warnings': [],
                'fatal_errors': []  # v4.0新增：致命错误
            }
            
            # ========== P0级验证：必需命令检测（v4.0新增，最高优先级）==========
            required_check = self._check_required_commands(config.path)
            
            if not required_check['has_all_required']:
                score = -999999.0  # 🔴 一票否决：缺少必需命令
                validation['p0_required_commands'] = False
                validation['fatal_errors'].append(
                    f"❌ 缺少必需命令：{', '.join(required_check['missing_commands'])}"
                )
                # 缺少必需命令直接标记为不可用，但仍继续检测其他问题
            
            if required_check['has_invalid_commands']:
                score -= 800.0  # 🔴 严重惩罚：使用不存在的命令
                validation['p0_required_commands'] = False
                validation['fatal_errors'].append(
                    f"❌ 使用无效命令：{', '.join(required_check['invalid_commands'])}"
                )
            
            # ========== P0级验证：文件编码检测 ==========
            # v4.0.1: 降低编码问题的惩罚，因为很多官方配置使用%注释
            encoding_check = self._check_file_encoding(config.path)
            
            if not encoding_check['is_ascii']:
                score -= 200.0  # 🟡 中度惩罚：UTF-8编码（从-1000降低）
                validation['p0_encoding'] = False
                validation['warnings'].append(f"⚠️ UTF-8编码问题：{encoding_check['issue']}")
            
            if encoding_check['has_chinese']:
                score -= 500.0  # 🔴 严重惩罚：中文字符（从-1000降低）
                validation['p0_encoding'] = False
                validation['warnings'].append(f"❌ 包含中文字符（字节{encoding_check['position']}）")
            
            if encoding_check['has_percent_comment']:
                # v4.0.1: %注释很常见，不扣分，仅提示
                validation['warnings'].append("ℹ️ 使用%注释符")
            
            # ========== P0级验证：天线配置方式检测 ==========
            # v4.0.1: 移除天线配置的强制要求，仅作为加分项
            antenna_check = self._check_antenna_config(config.path)
            
            # 天线配置作为P1加分项（不再扣分）
            if antenna_check['uses_antGeometryCfg']:
                # antGeometryCfg是有效命令，给予加分
                score += 10.0
                validation['warnings'].append("ℹ️ 使用antGeometryCfg配置")
            
            if antenna_check['missing_antGeometryBoard']:
                # 不扣分，很多配置不需要Board
                pass
            
            if antenna_check['uses_manual_config']:
                manual_completeness = antenna_check['manual_completeness']
                if manual_completeness >= 4:
                    score += 15.0  # 手动配置完整，加分
                    validation['warnings'].append("✓ 手动天线配置完整")
            
            # ========== P1级评分：固件名称语义匹配（v4.0.2新增，最高优先级）==========
            # 解决"所有固件推荐相同配置"的核心问题
            firmware_keywords = self._extract_firmware_keywords(firmware.filename)
            config_keywords = self._extract_config_keywords(config.filename)
            
            keyword_match_score = 0
            matched_keywords = []
            
            for fw_kw in firmware_keywords:
                for cfg_kw in config_keywords:
                    if fw_kw == cfg_kw:  # 完全匹配
                        keyword_match_score += 30
                        matched_keywords.append(fw_kw)
                    elif fw_kw in cfg_kw or cfg_kw in fw_kw:  # 部分匹配
                        keyword_match_score += 15
                        matched_keywords.append(f"{fw_kw}~{cfg_kw}")
            
            # 限制最高60分
            keyword_match_score = min(keyword_match_score, 60)
            score += keyword_match_score
            validation['p1_name_match'] = keyword_match_score
            
            if matched_keywords:
                validation['warnings'].append(
                    f"✓ 关键词匹配：{', '.join(matched_keywords[:3])}"  # 只显示前3个
                )
            
            # ========== P1级评分：同SDK路径关系（降低权重）==========
            # v4.0.2: 从80分降至40分，避免SDK路径主导排序
            if self._is_same_sdk(firmware.path, config.path):
                score += 40.0
                validation['p1_sdk'] = 40
            elif self._is_related_in_sdk(firmware.path, config.path):
                score += 30.0
                validation['p1_sdk'] = 30
            
            # ========== P1级评分：Demo目录关联（v4.0.2新增）==========
            if self._is_same_demo_directory(firmware.path, config.path):
                score += 30.0
                validation['p1_demo'] = 30
                validation['warnings'].append("✓ 同一Demo目录")
            
            # ========== P1级评分：核心参数匹配验证 ==========
            param_check = self._check_core_parameters(config.path, firmware)
            
            # frameCfg参数匹配
            if param_check['frameCfg_match']:
                score += 50.0
                validation['p1_params'] += 50
            else:
                validation['warnings'].append(
                    f"⚠️ frameCfg不匹配：{param_check['frameCfg_diff']}"
                )
            
            # runtimeCalibCfg检测
            if param_check['runtimeCalibCfg'] == 1:
                score += 30.0
                validation['p1_params'] += 30
            elif param_check['runtimeCalibCfg'] == 0:
                score -= 20.0
                validation['warnings'].append("⚠️ runtimeCalibCfg=0（禁用校准）")
            
            # lowPowerCfg匹配
            if param_check['lowPowerCfg'] == 1:
                score += 20.0
                validation['p1_params'] += 20
            elif param_check['lowPowerCfg'] == 0:
                score -= 10.0
                validation['warnings'].append("⚠️ lowPowerCfg=0（未启用低功耗）")
            
            # ========== P1级评分：配置文件名语义匹配 ==========
            config_semantics = self._parse_config_filename(config.filename)
            
            # InCabin Demo特殊处理
            if 'incabin' in firmware.path.lower():
                if 'cpd' in config.filename.lower():  # Child Presence Detection
                    score += 60.0
                elif 'sbr' in config.filename.lower():  # Seat Belt Reminder
                    score += 60.0
                elif 'intrusion' in config.filename.lower():  # Intrusion Detection
                    score += 60.0
            
            # 通用场景语义匹配
            if 'scene' in config_semantics:
                scene = config_semantics['scene']
                fw_lower = firmware.path.lower() + firmware.subcategory.lower()
                if any(kw in scene for kw in ['child_presence', 'intrusion', 'vital', 'gesture']):
                    if any(kw in fw_lower for kw in ['cpd', 'intrusion', 'vital', 'gesture']):
                        score += 50.0
            
            # ========== P2级评分：应用场景文本匹配 ==========
            if firmware.subcategory and config.application:
                if firmware.subcategory in config.application:
                    score += 20.0
                elif config.application in firmware.subcategory:
                    score += 15.0
            
            # ========== P2级评分：芯片型号匹配 ==========
            if '6844' in config.filename.lower():
                score += 20.0  # 6844专用
            elif '68xx' in config.filename.lower():
                score += 15.0  # 68xx系列通用
            
            # ========== P2级评分：检测距离合理性 ==========
            if config.range_m > 0:
                if config.range_m <= 10 and self._is_short_range_app(firmware):
                    score += 15.0
                elif 10 < config.range_m <= 50:
                    score += 10.0
            
            # ========== P2级评分：功耗模式匹配 ==========
            if 'power' in config_semantics:
                if config_semantics['power'] == 'low_power' and 'low_power' in firmware.path.lower():
                    score += 10.0
            
            matches.append((config, score, validation))
        
        # 按评分排序
        matches.sort(key=lambda x: x[1], reverse=True)
        return matches
    
    def _extract_sdk_root(self, path: str) -> str:
        """提取SDK根目录名称
        
        示例：
        C:\\ti\\radar_toolbox_3_30_00_06\\... → radar_toolbox_3_30_00_06
        C:\\ti\\MMWAVE_L_SDK_06_01_00_01\\... → MMWAVE_L_SDK_06_01_00_01
        """
        path_parts = path.replace('\\', '/').split('/')
        
        for part in path_parts:
            part_lower = part.lower()
            if 'radar_toolbox' in part_lower:
                return part
            if 'mmwave_l_sdk' in part_lower:
                return part
            if 'radar_academy' in part_lower:
                return part
                
        return ""
    
    def _is_same_sdk(self, path1: str, path2: str) -> bool:
        """判断两个文件是否在同一SDK中"""
        sdk1 = self._extract_sdk_root(path1)
        sdk2 = self._extract_sdk_root(path2)
        
        if sdk1 and sdk2 and sdk1 == sdk2:
            return True
        return False
    
    def _check_appimage_format(self, filepath: str) -> str:
        """检测appimage文件格式（Multi-Image vs Single-Image）
        
        ⚠️ 重要说明：
        文件格式检测只是**辅助验证手段**，真正的根本判断是SDK路径特征。
        
        - Multi-Image格式：通常来自MMWAVE_L_SDK（ti-arm-clang路径）
        - Single-Image格式：通常来自RADAR_TOOLBOX（prebuilt_binaries路径）
        
        格式判断依据（读取MSTR+4字节）：
        - Multi-Image：MSTR+4 = 文件大小-16（可烧录Flash）
        - Single-Image：MSTR+4 = 0x00000001（RAM加载，不可烧录）
        
        返回：
        - "Multi-Image": 可以烧录到Flash
        - "Single-Image": 只能RAM加载，烧录会0秒完成
        - "Unknown": 文件格式错误或无法识别
        
        参考：
        - SBL烧录0秒问题分析.md - SDK路径与兼容性章节
        - 问题根源在于SDK定位差异，不仅仅是文件格式
        """
        try:
            import struct
            import os
            
            with open(filepath, 'rb') as f:
                # 读取Magic（前4字节）
                magic = f.read(4)
                if magic != b'MSTR':
                    return "Unknown"
                
                # 读取MSTR+4字节的值
                mstr_value = struct.unpack('<I', f.read(4))[0]
                file_size = os.path.getsize(filepath)
                
                # 判断格式
                if mstr_value == 0x00000001:
                    # Single-Image格式：固定值1
                    return "Single-Image"
                elif abs(mstr_value - (file_size - 16)) < 100:
                    # Multi-Image格式：接近文件大小-16
                    return "Multi-Image"
                else:
                    return "Unknown"
                    
        except Exception as e:
            # 静默失败，返回Unknown
            return "Unknown"
    
    def _is_same_demo_directory(self, fw_path: str, cfg_path: str) -> bool:
        """判断固件和配置是否在同一Demo目录下
        
        示例：
        固件：C:\\ti\\radar_toolbox_3_30_00_06\\source\\ti\\examples\\
             Automotive_InCabin_Security_and_Safety\\AWRL6844_InCabin_Demos\\
             prebuilt_binaries\\demo_in_cabin_sensing_6844_system.release.appimage
        配置：C:\\ti\\radar_toolbox_3_30_00_06\\tools\\visualizers\\
             AWRL6844_Incabin_GUI\\src\\chirpConfigs6844\\cpd.cfg
        
        判断依据：
        1. 路径中都包含"InCabin"或"incabin"
        2. 都在同一radar_toolbox版本下
        3. 配置在visualizers/GUI工具目录下
        """
        fw_lower = fw_path.lower()
        cfg_lower = cfg_path.lower()
        
        # 检查是否在同一SDK
        if not self._is_same_sdk(fw_path, cfg_path):
            return False
        
        # InCabin Demo特殊规则
        if 'incabin' in fw_lower:
            if 'incabin_gui' in cfg_lower or 'awrl6844_incabin' in cfg_lower:
                return True
        
        # 可以继续添加其他Demo的规则
        # TODO: 添加其他Demo的目录关联规则
        
        return False
    
    def _is_related_in_sdk(self, fw_path: str, cfg_path: str) -> bool:
        """判断固件和配置是否在同一SDK的关联目录
        
        关联规则：
        - examples目录下的固件 → tools/visualizers下的配置
        - examples目录下的固件 → tools/mmwave_data_recorder下的配置
        """
        fw_lower = fw_path.lower()
        cfg_lower = cfg_path.lower()
        
        # 同一SDK
        if not self._is_same_sdk(fw_path, cfg_path):
            return False
        
        # 固件在examples，配置在tools
        if 'examples' in fw_lower and 'tools' in cfg_lower:
            return True
            
        return False
    
    def _extract_firmware_keywords(self, firmware_filename: str) -> List[str]:
        """从固件文件名提取关键词（v4.0.2新增）
        
        示例：
        hwa_dc_sub.system.release.appimage → ['hwa', 'dc', 'sub', 'system']
        hello_world.system.release.appimage → ['hello', 'world', 'basic', 'simple']
        demo_in_cabin_sensing_6844.system.release.appimage → ['incabin', 'cabin', 'sensing', '6844']
        """
        keywords = set()
        
        # 移除文件扩展名和常见后缀
        name = firmware_filename.lower()
        name = name.replace('.system.release.appimage', '')
        name = name.replace('.system.debug.appimage', '')
        name = name.replace('.release.appimage', '')
        name = name.replace('.debug.appimage', '')
        name = name.replace('.appimage', '')
        
        # 按下划线和点分割
        parts = re.split(r'[_.]', name)
        
        # 添加所有部分作为关键词
        for part in parts:
            if len(part) > 2:  # 过滤掉过短的部分
                keywords.add(part)
        
        # 特殊关键词映射
        keyword_mapping = {
            'hello': ['basic', 'simple', 'demo'],
            'empty': ['minimal', 'basic'],
            'incabin': ['cabin', 'cpd', 'sbr', 'intrusion'],
            'vital': ['signs', 'heartbeat', 'breathing'],
            'gesture': ['hand', 'motion'],
            'occupancy': ['presence', 'detection']
        }
        
        # 应用映射扩展关键词
        for kw in list(keywords):
            if kw in keyword_mapping:
                keywords.update(keyword_mapping[kw])
        
        return list(keywords)
    
    def _extract_config_keywords(self, config_filename: str) -> List[str]:
        """从配置文件名提取关键词（v4.0.2新增）
        
        示例：
        cpd.cfg → ['cpd', 'child', 'presence', 'detection']
        6844_profile_4T4R_tdm.cfg → ['6844', 'profile', '4t4r', 'tdm']
        high_accuracy_demo_68xx.cfg → ['accuracy', 'demo', '68xx', '6844']
        """
        keywords = set()
        
        # 移除文件扩展名
        name = config_filename.lower().replace('.cfg', '')
        
        # 按下划线、点、空格分割
        parts = re.split(r'[_.\s]', name)
        
        # 添加所有部分作为关键词
        for part in parts:
            if len(part) > 1:  # 过滤掉单字符
                keywords.add(part)
        
        # 特殊关键词映射
        keyword_mapping = {
            'cpd': ['child', 'presence', 'detection', 'cabin', 'incabin'],
            'sbr': ['seatbelt', 'belt', 'reminder', 'cabin', 'incabin'],
            'intrusion': ['intruder', 'detection', 'cabin', 'incabin'],
            'vital': ['signs', 'heartbeat', 'breathing'],
            '68xx': ['6844', '6843', '6843aop'],
            'hwa': ['hardware', 'accelerator'],
            'dc': ['datacollection', 'data']
        }
        
        # 应用映射扩展关键词
        for kw in list(keywords):
            if kw in keyword_mapping:
                keywords.update(keyword_mapping[kw])
        
        return list(keywords)
    
    def _parse_config_filename(self, filename: str) -> Dict[str, str]:
        """解析配置文件名的语义
        
        示例：
        cpd.cfg → {'scene': 'child_presence_detection', 'power': 'normal'}
        intrusion_detection_LP.cfg → {'scene': 'intrusion', 'power': 'low_power'}
        xWRL6844_4T4R_tdm.cfg → {'antenna': '4T4R', 'mode': 'tdm', 'chip': '6844'}
        """
        semantics = {}
        filename_lower = filename.lower()
        
        # 应用场景识别
        scene_keywords = {
            'cpd': 'child_presence_detection',
            'sbr': 'seat_belt_reminder',
            'intrusion': 'intrusion_detection',
            'vital': 'vital_signs',
            'gesture': 'gesture_recognition',
            'occupancy': 'occupancy_detection'
        }
        
        for keyword, scene in scene_keywords.items():
            if keyword in filename_lower:
                semantics['scene'] = scene
                break
        
        # 功耗模式识别
        if '_lp' in filename_lower or 'low_power' in filename_lower:
            semantics['power'] = 'low_power'
        else:
            semantics['power'] = 'normal'
        
        # 天线配置识别
        if '4t4r' in filename_lower:
            semantics['antenna'] = '4T4R'
        elif '2t4r' in filename_lower:
            semantics['antenna'] = '2T4R'
        
        # TDM/BPM模式
        if 'tdm' in filename_lower:
            semantics['mode'] = 'tdm'
        elif 'bpm' in filename_lower:
            semantics['mode'] = 'bpm'
        
        # 芯片型号
        if '6844' in filename_lower:
            semantics['chip'] = '6844'
        elif '6843' in filename_lower:
            semantics['chip'] = '6843'
        
        return semantics
    
    def _is_short_range_app(self, firmware: FirmwareInfo) -> bool:
        """判断是否为短距离应用（≤10m）
        
        短距离应用关键词：
        - InCabin（车内）
        - Indoor（室内）
        - Gesture（手势）
        - Vital Signs（生命体征）
        """
        short_range_keywords = [
            'incabin', 'indoor', 'gesture', 'vital', 
            'occupancy', '车内', '室内', '手势'
        ]
        
        fw_text = (firmware.path + firmware.subcategory).lower()
        
        return any(keyword in fw_text for keyword in short_range_keywords)
    
    def get_statistics(self) -> Dict:
        """获取扫描统计信息"""
        return {
            'application_count': len(self.application_firmwares),
            'sbl_count': len(self.sbl_firmwares),
            'config_count': len(self.config_files),
            'total_count': len(self.application_firmwares) + len(self.sbl_firmwares) + len(self.config_files)
        }
    
    def clear_results(self):
        """清空扫描结果"""
        self.application_firmwares.clear()
        self.sbl_firmwares.clear()
        self.config_files.clear()
    
    def _check_required_commands(self, config_path: str) -> dict:
        """检测必需命令（v4.0.1修正）
        
        基于实际配置文件分析的必需命令：
        1. channelCfg - 通道配置
        2. frameCfg - 帧配置
        3. sensorStart - 启动命令
        
        注意：天线配置不是必需的（很多配置依赖默认值）
        
        返回:
            {
                'has_all_required': True/False,
                'missing_commands': List[str],
                'has_invalid_commands': False,  # 已移除
                'invalid_commands': [],
                'antenna_config_mode': 'board'/'cfg'/'geometry'/'none'
            }
        """
        # 3个核心必需命令（移除天线配置要求）
        REQUIRED_COMMANDS = [
            'channelCfg',
            'frameCfg',
            'sensorStart'
        ]
        
        result = {
            'has_all_required': True,
            'missing_commands': [],
            'has_invalid_commands': False,  # 保持兼容性
            'invalid_commands': [],
            'antenna_config_mode': 'none'
        }
        
        try:
            with open(config_path, 'r', encoding='ascii', errors='ignore') as f:
                content = f.read()
            
            # 检查必需命令
            for cmd in REQUIRED_COMMANDS:
                if cmd not in content:
                    result['has_all_required'] = False
                    result['missing_commands'].append(cmd)
            
            # 识别天线配置方式（仅用于信息展示，不影响必需命令判断）
            if 'antGeometryBoard' in content:
                result['antenna_config_mode'] = 'board'
            elif 'antGeometryCfg' in content:
                result['antenna_config_mode'] = 'cfg'
            elif 'antGeometry0' in content or 'antGeometry1' in content:
                result['antenna_config_mode'] = 'geometry'
            else:
                result['antenna_config_mode'] = 'none'
            
            return result
            
        except Exception as e:
            result['has_all_required'] = False
            result['missing_commands'].append(f'Error reading file: {str(e)}')
            return result
    
    def _check_file_encoding(self, config_path: str) -> dict:
        """检测配置文件编码和中文字符（P0级验证）
        
        返回:
            {
                'is_ascii': True/False,      # 是否纯ASCII
                'has_chinese': True/False,   # 是否包含中文
                'has_percent_comment': True/False,  # 是否使用%注释
                'issue': str,                # 问题描述
                'position': int              # 问题位置（字节）
            }
        """
        result = {
            'is_ascii': True,
            'has_chinese': False,
            'has_percent_comment': False,
            'issue': '',
            'position': -1
        }
        
        try:
            # 读取文件二进制内容
            with open(config_path, 'rb') as f:
                content = f.read()
            
            # 检测BOM
            if content.startswith(b'\xef\xbb\xbf'):  # UTF-8 BOM
                result['is_ascii'] = False
                result['issue'] = 'UTF-8 BOM detected'
                return result
            
            # 逐字节检查
            for i, byte in enumerate(content):
                # ASCII范围：0x00-0x7F
                if byte > 0x7F:
                    result['is_ascii'] = False
                    result['has_chinese'] = True
                    result['position'] = i
                    result['issue'] = f'Non-ASCII byte 0x{byte:02x} at position {i}'
                    return result
            
            # 检测注释符（文本层面）
            try:
                text = content.decode('ascii')
                if '%' in text:
                    result['has_percent_comment'] = True
            except:
                pass
            
            return result
            
        except Exception as e:
            result['is_ascii'] = False
            result['issue'] = f'Error reading file: {str(e)}'
            return result
    
    def _check_antenna_config(self, config_path: str) -> dict:
        """检测天线配置方式（P0级验证）
        
        返回:
            {
                'uses_antGeometryCfg': True/False,      # 使用错误命令
                'missing_antGeometryBoard': True/False, # 缺少Board配置
                'uses_manual_config': True/False,       # 使用手动配置
                'manual_completeness': int              # 手动配置完整度(0-4)
            }
        """
        result = {
            'uses_antGeometryCfg': False,
            'missing_antGeometryBoard': False,
            'uses_manual_config': False,
            'manual_completeness': 0
        }
        
        try:
            with open(config_path, 'r', encoding='ascii', errors='ignore') as f:
                content = f.read()
            
            # 检测antGeometryCfg（错误命令）
            if 'antGeometryCfg' in content:
                result['uses_antGeometryCfg'] = True
            
            # 检测antGeometryBoard（推荐方式）
            if 'antGeometryBoard' not in content:
                result['missing_antGeometryBoard'] = True
                
                # 检查手动配置完整度
                manual_commands = [
                    'antGeometryTX',
                    'antGeometryRx',
                    'antGeometryDist',
                    'compRangeBiasAndRxChanPhase'
                ]
                completeness = sum(1 for cmd in manual_commands if cmd in content)
                
                if completeness > 0:
                    result['uses_manual_config'] = True
                    result['manual_completeness'] = completeness
            
            return result
            
        except Exception as e:
            return result
    
    def _check_core_parameters(self, config_path: str, firmware: FirmwareInfo) -> dict:
        """检测核心参数匹配度（P1级评分）
        
        返回:
            {
                'frameCfg_match': True/False,
                'frameCfg_diff': str,
                'runtimeCalibCfg': 0/1/-1,
                'lowPowerCfg': 0/1/-1,
                'adcDataDitherCfg': 0/1/-1
            }
        """
        result = {
            'frameCfg_match': False,
            'frameCfg_diff': '',
            'runtimeCalibCfg': -1,
            'lowPowerCfg': -1,
            'adcDataDitherCfg': -1
        }
        
        try:
            with open(config_path, 'r', encoding='ascii', errors='ignore') as f:
                content = f.read()
            
            # 解析frameCfg（期望：64 0 1358 1 100 0）
            import re
            frame_match = re.search(r'frameCfg\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)', content)
            if frame_match:
                loops = int(frame_match.group(3))
                period = int(frame_match.group(5))
                
                if loops == 1358 and period == 100:
                    result['frameCfg_match'] = True
                else:
                    result['frameCfg_diff'] = f'loops={loops}(期望1358), period={period}(期望100)'
            
            # 解析runtimeCalibCfg
            runtime_match = re.search(r'runtimeCalibCfg\s+(\d+)', content)
            if runtime_match:
                result['runtimeCalibCfg'] = int(runtime_match.group(1))
            
            # 解析lowPowerCfg
            lowpower_match = re.search(r'lowPowerCfg\s+(\d+)', content)
            if lowpower_match:
                result['lowPowerCfg'] = int(lowpower_match.group(1))
            
            # 解析adcDataDitherCfg
            dither_match = re.search(r'adcDataDitherCfg\s+(\d+)', content)
            if dither_match:
                result['adcDataDitherCfg'] = int(dither_match.group(1))
            
            return result
            
        except Exception as e:
            return result
