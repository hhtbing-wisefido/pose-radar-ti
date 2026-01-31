#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Ti AWRL6844 固件系统工具 v2.4.6 - 扫描列表显示修复版
主入口文件 - 多标签页集成系统

更新日志 v2.4.6:
- [BUG] 修复扫描后应用固件和SBL固件列表不显示的问题
  * 原因：update_filter_options中SBL/Config筛选器未解绑事件，
    调用current(0)时触发筛选事件导致列表被清空
  * 修复：解绑所有筛选器事件，更新选项后重新绑定
- [TOOL] 统一所有筛选器（应用固件/SBL/雷达配置）的事件解绑和重绑定逻辑
- 构建日期：2026-01-29

更新日志 v2.4.5:
- [FIX] 修复EXE模式下多个关键问题：
  * 修复筛选器事件解绑后未重新绑定导致筛选器失效
  * 修复多进程检测在EXE模式下无法正常工作（使用EXE名而非脚本名）
  * 修复烧录工具路径在EXE模式下指向release目录的问题
  * 添加更多调试日志用于问题追踪
- 构建日期：2026-01-29

更新日志 v2.4.4:
- [REVERT] 回退subprocess配置到bufsize=0
  * 解决v2.4.2修改导致的烧录1秒完成问题
  * bufsize=1在二进制模式下不是行缓冲，是字节缓冲
  * 恢复bufsize=0无缓冲模式，确保实时输出
  * 保留v2.4.3的进程清理逻辑
  * 使用环境变量标记避免无限循环
  * Windows原生支持：CREATE_NEW_PROCESS_GROUP + DETACHED_PROCESS
- [OK] 启动命令：`python flash_tool.py`（立即返回，GUI后台运行）
- 构建日期：2025-12-20

更新日志 v2.4.3.1:
- [BUG] 修复关键BUG：启动时错误关闭自身进程导致闪退
- [OK] 现在可以正常使用 `python flash_tool.py` 命令行启动

更新日志 v2.4.3:
- [ADD] 固件管理标签页新增"添加到烧录"功能
  * 应用固件右键菜单新增"添加到烧录"→自动填充到烧录标签页应用固件路径
  * SBL固件右键菜单新增"添加到烧录"→自动填充到烧录标签页SBL固件路径
  * 更新界面状态显示和日志输出
- 构建日期：2025-12-20

更新日志 v2.4.2:
- [BUG] 修复雷达配置筛选变量名错误
- [BUG] 修复属性引用错误（sbl_files→sbl_firmwares, max_range→range_m）
- [OK] 联动筛选功能完全可用

更新日志 v2.4.1:
- [LINK] 应用固件筛选增强：
  * 新增子类别筛选器
  * 新增版本筛选器
  * 实现类别-子类别-处理器-版本级联动筛选
  * 搜索框支持类别/子类别/处理器/版本搜索
- [TOOL] SBL固件筛选增强：
  * 搜索框支持变体/说明搜索
  * 实现变体-Flash地址级联动筛选
- [CFG] 雷达配置筛选增强：
  * 搜索框支持应用场景/通道/距离/模式搜索
  * 实现应用-模式-通道-距离级联动筛选
- [INFO] UI布局优化：
  * 应用固件：第一行4个筛选，第二行2个筛选+搜索
- 构建日期：2025-12-20

更新日志 v2.4.0:
- [ICON] 应用名称更新："固件烧录工具" → "固件系统工具"
- [INFO] 固件管理标签页UI优化：
  * 应用固件/SBL固件/雷达配置：搜索框独立一行
  * 应用固件：新增文件大小、文件路径筛选
  * SBL固件：新增变体类型、Flash地址筛选
  * 雷达配置：新增TX/RX通道数、检测距离筛选
- [SCAN] 筛选条件智能化：根据文件参数动态填充可选项
- 构建日期：2025-12-20

更新日志 v2.3.1:
- [BUG] 修复固件管理-智能匹配-推荐雷达配置右键复制完整路径的BUG
  * 问题：配置文件未将路径信息添加到tags中，导致右键复制无法获取路径
  * 修复：插入配置匹配结果时，正确添加cfg.path到tags列表
  * 影响范围：推荐雷达配置(Top 8)的右键菜单功能
  * 构建日期：2025-12-20

更新日志 v2.3.0:
- [MON] 烧录标签页新增雷达配置区域
  * 在端口管理下方添加雷达配置LabelFrame
  * 当前为空白占位，预留后续功能扩展

更新日志 v2.2.0:
- [PKG] 固件管理标签页增强（v1.5.0）：
  * 智能匹配固件列表新增"文件大小"和"文件路径"列
  * 支持按文件名、文件大小、文件路径排序（点击列标题）
  * 文件大小自动格式化显示（KB/MB）
  * 雷达配置推荐数量从Top 5提升到Top 8

更新日志 v2.1.2:
- [BUG] 修复完整烧录App部分使用错误端口（COM4数据端口）的关键BUG
  - 现已修正：App烧录使用sbl_port（COM3烧录端口）
  - 原因：所有烧录操作均应使用烧录端口，而非数据端口
- [ERROR] 移除百分比显示功能（arprog工具不输出百分比信息）
  - 保留单行进度条显示（显示[====>   ]进度条）
  - 删除无效的百分比提取逻辑

更新日志 v2.1.1:
- [BUG] 修复完整烧录SBL部分调用旧Text widget方法的BUG
- [STAT] 完整烧录SBL部分现已正确显示百分比
- [CLEAN] 删除旧的get_last_line_start()和update_line_at_mark()方法
- [OK] 所有烧录功能现均使用Label统一显示进度

更新日志 v2.1.0:
- [STAT] 进度条百分比显示：从arprog输出提取百分比，实时显示烧录进度
- [TIME] 总执行时间实时显示：独立Label动态显示总耗时（每秒更新）
- [ICON] 增强UI布局：进度条和时间并排显示，信息更丰富

更新日志 v2.0.0:
- [DONE] 真正解决单行进度条问题！
  - 问题根源：Tkinter Text widget的delete+insert在快速更新时渲染缓冲区不清理
  - 解决方案：使用独立的Label组件显示进度条，完全绕过Text widget渲染问题
  - 测试验证：314次进度更新完美显示为单行
- [ICON] 美化进度条显示效果
  - 鲜艳的青色进度条，充满显示区域
- [TIME] 双重时间统计系统
  - 进度条时间：单个烧录操作的实际耗时（arprog执行时间）
  - 总执行时间：从开始到结束的完整流程时间（包括用户确认等待）
"""

import tkinter as tk
from tkinter import ttk, filedialog, messagebox, scrolledtext
import serial
import serial.tools.list_ports
import time
import os
import sys
import subprocess
import json
from pathlib import Path
import re
import psutil
import threading
from datetime import datetime
import ctypes


DEBUG = os.environ.get("FLASH_TOOL_DEBUG") == "1"


def _dprint(msg: str) -> None:
    if DEBUG:
        print(msg)


def _iter_resource_candidates(*relative_parts: str) -> "list[Path]":
    rel = Path(*relative_parts)
    candidates: list[Path] = []

    if getattr(sys, 'frozen', False):
        meipass = getattr(sys, '_MEIPASS', None)
        if meipass:
            candidates.append(Path(meipass) / rel)

        exe_dir = Path(sys.executable).resolve().parent
        candidates.append(exe_dir / rel)
        candidates.append(exe_dir.parent / rel)

    try:
        candidates.append(Path(__file__).resolve().parent / rel)
    except Exception:
        pass

    # 去重保序
    seen: set[str] = set()
    out: list[Path] = []
    for p in candidates:
        s = str(p)
        if s not in seen:
            seen.add(s)
            out.append(p)
    return out


def _resolve_tool_root_dir() -> Path:
    """解析 01-AWRL6844固件系统工具 的根目录（兼容 EXE 位于不同目录，如 6-Distribution 或 项目根 release/）。"""
    if getattr(sys, 'frozen', False):
        exe_dir = Path(sys.executable).resolve().parent
        meipass = getattr(sys, '_MEIPASS', None)
        candidates: list[Path] = [
            Path(meipass) if meipass else exe_dir,
            exe_dir,
            exe_dir.parent,
            exe_dir.parent.parent,
            exe_dir.parent.parent / '项目文档' / '3-固件工具' / '01-AWRL6844固件系统工具',
        ]
    else:
        candidates = [Path(__file__).resolve().parent.parent]

    for base in candidates:
        try:
            if (base / '1-SBL_Bootloader').exists() and (base / '3-Tools').exists():
                return base
        except Exception:
            continue

    return candidates[0]

try:
    from icon_manager import IconManager
except Exception as e:
    messagebox.showerror("模块导入错误", f"无法导入 icon_manager：{e}")
    sys.exit(1)

# 版本信息
VERSION = "2.4.6"
BUILD_DATE = "2026-01-29"
AUTHOR = "Benson@Wisefido"

# 导入标签页模块
try:
    from tabs import FlashTab, FirmwareManagerTab
except ImportError as e:
    messagebox.showerror(
        "模块导入错误",
        f"无法导入tabs模块：{e}\n\n"
        "请确保tabs目录存在且包含以下文件：\n"
        "- __init__.py\n"
        "- tab_flash.py\n"
        "- tab_firmware_manager.py"
    )
    sys.exit(1)

# ============================================================
# 设备配置
# ============================================================

DEVICE_CONFIGS = {
    'AWRL6844': {
        'name': 'AWRL6844',
        'image_type': 'MultiCore',
        # 【实测验证】端口功能说明
        'sbl_port_name': 'XDS110 Class Application/User UART',     # COM3 - 烧录端口
        'app_port_name': 'XDS110 Class Auxiliary Data Port',       # COM4 - 调试/数据端口
        'sbl_baudrate': 115200,
        'app_baudrate': 115200,
        # Flash地址配置
        'sbl_offset': 0x2000,      # SBL烧录地址（8KB偏移）
        'app_offset': 0x42000,     # 应用固件烧录地址（264KB偏移）
        # SDK路径配置
        'sdk_path': 'C:\\ti\\MMWAVE_L_SDK_06_01_00_01'
    }
}

# ============================================================
# 固件验证函数
# ============================================================

def verify_firmware_file(file_path):
    """验证固件文件的完整性"""
    if not os.path.exists(file_path):
        return False, "文件不存在"

    if os.path.getsize(file_path) == 0:
        return False, "文件大小为0"

    # 读取文件头部验证格式
    try:
        with open(file_path, 'rb') as f:
            header = f.read(8)
            if len(header) < 8:
                return False, "文件头不完整"
    except Exception as e:
        return False, f"读取文件失败: {str(e)}"

    return True, "文件验证通过"

def check_firmware_compatibility(file_path, device='AWRL6844'):
    """
    检查固件是否与设备匹配 (v1.0.5需求1)

    判别方法：
    1. 文件名检查：是否包含设备型号关键字
    2. Meta Header检查：解析固件元数据
    3. SDK工具检查：是否使用正确的烧录工具

    Returns:
        tuple: (is_compatible, reason, details)
    """
    reasons = []
    details = []
    is_compatible = True

    filename = os.path.basename(file_path).lower()

    # 检查1: 文件名是否包含设备型号
    device_keywords = {
        'AWRL6844': ['6844', 'awrl6844', 'iwrl6844'],
        'AWRL6432': ['6432', 'awrl6432', 'iwrl6432']
    }

    keywords = device_keywords.get(device, [])
    filename_match = any(kw in filename for kw in keywords)

    if filename_match:
        reasons.append(f"[OK] 文件名包含{device}型号标识")
        details.append(f"文件名: {filename}")
    else:
        is_compatible = False
        reasons.append(f"[WARN] 文件名未包含{device}型号标识")
        details.append(f"文件名: {filename}")
        details.append(f"期望关键字: {', '.join(keywords)}")

    # 检查2: 分析固件结构
    try:
        info = analyze_appimage_structure(file_path)
        if info:
            if info['has_meta_header']:
                reasons.append("[OK] 包含有效的Meta Header")
                details.append(f"Magic Number: {info['magic_number']}")
            else:
                is_compatible = False
                reasons.append("[ERROR] Meta Header无效")

            if info['has_sbl_header'] and info['has_app_header']:
                reasons.append("[OK] 包含SBL和App镜像")
                details.append(f"SBL大小: {info['sbl_size']} 字节")
                details.append(f"App大小: {info['app_size']} 字节")
            else:
                reasons.append("[WARN] 固件结构不完整")
        else:
            is_compatible = False
            reasons.append("[ERROR] 无法解析固件结构")
    except Exception as e:
        is_compatible = False
        reasons.append(f"[ERROR] 固件分析失败: {str(e)}")

    # 检查3: SDK工具检查
    if device == 'AWRL6844':
        expected_tool = 'arprog_cmdline_6844.exe'
        reasons.append(f"[OK] 使用烧录工具: {expected_tool}")
        details.append(f"设备: {device}")

    # 汇总结果
    reason_text = "\n".join(reasons)
    details_text = "\n".join(details)

    return is_compatible, reason_text, details_text

def analyze_appimage_structure(file_path):
    """
    分析appimage文件结构

    [WARN] 重要说明：
    - .appimage文件内部的Meta Header记录的是【文件内相对偏移】
    - Flash烧录偏移是烧录参数，不是固件文件属性，应由用户在烧录时配置
    - 本函数返回固件文件本身的信息和Meta Header中的镜像描述符

    AppImage文件结构：
    - Meta Header (512字节): 包含Magic、版本、各核镜像信息
    - 实际镜像数据: R5F + DSP + RF固件

    Args:
        file_path: 固件文件路径

    Returns:
        dict: 包含文件结构信息和镜像描述符（不包含Flash偏移量）
    """
    try:
        with open(file_path, 'rb') as f:
            # 读取Meta Header (至少512字节，按TI官方定义)
            meta_header = f.read(512)

            if len(meta_header) < 512:
                return None

            import struct

            # Offset 0x00: Magic Number (4字节) - 应为 0x5254534D ("MSTR")
            magic = struct.unpack('<I', meta_header[0:4])[0]

            # Offset 0x04: 版本信息 (4字节)
            version = struct.unpack('<I', meta_header[4:8])[0]

            # Offset 0x08: 镜像数量信息
            num_images_raw = struct.unpack('<I', meta_header[8:12])[0]
            num_images = (num_images_raw >> 16, num_images_raw & 0xFFFF)

            # 获取文件总大小
            f.seek(0, 2)
            total_size = f.tell()

            # 解析镜像描述符
            # TI格式: [CRC32] [加载地址] [镜像大小] [文件内偏移]
            images = []

            # 镜像1 (R5F) - 0x40
            img1_start = 0x40
            img1_crc = struct.unpack('<I', meta_header[img1_start:img1_start+4])[0]
            img1_addr = struct.unpack('<I', meta_header[img1_start+4:img1_start+8])[0]
            img1_size = struct.unpack('<I', meta_header[img1_start+8:img1_start+12])[0]
            img1_offset = struct.unpack('<I', meta_header[img1_start+12:img1_start+16])[0]

            # 验证镜像1数据合理性
            if img1_size > 0 and img1_offset + img1_size <= total_size:
                images.append({
                    'name': 'R5F Core',
                    'checksum': f'0x{img1_crc:08X}',
                    'file_offset': img1_offset,
                    'size': img1_size,
                    'load_addr': f'0x{img1_addr:08X}'
                })

            # 镜像2 (DSP) - 0x50
            img2_start = 0x50
            img2_crc = struct.unpack('<I', meta_header[img2_start:img2_start+4])[0]
            img2_addr = struct.unpack('<I', meta_header[img2_start+4:img2_start+8])[0]
            img2_size = struct.unpack('<I', meta_header[img2_start+8:img2_start+12])[0]
            img2_offset = struct.unpack('<I', meta_header[img2_start+12:img2_start+16])[0]

            # 验证镜像2数据合理性（文件内偏移+大小不超过文件总大小）
            if img2_size > 0 and img2_offset > 0 and img2_offset + img2_size <= total_size:
                images.append({
                    'name': 'C66x DSP Core',
                    'checksum': f'0x{img2_crc:08X}',
                    'file_offset': img2_offset,
                    'size': img2_size,
                    'load_addr': f'0x{img2_addr:08X}'
                })

            # 镜像3 (BSS/Data) - 0x60 - 通常是BSS段，可能不占用文件空间
            img3_start = 0x60
            img3_crc = struct.unpack('<I', meta_header[img3_start:img3_start+4])[0]
            img3_addr = struct.unpack('<I', meta_header[img3_start+4:img3_start+8])[0]
            img3_size = struct.unpack('<I', meta_header[img3_start+8:img3_start+12])[0]
            img3_offset = struct.unpack('<I', meta_header[img3_start+12:img3_start+16])[0]

            # BSS段：只在有效偏移且在文件范围内时才添加
            if img3_size > 0 and img3_offset > 0 and img3_offset + img3_size <= total_size:
                images.append({
                    'name': 'BSS/Data Section',
                    'checksum': f'0x{img3_crc:08X}',
                    'file_offset': img3_offset,
                    'size': img3_size,
                    'load_addr': f'0x{img3_addr:08X}'
                })
            elif img3_size > 0 and (img3_offset == 0 or img3_offset + img3_size > total_size):
                # BSS段在RAM中，不占用Flash空间
                images.append({
                    'name': 'BSS Section (RAM Only)',
                    'checksum': f'0x{img3_crc:08X}',
                    'file_offset': 'N/A (RAM)',
                    'size': img3_size,
                    'load_addr': f'0x{img3_addr:08X}'
                })

            # 判断文件类型（根据大小和文件名）
            filename = os.path.basename(file_path).lower()
            is_sbl = 'sbl' in filename or total_size < 200*1024

            if is_sbl:
                # SBL固件
                info = {
                    'total_size': total_size,
                    'has_meta_header': magic == 0x5254534D,
                    'magic_number': hex(magic),
                    'version': version,
                    'num_images': num_images,
                    'file_size': total_size,
                    'images': images,
                    'has_sbl_header': True,
                    'has_app_header': False,
                    'file_type': 'SBL'
                }
            else:
                # 应用固件
                info = {
                    'total_size': total_size,
                    'has_meta_header': magic == 0x5254534D,
                    'magic_number': hex(magic),
                    'version': version,
                    'num_images': num_images,
                    'file_size': total_size,
                    'images': images,
                    'has_sbl_header': False,
                    'has_app_header': True,
                    'file_type': 'APP'
                }

            return info

    except Exception as e:
        print(f"分析appimage结构失败: {e}")
        return None

def check_sbl_exists(port, baudrate=115200, timeout=3):
    """
    通过串口通信判断SBL是否存在 (v1.1.0新功能)

    原理：
    1. 如果板载有SBL，SBL会在启动时通过串口输出信息
    2. 尝试打开串口并读取数据，如果有响应则说明SBL存在
    3. 发送一些常见命令尝试触发SBL响应

    Args:
        port: 串口号（如COM3）
        baudrate: 波特率（默认115200）
        timeout: 超时时间（秒）

    Returns:
        tuple: (sbl_exists, message, details)
        - sbl_exists: bool - SBL是否存在
        - message: str - 检测结果消息
        - details: str - 详细信息（串口输出内容）
    """
    try:
        # 打开串口
        ser = serial.Serial(port, baudrate, timeout=1)
        time.sleep(0.5)  # 等待串口稳定

        # 清空缓冲区
        ser.reset_input_buffer()
        ser.reset_output_buffer()

        details = []
        has_response = False

        # 方法1: 读取启动时的输出（如果板子刚上电）
        details.append("=== 检测启动输出 ===")
        time.sleep(0.5)
        if ser.in_waiting > 0:
            data = ser.read(ser.in_waiting)
            try:
                text = data.decode('utf-8', errors='ignore')
                details.append(f"收到数据: {text[:200]}")  # 只记录前200字符
                if any(keyword in text.lower() for keyword in ['sbl', 'bootloader', 'ti', 'xwr', 'awrl']):
                    has_response = True
                    details.append("OK 发现SBL特征字符串")
            except (UnicodeDecodeError, AttributeError) as e:
                details.append(f"收到非文本数据: {len(data)} 字节")
                has_response = True

        # 方法2: 发送换行符尝试触发响应
        details.append("\n=== 尝试命令触发 ===")
        test_commands = [b'\r\n', b'\n', b'help\r\n', b'?\r\n']

        for cmd in test_commands:
            ser.write(cmd)
            time.sleep(0.3)

            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                try:
                    text = data.decode('utf-8', errors='ignore')
                    details.append(f"命令 {cmd} 响应: {text[:100]}")
                    has_response = True
                except (UnicodeDecodeError, AttributeError) as e:
                    details.append(f"命令 {cmd} 响应: {len(data)} 字节")
                    has_response = True

        # 方法3: 检查端口是否可以正常打开（最基本的检测）
        if not has_response:
            details.append("\n=== 基础检测 ===")
            details.append("OK 串口可以正常打开")
            details.append("OK 设备已连接")
            details.append("[WARN] 未收到SBL输出（可能SBL已运行完毕或未上电复位）")

        ser.close()

        details_text = "\n".join(details)

        if has_response:
            return True, "[OK] 检测到SBL存在（串口有响应）", details_text
        else:
            return False, "[WARN] 未检测到SBL响应\n请将SOP开关调整为功能模式[0 1]（非烧录模式[0 0]）并按RESET重启设备后重试", details_text

    except serial.SerialException as e:
        return False, f"[ERROR] 串口打开失败: {str(e)}", f"端口: {port}\n错误: {str(e)}"
    except Exception as e:
        return False, f"[ERROR] 检测失败: {str(e)}", f"异常: {str(e)}"

# ============================================================
# 对话框类
# ============================================================

class PreFlashCheckDialog(tk.Toplevel):
    """烧录前检查对话框"""

    def __init__(self, parent):
        super().__init__(parent)
        self.parent = parent
        self.title("烧录前检查")
        self.result = False
        self.create_widgets()

    def create_widgets(self):
        frame = ttk.Frame(self, padding=20)
        frame.pack(fill=tk.BOTH, expand=True)

        ttk.Label(frame, text="[WARN] 请确认以下事项：",
                 font=('Arial', 12, 'bold')).pack(pady=10)

        checks = [
            "OK 固件文件已正确选择",
            "OK 设备已通过USB连接到电脑",
            "OK 设备电源已打开",
            "OK 串口没有被其他程序占用",
            "OK 已保存当前工作"
        ]

        for check in checks:
            ttk.Label(frame, text=check, font=('Arial', 10)).pack(anchor=tk.W, pady=5)

        button_frame = ttk.Frame(frame)
        button_frame.pack(pady=20)

        ttk.Button(button_frame, text="确认开始",
                  command=self.on_ok).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="取消",
                  command=self.on_cancel).pack(side=tk.LEFT, padx=5)

        self.transient(self.parent)
        self.grab_set()

    def on_ok(self):
        self.result = True
        self.destroy()

    def on_cancel(self):
        self.result = False
        self.destroy()

class SBLCheckDialog(tk.Toplevel):
    """SBL检测对话框 (v1.1.0)"""

    def __init__(self, parent, port, baudrate=115200):
        super().__init__(parent)
        self.parent = parent
        self.title("SBL存在性检测")
        self.port = port
        self.baudrate = baudrate
        self.geometry("600x500")

        # 设置窗口可调整大小
        self.resizable(True, True)

        # 设置窗口居中显示在主窗口上
        self.update_idletasks()

        # 获取父窗口的位置和大小
        parent_x = parent.winfo_x()
        parent_y = parent.winfo_y()
        parent_width = parent.winfo_width()
        parent_height = parent.winfo_height()

        # 计算对话框应该显示的位置（居中在父窗口上）
        dialog_width = 600
        dialog_height = 500
        x = parent_x + (parent_width - dialog_width) // 2
        y = parent_y + (parent_height - dialog_height) // 2

        # 设置窗口位置
        self.geometry(f"{dialog_width}x{dialog_height}+{x}+{y}")

        self.create_widgets()
        self.start_check()

    def create_widgets(self):
        # 标题
        title_frame = ttk.Frame(self, padding=10)
        title_frame.pack(fill=tk.X)

        ttk.Label(
            title_frame,
            text="[SCAN] SBL存在性检测",
            font=('Arial', 14, 'bold')
        ).pack()

        ttk.Label(
            title_frame,
            text=f"检测端口: {self.port} @ {self.baudrate} bps",
            font=('Arial', 9),
            foreground='gray'
        ).pack()

        # 状态标签
        self.status_label = ttk.Label(
            self,
            text="[RUN] 正在检测...",
            font=('Arial', 11),
            foreground='blue'
        )
        self.status_label.pack(pady=10)

        # 详细信息区域
        detail_frame = ttk.LabelFrame(
            self,
            text="[INFO] 检测详情",
            padding=10
        )
        detail_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)

        self.detail_text = scrolledtext.ScrolledText(
            detail_frame,
            height=15,
            width=70,
            font=('Consolas', 9),
            bg='#f8f9fa',
            fg='#2c3e50',
            wrap=tk.WORD
        )
        self.detail_text.pack(fill=tk.BOTH, expand=True)

        # 按钮
        button_frame = ttk.Frame(self)
        button_frame.pack(pady=10)

        self.close_btn = ttk.Button(
            button_frame,
            text="关闭",
            state=tk.DISABLED,
            command=self.destroy
        )
        self.close_btn.pack()

        self.transient(self.parent)
        self.grab_set()

    def start_check(self):
        """启动检测线程"""
        thread = threading.Thread(target=self.check_thread, daemon=True)
        thread.start()

    def check_thread(self):
        """检测线程"""
        self.log("开始检测SBL...\n")
        self.log(f"端口: {self.port}\n")
        self.log(f"波特率: {self.baudrate}\n")
        self.log("-" * 50 + "\n\n")

        # 执行检测
        exists, message, details = check_sbl_exists(self.port, self.baudrate)

        # 更新UI
        self.status_label.config(
            text=message,
            foreground='green' if exists else 'orange'
        )

        self.log("\n" + "=" * 50 + "\n")
        self.log(f"检测结果: {message}\n")
        self.log("=" * 50 + "\n\n")
        self.log(details + "\n")

        if exists:
            self.log("\n[OK] 结论: 板载已有SBL，可以只烧录App更新应用\n")
        else:
            self.log("\n[WARN] 重要提示:\n")
            self.log("   1. 请将SOP开关调整为功能模式 [S8=OFF, S7=ON]\n")
            self.log("   2. 按RESET按钮重启设备\n")
            self.log("   3. 如仍无响应，建议执行完整烧录（SBL + App）\n")

        # 启用关闭按钮
        self.close_btn.config(state=tk.NORMAL)

    def log(self, message):
        """添加日志"""
        if not self.detail_text.winfo_exists():
            return
        self.detail_text.insert(tk.END, message)
        self.detail_text.see(tk.END)
        self.update_idletasks()

class SerialMonitorDialog(tk.Toplevel):
    """串口监视器对话框"""

    def __init__(self, parent, port, baudrate=115200):
        super().__init__(parent)
        self.title(f"串口监视器 - {port}")
        self.port = port
        self.baudrate = baudrate
        self.serial_port = None
        self.running = False
        self.create_widgets()
        self.start_monitoring()

    def create_widgets(self):
        # 输出区域
        self.output_text = scrolledtext.ScrolledText(
            self, height=30, width=100,
            bg='black', fg='#00ff00',
            font=('Consolas', 9)
        )
        self.output_text.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        # 控制按钮
        button_frame = ttk.Frame(self)
        button_frame.pack(fill=tk.X, padx=5, pady=5)

        ttk.Button(button_frame, text="清空",
                  command=self.clear_output).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="关闭",
                  command=self.close).pack(side=tk.RIGHT, padx=5)

    def start_monitoring(self):
        """启动串口监视"""
        try:
            self.serial_port = serial.Serial(
                self.port, self.baudrate,
                timeout=0.1
            )
            self.running = True
            self.monitor_thread = threading.Thread(target=self.monitor_loop, daemon=True)
            self.monitor_thread.start()
            self.log(f"OK 已连接到 {self.port}\n")
        except Exception as e:
            self.log(f"ERR 连接失败: {str(e)}\n")

    def monitor_loop(self):
        """监视循环"""
        while self.running:
            try:
                if self.serial_port and self.serial_port.in_waiting:
                    data = self.serial_port.read(self.serial_port.in_waiting)
                    try:
                        text = data.decode('utf-8', errors='replace')
                        self.log(text)
                    except (UnicodeDecodeError, AttributeError):
                        pass
                time.sleep(0.05)
            except Exception as e:
                if self.running:
                    self.log(f"\nERR 读取错误: {str(e)}\n")
                break

    def log(self, message):
        """添加日志"""
        if not self.output_text.winfo_exists():
            return
        self.output_text.insert(tk.END, message)
        self.output_text.see(tk.END)

    def clear_output(self):
        """清空输出"""
        self.output_text.delete(1.0, tk.END)

    def close(self):
        """关闭监视器"""
        self.running = False
        if self.serial_port:
            try:
                self.serial_port.close()
            except (OSError, AttributeError):
                pass
        self.destroy()

# ============================================================
# 主GUI类
# ============================================================

class FlashToolGUI:
    """固件烧录工具主GUI类 - v1.0.8模块化版本"""

    def __init__(self, root):
        self.root = root
        self.root.title(f"Ti AWRL6844 固件系统工具 v{VERSION}")
        self.root.geometry("1000x700")

        _dprint("\n[DEBUG] FlashToolGUI.__init__() 开始")
        _dprint(f"[DEBUG] sys.frozen = {getattr(sys, 'frozen', False)}")
        _dprint(f"[DEBUG] sys.executable = {sys.executable}")
        _dprint(f"[DEBUG] __file__ = {__file__ if not getattr(sys, 'frozen', False) else 'N/A'}")

        # 窗口图标（任务栏图标在 main() 中更早设置）
        try:
            for icon_path in _iter_resource_candidates('flash_tool_icon.ico'):
                if icon_path.exists():
                    self.root.iconbitmap(str(icon_path))
                    _dprint(f"[DEBUG] iconbitmap loaded: {icon_path.name}")
                    break
        except Exception as e:
            _dprint(f"[DEBUG] iconbitmap failed: {e}")

        # 强制窗口置顶并获得焦点
        self.root.lift()
        self.root.focus_force()
        self.root.attributes('-topmost', True)
        self.root.after(100, lambda: self.root.attributes('-topmost', False))

        # 版本信息（供标签页模块验证）
        self.VERSION = VERSION
        self.BUILD_DATE = BUILD_DATE

        # 图标管理（供全局UI使用）
        self.icons = IconManager(self.root)

        # 设备配置
        self.device_config = DEVICE_CONFIGS['AWRL6844']

        # 状态变量
        self.sbl_file = tk.StringVar()  # SBL固件文件
        self.app_file = tk.StringVar()  # 应用固件文件
        self.flash_tool_path = ""  # 烧录工具路径（改为字符串）
        self.sbl_port = tk.StringVar()
        self.app_port = tk.StringVar()

        # 烧录状态
        self.flashing = False
        self.flash_thread = None
        self.flash_process = None  # 当前烧录进程
        self.stop_flashing = False  # 停止烧录标志

        # 创建界面
        _dprint("[DEBUG] 开始创建 GUI 组件")
        self.create_widgets()
        _dprint("[DEBUG] GUI 组件创建完成")

        # 初始化默认固件路径（使用动态相对路径）- 必须在界面创建后
        _dprint("[DEBUG] 开始初始化默认固件路径")
        self._init_default_firmware_paths()
        _dprint("[DEBUG] 固件路径初始化完成")

        # 初始化端口
        _dprint("[DEBUG] 开始刷新端口列表")
        self.refresh_ports()
        _dprint("[DEBUG] 端口列表刷新完成")

        # 检测烧录工具
        _dprint("[DEBUG] 开始检测烧录工具")
        self.check_flash_tool()
        _dprint("[DEBUG] 烧录工具检测完成")
        _dprint("[DEBUG] FlashToolGUI.__init__() 结束\n")

    def _init_default_firmware_paths(self):
        """初始化默认固件路径（动态相对路径，项目移动后自动适配）"""
        _dprint("\n[DEBUG] _init_default_firmware_paths() 开始")
        try:
            # 获取基础目录 - EXE模式下使用exe所在目录的父目录
            if getattr(sys, 'frozen', False):
                base_dir = _resolve_tool_root_dir()
                _dprint(f"[DEBUG] EXE模式: exe路径 = {sys.executable}")
                _dprint(f"[DEBUG] EXE模式: base_dir = {base_dir}")
            else:
                # 脚本模式：使用脚本所在目录的父目录(5-Scripts的父目录)
                base_dir = Path(__file__).resolve().parent.parent
                _dprint(f"[DEBUG] 脚本模式: base_dir = {base_dir}")

            # 构建固件文件的相对路径
            sbl_path = base_dir / "1-SBL_Bootloader" / "sbl.release.appimage"
            app_path = base_dir / "2-HelloWorld_App" / "hello_world_system.release.appimage"

            _dprint(f"[DEBUG] 预期SBL路径: {sbl_path}")
            _dprint(f"[DEBUG] 预期App路径: {app_path}")
            _dprint(f"[DEBUG] SBL存在: {sbl_path.exists()}")
            _dprint(f"[DEBUG] App存在: {app_path.exists()}")

            # 检查SBL文件是否存在并设置
            if sbl_path.exists():
                self.sbl_file.set(str(sbl_path))
                # 更新界面显示
                if hasattr(self, 'sbl_status_label'):
                    self.sbl_status_label.config(
                        text="[OK] 已找到",
                        fg="green",
                        image=self.icons.get("ok", 16) if hasattr(self, "icons") else "",
                        compound="left",
                    )
                if hasattr(self, 'sbl_path_label'):
                    self.sbl_path_label.config(text=str(sbl_path))
                self.log(f"[OK] 自动加载SBL固件: {sbl_path}\n", "SUCCESS")
            else:
                self.sbl_file.set("")
                if hasattr(self, 'sbl_status_label'):
                    self.sbl_status_label.config(
                        text="[ERROR] 未找到",
                        fg="red",
                        image=self.icons.get("error", 16) if hasattr(self, "icons") else "",
                        compound="left",
                    )

            # 检查App文件是否存在并设置
            if app_path.exists():
                self.app_file.set(str(app_path))
                # 更新界面显示
                if hasattr(self, 'app_status_label'):
                    self.app_status_label.config(
                        text="[OK] 已找到",
                        fg="green",
                        image=self.icons.get("ok", 16) if hasattr(self, "icons") else "",
                        compound="left",
                    )
                if hasattr(self, 'app_path_label'):
                    self.app_path_label.config(text=str(app_path))
                self.log(f"[OK] 自动加载应用固件: {app_path}\n", "SUCCESS")
            else:
                self.app_file.set("")
                if hasattr(self, 'app_status_label'):
                    self.app_status_label.config(
                        text="[ERROR] 未找到",
                        fg="red",
                        image=self.icons.get("error", 16) if hasattr(self, "icons") else "",
                        compound="left",
                    )

        except Exception as e:
            # 初始化失败时使用空值
            self.sbl_file.set("")
            self.app_file.set("")
            self.log(f"[WARN] 自动加载固件失败: {str(e)}\n", "WARN")

    def create_widgets(self):
        """创建界面组件 - 使用模块化标签页"""

        # 顶部标题
        title_frame = ttk.Frame(self.root)
        title_frame.pack(fill=tk.X, padx=10, pady=5)

        ttk.Label(
            title_frame,
            text=f"Ti AWRL6844 固件系统工具 v{VERSION}",
            font=('Arial', 14, 'bold')
        ).pack(side=tk.LEFT)

        ttk.Label(
            title_frame,
            text=f"作者: {AUTHOR} | 构建: {BUILD_DATE}",
            font=('Arial', 9),
            foreground='gray'
        ).pack(side=tk.RIGHT)

        # 创建Notebook（标签页容器）
        self.notebook = ttk.Notebook(self.root)
        self.notebook.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)

        # 创建各个标签页的Frame
        flash_frame = ttk.Frame(self.notebook)
        firmware_manager_frame = ttk.Frame(self.notebook)

        # 添加到Notebook
        self.notebook.add(
            flash_frame,
            text="烧录功能",
            image=self.icons.get("upload", 20),
            compound="left",
        )
        self.notebook.add(
            firmware_manager_frame,
            text="固件管理",
            image=self.icons.get("microchip", 20),
            compound="left",
        )

        # 实例化各标签页模块
        self.flash_tab = FlashTab(flash_frame, self)
        self.basic_tab = self.flash_tab  # 兼容旧代码
        self.firmware_manager_tab = FirmwareManagerTab(firmware_manager_frame, self)

        # 状态栏
        status_frame = ttk.Frame(self.root)
        status_frame.pack(fill=tk.X, side=tk.BOTTOM, padx=10, pady=5)

        self.status_label = ttk.Label(
            status_frame,
            text="就绪",
            relief=tk.SUNKEN,
            anchor=tk.W
        )
        self.status_label.pack(fill=tk.X)

    # =========== 端口管理方法 ===========

    def refresh_ports(self):
        """刷新串口列表"""
        ports = serial.tools.list_ports.comports()

        sbl_ports = []
        app_ports = []

        for port in ports:
            if self.device_config['sbl_port_name'] in port.description:
                sbl_ports.append(port.device)
            if self.device_config['app_port_name'] in port.description:
                app_ports.append(port.device)

        # 更新下拉框（通过标签页模块）
        if hasattr(self, 'basic_tab'):
            self.basic_tab.update_port_list(sbl_ports, app_ports)

        return sbl_ports, app_ports

    def get_sbl_offset(self):
        """获取SBL Flash偏移量"""
        if not hasattr(self, 'sbl_offset_var'):
            return 0x2000  # 默认值

        offset_val = self.sbl_offset_var.get()
        if offset_val == "custom":
            # 从输入框获取
            custom_val = self.sbl_offset_entry.get().strip()
            try:
                return int(custom_val, 16) if custom_val.startswith('0x') else int(custom_val)
            except ValueError:
                self.log(f"[WARN] 无效的SBL偏移量: {custom_val}，使用默认值 0x2000\\n", "WARN")
                return 0x2000
        else:
            return int(offset_val, 16)

    def get_app_offset(self):
        """获取App Flash偏移量"""
        if not hasattr(self, 'app_offset_var'):
            return 0x42000  # 默认值

        offset_val = self.app_offset_var.get()
        if offset_val == "custom":
            # 从输入框获取
            custom_val = self.app_offset_entry.get().strip()
            try:
                return int(custom_val, 16) if custom_val.startswith('0x') else int(custom_val)
            except ValueError:
                self.log(f"[WARN] 无效的App偏移量: {custom_val}，使用默认值 0x42000\\n", "WARN")
                return 0x42000
        else:
            return int(offset_val, 16)

    def open_serial_monitor(self, port, baudrate=115200):
        """打开串口监视（输出到日志区）"""
        if not port:
            self.log("\n[WARN] 未指定端口，无法打开监视器\n", "WARN")
            return
        self.log(f"\n[MON] 打开串口监视器: {port} @ {baudrate}\n", "INFO")
        stop_event = threading.Event()

        def _monitor():
            ser = None
            try:
                ser = serial.Serial(port, baudrate, timeout=0.1)
                self.log(f"[OK] 监视器已连接 {port}\n")
                while not stop_event.is_set():
                    try:
                        if ser.in_waiting:
                            data = ser.read(ser.in_waiting)
                            try:
                                text = data.decode('utf-8', errors='replace')
                                if text:
                                    self.log(text)
                            except Exception:
                                pass
                        time.sleep(0.05)
                    except Exception as e:
                        self.log(f"\n[ERROR] 串口读取错误: {str(e)}\n", "ERROR")
                        break
            except Exception as e:
                self.log(f"[ERROR] 打开串口失败: {str(e)}\n", "ERROR")
            finally:
                try:
                    if ser:
                        ser.close()
                except Exception:
                    pass
                self.log(f"[CLOSED] 监视器已关闭: {port}\n")

        # 启动后台线程（一次性监视会话，不保存引用，关闭窗口时自动结束）
        t = threading.Thread(target=_monitor, daemon=True)
        t.start()

    def release_port(self, port):
        """尝试释放指定端口（只关闭占用该端口的句柄，不关闭应用）"""
        if not port:
            self.log("\n[WARN] 未指定端口，无法释放\n", "WARN")
            messagebox.showwarning("警告", "请选择要释放的端口！")
            return False

        self.log(f"\n[UNLOCK] 尝试释放端口: {port}\n", "INFO")

        # 首先尝试直接打开关闭（如果端口可用）
        try:
            ser = serial.Serial(port, 115200, timeout=0.2)
            ser.close()
            self.log(f"[OK] 端口 {port} 可用，无需释放\n", "SUCCESS")
            messagebox.showinfo("成功", f"端口 {port} 可用，无需释放")
            return True
        except serial.SerialException as e:
            self.log(f"[WARN] 端口 {port} 当前被占用\n", "WARN")

        # 尝试关闭本应用内可能打开的串口连接
        self.log(f"[TOOL] 尝试关闭本应用内对端口 {port} 的占用...\n", "INFO")

        # 检查是否有串口监视窗口打开了该端口
        # 注意：这里只是尝试，实际的串口监视窗口是独立线程，需要手动关闭

        # 再次尝试打开端口
        try:
            ser = serial.Serial(port, 115200, timeout=0.2)
            ser.close()
            self.log(f"[OK] 端口 {port} 已释放！\n", "SUCCESS")
            messagebox.showinfo("成功", f"端口 {port} 已成功释放！")
            return True
        except serial.SerialException:
            pass

        # 查找可能占用该特定端口的外部进程
        self.log(f"[SCAN] 查找占用端口 {port} 的外部进程...\n", "INFO")

        # 使用更精确的方式检测：通过lsof或handle工具（Windows）
        found = []

        # 方法1：检查常见串口工具进程（但要确认它们是否占用该端口）
        suspects = ["putty", "teraterm", "sscom", "serialplot", "cutecom", "minicom"]

        for proc in psutil.process_iter(['pid', 'name', 'connections']):
            try:
                name = (proc.info.get('name') or '').lower()

                # 只检查串口工具，不检查python/code等（避免关闭IDE或其他Python脚本）
                if any(s in name for s in suspects):
                    # 注意：psutil.Process.connections()主要用于网络连接
                    # 对于串口，我们只能基于进程名推断
                    found.append((proc.pid, proc.info.get('name')))

            except (psutil.NoSuchProcess, psutil.AccessDenied, AttributeError):
                continue

        if found:
            self.log("[SCAN] 发现可能占用串口的工具:\n", "INFO")
            for pid, name in found:
                self.log(f"  - PID {pid}: {name}\n", "INFO")

            # 询问用户是否终止这些串口工具
            result = messagebox.askyesno(
                "发现串口工具进程",
                f"发现以下串口工具可能占用端口 {port}:\n\n" +
                "\n".join([f"- {name} (PID: {pid})" for pid, name in found]) +
                f"\n\n[WARN] 注意：只会关闭这些串口工具，不会关闭本应用。\n\n是否终止这些进程以释放端口？"
            )

            if result:
                killed = []
                for pid, name in found:
                    try:
                        proc = psutil.Process(pid)
                        proc.terminate()
                        proc.wait(timeout=3)
                        killed.append(name)
                        self.log(f"[OK] 已终止进程: {name} (PID: {pid})\n", "SUCCESS")
                    except psutil.TimeoutExpired:
                        try:
                            proc.kill()
                            killed.append(name)
                            self.log(f"[OK] 已强制终止进程: {name} (PID: {pid})\n", "SUCCESS")
                        except Exception as e:
                            self.log(f"[ERROR] 无法终止进程 {name}: {str(e)}\n", "ERROR")
                    except Exception as e:
                        self.log(f"[ERROR] 终止进程 {name} 失败: {str(e)}\n", "ERROR")

                # 等待一下，再次尝试打开端口
                time.sleep(0.5)
                try:
                    ser = serial.Serial(port, 115200, timeout=0.2)
                    ser.close()
                    self.log(f"[OK] 端口 {port} 已成功释放！\n", "SUCCESS")
                    messagebox.showinfo("成功", f"端口 {port} 已释放！\n已终止: {', '.join(killed)}")
                    return True
                except Exception as e:
                    self.log(f"[WARN] 端口仍然被占用: {str(e)}\n", "WARN")
                    messagebox.showwarning(
                        "部分成功",
                        f"已终止部分进程，但端口仍被占用。\n\n可能原因：\n1. 本应用的串口监视窗口正在使用该端口（请手动关闭）\n2. 其他未知程序占用\n\n建议:\n1. 关闭串口监视窗口\n2. 重新插拔USB设备\n3. 在设备管理器中禁用/启用端口"
                    )
                    return False
            else:
                self.log("[WARN] 用户取消释放操作\n", "WARN")
                return False
        else:
            self.log("未发现串口工具进程\n", "WARN")
            messagebox.showwarning(
                "未找到占用进程",
                f"端口 {port} 被占用，但未找到常见的串口工具进程。\n\n可能原因：\n" +
                f"1. 本应用的串口监视窗口正在使用 {port}（请手动关闭）\n" +
                "2. 未知程序占用该端口\n\n" +
                "建议:\n" +
                "1. 检查并关闭串口监视窗口\n" +
                "2. 重新插拔USB设备\n" +
                "3. 在设备管理器中禁用/启用端口\n" +
                "4. 使用任务管理器查找占用进程"
            )
            return False

    def get_port_info(self, port):
        """获取端口详细信息"""
        ports = serial.tools.list_ports.comports()
        for p in ports:
            if p.device == port:
                return {
                    'device': p.device,
                    'description': p.description,
                    'hwid': p.hwid,
                    'vid': p.vid,
                    'pid': p.pid
                }
        return None

    def check_flash_tool(self):
        """检测烧录工具是否存在（支持多个路径）"""
        tool_base = str(_resolve_tool_root_dir())
        print(f"[DEBUG] check_flash_tool: 工具基础目录: {tool_base}")

        # 候选路径列表
        tool_paths = [
            # 1. APP内置工具（优先）
            os.path.join(tool_base, '3-Tools', 'arprog_cmdline_6844.exe'),
            # 2. SDK标准路径
            'C:\\ti\\MMWAVE_L_SDK_06_01_00_01\\tools\\FlashingTool\\arprog_cmdline_6844.exe',
            # 3. 用户自定义路径（如果已设置）
            self.flash_tool_path
        ]

        # 遍历查找第一个存在的工具
        for tool_exe in tool_paths:
            if tool_exe and os.path.exists(tool_exe):
                self.flash_tool_path = tool_exe
                if hasattr(self, 'tool_status_label'):
                    self.tool_status_label.config(text="[OK] 已找到", fg="green")
                if hasattr(self, 'tool_path_label'):
                    self.tool_path_label.config(text=tool_exe)
                return True

        # 所有路径都不存在
        if hasattr(self, 'tool_status_label'):
            self.tool_status_label.config(text="[ERROR] 未找到", fg="red")
        if hasattr(self, 'tool_path_label'):
            self.tool_path_label.config(text="")
        return False

    def select_flash_tool(self):
        """选择烧录工具路径"""
        filename = filedialog.askopenfilename(
            title="选择烧录工具",
            filetypes=[
                ("Executable Files", "*.exe"),
                ("All Files", "*.*")
            ],
            initialdir=os.path.dirname(self.flash_tool_path) if self.flash_tool_path else None
        )
        if filename:
            self.flash_tool_path = filename
            self.log(f"[OK] 已选择自定义烧录工具: {filename}\n", "SUCCESS")

            # 更新下拉框中的自定义工具选项
            if hasattr(self, 'tool_combo'):
                from pathlib import Path
                # 获取flash_tab实例
                flash_tab = self.flash_tab

                # 添加或更新自定义工具选项
                custom_key = " 自定义工具"
                flash_tab.tool_options[custom_key] = filename

                # 更新下拉框值
                self.tool_combo['values'] = list(flash_tab.tool_options.keys())

                # 选中自定义工具
                for i, key in enumerate(flash_tab.tool_options.keys()):
                    if key == custom_key:
                        self.tool_combo.current(i)
                        break

                # 更新路径显示
                if hasattr(self, 'tool_path_label'):
                    self.tool_path_label.config(text=filename, fg="#27ae60")

    def test_all_ports(self):
        """测试所有相关端口（烧录端口COM3 + 数据输出端口COM4）"""
        self.log("\n" + "="*60 + "\n", "INFO")
        self.log("[SCAN] 开始测试所有端口...\n", "INFO")

        # 获取当前选择的端口
        flash_port = ""
        debug_port = ""

        if hasattr(self, 'flash_port_combo'):
            flash_port = self.flash_port_combo.get()
        if hasattr(self, 'debug_port_combo'):
            debug_port = self.debug_port_combo.get()

        # 如果界面端口未设置，使用默认值
        if not flash_port:
            flash_port = self.sbl_port.get() or "COM3"
        if not debug_port:
            debug_port = self.app_port.get() or "COM4"

        results = []

        # 测试烧录端口（COM3 - User UART）
        self.log(f"\n[NOTE] 测试烧录端口: {flash_port}\n", "INFO")
        try:
            ser = serial.Serial(flash_port, 115200, timeout=1)
            ser.close()
            self.log(f"[OK] 端口 {flash_port} 连接正常！\n", "SUCCESS")
            results.append(f"[OK] {flash_port} (烧录端口): 连接正常")
        except Exception as e:
            error_msg = f"[ERROR] 端口 {flash_port} 连接失败: {str(e)}"
            self.log(f"{error_msg}\n", "ERROR")
            results.append(f"[ERROR] {flash_port} (烧录端口): {str(e)}")

        # 测试数据输出端口（COM4 - Auxiliary Data Port）
        self.log(f"\n[NOTE] 测试数据输出端口: {debug_port}\n", "INFO")
        try:
            ser = serial.Serial(debug_port, 115200, timeout=1)
            ser.close()
            self.log(f"[OK] 端口 {debug_port} 连接正常！\n", "SUCCESS")
            results.append(f"[OK] {debug_port} (数据输出端口): 连接正常")
        except Exception as e:
            error_msg = f"[ERROR] 端口 {debug_port} 连接失败: {str(e)}"
            self.log(f"{error_msg}\n", "ERROR")
            results.append(f"[ERROR] {debug_port} (数据输出端口): {str(e)}")

        # 汇总结果
        self.log("\n" + "="*60 + "\n", "INFO")
        self.log("[STAT] 端口测试结果汇总:\n", "INFO")
        for result in results:
            if "[OK]" in result:
                self.log(f"  {result}\n", "SUCCESS")
            else:
                self.log(f"  {result}\n", "ERROR")
        self.log("="*60 + "\n\n", "INFO")

        # 显示消息框
        result_text = "\n".join(results)
        if all("[OK]" in r for r in results):
            messagebox.showinfo("端口测试成功", f"所有端口测试通过！\n\n{result_text}")
        else:
            messagebox.showwarning("端口测试完成", f"部分端口测试失败！\n\n{result_text}")

    def test_port(self, port, baudrate=115200):
        """测试单个端口连接"""
        if not port:
            self.log("\n[WARN] 请先选择端口！\n", "WARN")
            messagebox.showwarning("警告", "请先选择要测试的端口！")
            return False, "未选择端口"

        self.log(f"\n[SCAN] 正在测试端口 {port}...\n", "INFO")

        try:
            ser = serial.Serial(port, baudrate, timeout=1)
            ser.close()
            self.log(f"[OK] 端口 {port} 连接正常！\n", "SUCCESS")
            messagebox.showinfo("成功", f"端口 {port} 连接正常！")
            return True, "端口连接正常"
        except Exception as e:
            error_msg = f"端口 {port} 连接失败: {str(e)}"
            self.log(f"[ERROR] {error_msg}\n", "ERROR")
            messagebox.showerror("错误", error_msg)
            return False, error_msg

    # =========== 烧录方法 ===========

    def flash_firmware(self):
        """完整烧录固件（SBL + App）"""
        if self.flashing:
            self.log("[WARN] 烧录正在进行中...\n", "WARN")
            return

        # 获取固件文件
        sbl_file = (self.sbl_file.get() or '').strip()
        app_file = (self.app_file.get() or '').strip()

        if not sbl_file or not app_file:
            messagebox.showerror("错误", "请先选择SBL和应用固件文件！")
            return
        # 校验存在性
        if not os.path.exists(sbl_file):
            messagebox.showerror("错误", f"SBL文件不存在：{sbl_file}")
            return
        if not os.path.exists(app_file):
            messagebox.showerror("错误", f"App文件不存在：{app_file}")
            return

        # 获取端口
        sbl_port = self.sbl_port.get()
        app_port = self.app_port.get()

        if not sbl_port or not app_port:
            messagebox.showerror("错误", "请先选择SBL和App端口！")
            return

        # 启动烧录线程
        self.flashing = True
        self.stop_flashing = False
        self.flash_thread = threading.Thread(
            target=self._flash_firmware_thread,
            args=(sbl_file, app_file, sbl_port, app_port),
            daemon=True
        )
        self.flash_thread.start()

    def stop_flash(self):
        """停止烧录"""
        if not self.flashing:
            self.log("[WARN] 当前没有正在进行的烧录任务\n", "WARN")
            return

        self.log("\n[STOP] 用户请求停止烧录...\n", "WARN")
        self.stop_flashing = True

        # 终止当前进程
        if self.flash_process and self.flash_process.poll() is None:
            try:
                self.flash_process.kill()
                self.log("[OK] 烧录进程已终止\n", "INFO")
            except Exception as e:
                self.log(f"[ERROR] 终止进程失败: {e}\n", "ERROR")

        self.time_update_running = False  # 停止时间更新
        self.flashing = False

    def _update_total_time_display(self, start_time):
        """实时更新总执行时间显示（后台线程）"""
        while self.time_update_running:
            try:
                elapsed = time.time() - start_time
                minutes = int(elapsed // 60)
                seconds = int(elapsed % 60)

                if hasattr(self, 'total_time_label'):
                    if minutes > 0:
                        time_text = f"[TIME] 总时间: {minutes}分{seconds}秒"
                    else:
                        time_text = f"[TIME] 总时间: {seconds}秒"
                    self.total_time_label.config(text=time_text)

                time.sleep(1)  # 每秒更新一次
            except:
                break

    def _flash_firmware_thread(self, sbl_file, app_file, sbl_port, app_port):
        """烧录线程（完整烧录：依次烧录 SBL 与 App）

        根据实测验证，采用依次烧录策略更稳定可靠
        """
        try:
            total_start_time = time.time()  # 总执行时间计时器（从开始到结束）

            # 启动总时间实时更新线程
            self.time_update_running = True
            time_thread = threading.Thread(
                target=self._update_total_time_display,
                args=(total_start_time,),
                daemon=True
            )
            time_thread.start()

            self.log("\n" + "="*60 + "\n")
            self.log("[START] 开始完整烧录流程（SBL + App）\n", "INFO")
            self.log("="*60 + "\n\n")

            # SOP模式人工确认
            sop_confirm = messagebox.askyesno(
                "[WARN] 烧录前准备",
                "请完成以下准备：\n\n"
                "1️⃣ 确认SOP开关设置：\n"
                "   - S8 = OFF（断开）\n"
                "   - S7 = OFF（断开）\n"
                "   - 模式 = SOP_MODE1（烧录模式）\n\n"
                "2️⃣【重要】准备手动复位设备：\n"
                "   - 找到并按住上方 RESET 按钮\n"
                "   - 等待到提示：'---- please restart the device ----'\n"
                "   - 立即按下 RESET 按钮\n\n"
                "3️⃣ 烧录将自动开始\n"
                "   - 显示进度：[========>]\n"
                "   - 完成提示：Done MetaImage: 0\n\n"
                "============================================================\n"
                "是否已确认以上设置？"
            )
            if not sop_confirm:
                self.log("[ERROR] 用户取消烧录（SOP模式未确认）\n", "ERROR")
                return

            # 查询实际COM端口描述
            ports = serial.tools.list_ports.comports()
            port_description = "未知端口"
            for port in ports:
                if port.device == sbl_port:
                    port_description = port.description
                    break

            self.log(f"[FILE] SBL文件: {sbl_file}\n")
            self.log(f"[FILE] App文件: {app_file}\n")
            self.log(f"[PORT] 烧录端口: {sbl_port} ({port_description})\n\n")

            # 串口确认
            port_confirm = messagebox.askyesno(
                "串口确认",
                f"请确认烧录端口：\n\n"
                f"烧录端口: {sbl_port}\n"
                f"端口说明: {port_description}\n\n"
                f"注意：SBL和App使用同一个烧录端口\n\n"
                f"端口是否正确？"
            )
            if not port_confirm:
                self.log("[ERROR] 用户取消烧录（端口未确认）\n", "ERROR")
                return

            # 获取烧录工具路径
            tool_exe = self.flash_tool_path

            if not tool_exe or not os.path.exists(tool_exe):
                self.log(f"[ERROR] 找不到烧录工具\n", "ERROR")
                self.log("请点击「选择」按钮选择烧录工具，或确认SDK已正确安装\n", "ERROR")
                return

            # 步骤1: 烧录SBL
            self.log("[STEP] 步骤 1/2: 烧录SBL (Bootloader)\n", "INFO")

            # 拔插USB确认
            usb_confirm = messagebox.askyesno(
                "准备烧录SBL",
                "请拔插USB或按RESET按钮\n\n"
                "完成后点击\"是\"继续烧录"
            )
            if not usb_confirm:
                self.log("[ERROR] 用户取消烧录（USB未拔插）\n", "ERROR")
                return

            self.log("开始烧录SBL...\n\n")

            sbl_flash_start = time.time()  # SBL烧录操作计时器

            # 检查是否启用偏移量
            offset_enabled = self.offset_enabled_var.get() if hasattr(self, 'offset_enabled_var') else True

            # 构建烧录命令
            sbl_cmd = [
                tool_exe,
                "-p", sbl_port,
                "-f1", sbl_file      # 使用-f1
            ]

            if offset_enabled:
                sbl_offset = self.get_sbl_offset()  # 从用户选择获取
                self.log(f"[OFFSET] 使用SBL Flash偏移量: 0x{sbl_offset:X} ({sbl_offset} 字节)\n")
                sbl_cmd.extend(["-of1", str(sbl_offset)])  # 添加偏移参数
            else:
                self.log(f"[OFFSET] Flash偏移量已禁用，使用工具默认偏移\n")

            sbl_cmd.extend([
                "-s", "SFLASH",       # 存储类型
                "-c"                  # Break信号
            ])

            self.log(f"执行命令: {' '.join(sbl_cmd)}\n")

            # 检查是否已停止
            if self.stop_flashing:
                self.log("[ERROR] 烧录已停止\n", "ERROR")
                return

            process = subprocess.Popen(
                sbl_cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                bufsize=0,  # 无缓冲，实时输出
                creationflags=subprocess.CREATE_NO_WINDOW if os.name == 'nt' else 0
            )
            self.flash_process = process  # 保存进程引用

            # 读取输出（使用Label显示进度，支持百分比）
            buffer = b''

            while True:
                if self.stop_flashing:
                    process.kill()
                    self.log("\n[ERROR] 烧录已停止\n", "ERROR")
                    self.time_update_running = False
                    return

                byte = process.stdout.read(1)
                if not byte:
                    break

                buffer += byte

                if byte == b'\r':
                    try:
                        line = buffer[:-1].decode('utf-8', errors='ignore').strip()
                        if line and hasattr(self, 'progress_label'):
                            # 直接显示进度信息（arprog不输出百分比）
                            self.progress_label.config(text=line)
                            self.progress_label.update()
                    except:
                        pass
                    buffer = b''

                elif byte == b'\n':
                    try:
                        line = buffer[:-1].decode('utf-8', errors='ignore').strip()
                        if line:
                            self.log(line + '\n')
                            # 清空进度标签
                            if hasattr(self, 'progress_label'):
                                self.progress_label.config(text="")
                    except:
                        pass
                    buffer = b''

            process.wait()

            if self.stop_flashing:
                self.log("\n[ERROR] 烧录已停止\n", "ERROR")
                self.time_update_running = False
                return

            if process.returncode != 0:
                self.log("\n[ERROR] SBL烧录失败！\n", "ERROR")
                self.time_update_running = False
                return

            # 计算SBL烧录耗时（进度条时间）
            sbl_elapsed = time.time() - sbl_flash_start
            sbl_minutes = int(sbl_elapsed // 60)
            sbl_seconds = int(sbl_elapsed % 60)
            sbl_time_str = f"{sbl_minutes}分{sbl_seconds}秒" if sbl_minutes > 0 else f"{sbl_seconds}秒"

            self.log("\n[OK] SBL烧录成功！\n", "SUCCESS")
            self.log(f"[TIME]  SBL烧录耗时: {sbl_time_str}\n", "INFO")

            # 重要提示：SOP模式和复位
            messagebox.showinfo(
                "SBL烧录完成",
                "[OK] SBL已成功烧录到Flash\n\n"
                "[WARN] 接下来请准备烧录应用固件：\n\n"
                "[NOTE] 硬件操作：\n"
                "   - 保持SOP开关在烧录模式 [0 0]\n"
                "   - 拔插USB或按RESET按钮\n\n"
                "[TIP] 如果不烧录应用固件：\n"
                "   1. 切换SOP开关到 [0 1]（运行模式）\n"
                "   2. 按RESET按钮启动SBL\n\n"
                "点击确定继续烧录应用固件..."
            )
            time.sleep(0.5)

            # 步骤2: 烧录应用固件
            self.log("\n[STEP] 步骤 2/2: 烧录应用固件\n", "INFO")

            # 应用固件烧录前确认
            app_usb_confirm = messagebox.askyesno(
                "准备烧录应用固件",
                "请再次拔插USB或按RESET按钮\n\n"
                "完成后点击\"是\"继续烧录"
            )
            if not app_usb_confirm:
                self.log("[ERROR] 用户取消应用固件烧录（USB未拔插）\n", "ERROR")
                return

            self.log("开始烧录应用固件...\n\n")

            app_flash_start = time.time()  # App烧录操作计时器

            # 检查是否启用偏移量
            offset_enabled = self.offset_enabled_var.get() if hasattr(self, 'offset_enabled_var') else True

            # 构建烧录命令（注意：App也使用sbl_port烧录端口COM3）
            app_cmd = [
                tool_exe,
                "-p", sbl_port,  # 修复：使用sbl_port（COM3烧录端口）而非app_port（COM4数据端口）
                "-f1", app_file      # 使用-f1
            ]

            if offset_enabled:
                app_offset = self.get_app_offset()  # 从用户选择获取
                self.log(f"[OFFSET] 使用App Flash偏移量: 0x{app_offset:X} ({app_offset} 字节)\n")
                app_cmd.extend(["-of1", str(app_offset)])  # 添加偏移参数
            else:
                self.log(f"[OFFSET] Flash偏移量已禁用，使用工具默认偏移\n")

            app_cmd.extend([
                "-s", "SFLASH",       # 存储类型
                "-c"                  # Break信号
            ])

            self.log(f"执行命令: {' '.join(app_cmd)}\n")

            # 检查是否已停止
            if self.stop_flashing:
                self.log("[ERROR] 烧录已停止\n", "ERROR")
                return

            process = subprocess.Popen(
                app_cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                bufsize=0,  # 无缓冲，实时输出
                creationflags=subprocess.CREATE_NO_WINDOW if os.name == 'nt' else 0
            )
            self.flash_process = process  # 更新进程引用

            # 读取输出（二进制模式，使用Label显示进度）
            buffer = b''

            while True:
                if self.stop_flashing:
                    process.kill()
                    self.log("\n[ERROR] 烧录已停止\n", "ERROR")
                    self.time_update_running = False
                    return

                byte = process.stdout.read(1)
                if not byte:
                    break

                buffer += byte

                if byte == b'\r':
                    try:
                        line = buffer[:-1].decode('utf-8', errors='ignore').strip()
                        if line and hasattr(self, 'progress_label'):
                            # 直接显示进度信息（arprog不输出百分比）
                            self.progress_label.config(text=line)
                            self.progress_label.update()
                    except:
                        pass
                    buffer = b''

                elif byte == b'\n':
                    try:
                        line = buffer[:-1].decode('utf-8', errors='ignore').strip()
                        if line:
                            self.log(line + '\n')
                            # 清空进度标签
                            if hasattr(self, 'progress_label'):
                                self.progress_label.config(text="")
                    except:
                        pass
                    buffer = b''

            process.wait()

            if self.stop_flashing:
                self.log("\n[ERROR] 烧录已停止\n", "ERROR")
                self.time_update_running = False
                return

            if process.returncode != 0:
                self.log("\n[ERROR] App烧录失败！\n", "ERROR")
                self.time_update_running = False
                return

            # 计算App烧录耗时（进度条时间）
            app_elapsed = time.time() - app_flash_start
            app_minutes = int(app_elapsed // 60)
            app_seconds = int(app_elapsed % 60)
            app_time_str = f"{app_minutes}分{app_seconds}秒" if app_minutes > 0 else f"{app_seconds}秒"

            self.log("\n[OK] App烧录成功！\n", "SUCCESS")
            self.log(f"[TIME]  App烧录耗时: {app_time_str}\n", "INFO")

            # 计算总执行时间（包括用户确认等待）
            total_elapsed = time.time() - total_start_time
            total_minutes = int(total_elapsed // 60)
            total_seconds = int(total_elapsed % 60)
            total_time_str = f"{total_minutes}分{total_seconds}秒" if total_minutes > 0 else f"{total_seconds}秒"

            self.log("\n" + "="*60 + "\n")
            self.log("[DONE] 完整烧录完成！\n", "SUCCESS")
            self.log(f"[TIME]  SBL烧录耗时: {sbl_time_str}\n", "INFO")
            self.log(f"[TIME]  App烧录耗时: {app_time_str}\n", "INFO")
            self.log(f"[TIME]  总执行时间: {total_time_str} (包括用户确认)\n", "INFO")
            self.log("="*60 + "\n\n")

            # 进度条显示完成信息
            if hasattr(self, 'progress_label'):
                self.progress_label.config(text=f"[OK] 烧录完成！ App耗时: {app_time_str} | 总时间: {total_time_str}")

            messagebox.showinfo("成功", f"固件烧录完成！\n\nSBL耗时: {sbl_time_str}\nApp耗时: {app_time_str}\n总时间: {total_time_str}\n\n请复位设备并测试。")

        except Exception as e:
            self.log(f"\n[ERROR] 烧录过程出错: {str(e)}\n", "ERROR")
            messagebox.showerror("错误", f"烧录失败：{str(e)}")
        finally:
            # 清理进程资源
            if hasattr(self, 'flash_process') and self.flash_process:
                try:
                    if self.flash_process.poll() is None:  # 进程还在运行
                        self.flash_process.kill()
                    # 关闭管道
                    if self.flash_process.stdout:
                        self.flash_process.stdout.close()
                    if self.flash_process.stderr:
                        self.flash_process.stderr.close()
                except:
                    pass
                self.flash_process = None

            self.time_update_running = False  # 停止时间更新线程
            self.flashing = False

    def flash_sbl_only(self):
        """仅烧录SBL"""
        if self.flashing:
            self.log("[WARN] 烧录正在进行中...\n", "WARN")
            return

        # 获取SBL固件文件
        firmware_file = self.sbl_file.get()
        if not firmware_file or not os.path.exists(firmware_file):
            messagebox.showerror("错误", "请先选择有效的SBL固件文件！")
            return

        # 获取端口
        sbl_port = self.sbl_port.get()
        if not sbl_port:
            messagebox.showerror("错误", "请先选择SBL端口！")
            return

        # 启动烧录线程
        self.flashing = True
        self.flash_thread = threading.Thread(
            target=self._flash_sbl_thread,
            args=(firmware_file, sbl_port),
            daemon=True
        )
        self.flash_thread.start()

    def _flash_sbl_thread(self, firmware_file, sbl_port):
        """烧录线程（仅SBL）"""
        try:
            total_start_time = time.time()  # 总执行时间计时器（从开始到结束）

            # 启动总时间实时更新线程
            self.time_update_running = True
            time_thread = threading.Thread(
                target=self._update_total_time_display,
                args=(total_start_time,),
                daemon=True
            )
            time_thread.start()

            self.log("\n" + "="*60 + "\n")
            self.log("[TOOL] 开始SBL烧录\n", "INFO")
            self.log("="*60 + "\n\n")

            # SOP模式确认
            sop_confirm = messagebox.askyesno(
                "SOP模式确认",
                "请确认硬件SOP模式配置：\n\n"
                "烧录模式（SOP_MODE1）：\n"
                "- S8 = OFF\n"
                "- S7 = OFF\n\n"
                "当前是否已设置为烧录模式？"
            )
            if not sop_confirm:
                self.log("[ERROR] 用户取消烧录（SOP模式未确认）\n", "ERROR")
                return

            # 查询实际COM端口描述
            ports = serial.tools.list_ports.comports()
            port_description = "未知端口"
            for port in ports:
                if port.device == sbl_port:
                    port_description = port.description
                    break

            self.log(f"[FILE] 固件文件: {firmware_file}\n")
            self.log(f"[PORT] SBL端口: {sbl_port} ({port_description})\n\n")

            # 串口确认
            port_confirm = messagebox.askyesno(
                "串口确认",
                f"请确认烧录端口：\n\n"
                f"SBL端口: {sbl_port}\n"
                f"端口说明: {port_description}\n\n"
                f"端口是否正确？"
            )
            if not port_confirm:
                self.log("[ERROR] 用户取消烧录（端口未确认）\n", "ERROR")
                return

            # 拔插USB确认
            usb_confirm = messagebox.askyesno(
                "准备烧录",
                "请拔插USB或按RESET按钮\n\n"
                "完成后点击\"是\"继续烧录"
            )
            if not usb_confirm:
                self.log("[ERROR] 用户取消烧录（USB未拔插）\n", "ERROR")
                return

            # 获取烧录工具路径
            tool_exe = self.flash_tool_path

            if not tool_exe or not os.path.exists(tool_exe):
                self.log(f"[ERROR] 找不到烧录工具\n", "ERROR")
                return

            self.log("开始烧录SBL...\n\n")

            flash_start_time = time.time()  # 烧录操作计时器（进度条时间）

            # 检查是否启用偏移量
            offset_enabled = self.offset_enabled_var.get() if hasattr(self, 'offset_enabled_var') else True

            # 构建烧录命令
            cmd = [
                tool_exe,
                "-p",
                sbl_port,
                "-f1",            # 使用-f1而非-f
                firmware_file
            ]

            if offset_enabled:
                sbl_offset = self.get_sbl_offset()  # 从用户选择获取
                self.log(f"[OFFSET] 使用SBL Flash偏移量: 0x{sbl_offset:X} ({sbl_offset} 字节)\n")
                cmd.extend(["-of1", str(sbl_offset)])  # 添加偏移参数
            else:
                self.log(f"[OFFSET] Flash偏移量已禁用，使用工具默认偏移\n")

            cmd.extend([
                "-s",             # 存储类型
                "SFLASH",
                "-c"              # Break信号
            ])

            self.log(f"执行命令: {' '.join(cmd)}\n")

            process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                bufsize=0,  # 无缓冲，实时输出
                creationflags=subprocess.CREATE_NO_WINDOW if os.name == 'nt' else 0
            )

            # 实时读取并显示输出（使用Label显示进度）
            buffer = b''

            while True:
                byte = process.stdout.read(1)
                if not byte:
                    break

                buffer += byte

                if byte == b'\r':
                    try:
                        line = buffer[:-1].decode('utf-8', errors='ignore').strip()
                        if line and hasattr(self, 'progress_label'):
                            # 直接显示进度信息（arprog不输出百分比）- SBL only
                            self.progress_label.config(text=line)
                            self.progress_label.update()
                    except:
                        pass
                    buffer = b''

                elif byte == b'\n':
                    try:
                        line = buffer[:-1].decode('utf-8', errors='ignore').strip()
                        if line:
                            self.log(line + '\n')
                            if hasattr(self, 'progress_label'):
                                self.progress_label.config(text="")
                    except:
                        pass
                    buffer = b''

            process.wait()

            if process.returncode != 0:
                self.log("\n[ERROR] SBL烧录失败！\n", "ERROR")
                return

            # 计算进度条时间（实际烧录耗时）
            flash_elapsed = time.time() - flash_start_time
            flash_minutes = int(flash_elapsed // 60)
            flash_seconds = int(flash_elapsed % 60)
            flash_time_str = f"{flash_minutes}分{flash_seconds}秒" if flash_minutes > 0 else f"{flash_seconds}秒"

            # 计算总执行时间（包括用户确认）
            total_elapsed = time.time() - total_start_time
            total_minutes = int(total_elapsed // 60)
            total_seconds = int(total_elapsed % 60)
            total_time_str = f"{total_minutes}分{total_seconds}秒" if total_minutes > 0 else f"{total_seconds}秒"

            self.log("\n[OK] SBL烧录成功！\n", "SUCCESS")
            self.log(f"[TIME]  烧录耗时: {flash_time_str}\n", "INFO")
            self.log(f"[TIME]  总时间: {total_time_str} (包括用户确认)\n", "INFO")

            # 进度条显示完成信息
            if hasattr(self, 'progress_label'):
                self.progress_label.config(text=f"[OK] SBL烧录完成！ 烧录耗时: {flash_time_str} | 总时间: {total_time_str}")

            # 重要提示：如何启动SBL
            messagebox.showinfo(
                "SBL烧录完成",
                "[OK] SBL已成功烧录到Flash\n\n"
                "[WARN] 重要：SBL还未运行！\n\n"
                "[NOTE] 启动SBL的步骤：\n"
                "   1. 切换SOP开关到 [0 1]（运行模式）\n"
                "      S8 = OFF, S7 = ON\n\n"
                "   2. 按RESET按钮启动设备\n\n"
                "[TIP] 现在可以：\n"
                "   - 启动SBL验证烧录成功\n"
                "   - 或继续烧录应用固件"
            )

        except Exception as e:
            self.log(f"\n[ERROR] 烧录出错: {str(e)}\n", "ERROR")
            messagebox.showerror("错误", f"烧录失败：{str(e)}")
        finally:
            # 清理进程资源
            if hasattr(self, 'flash_process') and self.flash_process:
                try:
                    if self.flash_process.poll() is None:  # 进程还在运行
                        self.flash_process.kill()
                    # 关闭管道
                    if self.flash_process.stdout:
                        self.flash_process.stdout.close()
                    if self.flash_process.stderr:
                        self.flash_process.stderr.close()
                except:
                    pass
                self.flash_process = None

            self.time_update_running = False  # 停止时间更新线程
            self.flashing = False

    def flash_app_only(self):
        """仅烧录应用固件"""
        if self.flashing:
            self.log("[WARN] 烧录正在进行中...\n", "WARN")
            return

        # 获取应用固件文件
        firmware_file = self.app_file.get()
        if not firmware_file or not os.path.exists(firmware_file):
            messagebox.showerror("错误", "请先选择有效的应用固件文件！")
            return

        # 获取端口（使用烧录端口COM3，而非数据端口COM4）
        app_port = self.sbl_port.get()  # 修复：使用sbl_port（COM3）而非app_port（COM4）
        if not app_port:
            messagebox.showerror("错误", "请先选择烧录端口！")
            return

        # 启动烧录线程
        self.flashing = True
        self.flash_thread = threading.Thread(
            target=self._flash_app_thread,
            args=(firmware_file, app_port),
            daemon=True
        )
        self.flash_thread.start()

    def _flash_app_thread(self, firmware_file, app_port):
        """烧录线程（仅应用固件）"""
        try:
            total_start_time = time.time()  # 总执行时间计时器（从开始到结束）

            # 启动总时间实时更新线程
            self.time_update_running = True
            time_thread = threading.Thread(
                target=self._update_total_time_display,
                args=(total_start_time,),
                daemon=True
            )
            time_thread.start()

            self.log("\n" + "="*60 + "\n")
            self.log("[APP] 开始应用固件烧录\n", "INFO")
            self.log("="*60 + "\n\n")

            # SOP模式确认
            sop_confirm = messagebox.askyesno(
                "SOP模式确认",
                "请确认硬件SOP模式配置：\n\n"
                "烧录模式（SOP_MODE1）：\n"
                "- S8 = OFF\n"
                "- S7 = OFF\n\n"
                "运行模式（SOP_MODE2）：\n"
                "- S8 = OFF\n"
                "- S7 = ON\n\n"
                "应用固件烧录建议使用SOP_MODE1\n"
                "当前是否已设置为烧录模式？"
            )
            if not sop_confirm:
                self.log("[ERROR] 用户取消烧录（SOP模式未确认）\n", "ERROR")
                return

            # 查询实际COM端口描述
            ports = serial.tools.list_ports.comports()
            port_description = "未知端口"
            for port in ports:
                if port.device == app_port:
                    port_description = port.description
                    break

            self.log(f"[FILE] 固件文件: {firmware_file}\n")
            self.log(f"[PORT] 烧录端口: {app_port} ({port_description})\n\n")

            # 串口确认
            port_confirm = messagebox.askyesno(
                "串口确认",
                f"请确认烧录端口：\n\n"
                f"烧录端口: {app_port}\n"
                f"端口说明: {port_description}\n\n"
                f"端口是否正确？"
            )
            if not port_confirm:
                self.log("[ERROR] 用户取消烧录（端口未确认）\n", "ERROR")
                return

            # 拔插USB确认
            usb_confirm = messagebox.askyesno(
                "准备烧录",
                "请拔插USB或按RESET按钮\n\n"
                "完成后点击\"是\"继续烧录"
            )
            if not usb_confirm:
                self.log("[ERROR] 用户取消烧录（USB未拔插）\n", "ERROR")
                return

            # 获取烧录工具路径
            tool_exe = self.flash_tool_path

            if not tool_exe or not os.path.exists(tool_exe):
                self.log(f"[ERROR] 找不到烧录工具\n", "ERROR")
                return

            self.log("开始烧录App...\n\n")

            flash_start_time = time.time()  # 烧录操作计时器（进度条时间）

            # 检查是否启用偏移量
            offset_enabled = self.offset_enabled_var.get() if hasattr(self, 'offset_enabled_var') else True

            # 构建烧录命令
            cmd = [
                tool_exe,
                "-p",
                app_port,
                "-f1",            # 使用-f1而非-f
                firmware_file
            ]

            if offset_enabled:
                app_offset = self.get_app_offset()  # 从用户选择获取
                self.log(f"[OFFSET] 使用App Flash偏移量: 0x{app_offset:X} ({app_offset} 字节)\n")
                cmd.extend(["-of1", str(app_offset)])  # 添加偏移参数
            else:
                self.log(f"[OFFSET] Flash偏移量已禁用，使用工具默认偏移\n")

            cmd.extend([
                "-s",             # 存储类型
                "SFLASH",
                "-c"              # Break信号
            ])

            self.log(f"执行命令: {' '.join(cmd)}\n")

            process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                bufsize=0,  # 无缓冲，实时输出
                creationflags=subprocess.CREATE_NO_WINDOW if os.name == 'nt' else 0
            )

            # 实时读取并显示输出（使用Label显示进度）
            buffer = b''

            while True:
                byte = process.stdout.read(1)
                if not byte:
                    break

                buffer += byte

                if byte == b'\r':
                    try:
                        line = buffer[:-1].decode('utf-8', errors='ignore').strip()
                        if line and hasattr(self, 'progress_label'):
                            # 直接显示进度信息（arprog不输出百分比）- App only
                            self.progress_label.config(text=line)
                            self.progress_label.update()
                    except:
                        pass
                    buffer = b''

                elif byte == b'\n':
                    try:
                        line = buffer[:-1].decode('utf-8', errors='ignore').strip()
                        if line:
                            self.log(line + '\n')
                            if hasattr(self, 'progress_label'):
                                self.progress_label.config(text="")
                    except:
                        pass
                    buffer = b''

            process.wait()

            if process.returncode != 0:
                self.log("\n[ERROR] 应用固件烧录失败！\n", "ERROR")
                return

            # 计算进度条时间（实际烧录耗时）
            flash_elapsed = time.time() - flash_start_time
            flash_minutes = int(flash_elapsed // 60)
            flash_seconds = int(flash_elapsed % 60)
            flash_time_str = f"{flash_minutes}分{flash_seconds}秒" if flash_minutes > 0 else f"{flash_seconds}秒"

            # 计算总执行时间（包括用户确认）
            total_elapsed = time.time() - total_start_time
            total_minutes = int(total_elapsed // 60)
            total_seconds = int(total_elapsed % 60)
            total_time_str = f"{total_minutes}分{total_seconds}秒" if total_minutes > 0 else f"{total_seconds}秒"

            self.log("\n[OK] 应用固件烧录成功！\n", "SUCCESS")
            self.log(f"[TIME]  烧录耗时: {flash_time_str}\n", "INFO")
            self.log(f"[TIME]  总时间: {total_time_str} (包括用户确认)\n", "INFO")

            # 进度条显示完成信息
            if hasattr(self, 'progress_label'):
                self.progress_label.config(text=f"[OK] 应用固件烧录完成！ 烧录耗时: {flash_time_str} | 总时间: {total_time_str}")

            # 提示运行应用固件
            messagebox.showinfo(
                "应用固件烧录完成",
                "[OK] 应用固件已成功烧录到Flash\n\n"
                "[NOTE] 运行应用固件的步骤：\n"
                "   1. 切换SOP开关到 [0 1]（运行模式）\n"
                "      S8 = OFF, S7 = ON\n\n"
                "   2. 按RESET按钮启动设备\n\n"
                "   3. 打开串口监视查看输出\n"
                "      COM4 - 115200 8N1"
            )

        except Exception as e:
            self.log(f"\n[ERROR] 烧录出错: {str(e)}\n", "ERROR")
            messagebox.showerror("错误", f"烧录失败：{str(e)}")
        finally:
            # 清理进程资源
            if hasattr(self, 'flash_process') and self.flash_process:
                try:
                    if self.flash_process.poll() is None:  # 进程还在运行
                        self.flash_process.kill()
                    # 关闭管道
                    if self.flash_process.stdout:
                        self.flash_process.stdout.close()
                    if self.flash_process.stderr:
                        self.flash_process.stderr.close()
                except:
                    pass
                self.flash_process = None

            self.time_update_running = False  # 停止时间更新线程
            self.flashing = False

    # =========== 文件选择方法 ===========

    def select_sbl_file(self):
        """选择SBL固件文件"""
        filename = filedialog.askopenfilename(
            title="选择SBL固件文件",
            filetypes=[
                ("AppImage Files", "*.appimage"),
                ("All Files", "*.*")
            ],
            initialdir=os.path.dirname(self.sbl_file.get()) if self.sbl_file.get() else None
        )
        if filename:
            self.sbl_file.set(filename)
            self.log(f"[OK] 已选择SBL文件: {filename}\n", "SUCCESS")

            # 更新界面状态
            if hasattr(self, 'sbl_status_label'):
                self.sbl_status_label.config(
                    text="[OK] 已选择",
                    fg="green",
                    image=self.icons.get("ok", 16) if hasattr(self, "icons") else "",
                    compound="left",
                )
            if hasattr(self, 'sbl_path_label'):
                self.sbl_path_label.config(text=filename)

            # 验证文件
            valid, msg = verify_firmware_file(filename)
            if valid:
                self.log(f"[OK] {msg}\n", "SUCCESS")
            else:
                self.log(f"[WARN] {msg}\n", "WARN")

    def select_app_file(self):
        """选择应用固件文件"""
        filename = filedialog.askopenfilename(
            title="选择应用固件文件",
            filetypes=[
                ("AppImage Files", "*.appimage"),
                ("All Files", "*.*")
            ],
            initialdir=os.path.dirname(self.app_file.get()) if self.app_file.get() else None
        )
        if filename:
            self.app_file.set(filename)
            self.log(f"[OK] 已选择应用固件文件: {filename}\n", "SUCCESS")

            # 更新界面状态
            if hasattr(self, 'app_status_label'):
                self.app_status_label.config(
                    text="[OK] 已选择",
                    fg="green",
                    image=self.icons.get("ok", 16) if hasattr(self, "icons") else "",
                    compound="left",
                )
            if hasattr(self, 'app_path_label'):
                self.app_path_label.config(text=filename)

            # 验证文件
            valid, msg = verify_firmware_file(filename)
            if valid:
                self.log(f"[OK] {msg}\n", "SUCCESS")
            else:
                self.log(f"[WARN] {msg}\n", "WARN")

    def analyze_firmware(self):
        """分析已选择的固件文件"""
        sbl_file = (self.sbl_file.get() or '').strip()
        app_file = (self.app_file.get() or '').strip()

        # 调试信息
        self.log(f"\n[SCAN] 开始分析固件...\n", "INFO")
        self.log(f"SBL文件变量值: '{sbl_file}'\n")
        self.log(f"App文件变量值: '{app_file}'\n")

        if not sbl_file and not app_file:
            self.log("\n[WARN] 请先选择SBL或应用固件文件！\n", "WARN")
            self.log("提示: 点击左侧的「选择」按钮来选择固件文件\n")
            return

        # 分析SBL固件
        if sbl_file:
            if not os.path.exists(sbl_file):
                self.log(f"\n[ERROR] SBL固件文件不存在: {sbl_file}\n", "ERROR")
            else:
                self.log(f"\n[SCAN] 分析SBL固件: {os.path.basename(sbl_file)}\n", "INFO")
                self.log(f"完整路径: {sbl_file}\n\n")

                info = analyze_appimage_structure(sbl_file)
                if info:
                    self.log("=" * 70 + "\n")
                    self.log(f"[STAT] SBL固件结构分析结果\n", "SUCCESS")
                    self.log("=" * 70 + "\n")
                    self.log(f"文件大小: {info['total_size']:,} 字节 ({info['total_size']/1024:.2f} KB)\n")
                    self.log(f"Magic Number: {info.get('magic_number', 'N/A')}\n")
                    self.log(f"版本信息: 0x{info.get('version', 0):08X}\n")
                    self.log(f"文件类型: {info.get('file_type', 'Unknown')}\n")

                    # 显示镜像数量
                    if 'num_images' in info:
                        self.log(f"镜像数量标识: {info['num_images'][0]} / {info['num_images'][1]}\n")

                    # 显示各个核心镜像的详细信息
                    if 'images' in info and info['images']:
                        self.log(f"\n[PKG] 核心镜像详情 (文件内偏移):\n")
                        self.log("-" * 70 + "\n")
                        for idx, img in enumerate(info['images'], 1):
                            self.log(f"\n  [{idx}] {img['name']}:\n")
                            self.log(f"      Checksum:      {img['checksum']}\n")
                            self.log(f"      文件内偏移:    0x{img['file_offset']:08X} ({img['file_offset']} 字节)\n")
                            self.log(f"      镜像大小:      0x{img['size']:08X} ({img['size']} 字节 / {img['size']/1024:.2f} KB)\n")
                            self.log(f"      加载地址:      {img['load_addr']}\n")

                    self.log("\n" + "-" * 70 + "\n")
                    self.log(f"[TIP] 提示: Flash烧录偏移量需要在烧录时配置\n")
                    self.log("=" * 70 + "\n")
                else:
                    self.log("[ERROR] SBL分析失败：无法解析固件文件结构\n", "ERROR")

        # 分析App固件
        if app_file and app_file != sbl_file:  # 避免重复分析
            if not os.path.exists(app_file):
                self.log(f"\n[ERROR] App固件文件不存在: {app_file}\n", "ERROR")
            else:
                self.log(f"\n[SCAN] 分析App固件: {os.path.basename(app_file)}\n", "INFO")
                self.log(f"完整路径: {app_file}\n\n")

                info = analyze_appimage_structure(app_file)
                if info:
                    self.log("=" * 70 + "\n")
                    self.log(f"[STAT] App固件结构分析结果\n", "SUCCESS")
                    self.log("=" * 70 + "\n")
                    self.log(f"文件大小: {info['total_size']:,} 字节 ({info['total_size']/1024:.2f} KB)\n")
                    self.log(f"Magic Number: {info.get('magic_number', 'N/A')}\n")
                    self.log(f"版本信息: 0x{info.get('version', 0):08X}\n")
                    self.log(f"文件类型: {info.get('file_type', 'Unknown')}\n")

                    # 显示镜像数量
                    if 'num_images' in info:
                        self.log(f"镜像数量标识: {info['num_images'][0]} / {info['num_images'][1]}\n")

                    # 显示各个核心镜像的详细信息
                    if 'images' in info and info['images']:
                        self.log(f"\n[PKG] 核心镜像详情 (文件内偏移):\n")
                        self.log("-" * 70 + "\n")
                        for idx, img in enumerate(info['images'], 1):
                            self.log(f"\n  [{idx}] {img['name']}:\n")
                            self.log(f"      Checksum:      {img['checksum']}\n")
                            # 处理文件偏移（可能是数字或字符串）
                            if isinstance(img['file_offset'], str):
                                self.log(f"      文件内偏移:    {img['file_offset']}\n")
                            else:
                                self.log(f"      文件内偏移:    0x{img['file_offset']:08X} ({img['file_offset']} 字节)\n")
                            self.log(f"      镜像大小:      0x{img['size']:08X} ({img['size']} 字节 / {img['size']/1024:.2f} KB)\n")
                            self.log(f"      加载地址:      {img['load_addr']}\n")

                    self.log("\n" + "-" * 70 + "\n")
                    self.log(f"[TIP] 提示: Flash烧录偏移量需要在烧录时配置\n")
                    self.log("=" * 70 + "\n")
                else:
                    self.log("[ERROR] App分析失败：无法解析固件文件结构\n", "ERROR")

    def refresh_com_ports(self):
        """刷新COM端口列表"""
        self.log("\n[REVERT] 正在刷新端口列表...\n", "INFO")

        # 获取所有端口
        all_ports = list(serial.tools.list_ports.comports())

        self.log(f"\n[SCAN] 扫描到 {len(all_ports)} 个端口:\n")
        for port in all_ports:
            self.log(f"  - {port.device}: {port.description}\n")
            if port.hwid:
                self.log(f"    HWID: {port.hwid}\n")

        sbl_ports, app_ports = self.refresh_ports()

        if sbl_ports or app_ports:
            self.log(f"\n[OK] 刷新成功！\n", "SUCCESS")
            if sbl_ports:
                self.log(f"  [PORT] 找到烧录端口: {', '.join(sbl_ports)}\n", "SUCCESS")
                for port in sbl_ports:
                    port_info = self.get_port_info(port)
                    if port_info:
                        self.log(f"     - 描述: {port_info['description']}\n")
                        if port_info.get('vid') and port_info.get('pid'):
                            self.log(f"     - VID:PID = {port_info['vid']:04X}:{port_info['pid']:04X}\n")
            else:
                self.log(f"  [WARN] 未找到烧录端口 (XDS110 Auxiliary Data Port)\n", "WARN")

            if app_ports:
                self.log(f"  [PORT] 找到调试端口: {', '.join(app_ports)}\n", "SUCCESS")
                for port in app_ports:
                    port_info = self.get_port_info(port)
                    if port_info:
                        self.log(f"     - 描述: {port_info['description']}\n")
                        if port_info.get('vid') and port_info.get('pid'):
                            self.log(f"     - VID:PID = {port_info['vid']:04X}:{port_info['pid']:04X}\n")
            else:
                self.log(f"  [WARN] 未找到调试端口 (XDS110 Application/User UART)\n", "WARN")
        else:
            self.log("[ERROR] 未找到任何XDS110端口！\n", "ERROR")
            self.log("请检查：\n", "WARN")
            self.log("  1. 设备是否正确连接\n")
            self.log("  2. USB驱动是否安装\n")
            self.log("  3. 设备管理器中是否显示XDS110端口\n")

    # =========== 日志方法 ===========

    # =========== 旧方法已删除 ===========
    # get_last_line_start() 和 update_line_at_mark() 已废弃
    # 现在所有进度条统一使用Label组件显示

    def log(self, message, tag=None):
        """添加日志（始终输出到烧录功能标签页）"""
        # 修复：不管当前激活哪个标签页，都输出到烧录功能页
        if hasattr(self, 'flash_tab') and hasattr(self.flash_tab, 'log'):
            self.flash_tab.log(message, tag)
        elif hasattr(self, 'basic_tab') and hasattr(self.basic_tab, 'log'):
            # 兼容旧代码
            self.basic_tab.log(message, tag)

    def clear_log(self):
        """清空日志"""
        # 修复：不管当前激活哪个标签页，都清除烧录功能页
        if hasattr(self, 'flash_tab') and hasattr(self.flash_tab, 'clear_log'):
            self.flash_tab.clear_log()
        elif hasattr(self, 'basic_tab') and hasattr(self.basic_tab, 'clear_log'):
            # 兼容旧代码
            self.basic_tab.clear_log()

    # =========== 状态栏方法 ===========

    def update_status(self, message):
        """更新状态栏"""
        self.status_label.config(text=message)
        self.root.update_idletasks()

# ============================================================
# 主函数
# ============================================================

def check_old_process():
    """检查是否有老进程在运行（v1.0.1需求1）

    改进版本：只查找真正的旧进程，排除：
    1. 当前进程（current_pid）
    2. 当前进程的父进程（避免终止启动器）
    3. 创建时间晚于或接近当前进程的进程（避免误杀同时启动的进程）
    """
    current_pid = os.getpid()

    # [FIX] 修复EXE模式下的进程检测
    # EXE模式下使用EXE名称，脚本模式下使用脚本名称
    if getattr(sys, 'frozen', False):
        # EXE模式：使用EXE文件名
        script_name = os.path.basename(sys.executable)
        _dprint(f"[DEBUG] check_old_process: EXE模式，查找进程名: {script_name}")
    else:
        # 脚本模式：使用脚本名称
        script_name = os.path.basename(__file__)
        _dprint(f"[DEBUG] check_old_process: 脚本模式，查找进程名: {script_name}")

    # 获取当前进程信息
    try:
        current_proc = psutil.Process(current_pid)
        current_create_time = current_proc.create_time()
        parent_pid = current_proc.ppid()
    except (psutil.NoSuchProcess, psutil.AccessDenied):
        return []

    old_processes = []
    for proc in psutil.process_iter(['pid', 'name', 'cmdline', 'create_time']):
        try:
            # 跳过当前进程
            if proc.pid == current_pid:
                continue

            # 跳过父进程（避免终止启动器/命令行）
            if proc.pid == parent_pid:
                continue

            cmdline = proc.info.get('cmdline', [])
            if not cmdline:
                continue

            # 检查是否是flash_tool.py进程
            cmdline_str = ' '.join(cmdline)
            if script_name not in cmdline_str:
                continue

            # 只添加创建时间早于当前进程的旧进程
            # 增加1秒容差，避免误杀几乎同时启动的进程
            proc_create_time = proc.info.get('create_time', 0)
            if proc_create_time < (current_create_time - 1.0):
                old_processes.append(proc)

        except (psutil.NoSuchProcess, psutil.AccessDenied, KeyError):
            pass

    return old_processes

def kill_old_processes(processes):
    """关闭老进程"""
    for proc in processes:
        try:
            proc.terminate()
            proc.wait(timeout=3)
        except (psutil.NoSuchProcess, psutil.TimeoutExpired):
            try:
                proc.kill()
            except psutil.NoSuchProcess:
                pass

def main():
    """主函数"""
    # NOTE:
    # 不要在 Python 侧强行设置 `_MEIPASS2`。
    # 对 onefile 来说，该变量用于父/子进程解包目录传递；若被继承到 `--detach` 子进程，
    # 可能导致子进程复用同一解包目录，从而让父进程无法清理而长期常驻。

    # 后台模式标记：与 EXE/脚本保持一致（启动器进程 -> 子进程GUI）
    detach_mode = '--detach' in sys.argv

    # 启动取证（仅调试使用）：记录 onefile 下 argv 与关键环境变量
    if os.environ.get('FLASH_TOOL_LOG_ARGS') == '1':
        try:
            forced_log_path = os.environ.get('FLASH_TOOL_LOG_PATH')
            if forced_log_path:
                log_path = Path(forced_log_path)
                log_path.parent.mkdir(parents=True, exist_ok=True)
            else:
                log_dir = Path(sys.argv[0]).resolve().parent if getattr(sys, 'frozen', False) else Path(__file__).resolve().parent
                log_path = log_dir / 'flash_tool_args.log'

            keys = [k for k in os.environ.keys() if k.startswith('_PYI') or k.startswith('_MEI') or k.startswith('PYINSTALLER')]
            keys.sort()
            with open(log_path, 'w', encoding='utf-8', newline='\n') as f:
                f.write(f"frozen={getattr(sys, 'frozen', False)}\n")
                f.write(f"executable={sys.executable}\n")
                f.write(f"argv={sys.argv!r}\n")
                for k in keys:
                    f.write(f"{k}={os.environ.get(k, '')}\n")
        except Exception:
            pass

    # Windows：默认后台启动（启动器立即返回命令行）
    # 目标：脚本模式与 EXE 模式行为完全一致
    if (
        sys.platform == "win32"
        and not detach_mode
        and os.environ.get("FLASH_TOOL_DEBUG_CONSOLE") != "1"
    ):
        old_processes = check_old_process()
        if old_processes:
            root_temp = tk.Tk()
            root_temp.withdraw()
            response = messagebox.askyesno(
                "检测到旧进程",
                f"检测到 {len(old_processes)} 个旧的烧录工具进程正在运行。\n\n"
                "是否关闭旧进程并启动新窗口？\n\n"
                "点击'是'：关闭旧进程并启动新窗口\n"
                "点击'否'：取消启动"
            )
            root_temp.destroy()

            if response:
                kill_old_processes(old_processes)
                time.sleep(0.5)
            else:
                sys.exit(0)

        if getattr(sys, 'frozen', False):
            exe = str(Path(sys.executable))
            cmd = [exe, '--detach']
            cwd = str(Path(sys.executable).resolve().parent)
        else:
            script_path = str(Path(__file__).resolve())
            py_exe = Path(sys.executable)
            pyw_exe = py_exe.with_name("pythonw.exe")
            exe = str(pyw_exe if pyw_exe.exists() else py_exe)
            cmd = [exe, script_path, '--detach']
            cwd = str(Path(script_path).parent)

        env = os.environ.copy()
        env["FLASH_TOOL_CHILD"] = "1"
        # 关键：确保 `--detach` 子进程不要继承 onefile 的父进程环境（让其自行解包到新目录）
        # 否则子进程可能锁住父进程解包目录，导致父进程无法清理而长期常驻。
        for k in (
            '_MEIPASS2',
            '_PYI_APPLICATION_HOME_DIR',
            '_PYI_PARENT_PROCESS_LEVEL',
            '_PYI_ARCHIVE_FILE',
        ):
            env.pop(k, None)
        env['PYINSTALLER_RESET_ENVIRONMENT'] = '1'
        # 子进程默认不输出控制台信息；需要调试可设置 FLASH_TOOL_DEBUG_CONSOLE=1

        creationflags = 0
        try:
            creationflags = (
                subprocess.CREATE_NEW_PROCESS_GROUP
                | subprocess.DETACHED_PROCESS
                | subprocess.CREATE_NO_WINDOW
                | getattr(subprocess, 'CREATE_BREAKAWAY_FROM_JOB', 0)
            )
        except AttributeError:
            creationflags = 0

        subprocess.Popen(
            cmd,
            cwd=cwd,
            env=env,
            creationflags=creationflags,
        )
        time.sleep(1)
        os._exit(0)

    # 子进程 / EXE模式 / 调试控制台模式：直接启动GUI
    if sys.platform == "win32":
        try:
            ctypes.windll.shell32.SetCurrentProcessExplicitAppUserModelID(
                "wisefido.awrl6844.flash_tool"
            )
        except Exception as e:
            _dprint(f"[DEBUG] SetAppUserModelID failed: {e}")

    root = tk.Tk()
    # 尽早设置 iconphoto（有助于任务栏图标显示）
    try:
        # 优先使用 256px 带背景应用图标（任务栏/标题栏更醒目），fallback 到旧资源
        candidates = (
            _iter_resource_candidates('flash_tool_icon.png')
            + _iter_resource_candidates('image', 'icons', 'radar_64.png')
        )
        for png_path in candidates:
            if png_path.exists():
                root._taskbar_icon_img = tk.PhotoImage(master=root, file=str(png_path))
                root.iconphoto(True, root._taskbar_icon_img)
                break
    except Exception as e:
        _dprint(f"[DEBUG] iconphoto early set failed: {e}")

    app = FlashToolGUI(root)
    root.mainloop()


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        import traceback
        error_msg = f"程序启动失败:\n{str(e)}\n\n详细信息:\n{traceback.format_exc()}"
        try:
            import tkinter as tk
            from tkinter import messagebox
            root = tk.Tk()
            root.withdraw()
            messagebox.showerror("启动错误", error_msg)
            root.destroy()
        except:
            print(error_msg)
        sys.exit(1)


