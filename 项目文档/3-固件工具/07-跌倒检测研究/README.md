# 📊 07 - 跌倒检测研究

> **研究目标**: 基于AWRL6844-EVM实现跌倒检测功能  
> **技术路线**: 从xWRL6432迁移跌倒检测算法到AWRL6844  
> **创建日期**: 2025-12-25  
> **状态**: 研究中 🟡

---

## 🎯 研究目标

### 主要目标

1. **理解TI官方跌倒检测算法**
   - 深入研究Pose_And_Fall_Detection示例代码
   - 掌握姿态分类和跌倒判断算法
   - 理解CNN机器学习模型的应用

2. **完成芯片迁移**
   - 从xWRL6432（2TX 3RX）迁移到AWRL6844（4TX 4RX）
   - 解决硬件差异：MCU架构（M4F→R5F）、通道配置、内存映射
   - 适配SDK变化：DFP配置、数据格式、相位旋转

3. **验证与优化**
   - 在AWRL6844-EVM上验证跌倒检测功能
   - 优化检测精度和响应时间
   - 降低误报率

---

## 📁 目录结构

```
07-跌倒检测研究/
├── README.md                                           # 本文件
│
├── Migration_xWRL6432_to_6844_索引与快速指南.md       # ⭐ 迁移快速入门
├── Migration_xWRL6432_to_6844_深度总结_Part1.md       # 硬件对比与内存映射
├── Migration_xWRL6432_to_6844_深度总结_Part2.md       # 通道配置与Per-Chirp LUT
├── Migration_xWRL6432_to_6844_深度总结_Part3.md       # 时序配置与数据格式
│
└── 跌倒检测汇总资料/                                   # TI官方资料集合
    ├── README.md                                       # 资料索引
    ├── Pose_And_Fall_Detection/                        # ⭐ 核心参考代码
    │   ├── docs/                                       # 用户指南
    │   ├── prebuilt_binaries/                          # 预编译固件
    │   ├── retraining_resources/                       # ML训练资源
    │   └── src/                                        # 完整源代码
    ├── TI官方文档/                                     # HTML技术文档
    │   ├── fall_detection.html                         # 应用概述
    │   ├── Fall_Detection_Using_mmWave.html           # 实验案例
    │   └── medical_overview.html                       # 医疗应用总览
    └── 配置文件/                                       # 雷达配置文件
        ├── fall_detection.cfg                          # xWRL6432配置
        ├── sit_stand_detection.cfg                     # 坐站检测
        └── AOP_6m_staticRetention_FallDetection.cfg   # IWR6843配置
```

---

## 📚 核心文档说明

### 1. 迁移指南系列 🔄

#### Migration_xWRL6432_to_6844_索引与快速指南.md ⭐ 推荐首读

**用途**: 快速了解迁移要点

**核心内容**:
- ✅ 5分钟速览核心变化（10项必须修改）
- ✅ 硬件对比表（14项特性详细对比）
- ✅ 3条迁移路径建议（SDK/低层API/混合）
- ✅ 完整迁移检查清单（4大类检查项）

**适合人群**: 
- 需要快速评估迁移工作量
- 项目经理/技术负责人

**阅读时间**: 15分钟

---

#### Migration深度总结 - Part1: 硬件与内存

**内容**:
- 🔴 **第一部分**: 硬件特性对比（14项差异）
- 🔴 **第二部分**: 内存映射与分区（1MB→2.5MB）
- 🔴 **第三部分**: DFP变更详解（IF带宽翻倍、校准API）

**关键信息**:
```yaml
硬件核心变化:
  MCU: M4F/160MHz → R5F/200MHz
  DSP: 无 → C66x/450MHz
  内存: 1MB → 2.5MB
  TX通道: 2 → 4
  RX通道: 3 → 4
  最大IF: 5MHz → 10MHz
  最大采样率: 12.5Msps → 25Msps
```

**行数**: 317行  
**阅读时间**: 20分钟

---

#### Migration深度总结 - Part2: 通道配置

**内容**:
- 🔴 **TX通道掩码**: 2位→8位（每PA一位）
- 🔴 **RX通道掩码**: 3位→8位（特殊0x99格式）
- 🔴 **Per-Chirp LUT方法**: TDMA/BPM模式配置
- 🔴 **SDK CLI抽象**: channelCfg命令详解

**关键信息**:
```c
// TX通道配置（必须成对使能PA）
TX0AB: 0x03  // PA0A + PA0B
TX1AB: 0x0C  // PA1A + PA1B
TX2AB: 0x30  // PA2A + PA2B
TX3AB: 0xC0  // PA3A + PA3B
所有TX: 0xFF

// RX通道配置（特殊掩码）
6432: 0x07 (3个RX)  →  6844: 0x99 (4个RX)
```

**行数**: 484行  
**阅读时间**: 25分钟

---

#### Migration深度总结 - Part3: 时序与数据格式

**内容**:
- 🔴 **Chirp时序配置**: adc_start_time微调
- 🔴 **帧配置**: Burst空闲时间+131μs
- 🔴 **ADC数据格式**: IQ交织→I/Q分开 ⭐ 重要
- 🔴 **内置相位旋转**: 必须添加的校正 ⭐ 关键

**关键信息**:
```python
# 相位校正向量（必须应用）
phase_correction = [1,1,1,1,-1,-1,-1,-1,1,1,1,1,-1,-1,-1,-1]

# ADC数据格式变化
6432: [I0,Q0,I1,Q1,I2,Q2,...]  # IQ交织
6844: [I0,I1,I2,...,Q0,Q1,Q2,...]  # I/Q分开
```

**行数**: 760行  
**阅读时间**: 30分钟

---

### 2. 跌倒检测汇总资料 📦

#### 资料来源

- **TI Radar Toolbox**: v3.30.00.06
- **原始路径**: `C:\ti\radar_toolbox_3_30_00_06\`
- **收集日期**: 2025-12-25

#### 核心资源

##### Pose_And_Fall_Detection/ ⭐ 最重要

**用途**: TI官方完整参考设计

**包含内容**:
```
├── src/xWRL6432/              # 完整C源代码
│   ├── dpc.c                  # 数据处理链
│   ├── motion_detect.c        # 运动检测
│   ├── pose.c / pose.h        # 姿态分类（CNN）
│   ├── mmw_cli.c              # CLI接口
│   ├── tracker_utils.c        # GTrack跟踪
│   └── model/pose_model.a     # 训练好的CNN模型
│
├── prebuilt_binaries/         # 预编译固件（可直接烧录）
│   └── pose_and_fall_demo.appimage
│
├── retraining_resources/      # ML训练资源
│   ├── pose_and_fall_model_training.ipynb  # Jupyter训练脚本
│   └── dataset/               # ~10MB训练数据
│       └── classes/
│           ├── standing/      # 站立数据
│           ├── walking/       # 行走数据
│           ├── sitting/       # 坐姿数据
│           ├── lying/         # 躺卧数据
│           └── falling/       # 跌倒数据 ⭐
│
└── docs/                      # 用户指南
    ├── pose_and_fall_user_guide.html
    └── pose_and_fall_release_notes.html
```

**算法流程**:
```
ADC原始数据
    ↓
Range FFT（距离维处理）
    ↓
Doppler FFT（速度维处理）
    ↓
CFAR检测（目标检测）
    ↓
3D点云生成
    ↓
GTrack跟踪（多目标跟踪）
    ↓
特征提取（位置、速度、RCS等）
    ↓
CNN分类器（5种姿态）
    ↓
跌倒判断逻辑
    ↓
输出结果（姿态 + 跌倒事件）
```

**支持芯片**: 原始支持xWRL6432，需要迁移到AWRL6844

---

##### TI官方文档/

**1. fall_detection.html**
- **大小**: 69KB
- **内容**: 跌倒检测应用概述、技术原理、系统架构
- **关键参数**: 
  - 检测距离: 0.4-6m
  - FOV: 120° × 120°
  - 刷新率: 15 FPS
  - 功耗: <500mW

**2. Fall_Detection_Using_mmWave.html**
- **内容**: 实验设计、测试方案、性能基准
- **实验结果**: 检测率96.5%、误报率7.7%

**3. medical_overview.html**
- **大小**: 346KB
- **内容**: 医疗雷达应用总览、法规要求、参考设计

---

##### 配置文件/

**1. fall_detection.cfg**
- **芯片**: xWRL6432
- **用途**: 跌倒检测标准配置

**2. sit_stand_detection.cfg**
- **芯片**: xWRL6432
- **用途**: 坐站检测配置

**3. AOP_6m_staticRetention_FallDetection.cfg**
- **芯片**: IWR6843
- **用途**: 6米检测范围配置（带静态保留）

---

## 🚀 快速开始

### 第一步: 理解迁移要点

**阅读顺序**:
1. ✅ `Migration_xWRL6432_to_6844_索引与快速指南.md` - 了解全貌
2. ✅ `Migration_深度总结_Part1.md` - 硬件差异
3. ✅ `Migration_深度总结_Part2.md` - 通道配置
4. ✅ `Migration_深度总结_Part3.md` - 数据格式

**预计时间**: 1.5小时

---

### 第二步: 研究跌倒检测算法

**阅读资源**:
1. ✅ `跌倒检测汇总资料/README.md` - 资料索引
2. ✅ `跌倒检测汇总资料/TI官方文档/fall_detection.html` - 算法原理
3. ✅ `跌倒检测汇总资料/Pose_And_Fall_Detection/docs/` - 用户指南

**代码研究**:
- 📂 `Pose_And_Fall_Detection/src/xWRL6432/`
- 重点文件: `pose.c`, `pose.h`, `dpc.c`

**预计时间**: 2-3小时

---

### 第三步: 验证预编译固件（可选）

如果有xWRL6432-EVM，可以先验证功能：

```powershell
# 1. 连接xWRL6432-EVM
# 2. 烧录预编译固件
uniflash -ccxml connection.ccxml -program pose_and_fall_demo.appimage

# 3. 使用TI Visualizer查看结果
```

---

### 第四步: 代码迁移

**迁移清单**:
- [ ] 修改MCU启动代码（M4F→R5F）
- [ ] 更新通道配置（TX 2→4, RX 3→4）
- [ ] 调整内存分配（1MB→2.5MB）
- [ ] 修改DFP配置（采样率翻倍）
- [ ] 添加相位校正向量
- [ ] 更新ADC数据解析（IQ交织→I/Q分开）
- [ ] 适配SDK API变化

**预计时间**: 1-2周

---

## 🎯 关键技术点

### 1. 硬件差异对比

| 特性 | xWRL6432 | AWRL6844 | 迁移影响 |
|-----|----------|----------|---------|
| MCU | M4F/160MHz | R5F/200MHz | 🔴 固件不兼容 |
| DSP | 无 | C66x/450MHz | ✅ 更强算力 |
| TX通道 | 2 | 4 | 🔴 配置必改 |
| RX通道 | 3 | 4 | 🔴 配置必改 |
| 内存 | 1MB | 2.5MB | ✅ 更多空间 |
| 虚拟阵列 | 2×3=6 | 4×4=16 | ✅ 更好性能 |

### 2. 必须修改的10项

1. **MCU架构** - 固件二进制不兼容
2. **TX通道掩码** - 2位→8位
3. **RX通道掩码** - 3位→8位（0x99格式）
4. **采样率参数** - 数值翻倍
5. **校准API** - 使用新Factory Cal API
6. **相位校正** - 必须添加16元素向量
7. **ADC数据格式** - IQ交织→I/Q分开
8. **ADC接口** - RDIF→LVDS（DCA1000用户）
9. **Per-Chirp LUT** - 4个TX的TDMA配置
10. **Burst空闲时间** - 增加131μs

### 3. 跌倒检测算法核心

**5种姿态分类**:
```
1. Standing (站立) - 正常状态
2. Walking (行走) - 正常状态
3. Sitting (坐姿) - 正常状态
4. Lying (躺卧) - 正常状态（睡觉）
5. Falling (跌倒) - ⚠️ 异常事件
```

**CNN模型**:
- 输入: 特征向量（位置、速度、RCS等）
- 输出: 5种姿态的概率分布
- 模型文件: `pose_model.a` (预训练)

**跌倒判断逻辑**:
```python
if pose_probability['falling'] > threshold:
    trigger_fall_alert()
elif pose == 'lying' and previous_pose in ['standing', 'walking']:
    # 从站立/行走突然变为躺卧，可能是跌倒
    trigger_fall_alert()
```

---

## 📊 开发路线图

### Phase 1: 研究阶段 ✅ 当前

- [x] 收集TI官方资料
- [x] 研究迁移文档
- [x] 分析跌倒检测算法
- [ ] 理解源代码结构

### Phase 2: 迁移准备 🟡 进行中

- [ ] 搭建AWRL6844开发环境
- [ ] 配置SDK和工具链
- [ ] 准备测试硬件
- [ ] 制定详细迁移计划

### Phase 3: 代码迁移 ⏳ 待开始

- [ ] 移植启动代码（R5F）
- [ ] 修改通道配置
- [ ] 适配内存分配
- [ ] 添加相位校正
- [ ] 更新数据解析

### Phase 4: 功能验证 ⏳ 待开始

- [ ] 编译固件
- [ ] 烧录到AWRL6844-EVM
- [ ] 功能测试
- [ ] 性能调优

### Phase 5: 优化部署 ⏳ 待开始

- [ ] 优化检测精度
- [ ] 降低误报率
- [ ] 优化功耗
- [ ] 编写部署文档

---

## 🔍 参考资源

### TI官方文档

- **Migration文档**: `Migration_from_xWRLx432_to_xWRL6844_V0.3.pdf`
- **数据手册**: `AWRL6844 Technical Reference Manual`
- **SDK**: `mmwave_sdk_xwr68xx`
- **工具**: TI Radar Toolbox v3.30.00.06

### 外部资源

- **GTrack算法**: TI的多目标跟踪库
- **CNN模型**: TensorFlow Lite格式
- **数据集**: 真实采集的姿态数据

---

## 📝 注意事项

### ⚠️ 迁移风险

1. **MCU架构差异** - 固件完全不兼容，必须重新编译
2. **相位校正** - 忘记添加会导致错误的角度估计
3. **数据格式** - ADC数据解析错误会导致点云异常
4. **通道配置** - 掩码配置错误会导致雷达无法工作

### ✅ 成功要点

1. **仔细阅读迁移文档** - 不要遗漏任何变更点
2. **逐步验证** - 每个模块迁移后立即测试
3. **保留原始代码** - 便于对比和回退
4. **详细记录** - 记录所有修改和测试结果

---

## 🤝 贡献指南

本研究目录持续更新中，欢迎补充：

- 📝 迁移过程中的问题和解决方案
- 📊 测试结果和性能数据
- 💡 优化建议和最佳实践
- 🐛 Bug报告和修复方案

---

## 📞 联系方式

如有技术问题，请参考：
- **TI E2E Forum**: https://e2e.ti.com/
- **技术支持**: 通过TI官方渠道

---

> 💡 **提示**: 建议按照"快速开始"部分的顺序进行学习，先理解迁移要点，再深入研究跌倒检测算法。

> ⚠️ **重要**: 本研究基于xWRL6432到AWRL6844的迁移，实际开发中请以最新的TI官方文档为准。

**最后更新**: 2025-12-25  
**维护状态**: 活跃 ✅
