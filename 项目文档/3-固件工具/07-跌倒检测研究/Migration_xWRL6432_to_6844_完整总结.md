# 🔄 xWRL6432到xWRL6844迁移完整指南

> **基于TI官方Migration文档深度分析** | 涵盖所有技术细节、配置示例和迁移检查清单

**文档版本**: V1.0 综合版  
**最后更新**: 2025-11-30  
**源文档**: Migration_from_xWRLx432_to_xWRL6844_V0.3 (TI官方文档)  
**作者**: Yang, Zigang (Texas Instruments)  
**总结人**: AI Assistant  

---

## 📚 文档结构

本完整指南包含以下部分：

### 第一部分：硬件与内存
- ✅ 硬件特性对比 (14项功能详细对比)
- ✅ 内存映射与分区 (1MB→2.5MB)
- ✅ MCU架构变化 (M4F→R5F)

### 第二部分：DFP变更
- ✅ IF带宽翻倍 (5→10 MHz)
- ✅ 校准API更新 (4个变化点)
- ✅ 采样率计算变化

### 第三部分：TX/RX通道配置
- ✅ TX通道：2→4，8个PA
- ✅ RX通道：3→4，特殊掩码格式
- ✅ 通道掩码详解 (8位格式)

### 第四部分：Per-Chirp LUT方法
- ✅ Per-Chirp LUT概述
- ✅ TDMA模式配置示例 (3个)
- ✅ BPM模式配置示例
- ✅ SDK CLI抽象

### 第五部分：SDK CLI配置迁移
- ✅ channelCfg命令对比
- ✅ chirpComnCfg配置 (采样率翻倍)
- ✅ chirpTimingCfg时序配置
- ✅ frameCfg帧配置 (burst空闲时间+131μs)
- ✅ ADC数据采集 (RDIF→LVDS)

### 第六部分：ADC数据格式变化
- ✅ 数据格式变化 (IQ交织→I/Q分开)
- ✅ MATLAB解析代码示例
- ✅ 与xWR6843格式兼容

### 第七部分：内置相位旋转 ⭐ 重要
- ✅ 设备级相位旋转
- ✅ 天线馈电级相位旋转
- ✅ 整体板级相位旋转
- ✅ 相位校正向量 [1,1,1,1,-1,-1,-1,-1,1,1,1,1,-1,-1,-1,-1]
- ✅ 代码实现 (C/MATLAB/Python)

### 第八部分：迁移检查清单
- ✅ 硬件检查清单
- ✅ 固件检查清单
- ✅ 算法检查清单
- ✅ 测试验证清单

---

## ⚡ 快速开始：5分钟了解核心变化

### 🔴 必须修改的项目

| 序号 | 变更项 | xWRL6432 | xWRL6844 | 影响级别 |
|------|--------|----------|----------|---------|
| 1 | **MCU** | M4F/160MHz | R5F/200MHz | 🔴 固件不兼容 |
| 2 | **TX通道** | 2 | 4 | ⚠️ CLI配置必改 |
| 3 | **RX通道** | 3 | 4 | ⚠️ CLI配置必改 |
| 4 | **TX掩码** | 2位 | 8位 | 🔴 掩码格式变化 |
| 5 | **RX掩码** | 3位 | 8位(0x99格式) | 🔴 掩码格式变化 |
| 6 | **采样率参数** | 值 | 值×2 | ⚠️ CLI参数翻倍 |
| 7 | **校准API** | 旧API | 新Factory Cal API | 🔴 代码必须修改 |
| 8 | **相位校正** | 6432模式 | 6844模式 | 🔴 算法必须添加 |
| 9 | **ADC数据格式** | IQ交织 | I/Q分开 | 🔴 解析代码必改 |
| 10 | **ADC接口** | RDIF | LVDS | 🔴 DCA1000用户 |

### ✅ 可以保持的项目

- 基本CLI命令结构（chirpTimingCfg, frameCfg等）
- Chirp配置逻辑
- 帧配置逻辑
- 低功耗配置

---

## 🎯 迁移路径建议

### 路径1：使用SDK + CLI配置（推荐） ⭐

**适合人群**：
- 使用TI SDK的开发者
- 主要通过CLI配置雷达参数
- 不需要深度定制的应用

**工作量**：1-2天

**步骤**：
1. 更新CLI配置文件（主要是参数调整）
2. 添加相位校正代码
3. 更新ADC数据解析
4. 测试验证

**优点**：
- ✅ SDK自动处理Per-Chirp LUT
- ✅ 校准API封装良好
- ✅ 示例代码丰富
- ✅ 迁移快速

### 路径2：部分修改源码

**适合人群**：
- 需要优化性能的开发者
- 有特殊功能需求
- 熟悉mmWaveLink API

**工作量**：3-5天

**步骤**：
1. 更新链接脚本（内存分配）
2. 修改MCU配置代码
3. 更新通道配置
4. 更新校准流程
5. 添加相位校正
6. 测试验证

**优点**：
- ✅ 更好的性能控制
- ✅ 可以优化内存使用
- ✅ 灵活性更高

### 路径3：完全重写固件

**适合人群**：
- 定制化需求极高
- 需要完全控制底层
- 有充足开发时间

**工作量**：1-2周

**不推荐原因**：
- ❌ 工作量大
- ❌ 容易引入bug
- ❌ 维护成本高

---

## 📋 详细内容索引

### 第一部分：硬件与内存详解

**完整内容见**：`Migration_xWRL6432_to_6844_深度总结_Part1.md`

**包含**：
- 硬件特性完整对比表 (14项功能)
- 虚拟天线阵列增强 (6→16, +167%)
- MCU性能提升 (M4F→R5F, 25%速度提升)
- 内存容量增加 (1MB→2.5MB, +150%)
- 内存映射详解
- TCM灵活分配机制
- DSP子系统新增 (C66x/450MHz)

**关键图片**：
- Hardware Feature Comparison Table
- Memory Map Comparison
- TCM Architecture

### 第二部分：DFP变更详解

**完整内容见**：`Migration_xWRL6432_to_6844_深度总结_Part1.md`

**包含**：
- IF带宽翻倍详解 (5→10 MHz)
- 采样率计算公式变化
- 校准API变更 (4个变化点)
  1. RX IFA校准启用
  2. RX增益范围变化 (30-40dB→36-46dB)
  3. Factory Calibration API变更
  4. 校准时间增加
- 代码迁移示例
- API对比表

### 第三部分：通道配置详解

**完整内容见**：`Migration_xWRL6432_to_6844_深度总结_Part2.md`

**包含**：
- TX通道架构 (4通道，8个PA)
- TX 8位掩码详解
  - 每个PA一位
  - 必须成对使能
  - 掩码示例：0x03, 0x0C, 0x30, 0xC0
- RX通道架构 (4通道，4条基带链)
- RX特殊掩码格式 (0x99 = 0b10011001)
  - 只有Bit0,1,3,7有效
  - 其他位必须为0
- CLI命令对比
- 迁移注意事项

**关键图片**：
- `Migration_from_xWRLx432_to_xWRL6844_p4_img1.jpeg` - TX通道架构
- `Migration_from_xWRLx432_to_xWRL6844_p4_img2.jpeg` - RX通道架构

### 第四部分：Per-Chirp LUT详解

**完整内容见**：`Migration_xWRL6432_to_6844_深度总结_Part2.md`

**包含**：
- Per-Chirp LUT概述
- 16KB PER_CHIRP_RAM专用内存
- TDMA模式示例1：4 chirps基础配置
- TDMA模式示例2：8 chirps重复配置
- TDMA模式示例3：变化配置
- BPM模式示例：4 chirps相位控制
- API调用详解
- SDK CLI抽象

**关键图片**：
- `Migration_from_xWRLx432_to_xWRL6844_p5_img1.jpeg` - Per-Chirp LUT API
- `Migration_from_xWRLx432_to_xWRL6844_p6_img1-5.jpeg` - TDMA配置示例
- `Migration_from_xWRLx432_to_xWRL6844_p7_img1-5.jpeg` - BPM配置示例

### 第五部分：SDK CLI配置迁移

**完整内容见**：
- Part1: `Migration_xWRL6432_to_6844_深度总结_Part2.md`
- Part2: `Migration_xWRL6432_to_6844_深度总结_Part3.md`

**包含**：
- `channelCfg`命令对比
- `lowPowerCfg`命令
- `chirpComnCfg`命令 (采样率参数翻倍)
- `chirpTimingCfg`命令 (时序微调)
- `frameCfg`命令 (burst空闲时间+131μs)
- BPM/TDM配置
- ADC数据采集配置 (RDIF→LVDS)

**CLI配置速查表**：
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
```

### 第六部分：ADC数据格式变化

**完整内容见**：`Migration_xWRL6432_to_6844_深度总结_Part3.md`

**包含**：
- xWRL6432数据格式 (IQ交织)
- xWRL6844数据格式 (I/Q分开)
- 与xWR6843格式兼容
- MATLAB解析代码示例
- Python解析代码示例
- 数据格式图解

**关键图片**：
- `Migration_from_xWRLx432_to_xWRL6844_p9_img1.jpeg` - ADC数据格式对比

**数据格式对比**：
```
xWRL6432: [I0 Q0 I1 Q1 ... IN QN] × M chirps
xWRL6844: [I0 I1 ... IN][Q0 Q1 ... QN] × M chirps
          ↑ 所有I连续      ↑ 所有Q连续
```

### 第七部分：内置相位旋转 ⭐ 最关键

**完整内容见**：`Migration_xWRL6432_to_6844_深度总结_Part3.md`

**包含**：
- 相位旋转概述 (为什么需要？)
- 设备级相位旋转
  - RX: [+ + - -]
  - TX: [+ - - +]
- 天线馈电级相位旋转
  - TX: [+ + - -]
  - RX: [+ + - -]
- 整体板级相位旋转（最终结果）
  - RX总相位: [+ + + +] （所有同相）
  - TX总相位: [+ - + -] （交替反相）
- 虚拟天线阵列相位校正
- 相位校正代码实现 (C/MATLAB/Python)
- 相位校正验证方法

**关键图片**：
- `Migration_from_xWRLx432_to_xWRL6844_p10_img1.jpeg` - 设备级相位
- `Migration_from_xWRLx432_to_xWRL6844_p10_img2.jpeg` - 天线级相位

**相位校正向量（必须记住）**：
```c
const float PHASE_CORRECTION_6844[16] = {
    1.0f,  1.0f,  1.0f,  1.0f,   // TX0 × RX[0-3]
   -1.0f, -1.0f, -1.0f, -1.0f,   // TX1 × RX[0-3] ← 反相
    1.0f,  1.0f,  1.0f,  1.0f,   // TX2 × RX[0-3]
   -1.0f, -1.0f, -1.0f, -1.0f    // TX3 × RX[0-3] ← 反相
};
```

**应用位置**：Range FFT之后，Doppler FFT之前

### 第八部分：迁移检查清单

**完整内容见**：`Migration_xWRL6432_to_6844_深度总结_Part3.md`

**硬件检查清单**：
```
□ 确认使用xWRL6844 EVM板
□ 确认天线已正确安装
□ 确认电源供应（5V/2A）
□ 确认USB连接（数据+调试）
□ 确认SOP开关设置正确
□ 如使用DCA1000，确认LVDS连接
```

**固件检查清单**：
```
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

**算法检查清单**：
```
□ 更新虚拟天线数量（6→16）
□ 应用相位校正向量
□ 更新Angle FFT（适配16虚拟天线）
□ 更新ADC数据解析（新格式）
□ 验证Range-Doppler-Angle处理
□ 调整CFAR检测阈值
□ 验证角度估计精度
```

**测试验证清单**：
```
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

## 🚨 常见错误与避坑指南

### 错误1：忘记应用相位校正

**症状**：
- Angle FFT结果错误
- 角度估计偏差 >10°
- 虚拟天线阵列增益下降

**解决**：
```c
// 必须在Range FFT后应用
void applyPhaseCorrection(cmplx16_t* rangeFFTOut, uint32_t numRangeBins) {
    const float PHASE[16] = {1,1,1,1,-1,-1,-1,-1,1,1,1,1,-1,-1,-1,-1};
    for (uint32_t i = 0; i < numRangeBins; i++) {
        for (uint32_t vx = 0; vx < 16; vx++) {
            rangeFFTOut[i*16+vx].real *= PHASE[vx];
            rangeFFTOut[i*16+vx].imag *= PHASE[vx];
        }
    }
}
```

### 错误2：采样率参数没有翻倍

**症状**：
- 距离估计错误
- Range分辨率不正确

**解决**：
```bash
# 错误
chirpComnCfg 40 0 0 128 4 63.0 1
             ↑↑ 没翻倍

# 正确
chirpComnCfg 80 0 0 128 4 63.0 1
             ↑↑ 必须翻倍
```

### 错误3：ADC数据解析使用旧格式

**症状**：
- 数据解析完全错误
- I/Q分量混淆
- Range FFT结果异常

**解决**：
```matlab
% 错误（6432格式）
complexData = reshape(data, 2, N, M);  % IQ交织

% 正确（6844格式）
I_data = data(1:N, :);      % 前N个是I
Q_data = data(N+1:end, :);  % 后N个是Q
complexData = complex(I_data, Q_data);
```

### 错误4：RX掩码使用错误

**症状**：
- 某些RX通道无数据
- CLI配置报错

**解决**：
```bash
# 错误
channelCfg 15 15 0  # 使用6432的简单掩码
          ↑↑ 

# 正确（6844 RX特殊格式）
channelCfg 15 153 0  # 153 = 0x99 = 0b10011001
          ↑↑↑ 必须使用0x99格式
```

### 错误5：忘记调整帧配置

**症状**：
- 帧率不稳定
- 系统日志warning
- Burst timing错误

**解决**：
```bash
# 错误（chirp数量没改）
frameCfg 2 0 600 16 250 0
         ↑ 应该是4

# 正确
frameCfg 4 0 600 16 250 0
         ↑ 4TX TDMA需要4个chirp
```

---

## 📊 性能对比

### 虚拟天线阵列性能

| 指标 | xWRL6432 | xWRL6844 | 提升 |
|------|----------|----------|------|
| **虚拟天线数** | 6 (2TX×3RX) | 16 (4TX×4RX) | +167% |
| **角度分辨能力** | 中等 | 优秀 | ✅ 显著提升 |
| **旁瓣抑制** | 基础 | 更好 | ✅ 改善 |
| **多目标分辨** | 有限 | 增强 | ✅ 提升 |

### 处理性能

| 指标 | xWRL6432 | xWRL6844 | 提升 |
|------|----------|----------|------|
| **MCU** | M4F/160MHz | R5F/200MHz | +25% |
| **DSP** | 无 | C66x/450MHz | ✅ 新增 |
| **内存** | 1 MB | 2.5 MB | +150% |
| **算法复杂度** | 有限 | 高 | ✅ 支持更复杂算法 |

### 采样性能

| 指标 | xWRL6432 | xWRL6844 | 变化 |
|------|----------|----------|------|
| **采样率** | 12.5 Msps | 25 Msps | +100% |
| **IF带宽** | 5 MHz | 10 MHz | +100% |
| **距离分辨** | 3 cm | 1.5 cm | ✅ 改善50% |

---

## 🔧 开发工具与资源

### 必备工具

```
工具清单：
├─ CCS (Code Composer Studio)
│   └─ 版本: 12.x或更高
│
├─ mmWave SDK
│   └─ 版本: 最新稳定版
│
├─ UniFlash
│   └─ 用途: 固件烧录
│
├─ mmWave Studio（可选）
│   └─ 用途: 高级调试和原始数据采集
│
└─ DCA1000（可选）
    └─ 用途: ADC数据采集
```

### SDK路径

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
    │   ├─ xWRL6844_OOB_demo\   ← Out-of-Box Demo
    │   └─ ...
    └─ software_docs\           ← 软件文档
```

### 关键示例代码

```
推荐学习示例：
├─ xWRL6844_OOB_demo
│   └─ 开箱即用Demo，包含完整配置
│
├─ mmw_demo
│   └─ mmWave Demo，包含CLI配置
│
└─ people_tracking
    └─ 人员跟踪Demo，高级算法示例
```

### 官方文档

```
必读文档：
├─ xWRL6844 Technical Reference Manual (TRM)
│   └─ 硬件详细规格
│
├─ xWRL6844 Datasheet
│   └─ 电气特性
│
├─ Migration_from_xWRLx432_to_xWRL6844.pdf
│   └─ 本文档源
│
├─ mmWave SDK User Guide
│   └─ SDK使用说明
│
└─ ICD (Interface Control Document)
    └─ 接口协议定义
```

---

## 💡 最佳实践

### 1. 使用SDK封装的Per-Chirp LUT

**推荐**：
```c
// 使用SDK的高级配置
rlRfSetPerChirpPhShifterCfg(...)  // SDK封装
```

**不推荐**：
```c
// 直接写Per-Chirp LUT内存
*(volatile uint32_t*)(PER_CHIRP_RAM_BASE + offset) = value;  // 容易出错
```

### 2. 使用新的校准API

**推荐**：
```c
// xWRL6844新API
rlRfFecssRfFactoryCalDataSet(...)
rlRfFecssRfFactoryCalDataGet(...)
```

**不推荐**：
```c
// xWRL6432旧API（6844不支持）
rlRfCalibDataStore(...)  // 已废弃
rlRfCalibDataRestore(...)  // 已废弃
```

### 3. 分段读取大数据

**推荐**：
```c
// 分段处理Range FFT结果
for (int binIdx = 0; binIdx < numRangeBins; binIdx += CHUNK_SIZE) {
    applyPhaseCorrection(&rangeFFTOut[binIdx * 16], CHUNK_SIZE);
    performDopplerFFT(&rangeFFTOut[binIdx * 16], CHUNK_SIZE);
}
```

**不推荐**：
```c
// 一次性处理所有数据（可能内存不足）
applyPhaseCorrection(rangeFFTOut, ALL_BINS);  // 可能OOM
```

### 4. 使用CLI配置而非硬编码

**推荐**：
```c
// 从CLI配置读取
uint8_t txMask = gMmwMssMCB.cfg.channelCfg.txChannelEn;
uint8_t rxMask = gMmwMssMCB.cfg.channelCfg.rxChannelEn;
```

**不推荐**：
```c
// 硬编码
#define TX_MASK 0x0F  // 不灵活
#define RX_MASK 0x99
```

### 5. 验证相位校正效果

**必做**：
```matlab
% 验证静态目标的相位连续性
for vx = 1:15
    phaseDiff(vx) = angle(data(vx+1) / data(vx));
end
% 检查是否有180°跳变（说明相位校正有问题）
```

---

## 🎓 学习路径建议

### 阶段1：了解硬件差异（1天）
- ✅ 阅读硬件对比部分
- ✅ 理解MCU、内存、通道变化
- ✅ 熟悉虚拟天线阵列概念

### 阶段2：掌握CLI配置（2天）
- ✅ 学习CLI命令对比
- ✅ 理解参数翻倍的原因
- ✅ 实践修改CLI配置文件

### 阶段3：理解Per-Chirp LUT（2天）
- ✅ 阅读Per-Chirp LUT详解
- ✅ 理解TDMA和BPM模式
- ✅ 学习如何使用SDK封装

### 阶段4：实现相位校正（3天）
- ✅ 理解相位旋转原理
- ✅ 实现相位校正代码
- ✅ 验证校正效果

### 阶段5：更新数据解析（2天）
- ✅ 理解ADC数据格式变化
- ✅ 修改数据解析代码
- ✅ 验证解析结果

### 阶段6：集成测试（3-5天）
- ✅ 完整系统测试
- ✅ 性能对比
- ✅ 调试优化

**总计**：约2周完成迁移

---

## 📞 获取帮助

### TI官方资源

```
TI E2E论坛：
└─ https://e2e.ti.com/
    └─ mmWave Sensors版块

TI技术支持：
└─ 通过E2E论坛或官方渠道

TI培训视频：
└─ TI Training Portal
    └─ mmWave Radar培训系列
```

### 常见问题排查

**问题1**：编译报错"undefined reference to ..."
- ✅ 检查链接脚本是否更新
- ✅ 检查内存分配是否正确

**问题2**：运行时系统挂起
- ✅ 检查栈大小配置
- ✅ 检查内存分配是否越界

**问题3**：数据异常
- ✅ 检查相位校正是否应用
- ✅ 检查ADC数据解析格式
- ✅ 检查通道掩码配置

**问题4**：CLI配置不生效
- ✅ 检查参数是否翻倍
- ✅ 检查通道掩码格式
- ✅ 查看系统日志

---

## 📝 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| V1.0 | 2025-11-30 | 完整总结文档首次发布 |
| | | - 涵盖所有Migration内容 |
| | | - 添加代码示例 |
| | | - 添加检查清单 |
| | | - 添加避坑指南 |

---

## 🙏 致谢

本文档基于TI官方Migration文档深度分析编写，感谢：
- Yang, Zigang (TI) - 原始文档作者
- Texas Instruments - 提供详细的技术文档
- TI E2E社区 - 提供技术支持

---

## 📄 许可说明

本文档为学习总结文档，基于TI官方公开文档编写，仅供学习参考使用。所有技术内容版权归Texas Instruments所有。

---

**文档完成！祝迁移顺利！🚀**

> 💡 **提示**：建议结合实际项目需求，按照检查清单逐步进行迁移和验证。如遇到问题，参考"常见错误与避坑指南"部分。

