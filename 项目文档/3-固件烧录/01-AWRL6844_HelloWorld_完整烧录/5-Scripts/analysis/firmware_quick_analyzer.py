"""
轻量级固件分析器 - 只分析固件文件，快速对比

专注于 .appimage 固件文件分析，生成对比表格
"""

import os
import json
from pathlib import Path
from typing import Dict, List, Optional


class FirmwareQuickAnalyzer:
    """快速固件分析器 - 只分析固件，不扫描源码"""
    
    def __init__(self, firmware_files: List[str]):
        """
        初始化快速分析器
        
        Args:
            firmware_files: 固件文件路径列表
        """
        self.firmware_files = firmware_files
        self.analysis_result = None
    
    def analyze(self) -> Dict:
        """
        快速分析固件
        
        Returns:
            分析结果
        """
        firmwares = []
        
        for fw_path in self.firmware_files:
            if not os.path.exists(fw_path):
                continue
            
            fw_info = self._analyze_single_firmware(fw_path)
            if fw_info:
                firmwares.append(fw_info)
        
        # 排序：按大小
        firmwares.sort(key=lambda x: x['file_size'])
        
        self.analysis_result = {
            'firmwares': firmwares,
            'project_name': self._get_project_name(firmwares),
            'summary': self._generate_summary(firmwares)
        }
        
        return self.analysis_result
    
    def _analyze_single_firmware(self, fw_path: str) -> Dict:
        """
        分析单个固件文件
        
        Args:
            fw_path: 固件文件路径
        
        Returns:
            固件信息字典
        """
        file_name = os.path.basename(fw_path)
        file_size = os.path.getsize(fw_path)
        
        # 从路径和文件名提取信息
        path_parts = Path(fw_path).parts
        
        info = {
            'file_name': file_name,
            'file_path': fw_path,
            'file_size': file_size,
            'file_size_kb': file_size / 1024,
            
            # 架构分析
            'architecture': self._detect_architecture(file_name, path_parts),
            'core_type': self._detect_core_type(file_name, path_parts),
            
            # 操作系统
            'os_type': self._detect_os_type(file_name, path_parts),
            'os_name': self._get_os_name(file_name, path_parts),
            
            # 多核分析
            'is_system': self._is_system_firmware(file_name, path_parts),
            'is_multicore': self._is_multicore(file_name, path_parts),
            
            # 变体类型
            'variant_name': self._get_variant_name(file_name, path_parts),
            
            # 目录信息
            'directory': str(Path(fw_path).parent),
            'relative_dir': self._get_relative_dir(path_parts),
            
            # 特性
            'features': self._extract_features(file_name, path_parts, file_size)
        }
        
        return info
    
    def _detect_architecture(self, name: str, parts: tuple) -> str:
        """检测架构"""
        name_lower = name.lower()
        parts_str = '/'.join(parts).lower()
        
        if 'system' in name_lower or 'system' in parts_str:
            return 'Multi-core System'
        elif 'r5f' in name_lower or 'r5fss0' in parts_str:
            return 'R5F (ARM Cortex-R5)'
        elif 'c66' in name_lower or 'c66ss0' in parts_str:
            return 'C66x DSP'
        return 'Unknown'
    
    def _detect_core_type(self, name: str, parts: tuple) -> str:
        """检测核心类型"""
        parts_str = '/'.join(parts).lower()
        name_lower = name.lower()
        
        if 'system' in name_lower or 'system' in parts_str:
            return 'R5F + C66x + RF'
        elif 'r5f' in name_lower or 'r5fss0' in parts_str:
            return 'R5F Single Core'
        elif 'c66' in name_lower or 'c66ss0' in parts_str:
            return 'C66x DSP'
        return 'Unknown'
    
    def _detect_os_type(self, name: str, parts: tuple) -> str:
        """检测OS类型"""
        parts_str = '/'.join(parts).lower()
        name_lower = name.lower()
        
        if 'freertos' in name_lower or 'freertos' in parts_str:
            return 'FreeRTOS'
        elif 'nortos' in name_lower or 'nortos' in parts_str:
            return 'NoRTOS'
        return 'Unknown'
    
    def _get_os_name(self, name: str, parts: tuple) -> str:
        """获取OS名称"""
        os_type = self._detect_os_type(name, parts)
        if os_type == 'FreeRTOS':
            return 'FreeRTOS'
        elif os_type == 'NoRTOS':
            return 'Bare-metal'
        return 'Unknown'
    
    def _is_system_firmware(self, name: str, parts: tuple) -> bool:
        """是否System固件"""
        return 'system' in name.lower() or 'system' in '/'.join(parts).lower()
    
    def _is_multicore(self, name: str, parts: tuple) -> bool:
        """是否多核"""
        return self._is_system_firmware(name, parts)
    
    def _get_variant_name(self, name: str, parts: tuple) -> str:
        """获取变体名称"""
        parts_str = '/'.join(parts).lower()
        
        if 'system_freertos' in parts_str:
            return 'System FreeRTOS'
        elif 'system_nortos' in parts_str:
            return 'System NoRTOS'
        elif 'r5fss0-0_freertos' in parts_str:
            return 'R5F FreeRTOS'
        elif 'r5fss0-0_nortos' in parts_str:
            return 'R5F NoRTOS'
        elif 'c66ss0_freertos' in parts_str:
            return 'C66x FreeRTOS'
        elif 'c66ss0_nortos' in parts_str:
            return 'C66x NoRTOS'
        
        return os.path.splitext(name)[0]
    
    def _get_relative_dir(self, parts: tuple) -> str:
        """获取相对目录"""
        # 找到项目根目录标志
        for i, part in enumerate(parts):
            if part in ['hello_world', 'mmwave_demo', 'mmw_demo']:
                return '/'.join(parts[i:])
        
        # 返回最后3级目录
        return '/'.join(parts[-3:]) if len(parts) >= 3 else '/'.join(parts)
    
    def _extract_features(self, name: str, parts: tuple, size: int) -> List[str]:
        """提取固件特性"""
        features = []
        
        # OS特性
        if 'freertos' in name.lower() or 'freertos' in '/'.join(parts).lower():
            features.append('多任务支持')
            features.append('RTOS调度')
        elif 'nortos' in name.lower() or 'nortos' in '/'.join(parts).lower():
            features.append('裸机系统')
            features.append('最小开销')
        
        # 多核特性
        if self._is_system_firmware(name, parts):
            features.append('多核协同')
            features.append('RF子系统')
            features.append('DSP加速')
        
        # 大小特性
        size_kb = size / 1024
        if size_kb < 50:
            features.append('体积极小')
        elif size_kb < 100:
            features.append('体积小')
        elif size_kb > 200:
            features.append('功能完整')
        
        return features
    
    def _get_project_name(self, firmwares: List[Dict]) -> str:
        """获取项目名称"""
        if not firmwares:
            return 'Unknown'
        
        # 从第一个固件路径提取
        path = firmwares[0]['file_path']
        parts = Path(path).parts
        
        for part in reversed(parts):
            if part in ['hello_world', 'mmwave_demo', 'mmw_demo']:
                return part
            if 'demo' in part.lower() or 'example' in part.lower():
                return part
        
        return 'Unknown Project'
    
    def _generate_summary(self, firmwares: List[Dict]) -> Dict:
        """生成摘要"""
        if not firmwares:
            return {}
        
        return {
            'total_firmwares': len(firmwares),
            'smallest_size': min(fw['file_size_kb'] for fw in firmwares),
            'largest_size': max(fw['file_size_kb'] for fw in firmwares),
            'has_freertos': any('FreeRTOS' in fw['os_type'] for fw in firmwares),
            'has_nortos': any('NoRTOS' in fw['os_type'] for fw in firmwares),
            'has_system': any(fw['is_system'] for fw in firmwares),
            'variant_types': list(set(fw['variant_name'] for fw in firmwares))
        }


def quick_analyze_firmwares(firmware_paths: List[str]) -> Dict:
    """
    快速分析固件列表（便捷函数）
    
    Args:
        firmware_paths: 固件路径列表
    
    Returns:
        分析结果
    """
    analyzer = FirmwareQuickAnalyzer(firmware_paths)
    return analyzer.analyze()
