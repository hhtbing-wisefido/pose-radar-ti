# 📡 附录G：雷达配置文件对比分析

> **文档版本**: v1.0  
> **创建日期**: 2026-01-09  
> **适用范围**: AWRL6844 mmw_demo vs InCabin Demo 配置文件差异分析  
> **关联文档**: [附录F-TLV数据格式兼容性要求](./AWRL6844雷达健康检测-附录F-TLV数据格式兼容性要求.md)

---

## 📋 目录

1. [概述](#1-概述)
2. [配置文件来源与分类](#2-配置文件来源与分类)
3. [核心参数对比](#3-核心参数对比)
4. [CLI命令差异详解](#4-cli命令差异详解)
5. [兼容性矩阵](#5-兼容性矩阵)
6. [配置文件完整示例](#6-配置文件完整示例)
7. [应用场景选择指南](#7-应用场景选择指南)
8. [HealthDetect项目配置建议](#8-healthdetect项目配置建议)

---

## 1. 概述

### 1.1 两种Demo的定位

| 项目 | mmw_demo (标准Demo) | InCabin Demo |
|-----|---------------------|--------------|
| **全称** | mmWave Demo | Automotive InCabin Security and Safety Demo |
| **SDK来源** | MMWAVE_L_SDK 06.01.00.01 | Radar Toolbox 3.30.00.06 |
| **主要用途** | 通用雷达演示、开发基础 | 车内人员检测、安全带提醒 |
| **目标应用** | 点云生成、目标检测 | CPD/SBR/入侵检测 |
| **TLV格式** | 标准Type=1 | 私有Type=3001+ |
| **可视化工具** | SDK Visualizer | InCabin专用GUI |

### 1.2 配置文件的作用

```
┌─────────────────────────────────────────────────────────────┐
│                    配置文件工作流程                            │
└─────────────────────────────────────────────────────────────┘

步骤1: 固件烧录 → Flash (SBL + 应用固件)
          ↓
步骤2: 上电启动 → CLI等待配置
          ↓
步骤3: PC发送.cfg → 串口发送CLI命令
          ↓
步骤4: CLI解析 → RAM配置缓存
          ↓
步骤5: sensorStart → 配置生效，雷达工作
```

**关键理解**：
- 🔴 **配置文件必须匹配固件** - 不同固件支持不同CLI命令
- 🔴 **InCabin配置无法在SDK Visualizer使用** - 包含自定义CLI命令
- 🔴 **标准配置无法在InCabin GUI使用** - 缺少必需的自定义命令

---

## 2. 配置文件来源与分类

### 2.1 mmw_demo标准配置文件

| 文件名 | 路径 | 用途 | 天线配置 |
|-------|------|------|---------|
| `6844_profile_4T4R_tdm.cfg` | `MMWAVE_L_SDK/.../visualizer/tmp/` | SDK Visualizer默认 | 4TX 4RX TDM |
| `6844_profile_4T4R_tdm.cfg` | `radar_toolbox/.../mmwave_data_recorder/src/cfg/` | 数据采集 | 4TX 4RX TDM |
| `profile_4T4R_tdm.cfg` | `mmw_demo/profiles/` | Demo工程内置 | 4TX 4RX TDM |
| `profile_2T4R_bpm.cfg` | `mmw_demo/profiles/` | BPM模式 | 2TX 4RX BPM |
| `profile_3T4R_tdm.cfg` | `mmw_demo/profiles/` | 3发4收 | 3TX 4RX TDM |

### 2.2 InCabin Demo专用配置文件

| 文件名 | 路径 | 用途 | 运行模式 |
|-------|------|------|---------|
| `cpd.cfg` | `AWRL6844_Incabin_GUI/src/chirpConfigs6844/` | 儿童存在检测 | runningMode=2 |
| `sbr.cfg` | 同上 | 安全带提醒 | runningMode=1 |
| `intrusion_detection.cfg` | 同上 | 入侵检测 | runningMode=0 |
| `intrusion_detection_LP.cfg` | 同上 | 低功耗入侵检测 | runningMode=0 |

### 2.3 配置文件分类总结

```
雷达配置文件
├── 标准mmw_demo配置
│   ├── 特点：22条标准CLI命令
│   ├── 工具：SDK Visualizer
│   └── TLV：Type=1 标准点云
│
└── InCabin Demo配置
    ├── 特点：标准CLI + 20+自定义CLI命令
    ├── 工具：InCabin专用GUI
    └── TLV：Type=3001+ 私有格式
```

---

## 3. 核心参数对比

### 3.1 硬件配置对比

| 参数 | mmw_demo (4T4R_tdm) | InCabin CPD | InCabin SBR | InCabin ID |
|-----|---------------------|-------------|-------------|------------|
| **channelCfg** | `153 255 0` | `15 15 0` | `15 15 0` | `15 15 0` |
| **TX天线** | 4TX (TDM) | 2TX | 2TX | 2TX |
| **RX天线** | 4RX | 2RX | 2RX | 2RX |
| **虚拟天线数** | 16 | 4 | 4 | 4 |
| **角度分辨率** | 更高 | 较低 | 较低 | 较低 |

**channelCfg参数解析**：
```
mmw_demo:  153 = 0x99 = 1001_1001 (TX0,TX3 TDM)
           255 = 0xFF = 1111_1111 (RX0-3全开)

InCabin:   15 = 0x0F = 0000_1111 (TX0,TX1)
           15 = 0x0F = 0000_1111 (RX0-3部分)
```

### 3.2 Chirp配置对比

| 参数 | mmw_demo | InCabin CPD/SBR | InCabin ID |
|-----|----------|-----------------|------------|
| **chirpComnCfg** | `8 0 0 256 1 13.1 3` | `50 0 0 128 1 38.25 1` | `80 0 0 128 1 63 0` |
| **采样点数** | 256 | 128 | 128 |
| **采样率** | 13.1 Msps | 38.25 Msps | 63 Msps |
| **频率斜率** | 3 MHz/μs | 1 MHz/μs | 0 MHz/μs |
| **chirpTimingCfg** | `6 63 0 160 58` | `4 28 1.5 102 57.5` | `7 24 0 50 57.5` |

### 3.3 帧配置对比

| 参数 | mmw_demo | InCabin CPD/SBR | InCabin ID |
|-----|----------|-----------------|------------|
| **frameCfg** | `64 0 1358 1 100 0` | `4 12 4000 32 200 0` | `16 0 1400 1 100 0` |
| **每帧Chirp数** | 64 | 4×12=48 burst | 16 |
| **帧周期** | 100ms (10 FPS) | 200ms (5 FPS) | 100ms (10 FPS) |
| **处理模式** | 连续帧 | Burst模式 | 连续帧 |

### 3.4 检测算法配置对比

| 参数 | mmw_demo | InCabin |
|-----|----------|---------|
| **CFAR配置** | `cfarProcCfg` | `dynamicRACfarCfg` |
| **AOA配置** | `aoaProcCfg 64 64` | `dynamic2DAngleCfg` |
| **视场角** | `aoaFovCfg -60 60 -60 60` | `fovCfg 73.0 73.0` |
| **杂波移除** | `clutterRemoval 0` | `clutterRemoval 1` |
| **天线几何** | `antGeometryBoard xWRL6844EVM` | `antGeometry0/1` 手动定义 |

---

## 4. CLI命令差异详解

### 4.1 mmw_demo标准CLI命令 (22条)

```properties
# ═══════════════════════════════════════════════════════════
# mmw_demo 标准CLI命令 - SDK Visualizer支持
# ═══════════════════════════════════════════════════════════

# 基础控制
sensorStop 0                          # 停止传感器
sensorStart 0 0 0 0                   # 启动传感器

# 硬件配置
channelCfg 153 255 0                  # 天线通道配置
chirpComnCfg 8 0 0 256 1 13.1 3       # Chirp公共配置
chirpTimingCfg 6 63 0 160 58          # Chirp时序配置
adcDataDitherCfg 1                    # ADC抖动配置
frameCfg 64 0 1358 1 100 0            # 帧配置

# 系统配置
gpAdcMeasConfig 0 0                   # GP ADC测量配置
lowPowerCfg 1                         # 低功耗配置

# 检测算法
cfarProcCfg 0 2 8 4 3 0 9.0 0         # Range CFAR配置
cfarProcCfg 1 2 4 2 2 1 9.0 0         # Doppler CFAR配置
cfarFovCfg 0 0.25 9.0                 # CFAR距离视场
cfarFovCfg 1 -20.16 20.16             # CFAR速度视场
aoaProcCfg 64 64                      # AOA处理配置
aoaFovCfg -60 60 -60 60               # AOA视场配置
clutterRemoval 0                      # 杂波移除

# 校准配置
factoryCalibCfg 1 0 44 2 0x1ff000     # 工厂校准
runtimeCalibCfg 1                     # 运行时校准

# 天线配置
antGeometryBoard xWRL6844EVM          # 天线几何板级定义

# 数据输出
guiMonitor 1 1 0 0 0 1                # GUI监控配置
adcDataSource 0 adc_test_data.bin     # ADC数据源
adcLogging 0                          # ADC日志
```

### 4.2 InCabin专用CLI命令 (20+条)

```properties
# ═══════════════════════════════════════════════════════════
# InCabin Demo 专用CLI命令 - 仅InCabin GUI支持
# ═══════════════════════════════════════════════════════════

# ⭐ 运行模式选择 (核心命令)
runningMode <mode> [cnnMode]
# mode: 0=入侵检测(ID), 1=安全带提醒(SBR), 2=儿童存在检测(CPD)
# cnnMode: 0=LPD_ANN, 1=LPD_CNN (仅CPD模式)
runningMode 2 1                       # CPD模式 + CNN

# ⭐ 信号处理链配置
sigProcChainCommonCfg <numFrmPerSlidingWindow> <numProcBurstPerframe> <framePeriod_ms> <numFrames> <startingBurstIdx>
sigProcChainCommonCfg 4 32 200 0 0

sigProcChainCfg <azimuthFftSize> <elevationFftSize> <coherentDoppler>
sigProcChainCfg 16 16 2               # 仅入侵检测模式

# ⭐ 宏多普勒配置 (CPD专用)
macroDopplerCfg <enable> <delayLineLen>
macroDopplerCfg 1 40

macroDopMapScaleCfg <zone0> <zone1> <zone2> <zone3> <zone4>
macroDopMapScaleCfg 1.0 1.0 3.0 3.0 3.0

macroDopNumVoxelCfg <zone0> <zone1> <zone2> <zone3> <zone4>
macroDopNumVoxelCfg 30 30 30 30 30

macroDopRngBinOffsetCfg <zone0> <zone1> <zone2> <zone3> <zone4>
macroDopRngBinOffsetCfg 5 5 20 14 20

# ⭐ 动态CFAR配置
dynamicRACfarCfg <...15个参数...>
dynamicRACfarCfg 5 10 1 1 8 8 8 10 4 10 7.0 6.00 0.50 1 1

dynamicRangeAngleCfg <maxRange> <minRange> <mode> <reserved>
dynamicRangeAngleCfg 8.000 0.03 2 0

dynamic2DAngleCfg <numAngleBins> <enable> <reserved> <threshold1> <threshold2> <reserved>
dynamic2DAngleCfg 5 1 1 1.0 7.0 2

# ⭐ 天线几何定义 (手动)
antGeometry0 <16个索引值>             # 方位角索引
antGeometry0 -2 -2 -3 -3  0 0 -1 -1  0 0 -1 -1  -2 -2 -3 -3

antGeometry1 <16个索引值>             # 俯仰角索引
antGeometry1  0 -1 -1 0  0 -1 -1 0  -2 -3 -3 -2  -2 -3 -3 -2

antPhaseRot <16个相位旋转值>
antPhaseRot 1 1 1 1  1 1 1 1  1 1 1 1  1 1 1 1

antGeometryCfg <索引...> <间距>        # 入侵检测模式
antGeometryCfg 0 2 1 2 1 3 0 3  0 0 1 0 1 1 0 1  2 0 3 0 3 1 2 1  2 2 3 2 3 3 2 3  2.540 2.540

# ⭐ 传感器位置配置
sensorPosition <xOffset> <yOffset> <zOffset> <azimuthTilt> <elevationTilt>
sensorPosition 0 0.70 1.17 180 -120

# ⭐ 检测区域定义
# SBR/CPD模式 - 立方体区域
cuboidDef <seatId> <zoneId> <xMin> <xMax> <yMin> <yMax> <zMin> <zMax>
cuboidDef 0 0  0.15 0.7  0.45 1.06  0.6 1.2    # 驾驶座头部区域
cuboidDef 0 1  0.15 0.7  0.1 0.9   0.1 0.6     # 驾驶座胸部区域
cuboidDef 0 2  0.15 0.7  -0.2 0.5  -0.2 0.3    # 驾驶座腿部区域

# 入侵检测模式 - 占用盒
occupancyBox <zoneId> <xMin> <xMax> <yMin> <yMax> <zMin> <zMax>
occupancyBox 0 -0.20 0.20 -0.30 1.00 0.00 1.15

# ⭐ 特征提取配置 (SBR/CPD)
featExtrCfg <param1> <param2> <param3> <param4> <param5> <param6>
featExtrCfg 130 30 1 0 0.4 10

# ⭐ Z轴偏移配置
zOffsetCfg <zone0> <zone1> <zone2> <zone3> <zone4>
zOffsetCfg 0 0 0 0 0

# ⭐ 入侵检测配置
intruderDetAdvCfg <zoneIdx> <threshold> <free2ActiveCntr> <active2FreeCntr> <localPeakCheck> <sideLobeThre> <peakExpSamples>
intruderDetAdvCfg 1 9 2 10 2 0.9 2

# ⭐ 多普勒Bin选择
dopplerBinSelCfg <enable> <numBins> <startBin> <endBin>
dopplerBinSelCfg 1 32 0 4

# ⭐ 视场角配置 (简化版)
fovCfg <azimuthFov> <elevationFov>
fovCfg 73.0 73.0

# ⭐ 相位补偿配置
compRangeBiasAndRxChanPhase <bias> <32个I/Q值>
compRangeBiasAndRxChanPhase 0 1 0 1 0 1 0 1 0 -1 0 -1 0 -1 0 -1 0 1 0 1 0 1 0 1 0 -1 0 -1 0 -1 0 -1 0

# ⭐ 二次CFAR配置 (入侵检测)
cfarScndPassCfg <enabled> <averageMode> <winLen> <guardLen> <noiseDiv> <cyclicMode>
cfarScndPassCfg 1 2 4 3 2 1
```

### 4.3 命令兼容性总结

| 命令类别 | 标准mmw_demo | InCabin CPD | InCabin SBR | InCabin ID |
|---------|-------------|-------------|-------------|------------|
| 基础控制 | ✅ | ✅ | ✅ | ✅ |
| 硬件配置 | ✅ | ✅ | ✅ | ✅ |
| `cfarProcCfg` | ✅ | ❌ | ❌ | ❌ |
| `dynamicRACfarCfg` | ❌ | ✅ | ✅ | ❌ |
| `aoaProcCfg/aoaFovCfg` | ✅ | ❌ | ❌ | ❌ |
| `runningMode` | ❌ | ✅ | ✅ | ✅ |
| `cuboidDef` | ❌ | ✅ | ✅ | ❌ |
| `occupancyBox` | ❌ | ❌ | ❌ | ✅ |
| `sensorPosition` | ❌ | ✅ | ✅ | ✅ |
| `macroDopplerCfg` | ❌ | ✅ | ❌ | ❌ |
| `antGeometryBoard` | ✅ | ❌ | ❌ | ❌ |
| `antGeometry0/1` | ❌ | ✅ | ✅ | ❌ |

---

## 5. 兼容性矩阵

### 5.1 固件与配置文件兼容性

| 配置文件 | mmwave_demo固件 | InCabin固件 | 失败原因 |
|---------|----------------|-------------|---------|
| `6844_profile_4T4R_tdm.cfg` | ✅ 完全兼容 | ❌ 缺少runningMode | CLI命令不存在 |
| `cpd.cfg` | ❌ runningMode不识别 | ✅ 完全兼容 | 自定义命令 |
| `sbr.cfg` | ❌ runningMode不识别 | ✅ 完全兼容 | 自定义命令 |
| `intrusion_detection.cfg` | ❌ runningMode不识别 | ✅ 完全兼容 | 自定义命令 |

### 5.2 工具与配置文件兼容性

| 配置文件 | SDK Visualizer | InCabin GUI | Industrial Visualizer |
|---------|---------------|-------------|----------------------|
| `6844_profile_4T4R_tdm.cfg` | ✅ | ❌ | ✅ (可能版本警告) |
| `cpd.cfg` | ❌ | ✅ | ❌ |
| `sbr.cfg` | ❌ | ✅ | ❌ |
| `intrusion_detection.cfg` | ❌ | ✅ | ❌ |

### 5.3 TLV输出格式对比

| 数据类型 | mmw_demo TLV Type | InCabin TLV Type | 兼容性 |
|---------|------------------|------------------|--------|
| 点云数据 | Type = 1 | Type = 3001 | ❌ 不兼容 |
| Range Profile | Type = 2 | Type = 2 | ✅ 兼容 |
| 统计信息 | Type = 6 | Type = 6 | ✅ 兼容 |
| 温度信息 | Type = 9 | Type = 9 | ✅ 兼容 |
| 入侵检测结果 | ❌ 无 | Type = 1020 | InCabin专用 |
| 占用特征 | ❌ 无 | Type = 1040 | InCabin专用 |
| 占用分类结果 | ❌ 无 | Type = 1041 | InCabin专用 |
| 身高估计 | ❌ 无 | Type = 1042 | InCabin专用 |
| 宏多普勒 | ❌ 无 | Type = 1043 | InCabin专用 |

---

## 6. 配置文件完整示例

### 6.1 mmw_demo标准配置 (6844_profile_4T4R_tdm.cfg)

```properties
% ═══════════════════════════════════════════════════════════
% TI官方标准配置 - mmwave_demo SDK 06.01.00.01
% 用途: 4TX 4RX TDM模式，通用点云检测
% 工具: SDK Visualizer
% ═══════════════════════════════════════════════════════════

sensorStop 0

% 硬件配置
channelCfg 153 255 0
chirpComnCfg 8 0 0 256 1 13.1 3
chirpTimingCfg 6 63 0 160 58
adcDataDitherCfg 1
frameCfg 64 0 1358 1 100 0

% 系统配置
gpAdcMeasConfig 0 0
lowPowerCfg 1

% 检测算法
guiMonitor 1 1 0 0 0 1
cfarProcCfg 0 2 8 4 3 0 9.0 0
cfarProcCfg 1 2 4 2 2 1 9.0 0
cfarFovCfg 0 0.25 9.0
cfarFovCfg 1 -20.16 20.16
aoaProcCfg 64 64
aoaFovCfg -60 60 -60 60
clutterRemoval 0

% 校准配置
factoryCalibCfg 1 0 44 2 0x1ff000
runtimeCalibCfg 1

% 天线配置
antGeometryBoard xWRL6844EVM

% 数据配置
adcDataSource 0 adc_test_data_0001.bin
adcLogging 0

% 启动
sensorStart 0 0 0 0
```

**配置特点**：
- ✅ 22条标准CLI命令
- ✅ 4TX 4RX TDM模式，16虚拟天线
- ✅ 10 FPS帧率
- ✅ 使用`antGeometryBoard`简化天线配置
- ✅ 标准CFAR/AOA算法

### 6.2 InCabin CPD配置 (cpd.cfg)

```properties
% ═══════════════════════════════════════════════════════════
% InCabin CPD配置 - 儿童存在检测
% 用途: 车内儿童/成人存在检测
% 工具: InCabin专用GUI (occupancy_demo_gui.exe)
% ═══════════════════════════════════════════════════════════

sensorStop 0

% 硬件配置
channelCfg 15 15 0
chirpComnCfg 50 0 0 128 1 38.25 1
chirpTimingCfg 4 28 1.5 102 57.5
frameCfg 4 12 4000 32 200 0
factoryCalibCfg 1 0 38 2 0x1ff000
runtimeCalibCfg 0

% ⭐ InCabin专用: 运行模式
runningMode 2 1                       % CPD模式 + CNN分类

% ⭐ InCabin专用: 信号处理链
sigProcChainCommonCfg 4 32 200 0 0

% ⭐ InCabin专用: 宏多普勒
macroDopplerCfg 1 40

% GUI监控
guiMonitor 1 0 1 0 0 1 1 1

% ⭐ InCabin专用: 动态CFAR
dynamicRACfarCfg 5 10 1 1 8 8 8 10 4 10 7.0 6.00 0.50 1 1
dynamicRangeAngleCfg 8.000 0.03 2 0
dynamic2DAngleCfg 5 1 1 1.0 7.0 2
dopplerBinSelCfg 1 32 0 4

% ⭐ InCabin专用: 天线几何 (手动定义)
antGeometry0 -2 -2 -3 -3  0 0 -1 -1  0 0 -1 -1  -2 -2 -3 -3
antGeometry1  0 -1 -1 0  0 -1 -1 0  -2 -3 -3 -2  -2 -3 -3 -2
antPhaseRot 1 1 1 1  1 1 1 1  1 1 1 1  1 1 1 1
compRangeBiasAndRxChanPhase 0 1 0 1 0 1 0 1 0 -1 0 -1 0 -1 0 -1 0 1 0 1 0 1 0 1 0 -1 0 -1 0 -1 0 -1 0

% ⭐ InCabin专用: 视场角
fovCfg 73.0 73.0

% ⭐ InCabin专用: 传感器位置
sensorPosition 0 0.70 1.17 180 -120

% ⭐ InCabin专用: 座位区域定义 (5个座位 × 3个区域)
% 驾驶座
cuboidDef 0 0  0.15 0.7  0.45 1.06  0.6 1.2     % 头部
cuboidDef 0 1  0.15 0.7  0.1 0.9   0.1 0.6      % 胸部
cuboidDef 0 2  0.15 0.7  -0.2 0.5  -0.2 0.3     % 腿部
% 副驾驶
cuboidDef 1 0  -0.7 -0.15  0.45 1.06  0.6 1.2
cuboidDef 1 1  -0.7 -0.15  0.1 0.9   0.1 0.6
cuboidDef 1 2  -0.7 -0.15  -0.2 0.5  -0.2 0.3
% 后排左
cuboidDef 2 0  0.2 0.7  1.4 2.0  0.6 1.2
cuboidDef 2 1  0.2 0.7  1.1 2.0  0.2 0.6
cuboidDef 2 2  0.2 0.7  1.0 1.4  -0.35 0.35
% 后排中
cuboidDef 3 0  -0.15 0.15  1.4 1.90  0.30 1.2
cuboidDef 3 1  -0.15 0.15  1.1 1.3   0.2 0.6
cuboidDef 3 2  -0.15 0.15  1.0 1.35  0.1 0.35
% 后排右
cuboidDef 4 0  -0.7 -0.2  1.4 2.0  0.6 1.2
cuboidDef 4 1  -0.7 -0.2  1.06 2.0  0.2 0.6
cuboidDef 4 2  -0.7 -0.2  1.0 1.4  -0.35 0.35

% ⭐ InCabin专用: 特征提取
featExtrCfg 130 30 1 0 0.4 10

% ⭐ InCabin专用: Z轴偏移
zOffsetCfg 0 0 0 0 0

% ⭐ InCabin专用: 宏多普勒参数
macroDopMapScaleCfg 1.0 1.0 3.0 3.0 3.0
macroDopNumVoxelCfg 30 30 30 30 30
macroDopRngBinOffsetCfg 5 5 20 14 20

lowPowerCfg 0
```

**配置特点**：
- 🔴 包含20+条InCabin专用CLI命令
- 🔴 2TX 2RX模式，4虚拟天线
- 🔴 5 FPS帧率（Burst模式）
- 🔴 手动定义天线几何和检测区域
- 🔴 使用动态CFAR算法
- 🔴 TLV使用Type=3001私有格式

---

## 7. 应用场景选择指南

### 7.1 场景决策树

```
需要什么功能？
│
├─ 通用点云检测/开发基础
│   └─ ✅ 使用 mmw_demo + 6844_profile_4T4R_tdm.cfg
│       ├─ 固件: mmwave_demo.release.appimage
│       ├─ 工具: SDK Visualizer
│       └─ TLV: Type=1 标准格式
│
├─ 车内人员检测（CPD/SBR）
│   └─ ✅ 使用 InCabin Demo + cpd.cfg/sbr.cfg
│       ├─ 固件: demo_in_cabin_sensing_6844_system.release.appimage
│       ├─ 工具: InCabin GUI (occupancy_demo_gui.exe)
│       └─ TLV: Type=3001 InCabin格式
│
├─ 入侵检测
│   └─ ✅ 使用 InCabin Demo + intrusion_detection.cfg
│       ├─ 固件: demo_in_cabin_sensing_6844_system.release.appimage
│       ├─ 工具: InCabin GUI
│       └─ TLV: Type=1020 入侵检测格式
│
└─ 自定义健康检测（HealthDetect项目）
    └─ ✅ 基于 mmw_demo 标准架构
        ├─ 固件: 自定义固件（基于mmw_demo演进）
        ├─ 配置: 基于6844_profile_4T4R_tdm.cfg扩展
        ├─ TLV: Type=1 兼容SDK Visualizer
        └─ 扩展: Type=1000+ 健康检测专用
```

### 7.2 固件选择建议

| 应用场景 | 推荐固件 | 推荐配置 | 推荐工具 |
|---------|---------|---------|---------|
| **开发学习** | mmwave_demo | 6844_profile_4T4R_tdm.cfg | SDK Visualizer |
| **车内CPD** | InCabin Demo | cpd.cfg | InCabin GUI |
| **车内SBR** | InCabin Demo | sbr.cfg | InCabin GUI |
| **入侵检测** | InCabin Demo | intrusion_detection.cfg | InCabin GUI |
| **健康检测** | 自定义(基于mmw_demo) | 自定义 | SDK Visualizer |

---

## 8. HealthDetect项目配置建议

### 8.1 为什么基于mmw_demo而非InCabin

| 考虑因素 | mmw_demo | InCabin Demo | HealthDetect选择 |
|---------|----------|--------------|------------------|
| **CLI兼容性** | 标准22条命令 | 40+自定义命令 | ✅ 标准命令 |
| **工具兼容性** | SDK Visualizer | 专用GUI | ✅ SDK Visualizer |
| **TLV格式** | Type=1 标准 | Type=3001 私有 | ✅ Type=1 标准 |
| **天线配置** | 4TX 4RX (高分辨率) | 2TX 2RX | ✅ 4TX 4RX |
| **扩展性** | 易于扩展 | 耦合度高 | ✅ 易扩展 |
| **参考代码** | 简洁清晰 | 复杂 | ✅ 简洁 |

### 8.2 HealthDetect配置文件设计

基于 `6844_profile_4T4R_tdm.cfg` 扩展：

```properties
% ═══════════════════════════════════════════════════════════
% HealthDetect配置文件 - health_detect_4T4R.cfg
% 基于mmw_demo标准配置扩展
% ═══════════════════════════════════════════════════════════

sensorStop 0

% === 硬件配置 (保持标准) ===
channelCfg 153 255 0              % 4TX 4RX TDM
chirpComnCfg 8 0 0 256 1 13.1 3
chirpTimingCfg 6 63 0 160 58
adcDataDitherCfg 1
frameCfg 64 0 1358 1 100 0        % 10 FPS

% === 系统配置 ===
gpAdcMeasConfig 0 0
lowPowerCfg 0                     % 关闭低功耗，便于连续监测

% === 检测算法 ===
guiMonitor 1 1 0 0 0 1
cfarProcCfg 0 2 8 4 3 0 9.0 0     % Range CFAR
cfarProcCfg 1 2 4 2 2 1 9.0 0     % Doppler CFAR
cfarFovCfg 0 0.25 9.0
cfarFovCfg 1 -20.16 20.16
aoaProcCfg 64 64
aoaFovCfg -60 60 -60 60
clutterRemoval 1                  % 🔧 开启杂波移除（室内环境）

% === 校准配置 ===
factoryCalibCfg 1 0 44 2 0x1ff000
runtimeCalibCfg 1

% === 天线配置 ===
antGeometryBoard xWRL6844EVM

% === 数据配置 ===
adcDataSource 0 adc_test_data_0001.bin
adcLogging 0

sensorStart 0 0 0 0
```

**关键调整**：
1. `clutterRemoval 1` - 室内环境开启杂波移除
2. `lowPowerCfg 0` - 关闭低功耗便于连续监测
3. 保持4TX 4RX获得更高角度分辨率
4. 使用标准TLV格式便于SDK Visualizer验证

### 8.3 TLV扩展方案

```c
// 标准TLV类型 (Type 1-12) - 保持兼容
MMWDEMO_OUTPUT_MSG_DETECTED_POINTS = 1,      // 点云 (必须Type=1)
MMWDEMO_OUTPUT_MSG_RANGE_PROFILE = 2,
MMWDEMO_OUTPUT_MSG_STATS = 6,

// HealthDetect扩展TLV (Type 1000+) - 避开官方范围
MMWDEMO_OUTPUT_MSG_PRESENCE_DETECT = 1000,   // 人存检测结果
MMWDEMO_OUTPUT_MSG_HEALTH_FEATURES = 1001,   // 健康特征向量
MMWDEMO_OUTPUT_MSG_VITAL_SIGNS = 1002,       // 生命体征
MMWDEMO_OUTPUT_MSG_POSTURE_RESULT = 1003,    // 姿态检测
MMWDEMO_OUTPUT_MSG_FALL_DETECTION = 1004,    // 跌倒检测
```

---

## 📚 参考文档

| 文档 | 位置 | 用途 |
|-----|------|------|
| 雷达配置文件深度分析_v2.0 | `05-雷达配置参数研究/` | 22条命令详细参数 |
| 固件与雷达配置匹配关系报告 | `2-开发记录/2025-12-23/` | 实际测试验证 |
| 附录F-TLV数据格式兼容性要求 | 本目录 | TLV格式详解 |
| SDK源码架构分析 | `附录B` | CLI命令实现分析 |
| InCabin架构学习参考 | `附录C` | InCabin Demo架构 |

---

## 📝 版本历史

| 版本 | 日期 | 说明 |
|-----|------|------|
| v1.0 | 2026-01-09 | 初版，完整对比mmw_demo与InCabin配置差异 |

---

> ⚠️ **核心结论**：HealthDetect项目应基于mmw_demo标准Demo开发，使用标准CLI命令和Type=1 TLV格式，确保与SDK Visualizer兼容。InCabin Demo可作为功能参考，但不直接复用其配置文件和私有TLV格式。
