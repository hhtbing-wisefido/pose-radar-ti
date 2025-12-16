"""
报告生成器 - 将分析结果格式化为可展示的报告

生成适合在GUI中显示的格式化文本报告
"""

from typing import Dict, List
import tkinter as tk


class ReportGenerator:
    """报告生成器"""
    
    def __init__(self, analysis_result: Dict):
        """
        初始化报告生成器
        
        Args:
            analysis_result: ProjectAnalyzer的分析结果
        """
        self.analysis = analysis_result
    
    def generate_full_report_for_text_widget(self, text_widget: tk.Text):
        """
        生成完整报告并插入到Text Widget中
        
        Args:
            text_widget: Tkinter Text Widget
        """
        # 配置标签样式
        self._configure_text_tags(text_widget)
        
        # 清空现有内容
        text_widget.delete('1.0', tk.END)
        
        # 插入报告内容
        self._insert_title(text_widget)
        self._insert_basic_info(text_widget)
        self._insert_firmware_analysis(text_widget)
        self._insert_code_architecture(text_widget)
        self._insert_syscfg_analysis(text_widget)
        self._insert_structure_comparison(text_widget)
        self._insert_recommendations(text_widget)
    
    def _configure_text_tags(self, text_widget: tk.Text):
        """配置文本标签样式"""
        text_widget.tag_config("title", 
                              font=("Microsoft YaHei UI", 14, "bold"), 
                              foreground="#2c3e50")
        text_widget.tag_config("subtitle", 
                              font=("Microsoft YaHei UI", 11, "bold"), 
                              foreground="#3498db")
        text_widget.tag_config("section", 
                              font=("Microsoft YaHei UI", 10, "bold"), 
                              foreground="#27ae60")
        text_widget.tag_config("line", 
                              foreground="#95a5a6")
        text_widget.tag_config("highlight", 
                              foreground="#e74c3c", 
                              font=("Microsoft YaHei UI", 9, "bold"))
        text_widget.tag_config("code", 
                              font=("Consolas", 9), 
                              background="#f4f4f4")
    
    def _insert_title(self, tw: tk.Text):
        """插入标题"""
        basic = self.analysis['project_basic']
        tw.insert(tk.END, f"📊 {basic['项目名称']} 项目完整分析\n\n", "title")
        tw.insert(tk.END, "="*80 + "\n\n", "line")
    
    def _insert_basic_info(self, tw: tk.Text):
        """插入基本信息"""
        basic = self.analysis['project_basic']
        
        tw.insert(tk.END, "🎯 项目基本信息\n\n", "subtitle")
        tw.insert(tk.END, f"项目名称: {basic['项目名称']}\n")
        tw.insert(tk.END, f"项目类型: {basic['项目类型']}\n")
        tw.insert(tk.END, f"固件数量: {basic['固件数量']} 个\n")
        tw.insert(tk.END, f"源代码文件: {basic['源代码文件数']} 个\n")
        tw.insert(tk.END, f"配置文件: {basic['配置文件数']} 个\n\n")
        
        tw.insert(tk.END, "支持特性:\n", "section")
        tw.insert(tk.END, f"  • FreeRTOS: {'✅ 支持' if basic['支持FreeRTOS'] else '❌ 不支持'}\n")
        tw.insert(tk.END, f"  • NoRTOS: {'✅ 支持' if basic['支持NoRTOS'] else '❌ 不支持'}\n")
        tw.insert(tk.END, f"  • R5F单核: {'✅ 支持' if basic['支持R5F'] else '❌ 不支持'}\n")
        tw.insert(tk.END, f"  • C66x DSP: {'✅ 支持' if basic['支持C66x DSP'] else '❌ 不支持'}\n")
        tw.insert(tk.END, f"  • System多核: {'✅ 支持' if basic['包含System固件'] else '❌ 不支持'}\n\n")
        
        if basic['固件变体']:
            tw.insert(tk.END, f"固件变体: {', '.join(basic['固件变体'])}\n\n")
        
        tw.insert(tk.END, "—"*80 + "\n\n", "line")
    
    def _insert_firmware_analysis(self, tw: tk.Text):
        """插入固件分析"""
        firmwares = self.analysis['firmware_analysis']
        
        tw.insert(tk.END, f"📦 固件详细分析 (共{len(firmwares)}个)\n\n", "subtitle")
        
        for idx, fw in enumerate(firmwares, 1):
            # 固件标题
            marker = "⭐" if idx == 1 else f"{idx}."
            tw.insert(tk.END, f"{marker} {fw['file_name']}\n", "section")
            tw.insert(tk.END, "─"*60 + "\n", "line")
            
            # 基本信息
            tw.insert(tk.END, f"大小: {fw['file_size_kb']:.2f} KB ({fw['file_size']:,} 字节)\n")
            tw.insert(tk.END, f"架构: {fw['architecture']}\n")
            tw.insert(tk.END, f"操作系统: {fw['os_type']}\n")
            
            if fw.get('is_multicore'):
                tw.insert(tk.END, "类型: ", "highlight")
                tw.insert(tk.END, "多核System固件\n")
                if fw.get('cores'):
                    tw.insert(tk.END, f"包含核心: {', '.join(fw['cores'])}\n")
            
            # 适用场景
            if fw.get('适用场景'):
                tw.insert(tk.END, "\n适用场景:\n")
                for scenario in fw['适用场景']:
                    tw.insert(tk.END, f"  ✓ {scenario}\n")
            
            # 优势
            if fw.get('优势'):
                tw.insert(tk.END, "\n优势:\n")
                for adv in fw['优势']:
                    tw.insert(tk.END, f"  • {adv}\n")
            
            # 核心分工
            if fw.get('核心分工'):
                tw.insert(tk.END, "\n核心分工:\n")
                for core, tasks in fw['核心分工'].items():
                    tw.insert(tk.END, f"  {core}:\n")
                    for task in tasks:
                        tw.insert(tk.END, f"    - {task}\n")
            
            tw.insert(tk.END, "\n")
        
        tw.insert(tk.END, "—"*80 + "\n\n", "line")
    
    def _insert_code_architecture(self, tw: tk.Text):
        """插入代码架构分析"""
        code = self.analysis['code_architecture']
        
        tw.insert(tk.END, "🏗️ 代码架构\n\n", "subtitle")
        tw.insert(tk.END, f"源文件总数: {code['源文件总数']} 个\n")
        tw.insert(tk.END, f"源代码大小: {code['源代码总大小']}\n")
        tw.insert(tk.END, f"C文件: {code['C文件数量']} 个\n")
        tw.insert(tk.END, f"H文件: {code['H文件数量']} 个\n\n")
        
        if code['main文件']:
            tw.insert(tk.END, "入口文件:\n", "section")
            for main_file in code['main文件']:
                tw.insert(tk.END, f"  • {main_file}\n")
            tw.insert(tk.END, "\n")
        
        if code['关键函数']:
            tw.insert(tk.END, "关键函数:\n", "section")
            for func in code['关键函数'][:10]:
                tw.insert(tk.END, f"  • {func}()\n")
            tw.insert(tk.END, "\n")
        
        tw.insert(tk.END, "—"*80 + "\n\n", "line")
    
    def _insert_syscfg_analysis(self, tw: tk.Text):
        """插入SysConfig分析"""
        syscfg_list = self.analysis['syscfg_analysis']
        
        if not syscfg_list:
            return
        
        tw.insert(tk.END, f"⚙️ SysConfig配置分析 (共{len(syscfg_list)}个)\n\n", "subtitle")
        
        for cfg in syscfg_list:
            tw.insert(tk.END, f"📄 {cfg['文件名']}\n", "section")
            tw.insert(tk.END, f"   配置模块: {cfg['配置模块数']} 个\n")
            tw.insert(tk.END, f"   使用外设: {cfg['外设数量']} 个\n")
            
            if cfg['使用的模块']:
                tw.insert(tk.END, f"   模块: {', '.join(cfg['使用的模块'][:5])}\n")
            
            tw.insert(tk.END, "\n")
        
        tw.insert(tk.END, "—"*80 + "\n\n", "line")
    
    def _insert_structure_comparison(self, tw: tk.Text):
        """插入结构对比"""
        comparison = self.analysis.get('structure_comparison', {})
        
        if not comparison or not comparison.get('固件列表'):
            return
        
        tw.insert(tk.END, "📊 固件对比表\n\n", "subtitle")
        
        # 表头
        tw.insert(tk.END, f"{'固件名称':<40} {'大小':<15} {'架构':<20} {'OS':<15}\n", "code")
        tw.insert(tk.END, "─"*90 + "\n", "line")
        
        # 表内容
        for fw in comparison['固件列表']:
            name = fw['固件名称'][:38]
            size = fw['大小']
            arch = fw['架构'][:18]
            os_type = fw['操作系统'][:13]
            tw.insert(tk.END, f"{name:<40} {size:<15} {arch:<20} {os_type:<15}\n", "code")
        
        tw.insert(tk.END, "\n—"*80 + "\n\n", "line")
    
    def _insert_recommendations(self, tw: tk.Text):
        """插入推荐建议"""
        rec = self.analysis['recommendations']
        
        tw.insert(tk.END, "💡 推荐建议\n\n", "subtitle")
        
        if rec.get('学习路径'):
            tw.insert(tk.END, "学习路径:\n", "section")
            for step in rec['学习路径']:
                tw.insert(tk.END, f"{step}\n")
            tw.insert(tk.END, "\n")
        
        if rec.get('开发建议'):
            tw.insert(tk.END, "开发建议:\n", "section")
            for advice in rec['开发建议']:
                tw.insert(tk.END, f"  • {advice}\n")
            tw.insert(tk.END, "\n")
        
        if rec.get('注意事项'):
            tw.insert(tk.END, "注意事项:\n", "highlight")
            for note in rec['注意事项']:
                tw.insert(tk.END, f"{note}\n")
            tw.insert(tk.END, "\n")
    
    def generate_markdown_report(self) -> str:
        """
        生成Markdown格式的报告
        
        Returns:
            Markdown格式的报告文本
        """
        basic = self.analysis['project_basic']
        
        md = f"""# 📊 {basic['项目名称']} 项目完整分析

> **分析日期**: 自动生成
> **项目类型**: {basic['项目类型']}
> **固件数量**: {basic['固件数量']}

---

## 🎯 项目基本信息

| 属性 | 值 |
|------|-----|
| 项目名称 | {basic['项目名称']} |
| 项目类型 | {basic['项目类型']} |
| 固件数量 | {basic['固件数量']} 个 |
| 源代码文件 | {basic['源代码文件数']} 个 |
| 配置文件 | {basic['配置文件数']} 个 |

### 支持特性

- FreeRTOS: {'✅ 支持' if basic['支持FreeRTOS'] else '❌ 不支持'}
- NoRTOS: {'✅ 支持' if basic['支持NoRTOS'] else '❌ 不支持'}
- R5F单核: {'✅ 支持' if basic['支持R5F'] else '❌ 不支持'}
- C66x DSP: {'✅ 支持' if basic['支持C66x DSP'] else '❌ 不支持'}
- System多核: {'✅ 支持' if basic['包含System固件'] else '❌ 不支持'}

---

## 📦 固件详细分析

"""
        
        # 固件分析
        for idx, fw in enumerate(self.analysis['firmware_analysis'], 1):
            md += f"\n### {idx}. {fw['file_name']}\n\n"
            md += f"- **大小**: {fw['file_size_kb']:.2f} KB\n"
            md += f"- **架构**: {fw['architecture']}\n"
            md += f"- **操作系统**: {fw['os_type']}\n"
            
            if fw.get('适用场景'):
                md += "\n**适用场景**:\n"
                for scenario in fw['适用场景']:
                    md += f"- {scenario}\n"
            
            if fw.get('优势'):
                md += "\n**优势**:\n"
                for adv in fw['优势']:
                    md += f"- {adv}\n"
            
            md += "\n"
        
        # 推荐建议
        rec = self.analysis['recommendations']
        md += "\n---\n\n## 💡 推荐建议\n\n"
        
        if rec.get('学习路径'):
            md += "### 学习路径\n\n"
            for step in rec['学习路径']:
                md += f"{step}\n"
            md += "\n"
        
        if rec.get('开发建议'):
            md += "### 开发建议\n\n"
            for advice in rec['开发建议']:
                md += f"- {advice}\n"
            md += "\n"
        
        return md
    
    def save_markdown_report(self, output_path: str):
        """
        保存Markdown报告到文件
        
        Args:
            output_path: 输出文件路径
        """
        md = self.generate_markdown_report()
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(md)
