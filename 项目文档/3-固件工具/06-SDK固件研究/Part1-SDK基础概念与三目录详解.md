# 📚 AWRL6844 SDK基础概念与三目录详解

> **文档版本**: v1.0  
> **创建日期**: 2025-12-25  
> **适用硬件**: AWRL6844-EVM  
> **SDK版本**: MMWAVE_L_SDK 06.01.00.01 / radar_toolbox 3.30.00.06 / mmwave_studio 04.03.01.00

---

## 📋 目录

- [第一章：SDK基础概念](#第一章sdk基础概念)
- [第二章：MMWAVE_L_SDK详解](#第二章mmwave_l_sdk详解)
- [第三章：radar_toolbox详解](#第三章radar_toolbox详解)
- [第四章：mmwave_studio详解](#第四章mmwave_studio详解)
- [第五章：三者对比与选择](#第五章三者对比与选择)

---

## 第一章：SDK基础概念

### 1.1 什么是SDK？

**SDK（Software Development Kit）** = **软件开发工具包**

**定义**：
- SDK是一组用于开发特定平台应用的**工具集合**
- 包含：库文件、示例代码、文档、编译工具、调试工具等
- 目的：简化开发流程，提供标准化开发环境

**对于TI毫米波雷达**：
```
SDK = 固件源码 + 编译工具链 + 示例Demo + 配置文件 + 调试工具 + 文档
```

### 1.2 TI雷达SDK生态系统

TI为AWRL6844提供了**三个不同目的**的SDK：

| SDK名称 | 核心功能 | 主要用户 | 典型场景 |
|--------|---------|---------|---------|
| **MMWAVE_L_SDK** | 固件开发 + 烧录 | 固件开发者 | 编译固件、烧录固件、调试底层代码 |
| **radar_toolbox** | 应用Demo + 配置 | 应用开发者 | 快速测试、参数配置、可视化演示 |
| **mmwave_studio** | RF测试 + 标定 | 硬件工程师 | 射频调试、天线标定、原始数据采集 |

**三者关系**：
```
MMWAVE_L_SDK (底层开发)
    ↓ 编译生成
固件.appimage
    ↓ 使用
radar_toolbox (应用层)
    ↓ 测试
mmwave_studio (硬件层)
```

### 1.3 SDK与固件的关系

**核心关系**：
- **SDK** = 开发环境（工具 + 源码）
- **固件** = SDK编译的产物（.appimage二进制文件）

**类比说明**：
```
SDK     就像    厨房（工具 + 食材 + 菜谱）
固件    就像    成品菜（已烹饪好的食物）
```

**工作流程**：
```
1. 使用SDK开发
   ├─ 编写代码（修改示例或自定义）
   ├─ 配置参数（选择功能模块）
   └─ 编译构建（生成二进制）
   
2. 生成固件
   └─ .appimage文件（可烧录到Flash）
   
3. 烧录运行
   ├─ 使用SDK自带烧录工具
   └─ 固件在芯片上独立运行
```

---

## 第二章：MMWAVE_L_SDK详解

### 2.1 MMWAVE_L_SDK概述

**官方名称**：mmWave Low-Power SDK  
**版本示例**：MMWAVE_L_SDK_06_01_00_01  
**核心定位**：**固件开发和烧录的主要SDK**

**安装路径**：
```
C:\ti\MMWAVE_L_SDK_06_01_00_01\
```

### 2.2 目录结构（7层架构）

```
C:\ti\MMWAVE_L_SDK_06_01_00_01\
│
├── 📂 examples/                      ← ⭐ 核心：示例固件源码
│   ├── mmw_demo/                     # 多模式雷达Demo
│   │   └── xwrL684x-evm/            # 6844评估板专用
│   │       └── mmwave_demo.release.appimage  ← 固件成品
│   │
│   ├── hello_world/                  # Hello World入门示例
│   ├── drivers/                      # 驱动层示例
│   │   └── boot/sbl/                # SBL启动加载器
│   └── ...（更多示例）
│
├── 📂 tools/                         ← ⭐ 核心：开发工具
│   ├── FlashingTool/                 # 固件烧录工具
│   │   ├── arprog_cmdline_6844.exe  ← 命令行烧录工具
│   │   └── flasher_config.xml       # 烧录配置
│   │
│   ├── visualizer/                   # 可视化工具
│   │   ├── visualizer.exe           ← PC端可视化软件
│   │   └── tmp/                     # 临时配置文件
│   │
│   ├── ccs_load/                     # CCS调试配置
│   │   └── xwrl684x.ccxml          ← UniFlash使用的配置
│   │
│   └── buildImage_creator/          # 固件打包工具
│
├── 📂 mmwave_l_sdk_06_01_00_01/      ← 源码和库文件
│   ├── packages/                     # 驱动和中间件源码
│   ├── build/                        # 编译脚本
│   └── docs/                         # API文档
│
├── 📄 readme.md                      # SDK说明文档
├── 📄 release_notes.pdf              # 版本更新说明
└── 📄 manifest.html                  # 组件清单

```

### 2.3 核心功能模块

#### 2.3.1 固件示例（examples/）

**最重要的示例**：
```
examples/mmw_demo/xwrL684x-evm/
└── mmwave_demo.release.appimage     ← Multi-Image格式（推荐）
```

**特点**：
- ✅ Multi-Image格式（包含SBL）
- ✅ 可直接烧录到0x0地址
- ✅ 支持22条CLI命令
- ✅ 兼容radar_toolbox配置文件

**其他示例**：
| 示例名称 | 功能说明 | 适用场景 |
|---------|---------|---------|
| `hello_world` | 最简单的启动示例 | 学习芯片启动流程 |
| `adc_data_capture` | ADC数据采集 | 原始数据分析 |
| `people_tracking` | 人员跟踪 | 室内监测应用 |
| `overhead_occupancy` | 顶置占用检测 | 会议室/停车场 |

#### 2.3.2 烧录工具（tools/FlashingTool/）

**核心工具**：`arprog_cmdline_6844.exe`

**使用方法**：
```powershell
# Multi-Image固件烧录（推荐）
.\arprog_cmdline_6844.exe `
    -i mmwave_demo.release.appimage `
    -d xwrl684x `
    -o 0x0

# 传统SBL + App方式
.\arprog_cmdline_6844.exe -i sbl.appimage -d xwrl684x -o 0x2000
.\arprog_cmdline_6844.exe -i app.appimage -d xwrl684x -o 0x42000
```

**参数说明**：
- `-i`: 固件文件路径
- `-d`: 设备类型（xwrl684x）
- `-o`: Flash偏移地址（十六进制）

#### 2.3.3 可视化工具（tools/visualizer/）

**工具名称**：`visualizer.exe`

**功能**：
- 🎯 实时显示雷达检测目标
- 📊 显示距离-多普勒热图
- 📈 显示点云数据
- 🔄 支持多种显示模式

**使用流程**：
```
1. 烧录固件 → mmwave_demo.release.appimage
2. 连接串口 → CLI端口 + 数据端口
3. 启动可视化 → visualizer.exe
4. 加载配置 → 选择.cfg文件
5. 发送配置 → 通过CLI端口
6. 启动雷达 → sensorStart
7. 接收数据 → 实时显示
```

### 2.4 MMWAVE_L_SDK的典型用途

#### 用途1：快速测试雷达功能 ⭐⭐⭐

**步骤**：
```bash
1. 烧录Demo固件
   arprog_cmdline_6844.exe -i mmwave_demo.release.appimage -d xwrl684x -o 0x0

2. 配置雷达参数
   使用visualizer.exe加载配置文件
   或使用串口工具发送CLI命令

3. 启动雷达
   sensorStart

4. 观察数据
   在visualizer中查看目标检测结果
```

**适用场景**：
- ✅ 验证硬件是否正常工作
- ✅ 快速评估雷达性能
- ✅ 测试不同配置参数效果

#### 用途2：开发自定义固件 ⭐⭐

**步骤**：
```
1. 安装开发环境
   - Code Composer Studio (CCS)
   - ti-arm-clang编译器

2. 修改示例代码
   examples/mmw_demo/mss/mmw_cli.c  ← 添加自定义命令
   examples/mmw_demo/dss/dss_main.c ← 修改算法逻辑

3. 编译固件
   使用CCS或命令行编译

4. 生成appimage
   使用buildImage_creator工具打包

5. 烧录测试
   arprog_cmdline_6844.exe烧录
```

**适用场景**：
- ✅ 添加自定义算法
- ✅ 修改数据输出格式
- ✅ 优化性能参数

#### 用途3：固件更新和维护 ⭐⭐⭐

**场景**：
- 批量生产时的固件烧录
- 现场设备的固件升级
- 故障设备的固件恢复

**工具链**：
```
MMWAVE_L_SDK提供的烧录工具
    ↓
支持脚本自动化
    ↓
可集成到生产系统
```

### 2.5 MMWAVE_L_SDK关键文件

| 文件路径 | 文件作用 | 重要性 |
|---------|---------|-------|
| `examples/mmw_demo/.../mmwave_demo.release.appimage` | 标准Demo固件 | ⭐⭐⭐ |
| `tools/FlashingTool/arprog_cmdline_6844.exe` | 烧录工具 | ⭐⭐⭐ |
| `tools/visualizer/visualizer.exe` | 可视化软件 | ⭐⭐⭐ |
| `tools/ccs_load/xwrl684x.ccxml` | UniFlash配置 | ⭐⭐ |
| `examples/drivers/boot/sbl/...` | SBL源码 | ⭐⭐ |

---

## 第三章：radar_toolbox详解

### 3.1 radar_toolbox概述

**官方名称**：mmWave Radar Toolbox  
**版本示例**：radar_toolbox_3_30_00_06  
**核心定位**：**应用Demo和配置文件库**

**安装路径**：
```
C:\ti\radar_toolbox_3_30_00_06\
```

### 3.2 目录结构（5-6层架构）

```
C:\ti\radar_toolbox_3_30_00_06\
│
├── 📂 tools/                         ← ⭐ 核心：应用工具
│   ├── visualizers/                  # 各类可视化工具
│   │   ├── Industrial_Visualizer/   # 工业应用可视化
│   │   ├── Body_And_Chassis_Visualizer/  # 车载/人体检测
│   │   └── ...（更多专用可视化）
│   │
│   └── Adc_Data_Capture_Tool_DCA1000_CLI/  ← ⭐ 配置文件库
│       └── chirp_configs/            # 45个配置文件
│           ├── xWRL6844_4T4R_tdm.cfg      ← AWRL6844标准配置
│           ├── xWRL6844_2T4R_profile.cfg
│           └── ...（更多配置）
│
├── 📂 source/                        ← Demo源码（可选）
│   └── ti/examples/
│       ├── People_Tracking/
│       ├── Overhead_Occupancy/
│       └── ...
│
└── 📄 docs/                          # 应用文档

```

### 3.3 核心功能模块

#### 3.3.1 配置文件库 ⭐⭐⭐

**位置**：
```
tools/Adc_Data_Capture_Tool_DCA1000_CLI/chirp_configs/
```

**AWRL6844专用配置文件（45个）**：

**标准配置**：
| 配置文件名 | 天线配置 | 适用场景 |
|-----------|---------|---------|
| `xWRL6844_4T4R_tdm.cfg` | 4发4收TDM | 标准多目标检测 ⭐ |
| `xWRL6844_2T4R_profile.cfg` | 2发4收 | 低功耗应用 |
| `xWRL6844_1T4R_profile.cfg` | 1发4收 | 最低功耗 |

**专用配置**：
| 配置类型 | 文件示例 | 应用场景 |
|---------|---------|---------|
| 人员跟踪 | `people_tracking_*.cfg` | 室内人员监测 |
| 占用检测 | `occupancy_*.cfg` | 会议室/停车场 |
| 手势识别 | `gesture_*.cfg` | 人机交互 |
| 车载应用 | `automotive_*.cfg` | 车内监测 |

**配置文件格式示例**：
```cfg
% xWRL6844_4T4R_tdm.cfg - 标准配置

sensorStop
channelCfg 15 7 0              % 4TX 3RX使能
chirpComnCfg 0 0 0 0 0 0 0 1   % Chirp公共配置
chirpTimingCfg 1 1 0 0         % Timing配置
frameCfg 0 0 32 0 50 1 0       % 帧配置
sensorStart                    % 启动
```

#### 3.3.2 专用可视化工具

**工业可视化器**（Industrial_Visualizer）：
- 路径：`tools/visualizers/Applications_Visualizer/Industrial_Visualizer/`
- 功能：工厂自动化、人员检测、区域监控
- 特点：支持多区域配置、告警功能

**车载可视化器**（Body_And_Chassis_Visualizer）：
- 路径：`tools/visualizers/Applications_Visualizer/Body_and_Chassis_Visualizer/`
- 功能：车内人员检测、生命体征监测
- 特点：支持跌倒检测、姿态识别

### 3.4 radar_toolbox的典型用途

#### 用途1：快速加载标准配置 ⭐⭐⭐

**步骤**：
```
1. 从配置库选择配置文件
   chirp_configs/xWRL6844_4T4R_tdm.cfg

2. 通过串口发送配置
   使用任意串口工具（115200波特率）
   或使用visualizer.exe自动发送

3. 启动雷达
   配置最后一行：sensorStart

4. 接收数据
   数据端口（1250000波特率）接收点云数据
```

**优势**：
- ✅ 无需手动编写配置
- ✅ 参数已优化验证
- ✅ 支持多种应用场景

#### 用途2：参考配置开发 ⭐⭐

**场景**：
需要自定义配置时，以现有配置为模板

**方法**：
```
1. 找到相似场景的配置文件
   例如：人员跟踪场景 → people_tracking_6844.cfg

2. 复制并修改参数
   调整帧率、距离范围、角度范围等

3. 理解参数含义
   参考配置文件中的注释
   查阅SDK文档

4. 测试验证
   烧录固件 → 发送配置 → 观察效果
```

#### 用途3：应用Demo演示 ⭐⭐⭐

**场景**：
- 客户演示
- 功能评估
- 方案验证

**工具**：
```
Industrial_Visualizer
    ↓
专业的UI界面
    ↓
直观的目标显示
    ↓
支持录制回放
```

### 3.5 radar_toolbox关键文件

| 文件路径 | 文件作用 | 重要性 |
|---------|---------|-------|
| `tools/.../chirp_configs/xWRL6844_4T4R_tdm.cfg` | 标准配置文件 | ⭐⭐⭐ |
| `tools/visualizers/Industrial_Visualizer/` | 工业可视化 | ⭐⭐⭐ |
| `tools/visualizers/Body_And_Chassis_Visualizer/` | 车载可视化 | ⭐⭐ |
| `source/ti/examples/People_Tracking/` | 人员跟踪Demo源码 | ⭐⭐ |

---

## 第四章：mmwave_studio详解

### 4.1 mmwave_studio概述

**官方名称**：mmWave Studio (RadarStudio)  
**版本示例**：mmwave_studio_04_03_01_00  
**核心定位**：**RF测试、标定和原始数据采集工具**

**安装路径**：
```
C:\ti\mmwave_studio_04_03_01_00\
```

### 4.2 核心功能

#### 4.2.1 射频参数配置与测试

**功能**：
- 🔧 精确配置射频参数
- 📡 测试天线性能
- 🎛️ 调整发射功率
- 📊 测量接收灵敏度

**典型应用**：
```
1. 天线标定
   - 测量天线增益
   - 验证天线方向图
   - 评估副瓣抑制

2. RF性能测试
   - 发射功率测试
   - 接收灵敏度测试
   - 杂散信号测试
   - 谐波失真测试

3. 系统调试
   - 信号链路验证
   - 时序参数优化
   - 相位校准
```

#### 4.2.2 原始ADC数据采集

**功能**：
- 📊 采集未经处理的ADC数据
- 💾 保存为二进制文件
- 🔬 用于算法开发和验证

**工作流程**：
```
1. 配置数据采集卡（DCA1000EVM）
   连接雷达板 → 配置FPGA → 设置存储路径

2. 配置雷达参数
   使用mmWave Studio GUI配置Profile/Chirp/Frame

3. 触发采集
   启动雷达 → 采集数据 → 自动保存

4. 数据分析
   MATLAB/Python读取 → FFT处理 → 目标检测
```

**数据格式**：
```
原始ADC数据格式：
- 16位复数（I + Q）
- 按Chirp组织
- 按天线通道分离
- 可用于自定义算法开发
```

#### 4.2.3 LUA脚本自动化

**功能**：
- 🤖 自动化测试流程
- 📜 批量配置测试
- 🔄 重复性测试

**示例脚本**：
```lua
-- 配置Profile
ar1.ProfileConfig(0, 77, 400, 7, 60, 0, 0, 0, 0, 0, 0, 29.982, 0, 256, 5000, 0, 0, 30)

-- 配置Chirp
ar1.ChirpConfig(0, 0, 0, 0, 0, 0, 0, 0, 0, 0)

-- 配置Frame
ar1.FrameConfig(0, 0, 64, 0, 40, 0, 1)

-- 启动传感器
ar1.StartFrame()
```

### 4.3 mmwave_studio的典型用途

#### 用途1：硬件验证 ⭐⭐⭐

**场景**：
- PCB设计验证
- 天线性能测试
- 射频链路测试

**工具优势**：
- ✅ 直接访问底层硬件
- ✅ 精确控制RF参数
- ✅ 详细的测试报告

#### 用途2：算法开发 ⭐⭐

**场景**：
- 自定义信号处理算法
- 目标检测算法研究
- 性能优化研究

**工作流程**：
```
1. 采集原始数据
   mmWave Studio → DCA1000 → 保存ADC数据

2. 离线处理
   MATLAB/Python → 算法开发 → 性能评估

3. 固件集成
   将算法移植到DSP固件 → 实时运行

4. 验证测试
   mmWave Studio再次采集 → 对比算法效果
```

#### 用途3：生产测试 ⭐⭐

**场景**：
- 批量生产时的RF测试
- 出厂标定
- 质量检测

**自动化**：
```
LUA脚本
    ↓
自动化测试流程
    ↓
生成测试报告
    ↓
合格/不合格判定
```

### 4.4 mmwave_studio关键特性

| 特性 | 说明 | 适用场景 |
|-----|------|---------|
| GUI界面 | 图形化配置工具 | 快速测试 |
| LUA脚本 | 自动化脚本支持 | 批量测试 |
| DCA1000支持 | 高速数据采集 | 算法开发 |
| 寄存器操作 | 直接读写寄存器 | 底层调试 |
| 监控功能 | 实时监控RF状态 | 硬件验证 |

### 4.5 使用限制

**注意事项**：
- ⚠️ 主要用于**开发阶段**和**生产测试**
- ⚠️ **不适合**最终产品部署
- ⚠️ 需要额外硬件（DCA1000采集卡）
- ⚠️ 学习曲线较陡峭

**典型用户**：
- 硬件工程师
- RF工程师
- 算法研究人员
- 生产测试工程师

---

## 第五章：三者对比与选择

### 5.1 功能对比表

| 对比维度 | MMWAVE_L_SDK | radar_toolbox | mmwave_studio |
|---------|-------------|--------------|--------------|
| **核心功能** | 固件开发+烧录 | 应用Demo+配置 | RF测试+标定 |
| **目标用户** | 固件开发者 | 应用开发者 | 硬件工程师 |
| **学习难度** | ⭐⭐⭐ 高 | ⭐⭐ 中 | ⭐⭐⭐⭐ 很高 |
| **使用频率** | 开发初期+更新 | 日常开发 | 硬件验证阶段 |
| **是否必需** | ✅ 必需 | ⭐ 推荐 | ⚠️ 可选 |

### 5.2 内容对比表

| 内容类型 | MMWAVE_L_SDK | radar_toolbox | mmwave_studio |
|---------|-------------|--------------|--------------|
| **固件文件** | ✅ 完整源码+编译固件 | ❌ 无 | ❌ 无 |
| **配置文件** | ⭐ 少量示例 | ✅ 45个标准配置 | ✅ 可配置生成 |
| **烧录工具** | ✅ arprog_cmdline | ❌ 无 | ✅ 固件下载功能 |
| **可视化** | ✅ 通用visualizer | ✅ 专用可视化器 | ✅ 实时监控 |
| **源码** | ✅ 完整驱动+Demo | ⭐ 部分Demo | ❌ 无 |
| **文档** | ✅ API文档 | ✅ 应用文档 | ✅ 测试指南 |

### 5.3 使用场景选择指南

#### 场景1：快速评估硬件功能

**目标**：验证AWRL6844-EVM是否正常工作

**推荐方案**：
```
1️⃣ 使用MMWAVE_L_SDK
   - 烧录：mmwave_demo.release.appimage
   - 工具：arprog_cmdline_6844.exe

2️⃣ 使用radar_toolbox
   - 配置：xWRL6844_4T4R_tdm.cfg
   - 可视化：Industrial_Visualizer

3️⃣ 不需要mmwave_studio
```

**理由**：
- ✅ 标准固件已包含所有功能
- ✅ 标准配置已优化验证
- ✅ 可视化器可直观显示结果

#### 场景2：开发自定义应用

**目标**：在标准固件基础上添加自定义算法

**推荐方案**：
```
1️⃣ 主要使用MMWAVE_L_SDK
   - 修改源码：examples/mmw_demo/
   - 编译固件：CCS或命令行
   - 烧录测试：arprog_cmdline_6844.exe

2️⃣ 参考radar_toolbox
   - 配置模板：chirp_configs/*.cfg
   - 应用示例：source/ti/examples/

3️⃣ 可选mmwave_studio（算法开发）
   - 采集数据：DCA1000
   - 离线分析：MATLAB/Python
```

**理由**：
- ✅ 需要修改固件源码
- ✅ 可参考标准配置和示例
- ✅ 算法开发需要原始数据

#### 场景3：硬件设计验证

**目标**：验证自己设计的PCB和天线性能

**推荐方案**：
```
1️⃣ 主要使用mmwave_studio
   - RF测试：天线增益、方向图
   - 数据采集：DCA1000原始数据
   - 标定：相位、幅度校准

2️⃣ 辅助MMWAVE_L_SDK
   - 烧录固件：用于功能测试

3️⃣ 参考radar_toolbox
   - 配置文件：用于性能对比
```

**理由**：
- ✅ mmwave_studio提供底层RF控制
- ✅ 可直接读写寄存器
- ✅ 支持详细的测试报告

#### 场景4：批量生产

**目标**：批量烧录固件并测试

**推荐方案**：
```
1️⃣ 使用MMWAVE_L_SDK
   - 烧录脚本：arprog_cmdline_6844.exe自动化
   - 固件管理：版本控制和发布

2️⃣ 可选mmwave_studio
   - 生产测试：LUA脚本自动化
   - 质量检测：RF参数测试

3️⃣ 不需要radar_toolbox
```

**理由**：
- ✅ 烧录工具支持脚本自动化
- ✅ mmwave_studio可自动化测试
- ✅ 不需要可视化和配置调整

### 5.4 安装建议

**最小化安装（仅测试评估）**：
```
✅ MMWAVE_L_SDK        ← 必需（烧录工具+固件）
✅ radar_toolbox       ← 推荐（配置文件+可视化）
❌ mmwave_studio       ← 不需要
```

**完整开发环境**：
```
✅ MMWAVE_L_SDK        ← 必需
✅ radar_toolbox       ← 必需
✅ mmwave_studio       ← 可选（算法开发时需要）
✅ Code Composer Studio ← 必需（固件编译）
✅ UniFlash            ← 可选（替代烧录工具）
```

**硬件验证环境**：
```
✅ mmwave_studio       ← 必需
✅ MMWAVE_L_SDK        ← 辅助
❌ radar_toolbox       ← 不需要
✅ DCA1000EVM          ← 必需（数据采集卡）
```

---

## 📝 总结

### 三句话概括

1. **MMWAVE_L_SDK** = **开发SDK**，提供固件源码、编译工具和烧录工具
2. **radar_toolbox** = **应用库**，提供45个配置文件和专用可视化工具
3. **mmwave_studio** = **测试工具**，用于RF测试、标定和原始数据采集

### 核心记忆

```
固件开发     → MMWAVE_L_SDK
配置调试     → radar_toolbox
硬件测试     → mmwave_studio
```

### 下一步

- ➡️ 继续阅读：[Part2-固件校验方法完整指南.md](Part2-固件校验方法完整指南.md)
- ➡️ 继续阅读：[Part3-SDK与固件关系及工作流程.md](Part3-SDK与固件关系及工作流程.md)
- ➡️ 继续阅读：[Part4-实践案例与常见问题.md](Part4-实践案例与常见问题.md)

---

**最后更新**：2025-12-25  
**文档作者**：项目开发团队
