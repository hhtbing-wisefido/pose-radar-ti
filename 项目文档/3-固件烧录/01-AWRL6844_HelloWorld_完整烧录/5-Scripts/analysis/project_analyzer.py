"""
项目分析器 - 核心分析算法

整合文件扫描和信息提取，生成完整的项目分析结果
"""

from .file_scanner import FileScanner
from .info_extractor import InfoExtractor
from typing import Dict, List, Optional
from pathlib import Path


class ProjectAnalyzer:
    """项目分析器 - 核心分析引擎"""
    
    def __init__(self, project_root: str):
        """
        初始化项目分析器
        
        Args:
            project_root: 项目根目录路径
        """
        self.project_root = project_root
        self.scanner = FileScanner(project_root)
        self.extractor = InfoExtractor()
        self.analysis_result = None
    
    def analyze(self) -> Dict:
        """
        执行完整分析
        
        Returns:
            完整分析结果
        """
        # Step 1: 扫描文件
        print("📂 扫描项目文件...")
        scan_result = self.scanner.scan()
        print(f"   找到 {scan_result['statistics']['total_files']} 个文件")
        
        # Step 2: 提取信息
        print("🔍 提取项目信息...")
        extracted_info = self.extractor.extract_from_files(scan_result)
        print(f"   分析了 {len(extracted_info['firmware_info'])} 个固件文件")
        
        # Step 3: 综合分析
        print("📊 生成综合分析...")
        self.analysis_result = self._综合分析(scan_result, extracted_info)
        
        return self.analysis_result
    
    def _综合分析(self, scan_result: Dict, extracted_info: Dict) -> Dict:
        """
        综合分析 - 整合所有信息
        
        Args:
            scan_result: 扫描结果
            extracted_info: 提取的信息
        
        Returns:
            综合分析结果
        """
        analysis = {
            'project_basic': self._分析项目基本信息(scan_result, extracted_info),
            'firmware_analysis': self._分析固件详情(extracted_info['firmware_info']),
            'code_architecture': self._分析代码架构(extracted_info['source_code_info']),
            'syscfg_analysis': self._分析SysConfig(extracted_info['syscfg_info']),
            'structure_comparison': self._对比项目结构(extracted_info),
            'recommendations': self._生成推荐建议(extracted_info)
        }
        
        return analysis
    
    def _分析项目基本信息(self, scan_result: Dict, extracted_info: Dict) -> Dict:
        """分析项目基本信息"""
        structure = extracted_info['project_structure']
        
        return {
            '项目名称': self._get_project_name(),
            '项目类型': structure['project_type'],
            '项目路径': self.project_root,
            '固件数量': len(extracted_info['firmware_info']),
            '源代码文件数': scan_result['statistics']['code_files_count'],
            '配置文件数': scan_result['statistics']['config_files_count'],
            '支持FreeRTOS': structure['has_freertos'],
            '支持NoRTOS': structure['has_nortos'],
            '支持R5F': structure['has_r5f'],
            '支持C66x DSP': structure['has_c66x'],
            '包含System固件': structure['has_system'],
            '固件变体': structure['variants']
        }
    
    def _get_project_name(self) -> str:
        """获取项目名称"""
        path_parts = Path(self.project_root).parts
        for part in reversed(path_parts):
            if part and part not in ['xwrL684x-evm', 'xWRL6844', 'examples']:
                return part
        return 'Unknown'
    
    def _分析固件详情(self, firmware_list: List[Dict]) -> List[Dict]:
        """分析固件详情"""
        analyzed_firmwares = []
        
        for fw in firmware_list:
            analysis = {
                **fw,  # 包含基本信息
                '适用场景': self._get_firmware_use_case(fw),
                '优势': self._get_firmware_advantages(fw),
                '核心分工': self._get_core_division(fw) if fw.get('is_multicore') else None
            }
            analyzed_firmwares.append(analysis)
        
        # 按大小排序
        analyzed_firmwares.sort(key=lambda x: x['file_size'])
        
        return analyzed_firmwares
    
    def _get_firmware_use_case(self, fw: Dict) -> List[str]:
        """获取固件适用场景"""
        use_cases = []
        
        if 'NoRTOS' in fw['os_type']:
            use_cases.extend([
                '简单的单任务应用',
                '对实时性要求极高的场景',
                '资源受限的环境',
                '体积要求小的应用'
            ])
        elif 'FreeRTOS' in fw['os_type']:
            use_cases.extend([
                '需要多任务并发的应用',
                '复杂的系统管理',
                '需要任务调度和同步',
                '中等复杂度的应用'
            ])
        
        if fw.get('is_multicore'):
            use_cases.extend([
                '完整的雷达应用',
                '需要DSP信号处理',
                '复杂的并行计算任务',
                'RF子系统控制'
            ])
        
        return use_cases
    
    def _get_firmware_advantages(self, fw: Dict) -> List[str]:
        """获取固件优势"""
        advantages = []
        
        if 'NoRTOS' in fw['os_type']:
            advantages.extend([
                '体积小（比FreeRTOS版本小30-40%）',
                '启动快',
                '无调度开销',
                '代码简单易懂'
            ])
        elif 'FreeRTOS' in fw['os_type']:
            advantages.extend([
                '支持多任务调度',
                '丰富的同步原语',
                '成熟的RTOS生态',
                '便于复杂应用开发'
            ])
        
        if fw.get('is_multicore'):
            advantages.extend([
                '充分利用硬件资源',
                'DSP加速信号处理',
                'RF子系统完整支持',
                '性能最优'
            ])
        
        return advantages
    
    def _get_core_division(self, fw: Dict) -> Dict:
        """获取多核分工"""
        cores = fw.get('cores', [])
        
        division = {}
        if 'R5F' in cores:
            division['R5F'] = [
                '主控制器',
                '系统管理',
                '外设驱动',
                'CLI命令处理'
            ]
        if 'C66x DSP' in cores:
            division['C66x DSP'] = [
                '信号处理',
                'FFT运算',
                '雷达算法',
                'CFAR检测'
            ]
        if 'RF Subsystem' in cores:
            division['RF Subsystem'] = [
                '雷达射频控制',
                'Chirp生成',
                'ADC数据采集',
                '前端模拟控制'
            ]
        
        return division
    
    def _分析代码架构(self, source_info: Dict) -> Dict:
        """分析代码架构"""
        return {
            '源文件总数': source_info['total_files'],
            '源代码总大小': f"{source_info['total_size'] / 1024:.2f} KB",
            'C文件数量': len(source_info['c_files']),
            'H文件数量': len(source_info['h_files']),
            'main文件': [f['name'] for f in source_info['main_files']],
            '关键函数': source_info['key_functions'][:10] if source_info['key_functions'] else []
        }
    
    def _分析SysConfig(self, syscfg_list: List[Dict]) -> List[Dict]:
        """分析SysConfig配置"""
        analyzed_configs = []
        
        for cfg in syscfg_list:
            analysis = {
                '文件名': cfg['file_name'],
                '配置模块数': len(cfg['modules']),
                '外设数量': len(cfg['peripherals']),
                '使用的模块': cfg['modules'][:10],  # 限制显示数量
                '配置的外设': cfg['peripherals'][:10]
            }
            analyzed_configs.append(analysis)
        
        return analyzed_configs
    
    def _对比项目结构(self, extracted_info: Dict) -> Dict:
        """对比不同固件变体"""
        firmwares = extracted_info['firmware_info']
        
        if len(firmwares) <= 1:
            return {}
        
        # 生成对比表格数据
        comparison = {
            '对比维度': ['固件名称', '大小', '架构', '操作系统', '多核', '适用场景'],
            '固件列表': []
        }
        
        for fw in firmwares:
            comparison['固件列表'].append({
                '固件名称': fw['file_name'],
                '大小': f"{fw['file_size_kb']:.2f} KB",
                '架构': fw['architecture'],
                '操作系统': fw['os_type'],
                '多核': '是' if fw.get('is_multicore') else '否',
                '适用场景': '、'.join(self._get_firmware_use_case(fw)[:2])
            })
        
        return comparison
    
    def _生成推荐建议(self, extracted_info: Dict) -> Dict:
        """生成推荐建议"""
        project_type = extracted_info['project_structure']['project_type']
        
        recommendations = {
            '学习路径': [],
            '开发建议': [],
            '注意事项': []
        }
        
        if project_type == 'Hello World':
            recommendations['学习路径'] = [
                '1️⃣ 从 R5F FreeRTOS 版本开始学习',
                '2️⃣ 理解任务创建和串口通信',
                '3️⃣ 尝试 System 双核版本',
                '4️⃣ 进阶到 mmwave_demo 雷达应用'
            ]
            recommendations['开发建议'] = [
                '优先选择 FreeRTOS 版本（便于扩展）',
                'System 版本适合学习多核协作',
                '可以作为新项目的起点框架'
            ]
        elif project_type == 'mmwave_demo':
            recommendations['学习路径'] = [
                '1️⃣ 理解雷达配置文件参数',
                '2️⃣ 学习信号处理流程',
                '3️⃣ 掌握CFAR检测和AOA估计',
                '4️⃣ 根据应用场景定制算法'
            ]
            recommendations['开发建议'] = [
                '保留核心DPC和校准模块',
                '根据需求调整雷达参数',
                '定制数据处理算法',
                '优化功耗和性能平衡'
            ]
        
        recommendations['注意事项'] = [
            '⚠️ System固件必须配合SBL使用',
            '⚠️ 确保选择匹配的SBL版本',
            '⚠️ 雷达应用需要RF固件补丁'
        ]
        
        return recommendations
    
    def get_summary(self) -> str:
        """
        获取分析摘要（文本格式）
        
        Returns:
            分析摘要文本
        """
        if not self.analysis_result:
            return "请先执行 analyze() 方法"
        
        basic = self.analysis_result['project_basic']
        
        summary = f"""
📊 项目分析摘要
{'='*60}

项目名称: {basic['项目名称']}
项目类型: {basic['项目类型']}
固件数量: {basic['固件数量']} 个
源代码: {basic['源代码文件数']} 个文件
配置文件: {basic['配置文件数']} 个

支持特性:
  • FreeRTOS: {'✅' if basic['支持FreeRTOS'] else '❌'}
  • NoRTOS: {'✅' if basic['支持NoRTOS'] else '❌'}
  • R5F单核: {'✅' if basic['支持R5F'] else '❌'}
  • C66x DSP: {'✅' if basic['支持C66x DSP'] else '❌'}
  • System多核: {'✅' if basic['包含System固件'] else '❌'}

固件变体: {', '.join(basic['固件变体'])}

{'='*60}
"""
        return summary
