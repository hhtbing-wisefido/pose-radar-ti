#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
AWRL6844 雷达完整测试工具（修复版）
使用双端口异步监听模式 - 与test_serial_ports.py相同的成功方法
"""

import serial
import serial.tools.list_ports
import time
import threading
from datetime import datetime

# TI官方配置命令
TI_CONFIG_COMMANDS = [
    'sensorStop 0',
    'channelCfg 153 255 0',
    'chirpComnCfg 8 0 0 256 1 13.1 3',
    'chirpTimingCfg 6 63 0 160 58',
    'adcDataDitherCfg 1',
    'frameCfg 64 0 1358 1 100 0',
    'gpAdcMeasConfig 0 0',
    'guiMonitor 1 1 0 0 0 1',
    'cfarProcCfg 0 2 8 4 3 0 9.0 0',
    'cfarProcCfg 1 2 4 2 2 1 9.0 0',
    'cfarFovCfg 0 0.25 9.0',
    'cfarFovCfg 1 -20.16 20.16',
    'aoaProcCfg 64 64',
    'aoaFovCfg -60 60 -60 60',
    'clutterRemoval 0',
    'factoryCalibCfg 1 0 44 2 0x1ff000',
    'runtimeCalibCfg 1',
    'antGeometryBoard xWRL6844EVM',
    'adcDataSource 0 adc_test_data_0001.bin',
    'adcLogging 0',
    'lowPowerCfg 1',
    'sensorStart 0 0 0 0'
]

class PortMonitor:
    """端口监听器 - 独立线程异步读取"""
    def __init__(self, port, baudrate, name):
        self.port = port
        self.baudrate = baudrate
        self.name = name
        self.serial_conn = None
        self.is_running = False
        self.thread = None
        self.cli_responses = []
        self.data_bytes = 0
        self.last_data_time = None

    def connect(self):
        """连接串口"""
        try:
            self.serial_conn = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=0.1
            )
            time.sleep(0.5)
            print(f"OK: {self.name} 已连接: {self.port}@{self.baudrate}")
            return True
        except Exception as e:
            print(f"FAIL: {self.name} 连接失败: {e}")
            return False

    def start_monitor(self):
        """启动异步监听线程"""
        if not self.serial_conn or not self.serial_conn.is_open:
            return False

        self.is_running = True
        self.thread = threading.Thread(target=self._read_loop, daemon=True)
        self.thread.start()
        return True

    def _read_loop(self):
        """读取循环（在独立线程中运行）"""
        while self.is_running:
            try:
                if self.serial_conn.in_waiting > 0:
                    data = self.serial_conn.read(self.serial_conn.in_waiting)
                    self.data_bytes += len(data)
                    self.last_data_time = time.time()

                    # 尝试解码为文本（CLI响应）
                    try:
                        text = data.decode('utf-8', errors='ignore')
                        if text.strip():
                            self.cli_responses.append(text)
                            # 显示前80个字符
                            display = text.strip().replace('\n', ' ')[:80]
                            print(f"[{self.name}] {display}")
                    except:
                        # 二进制数据
                        print(f"[{self.name}] 数据: {len(data)} bytes")

                time.sleep(0.01)  # 10ms轮询间隔
            except Exception as e:
                print(f"[{self.name}] 读取错误: {e}")
                break

    def send_command(self, command):
        """发送命令"""
        if not self.serial_conn or not self.serial_conn.is_open:
            return False

        try:
            self.serial_conn.write(f"{command}\n".encode())
            return True
        except Exception as e:
            print(f"[{self.name}] 发送失败: {e}")
            return False

    def stop(self):
        """停止监听"""
        self.is_running = False
        if self.thread:
            self.thread.join(timeout=1)
        if self.serial_conn and self.serial_conn.is_open:
            self.serial_conn.close()

    def get_stats(self):
        """获取统计信息"""
        return {
            'name': self.name,
            'port': self.port,
            'baudrate': self.baudrate,
            'cli_responses': len(self.cli_responses),
            'data_bytes': self.data_bytes,
            'has_data': self.data_bytes > 0
        }


def main():
    print("\n" + "="*70)
    print("AWRL6844 雷达完整测试工具（修复版）")
    print("="*70)
    print("使用双端口异步监听模式")
    print("CLI端口: COM3@115200 (发送命令)")
    print("数据端口: COM4@1250000 (接收数据)")
    print("="*70)

    # 检查串口
    ports = serial.tools.list_ports.comports()
    print(f"\n发现 {len(ports)} 个串口:")
    for p in ports:
        print(f"  {p.device}: {p.description}")

    # 创建监听器
    cli_port = PortMonitor('COM3', 115200, 'CLI端口')
    data_port = PortMonitor('COM4', 1250000, '数据端口')

    # 连接串口
    print("\n" + "-"*70)
    print("INFO: 连接串口...")
    print("-"*70)

    if not cli_port.connect():
        print("\nFAIL: CLI端口连接失败！")
        return

    if not data_port.connect():
        print("\nFAIL: 数据端口连接失败！")
        cli_port.stop()
        return

    # 启动异步监听
    print("\n" + "-"*70)
    print("INFO: 启动异步监听线程...")
    print("-"*70)

    cli_port.start_monitor()
    data_port.start_monitor()
    time.sleep(1)

    # 发送配置命令
    print("\n" + "-"*70)
    print("INFO: 发送TI官方配置命令...")
    print("-"*70)
    print(f"命令数量: {len(TI_CONFIG_COMMANDS)}\n")

    for i, cmd in enumerate(TI_CONFIG_COMMANDS, 1):
        print(f"{i:2d}. {cmd}")
        cli_port.send_command(cmd)
        time.sleep(0.05)  # 50ms间隔（与test_serial_ports.py相同）

    # 等待数据收集
    print("\n" + "-"*70)
    print("INFO: 等待10秒收集数据...")
    print("-"*70)

    for i in range(10):
        time.sleep(1)
        cli_stats = cli_port.get_stats()
        data_stats = data_port.get_stats()
        print(f"  {i+1:2d}/10秒 - CLI响应:{cli_stats['cli_responses']} 数据:{data_stats['data_bytes']}bytes")

    # 停止监听
    print("\n" + "-"*70)
    print("INFO: 停止监听...")
    print("-"*70)

    cli_port.stop()
    data_port.stop()

    # 显示测试结果
    print("\n" + "="*70)
    print("测试结果")
    print("="*70)

    cli_stats = cli_port.get_stats()
    data_stats = data_port.get_stats()

    print(f"\nCLI端口 (COM3@115200):")
    print(f"  CLI响应数: {cli_stats['cli_responses']}")
    print(f"  数据量: {cli_stats['data_bytes']} bytes")

    print(f"\n数据端口 (COM4@1250000):")
    print(f"  CLI响应数: {data_stats['cli_responses']}")
    print(f"  数据量: {data_stats['data_bytes']} bytes")

    # 判断成功
    print("\n" + "="*70)
    if data_stats['data_bytes'] > 1000:
        print("OK: 测试成功！雷达工作正常！")
        print(f"   接收到 {data_stats['data_bytes']} bytes 数据")
        if data_stats['data_bytes'] > 30000:
            print("   数据量充足，雷达输出正常！")
    elif cli_stats['cli_responses'] > 20:
        print("WARN: 部分成功 - 收到CLI响应但数据较少")
        print(f"   CLI响应: {cli_stats['cli_responses']}")
        print(f"   数据量: {data_stats['data_bytes']} bytes")
    else:
        print("FAIL: 测试失败 - 没有足够的响应或数据")
        print("\n可能原因:")
        print("   1. 固件未加载")
        print("   2. 雷达未启动")
        print("   3. 配置命令错误")
    print("="*70)


if __name__ == "__main__":
    main()
