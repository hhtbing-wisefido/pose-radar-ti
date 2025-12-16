"""
信息提取器 - 从文件中提取关键信息

负责解析源代码、配置文件、固件文件，提取关键信息
"""

import os
import re
import json
from pathlib import Path
from typing import Dict, List, Optional, Any


class InfoExtractor:
    """信息提取器"""
    
    def __init__(self):
        """初始化信息提取器"""
        self.extracted_info = {}
    
    def extract_from_files(self, scan_result: Dict) -> Dict:
        """
        从扫描结果中提取信息
        
        Args:
            scan_result: FileScanner的扫描结果
        
        Returns:
            提取的信息字典
        """
        info = {
            'firmware_info': [],
            'syscfg_info': [],
            'source_code_info': {},
            'build_info': {},
            'project_structure': {}
        }
        
        # 提取固件信息
        for fw_file in scan_result.get('firmware_files', []):
            fw_info = self.extract_firmware_info(fw_file)
            if fw_info:
                info['firmware_info'].append(fw_info)
        
        # 提取SysConfig信息
        for cfg_file in scan_result.get('config_files', []):
            if cfg_file['extension'] == '.syscfg':
                cfg_info = self.extract_syscfg_info(cfg_file)
                if cfg_info:
                    info['syscfg_info'].append(cfg_info)
        
        # 提取源代码信息
        info['source_code_info'] = self.extract_source_code_summary(
            scan_result.get('code_files', [])
        )
        
        # 分析项目结构
        info['project_structure'] = self.analyze_project_structure(scan_result)
        
        return info
    
    def extract_firmware_info(self, fw_file: Dict) -> Dict:
        """
        提取固件文件信息
        
        Args:
            fw_file: 固件文件信息
        
        Returns:
            固件详细信息
        """
        info = {
            'file_name': fw_file['name'],
            'file_path': fw_file['path'],
            'file_size': fw_file['size'],
            'file_size_kb': fw_file['size'] / 1024,
            'architecture': self._detect_architecture(fw_file['name']),
            'os_type': self._detect_os_type(fw_file['name']),
            'is_system': 'system' in fw_file['name'].lower(),
            'is_multicore': False
        }
        
        # 如果是system固件，可能是多核
        if info['is_system']:
            info['is_multicore'] = True
            info['cores'] = self._detect_cores(fw_file)
        
        return info
    
    def _detect_architecture(self, filename: str) -> str:
        """检测固件架构"""
        filename_lower = filename.lower()
        if 'r5f' in filename_lower:
            return 'R5F (ARM Cortex-R5)'
        elif 'c66' in filename_lower:
            return 'C66x DSP'
        elif 'system' in filename_lower:
            return 'Multi-core System'
        return 'Unknown'
    
    def _detect_os_type(self, filename: str) -> str:
        """检测操作系统类型"""
        filename_lower = filename.lower()
        if 'freertos' in filename_lower:
            return 'FreeRTOS'
        elif 'nortos' in filename_lower:
            return 'NoRTOS (Bare-metal)'
        return 'Unknown'
    
    def _detect_cores(self, fw_file: Dict) -> List[str]:
        """检测多核固件包含的核心"""
        cores = []
        # 查找metaimage配置文件
        fw_dir = Path(fw_file['path']).parent
        metaimage_cfg = fw_dir / 'config' / f"metaimage_cfg.release.json"
        
        if metaimage_cfg.exists():
            try:
                with open(metaimage_cfg, 'r', encoding='utf-8') as f:
                    cfg = json.load(f)
                    for img in cfg.get('buildImages', []):
                        img_path = img.get('buildImagePath', '')
                        if 'r5f' in img_path.lower():
                            cores.append('R5F')
                        elif 'c66' in img_path.lower():
                            cores.append('C66x DSP')
                        elif 'rfs' in img_path.lower() or 'rf' in img_path.lower():
                            cores.append('RF Subsystem')
            except:
                pass
        
        return cores if cores else ['R5F']
    
    def extract_syscfg_info(self, cfg_file: Dict) -> Dict:
        """
        提取SysConfig配置信息
        
        Args:
            cfg_file: 配置文件信息
        
        Returns:
            配置详细信息
        """
        info = {
            'file_name': cfg_file['name'],
            'file_path': cfg_file['path'],
            'modules': [],
            'peripherals': [],
            'memory_regions': []
        }
        
        try:
            with open(cfg_file['path'], 'r', encoding='utf-8') as f:
                content = f.read()
                
                # 提取scripting模块
                modules = re.findall(r'scripting\.addModule\("([^"]+)"\)', content)
                info['modules'] = list(set(modules))
                
                # 提取外设配置
                peripherals = re.findall(r'\.usePeripheral\("([^"]+)"\)', content)
                info['peripherals'] = list(set(peripherals))
        except:
            pass
        
        return info
    
    def extract_source_code_summary(self, code_files: List[Dict]) -> Dict:
        """
        提取源代码摘要信息
        
        Args:
            code_files: 源代码文件列表
        
        Returns:
            源代码摘要信息
        """
        summary = {
            'total_files': len(code_files),
            'total_size': sum(f['size'] for f in code_files),
            'c_files': [],
            'h_files': [],
            'main_files': [],
            'key_functions': []
        }
        
        for file_info in code_files:
            ext = file_info['extension']
            if ext == '.c':
                summary['c_files'].append(file_info)
                if 'main' in file_info['name'].lower():
                    summary['main_files'].append(file_info)
                    # 提取main文件中的关键函数
                    key_funcs = self._extract_key_functions(file_info['path'])
                    summary['key_functions'].extend(key_funcs)
            elif ext == '.h':
                summary['h_files'].append(file_info)
        
        return summary
    
    def _extract_key_functions(self, file_path: str) -> List[str]:
        """提取关键函数名"""
        functions = []
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                # 匹配函数定义（简化版）
                pattern = r'^\s*(void|int|static\s+\w+)\s+(\w+)\s*\('
                matches = re.findall(pattern, content, re.MULTILINE)
                functions = [match[1] for match in matches if match[1] not in ['if', 'while', 'for']]
        except:
            pass
        return functions[:10]  # 限制数量
    
    def analyze_project_structure(self, scan_result: Dict) -> Dict:
        """
        分析项目结构类型
        
        Args:
            scan_result: 扫描结果
        
        Returns:
            项目结构分析
        """
        structure = {
            'project_type': 'Unknown',
            'has_freertos': False,
            'has_nortos': False,
            'has_r5f': False,
            'has_c66x': False,
            'has_system': False,
            'variants': []
        }
        
        # 分析固件文件名模式
        for fw_file in scan_result.get('firmware_files', []):
            name = fw_file['name'].lower()
            
            if 'freertos' in name:
                structure['has_freertos'] = True
            if 'nortos' in name:
                structure['has_nortos'] = True
            if 'r5f' in name:
                structure['has_r5f'] = True
            if 'c66' in name:
                structure['has_c66x'] = True
            if 'system' in name:
                structure['has_system'] = True
            
            # 识别变体
            variant = self._identify_variant(name)
            if variant and variant not in structure['variants']:
                structure['variants'].append(variant)
        
        # 判断项目类型
        if 'hello_world' in str(scan_result.get('all_files', [])[0].get('path', '')).lower():
            structure['project_type'] = 'Hello World'
        elif 'mmwave_demo' in str(scan_result.get('all_files', [])[0].get('path', '')).lower():
            structure['project_type'] = 'mmwave_demo'
        
        return structure
    
    def _identify_variant(self, filename: str) -> Optional[str]:
        """识别固件变体"""
        if 'r5fss0-0_freertos' in filename:
            return 'R5F FreeRTOS'
        elif 'r5fss0-0_nortos' in filename:
            return 'R5F NoRTOS'
        elif 'c66ss0_freertos' in filename:
            return 'C66x FreeRTOS'
        elif 'c66ss0_nortos' in filename:
            return 'C66x NoRTOS'
        elif 'system_freertos' in filename:
            return 'System FreeRTOS'
        elif 'system_nortos' in filename:
            return 'System NoRTOS'
        return None
    
    def extract_radar_config(self, config_file_path: str) -> Dict:
        """
        提取雷达配置文件信息
        
        Args:
            config_file_path: .cfg配置文件路径
        
        Returns:
            雷达配置信息
        """
        config = {
            'file_path': config_file_path,
            'channel_config': {},
            'chirp_config': {},
            'frame_config': {},
            'cfar_config': {}
        }
        
        try:
            with open(config_file_path, 'r', encoding='utf-8') as f:
                for line in f:
                    line = line.strip()
                    if not line or line.startswith('%'):
                        continue
                    
                    parts = line.split()
                    if len(parts) < 2:
                        continue
                    
                    cmd = parts[0]
                    
                    # 解析关键命令
                    if cmd == 'channelCfg':
                        config['channel_config'] = {
                            'rx_channel_mask': parts[1] if len(parts) > 1 else '',
                            'tx_channel_mask': parts[2] if len(parts) > 2 else '',
                            'description': self._parse_channel_mask(parts[1:3]) if len(parts) > 2 else ''
                        }
                    elif cmd == 'chirpComnCfg':
                        config['chirp_config'] = {
                            'start_idx': parts[1] if len(parts) > 1 else '',
                            'end_idx': parts[2] if len(parts) > 2 else '',
                            'num_adc_samples': parts[4] if len(parts) > 4 else '',
                            'chirp_time': parts[6] if len(parts) > 6 else '',
                            'bandwidth_ghz': parts[7] if len(parts) > 7 else ''
                        }
                    elif cmd == 'frameCfg':
                        config['frame_config'] = {
                            'num_chirps': parts[1] if len(parts) > 1 else '',
                            'frame_period_ms': parts[5] if len(parts) > 5 else ''
                        }
                    elif cmd == 'cfarProcCfg':
                        config['cfar_config'] = {
                            'direction': parts[1] if len(parts) > 1 else '',
                            'window_size': parts[3] if len(parts) > 3 else '',
                            'guard_cells': parts[4] if len(parts) > 4 else ''
                        }
        except:
            pass
        
        return config
    
    def _parse_channel_mask(self, masks: List[str]) -> str:
        """解析通道掩码"""
        if len(masks) < 2:
            return ''
        
        try:
            rx_mask = int(masks[0])
            tx_mask = int(masks[1])
            
            rx_count = bin(rx_mask).count('1')
            tx_count = bin(tx_mask).count('1')
            
            return f"{tx_count}T{rx_count}R"
        except:
            return ''
