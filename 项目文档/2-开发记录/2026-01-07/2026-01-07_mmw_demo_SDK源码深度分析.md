# 📊 mmw_demo SDK源码深度分析

> **分析日期**: 2026-01-07
> **SDK版本**: MMWAVE_L_SDK_06_01_00_01
> **目标平台**: AWRL6844-EVM
> **源码位置**: `project-code/mmw_demo_SDK_reference/mmwave_demo/`

---

## ⚠️ 重要更正说明

> **修正日期**: 2026-01-07
> **修正原因**: 初版文档错误地描述了AWRL6844架构，已根据TI官方文档完全更正

**错误描述**（已删除）：

- ❌ "AWRL6844只有R5F核心，没有DSP核心"
- ❌ "AWRL6844没有MSS/DSS"
- ❌ "单核+硬件加速架构"

**正确架构**（基于官方文档）：

- ✅ **APPSS**: ARM Cortex-R5F @ 200MHz (主控核心)
- ✅ **DSS**: TI C66x DSP @ 450MHz (信号处理核心)
- ✅ **DSS**: HWA 1.2 @ 200MHz (硬件加速器)
- ✅ **FECSS**: ARM Cortex-M3 @ 100/200MHz (前端控制器)
- ✅ 四子系统架构: APPSS/DSS/FECSS/RFANASS
- ✅ 2.5MB片上RAM（含896KB共享RAM）

**数据来源**：

- `awrl6844规格书.md` (SWRS353B, 第3-4页功能方框图)
- `xWRL68xx Technical Reference Manual.md` (SWRU621)

---

## 🎯 总体架构概览

### 工程结构

```
mmwave_demo/
├── xwrL684x-evm/              ← AWRL6844平台专用工程
│   └── r5fss0-0_freertos/     ← R5F核心 + FreeRTOS系统
│       ├── main.c             ← 入口文件
│       ├── example.syscfg     ← SysConfig配置
│       └── ti-arm-clang/      ← 编译工具链
├── source/                    ← 核心源代码（平台无关）
│   ├── mmwave_demo.c/h        ← 主应用逻辑
│   ├── mmw_cli.c/h            ← CLI命令解析
│   ├── dpc/                   ← 数据处理链（DPC）
│   ├── calibrations/          ← 校准模块
│   ├── lvds_streaming/        ← LVDS数据流
│   ├── mmwave_control/        ← 雷达控制
│   └── power_management/      ← 电源管理
├── profiles/                  ← 配置文件示例
│   ├── profile_2T4R_bpm.cfg
│   ├── profile_3T4R_tdm.cfg
│   └── profile_4T4R_tdm.cfg   ← 4发4收配置（适合AWRL6844）
└── prebuilt_binaries/         ← 预编译固件
```

---

## 🔧 AWRL6844平台架构详解

> **⚠️ 重要说明**：以下架构信息来自TI官方技术文档：
>
> - `awrl6844规格书.md` (SWRS353B)
> - `xWRL68xx Technical Reference Manual.md` (SWRU621)
> - 数据源日期: 2024年12月

### 1. 硬件架构：三核心 + 硬件加速架构

**AWRL6844真实架构（来自官方规格书第3-4页）**：

AWRL6844采用**多子系统架构**，包含以下四个可切换电源域：

| 子系统                           | 处理器核心     | 主频          | 功能职责                                   |
| -------------------------------- | -------------- | ------------- | ------------------------------------------ |
| **APPSS** (应用子系统)     | ARM Cortex-R5F | 200MHz        | 用户可编程主控核心，负责控制、汽车接口应用 |
| **DSS** (DSP子系统)        | TI C66x DSP    | 450MHz        | 高性能雷达信号处理（后处理）               |
| **DSS** (硬件加速器)       | HWA 1.2        | 200MHz        | FFT、CFAR等通用雷达处理（卸载DSP/R5F）     |
| **FECSS** (前端控制子系统) | ARM Cortex-M3  | 100MHz/200MHz | 雷达前端配置、控制、校准                   |

**关键架构特性**：

- ✅ **R5F核心**：双精度浮点FPU，768KB TCM RAM (512KB TCMA + 256KB TCMB)
- ✅ **C66x DSP**：雷达后处理专用核心，384KB L2 RAM + 512KB L3 RAM (AWRL6844独有)
- ✅ **HWA 1.2**：硬件加速FFT/CFAR/压缩/扩展，释放CPU算力
- ✅ **共享内存**：896KB DSS L3可与APPSS TCMA/TCMB/FECSS共享

**存储器架构（官方规格书第4页表4-1）**：

| 存储器区域     | 容量            | 共享对象                              |
| -------------- | --------------- | ------------------------------------- |
| DSS L3 本机RAM | 512KB           | 仅DSS专用（AWRL6844独有，AWRL6843无） |
| DSS L3 共享RAM | 512KB           | DSS L3 ↔ APPSS TCMA                  |
| DSS L3 共享RAM | 256KB           | DSS L3 ↔ APPSS TCMB                  |
| DSS L3 共享RAM | 128KB           | DSS L3 ↔ FECSS                       |
| **总计** | **2.5MB** | 片上RAM（AWRL6843为2MB）              |

### 2. AWRL6844的真实系统架构图

```
┌─────────────────────────────────────────────────────────────────────┐
│                  AWRL6844 系统架构（官方方框图）                      │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│ ┌──────────────────────────────────────────────────────────────┐  │
│ │          APPSS (Application SubSystem)                        │  │
│ │  ┌────────────────────────────────────────────────────────┐  │  │
│ │  │     ARM Cortex-R5F @ 200MHz (Lockstep Core)           │  │  │
│ │  │     - 双精度FPU                                         │  │  │
│ │  │     - 512KB TCMA RAM + 256KB TCMB RAM                  │  │  │
│ │  │     - 256KB ROM                                         │  │  │
│ │  │  ┌──────────────────────────────────────────────────┐  │  │  │
│ │  │  │         FreeRTOS 操作系统                         │  │  │  │
│ │  │  │  ┌──────────┐  ┌──────────┐  ┌─────────────┐    │  │  │  │
│ │  │  │  │ DPC Task │  │ TLV Task │  │  CLI Task   │    │  │  │  │
│ │  │  │  │ (协调)   │  │(输出格式)│  │ (命令解析)  │    │  │  │  │
│ │  │  │  └──────────┘  └──────────┘  └─────────────┘    │  │  │  │
│ │  │  └──────────────────────────────────────────────────┘  │  │  │
│ │  │     外设：CANFD×2, UART×2, SPI×2, I2C, QSPI, LIN      │  │  │
│ │  └────────────────────────────────────────────────────────┘  │  │
│ │  ┌────────────────────────────────────────────────────────┐  │  │
│ │  │ TOPSS: 时钟/电源管理, HSM(安全模块, 仅DBSxxxxxx型号)   │  │  │
│ │  └────────────────────────────────────────────────────────┘  │  │
│ └──────────────────────────────────────────────────────────────┘  │
│                              ↕                                     │
│ ┌──────────────────────────────────────────────────────────────┐  │
│ │          DSS (DSP SubSystem)                                  │  │
│ │  ┌────────────────────────────────────────────────────────┐  │  │
│ │  │     TI C66x DSP @ 450MHz                               │  │  │
│ │  │     - 64KB L1 RAM                                       │  │  │
│ │  │     - 384KB L2 RAM                                      │  │  │
│ │  │     - 512KB L3 RAM (AWRL6844专有，AWRL6843无)         │  │  │
│ │  │     功能：雷达后处理算法                                │  │  │
│ │  └────────────────────────────────────────────────────────┘  │  │
│ │  ┌────────────────────────────────────────────────────────┐  │  │
│ │  │     HWA 1.2 @ 200MHz (Radar Accelerator)              │  │  │
│ │  │  ┌──────────┐  ┌──────────┐  ┌──────────┐            │  │  │
│ │  │  │ Range FFT│  │Doppler FFT│ │ CFAR检测  │            │  │  │
│ │  │  │  (1D FFT)│  │  (2D FFT) │ │(目标检测) │            │  │  │
│ │  │  └──────────┘  └──────────┘  └──────────┘            │  │  │
│ │  │  ┌──────────┐  ┌──────────┐                          │  │  │
│ │  │  │压缩/解压缩│  │ DoA/AoA │ (角度估计)               │  │  │
│ │  │  └──────────┘  └──────────┘                          │  │  │
│ │  └────────────────────────────────────────────────────────┘  │  │
│ │  ┌────────────────────────────────────────────────────────┐  │  │
│ │  │     DSS L3 Shared RAM: 896KB                           │  │  │
│ │  │     (可与APPSS TCMA/TCMB、FECSS共享)                   │  │  │
│ │  └────────────────────────────────────────────────────────┘  │  │
│ └──────────────────────────────────────────────────────────────┘  │
│                              ↕                                     │
│ ┌──────────────────────────────────────────────────────────────┐  │
│ │     FECSS (Front End Controller SubSystem)                    │  │
│ │     ARM Cortex-M3 @ 100/200MHz                                │  │
│ │     功能：RF前端配置、校准、控制                               │  │
│ └──────────────────────────────────────────────────────────────┘  │
│                              ↕                                     │
│ ┌──────────────────────────────────────────────────────────────┐  │
│ │          RF/Analog SubSystem (RFANASS)                        │  │
│ │   57-64GHz FMCW雷达前端                                        │  │
│ │   TX: 4个发射天线 (AWRL6844) / 3个 (AWRL6843)                 │  │
│ │   RX: 4个接收天线                                              │  │
│ │   ADC Buffer: 16KB×2, GPADC×4                                 │  │
│ └──────────────────────────────────────────────────────────────┘  │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### 3. 子系统架构说明（官方技术手册）

**AWRL6844确实有明确的子系统划分**（来自xWRL68xx Technical Reference Manual）：

```
子系统架构：
┌──────────────────────────────────────────────────────────┐
│  APPSS (Application SubSystem)                           │
│  - ARM Cortex-R5F @ 200MHz                               │
│  - 用户应用主控、汽车接口、FreeRTOS任务调度               │
│  - 职责：雷达配置、CLI命令、数据输出、电源管理            │
├──────────────────────────────────────────────────────────┤
│  DSS (DSP SubSystem)                                     │
│  - TI C66x DSP @ 450MHz + HWA 1.2 @ 200MHz               │
│  - 雷达信号处理、FFT/CFAR加速、目标检测                  │
│  - 职责：Range-FFT、Doppler-FFT、CFAR、DoA/AoA           │
├──────────────────────────────────────────────────────────┤
│  FECSS (Front End Controller SubSystem)                  │
│  - ARM Cortex-M3 @ 100/200MHz                            │
│  - RF前端配置、校准、时序控制                             │
├──────────────────────────────────────────────────────────┤
│  RFANASS (RF/Analog SubSystem)                           │
│  - 射频前端、发射/接收链路、ADC采样                       │
└──────────────────────────────────────────────────────────┘
```

**与AWR1843/AWR2243等型号的对比**：

| 型号               | APPSS核心  | DSS核心        | HWA     | FECSS核心 | 适用场景         |
| ------------------ | ---------- | -------------- | ------- | --------- | ---------------- |
| **AWRL6844** | R5F@200MHz | C66x@450MHz    | HWA 1.2 | M3@200MHz | 车内检测、低功耗 |
| AWRL6843           | R5F@200MHz | C66x@450MHz    | HWA 1.2 | M3@200MHz | 车内检测         |
| AWR1843            | R4F@200MHz | C674x@600MHz   | 是      | -         | 经典雷达应用     |
| AWR2243            | R5F@400MHz | C66x×3@600MHz | 是      | -         | 级联成像雷达     |

**注意**：早期AWR18xx系列使用MSS/DSS命名，但xWRL68xx系列（包括AWRL6844）规范化为APPSS/DSS/FECSS命名

---

## 📂 AWRL6844工程详解

### 1. 入口文件：`xwrL684x-evm/r5fss0-0_freertos/main.c`

**关键代码分析**：

```c
int main(void)
{
    /* 初始化SOC和板级模块 */
    System_init();    // 初始化SOC（时钟、电源域等）
    Board_init();     // 初始化板级外设（UART、GPIO等）

    /* 创建主任务 - 最高优先级 */
    gMainTask = xTaskCreateStatic(
        freertos_main,      // 任务函数
        "freertos_main",    // 任务名称
        MAIN_TASK_SIZE,     // 栈大小 (16KB)
        NULL,
        MAIN_TASK_PRI,      // 优先级 (configMAX_PRIORITIES-1)
        gMainTaskStack,
        &gMainTaskObj
    );

    /* 启动FreeRTOS调度器 */
    vTaskStartScheduler();

    return 0;  // 永远不会到达这里
}

void freertos_main(void *args)
{
    mmwave_demo(NULL);  // 调用主应用函数
    vTaskDelete(NULL);  // 删除自己
}
```

**工作流程**：

1. **系统初始化** → SOC外设配置
2. **创建主任务** → FreeRTOS任务
3. **启动调度器** → 开始多任务调度
4. **进入应用** → mmwave_demo()

### 2. SysConfig配置：`example.syscfg`

**关键配置信息**：

```javascript
// 目标器件
device = "AWRL6844"
context = "r5fss0-0"  // R5F核心0
package = "FCCSP (ANC)"

// EDMA配置
edma1.instance = "EDMA_DSS_A"  // 使用DSS侧的EDMA（实际是HWA的DMA）

// 内存配置
L3 RAM: 1408 KB - 1 KB(IPC) - 16B(CRC) = ~1407 KB
本地RAM: 28 KB (用于DPC临时数据)
```

**注意**：虽然配置中看到"DSS"字样，但在AWRL6844中，这实际指的是**HWA硬件加速器的DMA接口**，不是独立的DSP核心。

---

## 🔄 数据处理流程（DPC）

### 1. DPC架构概览

**DPC = Data Processing Chain（数据处理链）**

```
┌────────────────────────────────────────────────────────────┐
│              mmwave_demo 数据处理链（DPC）                  │
├────────────────────────────────────────────────────────────┤
│                                                            │
│  雷达原始数据 (ADC Data)                                    │
│       ↓                                                    │
│  ┌────────────────────────────────────────────────────┐   │
│  │  DPU #1: Range Processing (1D FFT)                 │   │
│  │  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━   │   │
│  │  • 输入: ADC采样数据（时域）                        │   │
│  │  • 处理: FFT变换 → 距离维频域                       │   │
│  │  • 输出: Range-Doppler Matrix (每个chirp的FFT结果) │   │
│  │  • 硬件: HWA执行FFT加速                             │   │
│  └────────────────────────────────────────────────────┘   │
│       ↓                                                    │
│  ┌────────────────────────────────────────────────────┐   │
│  │  DPU #2: Doppler Processing (2D FFT)               │   │
│  │  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━   │   │
│  │  • 输入: Range bins的时间序列                       │   │
│  │  • 处理: 多普勒FFT → 速度维频域                     │   │
│  │  • 输出: Range-Doppler Heatmap                      │   │
│  │  • 硬件: HWA执行FFT加速                             │   │
│  └────────────────────────────────────────────────────┘   │
│       ↓                                                    │
│  ┌────────────────────────────────────────────────────┐   │
│  │  DPU #3: CFAR Detection (目标检测)                  │   │
│  │  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━   │   │
│  │  • 输入: Range-Doppler Heatmap                      │   │
│  │  • 处理: CA-CFAR / CASO-CFAR 自适应阈值检测         │   │
│  │  • 输出: 检测到的目标列表（距离、速度）              │   │
│  │  • 硬件: HWA执行CFAR算法                            │   │
│  └────────────────────────────────────────────────────┘   │
│       ↓                                                    │
│  ┌────────────────────────────────────────────────────┐   │
│  │  DPU #4: Angle of Arrival (DoA/AoA估计)            │   │
│  │  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━   │   │
│  │  • 输入: 检测目标的多天线数据                       │   │
│  │  • 处理: 2D AoA（方位角 + 俯仰角）                  │   │
│  │  • 算法: BPM/MUSIC/Capon/Beamforming                │   │
│  │  • 输出: 目标的3D坐标 (X, Y, Z)                     │   │
│  └────────────────────────────────────────────────────┘   │
│       ↓                                                    │
│  最终输出: Point Cloud (点云)                              │
│  ┌─────────────────────────────────────────┐              │
│  │ 每个点的信息：                            │              │
│  │ • 位置: (x, y, z) [米]                   │              │
│  │ • 速度: doppler [m/s]                    │              │
│  │ • 信噪比: SNR [dB]                       │              │
│  │ • 噪声: noise [dB]                       │              │
│  └─────────────────────────────────────────┘              │
│                                                            │
└────────────────────────────────────────────────────────────┘
```

### 2. DPC源码结构

**核心文件**：`source/dpc/dpc.c` 和 `source/dpc/dpc.h`

**主要函数**：

```c
// 初始化所有DPU
void DPC_Init() {
    DPC_ObjDet_RngDpuInit();      // 初始化Range DPU
    DPC_ObjDet_DopplerDpuInit();  // 初始化Doppler DPU
    DPC_ObjDet_CfarDpuInit();     // 初始化CFAR DPU
    DPC_ObjDet_AoaDpuInit();      // 初始化AoA DPU
}

// 配置所有DPU
void DPC_Config() {
    DPC_ObjDet_RngDpuCfg();       // 配置Range处理
    DPC_ObjDet_DopplerDpuCfg();   // 配置Doppler处理
    DPC_ObjDet_CfarDpuCfg();      // 配置CFAR检测
    DPC_ObjDet_AoaDpuCfg();       // 配置角度估计
}

// 执行数据处理链
void DPC_Execute() {
    // 1. Range FFT
    // 2. Doppler FFT
    // 3. CFAR Detection
    // 4. DoA Estimation
    // 5. 生成Point Cloud
}
```

### 3. DPU配置参数

**每个DPU都有对应的配置结构**：

```c
// Range Processing配置
DPU_RangeProcHWA_Config gRangeProcDpuCfg = {
    .fftWindow = MATHUTILS_WIN_BLACKMAN,  // FFT窗函数（布莱克曼窗）
    .numRangeBins = 256,                  // 距离bins数量
    .qFormat = 17,                        // Q格式（定点数精度）
    // ...
};

// CFAR配置
DPU_CFARProcHWA_Config gCfarProcDpuCfg = {
    .averageMode = 2,        // CA-CFAR
    .winLen = 8,             // 窗口长度
    .guardLen = 4,           // 保护单元
    .thresholdScale = 900,   // 阈值 (9.0 dB * 100)
    // ...
};

// AoA配置
DPU_AoAProcHWA_Config gAoa2dProcDpuCfg = {
    .numTxAntennas = 4,      // 发射天线数
    .numRxAntennas = 4,      // 接收天线数
    .numVirtualAntennas = 16, // 虚拟天线 (4×4)
    // ...
};
```

---

## 🎛️ CLI命令系统

### 1. CLI架构

**CLI = Command Line Interface（命令行接口）**

**文件位置**：`source/mmw_cli.c` 和 `source/mmw_cli.h`

**工作方式**：

- 通过UART接收配置命令
- 解析命令并更新配置参数
- 在 `sensorStart`命令后生效

### 2. 关键CLI命令

**CFAR配置命令**：

```bash
# 格式：cfarProcCfg <dimension> <averageMode> <winLen> <guardLen> <noiseDivShift> <cyclicMode> <thresholdScale> <peakGroupingEn>

# Range维度CFAR
cfarProcCfg 0 2 8 4 3 0 9.0 0
#           │ │ │ │ │ │  │  │
#           │ │ │ │ │ │  │  └─ 峰值分组关闭
#           │ │ │ │ │ │  └──── 阈值9.0dB
#           │ │ │ │ │ └─────── 循环模式关闭
#           │ │ │ │ └────────── 噪声除法移位
#           │ │ │ └───────────── 保护单元4
#           │ │ └──────────────── 窗口长度8
#           │ └─────────────────── CA-CFAR
#           └──────────────────── 维度0 (Range)

# Doppler维度CFAR
cfarProcCfg 1 2 4 2 2 1 9.0 0
#           │
#           └──────────────────── 维度1 (Doppler)
```

**AoA配置命令**：

```bash
# 格式：aoaProcCfg <numAngleBins> <numAngleBins>
aoaProcCfg 64 64
#          │  └─ 俯仰角bins数量
#          └──── 方位角bins数量

# 格式：aoaFovCfg <minAzimuthDeg> <maxAzimuthDeg> <minElevationDeg> <maxElevationDeg>
aoaFovCfg -60 60 -60 60
#         │   │   │   └─ 最大俯仰角+60°
#         │   │   └───── 最小俯仰角-60°
#         │   └───────── 最大方位角+60°
#         └───────────── 最小方位角-60°
```

**传感器控制命令**：

```bash
sensorStop 0           # 停止雷达
channelCfg 153 255 0   # 配置天线通道（4TX4RX）
frameCfg 64 0 1358 1 100 0  # 配置帧参数
sensorStart 0 0 0 0    # 启动雷达
```

### 3. CLI解析流程

```c
// CLI命令表（部分）
CLI_CmdTableEntry gCLICommands[] = {
    {"cfarProcCfg",    CLI_MMWaveCfarProcCfg},
    {"cfarFovCfg",     CLI_MMWaveCfarFovCfg},
    {"aoaProcCfg",     CLI_MMWaveAoaProcCfg},
    {"aoaFovCfg",      CLI_MMWaveAoaCfg},
    {"sensorStart",    CLI_MMWaveSensorStart},
    {"sensorStop",     CLI_MMWaveSensorStop},
    // ...
};

// CFAR命令解析函数
static int32_t CLI_MMWaveCfarProcCfg(int32_t argc, char* argv[]) {
    uint8_t dimension = (uint8_t) atoi(argv[1]);
  
    if (dimension == 0) {  // Range CFAR
        gMmwMssMCB.cfarRangeCfg.averageMode = (uint8_t) atoi(argv[2]);
        gMmwMssMCB.cfarRangeCfg.winLen = (uint8_t) atoi(argv[3]);
        gMmwMssMCB.cfarRangeCfg.guardLen = (uint8_t) atoi(argv[4]);
        // ...
    } else {  // Doppler CFAR
        gMmwMssMCB.cfarDopplerCfg.averageMode = (uint8_t) atoi(argv[2]);
        // ...
    }
  
    return 0;
}
```

---

## 🔢 关键数据结构

### 1. 主控制块：MmwDemo_MSS_MCB

```c
typedef struct MmwDemo_MSS_MCB_t {
    // CLI配置
    DPU_CFARProcHWA_Config  cfarRangeCfg;      // Range CFAR配置
    DPU_CFARProcHWA_Config  cfarDopplerCfg;    // Doppler CFAR配置
    DPU_AoAProcHWA_Config   aoaProcCfg;        // AoA配置
  
    // mmWave配置
    MMWave_Handle           ctrlHandle;        // 雷达控制句柄
    MMWave_OpenCfg          openCfg;           // 打开配置
    MMWave_CtrlCfg          ctrlCfg;           // 控制配置
  
    // EDMA
    EDMA_Handle             edmaHandle;        // EDMA句柄
  
    // HWA
    HWA_Handle              hwaHandle;         // HWA句柄
  
    // 任务同步
    SemaphoreP_Handle       dpcSemaphore;      // DPC任务信号量
    SemaphoreP_Handle       outputSemaphore;   // 输出任务信号量
  
    // 输出缓冲
    uint8_t                 outputBuffer[...]; // 输出数据缓冲
  
} MmwDemo_MSS_MCB;

// 全局实例
extern MmwDemo_MSS_MCB gMmwMssMCB;
```

### 2. 点云输出结构

```c
// 笛卡尔坐标点云
typedef struct DPIF_PointCloudCartesian_t {
    float x;        // X坐标 (米)
    float y;        // Y坐标 (米)
    float z;        // Z坐标 (米)
    float doppler;  // 多普勒速度 (m/s)
} DPIF_PointCloudCartesian;

// 点云附加信息
typedef struct DPIF_PointCloudSideInfo_t {
    int16_t snr;    // 信噪比 (dB)
    int16_t noise;  // 噪声 (dB)
} DPIF_PointCloudSideInfo;

// DPC执行结果
typedef struct DPC_ObjectDetection_ExecuteResult_t {
    uint32_t numObjOut;                           // 检测到的目标数量
    DPIF_PointCloudCartesian *objOut;             // 点云数组
    DPIF_PointCloudSideInfo *objOutSideInfo;      // 附加信息数组
    uint32_t *rngAzHeatMap;                       // Range-Azimuth热图
    uint16_t *rngDopplerHeatMap;                  // Range-Doppler热图
} DPC_ObjectDetection_ExecuteResult;
```

---

## 📊 FreeRTOS任务架构

### 任务列表

```c
// 任务优先级定义
#define DPC_TASK_PRI           5   // 数据处理任务（最高）
#define POWER_TASK_PRI         2   // 电源管理任务
#define TLV_TASK_PRI           3   // TLV输出任务
#define CLI_TASK_PRIORITY      1   // CLI解析任务（最低）

// 任务栈大小
#define DPC_TASK_STACK_SIZE    8192  // 8KB
#define TLV_TASK_STACK_SIZE    2048  // 2KB
#define CLI_TASK_STACK_SIZE    2048  // 2KB
```

### 任务工作流程

```
┌─────────────────────────────────────────────────────────────┐
│                    FreeRTOS任务调度                          │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  CLI Task (优先级1)                                         │
│  ┌──────────────────────────────────────────┐              │
│  │  • 监听UART输入                           │              │
│  │  • 解析配置命令                           │              │
│  │  • 更新全局配置                           │              │
│  └──────────────────────────────────────────┘              │
│                                                             │
│  DPC Task (优先级5) ← 最高优先级                            │
│  ┌──────────────────────────────────────────┐              │
│  │  1. 等待Frame Start中断                   │              │
│  │  2. 配置并启动HWA                         │              │
│  │  3. 等待HWA处理完成                       │              │
│  │  4. 执行DPC处理链：                       │              │
│  │     • Range FFT                          │              │
│  │     • Doppler FFT                        │              │
│  │     • CFAR Detection                     │              │
│  │     • DoA Estimation                     │              │
│  │  5. 生成Point Cloud                      │              │
│  │  6. 通知TLV Task输出                     │              │
│  └──────────────────────────────────────────┘              │
│               ↓                                             │
│  TLV Task (优先级3)                                         │
│  ┌──────────────────────────────────────────┐              │
│  │  • 等待DPC完成信号量                      │              │
│  │  • 格式化输出为TLV格式                    │              │
│  │  • 通过UART发送点云数据                   │              │
│  └──────────────────────────────────────────┘              │
│                                                             │
│  Power Task (优先级2)                                       │
│  ┌──────────────────────────────────────────┐              │
│  │  • 监控系统状态                           │              │
│  │  • 低功耗模式管理                         │              │
│  │  • PMIC看门狗                            │              │
│  └──────────────────────────────────────────┘              │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## 📝 配置文件详解

### profile_4T4R_tdm.cfg 分析

**位置**：`profiles/profile_4T4R_tdm.cfg`

**关键配置解析**：

```properties
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# 1. 天线通道配置
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
channelCfg 153 255 0
#          │   │   └─ Tx enable mask (0 = 使用默认)
#          │   └───── Rx enable: 0xFF = 1111 1111 (4个RX全开)
#          └───────── 解释：153 = 0x99 = 1001 1001 (TDM模式)
# TDM: 4个TX分时发射，避免相互干扰

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# 2. Chirp配置
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
chirpComnCfg 8 0 0 256 1 13.1 3
#            │ │ │  │  │  │   └─ 频率斜率: 3 MHz/μs
#            │ │ │  │  │  └───── 采样率: 13.1 Msps
#            │ │ │  │  └──────── ADC起始时间
#            │ │ │  └─────────── ADC采样点数: 256
#            │ │ └────────────── 起始频率偏移
#            │ └───────────────── TX输出功率
#            └────────────────── Chirp索引

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# 3. 帧配置
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
frameCfg 64 0 1358 1 100 0
#        │  │  │   │  │  └─ Trigger延迟
#        │  │  │   │  └──── 帧周期: 100ms (10 FPS)
#        │  │  │   └─────── 触发选择
#        │  │  └─────────── 帧内chirp循环次数
#        │  └────────────── 起始chirp索引
#        └───────────────── 每帧chirp数量: 64

# 计算：
# 每帧chirp数 = 64
# 每个chirp采样点 = 256
# 每帧总采样 = 64 × 256 = 16384 samples
# 4个RX → 总数据 = 16384 × 4 = 65536 samples/frame

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# 4. CFAR配置
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# Range CFAR
cfarProcCfg 0 2 8 4 3 0 9.0 0
#           │ │ │ │ │ │  │  └─ 峰值分组：关闭
#           │ │ │ │ │ │  └──── 阈值：9.0 dB
#           │ │ │ │ │ └─────── 循环模式：关闭
#           │ │ │ │ └────────── 噪声除法移位：3 (除以8)
#           │ │ │ └───────────── 保护单元：4 bins
#           │ │ └──────────────── 窗口长度：8 bins
#           │ └─────────────────── 平均模式：2 (CA-CFAR)
#           └──────────────────── 维度：0 (Range)

# Doppler CFAR
cfarProcCfg 1 2 4 2 2 1 9.0 0
#           │ │ │ │ │ │  │  └─ 峰值分组：关闭
#           │ │ │ │ │ │  └──── 阈值：9.0 dB
#           │ │ │ │ │ └─────── 循环模式：开启（多普勒是循环的）
#           │ │ │ │ └────────── 噪声除法移位：2 (除以4)
#           │ │ │ └───────────── 保护单元：2 bins
#           │ │ └──────────────── 窗口长度：4 bins
#           │ └─────────────────── 平均模式：2 (CA-CFAR)
#           └──────────────────── 维度：1 (Doppler)

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# 5. AoA配置
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
aoaProcCfg 64 64
#          │  └─ 俯仰角bins: 64
#          └──── 方位角bins: 64

aoaFovCfg -60 60 -60 60
#         │   │   │   └─ 最大俯仰角：+60°
#         │   │   └───── 最小俯仰角：-60°
#         │   └───────── 最大方位角：+60°
#         └───────────── 最小方位角：-60°

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# 6. 杂波移除
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
clutterRemoval 0
#              └─ 0 = 关闭静态杂波移除
# 注意：室内健康检测可能需要开启（clutterRemoval 1）

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# 7. 天线几何配置
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
antGeometryBoard xWRL6844EVM
# 使用预定义的AWRL6844 EVM天线阵列几何
# 4TX × 4RX = 16个虚拟天线
```

---

## 🚀 AWRL6844项目开发指南

### 1. 如何使用SDK源码

**步骤1：复制工程**

```bash
# 源路径
C:\ti\MMWAVE_L_SDK_06_01_00_01\examples\mmw_demo\

# 目标路径（已完成）
project-code\mmw_demo_SDK_reference\mmwave_demo\

# 你的开发工程
project-code\AWRL6844_HealthDetect\
```

**步骤2：理解工程结构**

```
AWRL6844_HealthDetect/    ← 你的项目
├── xwrL684x-evm/         ← 平台工程（从SDK复制）
│   └── r5fss0-0_freertos/
│       ├── main.c        ← 入口（通常不需要修改）
│       ├── example.syscfg ← 外设配置（可能需要微调）
│       └── ti-arm-clang/  ← 编译配置
├── source/               ← 核心代码（需要扩展）
│   ├── mmwave_demo.c     ← 主应用（添加你的逻辑）
│   ├── mmw_cli.c         ← CLI命令（添加新命令）
│   ├── dpc/              ← 数据处理链
│   │   ├── dpc.c         ← **核心：添加健康检测算法**
│   │   └── dpc.h
│   └── health_detect/    ← 🆕 新建：健康检测模块
│       ├── presence.c    ← 人存检测
│       ├── pose.c        ← 姿态检测
│       └── fall.c        ← 跌倒检测
├── profiles/             ← 配置文件
│   └── health_detect.cfg ← 🆕 健康检测专用配置
└── README.md
```

**步骤3：开发流程**

```
1. 🔧 环境搭建
   └─ 导入xwrL684x-evm工程到CCS
   └─ 验证编译通过
   └─ 烧录测试标准Demo

2. 📝 添加人存检测
   └─ 在source/health_detect/创建presence.c
   └─ 实现人存检测算法（基于点云数量/速度）
   └─ 在DPC_Execute()中调用
   └─ 添加CLI命令：presenceCfg

3. 🧍 添加姿态检测
   └─ 创建pose.c
   └─ 实现姿态分类（基于点云高度分布）
   └─ 集成到DPC流程

4. 🚨 添加跌倒检测
   └─ 创建fall.c
   └─ 实现状态机（站立→跌倒→躺地）
   └─ 添加跌倒报警机制

5. ✅ 测试验证
   └─ 单元测试
   └─ 集成测试
   └─ 实际场景测试
```

### 2. 关键修改点

**修改1：扩展DPC处理**

```c
// source/dpc/dpc.c - DPC_Execute() 函数中

void DPC_Execute() {
    // ━━━━ 标准mmwave_demo流程 ━━━━
    // 1. Range FFT
    DPC_ObjDet_RngDpuExecute();
  
    // 2. Doppler FFT
    DPC_ObjDet_DopplerDpuExecute();
  
    // 3. CFAR Detection
    DPC_ObjDet_CfarDpuExecute();
  
    // 4. DoA Estimation
    DPC_ObjDet_AoaDpuExecute();
  
    // 得到点云：result->objOut, result->numObjOut
  
    // ━━━━ 🆕 添加健康检测处理 ━━━━
    #ifdef HEALTH_DETECT_ENABLED
  
    // 5. 人存检测
    PresenceDetect_Result presenceResult;
    PresenceDetect_Process(
        result->objOut,
        result->numObjOut,
        &presenceResult
    );
  
    // 6. 姿态检测（如果检测到人）
    if (presenceResult.occupied) {
        PoseDetect_Result poseResult;
        PoseDetect_Process(
            result->objOut,
            result->numObjOut,
            &poseResult
        );
    
        // 7. 跌倒检测
        FallDetect_Result fallResult;
        FallDetect_Process(
            &poseResult,
            &fallResult
        );
    
        // 报警处理
        if (fallResult.fallDetected) {
            // 触发报警！
            GPIO_setHigh(ALARM_PIN);
            UART_sendAlert("Fall detected!");
        }
    }
    #endif
  
    // 8. TLV输出（包含健康检测结果）
    // ...
}
```

**修改2：添加CLI命令**

```c
// source/mmw_cli.c - 添加新命令

// 命令表
CLI_CmdTableEntry gCLICommands[] = {
    // ...现有命令...
  
    // 🆕 健康检测命令
    {"presenceCfg",    CLI_PresenceCfg},
    {"poseCfg",        CLI_PoseCfg},
    {"fallCfg",        CLI_FallCfg},
    {"healthMonitor",  CLI_HealthMonitor},
};

// 实现命令函数
static int32_t CLI_PresenceCfg(int32_t argc, char* argv[]) {
    // presenceCfg <minPoints> <minVelocity> <timeout>
    gMmwMssMCB.presenceCfg.minPoints = (uint16_t) atoi(argv[1]);
    gMmwMssMCB.presenceCfg.minVelocity = (float) atof(argv[2]);
    gMmwMssMCB.presenceCfg.timeout = (uint32_t) atoi(argv[3]);
    return 0;
}
```

**修改3：配置文件**

```properties
# profiles/health_detect.cfg

# ━━━━ 基础配置 ━━━━
sensorStop 0
channelCfg 153 255 0      # 4TX 4RX TDM
frameCfg 64 0 1358 1 100 0  # 10 FPS

# ━━━━ 标准检测配置 ━━━━
cfarProcCfg 0 2 8 4 3 0 6.0 0   # Range CFAR (降低阈值，提高灵敏度)
cfarProcCfg 1 2 4 2 2 1 6.0 0   # Doppler CFAR
aoaProcCfg 64 64
aoaFovCfg -60 60 -60 60
clutterRemoval 1  # 开启杂波移除（重要！）

# ━━━━ 🆕 健康检测配置 ━━━━
presenceCfg 3 0.1 5000   # 最少3个点，最小速度0.1m/s，超时5秒
poseCfg 0.5 1.2 1.6      # 躺/坐/站高度阈值：0.5m, 1.2m, 1.6m
fallCfg 1.5 2.0 500      # 跌倒判定：1.5m/s速度，2.0秒时间，500ms报警延迟

# ━━━━ 启动 ━━━━
sensorStart 0 0 0 0
```

### 3. 内存分配策略

**AWRL6844内存布局**：

```
┌────────────────────────────────────────────────────┐
│              AWRL6844 内存分配                      │
├────────────────────────────────────────────────────┤
│                                                    │
│  L3 RAM (1407 KB) - 主数据存储                     │
│  ┌──────────────────────────────────────────────┐ │
│  │  • Radar Cube (ADC数据)      [动态]         │ │
│  │  • Range-Doppler Matrix      [~512KB]       │ │
│  │  • Range-Azimuth Heatmap     [可选]         │ │
│  │  • Detection List            [~50KB]        │ │
│  │  • 🆕 健康检测缓冲区           [~20KB]       │ │
│  └──────────────────────────────────────────────┘ │
│                                                    │
│  Local RAM (28 KB) - DPU临时数据                   │
│  ┌──────────────────────────────────────────────┐ │
│  │  • DPU配置结构                               │ │
│  │  • 中间计算缓冲                              │ │
│  │  • 🆕 姿态历史缓冲 (状态机)   [~2KB]        │ │
│  └──────────────────────────────────────────────┘ │
│                                                    │
│  FreeRTOS Heap - 动态分配                          │
│  ┌──────────────────────────────────────────────┐ │
│  │  • 任务栈空间                                │ │
│  │  • 信号量/队列                               │ │
│  └──────────────────────────────────────────────┘ │
│                                                    │
└────────────────────────────────────────────────────┘
```

**健康检测内存需求**：

```c
// 估算
typedef struct HealthDetect_Memory_t {
    // 人存检测历史（10帧）
    PresenceHistory history[10];           // ~1 KB
  
    // 姿态检测状态机（100帧历史）
    PoseState poseHistory[100];            // ~2 KB
  
    // 跌倒检测缓冲
    FallDetect_Buffer fallBuffer;          // ~1 KB
  
    // 点云处理缓冲
    PointCloudFiltered filtered[500];      // ~16 KB
  
} HealthDetect_Memory;  // 总计：~20 KB

// 分配策略
// 1. 静态分配在L3 RAM（预留专用空间）
uint8_t gHealthDetectMem[20*1024] __attribute__((section(".bss.l3")));

// 2. 或使用已有的gMmwL3缓冲区末尾空间
```

---

## 📚 重要SDK头文件

### DPU（Data Processing Unit）头文件

```c
// Range Processing
#include <datapath/dpu/rangeproc/v1/rangeprochwa.h>

// Doppler Processing
#include <datapath/dpu/dopplerproc/v1/dopplerprochwa.h>

// CFAR Detection
#include <datapath/dpu/cfarproc/v1/cfarprochwa.h>

// AoA Estimation
#include <datapath/dpu/aoa2dproc/v1/aoa2dproc.h>

// 点云数据接口
#include <datapath/dpif/dpif_pointcloud.h>
```

### mmWave控制

```c
// mmWave控制API
#include <control/mmwave/mmwave.h>

// mmWaveLink底层API
#include <mmwavelink/include/rl_device.h>
#include <mmwavelink/include/rl_sensor.h>
```

### 硬件驱动

```c
// HWA驱动
#include <drivers/hwa.h>

// EDMA驱动
#include <drivers/edma.h>

// UART驱动
#include <drivers/uart.h>

// ADC Buffer
#include <drivers/adcbuf.h>
```

---

## 🔍 调试与监控

### 1. 串口输出

**两个UART端口**：

```
UART0 (CLI)：
• 波特率：115200
• 用途：配置命令输入
• 数据：文本命令

UART1 (Data)：
• 波特率：921600（可配置）
• 用途：点云数据输出
• 格式：TLV (Type-Length-Value)
```

### 2. TLV输出格式

```c
// TLV消息头
typedef struct {
    uint16_t type;     // 消息类型
    uint16_t length;   // 数据长度（字节）
} MmwDemo_output_message_tl;

// TLV类型定义
#define MMWDEMO_OUTPUT_MSG_DETECTED_POINTS         1  // 点云数据
#define MMWDEMO_OUTPUT_MSG_RANGE_PROFILE           2  // Range Profile
#define MMWDEMO_OUTPUT_MSG_NOISE_PROFILE           3  // 噪声Profile
#define MMWDEMO_OUTPUT_MSG_AZIMUT_STATIC_HEAT_MAP  4  // 方位角热图
#define MMWDEMO_OUTPUT_MSG_RANGE_DOPPLER_HEAT_MAP  5  // Range-Doppler热图

// 🆕 可添加自定义TLV类型
#define MMWDEMO_OUTPUT_MSG_PRESENCE_STATUS         100  // 人存状态
#define MMWDEMO_OUTPUT_MSG_POSE_INFO               101  // 姿态信息
#define MMWDEMO_OUTPUT_MSG_FALL_ALERT              102  // 跌倒报警
```

### 3. 性能监控

```c
// 在mmwave_demo.c中
typedef struct {
    uint32_t frameStartTime;      // 帧开始时间戳
    uint32_t dpcExecuteTime;      // DPC执行时间
    uint32_t tlvOutputTime;       // TLV输出时间
    uint32_t frameEndTime;        // 帧结束时间戳
    uint32_t cpuLoad;             // CPU负载百分比
} MmwDemo_PerformanceStats;

// 测量代码示例
frameStartTime = ClockP_getTimeUsec();
DPC_Execute();
dpcExecuteTime = ClockP_getTimeUsec() - frameStartTime;

// 输出性能统计
if (gMmwMssMCB.performanceMonitor) {
    CLI_write("Frame %d: DPC=%d us, TLV=%d us, Total=%d us\n",
              frameNum, dpcExecuteTime, tlvOutputTime, totalTime);
}
```

---

## 📌 关键要点总结

### AWRL6844特有特性

1. **多核心协同架构**

   - ✅ **APPSS**: R5F@200MHz (主控核心，FreeRTOS)
   - ✅ **DSS**: C66x DSP@450MHz + HWA 1.2@200MHz (信号处理)
   - ✅ **FECSS**: M3@200MHz (前端控制)
   - ✅ 四个可切换电源域，低功耗设计
   - ✅ 2.5MB片上RAM（含896KB共享RAM）
2. **工程结构（针对R5F核心）**

   - ✅ 编译 `xwrL684x-evm/r5fss0-0_freertos`工程
   - ✅ main.c是R5F入口，调用mmwave_demo()
   - ✅ R5F负责：应用逻辑、CLI命令、雷达配置、数据输出
   - ✅ DSP/HWA负责：FFT/CFAR/DoA等信号处理（通过DPC调度）
3. **开发重点**

   - ✅ 修改 `source/dpc/dpc.c` - 添加处理算法（调用DSP/HWA）
   - ✅ 修改 `source/mmw_cli.c` - 添加CLI命令
   - ✅ 创建新模块 `source/health_detect/` - 健康检测

### 数据处理流程

```
ADC数据 → Range FFT → Doppler FFT → CFAR检测 → DoA估计 → 点云
                                                           ↓
                                                    🆕 健康检测
                                                       ├─ 人存
                                                       ├─ 姿态
                                                       └─ 跌倒
```

### 配置要点

1. **天线配置**: `channelCfg 153 255 0` (4TX4RX TDM)
2. **帧率**: `frameCfg ... 100 ...` (100ms = 10 FPS)
3. **CFAR阈值**: 降低到6-7dB提高灵敏度
4. **杂波移除**: 室内场景必须开启 `clutterRemoval 1`
5. **AoA范围**: `aoaFovCfg -60 60 -60 60` (覆盖监测区域)

### 内存规划

- **L3 RAM**: 1407 KB - 主要数据存储
- **Local RAM**: 28 KB - DPU临时数据
- **健康检测**: 预留20KB即可

---

## 🚀 下一步行动

### 立即可做

1. ✅ **验证标准Demo** - 确认硬件工作正常
2. ✅ **理解DPC流程** - 熟悉点云生成过程
3. ✅ **修改配置文件** - 优化CFAR参数

### 短期目标（1-2天）

1. 📝 **添加人存检测** - 最简单的功能
2. 📝 **CLI命令扩展** - `presenceCfg`命令
3. 📝 **TLV输出扩展** - 输出人存状态

### 中期目标（3-5天）

1. 🧍 **姿态检测** - 基于点云高度分类
2. 🚨 **跌倒检测** - 状态机实现
3. 📊 **性能优化** - 降低CPU负载

---

## 📖 参考资料

### TI官方文档

1. **AWRL6844 EVM User Guide (SWRU630)**

   - 硬件说明、接口定义
2. **mmWave SDK User Guide**

   - SDK整体架构、API参考
3. **mmWave Demo User Guide**

   - mmwave_demo应用说明
4. **DPU Reference Manual**

   - 各个DPU的详细参数

### 代码位置

- **SDK安装路径**: `C:\ti\MMWAVE_L_SDK_06_01_00_01\`
- **示例代码**: `examples\mmw_demo\`
- **DPU库**: `datapath\dpu\`
- **参考备份**: `project-code\mmw_demo_SDK_reference\`

---

> 📅 **文档日期**: 2026-01-07
> 🎯 **目标**: 为AWRL6844健康检测项目提供SDK源码分析
> ✅ **状态**: 完整分析完成，可作为开发参考
