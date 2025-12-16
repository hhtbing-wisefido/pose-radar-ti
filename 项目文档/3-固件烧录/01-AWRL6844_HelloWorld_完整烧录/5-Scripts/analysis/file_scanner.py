"""
文件扫描器 - 扫描项目目录中的所有文件

负责递归扫描项目目录，识别和分类文件
"""

import os
from pathlib import Path
from typing import List, Dict, Set
import json


class FileScanner:
    """项目文件扫描器"""
    
    # 需要扫描的文件扩展名
    CODE_EXTENSIONS = {'.c', '.h', '.cpp', '.hpp'}
    CONFIG_EXTENSIONS = {'.syscfg', '.cfg', '.json', '.xml'}
    BUILD_EXTENSIONS = {'.projectspec', '.cmd', 'makefile'}
    FIRMWARE_EXTENSIONS = {'.appimage', '.rig', '.bin', '.out'}
    
    # 需要忽略的目录
    IGNORE_DIRS = {
        'Debug', 'Release', 
        '.git', '.vscode', '__pycache__',
        'node_modules', '.settings'
    }
    
    def __init__(self, project_root: str):
        """
        初始化文件扫描器
        
        Args:
            project_root: 项目根目录路径
        """
        self.project_root = Path(project_root)
        self.scan_result = {
            'code_files': [],        # 源代码文件
            'config_files': [],      # 配置文件
            'build_files': [],       # 构建文件
            'firmware_files': [],    # 固件文件
            'all_files': [],         # 所有文件
            'directory_tree': {}     # 目录树结构
        }
    
    def scan(self) -> Dict:
        """
        扫描项目目录
        
        Returns:
            扫描结果字典
        """
        if not self.project_root.exists():
            raise FileNotFoundError(f"项目路径不存在: {self.project_root}")
        
        # 扫描所有文件
        self._scan_recursive(self.project_root)
        
        # 生成目录树
        self.scan_result['directory_tree'] = self._build_directory_tree()
        
        # 统计信息
        self.scan_result['statistics'] = self._calculate_statistics()
        
        return self.scan_result
    
    def _scan_recursive(self, current_path: Path, depth: int = 0):
        """
        递归扫描目录
        
        Args:
            current_path: 当前扫描路径
            depth: 当前深度（限制最大深度防止过深递归）
        """
        if depth > 10:  # 限制最大深度
            return
        
        try:
            for item in current_path.iterdir():
                # 跳过隐藏文件和忽略目录
                if item.name.startswith('.') or item.name in self.IGNORE_DIRS:
                    continue
                
                if item.is_file():
                    self._classify_file(item)
                elif item.is_dir():
                    self._scan_recursive(item, depth + 1)
        except PermissionError:
            pass  # 跳过无权限的目录
    
    def _classify_file(self, file_path: Path):
        """
        分类文件
        
        Args:
            file_path: 文件路径
        """
        file_info = {
            'path': str(file_path),
            'name': file_path.name,
            'size': file_path.stat().st_size,
            'extension': file_path.suffix,
            'relative_path': str(file_path.relative_to(self.project_root))
        }
        
        # 添加到all_files
        self.scan_result['all_files'].append(file_info)
        
        # 按类型分类
        ext = file_path.suffix.lower()
        if ext in self.CODE_EXTENSIONS:
            self.scan_result['code_files'].append(file_info)
        elif ext in self.CONFIG_EXTENSIONS or file_path.name in {'makefile', 'Makefile'}:
            self.scan_result['config_files'].append(file_info)
        elif ext in self.BUILD_EXTENSIONS or file_path.name.endswith('.cmd'):
            self.scan_result['build_files'].append(file_info)
        elif ext in self.FIRMWARE_EXTENSIONS:
            self.scan_result['firmware_files'].append(file_info)
    
    def _build_directory_tree(self) -> Dict:
        """
        构建目录树结构
        
        Returns:
            目录树字典
        """
        tree = {}
        
        for file_info in self.scan_result['all_files']:
            rel_path = Path(file_info['relative_path'])
            parts = rel_path.parts
            
            # 构建树结构
            current = tree
            for i, part in enumerate(parts[:-1]):  # 处理目录部分
                if part not in current:
                    current[part] = {}
                current = current[part]
            
            # 添加文件
            if len(parts) > 0:
                file_name = parts[-1]
                current[file_name] = file_info
        
        return tree
    
    def _calculate_statistics(self) -> Dict:
        """
        计算统计信息
        
        Returns:
            统计信息字典
        """
        stats = {
            'total_files': len(self.scan_result['all_files']),
            'code_files_count': len(self.scan_result['code_files']),
            'config_files_count': len(self.scan_result['config_files']),
            'firmware_files_count': len(self.scan_result['firmware_files']),
            'total_code_size': sum(f['size'] for f in self.scan_result['code_files']),
            'total_firmware_size': sum(f['size'] for f in self.scan_result['firmware_files']),
        }
        
        return stats
    
    def get_files_by_pattern(self, pattern: str) -> List[Dict]:
        """
        根据模式获取文件列表
        
        Args:
            pattern: 文件名模式（支持通配符）
        
        Returns:
            匹配的文件信息列表
        """
        import fnmatch
        matching_files = []
        
        for file_info in self.scan_result['all_files']:
            if fnmatch.fnmatch(file_info['name'], pattern):
                matching_files.append(file_info)
        
        return matching_files
    
    def find_syscfg_files(self) -> List[Dict]:
        """查找所有syscfg配置文件"""
        return [f for f in self.scan_result['config_files'] if f['extension'] == '.syscfg']
    
    def find_firmware_files(self) -> List[Dict]:
        """查找所有固件文件"""
        return self.scan_result['firmware_files']
    
    def find_source_files(self) -> List[Dict]:
        """查找所有源代码文件"""
        return self.scan_result['code_files']
    
    def save_scan_result(self, output_path: str):
        """
        保存扫描结果到JSON文件
        
        Args:
            output_path: 输出文件路径
        """
        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(self.scan_result, f, indent=2, ensure_ascii=False)
    
    @staticmethod
    def load_scan_result(input_path: str) -> Dict:
        """
        从JSON文件加载扫描结果
        
        Args:
            input_path: 输入文件路径
        
        Returns:
            扫描结果字典
        """
        with open(input_path, 'r', encoding='utf-8') as f:
            return json.load(f)
