# 🔧 AWRL6844 硬件设计文件与SDK关系深度分析

> **文档版本**: v1.0  
> **创建日期**: 2025-12-25  
> **适用硬件**: AWRL6844-EVM  
> **相关SDK**: MMWAVE_L_SDK 06.01.00.01  
> **前置文档**: [Part1-Part5](README.md)

---

## 📋 目录

- [第一章：硬件设计文件概览](#第一章硬件设计文件概览)
- [第二章：EVM硬件架构](#第二章evm硬件架构)
- [第三章：硬件与固件的关系](#第三章硬件与固件的关系)
- [第四章：硬件与SysConfig的关系](#第四章硬件与sysconfig的关系)
- [第五章：硬件设计对SDK的影响](#第五章硬件设计对sdk的影响)

---

## 第一章：硬件设计文件概览

### 1.1 设计文件的重要性

**为什么需要了解硬件设计？**

```
软件开发 ≠ 只关注代码
软件开发 = 理解硬件 + 编写代码

硬件决定：
├─ 哪些外设可用（UART/SPI/CAN等）
├─ 引脚如何分配（GPIO映射）
├─ 性能限制（天线数量、内存大小）
└─ 功耗特性（电源管理拓扑）
```

**硬件设计文件的作用**：
- ✅ **理解硬件能力** - 知道芯片能做什么
- ✅ **正确配置固件** - 避免配置不兼容的功能
- ✅ **优化性能** - 充分利用硬件特性
- ✅ **故障排查** - 理解硬件电路帮助定位问题

### 1.2 AWRL6844-EVM设计文件清单

**核心文档**：

| 文档名称 | 文件编号 | 内容 | 作用 |
|---------|---------|------|------|
| **EVM User Guide (中文)** | ZHCUCO8 | 评估板使用指南 | 快速上手 |
| **EVM User Guide (英文)** | SWRU630A | 评估板完整规格 | 详细参考 |
| **芯片数据手册** | AWRL6844规格书 | 芯片电气特性 | 硬件设计 |
| **迁移指南** | Migration Guide | 从6432升级到6844 | 升级参考 |

**设计文件位置**：
```
知识库/雷达模块/设计文件/
├── zhcuco8.pdf           # EVM用户指南（中文）
├── swru630a.pdf          # EVM规格书（英文）
├── awrl6844规格书.md     # 芯片数据手册
└── SWRC394A/             # 完整设计包（原理图/PCB）
```

**已转换的Markdown文档**：
```
知识库/知识库PDF转机器可读文件/
├── AWRL6844-IWRL6844 评估模块zhcuco8.md
├── AWRL6844EVM-规格书swru630a.md
├── awrl6844规格书.md
└── Migration_from_xWRLx432_to_xWRL6844.md
```

### 1.3 硬件设计文件的层次

```
┌─────────────────────────────────────────┐
│  第1层：芯片级 (Chip Level)              │
│  └─ AWRL6844数据手册                    │
│     ├─ 芯片引脚定义                     │
│     ├─ 电气特性                         │
│     └─ 功能模块                         │
├─────────────────────────────────────────┤
│  第2层：模块级 (Module Level)            │
│  └─ EVM电路设计                         │
│     ├─ 电源管理 (PMIC)                  │
│     ├─ 外设接口 (UART/SPI/CAN)          │
│     └─ 天线设计 (4TX/4RX)               │
├─────────────────────────────────────────┤
│  第3层：系统级 (System Level)            │
│  └─ 完整雷达系统                        │
│     ├─ DCA1000数据采集                  │
│     ├─ LaunchPad扩展                    │
│     └─ 应用Demo                         │
└─────────────────────────────────────────┘
```

---

## 第二章：EVM硬件架构

### 2.1 AWRL6844-EVM核心组件

#### 功能方框图解析

**完整系统架构**：

```
┌─────────────────────────────────────────────────────────┐
│                    AWRL6844-EVM 系统架构                 │
└─────────────────────────────────────────────────────────┘

1️⃣ 电源子系统
   ├─ DC Jack输入 (5V-12V)
   ├─ TPS650365x PMIC（多路输出）
   │   ├─ 1.3V → 数字逻辑
   │   ├─ 1.8V → IO接口
   │   └─ 1.2V → RF电路
   ├─ TLV75533 (5V → 1.3V)
   └─ TPS2121 电源切换

2️⃣ 雷达核心 (AWRL6844)
   ├─ C66x DSP (450MHz)
   ├─ R5F MCU (200MHz)
   ├─ HWA 1.2 (200MHz)
   ├─ 4TX/4RX 天线
   └─ 60-64GHz RF前端

3️⃣ 存储子系统
   ├─ 64-bit QSPI Flash (板载)
   ├─ EEPROM (CAT24C08)
   └─ 片上内存 (2.5MB)

4️⃣ 调试/通信接口
   ├─ FT4232H (USB-UART/SPI桥)
   │   ├─ COM3: XDS110 JTAG
   │   ├─ COM4: UART调试 (115200)
   │   └─ COM5/6: 保留
   ├─ XDS110 仿真器 (板载)
   └─ 60-pin HD连接器 (DCA1000)

5️⃣ 外设接口
   ├─ 2x CAN-FD (TCAN3403DRQ1)
   ├─ 1x LIN (TLIN1039DDFRQ1)
   ├─ GPIO扩展
   └─ I2C总线

6️⃣ 电流/温度监控
   ├─ INA228A x3 (电流传感器)
   │   ├─ 监控1.3V轨
   │   ├─ 监控1.8V轨
   │   └─ 监控1.2V轨
   └─ TMP451A (温度传感器)

7️⃣ 用户接口
   ├─ LED x2 (用户可编程)
   ├─ 按钮 x2 (用户按钮)
   └─ 复位按钮 (NRST)
```

### 2.2 关键硬件特性

#### 2.2.1 天线配置 ⭐⭐⭐⭐⭐

**4TX + 4RX = 16 MIMO虚拟天线**

**天线布局**：
```
Rogers RO3003 PCB基板（高性能RF材料）

发射天线 (TX):        接收天线 (RX):
TX1  TX2             RX1  RX2
TX3  TX4             RX3  RX4

虚拟天线阵列 (16个):
[TX1-RX1] [TX1-RX2] [TX1-RX3] [TX1-RX4]
[TX2-RX1] [TX2-RX2] [TX2-RX3] [TX2-RX4]
[TX3-RX1] [TX3-RX2] [TX3-RX3] [TX3-RX4]
[TX4-RX1] [TX4-RX2] [TX4-RX3] [TX4-RX4]
```

**天线参数**：
| 参数 | 规格 | 说明 |
|-----|------|------|
| **频率范围** | 57-64 GHz | 7GHz带宽 |
| **天线增益** | 5-6 dBi | 每个天线元件 |
| **波束宽度** | ±60° | 方位角和仰角 |
| **角度分辨率** | 29° | 基于16 MIMO |
| **RX天线间距** | λ/2 | 半波长间隔 |
| **TX天线间距** | λ | 全波长间隔 |

**为什么4TX比3TX好？**
```
AWRL6843 (3TX/4RX):
- 12个虚拟天线
- 角度分辨率: 约40°
- 适合基础应用

AWRL6844 (4TX/4RX):
- 16个虚拟天线 ← 多33%
- 角度分辨率: 29° ← 提升27%
- 更精确的目标定位
- 更适合高级应用
```

#### 2.2.2 电源管理拓扑

**TPS650365x PMIC核心特性**：

```
输入: 5V-12V DC
    ↓
TPS650365x (7路输出)
    ├─ B1: 1.3V @ 最大1A    → R5F MCU, 数字逻辑
    ├─ B2: 1.8V @ 最大1A    → IO接口
    ├─ B3: 1.2V @ 最大1A    → RF模拟电路
    ├─ VOUT_PLL: 1.3V       → PLL
    ├─ VOUT_Synth: 1.2V     → 频率合成器
    ├─ VDDA_RF: 1.2V        → RF放大器
    └─ VBGAP: 1.2V          → 带隙参考
```

**电流监控**：
```
INA228A传感器x3:
├─ 监控1: 1.3V轨 (40mΩ采样电阻)
├─ 监控2: 1.8V轨 (40mΩ采样电阻)
└─ 监控3: 1.2V轨 (20mΩ+10mΩ采样电阻)

作用:
- 实时测量功耗
- 调试低功耗模式
- 性能优化
```

#### 2.2.3 存储架构

**片上存储 (On-Chip Memory)**：
```
总容量: 2.5MB

R5F子系统 (APPSS):
├─ TCM: 768KB
│   ├─ 程序存储
│   └─ 数据存储
└─ 可配置为L3或APPSS

C66x DSP子系统 (DSS):
├─ L2 Cache: 384KB
│   ├─ DSP程序
│   └─ DSP数据
└─ 高速计算

L3内存 (雷达数据立方体):
├─ 容量: 512KB
│   ├─ ADC采样数据
│   ├─ Range-FFT结果
│   └─ Doppler-FFT结果
└─ 由HWA加速器访问

共享RAM:
├─ 容量: 896KB
│   ├─ R5F和DSP共享
│   ├─ 数据交换区
│   └─ DMA缓冲区
```

**片外存储 (Off-Chip)**：
```
QSPI Flash (64-bit接口):
├─ 容量: 通常16MB-64MB
├─ 存储内容:
│   ├─ SBL引导程序
│   ├─ 应用固件
│   ├─ 校准数据
│   └─ 配置文件（可选）
└─ 速度: 50MHz QSPI时钟
```

**内存对比 (6432 vs 6844)**：
| 内存类型 | xWRL6432 | xWRL6844 | 提升 |
|---------|---------|---------|------|
| **总片上内存** | 1MB | 2.5MB | 150% ⬆️ |
| **APPSS** | 512KB | 768KB | 50% ⬆️ |
| **L3 (雷达立方体)** | 256KB | 512KB | 100% ⬆️ |
| **DSS L2** | 无 | 384KB | 新增 |
| **共享RAM** | 256KB | 896KB | 250% ⬆️ |

#### 2.2.4 调试接口详解

**XDS110调试器**：
```
功能:
├─ JTAG调试 (4线)
│   ├─ TCK: 时钟
│   ├─ TMS: 模式选择
│   ├─ TDI: 数据输入
│   └─ TDO: 数据输出
├─ 固件下载
│   ├─ RAM加载（调试）
│   └─ Flash烧录
└─ 实时跟踪
    ├─ 断点调试
    └─ 变量监控

连接:
USB Micro-B → FT4232H → XDS110 → AWRL6844
```

**串口映射**：
```
FT4232H桥接器 (4通道):

COM3 (XDS110 UART):
├─ 用途: CCS调试输出
├─ 波特率: 115200
└─ 格式: 8N1

COM4 (应用UART):
├─ 用途: 雷达配置命令
├─ 波特率: 115200
├─ 协议: CLI命令行
└─ 数据: 雷达输出数据

COM5 (保留):
└─ 可用于自定义通信

COM6 (保留):
└─ 可用于自定义通信
```

---

## 第三章：硬件与固件的关系

### 3.1 硬件决定固件能力

**硬件 → 固件的映射关系**：

```
┌──────────────────┐        ┌──────────────────┐
│  硬件资源        │   →    │  固件功能        │
└──────────────────┘        └──────────────────┘

4TX天线           →  支持4TX chirp配置
4RX天线           →  16通道ADC数据采集
2.5MB片上内存     →  更大的雷达立方体
C66x DSP          →  复杂算法加速
HWA 1.2           →  FFT/CFAR硬件加速
QSPI Flash        →  固件持久化存储
```

### 3.2 固件如何使用硬件

#### 3.2.1 天线配置示例

**固件中的天线配置代码**：

```c
// mmw_config.c (固件源码)

// 通道配置（基于硬件4TX/4RX）
channelCfg 15 7 0  
// 参数1=15: RX使能掩码
//   15 = 0b1111 → RX1,RX2,RX3,RX4全开
// 参数2=7: TX使能掩码  
//   7 = 0b0111 → TX1,TX2,TX3开，TX4关（3TX模式）
// 参数2=15: TX使能掩码
//   15 = 0b1111 → TX1,TX2,TX3,TX4全开（4TX模式）

// 硬件约束检查
if (txChannelMask > 0x0F) {
    // 错误：硬件只有4个TX
    return ERROR_INVALID_TX_MASK;
}

if (rxChannelMask > 0x0F) {
    // 错误：硬件只有4个RX
    return ERROR_INVALID_RX_MASK;
}
```

**配置文件中的天线配置**：
```cfg
% AWRL6844专用4TX配置
channelCfg 15 15 0
% RX=15 (0b1111) → 4个RX全开
% TX=15 (0b1111) → 4个TX全开

% AWRL6843兼容3TX配置
channelCfg 15 7 0
% RX=15 (0b1111) → 4个RX全开
% TX=7 (0b0111) → 只用3个TX（浪费硬件）
```

#### 3.2.2 内存映射示例

**固件如何分配内存**：

```c
// radar_ss.lds (链接脚本)

MEMORY
{
    // R5F程序存储
    R5F_TCMA : origin=0x00000000  length=0x00008000  /* 32KB */
    R5F_TCMB : origin=0x00080000  length=0x00008000  /* 32KB */
    
    // L3雷达数据立方体
    L3_RAM   : origin=0x51000000  length=0x00080000  /* 512KB */
    
    // 共享内存
    SHARED   : origin=0x10200000  length=0x000E0000  /* 896KB */
    
    // DSP L2内存
    DSP_L2   : origin=0x007E0000  length=0x00060000  /* 384KB */
}

SECTIONS
{
    // ADC数据缓冲区 → L3内存
    .radarCube : {
        *(.adcData)
    } > L3_RAM
    
    // DSP算法 → DSP L2
    .dspCode : {
        *(.dspAlgo)
    } > DSP_L2
    
    // 共享数据 → 共享内存
    .sharedData : {
        *(.ipcBuffer)
    } > SHARED
}
```

**内存使用影响性能**：
```
小内存（1MB，如6432）:
├─ 限制：
│   ├─ ADC采样点少（最多256点）
│   ├─ Chirp数量少（最多64 chirps）
│   └─ FFT大小受限
└─ 结果：距离/速度分辨率降低

大内存（2.5MB，如6844）:
├─ 优势：
│   ├─ ADC采样点多（最多512点）
│   ├─ Chirp数量多（最多128+ chirps）
│   └─ FFT大小更大
└─ 结果：更高的分辨率和精度
```

### 3.3 硬件约束影响固件

**硬件限制 → 固件必须遵守**：

| 硬件限制 | 固件约束 | 违反后果 |
|---------|---------|---------|
| **4个TX最多** | `txMask ≤ 0xF` | 配置失败/硬件损坏 |
| **4个RX最多** | `rxMask ≤ 0xF` | 数据采集失败 |
| **ADC采样率≤25Msps** | `samplingRate ≤ 25M` | ADC溢出/数据丢失 |
| **IF带宽≤10MHz** | `IFmax ≤ 10MHz` | 频率折叠/混叠 |
| **L3内存512KB** | `dataSize ≤ 512KB` | 内存溢出/崩溃 |
| **Flash擦写次数** | 限制烧录次数 | Flash损坏 |

**示例：违反硬件约束**：
```c
// ❌ 错误：尝试使用8个TX（硬件只有4个）
channelCfg 15 255 0  // txMask=255=0xFF（8个TX）

// 固件检查：
if (txMask > 0x0F) {
    printf("Error: Only 4 TX channels available!\n");
    return -1;
}

// ❌ 错误：ADC采样率过高
adcConfig 1 1 50  // 50Msps（硬件最大25Msps）

// 硬件限制：
if (samplingRate > 25000000) {
    printf("Error: Max sampling rate is 25Msps!\n");
    return -1;
}
```

---

## 第四章：硬件与SysConfig的关系

### 4.1 SysConfig读取硬件定义

**SysConfig如何知道硬件信息？**

```
┌─────────────────────────────────────────┐
│  硬件设计 → TI → 设备数据库 → SysConfig  │
└─────────────────────────────────────────┘

1️⃣ TI硬件设计团队设计AWRL6844
   ├─ 引脚定义（207个引脚）
   ├─ 外设配置（UART/SPI/CAN等）
   └─ 电气特性（电压/电流）

2️⃣ 生成设备数据库文件
   ├─ AWRL6844.json
   ├─ xwrl684x_pinmux.json
   └─ peripherals/*.json

3️⃣ 打包到SysConfig安装包
   └─ C:\ti\sysconfig_1.20.0\products\68xx\

4️⃣ SysConfig读取数据库
   └─ 显示可用引脚和外设
```

### 4.2 硬件定义文件解析

**AWRL6844.json（简化版）**：
```json
{
  "name": "AWRL6844",
  "displayName": "AWRL6844 mmWave Radar Sensor",
  "family": "xWRL684x",
  "device": {
    "cpu": ["C674x DSP", "ARM Cortex-R5F"],
    "memory": {
      "flash": "External QSPI",
      "sram": "2.5MB On-chip"
    },
    "package": "FCBGA_207",
    "pins": 207
  },
  
  "pins": [
    {
      "number": "A1",
      "name": "GPIO_0",
      "ball": "A1",
      "type": "IO",
      "functions": [
        {"name": "GPIO", "mode": 0},
        {"name": "UART0_TX", "mode": 1},
        {"name": "SPI0_CLK", "mode": 2}
      ],
      "voltage": "1.8V",
      "current_max": "8mA"
    },
    {
      "number": "A2",
      "name": "GPIO_1",
      "ball": "A2",
      "type": "IO",
      "functions": [
        {"name": "GPIO", "mode": 0},
        {"name": "UART0_RX", "mode": 1},
        {"name": "SPI0_MISO", "mode": 2}
      ]
    }
    // ... 205个引脚定义
  ],
  
  "peripherals": {
    "UART": {
      "instances": ["UART0", "UART1"],
      "pins": {
        "UART0": {
          "TX": ["GPIO_0", "GPIO_28"],
          "RX": ["GPIO_1", "GPIO_29"]
        }
      },
      "baudrates": [9600, 115200, 921600, 3000000],
      "features": ["DMA", "FIFO", "IrDA"]
    },
    
    "SPI": {
      "instances": ["SPI0", "SPI1", "SPI2"],
      "maxSpeed": "50MHz",
      "modes": ["Master", "Slave"]
    },
    
    "CAN": {
      "instances": ["CAN_FD0", "CAN_FD1"],
      "type": "CAN-FD",
      "bitrate": "5Mbps"
    },
    
    "ADC": {
      "channels": 4,
      "resolution": "12/14/16 bits",
      "samplingRate": "25Msps"
    }
  }
}
```

### 4.3 SysConfig使用硬件信息

**SysConfig配置流程**：

```
1️⃣ 用户打开SysConfig
   └─ 加载AWRL6844设备数据

2️⃣ 显示可配置外设
   ├─ UART0/UART1
   ├─ SPI0/SPI1/SPI2
   ├─ I2C0
   ├─ CAN-FD0/CAN-FD1
   └─ GPIO (207个引脚)

3️⃣ 用户添加UART0
   └─ SysConfig显示可用引脚：
       TX: GPIO_0 or GPIO_28
       RX: GPIO_1 or GPIO_29

4️⃣ 用户选择GPIO_28作为TX
   └─ SysConfig检查冲突：
       ✅ GPIO_28未被占用
       ✅ 可以分配

5️⃣ SysConfig生成代码
   └─ ti_drivers_config.c:
       // UART0 TX引脚配置
       GPIO_setMux(GPIO_28, GPIO_MUX_UART0_TX);
```

**引脚冲突检测示例**：
```
场景1: 用户尝试同时配置
- UART0_TX → GPIO_28
- SPI0_CLK → GPIO_28

SysConfig检测:
❌ 冲突！GPIO_28已被UART0占用
⚠️ 提示：请选择其他引脚或禁用UART0

场景2: 用户配置
- UART0_TX → GPIO_28
- UART0_RX → GPIO_29
- SPI0_CLK → GPIO_0

SysConfig检测:
✅ 无冲突
✅ 生成配置代码
```

### 4.4 硬件约束在SysConfig中的体现

**SysConfig如何强制硬件约束**：

```javascript
// SysConfig配置校验逻辑（伪代码）

function validateUARTConfig(config) {
    // 检查波特率
    if (!硬件支持波特率.includes(config.baudRate)) {
        return {
            error: "不支持的波特率",
            supported: [9600, 115200, 921600, 3000000]
        };
    }
    
    // 检查引脚分配
    if (!UART0可用引脚.includes(config.txPin)) {
        return {
            error: "TX引脚不可用于UART0",
            availablePins: ["GPIO_0", "GPIO_28"]
        };
    }
    
    // 检查引脚冲突
    if (引脚已被占用(config.txPin)) {
        return {
            error: "引脚冲突",
            conflictWith: 获取占用者(config.txPin)
        };
    }
    
    return {success: true};
}
```

**实际限制示例**：

| 硬件限制 | SysConfig约束 | 用户看到的提示 |
|---------|--------------|---------------|
| GPIO_0只能是1.8V | 电压选择器只显示1.8V | "此引脚仅支持1.8V" |
| UART最大3Mbps | 波特率下拉框最大3000000 | "最大波特率: 3Mbps" |
| SPI最大50MHz | 频率滑块上限50MHz | "最大SPI时钟: 50MHz" |
| 只有4个TX天线 | TX通道复选框只有4个 | "TX0-TX3可用" |

---

## 第五章：硬件设计对SDK的影响

### 5.1 硬件决定SDK架构

**AWRL6844硬件 → SDK设计决策**：

```
硬件特性                SDK对应设计
─────────────────      ─────────────────
C66x DSP (450MHz)  →   DSP专用算法库
                       └─ mmwavelib (FFT/CFAR/DOA)

R5F MCU (200MHz)   →   R5F控制程序
                       └─ mmw_demo主控制

HWA 1.2 (200MHz)   →   硬件加速器驱动
                       └─ HWA API

4TX/4RX天线        →   4TX专用配置
                       └─ xWRL6844_4T4R_tdm.cfg

2.5MB片上内存      →   大容量雷达立方体
                       └─ 512KB L3内存支持

LVDS数据接口       →   DCA1000原始数据采集
                       └─ 12/14/16-bit ADC
```

### 5.2 SDK示例与硬件的映射

**mmWave Demo固件如何使用硬件**：

```c
// mmw_config.h (固件配置)

// 基于硬件4TX/4RX
#define MAX_TX_ANTENNAS  4   // 硬件最大TX数
#define MAX_RX_ANTENNAS  4   // 硬件最大RX数
#define MAX_VIRTUAL_ANT  16  // 4TX * 4RX = 16 MIMO

// 基于内存大小
#define MAX_ADC_SAMPLES  512   // L3内存支持
#define MAX_CHIRPS       128   // 内存足够大
#define MAX_RANGE_BINS   512
#define MAX_DOPPLER_BINS 128

// 基于DSP性能
#define DSP_CLOCK_MHZ    450   // C66x频率
#define MAX_PROC_TIME_MS 33    // 30FPS时的处理时间
```

### 5.3 不同硬件版本的SDK差异

**xWRL6432 vs xWRL6844 SDK差异**：

| 特性 | xWRL6432 SDK | xWRL6844 SDK | 原因 |
|-----|-------------|-------------|------|
| **DSP支持** | ❌ 无DSP | ✅ C66x DSP | 硬件差异 |
| **最大TX** | 2个 | 4个 | 硬件天线数 |
| **最大采样率** | 12.5Msps | 25Msps | ADC硬件升级 |
| **L3内存** | 256KB | 512KB | 内存容量 |
| **mmwavelib** | 精简版 | 完整版 | DSP算法库 |
| **LVDS接口** | ❌ 无 | ✅ 有 | 硬件接口 |

**示例：6432代码迁移到6844需要的改动**：

```c
// xWRL6432代码
#define NUM_TX_ANT 2   // 6432只有2个TX
#define NUM_RX_ANT 3   // 6432只有3个RX

channelCfg 7 3 0       // RX=7(0b0111), TX=3(0b0011)

// ↓ 迁移到xWRL6844

#define NUM_TX_ANT 4   // 6844有4个TX
#define NUM_RX_ANT 4   // 6844有4个RX

channelCfg 15 15 0     // RX=15(0b1111), TX=15(0b1111)

// 性能提升：
// 虚拟天线：2×3=6 → 4×4=16 (提升167%)
// 角度分辨率：更精确
```

### 5.4 硬件参考设计的价值

**为什么需要研究EVM硬件设计？**

**对固件开发者的价值**：

1. **理解硬件能力边界**
   ```
   问题：能否同时开启4个UART？
   查阅EVM设计：
   ├─ FT4232H只有4个通道
   ├─ COM3: XDS110 (固定)
   ├─ COM4: UART0 (固定)
   ├─ COM5/6: 可用
   └─ 结论：最多3个用户UART
   ```

2. **功耗优化**
   ```
   问题：如何降低功耗？
   查阅电源设计：
   ├─ INA228A监控各电源轨
   ├─ 识别高功耗模块
   ├─ 通过寄存器关闭未使用外设
   └─ 使用低功耗模式
   ```

3. **故障排查**
   ```
   问题：雷达无输出
   检查硬件设计：
   ├─ 天线是否正确连接？
   ├─ RF电源轨是否正常？（检查1.2V_RF）
   ├─ SOP开关设置正确吗？
   └─ PMIC看门狗是否超时？
   ```

4. **性能优化**
   ```
   问题：如何提高角度分辨率？
   查阅天线设计：
   ├─ 6844有4个TX
   ├─ 使用全部4TX配置
   ├─ 16 MIMO虚拟天线
   └─ 角度分辨率从40°→29°
   ```

**对系统集成者的价值**：

1. **接口设计**
   - 了解可用的GPIO数量
   - 选择合适的通信接口（SPI/I2C/CAN）
   - 设计扩展板（BoosterPack）

2. **电源设计**
   - 参考PMIC配置
   - 计算功耗预算
   - 设计散热方案

3. **PCB布局**
   - 参考天线布局
   - 射频走线设计
   - 电源平面设计

---

## 📝 总结

### Part6核心要点

1. **硬件是基础**
   - 硬件定义了软件的能力边界
   - 4TX/4RX天线 = 16 MIMO虚拟天线
   - 2.5MB内存 = 更大的雷达立方体

2. **硬件 → 固件映射**
   - 天线数量决定channelCfg配置
   - 内存大小决定采样点数
   - DSP性能决定算法复杂度

3. **SysConfig读取硬件定义**
   - 设备数据库（JSON格式）
   - 引脚复用配置
   - 硬件约束检查

4. **SDK基于硬件设计**
   - mmwavelib需要DSP支持
   - 配置文件针对4TX优化
   - 驱动程序匹配硬件外设

5. **硬件设计文件的价值**
   - 理解能力边界
   - 故障排查依据
   - 优化性能参考

### 学习建议

**新手路线**：
```
Day 1: 阅读EVM User Guide了解硬件组成
Day 2: 理解天线配置和电源管理
Day 3: 查看SysConfig设备数据库
Day 4: 对比6432和6844硬件差异
Day 5: 实践：修改channelCfg使用4TX
```

**进阶路线**：
```
Week 1: 深入研究内存映射
Week 2: 分析电源管理优化功耗
Week 3: 研究天线设计提升性能
Week 4: 设计自己的硬件扩展板
```

---

## 🔗 相关文档

### 本项目文档
- [Part1 - SDK基础概念](Part1-SDK基础概念与三目录详解.md)
- [Part2 - 固件校验方法](Part2-固件校验方法完整指南.md)
- [Part3 - SDK与固件关系](Part3-SDK与固件关系及工作流程.md)
- [Part4 - 实践案例FAQ](Part4-实践案例与常见问题.md)
- [Part5 - SysConfig工具](Part5-SysConfig工具深度分析.md)
- Part7 - Radar Academy学习资源（待创建）

### TI官方文档
- **ZHCUCO8** - AWRL6844 EVM用户指南（中文）
- **SWRU630A** - AWRL6844 EVM规格书（英文）
- **AWRL6844数据手册** - 芯片完整规格
- **Migration Guide** - 从6432迁移到6844指南

---

**最后更新**：2025-12-25  
**文档作者**：项目开发团队  
**适用硬件**：AWRL6844-EVM  
**相关SDK**：MMWAVE_L_SDK 06.01.00.01 + radar_toolbox 3.30.00.06
