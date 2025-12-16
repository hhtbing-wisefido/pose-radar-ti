"""
快速报告生成器 - 生成固件对比表格

专注于固件对比展示，快速生成格式化报告
"""

import tkinter as tk
from typing import Dict, List


class QuickReportGenerator:
    """快速报告生成器 - 固件对比表格"""
    
    def __init__(self, analysis_result: Dict):
        """
        初始化报告生成器
        
        Args:
            analysis_result: FirmwareQuickAnalyzer的分析结果
        """
        self.analysis = analysis_result
    
    def generate_comparison_report(self, text_widget: tk.Text):
        """
        生成固件对比报告
        
        Args:
            text_widget: Tkinter Text Widget
        """
        # 配置样式
        self._configure_styles(text_widget)
        
        # 清空内容
        text_widget.delete('1.0', tk.END)
        
        # 插入报告
        self._insert_title(text_widget)
        self._insert_summary(text_widget)
        self._insert_comparison_table(text_widget)
        self._insert_detailed_list(text_widget)
        self._insert_recommendations(text_widget)
    
    def _configure_styles(self, tw: tk.Text):
        """配置文本样式"""
        tw.tag_config("title", 
                     font=("Microsoft YaHei UI", 14, "bold"), 
                     foreground="#2c3e50")
        tw.tag_config("subtitle", 
                     font=("Microsoft YaHei UI", 11, "bold"), 
                     foreground="#3498db")
        tw.tag_config("section", 
                     font=("Microsoft YaHei UI", 10, "bold"), 
                     foreground="#27ae60")
        tw.tag_config("highlight", 
                     foreground="#e74c3c", 
                     font=("Microsoft YaHei UI", 9, "bold"))
        tw.tag_config("table_header", 
                     font=("Consolas", 9, "bold"), 
                     background="#ecf0f1")
        tw.tag_config("table_row", 
                     font=("Consolas", 9))
        tw.tag_config("line", 
                     foreground="#95a5a6")
    
    def _insert_title(self, tw: tk.Text):
        """插入标题"""
        project_name = self.analysis.get('project_name', 'Unknown')
        tw.insert(tk.END, f"📊 {project_name} 固件对比分析\n\n", "title")
        tw.insert(tk.END, "="*90 + "\n\n", "line")
    
    def _insert_summary(self, tw: tk.Text):
        """插入摘要"""
        summary = self.analysis.get('summary', {})
        
        tw.insert(tk.END, "📦 项目概览\n\n", "subtitle")
        tw.insert(tk.END, f"固件总数: {summary.get('total_firmwares', 0)} 个\n")
        tw.insert(tk.END, f"大小范围: {summary.get('smallest_size', 0):.1f} KB ~ {summary.get('largest_size', 0):.1f} KB\n\n")
        
        tw.insert(tk.END, "支持特性:\n", "section")
        tw.insert(tk.END, f"  • FreeRTOS: {'✅ 支持' if summary.get('has_freertos') else '❌ 不支持'}\n")
        tw.insert(tk.END, f"  • NoRTOS: {'✅ 支持' if summary.get('has_nortos') else '❌ 不支持'}\n")
        tw.insert(tk.END, f"  • System多核: {'✅ 支持' if summary.get('has_system') else '❌ 不支持'}\n\n")
        
        if summary.get('variant_types'):
            tw.insert(tk.END, f"固件变体: {', '.join(summary['variant_types'])}\n\n")
        
        tw.insert(tk.END, "—"*90 + "\n\n", "line")
    
    def _insert_comparison_table(self, tw: tk.Text):
        """插入对比表格"""
        firmwares = self.analysis.get('firmwares', [])
        
        tw.insert(tk.END, "📋 固件对比表\n\n", "subtitle")
        
        # 表头
        header = f"{'#':<4}{'固件名称':<35}{'大小':>12}{'架构':<22}{'OS':<15}{'类型':<12}\n"
        tw.insert(tk.END, header, "table_header")
        tw.insert(tk.END, "─"*100 + "\n", "line")
        
        # 表内容
        for idx, fw in enumerate(firmwares, 1):
            marker = "⭐" if idx == 1 else f"{idx}."
            name = fw['file_name'][:33] + '..' if len(fw['file_name']) > 35 else fw['file_name']
            size = f"{fw['file_size_kb']:.1f} KB"
            arch = fw['core_type'][:20]
            os_type = fw['os_name'][:13]
            multicore = "多核" if fw['is_multicore'] else "单核"
            
            row = f"{marker:<4}{name:<35}{size:>12}{arch:<22}{os_type:<15}{multicore:<12}\n"
            tw.insert(tk.END, row, "table_row")
        
        tw.insert(tk.END, "\n—"*90 + "\n\n", "line")
    
    def _insert_detailed_list(self, tw: tk.Text):
        """插入详细列表"""
        firmwares = self.analysis.get('firmwares', [])
        
        tw.insert(tk.END, f"📁 固件详细信息 (共{len(firmwares)}个)\n\n", "subtitle")
        
        for idx, fw in enumerate(firmwares, 1):
            # 固件标题
            marker = "⭐" if idx == 1 else f"{idx}."
            tw.insert(tk.END, f"{marker} {fw['file_name']}\n", "section")
            tw.insert(tk.END, "   " + "─"*80 + "\n", "line")
            
            # 基本信息
            tw.insert(tk.END, f"   文件大小: {fw['file_size_kb']:.2f} KB ({fw['file_size']:,} 字节)\n")
            tw.insert(tk.END, f"   架构类型: {fw['architecture']}\n")
            tw.insert(tk.END, f"   核心配置: {fw['core_type']}\n")
            tw.insert(tk.END, f"   操作系统: {fw['os_type']}\n")
            tw.insert(tk.END, f"   变体名称: {fw['variant_name']}\n")
            tw.insert(tk.END, f"   多核支持: {'是 (R5F + C66x + RF)' if fw['is_multicore'] else '否'}\n")
            
            # 特性
            if fw.get('features'):
                tw.insert(tk.END, f"\n   特性标签:\n")
                for feature in fw['features']:
                    tw.insert(tk.END, f"     • {feature}\n")
            
            # 路径
            tw.insert(tk.END, f"\n   相对路径: {fw['relative_dir']}\n")
            tw.insert(tk.END, f"   完整路径: {fw['file_path']}\n")
            
            tw.insert(tk.END, "\n")
        
        tw.insert(tk.END, "—"*90 + "\n\n", "line")
    
    def _insert_recommendations(self, tw: tk.Text):
        """插入推荐建议"""
        firmwares = self.analysis.get('firmwares', [])
        summary = self.analysis.get('summary', {})
        
        tw.insert(tk.END, "💡 选择建议\n\n", "subtitle")
        
        # 根据固件特点给建议
        if summary.get('has_freertos'):
            tw.insert(tk.END, "学习入门:\n", "section")
            tw.insert(tk.END, "  1️⃣ 建议从 ", "highlight")
            
            # 找到最小的FreeRTOS固件
            freertos_fw = [fw for fw in firmwares if 'FreeRTOS' in fw['os_type']]
            if freertos_fw:
                smallest_freertos = min(freertos_fw, key=lambda x: x['file_size'])
                tw.insert(tk.END, f"{smallest_freertos['variant_name']}")
                tw.insert(tk.END, " 开始\n")
                tw.insert(tk.END, f"     优势: 多任务支持、成熟的RTOS生态\n")
            tw.insert(tk.END, "\n")
        
        if summary.get('has_system'):
            tw.insert(tk.END, "高级应用:\n", "section")
            tw.insert(tk.END, "  2️⃣ 需要完整功能时选择 ", "highlight")
            
            # 找System固件
            system_fw = [fw for fw in firmwares if fw['is_system']]
            if system_fw:
                tw.insert(tk.END, f"{system_fw[0]['variant_name']}")
                tw.insert(tk.END, "\n")
                tw.insert(tk.END, "     优势: R5F + C66x DSP + RF完整支持\n")
                tw.insert(tk.END, "     适用: 雷达应用、信号处理、多核协同\n")
            tw.insert(tk.END, "\n")
        
        if summary.get('has_nortos'):
            tw.insert(tk.END, "资源受限场景:\n", "section")
            tw.insert(tk.END, "  3️⃣ 对体积和性能要求极高时选择 ", "highlight")
            
            # 找到最小的NoRTOS固件
            nortos_fw = [fw for fw in firmwares if 'NoRTOS' in fw['os_type']]
            if nortos_fw:
                smallest_nortos = min(nortos_fw, key=lambda x: x['file_size'])
                tw.insert(tk.END, f"{smallest_nortos['variant_name']}")
                tw.insert(tk.END, "\n")
                tw.insert(tk.END, f"     优势: 体积小 ({smallest_nortos['file_size_kb']:.1f} KB)、无OS开销\n")
            tw.insert(tk.END, "\n")
        
        # 对比建议
        tw.insert(tk.END, "对比要点:\n", "section")
        tw.insert(tk.END, "  • 体积: ")
        
        smallest = min(firmwares, key=lambda x: x['file_size'])
        largest = max(firmwares, key=lambda x: x['file_size'])
        
        size_diff_percent = ((largest['file_size'] - smallest['file_size']) / smallest['file_size']) * 100
        tw.insert(tk.END, f"{smallest['variant_name']} 最小，{largest['variant_name']} 最大（相差 {size_diff_percent:.0f}%）\n")
        
        tw.insert(tk.END, "  • 功能: System版本功能最完整，单核版本更简洁\n")
        tw.insert(tk.END, "  • 开发: FreeRTOS便于调试，NoRTOS性能更优\n\n")
        
        # 注意事项
        tw.insert(tk.END, "⚠️  注意事项\n", "highlight")
        tw.insert(tk.END, "  • System固件必须配合正确的SBL使用\n")
        tw.insert(tk.END, "  • 不同固件的内存布局可能不同\n")
        tw.insert(tk.END, "  • 雷达功能需要RF固件补丁支持\n")
