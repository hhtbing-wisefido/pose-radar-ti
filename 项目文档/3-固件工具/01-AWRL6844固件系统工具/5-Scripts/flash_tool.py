#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Ti AWRL6844 固件烧录工具 v1.6.3 - 确认烧录功能完整性
主入口文件 - 单一烧录功能标签页

更新日志 v1.6.3:
- 确认3种烧录方式完整实现（完整烧录、仅SBL、仅App）
- 确认所有烧录方法使用正确命令格式（-f1/-of1/-s SFLASH/-c）
- 确认依次烧录策略（根据测试报告验证）
- 删除烧录超时相关功能
- 根据实际测试优化烧录流程
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

# 版本信息
VERSION = "1.6.5"
BUILD_DATE = "2025-12-19"
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
        'sbl_port_name': 'XDS110 Class Application/User UART',     # COM3 - 用于Flash烧录
        'app_port_name': 'XDS110 Class Auxiliary Data Port',       # COM4 - 雷达数据输出端口
        'sbl_baudrate': 115200,
        'app_baudrate': 115200,
        # Flash地址配置
        'sbl_offset': 0x2000,      # SBL烧录地址（8KB偏移）
        'app_offset': 0x42000,     # App烧录地址（264KB偏移）
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
        reasons.append(f"✅ 文件名包含{device}型号标识")
        details.append(f"文件名: {filename}")
    else:
        is_compatible = False
        reasons.append(f"⚠️ 文件名未包含{device}型号标识")
        details.append(f"文件名: {filename}")
        details.append(f"期望关键字: {', '.join(keywords)}")
    
    # 检查2: 分析固件结构
    try:
        info = analyze_appimage_structure(file_path)
        if info:
            if info['has_meta_header']:
                reasons.append("✅ 包含有效的Meta Header")
                details.append(f"Magic Number: {info['magic_number']}")
            else:
                is_compatible = False
                reasons.append("❌ Meta Header无效")
            
            if info['has_sbl_header'] and info['has_app_header']:
                reasons.append("✅ 包含SBL和App镜像")
                details.append(f"SBL大小: {info['sbl_size']} 字节")
                details.append(f"App大小: {info['app_size']} 字节")
            else:
                reasons.append("⚠️ 固件结构不完整")
        else:
            is_compatible = False
            reasons.append("❌ 无法解析固件结构")
    except Exception as e:
        is_compatible = False
        reasons.append(f"❌ 固件分析失败: {str(e)}")
    
    # 检查3: SDK工具检查
    if device == 'AWRL6844':
        expected_tool = 'arprog_cmdline_6844.exe'
        reasons.append(f"✅ 使用烧录工具: {expected_tool}")
        details.append(f"设备: {device}")
    
    # 汇总结果
    reason_text = "\n".join(reasons)
    details_text = "\n".join(details)
    
    return is_compatible, reason_text, details_text

def analyze_appimage_structure(file_path):
    """
    分析appimage文件结构（修正版）
    
    ⚠️ 重要说明：
    - .appimage文件内部的Meta Header记录的是【文件内相对偏移】
    - Flash烧录偏移是固定的：SBL=0x2000, App=0x42000（由SDK/sbl.h定义）
    - 本函数返回【Flash烧录偏移】，而非文件内偏移
    
    AppImage文件结构：
    - Meta Header (512字节): 包含Magic、版本、各核镜像信息
    - 实际镜像数据: R5F + DSP + RF固件
    
    Returns:
        dict: 包含文件结构信息和Flash烧录偏移
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
            
            # 获取文件总大小
            f.seek(0, 2)
            total_size = f.tell()
            
            # 判断文件类型（根据大小和文件名）
            filename = os.path.basename(file_path).lower()
            is_sbl = 'sbl' in filename or total_size < 200*1024
            
            # ⚠️ 关键修正：返回Flash烧录偏移，而非文件内偏移
            # 这些值来自官方SDK的sbl.h定义
            FLASH_SBL_OFFSET = 0x2000    # M_META_SBL_OFFSET
            FLASH_APP_OFFSET = 0x42000   # M_META_IMAGE_OFFSET
            
            if is_sbl:
                # SBL固件
                info = {
                    'total_size': total_size,
                    'has_meta_header': magic == 0x5254534D,
                    'magic_number': hex(magic),
                    'version': version,
                    'sbl_offset': FLASH_SBL_OFFSET,  # Flash烧录偏移
                    'sbl_size': total_size,          # 整个文件就是SBL
                    'app_offset': 0,
                    'app_size': 0,
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
                    'sbl_offset': 0,
                    'sbl_size': 0,
                    'app_offset': FLASH_APP_OFFSET,  # Flash烧录偏移
                    'app_size': total_size,          # 整个文件就是App
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
                    details.append("✓ 发现SBL特征字符串")
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
            details.append("✓ 串口可以正常打开")
            details.append("✓ 设备已连接")
            details.append("⚠ 未收到SBL输出（可能SBL已运行完毕或未上电复位）")
        
        ser.close()
        
        details_text = "\n".join(details)
        
        if has_response:
            return True, "✅ 检测到SBL存在（串口有响应）", details_text
        else:
            return False, "⚠️ 未检测到SBL响应（建议复位设备后重试）", details_text
        
    except serial.SerialException as e:
        return False, f"❌ 串口打开失败: {str(e)}", f"端口: {port}\n错误: {str(e)}"
    except Exception as e:
        return False, f"❌ 检测失败: {str(e)}", f"异常: {str(e)}"

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
        
        ttk.Label(frame, text="⚠️ 请确认以下事项：", 
                 font=('Arial', 12, 'bold')).pack(pady=10)
        
        checks = [
            "✓ 固件文件已正确选择",
            "✓ 设备已通过USB连接到电脑",
            "✓ 设备电源已打开",
            "✓ 串口没有被其他程序占用",
            "✓ 已保存当前工作"
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
            text="🔍 SBL存在性检测",
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
            text="⏳ 正在检测...",
            font=('Arial', 11),
            foreground='blue'
        )
        self.status_label.pack(pady=10)
        
        # 详细信息区域
        detail_frame = ttk.LabelFrame(
            self,
            text="📋 检测详情",
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
            self.log("\n✅ 结论: 板载已有SBL，可以只烧录App更新应用\n")
        else:
            self.log("\n⚠️ 结论: 建议执行完整烧录（SBL + App）\n")
        
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
            self.log(f"✓ 已连接到 {self.port}\n")
        except Exception as e:
            self.log(f"✗ 连接失败: {str(e)}\n")
            
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
                    self.log(f"\n✗ 读取错误: {str(e)}\n")
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
        self.root.title(f"Ti AWRL6844 固件烧录工具 v{VERSION}")
        self.root.geometry("1000x700")
        
        # 强制窗口置顶并获得焦点
        self.root.lift()
        self.root.focus_force()
        self.root.attributes('-topmost', True)
        self.root.after(100, lambda: self.root.attributes('-topmost', False))
        
        # 版本信息（供标签页模块验证）
        self.VERSION = VERSION
        self.BUILD_DATE = BUILD_DATE
        
        # 设备配置
        self.device_config = DEVICE_CONFIGS['AWRL6844']
        
        # 状态变量
        self.sbl_file = tk.StringVar()  # SBL固件文件
        self.app_file = tk.StringVar()  # App固件文件
        self.flash_tool_path = ""  # 烧录工具路径（改为字符串）
        self.sbl_port = tk.StringVar()
        self.app_port = tk.StringVar()
        
        # 烧录状态
        self.flashing = False
        self.flash_thread = None
        self.flash_process = None  # 当前烧录进程
        self.stop_flashing = False  # 停止烧录标志
        
        # 创建界面
        self.create_widgets()
        
        # 初始化默认固件路径（使用动态相对路径）- 必须在界面创建后
        self._init_default_firmware_paths()
        
        # 初始化端口
        self.refresh_ports()
        
        # 检测烧录工具
        self.check_flash_tool()
    
    def _init_default_firmware_paths(self):
        """初始化默认固件路径（动态相对路径，项目移动后自动适配）"""
        try:
            # 获取当前脚本的绝对路径
            script_dir = Path(__file__).resolve().parent
            
            # 构建固件文件的相对路径
            sbl_path = script_dir.parent / "1-SBL_Bootloader" / "sbl.release.appimage"
            app_path = script_dir.parent / "2-HelloWorld_App" / "hello_world_system.release.appimage"
            
            # 检查SBL文件是否存在并设置
            if sbl_path.exists():
                self.sbl_file.set(str(sbl_path))
                # 更新界面显示
                if hasattr(self, 'sbl_status_label'):
                    self.sbl_status_label.config(text="✅ 已找到", fg="green")
                if hasattr(self, 'sbl_path_label'):
                    self.sbl_path_label.config(text=str(sbl_path))
                self.log(f"✅ 自动加载SBL固件: {sbl_path}\n", "SUCCESS")
            else:
                self.sbl_file.set("")
                if hasattr(self, 'sbl_status_label'):
                    self.sbl_status_label.config(text="❌ 未找到", fg="red")
                
            # 检查App文件是否存在并设置
            if app_path.exists():
                self.app_file.set(str(app_path))
                # 更新界面显示
                if hasattr(self, 'app_status_label'):
                    self.app_status_label.config(text="✅ 已找到", fg="green")
                if hasattr(self, 'app_path_label'):
                    self.app_path_label.config(text=str(app_path))
                self.log(f"✅ 自动加载App固件: {app_path}\n", "SUCCESS")
            else:
                self.app_file.set("")
                if hasattr(self, 'app_status_label'):
                    self.app_status_label.config(text="❌ 未找到", fg="red")
                
        except Exception as e:
            # 初始化失败时使用空值
            self.sbl_file.set("")
            self.app_file.set("")
            self.log(f"⚠️ 自动加载固件失败: {str(e)}\n", "WARN")
        
    def create_widgets(self):
        """创建界面组件 - 使用模块化标签页"""
        
        # 顶部标题
        title_frame = ttk.Frame(self.root)
        title_frame.pack(fill=tk.X, padx=10, pady=5)
        
        ttk.Label(
            title_frame,
            text=f"Ti AWRL6844 固件烧录工具 v{VERSION}",
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
        self.notebook.add(flash_frame, text="  烧录功能  ")
        self.notebook.add(firmware_manager_frame, text="  固件管理  ")
        
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

    def open_serial_monitor(self, port, baudrate=115200):
        """打开串口监视（输出到日志区）"""
        if not port:
            self.log("\n⚠️ 未指定端口，无法打开监视器\n", "WARN")
            return
        self.log(f"\n📡 打开串口监视器: {port} @ {baudrate}\n", "INFO")
        stop_event = threading.Event()

        def _monitor():
            ser = None
            try:
                ser = serial.Serial(port, baudrate, timeout=0.1)
                self.log(f"✅ 监视器已连接 {port}\n")
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
                        self.log(f"\n❌ 串口读取错误: {str(e)}\n", "ERROR")
                        break
            except Exception as e:
                self.log(f"❌ 打开串口失败: {str(e)}\n", "ERROR")
            finally:
                try:
                    if ser:
                        ser.close()
                except Exception:
                    pass
                self.log(f"📴 监视器已关闭: {port}\n")

        # 启动后台线程（一次性监视会话，不保存引用，关闭窗口时自动结束）
        t = threading.Thread(target=_monitor, daemon=True)
        t.start()

    def release_port(self, port):
        """尝试释放指定端口（只关闭占用该端口的句柄，不关闭应用）"""
        if not port:
            self.log("\n⚠️ 未指定端口，无法释放\n", "WARN")
            messagebox.showwarning("警告", "请选择要释放的端口！")
            return False
        
        self.log(f"\n🔓 尝试释放端口: {port}\n", "INFO")
        
        # 首先尝试直接打开关闭（如果端口可用）
        try:
            ser = serial.Serial(port, 115200, timeout=0.2)
            ser.close()
            self.log(f"✅ 端口 {port} 可用，无需释放\n", "SUCCESS")
            messagebox.showinfo("成功", f"端口 {port} 可用，无需释放")
            return True
        except serial.SerialException as e:
            self.log(f"⚠️ 端口 {port} 当前被占用\n", "WARN")
        
        # 尝试关闭本应用内可能打开的串口连接
        self.log(f"🔧 尝试关闭本应用内对端口 {port} 的占用...\n", "INFO")
        
        # 检查是否有串口监视窗口打开了该端口
        # 注意：这里只是尝试，实际的串口监视窗口是独立线程，需要手动关闭
        
        # 再次尝试打开端口
        try:
            ser = serial.Serial(port, 115200, timeout=0.2)
            ser.close()
            self.log(f"✅ 端口 {port} 已释放！\n", "SUCCESS")
            messagebox.showinfo("成功", f"端口 {port} 已成功释放！")
            return True
        except serial.SerialException:
            pass
        
        # 查找可能占用该特定端口的外部进程
        self.log(f"🔎 查找占用端口 {port} 的外部进程...\n", "INFO")
        
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
            self.log("🔎 发现可能占用串口的工具:\n", "INFO")
            for pid, name in found:
                self.log(f"  • PID {pid}: {name}\n", "INFO")
            
            # 询问用户是否终止这些串口工具
            result = messagebox.askyesno(
                "发现串口工具进程",
                f"发现以下串口工具可能占用端口 {port}:\n\n" +
                "\n".join([f"• {name} (PID: {pid})" for pid, name in found]) +
                f"\n\n⚠️ 注意：只会关闭这些串口工具，不会关闭本应用。\n\n是否终止这些进程以释放端口？"
            )
            
            if result:
                killed = []
                for pid, name in found:
                    try:
                        proc = psutil.Process(pid)
                        proc.terminate()
                        proc.wait(timeout=3)
                        killed.append(name)
                        self.log(f"✅ 已终止进程: {name} (PID: {pid})\n", "SUCCESS")
                    except psutil.TimeoutExpired:
                        try:
                            proc.kill()
                            killed.append(name)
                            self.log(f"✅ 已强制终止进程: {name} (PID: {pid})\n", "SUCCESS")
                        except Exception as e:
                            self.log(f"❌ 无法终止进程 {name}: {str(e)}\n", "ERROR")
                    except Exception as e:
                        self.log(f"❌ 终止进程 {name} 失败: {str(e)}\n", "ERROR")
                
                # 等待一下，再次尝试打开端口
                time.sleep(0.5)
                try:
                    ser = serial.Serial(port, 115200, timeout=0.2)
                    ser.close()
                    self.log(f"✅ 端口 {port} 已成功释放！\n", "SUCCESS")
                    messagebox.showinfo("成功", f"端口 {port} 已释放！\n已终止: {', '.join(killed)}")
                    return True
                except Exception as e:
                    self.log(f"⚠️ 端口仍然被占用: {str(e)}\n", "WARN")
                    messagebox.showwarning(
                        "部分成功",
                        f"已终止部分进程，但端口仍被占用。\n\n可能原因：\n1. 本应用的串口监视窗口正在使用该端口（请手动关闭）\n2. 其他未知程序占用\n\n建议:\n1. 关闭串口监视窗口\n2. 重新插拔USB设备\n3. 在设备管理器中禁用/启用端口"
                    )
                    return False
            else:
                self.log("⚠️ 用户取消释放操作\n", "WARN")
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
        # 获取脚本所在目录的绝对路径
        script_dir = os.path.dirname(os.path.abspath(__file__))
        # 向上两级到达 01-AWRL6844固件系统工具
        tool_base = os.path.dirname(os.path.dirname(script_dir))
        
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
                    self.tool_status_label.config(text="✅ 已找到", fg="green")
                if hasattr(self, 'tool_path_label'):
                    self.tool_path_label.config(text=tool_exe)
                return True
        
        # 所有路径都不存在
        if hasattr(self, 'tool_status_label'):
            self.tool_status_label.config(text="❌ 未找到", fg="red")
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
            self.log(f"✅ 已选择自定义烧录工具: {filename}\n", "SUCCESS")
            
            # 更新下拉框中的自定义工具选项
            if hasattr(self, 'tool_combo'):
                from pathlib import Path
                # 获取flash_tab实例
                flash_tab = self.flash_tab
                
                # 添加或更新自定义工具选项
                custom_key = "✨ 自定义工具"
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
        self.log("🔍 开始测试所有端口...\n", "INFO")
        
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
        self.log(f"\n📌 测试烧录端口: {flash_port}\n", "INFO")
        try:
            ser = serial.Serial(flash_port, 115200, timeout=1)
            ser.close()
            self.log(f"✅ 端口 {flash_port} 连接正常！\n", "SUCCESS")
            results.append(f"✅ {flash_port} (烧录端口): 连接正常")
        except Exception as e:
            error_msg = f"❌ 端口 {flash_port} 连接失败: {str(e)}"
            self.log(f"{error_msg}\n", "ERROR")
            results.append(f"❌ {flash_port} (烧录端口): {str(e)}")
        
        # 测试数据输出端口（COM4 - Auxiliary Data Port）
        self.log(f"\n📌 测试数据输出端口: {debug_port}\n", "INFO")
        try:
            ser = serial.Serial(debug_port, 115200, timeout=1)
            ser.close()
            self.log(f"✅ 端口 {debug_port} 连接正常！\n", "SUCCESS")
            results.append(f"✅ {debug_port} (数据输出端口): 连接正常")
        except Exception as e:
            error_msg = f"❌ 端口 {debug_port} 连接失败: {str(e)}"
            self.log(f"{error_msg}\n", "ERROR")
            results.append(f"❌ {debug_port} (数据输出端口): {str(e)}")
        
        # 汇总结果
        self.log("\n" + "="*60 + "\n", "INFO")
        self.log("📊 端口测试结果汇总:\n", "INFO")
        for result in results:
            if "✅" in result:
                self.log(f"  {result}\n", "SUCCESS")
            else:
                self.log(f"  {result}\n", "ERROR")
        self.log("="*60 + "\n\n", "INFO")
        
        # 显示消息框
        result_text = "\n".join(results)
        if all("✅" in r for r in results):
            messagebox.showinfo("端口测试成功", f"所有端口测试通过！\n\n{result_text}")
        else:
            messagebox.showwarning("端口测试完成", f"部分端口测试失败！\n\n{result_text}")
    
    def test_port(self, port, baudrate=115200):
        """测试单个端口连接"""
        if not port:
            self.log("\n⚠️ 请先选择端口！\n", "WARN")
            messagebox.showwarning("警告", "请先选择要测试的端口！")
            return False, "未选择端口"
        
        self.log(f"\n🔍 正在测试端口 {port}...\n", "INFO")
        
        try:
            ser = serial.Serial(port, baudrate, timeout=1)
            ser.close()
            self.log(f"✅ 端口 {port} 连接正常！\n", "SUCCESS")
            messagebox.showinfo("成功", f"端口 {port} 连接正常！")
            return True, "端口连接正常"
        except Exception as e:
            error_msg = f"端口 {port} 连接失败: {str(e)}"
            self.log(f"❌ {error_msg}\n", "ERROR")
            messagebox.showerror("错误", error_msg)
            return False, error_msg
    
    # =========== 烧录方法 ===========
    
    def flash_firmware(self):
        """完整烧录固件（SBL + App）"""
        if self.flashing:
            self.log("⚠️ 烧录正在进行中...\n", "WARN")
            return
        
        # 获取固件文件
        sbl_file = (self.sbl_file.get() or '').strip()
        app_file = (self.app_file.get() or '').strip()
        
        if not sbl_file or not app_file:
            messagebox.showerror("错误", "请先选择SBL和App固件文件！")
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
            self.log("⚠️ 当前没有正在进行的烧录任务\n", "WARN")
            return
        
        self.log("\n🛑 用户请求停止烧录...\n", "WARN")
        self.stop_flashing = True
        
        # 终止当前进程
        if self.flash_process and self.flash_process.poll() is None:
            try:
                self.flash_process.kill()
                self.log("✅ 烧录进程已终止\n", "INFO")
            except Exception as e:
                self.log(f"❌ 终止进程失败: {e}\n", "ERROR")
        
        self.flashing = False
    
    def _flash_firmware_thread(self, sbl_file, app_file, sbl_port, app_port):
        """烧录线程（完整烧录：依次烧录 SBL 与 App）
        
        根据实测验证，采用依次烧录策略更稳定可靠
        """
        try:
            self.log("\n" + "="*60 + "\n")
            self.log("🚀 开始完整烧录流程（SBL + App）\n", "INFO")
            self.log("="*60 + "\n\n")
            
            # SOP模式人工确认
            sop_confirm = messagebox.askyesno(
                "⚠️ 烧录前准备",
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
                self.log("❌ 用户取消烧录（SOP模式未确认）\n", "ERROR")
                return
            
            self.log(f"📁 SBL文件: {sbl_file}\n")
            self.log(f"📁 App文件: {app_file}\n")
            self.log(f"🔌 烧录端口: {sbl_port} (XDS110 Class Application/User UART)\n\n")
            
            # 串口确认
            port_confirm = messagebox.askyesno(
                "串口确认",
                f"请确认烧录端口：\n\n"
                f"烧录端口: {sbl_port}\n"
                f"端口说明: XDS110 Class Application/User UART\n\n"
                f"注意：SBL和App使用同一个烧录端口\n\n"
                f"端口是否正确？"
            )
            if not port_confirm:
                self.log("❌ 用户取消烧录（端口未确认）\n", "ERROR")
                return
            
            # 获取烧录工具路径
            tool_exe = self.flash_tool_path
            
            if not tool_exe or not os.path.exists(tool_exe):
                self.log(f"❌ 找不到烧录工具\n", "ERROR")
                self.log("请点击「选择」按钮选择烧录工具，或确认SDK已正确安装\n", "ERROR")
                return
            
            # 步骤1: 烧录SBL
            self.log("📝 步骤 1/2: 烧录SBL (Bootloader)\n", "INFO")
            
            # 拔插USB确认
            usb_confirm = messagebox.askyesno(
                "准备烧录SBL",
                "请拔插USB或按RESET按钮\n\n"
                "完成后点击\"是\"继续烧录"
            )
            if not usb_confirm:
                self.log("❌ 用户取消烧录（USB未拔插）\n", "ERROR")
                return
            
            self.log("开始烧录SBL...\n\n")
            
            sbl_offset = self.device_config.get('sbl_offset', 0x2000)
            
            # 使用正确的命令格式
            sbl_cmd = [
                tool_exe, 
                "-p", sbl_port, 
                "-f1", sbl_file,      # 使用-f1
                "-of1", str(sbl_offset),  # 使用-of1
                "-s", "SFLASH",       # 存储类型
                "-c"                  # Break信号
            ]
            
            self.log(f"执行命令: {' '.join(sbl_cmd)}\n")
            
            # 检查是否已停止
            if self.stop_flashing:
                self.log("❌ 烧录已停止\n", "ERROR")
                return
            
            process = subprocess.Popen(
                sbl_cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1,
                creationflags=subprocess.CREATE_NO_WINDOW
            )
            self.flash_process = process  # 保存进程引用
            
            # 读取输出
            if process.stdout:
                for line in process.stdout:
                    if self.stop_flashing:
                        process.kill()
                        self.log("\n❌ 烧录已停止\n", "ERROR")
                        return
                    self.log(line)
                    if process.poll() is not None:
                        break
            
            process.wait()
            
            if self.stop_flashing:
                self.log("\n❌ 烧录已停止\n", "ERROR")
                return
            
            if process.returncode != 0:
                self.log("\n❌ SBL烧录失败！\n", "ERROR")
                return
            
            self.log("\n✅ SBL烧录成功！\n", "SUCCESS")
            time.sleep(1)
            
            # 步骤2: 烧录App
            self.log("\n📝 步骤 2/2: 烧录App (应用程序)\n", "INFO")
            
            # App烧录前确认
            app_usb_confirm = messagebox.askyesno(
                "准备烧录App",
                "请再次拔插USB或按RESET按钮\n\n"
                "完成后点击\"是\"继续烧录"
            )
            if not app_usb_confirm:
                self.log("❌ 用户取消App烧录（USB未拔插）\n", "ERROR")
                return
            
            self.log("开始烧录App...\n\n")
            
            app_offset = self.device_config.get('app_offset', 0x42000)
            
            # 使用正确的命令格式
            app_cmd = [
                tool_exe, 
                "-p", app_port, 
                "-f1", app_file,      # 使用-f1
                "-of1", str(app_offset),  # 使用-of1
                "-s", "SFLASH",       # 存储类型
                "-c"                  # Break信号
            ]
            
            self.log(f"执行命令: {' '.join(app_cmd)}\n")
            
            # 检查是否已停止
            if self.stop_flashing:
                self.log("❌ 烧录已停止\n", "ERROR")
                return
            
            process = subprocess.Popen(
                app_cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1,
                creationflags=subprocess.CREATE_NO_WINDOW
            )
            self.flash_process = process  # 更新进程引用
            
            # 读取输出
            if process.stdout:
                for line in process.stdout:
                    if self.stop_flashing:
                        process.kill()
                        self.log("\n❌ 烧录已停止\n", "ERROR")
                        return
                    self.log(line)
                    if process.poll() is not None:
                        break
            
            process.wait()
            
            if self.stop_flashing:
                self.log("\n❌ 烧录已停止\n", "ERROR")
                return
            
            if process.returncode != 0:
                self.log("\n❌ App烧录失败！\n", "ERROR")
                return
            
            self.log("\n✅ App烧录成功！\n", "SUCCESS")
            
            # 完成
            self.log("\n" + "="*60 + "\n")
            self.log("🎉 完整烧录完成！\n", "SUCCESS")
            self.log("="*60 + "\n\n")
            
            messagebox.showinfo("成功", "固件烧录完成！\n\n请复位设备并测试。")
            
        except Exception as e:
            self.log(f"\n❌ 烧录过程出错: {str(e)}\n", "ERROR")
            messagebox.showerror("错误", f"烧录失败：{str(e)}")
        finally:
            self.flashing = False
    
    def flash_sbl_only(self):
        """仅烧录SBL"""
        if self.flashing:
            self.log("⚠️ 烧录正在进行中...\n", "WARN")
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
            self.log("\n" + "="*60 + "\n")
            self.log("🔧 开始SBL烧录\n", "INFO")
            self.log("="*60 + "\n\n")
            
            # SOP模式确认
            sop_confirm = messagebox.askyesno(
                "SOP模式确认",
                "请确认硬件SOP模式配置：\n\n"
                "烧录模式（SOP_MODE1）：\n"
                "• S8 = OFF\n"
                "• S7 = OFF\n\n"
                "当前是否已设置为烧录模式？"
            )
            if not sop_confirm:
                self.log("❌ 用户取消烧录（SOP模式未确认）\n", "ERROR")
                return
            
            self.log(f"📁 固件文件: {firmware_file}\n")
            self.log(f"🔌 SBL端口: {sbl_port}\n\n")
            
            # 串口确认
            port_confirm = messagebox.askyesno(
                "串口确认",
                f"请确认烧录端口：\n\n"
                f"SBL端口: {sbl_port}\n\n"
                f"端口是否正确？"
            )
            if not port_confirm:
                self.log("❌ 用户取消烧录（端口未确认）\n", "ERROR")
                return
            
            # 拔插USB确认
            usb_confirm = messagebox.askyesno(
                "准备烧录",
                "请拔插USB或按RESET按钮\n\n"
                "完成后点击\"是\"继续烧录"
            )
            if not usb_confirm:
                self.log("❌ 用户取消烧录（USB未拔插）\n", "ERROR")
                return
            
            # 获取烧录工具路径
            tool_exe = self.flash_tool_path
            
            if not tool_exe or not os.path.exists(tool_exe):
                self.log(f"❌ 找不到烧录工具\n", "ERROR")
                return
            
            self.log("开始烧录SBL...\n\n")
            
            sbl_offset = self.device_config.get('sbl_offset', 0x2000)
            
            # 使用正确的命令格式（实测验证）
            cmd = [
                tool_exe,
                "-p",
                sbl_port,
                "-f1",            # 使用-f1而非-f
                firmware_file,
                "-of1",           # 使用-of1而非-of
                str(sbl_offset),
                "-s",             # 存储类型
                "SFLASH",
                "-c"              # Break信号
            ]
            
            self.log(f"执行命令: {' '.join(cmd)}\n")
            
            process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1,
                creationflags=subprocess.CREATE_NO_WINDOW
            )
            
            # 实时读取并显示输出
            if process.stdout:
                for line in process.stdout:
                    self.log(line)
                    if process.poll() is not None:
                        break
            
            process.wait()
            
            if process.returncode != 0:
                self.log("\n❌ SBL烧录失败！\n", "ERROR")
                return
            
            self.log("\n✅ SBL烧录成功！\n", "SUCCESS")
            messagebox.showinfo("成功", "SBL烧录完成！")
            
        except Exception as e:
            self.log(f"\n❌ 烧录出错: {str(e)}\n", "ERROR")
            messagebox.showerror("错误", f"烧录失败：{str(e)}")
        finally:
            self.flashing = False
    
    def flash_app_only(self):
        """仅烧录App"""
        if self.flashing:
            self.log("⚠️ 烧录正在进行中...\n", "WARN")
            return
        
        # 获取App固件文件
        firmware_file = self.app_file.get()
        if not firmware_file or not os.path.exists(firmware_file):
            messagebox.showerror("错误", "请先选择有效的App固件文件！")
            return
        
        # 获取端口
        app_port = self.app_port.get()
        if not app_port:
            messagebox.showerror("错误", "请先选择App端口！")
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
        """烧录线程（仅App）"""
        try:
            self.log("\n" + "="*60 + "\n")
            self.log("📱 开始App烧录\n", "INFO")
            self.log("="*60 + "\n\n")
            
            # SOP模式确认
            sop_confirm = messagebox.askyesno(
                "SOP模式确认",
                "请确认硬件SOP模式配置：\n\n"
                "烧录模式（SOP_MODE1）：\n"
                "• S8 = OFF\n"
                "• S7 = OFF\n\n"
                "运行模式（SOP_MODE2）：\n"
                "• S8 = OFF\n"
                "• S7 = ON\n\n"
                "App烧录建议使用SOP_MODE1\n"
                "当前是否已设置为烧录模式？"
            )
            if not sop_confirm:
                self.log("❌ 用户取消烧录（SOP模式未确认）\n", "ERROR")
                return
            
            self.log(f"📁 固件文件: {firmware_file}\n")
            self.log(f"🔌 App端口: {app_port}\n\n")
            
            # 串口确认
            port_confirm = messagebox.askyesno(
                "串口确认",
                f"请确认烧录端口：\n\n"
                f"App端口: {app_port}\n\n"
                f"端口是否正确？"
            )
            if not port_confirm:
                self.log("❌ 用户取消烧录（端口未确认）\n", "ERROR")
                return
            
            # 拔插USB确认
            usb_confirm = messagebox.askyesno(
                "准备烧录",
                "请拔插USB或按RESET按钮\n\n"
                "完成后点击\"是\"继续烧录"
            )
            if not usb_confirm:
                self.log("❌ 用户取消烧录（USB未拔插）\n", "ERROR")
                return
            
            # 获取烧录工具路径
            tool_exe = self.flash_tool_path
            
            if not tool_exe or not os.path.exists(tool_exe):
                self.log(f"❌ 找不到烧录工具\n", "ERROR")
                return
            
            self.log("开始烧录App...\n\n")
            
            app_offset = self.device_config.get('app_offset', 0x42000)
            
            # 使用正确的命令格式（实测验证）
            cmd = [
                tool_exe,
                "-p",
                app_port,
                "-f1",            # 使用-f1而非-f
                firmware_file,
                "-of1",           # 使用-of1而非-of
                str(app_offset),
                "-s",             # 存储类型
                "SFLASH",
                "-c"              # Break信号
            ]
            
            self.log(f"执行命令: {' '.join(cmd)}\n")
            
            process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1,
                creationflags=subprocess.CREATE_NO_WINDOW
            )
            
            # 实时读取并显示输出
            if process.stdout:
                for line in process.stdout:
                    self.log(line)
                    if process.poll() is not None:
                        break
            
            process.wait()
            
            if process.returncode != 0:
                self.log("\n❌ App烧录失败！\n", "ERROR")
                return
            
            self.log("\n✅ App烧录成功！\n", "SUCCESS")
            messagebox.showinfo("成功", "App烧录完成！")
            
        except Exception as e:
            self.log(f"\n❌ 烧录出错: {str(e)}\n", "ERROR")
            messagebox.showerror("错误", f"烧录失败：{str(e)}")
        finally:
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
            self.log(f"✅ 已选择SBL文件: {filename}\n", "SUCCESS")
            
            # 更新界面状态
            if hasattr(self, 'sbl_status_label'):
                self.sbl_status_label.config(text="✅ 已选择", fg="green")
            if hasattr(self, 'sbl_path_label'):
                self.sbl_path_label.config(text=filename)
            
            # 验证文件
            valid, msg = verify_firmware_file(filename)
            if valid:
                self.log(f"✅ {msg}\n", "SUCCESS")
            else:
                self.log(f"⚠️ {msg}\n", "WARN")
    
    def select_app_file(self):
        """选择App固件文件"""
        filename = filedialog.askopenfilename(
            title="选择App固件文件",
            filetypes=[
                ("AppImage Files", "*.appimage"),
                ("All Files", "*.*")
            ],
            initialdir=os.path.dirname(self.app_file.get()) if self.app_file.get() else None
        )
        if filename:
            self.app_file.set(filename)
            self.log(f"✅ 已选择App文件: {filename}\n", "SUCCESS")
            
            # 更新界面状态
            if hasattr(self, 'app_status_label'):
                self.app_status_label.config(text="✅ 已选择", fg="green")
            if hasattr(self, 'app_path_label'):
                self.app_path_label.config(text=filename)
            
            # 验证文件
            valid, msg = verify_firmware_file(filename)
            if valid:
                self.log(f"✅ {msg}\n", "SUCCESS")
            else:
                self.log(f"⚠️ {msg}\n", "WARN")
    
    def analyze_firmware(self):
        """分析已选择的固件文件"""
        sbl_file = (self.sbl_file.get() or '').strip()
        app_file = (self.app_file.get() or '').strip()
        
        # 调试信息
        self.log(f"\n🔍 开始分析固件...\n", "INFO")
        self.log(f"SBL文件变量值: '{sbl_file}'\n")
        self.log(f"App文件变量值: '{app_file}'\n")
        
        if not sbl_file and not app_file:
            self.log("\n⚠️ 请先选择SBL或App固件文件！\n", "WARN")
            self.log("提示: 点击左侧的「选择」按钮来选择固件文件\n")
            return
        
        # 分析SBL固件
        if sbl_file:
            if not os.path.exists(sbl_file):
                self.log(f"\n❌ SBL固件文件不存在: {sbl_file}\n", "ERROR")
            else:
                self.log(f"\n🔍 分析SBL固件: {os.path.basename(sbl_file)}\n", "INFO")
                self.log(f"完整路径: {sbl_file}\n\n")
                
                info = analyze_appimage_structure(sbl_file)
                if info:
                    self.log("=" * 50 + "\n")
                    self.log(f"📊 SBL固件结构分析结果\n", "SUCCESS")
                    self.log("=" * 50 + "\n")
                    self.log(f"文件大小: {info['total_size']:,} 字节 ({info['total_size']/1024:.2f} KB)\n")
                    self.log(f"Magic Number: {info.get('magic_number', 'N/A')}\n")
                    self.log(f"文件类型: {info.get('file_type', 'Unknown')}\n")
                    self.log(f"\n📍 Flash烧录信息:\n")
                    self.log(f"  - Flash偏移: 0x{info['sbl_offset']:X} ({info['sbl_offset']} 字节)\n")
                    self.log(f"  - 固件大小: {info['sbl_size']:,} 字节 ({info['sbl_size']/1024:.2f} KB)\n")
                    self.log(f"  - 预留空间: 256 KB (0x40000)\n")
                    self.log("=" * 50 + "\n")
                else:
                    self.log("❌ SBL分析失败：无法解析固件文件结构\n", "ERROR")
        
        # 分析App固件
        if app_file and app_file != sbl_file:  # 避免重复分析
            if not os.path.exists(app_file):
                self.log(f"\n❌ App固件文件不存在: {app_file}\n", "ERROR")
            else:
                self.log(f"\n🔍 分析App固件: {os.path.basename(app_file)}\n", "INFO")
                self.log(f"完整路径: {app_file}\n\n")
                
                info = analyze_appimage_structure(app_file)
                if info:
                    self.log("=" * 50 + "\n")
                    self.log(f"📊 App固件结构分析结果\n", "SUCCESS")
                    self.log("=" * 50 + "\n")
                    self.log(f"文件大小: {info['total_size']:,} 字节 ({info['total_size']/1024:.2f} KB)\n")
                    self.log(f"Magic Number: {info.get('magic_number', 'N/A')}\n")
                    self.log(f"文件类型: {info.get('file_type', 'Unknown')}\n")
                    self.log(f"\n📍 Flash烧录信息:\n")
                    self.log(f"  - Flash偏移: 0x{info['app_offset']:X} ({info['app_offset']} 字节)\n")
                    self.log(f"  - 固件大小: {info['app_size']:,} 字节 ({info['app_size']/1024:.2f} KB)\n")
                    self.log(f"  - 最大空间: 1784 KB\n")
                    self.log("=" * 50 + "\n")
                else:
                    self.log("❌ App分析失败：无法解析固件文件结构\n", "ERROR")
    
    def refresh_com_ports(self):
        """刷新COM端口列表"""
        self.log("\n🔄 正在刷新端口列表...\n", "INFO")
        
        # 获取所有端口
        all_ports = list(serial.tools.list_ports.comports())
        
        self.log(f"\n🔍 扫描到 {len(all_ports)} 个端口:\n")
        for port in all_ports:
            self.log(f"  - {port.device}: {port.description}\n")
            if port.hwid:
                self.log(f"    HWID: {port.hwid}\n")
        
        sbl_ports, app_ports = self.refresh_ports()
        
        if sbl_ports or app_ports:
            self.log(f"\n✅ 刷新成功！\n", "SUCCESS")
            if sbl_ports:
                self.log(f"  🔌 找到烧录端口: {', '.join(sbl_ports)}\n", "SUCCESS")
                for port in sbl_ports:
                    port_info = self.get_port_info(port)
                    if port_info:
                        self.log(f"     - 描述: {port_info['description']}\n")
                        if port_info.get('vid') and port_info.get('pid'):
                            self.log(f"     - VID:PID = {port_info['vid']:04X}:{port_info['pid']:04X}\n")
            else:
                self.log(f"  ⚠️ 未找到烧录端口 (XDS110 Auxiliary Data Port)\n", "WARN")
            
            if app_ports:
                self.log(f"  🔌 找到调试端口: {', '.join(app_ports)}\n", "SUCCESS")
                for port in app_ports:
                    port_info = self.get_port_info(port)
                    if port_info:
                        self.log(f"     - 描述: {port_info['description']}\n")
                        if port_info.get('vid') and port_info.get('pid'):
                            self.log(f"     - VID:PID = {port_info['vid']:04X}:{port_info['pid']:04X}\n")
            else:
                self.log(f"  ⚠️ 未找到调试端口 (XDS110 Application/User UART)\n", "WARN")
        else:
            self.log("❌ 未找到任何XDS110端口！\n", "ERROR")
            self.log("请检查：\n", "WARN")
            self.log("  1. 设备是否正确连接\n")
            self.log("  2. USB驱动是否安装\n")
            self.log("  3. 设备管理器中是否显示XDS110端口\n")
    
    # =========== 日志方法 ===========
    
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
    """检查是否有老进程在运行（v1.0.1需求1）"""
    current_pid = os.getpid()
    script_name = os.path.basename(__file__)
    
    old_processes = []
    for proc in psutil.process_iter(['pid', 'name', 'cmdline']):
        try:
            if proc.pid == current_pid:
                continue
            cmdline = proc.info.get('cmdline', [])
            if cmdline and script_name in ' '.join(cmdline):
                old_processes.append(proc)
        except (psutil.NoSuchProcess, psutil.AccessDenied):
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
    # v1.0.1需求1: 检查老进程
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
            time.sleep(0.5)  # 等待旧进程完全关闭
        else:
            sys.exit(0)
    
    # 启动GUI
    root = tk.Tk()
    app = FlashToolGUI(root)
    root.mainloop()

if __name__ == "__main__":
    main()
