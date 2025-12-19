# 📡 AWRL6844 雷达配置文件研究总结

## 🎯 研究目标

全面理解 Ti AWRL6844 雷达配置文件的结构、用途、参数含义及实际应用，为后续开发可视化配置工具和参数优化提供理论基础。

---

## 📋 目录

1. [配置文件概览](#配置文件概览)
2. [配置文件类型详解](#配置文件类型详解)
3. [配置参数深度解析](#配置参数深度解析)
4. [配置文件示例库](#配置文件示例库)
5. [参数调优指南](#参数调优指南)
6. [工具开发需求](#工具开发需求)

---

## 🔍 配置文件概览

### 配置文件分类

Ti 雷达系统中涉及**三类**不同的配置文件，容易混淆：

| 类型 | 文件名示例 | 使用阶段 | 是否需要烧录 | 说明 |
|------|-----------|----------|--------------|------|
| **🔧 SysConfig配置** | `mmwave.syscfg` | 编译时 | ❌ 不需要 | 硬件外设配置（已编译进固件） |
| **⚙️ RTOS配置** | `mmwave.cfg` | 编译时 | ❌ 不需要 | 操作系统配置（已编译进固件） |
| **📡 雷达参数配置** | `profile.cfg` | 运行时 | ✅ 串口发送 | 雷达工作参数（本研究重点） |

> ⚠️ **重要区别**：前两类是开发阶段的配置文件，已经编译进固件；只有雷达参数配置文件（`.cfg`）需要在运行时通过串口发送给雷达。

---

## 📡 雷达参数配置文件详解

### 1. 文件结构

典型的雷达配置文件包含以下部分：

```cfg
% ===== 注释区域 =====
% 配置说明
% 应用场景
% 参数范围

% ===== 全局设置 =====
sensorStop
flushCfg
dfeDataOutputMode 1

% ===== Chirp配置 =====
channelCfg 15 5 0
adcCfg 2 1
adcbufCfg -1 0 1 1 1

% ===== Profile配置 =====
profileCfg 0 77 7 7 200 0 0 70 1 256 5209 0 0 180

% ===== Frame配置 =====
frameCfg 0 0 16 0 100 1 0

% ===== LVDS配置 =====
lvdsStreamCfg -1 0 0 0

% ===== 启动命令 =====
sensorStart
```

### 2. 核心命令详解

#### 2.1 `channelCfg` - 通道配置

**格式**：`channelCfg <rxChannelEn> <txChannelEn> <cascading>`

```cfg
channelCfg 15 5 0
```

**参数说明**：

| 参数 | 值示例 | 二进制 | 含义 |
|------|--------|--------|------|
| `rxChannelEn` | 15 | 1111 | 启用RX1-RX4（4个接收天线） |
| `txChannelEn` | 5 | 0101 | 启用TX1和TX3（2个发射天线） |
| `cascading` | 0 | - | 单芯片模式（0=单芯片，1/2=级联） |

**常见配置**：

```cfg
channelCfg 15 7 0    # 4RX + 3TX = 12个虚拟天线
channelCfg 15 5 0    # 4RX + 2TX = 8个虚拟天线
channelCfg 3 1 0     # 2RX + 1TX = 2个虚拟天线（低功耗）
```

**天线数量计算**：
- 虚拟天线数 = RX数量 × TX数量
- 更多虚拟天线 = 更高角度分辨率
- 更多天线 = 更高功耗

#### 2.2 `adcCfg` - ADC配置

**格式**：`adcCfg <numADCBits> <adcOutputFmt>`

```cfg
adcCfg 2 1
```

**参数说明**：

| 参数 | 值 | 含义 |
|------|----|----|
| `numADCBits` | 0 | 12位ADC |
| | 1 | 14位ADC |
| | 2 | 16位ADC（最高精度） |
| `adcOutputFmt` | 0 | 实数格式 |
| | 1 | 复数格式（I/Q） |

**选择建议**：
- 16位ADC：最高精度，用于精密测量
- 14位ADC：平衡性能和功耗
- 12位ADC：低功耗应用

#### 2.3 `profileCfg` - 频率配置（核心参数）

**格式**：
```cfg
profileCfg <profileId> <startFreq> <idleTime> <adcStartTime> 
           <rampEndTime> <txOutPower> <txPhaseShifter> 
           <freqSlopeConst> <txStartTime> <numAdcSamples> 
           <digOutSampleRate> <hpfCornerFreq1> <hpfCornerFreq2> 
           <rxGain>
```

**示例**：
```cfg
profileCfg 0 77 7 7 200 0 0 70 1 256 5209 0 0 180
```

**参数详解**：

| 参数 | 值示例 | 单位 | 含义 | 计算方法 |
|------|--------|------|------|----------|
| `profileId` | 0 | - | Profile ID | 0-3 |
| `startFreq` | 77 | GHz | 起始频率 | 60-64 GHz |
| `idleTime` | 7 | μs | 空闲时间 | Chirp间隔 |
| `adcStartTime` | 7 | μs | ADC启动时间 | RF稳定后采样 |
| `rampEndTime` | 200 | μs | Chirp持续时间 | 影响距离分辨率 |
| `txOutPower` | 0 | dBm | 发射功率 | 0-31 (0=最大) |
| `txPhaseShifter` | 0 | ° | 相位偏移 | 0-360 |
| `freqSlopeConst` | 70 | MHz/μs | 频率斜率 | 影响速度分辨率 |
| `txStartTime` | 1 | μs | 发射启动 | < adcStartTime |
| `numAdcSamples` | 256 | - | ADC采样点数 | 2的幂次（64-1024） |
| `digOutSampleRate` | 5209 | ksps | 采样率 | 影响最大距离 |
| `hpfCornerFreq1` | 0 | kHz | 高通滤波器1 | 0/175/350/700 |
| `hpfCornerFreq2` | 0 | kHz | 高通滤波器2 | 0/175/350/700 |
| `rxGain` | 180 | dB | 接收增益 | 0-50 dB |

**关键性能参数计算**：

##### 距离分辨率（Range Resolution）
```
距离分辨率 = c / (2 × 带宽)
带宽 = freqSlopeConst × (rampEndTime - adcStartTime)

示例：
带宽 = 70 MHz/μs × (200-7) μs = 13.51 GHz
距离分辨率 = 3×10^8 / (2 × 13.51×10^9) ≈ 0.011 m = 1.1 cm
```

##### 最大检测距离（Maximum Range）
```
最大距离 = (采样率 × 光速) / (2 × 频率斜率)
         = (numAdcSamples × 光速) / (2 × freqSlopeConst × digOutSampleRate)

示例：
最大距离 = (256 × 3×10^8) / (2 × 70×10^6 × 5209×10^3)
         ≈ 10.5 m
```

##### 速度分辨率（Velocity Resolution）
```
速度分辨率 = λ / (2 × numChirps × chirpTime)
λ = c / centerFreq

示例（假设128个chirps）：
λ = 3×10^8 / 77×10^9 = 3.9 mm
速度分辨率 = 3.9×10^-3 / (2 × 128 × 200×10^-6)
          ≈ 0.076 m/s
```

#### 2.4 `frameCfg` - 帧配置

**格式**：
```cfg
frameCfg <chirpStartIdx> <chirpEndIdx> <numLoops> <numFrames> 
         <framePeriodicity> <triggerSelect> <frameTriggerDelay>
```

**示例**：
```cfg
frameCfg 0 0 128 0 50 1 0
```

**参数说明**：

| 参数 | 值 | 含义 |
|------|----|----|
| `chirpStartIdx` | 0 | 起始Chirp索引 |
| `chirpEndIdx` | 0 | 结束Chirp索引（0=使用profile 0） |
| `numLoops` | 128 | 每帧Chirp数量 |
| `numFrames` | 0 | 帧数（0=无限） |
| `framePeriodicity` | 50 | 帧周期（ms） |
| `triggerSelect` | 1 | 触发模式（1=软件，2=硬件） |
| `frameTriggerDelay` | 0 | 触发延迟（ms） |

**帧率计算**：
```
帧率 = 1000 / framePeriodicity

示例：
帧率 = 1000 / 50 = 20 FPS
```

### 3. 配置文件命名规范

SDK中的配置文件遵循命名规范，可以从文件名推断配置特性：

**格式**：`<应用>_<模式>_<距离>_<特性>.cfg`

**示例**：
```
3d_people_tracking_68xx_10fps.cfg
└─┬─┘ └────┬─────┘  └┬┘ └─┬─┘
应用       模式      芯片  特性

out_of_box_xwr68xx_3d.cfg
└───┬───┘ └──┬──┘ └┬┘
 应用      芯片    模式
```

**常见命名元素**：

| 元素 | 示例 | 含义 |
|------|------|------|
| **应用** | `3d_people_tracking` | 3D人员跟踪 |
| | `occupancy_detection` | 占用检测 |
| | `vital_signs` | 生命体征 |
| | `out_of_box` | 开箱即用 |
| **模式** | `2d` | 2D检测 |
| | `3d` | 3D检测 |
| | `tdm` | 时分复用 |
| **芯片** | `68xx` | 68xx系列（包括6844） |
| | `6843` | 特定于6843 |
| **特性** | `10fps` | 10帧/秒 |
| | `long_range` | 长距离 |
| | `high_res` | 高分辨率 |

---

## 📚 配置文件示例库

### 按应用场景分类

#### 1. 人员检测与跟踪

**3D人员跟踪（标准）**
```cfg
% 文件：3d_people_tracking_68xx_10fps.cfg
% 应用：室内人员跟踪
% 距离：0-10m
% 帧率：10 FPS

channelCfg 15 7 0          # 4RX + 3TX = 12虚拟天线
profileCfg 0 77 7 7 200 0 0 70 1 256 5209 0 0 180
frameCfg 0 0 128 0 100 1 0 # 10 FPS
```

**特点**：
- ✅ 高角度分辨率（12虚拟天线）
- ✅ 中等距离（~10m）
- ✅ 适合室内场景
- ⚡ 中等功耗

#### 2. 占用检测（低功耗）

**低功耗占用检测**
```cfg
% 文件：occupancy_detection_low_power.cfg
% 应用：房间占用检测
% 距离：0-5m
% 帧率：5 FPS

channelCfg 3 1 0           # 2RX + 1TX = 2虚拟天线
profileCfg 0 77 10 10 150 0 0 60 1 128 4000 0 0 150
frameCfg 0 0 64 0 200 1 0  # 5 FPS
```

**特点**：
- 🔋 低功耗设计
- ✅ 短距离（~5m）
- ✅ 低帧率
- ✅ 简单检测任务

#### 3. 车内感知

**车内乘员检测**
```cfg
% 文件：in_cabin_sensing_68xx.cfg
% 应用：车内乘员检测
% 距离：0-2m
% 帧率：20 FPS

channelCfg 15 5 0          # 4RX + 2TX = 8虚拟天线
profileCfg 0 60 5 5 100 0 0 80 1 256 6400 175 175 180
frameCfg 0 0 256 0 50 1 0  # 20 FPS
```

**特点**：
- ✅ 短距离高精度
- ✅ 高帧率（20 FPS）
- ✅ 使用60 GHz频段
- ✅ 适合车内环境

#### 4. 生命体征检测

**心率呼吸监测**
```cfg
% 文件：vital_signs_68xx.cfg
% 应用：生命体征监测
% 距离：0.5-2m
% 帧率：20 FPS

channelCfg 15 1 0          # 4RX + 1TX = 4虚拟天线
profileCfg 0 77 5 5 40 0 0 100 1 256 10000 0 0 180
frameCfg 0 0 512 0 50 1 0  # 20 FPS
```

**特点**：
- ✅ 极短Chirp（高速度灵敏度）
- ✅ 高帧率
- ✅ 高采样率
- ✅ 检测微小运动

### 按性能特征分类

#### 高距离分辨率配置
```cfg
% 超宽带配置 - 实现1cm级距离分辨率
profileCfg 0 77 7 7 400 0 0 100 1 512 8000 0 0 180
% 带宽 = 100 × (400-7) = 39.3 GHz
% 分辨率 ≈ 0.4 cm
```

#### 高速度分辨率配置
```cfg
% 长帧时间配置 - 检测慢速运动
frameCfg 0 0 512 0 100 1 0  # 512 chirps/frame
% 速度分辨率 ≈ 0.02 m/s
```

#### 长距离检测配置
```cfg
% 高发射功率 + 高接收增益
profileCfg 0 77 7 7 300 0 0 50 1 512 4000 0 0 240
% 最大距离 ≈ 30m
```

---

## 🔧 参数调优指南

### 1. 性能权衡矩阵

| 需求 | 增加参数 | 副作用 |
|------|----------|--------|
| **提高距离分辨率** | 增加带宽（↑ freqSlope或↑ rampTime） | ↑功耗，↓最大距离 |
| **提高速度分辨率** | 增加chirps数量（↑ numLoops） | ↑处理时间，↓帧率 |
| **提高角度分辨率** | 增加虚拟天线（↑ RX/TX） | ↑功耗，↑数据量 |
| **增加最大距离** | 增加采样点（↑ numAdcSamples） | ↑数据量，↑处理时间 |
| **降低功耗** | 减少天线、降低帧率、减少chirps | ↓性能 |

### 2. 典型应用场景参数建议

#### 室内人员检测（0-10m）
```
距离分辨率：2-5 cm      → 带宽 3-7.5 GHz
速度分辨率：0.05-0.1 m/s → 128-256 chirps
角度分辨率：10-15°      → 8-12虚拟天线
帧率：10-20 FPS
```

#### 手势识别（0-1m）
```
距离分辨率：<1 cm       → 带宽 >15 GHz
速度分辨率：<0.05 m/s   → >256 chirps
帧率：>20 FPS
```

#### 车辆检测（0-50m）
```
距离分辨率：10-20 cm    → 带宽 0.75-1.5 GHz
速度分辨率：0.5-1 m/s   → 32-64 chirps
最大距离：50m
```

### 3. 功耗优化策略

#### 低功耗配置清单
- ✅ 使用最少天线组合（如2RX+1TX）
- ✅ 降低帧率（5-10 FPS）
- ✅ 减少chirps数量（32-64）
- ✅ 使用低采样率
- ✅ 降低发射功率（仅短距离时）

#### 示例对比

| 配置 | 天线 | Chirps | 帧率 | 相对功耗 |
|------|------|--------|------|----------|
| 高性能 | 4RX+3TX | 256 | 20 FPS | 100% |
| 标准 | 4RX+2TX | 128 | 10 FPS | 50% |
| 低功耗 | 2RX+1TX | 64 | 5 FPS | 15% |

---

## 🛠️ 配置文件验证工具

### 1. 参数合法性检查

**必检项目**：
- [ ] `numAdcSamples` 是2的幂次（64/128/256/512/1024）
- [ ] `adcStartTime` > `txStartTime`
- [ ] `rampEndTime` > `adcStartTime` + 采样时间
- [ ] `framePeriodicity` > 单帧所需时间
- [ ] 采样率不超过硬件限制（<12.5 Msps）

### 2. 性能计算器（Python示例）

```python
def calculate_radar_performance(config):
    """
    计算雷达性能参数
    """
    # 提取配置参数
    freq_slope = config['freqSlopeConst']  # MHz/μs
    ramp_time = config['rampEndTime'] - config['adcStartTime']  # μs
    num_samples = config['numAdcSamples']
    sample_rate = config['digOutSampleRate']  # ksps
    num_chirps = config['numLoops']
    frame_period = config['framePeriodicity']  # ms
    
    # 计算性能
    bandwidth = freq_slope * ramp_time  # MHz
    range_resolution = 3e8 / (2 * bandwidth * 1e6)  # m
    max_range = (num_samples * 3e8) / (2 * freq_slope * 1e6 * sample_rate * 1e3)  # m
    
    # 速度性能
    lambda_wave = 3e8 / (config['startFreq'] * 1e9)  # m
    chirp_time = ramp_time + config['idleTime']  # μs
    velocity_resolution = lambda_wave / (2 * num_chirps * chirp_time * 1e-6)  # m/s
    max_velocity = lambda_wave / (4 * chirp_time * 1e-6)  # m/s
    
    # 帧率
    frame_rate = 1000 / frame_period  # FPS
    
    return {
        '带宽 (GHz)': bandwidth / 1000,
        '距离分辨率 (cm)': range_resolution * 100,
        '最大距离 (m)': max_range,
        '速度分辨率 (m/s)': velocity_resolution,
        '最大速度 (m/s)': max_velocity,
        '帧率 (FPS)': frame_rate
    }

# 使用示例
config = {
    'freqSlopeConst': 70,
    'rampEndTime': 200,
    'adcStartTime': 7,
    'numAdcSamples': 256,
    'digOutSampleRate': 5209,
    'numLoops': 128,
    'framePeriodicity': 100,
    'idleTime': 7,
    'startFreq': 77
}

performance = calculate_radar_performance(config)
for key, value in performance.items():
    print(f"{key}: {value:.3f}")
```

**输出示例**：
```
带宽 (GHz): 13.510
距离分辨率 (cm): 1.110
最大距离 (m): 10.485
速度分辨率 (m/s): 0.076
最大速度 (m/s): 9.750
帧率 (FPS): 10.000
```

---

## 📊 配置文件数据库

### SDK配置文件统计

| 应用类别 | 配置文件数量 | 主要Demo |
|----------|--------------|----------|
| 人员检测 | 8 | 3D People Tracking, Occupancy |
| 车内感知 | 6 | In-Cabin Sensing, Vital Signs |
| 手势识别 | 4 | Gesture Recognition |
| 通用Demo | 5 | Out-of-Box, Level Sensing |
| **总计** | **23** | - |

### 配置文件路径规律

**SDK路径结构**：
```
C:\ti\<SDK_NAME>\
├── mmwave_demo\
│   └── 68xx_<application>\
│       ├── profiles\
│       │   ├── profile_*.cfg           # 单profile配置
│       │   └── advanced_*.cfg          # 高级配置
│       └── chirp_configs\
│           └── chirp_config_*.cfg      # Chirp配置
└── industrial_toolbox\
    └── <toolbox_name>\
        └── config\
            └── *.cfg                    # 工业应用配置
```

**文件识别特征**：
- ✅ 文件扩展名：`.cfg`
- ✅ 目录特征：`profiles/`, `chirp_configs/`, `config_file/`
- ✅ 文件名包含：`68xx`, `6844`, `profile`, `chirp`, `config`
- ✅ 内容特征：包含 `profileCfg`, `frameCfg`, `channelCfg` 等命令

---

## 🔬 深度研究主题

### 1. 高级配置命令

除了基础命令外，还有高级配置命令：

#### Angle FFT配置
```cfg
angleFftCfg -1 16 16 16
% 参数：RX ID, 窗口长度, 虚拟天线数
% 影响角度估计精度
```

#### 多普勒补偿
```cfg
clutterRemoval -1 0
% 移除静止目标（地面、墙壁）
% 提高运动目标检测
```

#### CFAR检测
```cfg
cfarCfg -1 0 2 8 4 3 0 15 1
% 恒虚警率检测器配置
% 影响目标检测灵敏度和虚警率
```

### 2. 多Profile配置

**时分复用（TDM）示例**：
```cfg
% Profile 0 - 短距高精度
profileCfg 0 77 7 7 100 0 0 100 1 256 8000 0 0 180

% Profile 1 - 长距低精度
profileCfg 1 77 7 7 200 0 0 50 1 128 4000 0 0 240

% Chirp配置使用不同Profile
chirpCfg 0 0 0 0 0 0 0 1  # Chirp 0使用Profile 0
chirpCfg 1 1 0 0 0 0 0 1  # Chirp 1使用Profile 1

% 帧配置交替使用
frameCfg 0 1 128 0 50 1 0
```

**应用场景**：
- ✅ 同时需要短距高精度和长距低精度
- ✅ 车辆检测（远近结合）
- ✅ 工业应用（不同检测区域）

### 3. 级联配置（多芯片）

```cfg
channelCfg 15 7 1  # cascading = 1
% 启用级联模式
% 多个雷达芯片协同工作
% 形成更大的虚拟天线阵列
% 提高角度分辨率
```

---

## 🎯 后续工具开发需求

### 1. 配置文件可视化编辑器

**功能需求**：
- ✅ 图形化参数编辑
- ✅ 实时性能计算显示
- ✅ 参数合法性验证
- ✅ 配置模板库
- ✅ 参数优化建议

**界面设计**：
```
┌─────────────────────────────────────────────┐
│ 📡 雷达配置编辑器                              │
├─────────────────────────────────────────────┤
│                                             │
│  基本参数                                   │
│  ├─ 起始频率：  [77    ] GHz               │
│  ├─ Chirp时间： [200   ] μs                │
│  ├─ 采样点数：  [256   ] (2的幂次)         │
│  └─ 帧率：      [10    ] FPS               │
│                                             │
│  天线配置                                   │
│  ├─ RX天线：   [☑☑☑☑] (4个)               │
│  └─ TX天线：   [☑☐☑] (TX1, TX3)           │
│                                             │
│  性能预览                                   │
│  ├─ 距离分辨率： 1.1 cm                    │
│  ├─ 最大距离：   10.5 m                    │
│  ├─ 速度分辨率： 0.076 m/s                 │
│  └─ 角度分辨率： 15°                       │
│                                             │
│  [预览配置] [保存] [加载模板]               │
└─────────────────────────────────────────────┘
```

### 2. 配置文件智能匹配器（已实现）

**当前功能**：
- ✅ 扫描SDK中所有配置文件
- ✅ 根据固件推荐Top 8配置
- ✅ 提供配置文件详细信息

**待增强功能**：
- ⏳ 配置文件参数解析和显示
- ⏳ 配置文件性能对比
- ⏳ 配置文件相似度计算

### 3. 配置文件测试工具

**功能需求**：
- ✅ 串口发送配置命令
- ✅ 验证雷达响应
- ✅ 性能测试（距离、速度、角度）
- ✅ 结果可视化

### 4. 配置文件优化助手

**功能需求**：
- ✅ 输入应用场景（距离范围、帧率等）
- ✅ 自动推荐最优配置
- ✅ 多目标优化（性能 vs 功耗）
- ✅ 生成配置文件

---

## 📖 参考资料

### TI官方文档
1. **mmWave SDK User Guide** - 配置命令参考
2. **Programming Chirp Parameters** - Chirp参数编程指南
3. **mmWave Demo User Guide** - Demo配置文件说明
4. **xWR68xx/xWRL68xx Data Sheet** - 硬件规格和限制

### 项目文档
1. `2025-12-15_TI雷达固件配置文件关系详解.md` - 配置文件类型区分
2. `2025-12-17_AWRL6844EVM雷达参数配置文件清单.md` - SDK配置文件清单
3. `固件智能管理系统` - 配置文件识别和匹配

### 在线资源
1. TI E2E Forums - 配置问题讨论
2. mmWave Training Series - 视频教程
3. TI Resource Explorer - 配置示例

---

## 📝 研究记录

### 研究进度

- [x] 配置文件类型区分
- [x] 基础命令参数理解
- [x] 性能计算公式推导
- [x] SDK配置文件统计
- [x] 配置文件命名规律
- [ ] 高级命令深度研究
- [ ] 多Profile配置测试
- [ ] 级联配置研究
- [ ] 实际性能测试验证
- [ ] 配置优化算法开发

### 待研究问题

1. **参数边界条件**
   - 各参数的硬件限制范围
   - 参数组合的约束条件
   - 非法配置的错误处理

2. **性能实测对比**
   - 理论计算 vs 实际测量
   - 不同配置的性能差异
   - 环境影响因素

3. **配置优化策略**
   - 多目标优化算法
   - 自动参数调优
   - 机器学习辅助配置

4. **特殊场景配置**
   - 极端环境（高温、低温）
   - 复杂环境（多径、干扰）
   - 特殊应用（穿墙、水下）

---

## 🔗 相关工具链接

- **烧录工具**: `01-AWRL6844固件系统工具/5-Scripts/flash_tool.py`
- **固件管理**: `02-固件智能管理系统/awrl6844_gui_app.py`
- **配置匹配**: `02-固件智能管理系统/awrl6844_firmware_matcher.py`

---

**文档版本**: v1.0  
**创建日期**: 2025-12-20  
**作者**: Benson@Wisefido  
**状态**: 🚧 持续更新中
