#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
SDK配置文件扫描和分析工具
扫描Ti SDK中的所有雷达配置文件并进行分析分类
"""

import os
import re
from pathlib import Path
from typing import List, Dict
from collections import defaultdict

class ConfigScanner:
    """配置文件扫描器"""
    
    def __init__(self):
        self.config_files = []
        self.stats = defaultdict(int)
    
    def scan_directory(self, root_dir: str, recursive: bool = True) -> List[Dict]:
        """
        扫描目录查找配置文件
        
        Args:
            root_dir: 根目录
            recursive: 是否递归搜索
            
        Returns:
            配置文件信息列表
        """
        root_path = Path(root_dir)
        
        if not root_path.exists():
            print(f"❌ 目录不存在: {root_dir}")
            return []
        
        print(f"🔍 扫描目录: {root_dir}")
        print(f"   递归模式: {'是' if recursive else '否'}")
        
        pattern = '**/*.cfg' if recursive else '*.cfg'
        
        for cfg_file in root_path.glob(pattern):
            if self._is_radar_config(cfg_file):
                info = self._extract_info(cfg_file)
                self.config_files.append(info)
                self._update_stats(info)
        
        print(f"✅ 找到 {len(self.config_files)} 个配置文件")
        
        return self.config_files
    
    def _is_radar_config(self, file_path: Path) -> bool:
        """判断是否为雷达配置文件"""
        # 排除RTOS配置文件（.cfg但是是XDC配置）
        if 'sysbios' in str(file_path).lower():
            return False
        if 'rtos' in str(file_path).lower() and 'chirp' not in str(file_path).lower():
            return False
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read(1000)  # 只读前1000字符
            
            # 检查是否包含雷达配置命令
            radar_keywords = [
                'profileCfg',
                'frameCfg',
                'channelCfg',
                'chirpCfg',
                'sensorStart'
            ]
            
            return any(keyword in content for keyword in radar_keywords)
        
        except Exception:
            return False
    
    def _extract_info(self, file_path: Path) -> Dict:
        """提取配置文件信息"""
        info = {
            'path': str(file_path),
            'name': file_path.name,
            'size': file_path.stat().st_size,
            'directory': file_path.parent.name,
            'relative_path': None,
            'application': 'unknown',
            'chip': 'unknown',
            'mode': 'unknown',
            'features': [],
            'channels': {},
            'profile': {},
            'frame': {}
        }
        
        # 从路径提取信息
        path_parts = file_path.parts
        for part in path_parts:
            # 检测应用类型
            if 'people_tracking' in part.lower():
                info['application'] = '3D人员跟踪'
            elif 'occupancy' in part.lower():
                info['application'] = '占用检测'
            elif 'vital' in part.lower():
                info['application'] = '生命体征'
            elif 'gesture' in part.lower():
                info['application'] = '手势识别'
            elif 'in_cabin' in part.lower() or 'cabin' in part.lower():
                info['application'] = '车内感知'
            elif 'out_of_box' in part.lower():
                info['application'] = '开箱即用'
            elif 'level' in part.lower():
                info['application'] = '液位检测'
            
            # 检测芯片型号
            if '68' in part and ('xx' in part.lower() or '43' in part or '44' in part):
                info['chip'] = part
        
        # 从文件名提取信息
        filename_lower = file_path.name.lower()
        
        # 检测模式
        if '2d' in filename_lower:
            info['mode'] = '2D'
        elif '3d' in filename_lower:
            info['mode'] = '3D'
        elif 'tdm' in filename_lower:
            info['mode'] = 'TDM'
        
        # 检测特性
        if 'fps' in filename_lower:
            fps_match = re.search(r'(\d+)fps', filename_lower)
            if fps_match:
                info['features'].append(f"{fps_match.group(1)} FPS")
        
        if 'long' in filename_lower and 'range' in filename_lower:
            info['features'].append('长距离')
        
        if 'short' in filename_lower and 'range' in filename_lower:
            info['features'].append('短距离')
        
        if 'high' in filename_lower and 'res' in filename_lower:
            info['features'].append('高分辨率')
        
        if 'low' in filename_lower and 'power' in filename_lower:
            info['features'].append('低功耗')
        
        # 解析配置内容
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
            
            # 解析channelCfg
            match = re.search(r'channelCfg\s+(\d+)\s+(\d+)\s+(\d+)', content)
            if match:
                rx_mask = int(match.group(1))
                tx_mask = int(match.group(2))
                info['channels'] = {
                    'rx': self._count_bits(rx_mask),
                    'tx': self._count_bits(tx_mask),
                    'virtual': self._count_bits(rx_mask) * self._count_bits(tx_mask)
                }
            
            # 解析profileCfg（简化）
            match = re.search(
                r'profileCfg\s+\d+\s+([\d.]+)\s+[\d.]+\s+[\d.]+\s+([\d.]+)',
                content
            )
            if match:
                info['profile'] = {
                    'start_freq': float(match.group(1)),
                    'ramp_time': float(match.group(2))
                }
            
            # 解析frameCfg（简化）
            match = re.search(
                r'frameCfg\s+\d+\s+\d+\s+(\d+)\s+\d+\s+([\d.]+)',
                content
            )
            if match:
                info['frame'] = {
                    'num_chirps': int(match.group(1)),
                    'frame_period': float(match.group(2))
                }
        
        except Exception as e:
            print(f"⚠️ 解析失败 {file_path.name}: {e}")
        
        return info
    
    def _count_bits(self, mask: int) -> int:
        """计算位掩码中1的个数"""
        count = 0
        while mask:
            count += mask & 1
            mask >>= 1
        return count
    
    def _update_stats(self, info: Dict):
        """更新统计信息"""
        self.stats['total'] += 1
        self.stats[f"app_{info['application']}"] += 1
        self.stats[f"chip_{info['chip']}"] += 1
        self.stats[f"mode_{info['mode']}"] += 1
    
    def print_summary(self):
        """打印扫描摘要"""
        print("\n" + "=" * 60)
        print("📊 扫描摘要")
        print("=" * 60)
        
        print(f"\n总配置文件数: {self.stats['total']}")
        
        # 按应用分类
        print("\n按应用分类:")
        app_stats = {k: v for k, v in self.stats.items() if k.startswith('app_')}
        for app, count in sorted(app_stats.items(), key=lambda x: x[1], reverse=True):
            app_name = app.replace('app_', '')
            print(f"  • {app_name}: {count}")
        
        # 按芯片分类
        print("\n按芯片分类:")
        chip_stats = {k: v for k, v in self.stats.items() if k.startswith('chip_')}
        for chip, count in sorted(chip_stats.items(), key=lambda x: x[1], reverse=True):
            chip_name = chip.replace('chip_', '')
            print(f"  • {chip_name}: {count}")
        
        # 按模式分类
        print("\n按模式分类:")
        mode_stats = {k: v for k, v in self.stats.items() if k.startswith('mode_')}
        for mode, count in sorted(mode_stats.items(), key=lambda x: x[1], reverse=True):
            mode_name = mode.replace('mode_', '')
            print(f"  • {mode_name}: {count}")
        
        print("\n" + "=" * 60)
    
    def print_detailed_list(self, show_config: bool = False):
        """打印详细列表"""
        print("\n" + "=" * 60)
        print("📋 配置文件详细列表")
        print("=" * 60)
        
        # 按应用分组
        apps = defaultdict(list)
        for cfg in self.config_files:
            apps[cfg['application']].append(cfg)
        
        for app, configs in sorted(apps.items()):
            print(f"\n【{app}】({len(configs)} 个)")
            print("-" * 60)
            
            for cfg in configs:
                print(f"\n  📄 {cfg['name']}")
                print(f"     路径: {cfg['directory']}/")
                print(f"     芯片: {cfg['chip']}")
                print(f"     模式: {cfg['mode']}")
                
                if cfg['features']:
                    print(f"     特性: {', '.join(cfg['features'])}")
                
                if show_config:
                    if cfg['channels']:
                        ch = cfg['channels']
                        print(f"     天线: {ch['rx']}RX + {ch['tx']}TX = {ch['virtual']}虚拟")
                    
                    if cfg['frame']:
                        fr = cfg['frame']
                        print(f"     帧配置: {fr.get('num_chirps', 'N/A')} chirps, "
                              f"{fr.get('frame_period', 'N/A')} ms周期")
    
    def export_to_csv(self, output_file: str):
        """导出到CSV文件"""
        import csv
        
        with open(output_file, 'w', newline='', encoding='utf-8') as f:
            fieldnames = [
                '文件名', '应用', '芯片', '模式', '特性',
                'RX', 'TX', '虚拟天线', 'Chirps数', '帧周期(ms)', '路径'
            ]
            writer = csv.DictWriter(f, fieldnames=fieldnames)
            writer.writeheader()
            
            for cfg in self.config_files:
                writer.writerow({
                    '文件名': cfg['name'],
                    '应用': cfg['application'],
                    '芯片': cfg['chip'],
                    '模式': cfg['mode'],
                    '特性': ', '.join(cfg['features']),
                    'RX': cfg['channels'].get('rx', ''),
                    'TX': cfg['channels'].get('tx', ''),
                    '虚拟天线': cfg['channels'].get('virtual', ''),
                    'Chirps数': cfg['frame'].get('num_chirps', ''),
                    '帧周期(ms)': cfg['frame'].get('frame_period', ''),
                    '路径': cfg['path']
                })
        
        print(f"\n✅ 已导出到: {output_file}")


def main():
    """主函数"""
    import sys
    
    print("=" * 60)
    print("📡 SDK配置文件扫描工具")
    print("=" * 60)
    
    # 默认扫描路径
    default_paths = [
        r"C:\ti\mmwave_sdk_03_06_01_00_LTS",
        r"C:\ti\MMWAVE_L_SDK_06_01_00_01",
        r"C:\ti\mmwave_industrial_toolbox_4_12_0"
    ]
    
    scan_dirs = []
    
    if len(sys.argv) > 1:
        # 使用命令行参数
        scan_dirs = sys.argv[1:]
    else:
        # 使用默认路径（存在的）
        scan_dirs = [p for p in default_paths if os.path.exists(p)]
    
    if not scan_dirs:
        print("\n❌ 未找到SDK目录")
        print("\n用法: python config_scanner.py [目录1] [目录2] ...")
        print("\n默认搜索路径:")
        for path in default_paths:
            print(f"  - {path}")
        return
    
    print(f"\n将扫描以下目录:")
    for dir_path in scan_dirs:
        print(f"  • {dir_path}")
    
    # 创建扫描器
    scanner = ConfigScanner()
    
    # 扫描所有目录
    for dir_path in scan_dirs:
        scanner.scan_directory(dir_path, recursive=True)
    
    # 打印摘要
    scanner.print_summary()
    
    # 打印详细列表
    scanner.print_detailed_list(show_config=True)
    
    # 导出CSV
    output_csv = 'config_files_list.csv'
    scanner.export_to_csv(output_csv)


if __name__ == "__main__":
    main()
