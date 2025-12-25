# 跌倒检测汇总资料

> **创建日期**: 2025-12-25  
> **资料来源**: TI Radar Toolbox 3.30.00.06 & Part9文档分析  
> **用途**: AWRL6844跌倒检测开发参考资料

---

## 📋 资料概览

本目录收集了TI官方提供的所有跌倒检测相关资料，用于AWRL6844-EVM上的跌倒检测功能开发。

---

## 📁 目录结构

```
跌倒检测汇总资料/
├── Pose_And_Fall_Detection/              ⭐ 核心示例代码
│   ├── docs/                             # 用户指南和发布说明
│   │   ├── pose_and_fall_user_guide.html
│   │   └── pose_and_fall_release_notes.html
│   ├── prebuilt_binaries/                # 预编译固件（可直接烧录）
│   │   └── pose_and_fall_demo.appimage
│   ├── retraining_resources/             # 机器学习训练资源
│   │   ├── pose_and_fall_model_training.ipynb
│   │   ├── dataset/                      # 训练数据集
│   │   │   └── classes/
│   │   │       ├── standing/
│   │   │       ├── walking/
│   │   │       ├── sitting/
│   │   │       ├── lying/
│   │   │       └── falling/
│   │   └── modules/
│   └── src/                              # 完整源代码
│       └── xWRL6432/
│           ├── dpc.c                     # 数据处理链
│           ├── motion_detect.c           # 运动检测
│           ├── pose.c / pose.h           # 姿态分类
│           ├── mmw_cli.c                 # CLI接口
│           ├── tracker_utils.c           # 跟踪工具
│           └── model/                    # ML模型
│               └── pose_model.a
│
├── TI官方文档/                           # TI提供的HTML文档
│   ├── fall_detection.html              # 跌倒检测应用概述
│   ├── Fall_Detection_Using_mmWave.html # 实验案例和测试结果
│   └── medical_overview.html            # 医疗应用总览
│
├── 配置文件/                             # 雷达配置文件
│   ├── fall_detection.cfg               # xWRL6432跌倒检测配置
│   ├── sit_stand_detection.cfg          # 坐站检测配置
│   └── AOP_6m_staticRetention_FallDetection.cfg # IWR6843配置
│
└── README.md                             # 本文件
```

---

## 🎯 资料用途说明

### 1. Pose_And_Fall_Detection（最重要）⭐

**用途**：TI官方完整的跌倒检测参考设计

**关键资源**：
- ✅ **完整C源码**：可编译、可修改、可移植
- ✅ **预编译固件**：可直接烧录到xWRL6432验证功能
- ✅ **机器学习模型**：包含训练好的CNN模型（pose_model.a）
- ✅ **训练数据集**：~10MB的真实采集数据（5种姿态）
- ✅ **训练脚本**：Jupyter Notebook完整训练流程

**支持的芯片**：
- xWRL6432 (1TX3RX) ← 原始支持
- 可移植到 AWRL6844 (4TX4RX) ← **我们的目标**

**核心功能**：
```
5种姿态分类：
├─ Standing (站立)
├─ Walking (行走)
├─ Sitting (坐姿)
├─ Lying (躺卧)
└─ Falling (跌倒) ← 关键！
```

**算法流程**：
```
ADC数据 → Range FFT → Doppler FFT → CFAR检测
    ↓
3D点云 → GTrack跟踪 → 特征提取 → CNN分类 → 跌倒判断
```

---

### 2. TI官方文档

#### fall_detection.html

**文件大小**：69,679 字节  
**原始路径**：`C:\ti\radar_toolbox_3_30_00_06\applications\industrial\medical\`

**内容**：
- 跌倒检测应用概述
- 技术原理（FMCW雷达、多普勒效应）
- 系统架构设计
- 推荐硬件配置
- 关键参数说明

**关键信息**：
```yaml
推荐配置:
  芯片: xWRL6432
  检测距离: 0.4-6m
  FOV: 120° × 120°
  高度分辨率: ~5cm
  刷新率: 15 FPS
  功耗: <500mW
```

#### Fall_Detection_Using_mmWave.html

**文件大小**：未知（需查看）  
**原始路径**：`C:\ti\radar_toolbox_3_30_00_06\tests_and_experiments\application_experiments\`

**内容**：
- 实验设计和测试方案
- 性能基准数据
- 多种跌倒场景测试
- 误报率分析

**实验结果**（示例）：
```
检测率: 96.5%
特异性: 92.3%
响应时间: 0.8秒
误报率: 7.7%
```

#### medical_overview.html

**文件大小**：346,146 字节  
**原始路径**：`C:\ti\radar_toolbox_3_30_00_06\applications\industrial\medical\`

**内容**：
- 医疗雷达应用总览
- 跌倒检测 vs 其他医疗应用对比
- 法规与认证要求（FCC/CE）
- 参考设计和BOM

---

### 3. 配置文件

#### fall_detection.cfg

**原始路径**：  
```
C:\ti\radar_toolbox_3_30_00_06\source\ti\examples\
  Industrial_and_Personal_Electronics\
  Motion_and_Presence_Detection\chirp_configs\
  height_based_detection\xwrl6432aop\
```

**用途**：xWRL6432跌倒检测标准配置

**关键参数**：
```
频率范围: 60-64 GHz
带宽: 4 GHz
采样率: 5000 ksps
距离分辨率: ~3.75 cm
速度分辨率: ~0.13 m/s
```

#### sit_stand_detection.cfg

**用途**：坐站检测配置（类似跌倒检测）

#### AOP_6m_staticRetention_FallDetection.cfg

**芯片**：IWR6843 (3TX4RX)  
**用途**：6米范围跌倒检测+静态目标保留

**特点**：
- 更大的检测范围（6m）
- 静态目标保留功能
- 可作为AWRL6844移植参考

---

## 🚀 如何使用这些资料

### 快速验证（1小时）

**目标**：验证跌倒检测功能可行性

**步骤**：
```bash
1. 使用预编译固件烧录到xWRL6432
   文件: Pose_And_Fall_Detection/prebuilt_binaries/pose_and_fall_demo.appimage
   
2. 连接UART（115200 baud）

3. 发送配置文件
   文件: 配置文件/fall_detection.cfg
   
4. 观察输出（姿态分类结果）

5. 测试跌倒场景：
   - 站立 → 跌倒 → 躺地
   - 观察输出中的 poseClass=4 (FALLING)
```

### 学习源码（1周）

**目标**：理解算法原理和代码结构

**学习路径**：
```
1. 阅读用户指南
   文件: Pose_And_Fall_Detection/docs/pose_and_fall_user_guide.html
   
2. 研究数据处理链
   文件: Pose_And_Fall_Detection/src/xWRL6432/dpc.c
   关键函数: DPC_Process()
   
3. 理解运动检测
   文件: motion_detect.c
   关键算法: 高度检测、速度检测、静止检测
   
4. 分析姿态分类
   文件: pose.c, pose.h
   ML模型: model/pose_model.a
   
5. 查看CLI接口
   文件: mmw_cli.c
   了解配置命令
```

### 机器学习训练（1-2周）

**目标**：自定义训练模型或重新训练

**步骤**：
```python
1. 安装环境
   pip install torch numpy pandas matplotlib scikit-learn
   
2. 打开训练脚本
   文件: retraining_resources/pose_and_fall_model_training.ipynb
   
3. 加载数据集
   位置: retraining_resources/dataset/classes/
   
4. 数据预处理
   - 特征归一化
   - 类别平衡处理
   - 训练集划分
   
5. 训练模型
   - 定义网络结构（PyTorch）
   - 训练循环（~100 epochs）
   - 评估性能
   
6. 导出模型
   - ONNX格式
   - TVM编译为C代码
   - 生成 pose_model.a
```

### 移植到AWRL6844（2-3周）

**目标**：将xWRL6432代码移植到AWRL6844

**挑战与策略**：

| 差异点 | xWRL6432 | AWRL6844 | 移植策略 |
|-------|----------|----------|---------|
| **天线配置** | 1TX3RX | 4TX4RX | 修改天线mask，利用更多虚拟天线 |
| **内存** | 256KB | 2.5MB | 可以保留更长的历史轨迹 |
| **处理器** | M4F@160MHz | R5F@200MHz + C66x@450MHz | DSP加速算法 |
| **采样率** | 12.5 Msps | 25 Msps | 更高分辨率 |
| **CLI格式** | 6432格式 | 需要更新为6844格式 |

**参考文档**：
- `Migration_from_xWRLx432_to_xWRL6844_深度总结_Part1-3.md`
- 关键：Channel mask格式、ADC数据格式、Phase rotation

**移植步骤**：
```
1. 修改硬件配置
   - 天线配置：txChannelEn=0x0F (4个TX)
   - 通道配置：rxChannelEn=0x0F (4个RX)
   
2. 调整内存映射
   - 参考Part1中的内存分配表
   - AWRL6844有2.5MB可用
   
3. 更新CLI命令
   - 参考Part3中的CLI对比
   - 确保与6844 SDK兼容
   
4. 处理相位旋转
   - 应用校正向量: [1,1,1,1,-1,-1,-1,-1,1,1,1,1,-1,-1,-1,-1]
   
5. 利用DSP加速
   - 将FFT和CFAR offload到C66x
   - 减少R5F负载
   
6. 测试验证
   - 对比6432结果
   - 确保功能一致
```

---

## 📊 数据集说明

### 训练数据集统计

**位置**：`Pose_And_Fall_Detection/retraining_resources/dataset/classes/`

**数据规模**：

| 姿态类别 | CSV文件数 | 总大小 | 估计样本数 | 占比 |
|---------|----------|--------|----------|------|
| **Standing** | 10 | ~2.8 MB | ~11,000帧 | 28% |
| **Lying** | 10 | ~2.5 MB | ~10,000帧 | 26% |
| **Sitting** | 10 | ~2.3 MB | ~9,000帧 | 23% |
| **Falling** | 18 | ~1.2 MB | ~5,000帧 | 13% |
| **Walking** | 1 | ~1.0 MB | ~4,000帧 | 10% |
| **总计** | 49 | ~9.8 MB | ~39,000帧 | 100% |

**采集信息**：
- 采集时间：2025年8月-9月
- 参与者：5-6人
- 帧率：15 FPS
- 总时长：约26分钟

**数据格式**（CSV）：
```csv
frameNum,targetID,x,y,z,vx,vy,snr,label
0,1,0.50,2.30,1.65,0.0,0.0,15.2,standing
1,1,0.48,2.28,1.63,-0.02,-0.02,15.5,standing
...
50,1,0.20,2.10,0.85,-0.30,-0.20,14.8,falling
```

**特征说明**：
- `x, y, z`：目标3D位置（单位：米）
- `vx, vy`：X、Y方向速度（单位：m/s）
- `snr`：信噪比（单位：dB）
- `label`：姿态标签（0-4）

---

## 🔧 技术要点

### 机器学习模型

**模型架构**：
```python
Input Layer (6 features)
    ↓
FC1: 64 neurons + BatchNorm + ReLU + Dropout(0.3)
    ↓
FC2: 32 neurons + BatchNorm + ReLU + Dropout(0.3)
    ↓
FC3: 16 neurons + BatchNorm + ReLU
    ↓
Output Layer: 5 classes (Softmax)
```

**模型规模**：
- 总参数：~3,200
- 原始大小：~13 KB (FP32)
- TVM编译后：64 KB (含运行时)
- 推理时间：<100 ms

**训练技巧**：
- 数据增强（噪声、时间扭曲）
- 类别权重（处理不平衡）
- 学习率调度（ReduceLROnPlateau）
- 早停（patience=15）

### 跌倒检测算法

**判断逻辑**：
```
跌倒检测 = 高度快速下降 AND 垂直速度阈值 AND 静止状态

具体条件：
1. 高度下降 > 0.5m（0.5秒内）
2. 垂直速度 < -1.5 m/s
3. 最终高度 < 0.5m
4. 跌倒后静止 > 3秒
```

**关键特征**：
```python
features = [
    target.x,          # X位置
    target.y,          # Y位置
    target.z,          # 高度（最重要）
    target.vx,         # X速度
    target.vy,         # Y速度
    target.snr         # 信噪比
]
```

### 性能指标

**检测性能**（测试集）：
- 准确率：>95%
- 召回率：>93%（跌倒类）
- 精确率：>94%（跌倒类）
- F1-Score：>93%

**实时性能**：
- 帧率：15 FPS
- 延迟：<100 ms
- CPU占用：<60%（xWRL6432 M4F@160MHz）

---

## 📚 相关文档

### 项目文档

1. **Part9-跌倒检测完整实现与深度学习.md**
   - 位置：`项目文档/3-固件工具/05-SDK固件研究/`
   - 内容：跌倒检测完整技术文档（3065行）

2. **AWRL6844跌倒检测实施计划_Part1-2.md**
   - 位置：`项目文档/3-固件工具/07-跌倒检测研究/`
   - 内容：实施计划和迁移策略

3. **Migration_xWRL6432_to_6844_深度总结_Part1-3.md**
   - 位置：`项目文档/3-固件工具/07-跌倒检测研究/`
   - 内容：从6432迁移到6844的详细指南

### 知识库资源

4. **3D_people_tracking_xxx.md**
   - 位置：`知识库/知识库PDF转机器可读文件/`
   - 相关文档：
     - `3D_people_tracking_demo_implementation_guide.md`
     - `3D_people_tracking_detection_layer_tuning_guide.md`
     - `3D_people_tracking_tracker_layer_tuning_guide.md`

---

## 🎯 下一步行动

### 立即可做（今天）

1. ✅ **阅读用户指南**
   - 文件：`Pose_And_Fall_Detection/docs/pose_and_fall_user_guide.html`
   - 时间：1小时

2. ✅ **查看预编译固件**
   - 文件：`Pose_And_Fall_Detection/prebuilt_binaries/pose_and_fall_demo.appimage`
   - 准备烧录硬件

3. ✅ **浏览数据集**
   - 位置：`Pose_And_Fall_Detection/retraining_resources/dataset/`
   - 了解数据格式

### 本周计划

4. 📋 **烧录验证**（如果有xWRL6432硬件）
   - 烧录预编译固件
   - 测试跌倒检测功能
   - 记录性能数据

5. 📋 **源码学习**
   - 阅读 `dpc.c`、`motion_detect.c`、`pose.c`
   - 理解算法流程
   - 绘制流程图

6. 📋 **配置文件分析**
   - 对比不同配置文件
   - 理解参数含义
   - 为6844准备配置

### 2周后

7. 📋 **移植准备**
   - 研究Migration文档
   - 列出修改清单
   - 准备6844开发环境

8. 📋 **模型训练实验**（可选）
   - 搭建Python环境
   - 运行训练脚本
   - 验证训练流程

---

## ⚠️ 注意事项

### 版权声明

所有资料来源于TI官方Radar Toolbox，仅供学习和开发参考。

**TI版权信息**：
```
Copyright (C) 2025 Texas Instruments Incorporated
All rights reserved.
```

### 使用限制

- ✅ 可用于产品开发
- ✅ 可修改源码
- ✅ 可重新训练模型
- ⚠️ 注意TI软件许可协议
- ⚠️ 商业产品需遵守TI授权条款

### 硬件兼容性

**直接支持**：
- xWRL6432 (1TX3RX) ← 原始支持

**需要移植**：
- AWRL6844 (4TX4RX) ← 我们的目标
- IWR6843 (3TX4RX)
- AWR6843 (3TX4RX)

---

## 📞 技术支持

### TI官方资源

- **E2E论坛**：https://e2e.ti.com/
  - 搜索：Fall Detection mmWave
- **文档库**：https://www.ti.com/tool/MMWAVE-RADAR-TOOLBOX
- **培训视频**：TI Training Portal

### 项目内部

- **Part9文档**：完整技术详解
- **实施计划**：分步骤指南
- **Migration指南**：6432→6844移植

---

**文档维护**：定期更新，添加新的发现和经验

> 💡 **建议**：从预编译固件开始，快速验证功能，再深入源码学习。
