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
        """为应用固件匹配SBL"""
        matches = []
        
        for sbl in self.sbl_firmwares:
            score = 0.0
            
            # 检查是否在同一SDK
            if self._is_same_sdk(firmware.path, sbl.path):
                score += 50.0
            
            # 标准版SBL优先
            if sbl.variant == "标准版":
                score += 30.0
            elif sbl.variant == "轻量版":
                score += 20.0
            
            # 检查硬件平台
            if 'xwrL684x' in sbl.path.lower():
                score += 20.0
            
            matches.append((sbl, score))
        
        # 按评分排序
        matches.sort(key=lambda x: x[1], reverse=True)
        return matches
    
    def match_configs_for_firmware(self, firmware: FirmwareInfo) -> List[Tuple[ConfigInfo, float]]:
        """为应用固件匹配雷达配置文件（改进算法v2）
        
        评分体系（总分最高可达200+）：
        - 同Demo目录：100分（最高优先级）
        - 同SDK关联目录：80分
        - 配置文件名语义匹配：60分
        - 应用场景文本匹配：20分
        - 芯片型号匹配：20分
        - 检测距离合理性：15分
        - 功耗模式匹配：5分
        """
        matches = []
        
        for config in self.config_files:
            score = 0.0
            
            # ========== 1. 目录树关系匹配（最高优先级）==========
            # 检查是否在同一Demo目录下
            if self._is_same_demo_directory(firmware.path, config.path):
                score += 100.0  # 最高优先级
            
            # 检查是否在同一SDK下的关联目录
            if self._is_related_in_sdk(firmware.path, config.path):
                score += 80.0
            
            # ========== 2. 配置文件名语义匹配 ==========
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
                if 'child_presence' in scene and 'cpd' in fw_lower:
                    score += 50.0
                elif 'intrusion' in scene and 'intrusion' in fw_lower:
                    score += 50.0
                elif 'vital' in scene and 'vital' in fw_lower:
                    score += 50.0
                elif 'gesture' in scene and 'gesture' in fw_lower:
                    score += 50.0
            
            # ========== 3. 应用场景文本匹配（降低权重）==========
            if firmware.subcategory and config.application:
                if firmware.subcategory in config.application:
                    score += 20.0  # 从40降到20
                elif config.application in firmware.subcategory:
                    score += 15.0
            
            # ========== 4. 芯片型号匹配 ==========
            if '6844' in config.filename.lower():
                score += 20.0  # 6844专用
            elif '68xx' in config.filename.lower():
                score += 15.0  # 68xx系列通用
            elif '6843' in config.filename.lower():
                score += 10.0  # 6843可能兼容
            
            # ========== 5. 检测距离合理性 ==========
            if config.range_m > 0:
                if config.range_m <= 10 and self._is_short_range_app(firmware):
                    score += 15.0  # 短距离匹配
                elif 10 < config.range_m <= 50:
                    score += 10.0  # 中距离
                elif config.range_m > 50:
                    score += 5.0   # 长距离
            
            # ========== 6. 功耗模式匹配 ==========
            if 'power' in config_semantics:
                if config_semantics['power'] == 'low_power' and 'low_power' in firmware.path.lower():
                    score += 10.0
                elif '_lp' in config.filename.lower() and '_lp' in firmware.path.lower():
                    score += 10.0
            elif config.power_mode == '低功耗' and 'low_power' in firmware.path.lower():
                score += 5.0
            
            matches.append((config, score))
        
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
