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
VERSION = "1.0.8"
BUILD_DATE = "2025-11-30"

# 导入标签页模块
try:
    from tabs import BasicTab, AdvancedTab, MonitorTab, PortsTab
except ImportError as e:
    messagebox.showerror(
        "模块导入错误",
        f"无法导入tabs模块：{e}\n\n"
        "请确保tabs目录存在且包含以下文件：\n"
        "- __init__.py\n"
        "- tab_basic.py\n"
        "- tab_advanced.py\n"
        "- tab_monitor.py\n"
        "- tab_ports.py"
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
        'app_timeout': 120
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

def analyze_appimage_structure(file_path):
    """分析appimage文件结构"""
    try:
        with open(file_path, 'rb') as f:
            data = f.read()
        
        info = {
            'total_size': len(data),
            'has_meta_header': False,
            'has_sbl_header': False,
            'has_app_header': False,
            'sbl_offset': 0,
            'app_offset': 0
        }
        
        # 简化分析
        if len(data) > 256:
            # 查找可能的SBL和App起始位置
            sbl_pattern = b'SBL'
            app_pattern = b'APP'
            
            sbl_pos = data.find(sbl_pattern)
            app_pos = data.find(app_pattern)
            
            if sbl_pos > 0:
                info['has_sbl_header'] = True
                info['sbl_offset'] = sbl_pos
            
            if app_pos > 0:
                info['has_app_header'] = True
                info['app_offset'] = app_pos
        
        return info
        
    except Exception as e:
        return None

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
                    except:
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
            except:
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
            text=f"构建日期: {BUILD_DATE}",
            font=('Arial', 9),
            foreground='gray'
        ).pack(side=tk.RIGHT)
        
        # 创建Notebook（标签页容器）
        self.notebook = ttk.Notebook(self.root)
        self.notebook.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)
        
        # 创建各个标签页的Frame
        basic_frame = ttk.Frame(self.notebook)
        advanced_frame = ttk.Frame(self.notebook)
        monitor_frame = ttk.Frame(self.notebook)
        ports_frame = ttk.Frame(self.notebook)
        
        # 添加到Notebook
        self.notebook.add(basic_frame, text="  基本烧录  ")
        self.notebook.add(advanced_frame, text="  高级功能  ")
        self.notebook.add(monitor_frame, text="  串口监视  ")
        self.notebook.add(ports_frame, text="  端口管理  ")
        
        # 实例化各标签页模块
        self.basic_tab = BasicTab(basic_frame, self)
        self.advanced_tab = AdvancedTab(advanced_frame, self)
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
        """打开固件文件夹"""
        folder = filedialog.askdirectory(title="选择固件文件夹")
        if folder:
            self.log(f"打开文件夹: {folder}\n")
            # TODO: 扫描文件夹中的固件文件
    
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

def main():
    """主函数"""
    root = tk.Tk()
    app = FlashToolGUI(root)
    root.mainloop()

if __name__ == "__main__":
    main()
