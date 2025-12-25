# 📦 Radar Toolbox工具链与应用实例深度分析

> **文档版本**: v1.0  
> **创建日期**: 2025-12-25  
> **Toolbox版本**: radar_toolbox_3_30_00_06  
> **作者**: AI Assistant  
> **文档定位**: Radar Toolbox完整解析 - 工具链、应用实例、算法库

---

## 📋 目录

- [第一章：Radar Toolbox整体架构](#第一章radar-toolbox整体架构)
  - [1.1 目录结构全景](#11-目录结构全景)
  - [1.2 核心功能模块](#12-核心功能模块)
  - [1.3 与SDK的关系](#13-与sdk的关系)
- [第二章：应用分类体系](#第二章应用分类体系)
- [第三章：工具链详解](#第三章工具链详解)
- [第四章：可视化工具系统](#第四章可视化工具系统)
- [第五章：算法库与源码](#第五章算法库与源码)
- [第六章：跌倒检测完整实现](#第六章跌倒检测完整实现)

---

## 第一章：Radar Toolbox整体架构

### 1.1 目录结构全景

**Radar Toolbox 路径**：
```
C:\ti\radar_toolbox_3_30_00_06\
```

**顶层目录结构**：

```
radar_toolbox_3_30_00_06/
├── 📁 applications/              ← 应用场景分类文档（HTML）
├── 📁 getting_started/           ← 快速入门指南
├── 📁 hardware_docs/             ← 硬件文档（天线、参考设计）
├── 📁 software_docs/             ← 软件文档（调试、迁移指南）
├── 📁 source/                    ← 源代码（算法库、示例项目）
├── 📁 tests_and_experiments/     ← 测试实验案例
├── 📁 toolbox_docs/              ← Toolbox总览文档
└── 📁 tools/                     ← 开发工具集合
```

### 1.2 核心功能模块

#### 功能分层架构

```
┌─────────────────────────────────────────────────────┐
│            Radar Toolbox 功能分层                     │
├─────────────────────────────────────────────────────┤
│  📱 应用层 (applications/)                            │
│  - 汽车、工业、个人电子应用场景文档                       │
│  - 应用需求 → 硬件选型 → 配置建议                        │
├─────────────────────────────────────────────────────┤
│  🛠️ 工具层 (tools/)                                  │
│  - 数据采集工具（DCA1000、Studio CLI）                  │
│  - 可视化工具（18个GUI）                                │
│  - 烧录工具（JTAG、CAN、OTA）                           │
│  - 机器学习流程（Edge AI Studio、Jupyter）             │
├─────────────────────────────────────────────────────┤
│  🧩 算法层 (source/ti/alg/)                          │
│  - GTrack跟踪算法（2D/3D）                             │
│  - 手势识别算法                                         │
│  - 姿态检测算法                                         │
├─────────────────────────────────────────────────────┤
│  💻 示例层 (source/ti/examples/)                     │
│  - 21个工业应用示例                                     │
│  - 6个汽车应用示例                                      │
│  - 完整源码 + 预编译二进制                              │
├─────────────────────────────────────────────────────┤
│  📚 文档层 (docs/)                                    │
│  - Getting Started指南                               │
│  - 调试指南、迁移指南                                   │
│  - 实验案例                                            │
└─────────────────────────────────────────────────────┘
```

#### 关键数字统计

| 类别 | 数量 | 说明 |
|------|------|------|
| **应用场景文档** | 30+ | 涵盖汽车/工业/个人电子 |
| **示例项目** | 27+ | 完整源码 + 预编译固件 |
| **可视化工具** | 18 | Python/MATLAB GUI |
| **算法库** | 3+ | GTrack、手势识别、姿态检测 |
| **开发工具** | 14 | 数据采集、烧录、调试工具 |
| **实验案例** | 15+ | 跌倒检测、悬崖检测、占用检测等 |

### 1.3 与SDK的关系

#### 三大SDK目录的协作关系

```
MMWAVE_L_SDK (底层固件开发)
       ↓
[提供] 芯片驱动、RTOS、编译工具
       ↓
radar_toolbox (应用开发) ← 本文档重点
       ↓
[提供] 应用Demo、配置文件、可视化
       ↓
mmwave_studio (RF调试)
       ↓
[提供] 参数调优、波形分析
```

#### Radar Toolbox的定位

**🎯 核心定位**：应用开发加速器

| 维度 | 说明 |
|------|------|
| **输入** | 应用需求（如"跌倒检测"） |
| **提供** | 完整解决方案（代码+配置+工具） |
| **输出** | 可直接运行的Demo + 可定制源码 |

**🔄 典型使用流程**：

```
1️⃣ 确定应用场景
   → 查看 applications/ 分类文档
   
2️⃣ 找到对应示例
   → source/ti/examples/[类别]/[应用名]/
   
3️⃣ 获取预编译固件
   → prebuilt_binaries/
   
4️⃣ 测试运行
   → 使用 visualizers/ 工具查看效果
   
5️⃣ 修改源码
   → src/ 目录（需要CCS + SDK）
   
6️⃣ 优化配置
   → 参考 chirp_configs/ 和 tuning guides
```

#### 与MMWAVE_L_SDK的文件关联

**示例：Pose_And_Fall_Detection**

```
radar_toolbox/source/ti/examples/Industrial_and_Personal_Electronics/
└── Pose_And_Fall_Detection/
    ├── src/xWRL6432/              ← 源码（需SDK编译）
    ├── prebuilt_binaries/         ← 预编译固件（可直接烧录）
    ├── docs/                      ← 应用文档
    └── retraining_resources/      ← 机器学习重训练资源
         └── pose_and_fall_model_training.ipynb
```

**编译依赖**：

```c
// 源码引用SDK头文件
#include <ti/drivers/UART.h>           ← SDK提供
#include <ti/sysbios/BIOS.h>           ← SDK提供
#include <gtrack.h>                    ← Toolbox算法库
```

**配置文件关联**：

```
radar_toolbox配置文件 (.cfg)
       ↓
[使用] mmwave_L_SDK的CLI接口
       ↓
channelCfg, chirpCfg, frameCfg等命令
       ↓
[调用] SDK底层驱动配置芯片
```

---

## 第二章：应用分类体系

### 2.1 应用分类总览

**Radar Toolbox 将应用分为三大类**：

```
applications/
├── 🚗 automotive/                    ← 汽车应用
│   ├── ADAS_and_parking/            ← ADAS与停车
│   ├── hands-free_access_and_door/  ← 免提门禁
│   └── in-cabin_security/           ← 车内安全
│
├── 🏭 industrial/                    ← 工业应用
│   ├── appliances/                  ← 家电
│   ├── building_automation/         ← 楼宇自动化
│   ├── industrial_transport/        ← 工业运输
│   ├── level_sensing/               ← 液位检测
│   ├── medical/                     ← 医疗
│   └── robotics/                    ← 机器人
│
└── 📱 personal_electronics/          ← 个人电子
    ├── pc_notebooks/                ← PC笔记本
    ├── tv_and_speakers/             ← 电视音响
    └── wearables/                   ← 可穿戴设备
```

### 2.2 工业应用详细分类

#### 2.2.1 医疗类应用 (medical/)

**应用场景文档**：

| 文档名 | 应用场景 | 关键功能 |
|--------|---------|---------|
| `fall_detection.html` | ⭐ **跌倒检测** | 姿态识别、跌倒事件检测、报警 |
| `vital_signs_monitoring.html` | 生命体征监测 | 呼吸心跳检测、非接触监护 |
| `medical_overview.html` | 医疗应用总览 | 应用对比、硬件选型 |

#### 2.2.2 机器人类应用 (robotics/)

| 文档名 | 应用场景 | 文件大小 |
|--------|---------|---------|
| `agv_and_amr.html` | AGV/AMR机器人 | 2.8 MB |
| `industrial_robots.html` | 工业机器人 | 829 KB |
| `robotics_overview.html` | 机器人总览 | 560 KB |

#### 2.2.3 楼宇自动化 (building_automation/)

| 文档名 | 应用场景 |
|--------|---------|
| `automated_doors_and_gates.html` | 自动门/闸机 |
| `commercial_and_home_surveillance.html` | 商业/家庭监控 |
| `occupancy_sensing_and_lighting.html` | 占用检测/智能照明 |

#### 2.2.4 家电类应用 (appliances/)

| 文档名 | 应用场景 | 文件大小 |
|--------|---------|---------|
| `home_appliances.html` | 家用电器 | 297 KB |
| `residential_air_conditioners.html` | 空调 | 297 KB |
| `thermostats.html` | 智能温控器 | 293 KB |

### 2.3 汽车应用详细分类

#### 2.3.1 ADAS与停车 (ADAS_and_parking/)

| 文档名 | 应用场景 |
|--------|---------|
| `corner_radar_overview.html` | 角雷达 |
| `front_radar_overview.html` | 前雷达 |
| `Near-Field_sensing_overview.html` | 近场感知 |

#### 2.3.2 车内安全 (in-cabin_security/)

| 文档名 | 应用场景 | 文件大小 |
|--------|---------|---------|
| `in-cabin_sensing_overview.html` | 车内感知总览 | 872 KB |

#### 2.3.3 免提门禁 (hands-free_access/)

| 文档名 | 应用场景 | 文件大小 |
|--------|---------|---------|
| `exterior_sensing_overview.html` | 外部感知 | 2.6 MB |

### 2.4 应用文档的价值

**每个应用HTML文档包含**：

```
1️⃣ 应用场景描述
   - 典型使用场景
   - 性能要求
   - 环境挑战

2️⃣ 硬件选型建议
   - 推荐芯片型号
   - 天线配置
   - 性能参数对比

3️⃣ 示例项目链接
   - 对应source/ti/examples/路径
   - 预编译固件下载
   - 配置文件路径

4️⃣ 关键参数建议
   - 检测距离
   - 角度范围
   - 分辨率
   - 帧率

5️⃣ 相关文档链接
   - 芯片手册
   - 应用笔记
   - 调试指南
```

---

## 第三章：工具链详解

### 3.1 工具分类总览

**Radar Toolbox提供14类开发工具**：

```
tools/
├── 📊 数据采集类
│   ├── Adc_Data_Capture_Tool_DCA1000_CLI  ← DCA1000数据采集
│   ├── mmwave_data_recorder               ← 数据记录器
│   └── SPI_Data_Capture                   ← SPI数据采集
│
├── 🔧 烧录工具类
│   ├── JTAG_Flasher                       ← JTAG烧录
│   ├── CAN_SBL_Flasher                    ← CAN烧录
│   └── OTA_Swap                           ← OTA固件交换
│
├── 🎮 控制工具类
│   ├── studio_cli                         ← mmWave Studio CLI
│   ├── VSCode_CLI_command_utility         ← VSCode CLI工具
│   └── remote_access                      ← 远程访问
│
├── 🤖 机器学习类
│   ├── machine_learning_flow              ← ML完整流程
│   └── kobuki_surface_classification_src  ← 表面分类
│
├── 📈 可视化类
│   └── visualizers/                       ← 18个GUI工具
│
├── 📐 工程工具类
│   ├── sensing_estimator                  ← 感知估算器
│   ├── memory_compression                 ← 内存压缩
│   └── scripts/                           ← MATLAB/Lua脚本
```

### 3.2 数据采集工具

#### 3.2.1 DCA1000数据采集工具

**工具路径**：
```
tools/Adc_Data_Capture_Tool_DCA1000_CLI/
```

**功能定位**：
- 🎯 通过DCA1000 EVM采集原始ADC数据
- 📊 支持高速数据流（最高5.5 Gbps）
- 💾 保存为二进制文件供后处理

**核心文件**：

| 文件/目录 | 说明 |
|----------|------|
| `gui/` | Python GUI界面 |
| `chirp_configs/` | 预设配置文件 |
| `prebuilt_binaries/` | 预编译工具 |
| `docs/` | 使用文档 |
| `data/` | 采集数据存储 |

**典型使用场景**：

```
1️⃣ 算法开发验证
   - 采集真实场景数据
   - 离线处理和算法测试
   - MATLAB/Python后处理

2️⃣ 性能基准测试
   - 记录测试场景数据
   - 可重复性验证
   - 多次对比分析

3️⃣ 机器学习训练
   - 采集训练数据集
   - 标注数据
   - 模型训练
```

**配置文件示例**：
```
chirp_configs/xWRL6844_4T4R_tdm.cfg  ← 4发4收TDM配置
```

#### 3.2.2 mmWave数据记录器

**工具路径**：
```
tools/mmwave_data_recorder/
```

**功能特点**：
- 📡 通过UART记录处理后的点云数据
- 🎬 支持长时间录制
- 📝 CSV格式输出

**配置示例**：
```
src/cfg/6844_profile_4T4R_tdm.cfg  ← AWRL6844配置
```

### 3.3 烧录工具链

#### 3.3.1 JTAG烧录工具

**工具路径**：
```
tools/JTAG_Flasher/
```

**支持芯片**：

| 芯片系列 | 工具目录 | 说明 |
|---------|---------|------|
| xWR16xx | `tool/xwr16xx/` | IWR1642等 |
| xWR18xx | `tool/xwr18xx/` | IWR1843等 |
| xWR68xx | `tool/xwr68xx/` | AWR6843、IWR6843等 |
| xWRL6432 | `tool/xwrl6432/` | AWRL6432、IWRL6432 |
| AWR2944 | `tool/awr2944/` | AWR2944 |

**核心功能**：
```
✅ 通过JTAG接口烧录固件到QSPI Flash
✅ 支持批量烧录
✅ 烧录验证
✅ 日志记录
```

**使用场景**：
- 🔧 开发阶段频繁更新固件
- 🏭 生产线批量烧录
- 🔄 固件恢复和升级

#### 3.3.2 CAN SBL烧录工具

**工具路径**：
```
tools/CAN_SBL_Flasher/
```

**功能定位**：
- 🚗 通过CAN总线烧录（适用于车载环境）
- 📦 使用Secondary Bootloader (SBL)
- 🔒 支持安全启动

#### 3.3.3 OTA固件交换

**工具路径**：
```
tools/OTA_Swap/
```

**功能特点**：
```
✅ 支持双Bank固件存储
✅ 在线固件升级（Over-The-Air）
✅ 升级失败自动回滚
✅ 适用于AWR2944芯片
```

**文件结构**：
```
OTA_Swap/
├── src/
│   ├── ota_swap/          ← OTA应用代码
│   └── sbl_qspi/          ← QSPI Bootloader
├── prebuilt_binaries/     ← 预编译固件
├── Tool/                  ← 上位机工具
└── Docs/                  ← 使用文档
```

### 3.4 机器学习工具链

#### 3.4.1 机器学习完整流程

**工具路径**：
```
tools/machine_learning_flow/
```

**支持两种开发方式**：

##### 方式1：Edge AI Studio（推荐新手）

```
machine_learning_flow/Edge_AI_Studio/
└── 图形化界面，零代码机器学习
```

**特点**：
- 🎨 可视化拖拽式工作流
- 🚀 快速原型验证
- 📊 自动模型优化

##### 方式2：Jupyter Notebook（灵活定制）

```
machine_learning_flow/Jupyter_Notebook/
├── src/
│   ├── 1-Data_Collection/        ← 数据采集
│   ├── 2-Model_training/         ← 模型训练
│   ├── 3-TVM_codegen/            ← 代码生成
│   └── 4-CCS-Integration/        ← CCS集成
```

**完整ML流程**：

```
步骤1：数据采集
├─ 使用DCA1000采集原始数据
├─ 标注数据集（falling/lying/sitting/standing/walking）
└─ 保存为二进制或CSV格式

步骤2：模型训练
├─ pytorch_fp_model-DRY-WET.ipynb        ← 干湿分类
├─ pytorch_fp_model-GRASS-NOTGRASS.ipynb ← 草地分类
├─ 训练卷积神经网络
└─ 导出ONNX模型

步骤3：TVM代码生成
├─ ONNX → TVM优化
├─ 生成C代码
└─ 适配DSP/ARM

步骤4：CCS集成
├─ 导入生成的C代码
├─ 与雷达Demo集成
└─ 编译烧录测试
```

**示例应用**：

| 应用场景 | Notebook | 分类类别 |
|---------|----------|---------|
| 表面干湿检测 | `pytorch_fp_model-DRY-WET.ipynb` | 干/混合/湿 |
| 草地检测 | `pytorch_fp_model-GRASS-NOTGRASS.ipynb` | 草地/非草地 |
| 姿态检测 | （在examples中） | 站立/行走/坐/躺/跌倒 |

#### 3.4.2 表面分类源码

**工具路径**：
```
tools/kobuki_surface_classification_src/
```

**功能**：
- 🤖 机器人地面材质分类
- 📡 支持IWR6843和IWRL6432
- 🧠 深度学习模型部署

### 3.5 控制与调试工具

#### 3.5.1 mmWave Studio CLI

**工具路径**：
```
tools/studio_cli/
```

**功能定位**：
- 🖥️ 命令行版mmWave Studio
- 📜 支持脚本自动化
- 🔧 批量配置和测试

**支持芯片**：
```
src/
├── 1443/    ← IWR1443
├── 1642/    ← IWR1642
├── 1843/    ← IWR1843
├── 2544/    ← AWR2544
├── 2944/    ← AWR2944
├── 6843/    ← AWR6843/IWR6843
└── common/  ← 通用代码
```

#### 3.5.2 VSCode CLI命令工具

**工具路径**：
```
tools/VSCode_CLI_command_utility/
```

**功能**：
- 💻 在VSCode中直接发送CLI命令
- 🔄 实时配置雷达参数
- 📊 查看返回结果

#### 3.5.3 远程访问工具

**工具路径**：
```
tools/remote_access/
```

**功能**：
- 🌐 通过网络远程控制雷达
- 📡 支持多设备管理
- 🔐 安全连接

### 3.6 工程辅助工具

#### 3.6.1 感知估算器

**工具路径**：
```
tools/sensing_estimator/
```

**功能定位**：
- 📐 参数规划工具
- 🎯 根据需求估算雷达配置
- 📊 性能预测

**典型输入**：
```
- 检测距离需求：6米
- 角度范围：±60°
- 距离分辨率：5cm
- 速度分辨率：0.1m/s
```

**输出建议**：
```
- 推荐芯片型号
- Chirp参数配置
- 天线配置
- 预期性能
```

#### 3.6.2 内存压缩工具

**工具路径**：
```
tools/memory_compression/
```

**功能**：
- 💾 减少固件存储空间
- 🗜️ QSPI Flash优化
- 📦 支持多种压缩算法

#### 3.6.3 脚本库

**工具路径**：
```
tools/scripts/
├── matlab_postProc_examples/    ← MATLAB后处理脚本
└── mmWaveStudio_lua_examples/   ← mmWave Studio Lua脚本
```

**MATLAB脚本功能**：
```matlab
% 示例：xWRL6844后处理
- Range-FFT处理
- Doppler-FFT处理
- DOA估计
- 点云生成
```

**Lua脚本功能**：
```lua
-- 示例：自动化测试
- 批量参数扫描
- 性能测试
- 数据记录
```

---

## 第四章：可视化工具系统

### 4.1 可视化工具总览

**Radar Toolbox提供18个专用GUI工具**：

```
tools/visualizers/
├── 🚗 汽车类 (7个)
│   ├── Automotive_Visualiser              ← 通用汽车可视化
│   ├── Automotive_Obstacle_Detection_GUI  ← 障碍物检测
│   ├── Automated_Parking_GUI              ← 自动泊车
│   ├── MRR_BeamSteering_GUI               ← 中程雷达波束控制
│   ├── MRR_GUI                            ← 中程雷达
│   ├── SRR_GUI                            ← 短程雷达
│   └── mmwave_2_chip_cascade_visualizer   ← 双芯片级联
│
├── 🚙 车内感知类 (5个)
│   ├── AWRL6844_Incabin_GUI               ← AWRL6844车内通用
│   ├── InCabin_CPD_w_Classification_GUI   ← 车内人员分类
│   ├── InCabin_IntruderDetection_GUI      ← 入侵者检测
│   ├── InCabin_LifePresenceDetection_GUI  ← 生命存在检测
│   └── InCabin_TruckbedRadar_GUI          ← 卡车货箱雷达
│
├── 🏭 工业/个人电子类 (4个)
│   ├── Applications_Visualizer            ← ⭐ 通用应用可视化
│   │   ├── Industrial_Visualizer.exe      ← 工业应用
│   │   └── Body_And_Chassis_Visualizer.exe← 车身底盘
│   ├── Area_Scanner_GUI                   ← 区域扫描
│   ├── Doors_And_Gates_Visualizer         ← 门闸
│   └── Traffic_Monitoring_Visualizer      ← 交通监控
│
└── 🛠️ 专用工具类 (2个)
    ├── Parking_Garage_Visualizer          ← 停车场
    └── mmwave_can_visualizer              ← CAN总线可视化
```

### 4.2 Applications_Visualizer（通用应用可视化）⭐

**工具路径**：
```
tools/visualizers/Applications_Visualizer/
```

**这是最重要的可视化工具**，分为两个子工具：

#### 4.2.1 Industrial_Visualizer（工业可视化）

**可执行文件**：
```
Industrial_Visualizer/Industrial_Visualizer.exe
```

**支持的应用Demo**：

| Demo类型 | 说明 | 典型应用 |
|---------|------|---------|
| **People Tracking** | 人员跟踪 | 楼宇占用检测、智能照明 |
| **Vital Signs** | 生命体征 | 呼吸心跳监测 |
| **Gesture Recognition** | 手势识别 | 智能家居控制 |
| **Fall Detection** | ⭐ 跌倒检测 | 老人监护、医疗 |
| **Level Sensing** | 液位检测 | 储罐监测 |
| **Area Scanner** | 区域扫描 | 安防监控 |
| **Traffic Monitoring** | 交通监控 | 车流统计 |

**界面功能**：

```
┌─────────────────────────────────────────────────────┐
│  Industrial Visualizer                              │
├─────────────────────────────────────────────────────┤
│  [连接设置]                                           │
│  ├─ COM Port: COM3                                  │
│  ├─ Baudrate: 115200                                │
│  └─ 配置文件: Browse...                              │
├─────────────────────────────────────────────────────┤
│  [3D点云显示]                                         │
│  ├─ X-Y平面图                                        │
│  ├─ 距离-高度图                                       │
│  └─ 实时轨迹                                         │
├─────────────────────────────────────────────────────┤
│  [目标信息]                                           │
│  ├─ 目标ID: 1                                        │
│  ├─ 位置: (x, y, z)                                 │
│  ├─ 速度: vx, vy                                    │
│  └─ 状态: Standing/Walking/Falling                  │
├─────────────────────────────────────────────────────┤
│  [统计信息]                                           │
│  ├─ 检测目标数: 3                                    │
│  ├─ 帧率: 15 FPS                                    │
│  └─ 丢包率: 0.5%                                     │
└─────────────────────────────────────────────────────┘
```

#### 4.2.2 Body_And_Chassis_Visualizer（车身底盘）

**可执行文件**：
```
Body_And_Chassis_Visualizer/Body_and_Chassis_Visualizer.exe
```

**支持的应用**：
- 🚗 车门开启检测
- 🚙 免提门禁
- 🛡️ 车身周围障碍物检测

### 4.3 车内感知可视化工具

#### 4.3.1 AWRL6844车内通用GUI

**工具路径**：
```
tools/visualizers/AWRL6844_Incabin_GUI/
```

**功能特点**：
- 🎯 专为AWRL6844优化
- 📊 4TX4RX MIMO显示
- 🔧 包含配置文件和复位代码

**目录结构**：
```
AWRL6844_Incabin_GUI/
├── src/
│   ├── chirpConfigs6844/     ← 配置文件
│   ├── resetCode/            ← 复位工具
│   └── utilityFiles/         ← 工具函数
├── docs/                     ← 使用文档
└── AWRL6844_Incabin_GUI.py   ← 主程序
```

#### 4.3.2 车内人员分类GUI

**工具路径**：
```
tools/visualizers/InCabin_CPD_w_Classification_GUI/
```

**功能**：
- 👤 成人/儿童分类
- 📍 座位位置识别
- 🎯 人员计数

#### 4.3.3 入侵者检测GUI

**工具路径**：
```
tools/visualizers/InCabin_IntruderDetection_GUI/
```

**功能**：
- 🚨 车内异常活动检测
- 🔐 防盗报警
- 📹 配合安防系统

**配置文件示例**：
```
config_file/major_motion_intruder_aopevm_front.cfg
```

### 4.4 ADAS可视化工具

#### 4.4.1 自动泊车GUI

**工具路径**：
```
tools/visualizers/Automated_Parking_GUI/
```

**显示内容**：
- 🅿️ 停车位检测
- 📏 障碍物距离
- 🎯 泊车路径规划

#### 4.4.2 中程雷达GUI (MRR)

**工具路径**：
```
tools/visualizers/MRR_GUI/
```

**应用场景**：
- 🚗 自适应巡航控制 (ACC)
- 🛑 自动紧急制动 (AEB)
- ⚠️ 前向碰撞预警 (FCW)

**显示特点**：
- 📊 距离-速度图
- 🎯 多目标跟踪
- ⚡ 实时更新 (>20 FPS)

#### 4.4.3 短程雷达GUI (SRR)

**工具路径**：
```
tools/visualizers/SRR_GUI/
```

**应用场景**：
- 👁️ 盲点检测 (BSD)
- 🚪 车道变换辅助 (LCA)
- ⚠️ 后方交叉交通警报 (RCTA)

### 4.5 工业应用可视化

#### 4.5.1 门闸可视化

**工具路径**：
```
tools/visualizers/Doors_And_Gates_Visualizer/
```

**功能**：
- 🚪 自动门开启控制
- 👤 接近速度检测
- 🛡️ 防夹检测

#### 4.5.2 交通监控可视化

**工具路径**：
```
tools/visualizers/Traffic_Monitoring_Visualizer/
```

**功能**：
- 🚗 车流量统计
- 📊 速度监测
- 🎯 车辆分类（小车/卡车）

#### 4.5.3 停车场可视化

**工具路径**：
```
tools/visualizers/Parking_Garage_Visualizer/
```

**功能**：
- 🅿️ 车位占用检测
- 📍 车辆定位
- 📊 停车场利用率

### 4.6 可视化工具使用流程

**标准使用流程**：

```
步骤1：硬件连接
├─ 雷达模块通过USB连接PC
├─ 确认COM口号（如COM3）
└─ 上电雷达

步骤2：启动可视化工具
├─ 运行对应的.exe或.py文件
├─ Industrial_Visualizer.exe（工业应用）
└─ Body_and_Chassis_Visualizer.exe（汽车应用）

步骤3：配置连接
├─ 选择COM口
├─ 设置波特率（通常115200）
└─ 加载配置文件（.cfg）

步骤4：烧录固件
├─ 选择预编译固件（.bin）
├─ 点击"Flash"烧录
└─ 等待烧录完成

步骤5：开始采集
├─ 点击"Connect"连接
├─ 点击"Start"开始
└─ 实时查看点云和目标信息

步骤6：数据记录（可选）
├─ 点击"Record"录制
├─ 保存为CSV或二进制
└─ 用于离线分析
```

**典型界面元素**：

```
┌─────────────────────────────────────────────────────┐
│  [1] 连接控制区                                       │
│  ├─ COM Port Selection                              │
│  ├─ Configuration File Browser                      │
│  ├─ Binary File Browser                             │
│  └─ Connect/Disconnect Buttons                      │
├─────────────────────────────────────────────────────┤
│  [2] 3D点云显示区                                     │
│  ├─ Top View (X-Y)                                  │
│  ├─ Side View (Range-Height)                        │
│  └─ 3D Perspective View                             │
├─────────────────────────────────────────────────────┤
│  [3] 目标列表区                                       │
│  ├─ Target ID                                       │
│  ├─ Position (x, y, z)                              │
│  ├─ Velocity (vx, vy)                               │
│  └─ Classification (如果支持)                        │
├─────────────────────────────────────────────────────┤
│  [4] 控制按钮区                                       │
│  ├─ Start/Stop                                      │
│  ├─ Record/Playback                                 │
│  └─ Settings                                        │
├─────────────────────────────────────────────────────┤
│  [5] 状态栏                                          │
│  ├─ Frame Rate                                      │
│  ├─ Target Count                                    │
│  └─ Connection Status                               │
└─────────────────────────────────────────────────────┘
```

---

## 第五章：算法库与源码示例

### 5.1 算法库总览

**Radar Toolbox提供三大核心算法库**：

```
source/ti/alg/
└── gtrack/                    ← 目标跟踪算法库
    ├── include/              ← 头文件
    ├── lib/                  ← 预编译库
    ├── src/                  ← 源码
    └── test/                 ← 测试用例

source/ti/common/
└── gesture_recognition/      ← 手势识别算法
    └── datapath/            ← 数据处理管道
```

**源码示例完整列表**（27+个）：

```
source/ti/examples/
├── 🚗 Automotive_ADAS_and_Parking/ (6个)
├── 🚙 Automotive_HandsFree_Access/ (2个)
├── 🚗 Automotive_InCabin_Security_and_Safety/ (3个)
├── 🎓 Fundamentals/ (基础示例)
├── 🏭 Industrial_and_Personal_Electronics/ (21个)
└── 📦 Out_Of_Box_Demo/ (开箱即用)
```

### 5.2 GTrack目标跟踪算法 ⭐

#### 5.2.1 GTrack简介

**官方名称**：Group Tracker (GTrack)

**核心功能**：
- 🎯 多目标跟踪算法
- 📊 支持2D和3D跟踪
- 🔄 卡尔曼滤波预测
- 🧮 数据关联和航迹管理

**算法特点**：

```
✅ 实时性能：支持>100个点云，15+ FPS
✅ 鲁棒性：处理遮挡、目标合并/分离
✅ 可配置：灵活的参数调优
✅ 多场景：人员、车辆、工业目标
```

#### 5.2.2 GTrack源码结构

**核心文件**（19个C文件）：

| 文件名 | 功能 | 说明 |
|--------|------|------|
| `gtrack_create.c` | 创建跟踪器实例 | 初始化内存、配置参数 |
| `gtrack_delete.c` | 删除跟踪器实例 | 释放资源 |
| `gtrack_step.c` | ⭐ 主处理循环 | 每帧调用一次 |
| `gtrack_module.c` | 模块管理 | 状态机控制 |
| `gtrack_unit_predict.c` | 预测步骤 | 卡尔曼滤波预测 |
| `gtrack_unit_update.c` | 更新步骤 | 测量更新 |
| `gtrack_unit_score.c` | 评分 | 数据关联得分计算 |
| `gtrack_unit_event.c` | 事件处理 | 目标生成/删除事件 |
| `gtrack_unit_report.c` | 报告输出 | 生成跟踪结果 |
| `gtrack_utilities_2d.c` | 2D工具函数 | 2D跟踪辅助函数 |
| `gtrack_utilities_3d.c` | 3D工具函数 | 3D跟踪辅助函数 |
| `gtrack_math.c` | 数学运算 | 矩阵运算、三角函数 |
| `gtrack_listlib.c` | 链表库 | 航迹管理 |

**头文件**：

```c
// 主头文件
gtrack.h              ← API接口定义

// 内部头文件
include/
├── gtrack_2d.h       ← 2D跟踪内部结构
├── gtrack_3d.h       ← 3D跟踪内部结构
├── gtrack_int.h      ← 内部数据结构
└── gtrack_listlib.h  ← 链表操作
```

**预编译库**：

```
lib/
├── libgtrack.ae674       ← C674x DSP库（2D）
├── libgtrack.aer4f       ← R4F ARM库（2D）
├── libgtrack3D.ae674     ← C674x DSP库（3D）
└── libgtrack3D.aer4f     ← R4F ARM库（3D）
```

#### 5.2.3 GTrack工作流程

**算法流程图**：

```
输入：点云数据 (x, y, z, v)
    ↓
┌──────────────────────────────┐
│  步骤1：预测 (Predict)         │
│  - 使用运动模型预测目标位置      │
│  - 卡尔曼滤波预测              │
└──────────────────────────────┘
    ↓
┌──────────────────────────────┐
│  步骤2：关联 (Associate)       │
│  - 计算点云到航迹的距离          │
│  - 评分和匹配                  │
└──────────────────────────────┘
    ↓
┌──────────────────────────────┐
│  步骤3：更新 (Update)          │
│  - 卡尔曼滤波更新              │
│  - 更新航迹状态                │
└──────────────────────────────┘
    ↓
┌──────────────────────────────┐
│  步骤4：管理 (Manage)          │
│  - 创建新航迹                  │
│  - 删除丢失航迹                │
│  - 航迹合并/分离              │
└──────────────────────────────┘
    ↓
输出：跟踪目标 (ID, 位置, 速度)
```

**核心API**：

```c
// 1. 创建跟踪器实例
void *gtrack_create(GTRACK_moduleConfig *config, int32_t *errCode);

// 2. 主处理函数（每帧调用）
void gtrack_step(
    void *handle,                    // 跟踪器句柄
    GTRACK_measurementPoint *points, // 输入点云
    uint16_t numPoints,              // 点云数量
    GTRACK_targetDesc *targets,      // 输出目标
    uint16_t *numTargets             // 目标数量
);

// 3. 删除跟踪器
void gtrack_delete(void *handle);
```

**配置参数示例**：

```c
GTRACK_moduleConfig config = {
    .maxNumPoints = 500,           // 最大点数
    .maxNumTracks = 20,            // 最大航迹数
    .stateVectorType = GTRACK_STATE_VECTORS_3DA,  // 3D + 加速度
    .verbose = GTRACK_VERBOSE_NONE,
    .initialRadialVelocity = 0.0f,
    .maxAcceleration = 2.0f,       // 最大加速度 (m/s²)
    .maxRadialVelocity = 10.0f,    // 最大径向速度 (m/s)
    .radialVelocityResolution = 0.1f,
    .deltaT = 0.05f                // 帧间隔 (s)
};
```

#### 5.2.4 GTrack测试用例

**测试目录**：
```
source/ti/alg/gtrack/test/
├── common/                  ← 通用测试代码
├── usecases/               ← 应用场景测试
│   ├── dss/               ← DSP测试
│   └── mss/               ← ARM测试
└── vectors/               ← 测试向量
    └── usecases/
        └── people_counting/  ← 人员计数测试数据
```

### 5.3 手势识别算法

#### 5.3.1 手势识别架构

**代码路径**：
```
source/ti/common/gesture_recognition/
```

**支持芯片**：
```
├── xwrL14xx-evm/          ← AWRL1432、IWRL1432
└── xwrL64xx-evm/          ← AWRL6432、IWRL6432
```

**核心模块**：

```
gesture_recognition/
└── datapath/
    ├── dpu/                      ← Data Processing Unit
    │   └── aoaproc/             ← 角度处理
    │       └── v0/              ← 版本0
    └── lib/                      ← 预编译库
```

#### 5.3.2 手势识别示例

**示例路径**：
```
source/ti/examples/Industrial_and_Personal_Electronics/Gesture_Recognition/
```

**核心源码文件**：

| 文件名 | 功能 |
|--------|------|
| `main.c` | 主程序入口 |
| `gesture.c` | ⭐ 手势识别逻辑 |
| `data_path.c` | 数据处理管道 |
| `objectdetection.c` | 目标检测 |
| `rangeprochwa.c` | Range FFT处理（硬件加速）|
| `dopplerprochwa.c` | Doppler FFT处理（硬件加速）|
| `classifier_prepost_proc.c` | 分类器前后处理 |
| `ann_utils.c` | 神经网络工具 |
| `cli.c` | 命令行接口 |

**手势分类**：
```
支持的手势：
├── Swipe Left   ← 向左滑动
├── Swipe Right  ← 向右滑动
├── Swipe Up     ← 向上滑动
├── Swipe Down   ← 向下滑动
├── Circle CW    ← 顺时针画圈
└── Circle CCW   ← 逆时针画圈
```

### 5.4 源码示例详解

#### 5.4.1 工业与个人电子示例（21个）

**完整列表**：

| 序号 | 示例名称 | 应用场景 | 芯片支持 |
|------|---------|---------|---------|
| 1 | **Pose_And_Fall_Detection** | ⭐ 姿态与跌倒检测 | xWRL6432 |
| 2 | People_Tracking | 人员跟踪 | 多芯片 |
| 3 | Vital_Signs | 生命体征 | xWR68xx |
| 4 | Gesture_Recognition | 手势识别 | xWRL14xx, xWRL64xx |
| 5 | Motion_and_Presence_Detection | 运动和存在检测 | xWRL64xx |
| 6 | Area_Scanner | 区域扫描 | xWR18xx |
| 7 | Automated_Doors_And_Gates | 自动门闸 | xWR64xx |
| 8 | Level_Sensing | 液位检测 | xWRL14xx |
| 9 | Long_Range_People_Detection | 远程人员检测 | xWR68xx |
| 10 | Traffic_Monitoring | 交通监控 | xWR18xx |
| 11 | Parking_Garage_Sensor | 停车场传感器 | xWR64xx |
| 12 | Overhead_Lighting | 顶灯控制 | xWRL64xx |
| 13 | Smart_Toilet | 智能马桶 | xWRL64xx |
| 14 | Video_Doorbell | 可视门铃 | xWR68xx |
| 15 | Bike_Radar | 自行车雷达 | xWR18xx |
| 16 | Robotics | 机器人应用 | 多芯片 |
| 17 | True_Ground_Speed | 真实地速 | xWR18xx |
| 18 | 1D_Sensing | 一维感知 | xWRL14xx |
| 19 | human_non-human_classification | 人类/非人类分类 | xWRL64xx |
| 20 | Machine_Learning_Examples | 机器学习示例 | xWRL64xx |
| 21 | mmWave_Demo | 通用Demo | 多芯片 |

#### 5.4.2 汽车应用示例（11个）

**ADAS与停车**（6个）：

| 示例名称 | 应用场景 |
|---------|---------|
| automated_parking | 自动泊车 |
| awr2944_safety_demo | 安全Demo（AWR2944）|
| awrl1432_entry_level_blind_spot_detection | 入门级盲点检测 |
| high_end_corner_radar | 高端角雷达 |
| medium_range_radar | 中程雷达 |
| short_range_radar | 短程雷达 |

**车内安全**（3个）：

| 示例名称 | 应用场景 |
|---------|---------|
| AWRL6844_InCabin_Demos | AWRL6844车内Demo集合 |
| Exterior_Intrusion_Monitoring | 外部入侵监控 |
| child_presence_detection | 儿童存在检测 |

**免提门禁**（2个）：

| 示例名称 | 应用场景 |
|---------|---------|
| hands_free_access | 免提门禁 |
| kick_detection | 踢腿检测 |

#### 5.4.3 Out_Of_Box_Demo（开箱即用）⭐

**路径**：
```
source/ti/examples/Out_Of_Box_Demo/
```

**支持芯片**（15个）：

```
src/
├── xwr1443/      ← IWR1443
├── xwr1642/      ← IWR1642
├── xwr1843/      ← IWR1843
├── xwr6443/      ← IWR6443
├── xwr6843AOP/   ← AWR6843 (AOP封装)
├── xwr6843ISK/   ← IWR6843 (ISK)
├── xwr6843ODS/   ← AWR6843 (ODS)
├── xwrL1432/     ← AWRL1432/IWRL1432
├── xwrL6432/     ← AWRL6432/IWRL6432
├── xwrL6432AOP/  ← AWRL6432 (AOP)
├── awr2544/      ← AWR2544
├── awr294x/      ← AWR2944
├── awr2x44ECO/   ← AWR2243/AWR2443 ECO
├── awr2x44LC/    ← AWR2243/AWR2443 LC
└── awr2x44p/     ← AWR2243/AWR2443 P
```

**OOB Demo功能**：

```
开箱即用Demo提供：
├── 基础点云检测
├── GTrack目标跟踪
├── UART输出格式化
├── CLI命令接口
└── 标准配置文件

适用场景：
✅ 快速验证硬件
✅ 学习雷达基础
✅ 二次开发基础
```

#### 5.4.4 标准示例目录结构

**典型示例项目结构**：

```
<Example_Name>/
├── 📁 docs/                      ← 文档
│   ├── images/                  ← 图片资源
│   └── <Example>_User_Guide.pdf ← 用户指南
│
├── 📁 prebuilt_binaries/         ← 预编译固件
│   ├── <chip>_demo.bin          ← 可直接烧录
│   └── <chip>_demo_meta.bin     ← 元数据
│
├── 📁 chirp_configs/             ← 配置文件
│   ├── profile_1.cfg            ← 配置方案1
│   └── profile_2.cfg            ← 配置方案2
│
├── 📁 src/                       ← 源代码
│   ├── <chip>/                  ← 芯片特定代码
│   │   ├── dss/                ← DSP子系统
│   │   ├── mss/                ← ARM子系统
│   │   └── common/             ← 共用代码
│   └── utils/                   ← 工具函数
│
└── 📄 README.md                  ← 项目说明
```

### 5.5 源码编译与集成

#### 5.5.1 编译依赖

**必需工具**：
```
1. Code Composer Studio (CCS) 12.0+
2. MMWAVE_L_SDK 06.01.00.01
3. TI ARM Compiler 或 GCC ARM
4. (可选) C674x Compiler (DSP芯片)
```

**编译流程**：

```
步骤1：导入项目
├─ CCS → Import → Browse
└─ 选择 src/<chip>/ 目录

步骤2：配置SDK路径
├─ Project Properties
├─ Resource → Linked Resources
└─ 设置 MMWAVE_L_SDK_INSTALL_PATH

步骤3：选择构建配置
├─ Debug (调试版本)
└─ Release (发布版本)

步骤4：编译
├─ Build Project
└─ 生成 .bin 文件

步骤5：烧录测试
├─ 使用 UniFlash 或 JTAG_Flasher
└─ 运行验证
```

#### 5.5.2 集成到自定义项目

**集成GTrack算法**：

```c
// 1. 包含头文件
#include <ti/alg/gtrack/gtrack.h>

// 2. 链接库文件
// Project Properties → Linker → File Search Path
// 添加：${RADAR_TOOLBOX}/source/ti/alg/gtrack/lib/libgtrack.aer4f

// 3. 初始化
GTRACK_moduleConfig config = {...};
void *gtrackHandle = gtrack_create(&config, &errCode);

// 4. 每帧调用
gtrack_step(gtrackHandle, points, numPoints, targets, &numTargets);

// 5. 清理
gtrack_delete(gtrackHandle);
```

**集成手势识别**：

```c
// 1. 包含头文件
#include <ti/common/gesture_recognition/gesture.h>

// 2. 初始化手势识别
Gesture_Init(&config);

// 3. 处理手势
GestureType gesture = Gesture_Classify(features);

// 4. 根据手势执行动作
switch(gesture) {
    case GESTURE_SWIPE_LEFT:
        // 执行向左操作
        break;
    case GESTURE_SWIPE_RIGHT:
        // 执行向右操作
        break;
    // ...
}
```

### 5.6 性能优化建议

#### 5.6.1 GTrack优化参数

**针对不同场景的优化**：

| 场景 | maxNumPoints | maxNumTracks | deltaT | 说明 |
|------|--------------|--------------|--------|------|
| 室内人员检测 | 200-300 | 10-15 | 50ms | 中等复杂度 |
| 停车场监控 | 100-150 | 5-8 | 100ms | 低复杂度 |
| 高速公路 | 500+ | 20-30 | 50ms | 高复杂度 |
| 跌倒检测 | 150-200 | 3-5 | 33ms | 快速响应 |

#### 5.6.2 内存优化

**减少内存占用**：
```c
// 方法1：减少最大点数和航迹数
config.maxNumPoints = 200;   // 默认500
config.maxNumTracks = 10;    // 默认20

// 方法2：使用简单状态向量
config.stateVectorType = GTRACK_STATE_VECTORS_2D; // 2D而非3D

// 方法3：禁用详细日志
config.verbose = GTRACK_VERBOSE_NONE;
```

#### 5.6.3 实时性能优化

**提高帧率**：
```c
// 1. 使用硬件加速
// 启用HWA进行FFT计算

// 2. 优化点云预处理
// 在CFAR前过滤明显噪声点

// 3. 减少跟踪复杂度
// 限制搜索范围和关联矩阵大小

// 4. 使用DSP并行处理
// 将Range/Doppler/Angle处理分配到DSS
```

---

*（第三部分完成，包含算法库与源码示例详解。第五章已完成！）*

