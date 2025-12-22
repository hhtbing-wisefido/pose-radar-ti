#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
雷达配置文件性能计算器
用于计算雷达配置文件的各项性能参数
"""

import re
from pathlib import Path
from typing import Dict, Optional

class RadarConfigCalculator:
    """雷达配置性能计算器"""
    
    def __init__(self):
        self.c = 3e8  # 光速 (m/s)
    
    def parse_config_file(self, cfg_path: str) -> Dict:
        """
        解析.cfg配置文件
        
        Args:
            cfg_path: 配置文件路径
            
        Returns:
            配置参数字典
        """
        config = {}
        
        try:
            with open(cfg_path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            # 解析channelCfg
            match = re.search(r'channelCfg\s+(\d+)\s+(\d+)\s+(\d+)', content)
            if match:
                config['rxChannelEn'] = int(match.group(1))
                config['txChannelEn'] = int(match.group(2))
                config['cascading'] = int(match.group(3))
            
            # 解析profileCfg
            match = re.search(
                r'profileCfg\s+(\d+)\s+([\d.]+)\s+([\d.]+)\s+([\d.]+)\s+([\d.]+)\s+'
                r'([\d.]+)\s+([\d.]+)\s+([\d.]+)\s+([\d.]+)\s+(\d+)\s+([\d.]+)\s+'
                r'([\d.]+)\s+([\d.]+)\s+([\d.]+)',
                content
            )
            if match:
                config['profileId'] = int(match.group(1))
                config['startFreq'] = float(match.group(2))
                config['idleTime'] = float(match.group(3))
                config['adcStartTime'] = float(match.group(4))
                config['rampEndTime'] = float(match.group(5))
                config['txOutPower'] = float(match.group(6))
                config['txPhaseShifter'] = float(match.group(7))
                config['freqSlopeConst'] = float(match.group(8))
                config['txStartTime'] = float(match.group(9))
                config['numAdcSamples'] = int(match.group(10))
                config['digOutSampleRate'] = float(match.group(11))
                config['hpfCornerFreq1'] = float(match.group(12))
                config['hpfCornerFreq2'] = float(match.group(13))
                config['rxGain'] = float(match.group(14))
            
            # 解析frameCfg
            match = re.search(
                r'frameCfg\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+([\d.]+)\s+(\d+)\s+([\d.]+)',
                content
            )
            if match:
                config['chirpStartIdx'] = int(match.group(1))
                config['chirpEndIdx'] = int(match.group(2))
                config['numLoops'] = int(match.group(3))
                config['numFrames'] = int(match.group(4))
                config['framePeriodicity'] = float(match.group(5))
                config['triggerSelect'] = int(match.group(6))
                config['frameTriggerDelay'] = float(match.group(7))
            
        except Exception as e:
            print(f"解析配置文件失败: {e}")
            return {}
        
        return config
    
    def count_enabled_channels(self, channel_mask: int) -> int:
        """计算启用的通道数量"""
        count = 0
        while channel_mask:
            count += channel_mask & 1
            channel_mask >>= 1
        return count
    
    def calculate_performance(self, config: Dict) -> Dict:
        """
        计算雷达性能参数
        
        Args:
            config: 配置参数字典
            
        Returns:
            性能参数字典
        """
        if not config:
            return {}
        
        performance = {}
        
        try:
            # 天线配置
            num_rx = self.count_enabled_channels(config.get('rxChannelEn', 0))
            num_tx = self.count_enabled_channels(config.get('txChannelEn', 0))
            num_virtual_antennas = num_rx * num_tx
            
            performance['RX天线数'] = num_rx
            performance['TX天线数'] = num_tx
            performance['虚拟天线数'] = num_virtual_antennas
            
            # 频率和时间参数
            freq_slope = config.get('freqSlopeConst', 0)  # MHz/μs
            ramp_time = config.get('rampEndTime', 0) - config.get('adcStartTime', 0)  # μs
            idle_time = config.get('idleTime', 0)  # μs
            chirp_time = ramp_time + idle_time  # μs
            
            # 带宽
            if freq_slope > 0 and ramp_time > 0:
                bandwidth = freq_slope * ramp_time  # MHz
                performance['带宽 (MHz)'] = bandwidth
                performance['带宽 (GHz)'] = bandwidth / 1000
            else:
                bandwidth = 0
            
            # 距离性能
            if bandwidth > 0:
                range_resolution = self.c / (2 * bandwidth * 1e6)  # m
                performance['距离分辨率 (m)'] = range_resolution
                performance['距离分辨率 (cm)'] = range_resolution * 100
            
            num_samples = config.get('numAdcSamples', 0)
            sample_rate = config.get('digOutSampleRate', 0)  # ksps
            
            if freq_slope > 0 and sample_rate > 0:
                max_range = (num_samples * self.c) / (2 * freq_slope * 1e6 * sample_rate * 1e3)  # m
                performance['最大检测距离 (m)'] = max_range
            
            # 速度性能
            start_freq = config.get('startFreq', 77)  # GHz
            lambda_wave = self.c / (start_freq * 1e9)  # m
            performance['波长 (mm)'] = lambda_wave * 1000
            
            num_chirps = config.get('numLoops', 0)
            if chirp_time > 0 and num_chirps > 0:
                velocity_resolution = lambda_wave / (2 * num_chirps * chirp_time * 1e-6)  # m/s
                max_velocity = lambda_wave / (4 * chirp_time * 1e-6)  # m/s
                
                performance['速度分辨率 (m/s)'] = velocity_resolution
                performance['速度分辨率 (km/h)'] = velocity_resolution * 3.6
                performance['最大速度 (m/s)'] = max_velocity
                performance['最大速度 (km/h)'] = max_velocity * 3.6
            
            # 帧率
            frame_period = config.get('framePeriodicity', 0)  # ms
            if frame_period > 0:
                frame_rate = 1000 / frame_period  # FPS
                performance['帧周期 (ms)'] = frame_period
                performance['帧率 (FPS)'] = frame_rate
            
            # 时间参数
            performance['Chirp时间 (μs)'] = chirp_time
            performance['单帧时间 (ms)'] = num_chirps * chirp_time / 1000
            
            # 数据量估算
            bytes_per_sample = 4  # 复数I/Q，每个16位
            data_per_chirp = num_samples * num_rx * bytes_per_sample  # bytes
            data_per_frame = data_per_chirp * num_chirps  # bytes
            performance['单帧数据量 (KB)'] = data_per_frame / 1024
            performance['单帧数据量 (MB)'] = data_per_frame / (1024 * 1024)
            
            if frame_period > 0:
                data_rate = (data_per_frame / (frame_period / 1000)) / (1024 * 1024)  # MB/s
                performance['数据率 (MB/s)'] = data_rate
            
            # 角度分辨率估算（简化计算）
            if num_virtual_antennas > 1:
                # 假设线性阵列，天线间距为半波长
                angle_resolution = 2 / num_virtual_antennas * 180 / 3.14159  # 度
                performance['角度分辨率 (度)'] = angle_resolution
            
        except Exception as e:
            print(f"计算性能失败: {e}")
            return {}
        
        return performance
    
    def print_performance(self, performance: Dict):
        """打印性能参数"""
        if not performance:
            print("❌ 无性能数据")
            return
        
        print("\n" + "=" * 60)
        print("📊 雷达性能参数")
        print("=" * 60)
        
        # 天线配置
        print("\n🔌 天线配置")
        print(f"  RX天线数: {performance.get('RX天线数', 'N/A')}")
        print(f"  TX天线数: {performance.get('TX天线数', 'N/A')}")
        print(f"  虚拟天线数: {performance.get('虚拟天线数', 'N/A')}")
        
        # 频率和带宽
        print("\n📡 频率配置")
        print(f"  带宽: {performance.get('带宽 (GHz)', 'N/A'):.3f} GHz")
        print(f"  波长: {performance.get('波长 (mm)', 'N/A'):.3f} mm")
        
        # 距离性能
        print("\n📏 距离性能")
        print(f"  距离分辨率: {performance.get('距离分辨率 (cm)', 'N/A'):.2f} cm")
        print(f"  最大检测距离: {performance.get('最大检测距离 (m)', 'N/A'):.2f} m")
        
        # 速度性能
        print("\n🚀 速度性能")
        print(f"  速度分辨率: {performance.get('速度分辨率 (m/s)', 'N/A'):.3f} m/s ({performance.get('速度分辨率 (km/h)', 'N/A'):.3f} km/h)")
        print(f"  最大速度: {performance.get('最大速度 (m/s)', 'N/A'):.2f} m/s ({performance.get('最大速度 (km/h)', 'N/A'):.2f} km/h)")
        
        # 角度性能
        if '角度分辨率 (度)' in performance:
            print("\n📐 角度性能")
            print(f"  角度分辨率: {performance.get('角度分辨率 (度)', 'N/A'):.2f}°")
        
        # 时间和帧率
        print("\n⏱️ 时间参数")
        print(f"  Chirp时间: {performance.get('Chirp时间 (μs)', 'N/A'):.2f} μs")
        print(f"  单帧时间: {performance.get('单帧时间 (ms)', 'N/A'):.2f} ms")
        print(f"  帧周期: {performance.get('帧周期 (ms)', 'N/A'):.2f} ms")
        print(f"  帧率: {performance.get('帧率 (FPS)', 'N/A'):.2f} FPS")
        
        # 数据量
        print("\n💾 数据量")
        print(f"  单帧数据量: {performance.get('单帧数据量 (KB)', 'N/A'):.2f} KB")
        print(f"  数据率: {performance.get('数据率 (MB/s)', 'N/A'):.3f} MB/s")
        
        print("\n" + "=" * 60)
    
    def validate_config(self, config: Dict) -> list:
        """
        验证配置参数合法性
        
        Returns:
            错误/警告信息列表
        """
        issues = []
        
        if not config:
            issues.append("❌ 配置为空")
            return issues
        
        # 检查ADC采样点数是否为2的幂
        num_samples = config.get('numAdcSamples', 0)
        if num_samples & (num_samples - 1) != 0:
            issues.append(f"❌ numAdcSamples ({num_samples}) 必须是2的幂次")
        
        # 检查时间顺序
        tx_start = config.get('txStartTime', 0)
        adc_start = config.get('adcStartTime', 0)
        ramp_end = config.get('rampEndTime', 0)
        
        if tx_start >= adc_start:
            issues.append(f"❌ txStartTime ({tx_start}) 必须小于 adcStartTime ({adc_start})")
        
        if adc_start >= ramp_end:
            issues.append(f"❌ adcStartTime ({adc_start}) 必须小于 rampEndTime ({ramp_end})")
        
        # 检查采样时间
        sampling_time = (num_samples / (config.get('digOutSampleRate', 1) * 1000)) * 1e6  # μs
        available_time = ramp_end - adc_start
        
        if sampling_time > available_time:
            issues.append(f"⚠️ 采样时间 ({sampling_time:.2f} μs) 超过可用时间 ({available_time:.2f} μs)")
        
        # 检查帧时间
        num_chirps = config.get('numLoops', 0)
        chirp_time = ramp_end + config.get('idleTime', 0)
        frame_time = num_chirps * chirp_time / 1000  # ms
        frame_period = config.get('framePeriodicity', 0)
        
        if frame_time > frame_period:
            issues.append(f"❌ 单帧时间 ({frame_time:.2f} ms) 超过帧周期 ({frame_period:.2f} ms)")
        
        # 检查数据率
        performance = self.calculate_performance(config)
        data_rate = performance.get('数据率 (MB/s)', 0)
        
        if data_rate > 50:
            issues.append(f"⚠️ 数据率过高 ({data_rate:.2f} MB/s)，可能超过接口带宽")
        
        if not issues:
            issues.append("✅ 配置参数验证通过")
        
        return issues


def main():
    """主函数"""
    import sys
    
    print("=" * 60)
    print("📡 雷达配置文件性能计算器")
    print("=" * 60)
    
    if len(sys.argv) < 2:
        print("\n用法: python config_calculator.py <config_file.cfg>")
        print("\n示例配置文件路径:")
        print("  C:\\ti\\MMWAVE_SDK\\demos\\profile.cfg")
        return
    
    cfg_file = sys.argv[1]
    
    if not Path(cfg_file).exists():
        print(f"\n❌ 配置文件不存在: {cfg_file}")
        return
    
    print(f"\n📂 配置文件: {cfg_file}")
    
    # 创建计算器
    calculator = RadarConfigCalculator()
    
    # 解析配置文件
    print("\n🔍 解析配置文件...")
    config = calculator.parse_config_file(cfg_file)
    
    if not config:
        print("❌ 解析失败")
        return
    
    print("✅ 解析成功")
    
    # 验证配置
    print("\n🔬 验证配置参数...")
    issues = calculator.validate_config(config)
    for issue in issues:
        print(f"  {issue}")
    
    # 计算性能
    performance = calculator.calculate_performance(config)
    
    # 打印性能
    calculator.print_performance(performance)


if __name__ == "__main__":
    main()
