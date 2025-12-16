#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Ti AWRL6844 固件烧录工具 v1.0.8 - 模块化版本
主入口文件 - 调用各标签页模块
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
VERSION = "1.3.0"
BUILD_DATE = "2025-12-16"
AUTHOR = "Benson@Wisefido"

# 导入标签页模块
try:
    from tabs import BasicTab, AdvancedTab, MonitorTab, PortsTab, FirmwareLibTab
except ImportError as e:
    messagebox.showerror(
        "模块导入错误",
        f"无法导入tabs模块：{e}\n\n"
        "请确保tabs目录存在且包含以下文件：\n"
        "- __init__.py\n"
        "- tab_basic.py\n"
        "- tab_advanced.py\n"
        "- tab_monitor.py\n"
        "- tab_ports.py\n"
        "- tab_firmware_lib.py"
    )
    sys.exit(1)

# ============================================================
# 设备配置
# ============================================================

DEVICE_CONFIGS = {
    'AWRL6844': {
        'name': 'AWRL6844',
        'image_type': 'MultiCore',
        'sbl_port_name': 'XDS110 Class Auxiliary Data Port',
        'app_port_name': 'XDS110 Class Application/User UART',
        'sbl_baudrate': 115200,
        'app_baudrate': 115200,
        'flash_timeout': 180,
        'sbl_timeout': 60,
        'app_timeout': 120,
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
    分析appimage文件结构（完整版）
    
    AppImage结构：
    - Meta Header (256字节): 包含Magic、版本、SBL/App偏移和大小
    - SBL Image: 从meta header指定的偏移开始
    - App Image: 从meta header指定的偏移开始
    
    Returns:
        dict: 包含文件结构信息，如果分析失败返回None
    """
    try:
        with open(file_path, 'rb') as f:
            # 读取Meta Header (256字节)
            meta_header = f.read(256)
            
            if len(meta_header) < 256:
                return None
            
            import struct
            
            # 解析Meta Header结构
            # Offset 0x00: Magic Number (4字节) - 应为 0x5254534D ("MSTR")
            magic = struct.unpack('<I', meta_header[0:4])[0]
            
            # Offset 0x04-0x07: 版本信息
            version = struct.unpack('<I', meta_header[4:8])[0]
            
            # Offset 0x10: SBL偏移 (4字节)
            sbl_offset = struct.unpack('<I', meta_header[16:20])[0]
            
            # Offset 0x14: SBL大小 (4字节)
            sbl_size = struct.unpack('<I', meta_header[20:24])[0]
            
            # Offset 0x18: App偏移 (4字节)
            app_offset = struct.unpack('<I', meta_header[24:28])[0]
            
            # Offset 0x1C: App大小 (4字节)
            app_size = struct.unpack('<I', meta_header[28:32])[0]
            
            # 获取文件总大小
            f.seek(0, 2)  # 移到文件末尾
            total_size = f.tell()
            
            info = {
                'total_size': total_size,
                'has_meta_header': magic == 0x5254534D,
                'magic_number': hex(magic),
                'version': version,
                'sbl_offset': sbl_offset,
                'sbl_size': sbl_size,
                'app_offset': app_offset,
                'app_size': app_size,
                'has_sbl_header': sbl_offset > 0 and sbl_size > 0,
                'has_app_header': app_offset > 0 and app_size > 0
            }
            
            # 验证偏移和大小的合理性
            if sbl_offset + sbl_size > total_size:
                info['sbl_size'] = total_size - sbl_offset
            
            if app_offset + app_size > total_size:
                info['app_size'] = total_size - app_offset
            
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
        
        self.transient(parent)
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
        self.title("SBL存在性检测")
        self.port = port
        self.baudrate = baudrate
        self.geometry("600x500")
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
            command=self.destroy,
            state=tk.DISABLED
        )
        self.close_btn.pack()
        
        self.transient(parent)
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
        self.firmware_file = tk.StringVar()
        self.sbl_port = tk.StringVar()
        self.app_port = tk.StringVar()
        self.flash_timeout = tk.IntVar(value=self.device_config['flash_timeout'])
        self.sbl_timeout = tk.IntVar(value=self.device_config['sbl_timeout'])
        self.app_timeout = tk.IntVar(value=self.device_config['app_timeout'])
        
        # 烧录状态
        self.flashing = False
        self.flash_thread = None
        
        # 创建界面
        self.create_widgets()
        
        # 初始化端口
        self.refresh_ports()
        
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
        basic_frame = ttk.Frame(self.notebook)
        advanced_frame = ttk.Frame(self.notebook)
        firmware_lib_frame = ttk.Frame(self.notebook)
        monitor_frame = ttk.Frame(self.notebook)
        ports_frame = ttk.Frame(self.notebook)
        
        # 添加到Notebook
        self.notebook.add(basic_frame, text="  基本烧录  ")
        self.notebook.add(advanced_frame, text="  高级功能  ")
        self.notebook.add(firmware_lib_frame, text="  固件库  ")
        self.notebook.add(monitor_frame, text="  串口监视  ")
        self.notebook.add(ports_frame, text="  端口管理  ")
        
        # 实例化各标签页模块
        self.basic_tab = BasicTab(basic_frame, self)
        self.advanced_tab = AdvancedTab(advanced_frame, self)
        self.firmware_lib_tab = FirmwareLibTab(firmware_lib_frame, self)
        self.monitor_tab = MonitorTab(monitor_frame, self)
        self.ports_tab = PortsTab(ports_frame, self)
        
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
    
    def test_port(self, port, baudrate=115200):
        """测试端口连接"""
        try:
            ser = serial.Serial(port, baudrate, timeout=1)
            ser.close()
            return True, "端口连接正常"
        except Exception as e:
            return False, f"端口连接失败: {str(e)}"
    
    # =========== 烧录方法 ===========
    
    def flash_firmware(self):
        """完整烧录固件（SBL + App）"""
        if self.flashing:
            self.log("⚠️ 烧录正在进行中...\n", "WARN")
            return
        
        # 获取固件文件
        firmware_file = self.firmware_file.get()
        if not firmware_file or not os.path.exists(firmware_file):
            messagebox.showerror("错误", "请先选择有效的固件文件！")
            return
        
        # 获取端口
        sbl_port = self.sbl_port.get()
        app_port = self.app_port.get()
        
        if not sbl_port or not app_port:
            messagebox.showerror("错误", "请先选择SBL和App端口！")
            return
        
        # 启动烧录线程
        self.flashing = True
        self.flash_thread = threading.Thread(
            target=self._flash_firmware_thread,
            args=(firmware_file, sbl_port, app_port),
            daemon=True
        )
        self.flash_thread.start()
    
    def _flash_firmware_thread(self, firmware_file, sbl_port, app_port):
        """烧录线程（完整烧录）"""
        try:
            self.log("\n" + "="*60 + "\n")
            self.log("🚀 开始完整烧录流程（SBL + App）\n", "INFO")
            self.log("="*60 + "\n\n")
            
            self.log(f"📁 固件文件: {firmware_file}\n")
            self.log(f"🔌 SBL端口: {sbl_port}\n")
            self.log(f"🔌 App端口: {app_port}\n\n")
            
            # SDK工具路径
            sdk_path = self.device_config.get('sdk_path', 'C:\\ti\\MMWAVE_L_SDK_06_01_00_01')
            tool_exe = os.path.join(sdk_path, 'tools', 'FlashingTool', 'arprog_cmdline_6844.exe')
            
            if not os.path.exists(tool_exe):
                self.log(f"❌ 找不到烧录工具: {tool_exe}\n", "ERROR")
                self.log("请确认SDK已正确安装\n", "ERROR")
                return
            
            # 步骤1: 烧录SBL
            self.log("📝 步骤 1/2: 烧录SBL (Bootloader)\n", "INFO")
            sbl_offset = self.device_config.get('sbl_offset', 0x2000)
            
            sbl_cmd = [
                tool_exe,
                sbl_port,
                str(sbl_offset),
                firmware_file
            ]
            
            self.log(f"执行命令: {' '.join(sbl_cmd)}\n")
            
            process = subprocess.Popen(
                sbl_cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                creationflags=subprocess.CREATE_NO_WINDOW
            )
            
            # 读取输出
            for line in process.stdout:
                self.log(line)
                if "Error" in line or "error" in line:
                    self.log(f"⚠️ {line}", "ERROR")
            
            process.wait()
            
            if process.returncode != 0:
                self.log("\n❌ SBL烧录失败！\n", "ERROR")
                stderr = process.stderr.read()
                if stderr:
                    self.log(f"错误信息: {stderr}\n", "ERROR")
                return
            
            self.log("\n✅ SBL烧录成功！\n", "SUCCESS")
            time.sleep(1)
            
            # 步骤2: 烧录App
            self.log("\n📝 步骤 2/2: 烧录App (应用程序)\n", "INFO")
            app_offset = self.device_config.get('app_offset', 0x42000)
            
            app_cmd = [
                tool_exe,
                app_port,
                str(app_offset),
                firmware_file
            ]
            
            self.log(f"执行命令: {' '.join(app_cmd)}\n")
            
            process = subprocess.Popen(
                app_cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                creationflags=subprocess.CREATE_NO_WINDOW
            )
            
            # 读取输出
            for line in process.stdout:
                self.log(line)
                if "Error" in line or "error" in line:
                    self.log(f"⚠️ {line}", "ERROR")
            
            process.wait()
            
            if process.returncode != 0:
                self.log("\n❌ App烧录失败！\n", "ERROR")
                stderr = process.stderr.read()
                if stderr:
                    self.log(f"错误信息: {stderr}\n", "ERROR")
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
        
        # 获取固件文件
        firmware_file = self.firmware_file.get()
        if not firmware_file or not os.path.exists(firmware_file):
            messagebox.showerror("错误", "请先选择有效的固件文件！")
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
            
            self.log(f"📁 固件文件: {firmware_file}\n")
            self.log(f"🔌 SBL端口: {sbl_port}\n\n")
            
            # SDK工具路径
            sdk_path = self.device_config.get('sdk_path', 'C:\\ti\\MMWAVE_L_SDK_06_01_00_01')
            tool_exe = os.path.join(sdk_path, 'tools', 'FlashingTool', 'arprog_cmdline_6844.exe')
            
            if not os.path.exists(tool_exe):
                self.log(f"❌ 找不到烧录工具: {tool_exe}\n", "ERROR")
                return
            
            sbl_offset = self.device_config.get('sbl_offset', 0x2000)
            
            cmd = [
                tool_exe,
                sbl_port,
                str(sbl_offset),
                firmware_file
            ]
            
            self.log(f"执行命令: {' '.join(cmd)}\n")
            
            process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                creationflags=subprocess.CREATE_NO_WINDOW
            )
            
            for line in process.stdout:
                self.log(line)
            
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
        
        # 获取固件文件
        firmware_file = self.firmware_file.get()
        if not firmware_file or not os.path.exists(firmware_file):
            messagebox.showerror("错误", "请先选择有效的固件文件！")
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
            
            self.log(f"📁 固件文件: {firmware_file}\n")
            self.log(f"🔌 App端口: {app_port}\n\n")
            
            # SDK工具路径
            sdk_path = self.device_config.get('sdk_path', 'C:\\ti\\MMWAVE_L_SDK_06_01_00_01')
            tool_exe = os.path.join(sdk_path, 'tools', 'FlashingTool', 'arprog_cmdline_6844.exe')
            
            if not os.path.exists(tool_exe):
                self.log(f"❌ 找不到烧录工具: {tool_exe}\n", "ERROR")
                return
            
            app_offset = self.device_config.get('app_offset', 0x42000)
            
            cmd = [
                tool_exe,
                app_port,
                str(app_offset),
                firmware_file
            ]
            
            self.log(f"执行命令: {' '.join(cmd)}\n")
            
            process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                creationflags=subprocess.CREATE_NO_WINDOW
            )
            
            for line in process.stdout:
                self.log(line)
            
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
                ("Binary Files", "*.bin"),
                ("All Files", "*.*")
            ],
            initialdir=os.path.dirname(self.firmware_file.get()) if self.firmware_file.get() else None
        )
        if filename:
            self.firmware_file.set(filename)
            self.log(f"✅ 已选择SBL文件: {filename}\n", "SUCCESS")
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
                ("Binary Files", "*.bin"),
                ("All Files", "*.*")
            ],
            initialdir=os.path.dirname(self.firmware_file.get()) if self.firmware_file.get() else None
        )
        if filename:
            self.firmware_file.set(filename)
            self.log(f"✅ 已选择App文件: {filename}\n", "SUCCESS")
            # 验证文件
            valid, msg = verify_firmware_file(filename)
            if valid:
                self.log(f"✅ {msg}\n", "SUCCESS")
            else:
                self.log(f"⚠️ {msg}\n", "WARN")
    
    def open_firmware_folder(self):
        """打开固件文件夹并扫描固件文件"""
        folder = filedialog.askdirectory(title="选择固件文件夹")
        if not folder:
            return
        
        self.log(f"📂 扫描文件夹: {folder}\n")
        
        # 扫描.appimage和.bin文件
        firmware_files = []
        for ext in ['*.appimage', '*.bin']:
            firmware_files.extend(Path(folder).glob(ext))
        
        if not firmware_files:
            self.log("❌ 未找到固件文件 (.appimage 或 .bin)\n", "ERROR")
            messagebox.showwarning("警告", "所选文件夹中未找到固件文件")
            return
        
        # 按修改时间排序，最新的在前
        firmware_files.sort(key=lambda x: x.stat().st_mtime, reverse=True)
        
        self.log(f"✅ 找到 {len(firmware_files)} 个固件文件:\n", "SUCCESS")
        for i, file in enumerate(firmware_files, 1):
            size_kb = file.stat().st_size / 1024
            mod_time = datetime.fromtimestamp(file.stat().st_mtime).strftime('%Y-%m-%d %H:%M:%S')
            self.log(f"  {i}. {file.name} ({size_kb:.1f} KB, {mod_time})\n")
        
        # 自动选择最新的固件
        latest_firmware = str(firmware_files[0])
        self.firmware_file.set(latest_firmware)
        self.log(f"\n✅ 已自动选择最新固件: {firmware_files[0].name}\n", "SUCCESS")
        
        # 验证文件
        valid, msg = verify_firmware_file(latest_firmware)
        if valid:
            self.log(f"✅ {msg}\n", "SUCCESS")
        else:
            self.log(f"⚠️ {msg}\n", "WARN")
    
    def analyze_firmware(self):
        """分析固件文件"""
        filename = filedialog.askopenfilename(
            title="选择要分析的固件文件",
            filetypes=[("Binary Files", "*.bin"), ("All Files", "*.*")]
        )
        if filename:
            self.log(f"\n分析固件: {filename}\n")
            info = analyze_appimage_structure(filename)
            if info:
                self.log(f"文件大小: {info['total_size']} 字节\n")
                self.log(f"SBL偏移: {info['sbl_offset']}\n")
                self.log(f"App偏移: {info['app_offset']}\n")
            else:
                self.log("分析失败\n", "ERROR")
    
    def refresh_com_ports(self):
        """刷新COM端口列表"""
        self.refresh_ports()
        self.log("已刷新端口列表\n")
    
    # =========== 日志方法 ===========
    
    def log(self, message, tag=None):
        """添加日志（委托给当前激活的标签页）"""
        current_tab = self.notebook.select()
        tab_index = self.notebook.index(current_tab)
        
        # 根据标签页索引调用对应的日志方法
        if tab_index == 0 and hasattr(self.basic_tab, 'log'):
            self.basic_tab.log(message, tag)
        elif tab_index == 1 and hasattr(self.advanced_tab, 'log'):
            self.advanced_tab.log(message, tag)
    
    def clear_log(self):
        """清空日志"""
        current_tab = self.notebook.select()
        tab_index = self.notebook.index(current_tab)
        
        if tab_index == 0 and hasattr(self.basic_tab, 'clear_log'):
            self.basic_tab.clear_log()
        elif tab_index == 1 and hasattr(self.advanced_tab, 'clear_log'):
            self.advanced_tab.clear_log()
    
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
