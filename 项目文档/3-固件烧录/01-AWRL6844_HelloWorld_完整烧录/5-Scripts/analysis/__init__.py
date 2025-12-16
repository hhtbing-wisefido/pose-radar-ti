"""
项目分析模块

这个模块提供对TI雷达固件项目的完整分析功能
"""

from .project_analyzer import ProjectAnalyzer
from .file_scanner import FileScanner
from .info_extractor import InfoExtractor
from .report_generator import ReportGenerator
from .firmware_quick_analyzer import FirmwareQuickAnalyzer
from .quick_report_generator import QuickReportGenerator

__all__ = [
    'ProjectAnalyzer',
    'FileScanner',
    'InfoExtractor',
    'ReportGenerator',
    'FirmwareQuickAnalyzer',
    'QuickReportGenerator'
]

__version__ = '1.3.0'
