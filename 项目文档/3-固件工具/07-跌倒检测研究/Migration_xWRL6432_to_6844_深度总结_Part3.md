# Migration深度总结 - 第三部分：时序配置、数据格式与相位旋转

## 第五部分（续）：SDK CLI配置迁移

### 5.4 Chirp时序配置

#### 命令格式

```bash
chirpTimingCfg <idle_time> <adc_start_time> <ramp_end_time> 
               <tx_start_time> <chirp_start_freq>
```

#### 参数对比

| 参数 | xWRL6432 | xWRL6844 | 说明 |
|------|----------|----------|------|
| **idle_time** | 7 | 7 | Chirp之间的空闲时间（μs） |
| **adc_start_time** | 28 | 26 | ADC开始采样时间（μs） |
| **ramp_end_time** | 0 | 0 | Ramp结束时间偏移 |
| **tx_start_time** | 60.0 | 60.0 | TX开始时间（μs） |
| **chirp_start_freq** | 58.1 | 58.1 | Chirp起始频率（GHz） |

**示例**：
```bash
# xWRL6432
chirpTimingCfg 7 28 0 60.0 58.1

# xWRL6844  
chirpTimingCfg 7 26 0 60.0 58.1
```

**⚠️ 注意**：adc_start_time可能需要微调，但变化很小。

### 5.5 帧配置

#### 命令格式

```bash
frameCfg <numChirps> <chirpStartIdx> <chirpEndIdx> 
         <numLoops> <framePeriodicity> <triggerSelect>
```

#### 关键变化：Burst空闲时间

```
🔴 重要变更：
xWRL6844的Burst空闲时间增加：
├─ 原因：RF LDO稳定时间增加
├─ 最小增加：131μs
└─ 影响：帧周期略微增加
```

#### 参数对比

| 参数 | xWRL6432 | xWRL6844 | 变化说明 |
|------|----------|----------|---------|
| **numChirps** | 2 | 4 | ⚠️ 取决于TDMA/BPM配置 |
| **chirpStartIdx** | 0 | 0 | 无变化 |
| **chirpEndIdx** | 600 | 600 | 无变化 |
| **numLoops** | 16 | 16 | 无变化 |
| **framePeriodicity** | 250 | 250 | 可能需要微调 |
| **triggerSelect** | 0 | 0 | 无变化 |

**示例**：
```bash
# xWRL6432（2TX TDMA）
frameCfg 2 0 600 16 250 0
         ↑
    2个chirp/burst (TX0, TX1)

# xWRL6844（4TX TDMA）
frameCfg 4 0 600 16 250 0
         ↑
    4个chirp/burst (TX0, TX1, TX2, TX3)
```

**⚠️ 帧周期计算**：
```
帧周期必须考虑：
├─ Chirp数量增加（2→4）
├─ Burst空闲时间增加（+131μs）
└─ 可能需要从250ms调整到260ms

验证方法：
├─ 实际测量帧率
└─ 检查系统日志中的timing warning
```

### 5.6 BPM/TDM配置

#### xWRL6432配置

```bash
# chirpComnCfg的chirpTxMimoPatSel参数
chirpComnCfg 40 0 0 128 1 63.0 1
                          ↑
                    1 = TDMA mode

chirpComnCfg 40 0 0 128 4 63.0 1
                          ↑
                    4 = BPM mode
```

#### xWRL6844配置

```bash
# 相同的配置方式，SDK内部自动处理Per-Chirp LUT
chirpComnCfg 80 0 0 128 1 63.0 1  # TDMA
chirpComnCfg 80 0 0 128 4 63.0 1  # BPM
```

**✅ 迁移说明**：
```
使用SDK CLI时：
├─ 配置方式保持不变
├─ SDK自动生成Per-Chirp LUT
├─ 无需手动配置LUT
└─ 相位设计自动处理
```

### 5.7 ADC数据采集（DCA1000）

#### 数据采集接口变化

| 特性 | xWRL6432 | xWRL6844 | 说明 |
|------|----------|----------|------|
| **接口类型** | RDIF | LVDS | ⚠️ 硬件接口变化 |
| **数据位宽** | 12-bit | 12/14/16-bit | ✅ 可配置 |
| **带宽** | 较低 | 更高 | ✅ 支持更高采样率 |

#### CLI配置

```bash
adcLogging <mode>

# xWRL6432（RDIF）
adcLogging 0

# xWRL6844（LVDS）
adcLogging 0
```

**⚠️ 迁移注意事项**：
```
如果使用DCA1000采集ADC数据：
├─ DCA1000硬件连接需要调整
├─ LVDS配置需要更新
├─ 数据解析代码需要修改
└─ 参考第5.8节数据格式变化
```

### 5.8 ADC数据格式变化 🔴 重要

#### xWRL6432数据格式

```
数据排列：复数交织格式（IQ交织）
[I0 Q0 I1 Q1 I2 Q2 ... IN QN] × M chirps

特点：
├─ 每个采样点：I和Q相邻
├─ 按时间顺序排列
└─ 解析相对复杂
```

#### xWRL6844数据格式 ⭐ 回归xWR6843格式

```
数据排列：实数格式（I和Q分开）
[I0 I1 I2 ... IN] [Q0 Q1 Q2 ... QN] × M chirps

特点：
├─ 所有I数据连续
├─ 所有Q数据连续
├─ 与xWR6843相同
└─ 解析更简单
```

#### 数据格式图解

**xWRL6432格式**：
```
Chirp 0: |I0|Q0|I1|Q1|...|IN|QN|
Chirp 1: |I0|Q0|I1|Q1|...|IN|QN|
...
Chirp M: |I0|Q0|I1|Q1|...|IN|QN|
```

**xWRL6844格式**：
```
Chirp 0: |I0|I1|I2|...|IN|Q0|Q1|Q2|...|QN|
Chirp 1: |I0|I1|I2|...|IN|Q0|Q1|Q2|...|QN|
...
Chirp M: |I0|I1|I2|...|IN|Q0|Q1|Q2|...|QN|
```

#### MATLAB解析代码示例

**xWRL6844数据解析伪代码**：
```matlab
% 假设：
% N = 每个chirp的ADC采样数
% M = chirp总数
% data = 原始ADC数据（1D数组）

% 重组数据
numSamplesPerChirp = N * 2;  % I和Q各N个
reshapedData = reshape(data, numSamplesPerChirp, M);

% 分离I和Q
I_data = reshapedData(1:N, :);      % 前N个是I
Q_data = reshapedData(N+1:end, :);  % 后N个是Q

% 构建复数
complexData = complex(I_data, Q_data);

% 现在可以进行FFT等处理
rangeFFT = fft(complexData, N, 1);
```

**⚠️ 迁移要点**：
```
如果现有代码处理6432数据：
🔴 必须修改数据解析逻辑
🔴 I/Q分离方式完全不同
🔴 参考xWR6843的数据处理代码

✅ 好处：
├─ 与xWR6843兼容，可复用代码
├─ 数据处理更简单
└─ 内存访问更高效
```

---

## 第六部分：内置相位旋转 ⭐ 关键特性

### 6.1 相位旋转概述

#### 为什么需要相位校正？

```
xWRL6844的物理设计引入了相位差：
├─ 芯片级别的相位差
├─ 天线馈电级别的相位差
└─ 需要软件算法补偿
```

#### 三个层级的相位旋转

```
1. 设备级（Device Level）
   └─ 芯片内部固有相位差

2. 天线馈电级（Antenna Feed Level）
   └─ PCB走线和天线设计引入

3. 整体板级（Overall Board Level）
   └─ 1和2的综合结果
```

### 6.2 设备级相位旋转

#### RX通道相位

```
RX相位模式：[+ + - -]

解释：
├─ RX0A, RX1A: 同相（+）
├─ RX2A, RX3A: 同相（+）
└─ 但两组之间反相（180°差异）

数学表示：
RX_phase = [1, 1, -1, -1]
```

#### TX通道相位（设备级）

```
TX相位模式：[+ - - +]

解释：
├─ TX0AB, TX3AB: 同相（+）
├─ TX1AB, TX2AB: 同相（+）
└─ 但两组之间反相

数学表示：
TX_phase_device = [1, -1, -1, 1]
```

### 6.3 天线馈电级相位旋转

#### TX天线馈电

```
TX相位模式：[+ + - -]

解释：
├─ TX0AB, TX1AB: 同相（+）
├─ TX2AB, TX3AB: 同相（+）
└─ 但两组之间反相

数学表示：
TX_phase_antenna = [1, 1, -1, -1]
```

#### RX天线馈电

```
RX相位模式：[+ + - -]

数学表示：
RX_phase_antenna = [1, 1, -1, -1]
```

### 6.4 整体板级相位旋转 🔴 最终结果

#### RX通道最终相位

```
RX总相位 = 设备级 × 天线级
        = [+, +, -, -] × [+, +, -, -]
        = [+, +, +, +]

结果：所有4个RX通道同相！✅

数学表示：
RX_phase_total = [1, 1, 1, 1]
```

#### TX通道最终相位

```
TX总相位 = 设备级 × 天线级
        = [+, -, -, +] × [+, +, -, -]
        = [+, -, +, -]

结果：TX0/TX2同相，TX1/TX3同相，两组反相

数学表示：
TX_phase_total = [1, -1, 1, -1]
```

### 6.5 虚拟天线阵列相位校正

#### 完整的相位旋转模式

```
4TX × 4RX = 16个虚拟天线

相位校正向量（按虚拟天线编号）：
Phase_correction = [
    1,  1,  1,  1,   // TX0 × [RX0, RX1, RX2, RX3]
   -1, -1, -1, -1,   // TX1 × [RX0, RX1, RX2, RX3]  ← 反相
    1,  1,  1,  1,   // TX2 × [RX0, RX1, RX2, RX3]
   -1, -1, -1, -1    // TX3 × [RX0, RX1, RX2, RX3]  ← 反相
]
```

#### 代码实现

**C代码**：
```c
// 相位校正系数
const float phaseCorrection[16] = {
    1.0f,  1.0f,  1.0f,  1.0f,   // TX0
   -1.0f, -1.0f, -1.0f, -1.0f,   // TX1
    1.0f,  1.0f,  1.0f,  1.0f,   // TX2
   -1.0f, -1.0f, -1.0f, -1.0f    // TX3
};

// 应用相位校正
for (int vx = 0; vx < 16; vx++) {
    // 对每个虚拟天线的数据
    complexData[vx] *= phaseCorrection[vx];
}
```

**MATLAB代码**：
```matlab
% 相位校正向量
phaseCorrection = [1, 1, 1, 1, -1, -1, -1, -1, 1, 1, 1, 1, -1, -1, -1, -1];

% 应用到Range-Doppler数据（假设数据已经过Range FFT）
% rangeData: [numRangeBins × 16虚拟天线]
correctedData = rangeData .* phaseCorrection;

% 或者应用到Radar Cube
% radarCube: [numSamples × 16虚拟天线 × numChirps]
for chirpIdx = 1:numChirps
    radarCube(:, :, chirpIdx) = radarCube(:, :, chirpIdx) .* phaseCorrection;
end
```

#### Python代码：
```python
import numpy as np

# 相位校正向量
phase_correction = np.array([
    1, 1, 1, 1, -1, -1, -1, -1, 1, 1, 1, 1, -1, -1, -1, -1
])

# 应用相位校正
# radar_cube: shape = (num_samples, 16, num_chirps)
corrected_cube = radar_cube * phase_correction[np.newaxis, :, np.newaxis]
```

### 6.6 相位校正的时机

#### 在数据处理流程中的位置

```
ADC数据采集
    ↓
Range FFT
    ↓
【🎯 在这里应用相位校正】← 推荐
    ↓
Doppler FFT
    ↓
Angle FFT (Azimuth/Elevation)
    ↓
CFAR检测
    ↓
点云输出
```

**为什么在Range FFT后应用？**
```
✅ 优点：
├─ 只对有效数据校正（节省计算）
├─ 在频域操作更高效
├─ 不影响Range处理
└─ 为Doppler和Angle处理准备正确数据

❌ 不推荐在ADC原始数据阶段：
├─ 增加数据量（每个采样点都要处理）
├─ 时域校正更复杂
└─ 影响Range FFT性能
```

### 6.7 相位校正验证方法

#### 验证步骤

**1. 静态目标测试**：
```
测试设置：
├─ 放置一个强反射目标（角反射器）
├─ 距离：3-5m
├─ 方位角：0° (正前方)
└─ 记录所有虚拟天线数据

预期结果：
├─ 未校正：TX1和TX3的虚拟天线相位反转
├─ 校正后：所有虚拟天线相位一致
└─ Angle FFT后：角度估计准确
```

**2. 检查相位连续性**：
```matlab
% 计算虚拟天线间的相位差
for vx = 1:15
    phaseDiff(vx) = angle(rangeData(targetBin, vx+1) / rangeData(targetBin, vx));
end

% 未校正时：
% 在虚拟天线4,8,12处会出现180°跳变

% 校正后：
% 相位差应该平滑变化（取决于目标角度）
```

**3. DOA估计验证**：
```python
# 对已知角度的目标进行DOA估计
angle_estimated = calculate_doa(corrected_data, target_bin)
angle_error = abs(angle_estimated - angle_true)

# 校正后误差应该 < 5°
assert angle_error < 5.0, "相位校正可能有问题"
```

### 6.8 常见问题

#### Q1: 为什么TX1和TX3需要反相？

```
A: 这是xWRL6844 EVM板的物理设计结果

设备级相位：[+, -, -, +]
天线馈电：  [+, +, -, -]
────────────────────────
总相位：    [+, -, +, -]

TX0和TX2: +（同相）
TX1和TX3: -（反相180°）
```

#### Q2: 忘记校正会怎样？

```
❌ 后果：
├─ Angle FFT结果错误
├─ 角度估计偏差 >10°
├─ 虚拟天线阵列增益下降
├─ 旁瓣抑制能力下降
└─ 多目标分辨能力降低
```

#### Q3: 需要针对每个芯片校准吗？

```
✅ 不需要！

相位模式是：
├─ 芯片设计决定的
├─ EVM板设计决定的
└─ 对所有xWRL6844 EVM板相同

✅ 使用固定的相位校正向量即可
```

#### Q4: 与xWRL6432的差异？

```
xWRL6432:
├─ 2TX × 3RX = 6虚拟天线
├─ 也有相位旋转，但模式不同
└─ 参考xWRL6432 TRM文档

xWRL6844:
├─ 4TX × 4RX = 16虚拟天线
├─ 相位模式：[1,1,1,1,-1,-1,-1,-1,1,1,1,1,-1,-1,-1,-1]
└─ 必须在算法中应用校正
```

---

## 第七部分：迁移检查清单

### 7.1 硬件层面

```
硬件检查清单：
□ 确认使用xWRL6844 EVM板
□ 确认天线已正确安装
□ 确认电源供应（5V/2A）
□ 确认USB连接（数据+调试）
□ 确认SOP开关设置正确
□ 如使用DCA1000，确认LVDS连接
```

### 7.2 固件层面

```
固件迁移检查清单：
□ 更新链接脚本（内存大小）
□ 更新MCU配置（M4F→R5F）
□ 更新通道配置（TX 2→4, RX 3→4）
□ 更新通道掩码（使用8位格式）
□ 更新采样率参数（digOutputSampRate翻倍）
□ 更新校准API（使用新的Factory Cal API）
□ 添加相位校正代码
□ 更新帧配置（chirp数量）
□ 如使用DCA1000，更新数据接口（RDIF→LVDS）
```

### 7.3 算法层面

```
算法迁移检查清单：
□ 更新虚拟天线数量（6→16）
□ 应用相位校正向量
□ 更新Angle FFT（适配16虚拟天线）
□ 更新ADC数据解析（新格式）
□ 验证Range-Doppler-Angle处理
□ 调整CFAR检测阈值
□ 验证角度估计精度
```

### 7.4 测试验证

```
测试验证清单：
□ 静态目标检测
□ 动态目标跟踪
□ 多目标场景
□ 角度估计精度
□ 距离精度
□ 速度精度
□ 功耗测试
□ 长时间稳定性测试
```

---

## 第八部分：快速参考

### 8.1 关键差异速查表

| 项目 | xWRL6432 | xWRL6844 | 迁移动作 |
|------|----------|----------|---------|
| MCU | M4F/160MHz | R5F/200MHz | 🔴 重新编译 |
| TX通道 | 2 | 4 | ⚠️ 更新配置 |
| RX通道 | 3 | 4 | ⚠️ 更新配置 |
| 内存 | 1MB | 2.5MB | ⚠️ 更新链接脚本 |
| 采样率参数 | 值 | 值×2 | ⚠️ 更新CLI |
| 校准API | 旧API | 新API | 🔴 修改代码 |
| TDMA/BPM | Profile API | Per-Chirp LUT | ⚠️ 使用SDK CLI |
| ADC接口 | RDIF | LVDS | 🔴 DCA1000用户需修改 |
| 数据格式 | IQ交织 | I/Q分开 | 🔴 修改解析代码 |
| 相位校正 | 6432模式 | 6844模式 | 🔴 应用新向量 |

### 8.2 CLI配置快速对比

```bash
# ============ channelCfg ============
6432: channelCfg 7 3 0
6844: channelCfg 15 15 0

# ============ chirpComnCfg ============
6432: chirpComnCfg 40 0 0 128 4 63.0 1
6844: chirpComnCfg 80 0 0 128 4 63.0 1
                   ↑↑ 翻倍

# ============ frameCfg ============
6432: frameCfg 2 0 600 16 250 0
6844: frameCfg 4 0 600 16 250 0
               ↑ chirp数量

# ============ 其他配置 ============
chirpTimingCfg, lowPowerCfg: 基本相同
```

### 8.3 相位校正代码模板

```c
// C代码模板
const float PHASE_CORRECTION_6844[16] = {
    1.0f,  1.0f,  1.0f,  1.0f,   // TX0 × RX[0-3]
   -1.0f, -1.0f, -1.0f, -1.0f,   // TX1 × RX[0-3]
    1.0f,  1.0f,  1.0f,  1.0f,   // TX2 × RX[0-3]
   -1.0f, -1.0f, -1.0f, -1.0f    // TX3 × RX[0-3]
};

void applyPhaseCorrection(cmplx16_t* rangeFFTOut, uint32_t numRangeBins) {
    for (uint32_t i = 0; i < numRangeBins; i++) {
        for (uint32_t vx = 0; vx < 16; vx++) {
            rangeFFTOut[i * 16 + vx].real *= PHASE_CORRECTION_6844[vx];
            rangeFFTOut[i * 16 + vx].imag *= PHASE_CORRECTION_6844[vx];
        }
    }
}
```

---

## 附录：参考资源

### A. 官方文档

```
必读文档：
├─ xWRL6844 Technical Reference Manual (TRM)
├─ xWRL6844 Datasheet
├─ Migration_from_xWRLx432_to_xWRL6844.pdf ← 本文档源
├─ mmWave SDK User Guide
└─ ICD (Interface Control Document)
```

### B. SDK路径

```
关键SDK路径：
C:\ti\
├─ mmwave_sdk_<version>\
│   ├─ docs\                    ← 文档
│   ├─ packages\                ← SDK库
│   └─ examples\                ← 示例代码
│
└─ radar_toolbox_<version>\
    ├─ source\ti\examples\      ← 完整Demo
    └─ software_docs\           ← 软件文档
```

### C. 关键API参考

```
mmWaveLink API：
├─ rl_fecssRfFactoryCalDataSet/Get   ← 新校准API
├─ rlRfSetPerChirpPhShifterCfg       ← Per-Chirp TX使能
├─ rlRfSetPerChirpBpmConfig          ← Per-Chirp BPM
└─ rlRfSetChirpCommonConfig          ← Chirp通用配置
```

---

## 总结

### 迁移难度评估

```
难度等级：⭐⭐⭐⭐ (中高)

容易部分：
✅ CLI配置调整（SDK自动处理大部分）
✅ 硬件连接（标准EVM板）

中等难度：
⚠️ 固件层面修改（链接脚本、内存）
⚠️ 校准API更新

困难部分：
🔴 Per-Chirp LUT配置（如果直接使用API）
🔴 数据格式解析（IQ交织→分离）
🔴 相位校正实现
```

### 时间估算

```
预计迁移时间：
├─ 使用SDK CLI: 1-2天
├─ 部分修改源码: 3-5天
└─ 完全重写固件: 1-2周

包括：
├─ 学习文档: 20%
├─ 代码修改: 40%
├─ 测试验证: 30%
└─ 问题调试: 10%
```

### 最终建议

```
🎯 推荐路径：
1. ✅ 优先使用SDK + CLI配置
2. ✅ 复用SDK提供的Per-Chirp LUT封装
3. ✅ 参考SDK example代码
4. ⚠️ 只在必要时直接使用mmWaveLink API

🚨 必须注意：
- 相位校正不可省略
- 数据格式解析必须更新
- 校准API必须使用新版本
- 内存配置必须正确
```

---

**文档完成！**

> 本文档基于TI官方Migration文档深度分析编写，涵盖了从xWRL6432迁移到xWRL6844的所有关键技术点。建议结合实际项目需求，按照检查清单逐步进行迁移和验证。

