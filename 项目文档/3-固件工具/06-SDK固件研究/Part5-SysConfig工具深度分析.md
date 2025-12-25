# 🔧 TI SysConfig 工具深度分析

> **文档版本**: v1.0  
> **创建日期**: 2025-12-25  
> **适用硬件**: AWRL6844-EVM  
> **SysConfig版本**: 1.20.0  
> **前置文档**: [Part1-SDK基础概念](Part1-SDK基础概念与三目录详解.md)

---

## 📋 目录

- [第一章：SysConfig是什么](#第一章sysconfig是什么)
- [第二章：SysConfig目录结构分析](#第二章sysconfig目录结构分析)
- [第三章：SysConfig与固件的关系](#第三章sysconfig与固件的关系)
- [第四章：SysConfig与SDK的关系](#第四章sysconfig与sdk的关系)
- [第五章：实际应用案例](#第五章实际应用案例)

---

## 第一章：SysConfig是什么

### 1.1 核心定义

**SysConfig** = **System Configuration Tool**（系统配置工具）

**官方描述**：
- 🎨 **图形化配置工具**：通过GUI界面配置芯片外设
- 🔧 **代码生成器**：自动生成初始化代码
- ✅ **冲突检查器**：验证配置的正确性和兼容性
- 📦 **跨平台工具**：支持TI多个MCU/MPU/无线产品线

**类比理解**：
```
传统方式 = 手写配置代码（容易出错）
SysConfig = 拖拽配置 + 自动生成代码（快速准确）

就像：
手动布线电路板 vs 使用EDA软件自动布线
```

### 1.2 SysConfig的作用

#### 作用1: 简化硬件配置 ⭐⭐⭐

**解决的问题**：
```
传统方式（困难）：
1. 阅读数据手册（1000+页）
2. 手写寄存器配置代码
3. 调试硬件冲突
4. 修改参数重新编译

SysConfig方式（简单）：
1. 图形界面拖拽组件
2. 设置参数（下拉框/输入框）
3. 自动检查冲突
4. 一键生成代码
```

**配置范围**：
- ✅ GPIO引脚配置
- ✅ 外设初始化（UART、SPI、I2C等）
- ✅ 时钟配置
- ✅ 中断优先级
- ✅ DMA通道分配
- ✅ 电源管理

#### 作用2: 自动生成初始化代码 ⭐⭐⭐⭐

**工作流程**：
```
1. 用户在GUI中配置
   ├─ 选择外设（如UART1）
   ├─ 设置参数（波特率115200）
   └─ 分配引脚（TX=P8.28, RX=P8.29）

2. SysConfig分析配置
   ├─ 检查引脚冲突
   ├─ 验证参数有效性
   └─ 计算时钟分频

3. 生成C代码
   ├─ ti_drivers_config.c    ← 配置实现代码
   ├─ ti_drivers_config.h    ← 头文件声明
   └─ Board.h                ← 板级定义
```

**生成的代码示例**：
```c
// ti_drivers_config.c（自动生成）
#include "ti_drivers_config.h"

// UART配置
const UART_Config UART_config[1] = {
    {
        .fxnTablePtr = &UARTMSP432E4_fxnTable,
        .object      = &uartMSP432E4Objects[0],
        .hwAttrs     = &uartMSP432E4HWAttrs[0]
    }
};

// GPIO配置
const GPIO_Config GPIO_config[2] = {
    {GPIO_LED_RED,   GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW},
    {GPIO_BUTTON,    GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_RISING}
};
```

#### 作用3: 配置验证和冲突检查 ⭐⭐⭐⭐⭐

**自动检查**：
```
❌ 引脚冲突检测
   例如：同一个GPIO引脚不能同时分配给UART和SPI

❌ 时钟速率验证
   例如：外设时钟不能超过芯片最大频率

❌ DMA通道冲突
   例如：同一DMA通道不能同时服务两个外设

✅ 实时错误提示
   配置错误时立即显示红色警告
```

**示例错误提示**：
```
⚠️ Pin conflict detected!
   GPIO_28 is already assigned to UART1_TX
   Cannot assign to SPI0_CLK

⚠️ Clock frequency too high!
   Peripheral clock 80MHz exceeds maximum 72MHz
   Please reduce PLL multiplier
```

### 1.3 为什么需要SysConfig？

#### 问题背景

**没有SysConfig时的开发流程**：
```
1. 阅读芯片数据手册（1000+页）
   ├─ 查找GPIO寄存器地址
   ├─ 查找UART配置寄存器
   └─ 查找时钟树配置

2. 手写初始化代码
   ├─ 容易出错（寄存器地址、位操作）
   ├─ 难以维护（参数硬编码）
   └─ 移植困难（不同芯片差异大）

3. 调试硬件问题
   ├─ 引脚冲突难以发现
   ├─ 时钟配置错误难以排查
   └─ 花费大量时间

4. 修改配置
   ├─ 修改代码
   ├─ 重新编译
   └─ 重新测试
```

**使用SysConfig后**：
```
1. 打开SysConfig界面（1分钟）
   ├─ 可视化芯片引脚图
   ├─ 拖拽添加外设
   └─ 下拉框选择参数

2. 自动生成代码（1秒）
   ├─ 代码规范统一
   ├─ 参数自动计算
   └─ 跨芯片移植容易

3. 实时冲突检查（0秒）
   ├─ 配置错误立即提示
   ├─ 避免编译后才发现问题
   └─ 节省调试时间

4. 修改配置（1分钟）
   ├─ GUI中修改参数
   ├─ 重新生成代码
   └─ 编译即可
```

**时间对比**：
| 任务 | 手动配置 | 使用SysConfig | 节省时间 |
|-----|---------|--------------|---------|
| GPIO配置 | 30分钟 | 5分钟 | 83% ⬇️ |
| UART配置 | 1小时 | 10分钟 | 83% ⬇️ |
| 时钟配置 | 2小时 | 15分钟 | 88% ⬇️ |
| 调试冲突 | 4小时 | 0分钟 | 100% ⬇️ |
| **总计** | **7.5小时** | **30分钟** | **93% ⬇️** |

---

## 第二章：SysConfig目录结构分析

### 2.1 安装目录概览

**路径**: `C:\ti\sysconfig_1.20.0\`

**目录大小**: 约500MB

**主要组成**：
```
C:\ti\sysconfig_1.20.0\
│
├── 📂 nw/                          ← ⭐ 核心应用程序
│   ├── sysconfig.exe              # Windows可执行文件
│   ├── node.exe                   # Node.js运行时
│   └── ...（应用程序文件）
│
├── 📂 gcc/                         ← GNU编译器（ARM）
│   └── ...（编译器工具链）
│
├── 📂 ccs/                         ← Code Composer Studio集成
│   └── ...（CCS插件文件）
│
├── 📂 docs/                        ← 文档
│   ├── SysConfig_User_Guide.pdf
│   └── ...（API文档）
│
├── 📂 eclipse/                     ← Eclipse插件
│   └── ...（Eclipse集成文件）
│
├── 📂 products/                    ← ⭐⭐⭐ 芯片支持库
│   ├── .metadata/                 # 设备数据库
│   ├── 68xx/                      # xWRL68xx系列（包含6844）
│   ├── 62xx/                      # xWR62xx系列
│   ├── 18xx/                      # xWR18xx系列
│   └── ...（其他芯片系列）
│
└── 📄 README.txt                   # 说明文档
```

### 2.2 核心目录详解

#### 2.2.1 nw/ - 应用程序核心

**内容**：
- `sysconfig.exe` - 主程序（独立GUI工具）
- `node.exe` - Node.js引擎（SysConfig基于Electron）
- 前端资源（HTML/CSS/JS）

**运行方式**：
```powershell
# 方式1: 直接启动GUI
C:\ti\sysconfig_1.20.0\nw\sysconfig.exe

# 方式2: 命令行模式
C:\ti\sysconfig_1.20.0\nw\sysconfig.exe --script generate_config.syscfg

# 方式3: 通过CCS启动（集成模式）
# 在CCS中打开.syscfg文件时自动调用
```

#### 2.2.2 products/ - 设备支持库 ⭐⭐⭐

**路径**: `C:\ti\sysconfig_1.20.0\products\`

**AWRL6844支持文件**：
```
products/
├── .metadata/
│   └── product.json               # 设备数据库索引
│
├── 68xx/                          ← AWRL6844在这里
│   ├── xwrl684x/                  # xWRL684x系列
│   │   ├── devices/
│   │   │   ├── AWRL6844.json     ← 6844设备定义
│   │   │   ├── IWRL6844.json
│   │   │   └── xWRL6843.json
│   │   │
│   │   ├── pinmux/               # 引脚复用配置
│   │   │   └── xwrl684x_pinmux.json
│   │   │
│   │   └── peripherals/          # 外设定义
│   │       ├── uart.json
│   │       ├── spi.json
│   │       ├── gpio.json
│   │       └── ...
│   │
│   └── templates/                # 代码模板
│       ├── ti_drivers_config.c.xdt
│       ├── ti_drivers_config.h.xdt
│       └── Board.h.xdt
│
└── ...（其他芯片系列）
```

**设备定义文件示例**（AWRL6844.json）：
```json
{
  "name": "AWRL6844",
  "family": "xWRL684x",
  "cpu": "C674x + R5F",
  "flashSize": "2MB",
  "ramSize": "256KB",
  "pins": [
    {
      "number": "A1",
      "name": "GPIO_0",
      "functions": ["GPIO", "UART0_TX", "SPI0_CLK"]
    },
    {
      "number": "A2",
      "name": "GPIO_1",
      "functions": ["GPIO", "UART0_RX", "SPI0_MOSI"]
    }
    // ... 207个引脚定义
  ],
  "peripherals": [
    "UART0", "UART1",
    "SPI0", "SPI1",
    "I2C0",
    "GPIO",
    "CAN",
    "QSPI"
  ]
}
```

#### 2.2.3 gcc/ - ARM编译器

**作用**：
- 为某些芯片提供编译工具链
- AWRL6844使用ti-arm-clang（在MMWAVE_L_SDK中）
- SysConfig主要用于代码生成，不负责编译

#### 2.2.4 ccs/ - CCS集成插件

**作用**：
- 在Code Composer Studio中集成SysConfig
- 双击`.syscfg`文件时自动打开SysConfig编辑器
- 编译时自动调用SysConfig生成代码

**集成位置**：
```
CCS安装目录/
├── eclipse/
│   └── plugins/
│       └── com.ti.sysconfig_x.x.x/  ← CCS插件
│           └── 引用 sysconfig_1.20.0/ccs/
```

---

## 第三章：SysConfig与固件的关系

### 3.1 在固件开发流程中的位置

```
┌─────────────────────────────────────────────────────────┐
│              固件开发完整流程                            │
└─────────────────────────────────────────────────────────┘

1️⃣ 硬件配置阶段（使用SysConfig）
   ├─ 打开 example.syscfg 文件
   ├─ 配置GPIO、UART、SPI等外设
   ├─ 设置引脚复用和参数
   └─ 保存配置（生成.syscfg文件）
        ↓
   【SysConfig自动生成】
        ↓
   ✅ ti_drivers_config.c   ← 配置实现代码
   ✅ ti_drivers_config.h   ← 头文件
   ✅ Board.h               ← 板级定义
        ↓
2️⃣ 应用代码开发阶段
   ├─ #include "ti_drivers_config.h"
   ├─ 调用初始化函数
   ├─ 编写应用逻辑
   └─ 使用外设（UART_read、GPIO_write等）
        ↓
3️⃣ 编译阶段（ti-arm-clang）
   ├─ 编译SysConfig生成的代码
   ├─ 编译应用代码
   ├─ 链接生成.out文件
   └─ 转换为.appimage固件
        ↓
4️⃣ 烧录和运行
   ├─ 烧录.appimage到Flash
   └─ 芯片运行固件
```

### 3.2 SysConfig配置文件（.syscfg）

#### 格式说明

**.syscfg文件**：
- 📝 **纯文本**格式（JavaScript语法）
- 🔧 描述硬件配置
- 💾 可版本控制（Git友好）
- 🔄 可在不同项目间复用

**示例文件**（example.syscfg）：
```javascript
/**
 * AWRL6844 硬件配置文件
 * 用于配置GPIO、UART等外设
 */

const GPIO = scripting.addModule("/ti/drivers/GPIO");
const UART = scripting.addModule("/ti/drivers/UART");

// GPIO配置：LED
const gpio0 = GPIO.addInstance();
gpio0.$name = "CONFIG_GPIO_LED_RED";
gpio0.mode = "Output";
gpio0.gpioPin.$assign = "GPIO_0";

// UART配置：调试串口
const uart0 = UART.addInstance();
uart0.$name = "CONFIG_UART_DEBUG";
uart0.baudRate = 115200;
uart0.uart.$assign = "UART0";
uart0.uart.txPin.$assign = "GPIO_28";
uart0.uart.rxPin.$assign = "GPIO_29";
```

#### 与固件的关系

```
.syscfg配置文件
    ↓ SysConfig工具处理
自动生成的C代码
    ↓ 编译器编译
固件二进制代码
    ↓ 烧录
Flash存储器
    ↓ 运行
芯片执行
```

**关键点**：
- ✅ `.syscfg`文件**不烧录**到芯片
- ✅ 生成的C代码**编译后**成为固件的一部分
- ✅ 修改`.syscfg`后需要**重新编译**固件

### 3.3 SysConfig在不同固件中的角色

#### 角色1: 源码项目（需要SysConfig）

**场景**：从SDK示例开始开发

**目录结构**：
```
mmw_demo/
├── mss/                           # MSS核心代码
│   ├── mmw_main.c                # 主程序
│   ├── example.syscfg            ← SysConfig配置
│   └── ...
│
├── dss/                           # DSS核心代码
│   └── ...
│
└── ti_drivers_config.c/h         ← SysConfig生成（自动）
```

**开发流程**：
```
1. 修改 example.syscfg
   └─ 添加UART2外设

2. 保存文件
   └─ SysConfig自动重新生成ti_drivers_config.c

3. 编译项目
   └─ 新的UART2配置编译到固件

4. 烧录测试
   └─ UART2可用
```

#### 角色2: 预编译固件（不需要SysConfig）

**场景**：使用TI提供的预编译固件

**文件**：
```
mmwave_demo.release.appimage      ← 预编译固件
```

**特点**：
- ❌ 没有`.syscfg`文件
- ❌ 没有源码
- ✅ 硬件配置已固定（编译时确定）
- ✅ 无法修改GPIO/UART等配置

**使用方式**：
```
1. 直接烧录固件
2. 通过CLI命令配置雷达参数（不是硬件配置）
3. 硬件配置无法更改
```

#### 对比总结

| 特征 | 源码项目 | 预编译固件 |
|-----|---------|-----------|
| `.syscfg`文件 | ✅ 有 | ❌ 无 |
| 修改硬件配置 | ✅ 可以 | ❌ 不可以 |
| 需要SysConfig | ✅ 需要 | ❌ 不需要 |
| 需要编译器 | ✅ 需要 | ❌ 不需要 |
| 开发灵活性 | ⭐⭐⭐⭐⭐ | ⭐ |

---

## 第四章：SysConfig与SDK的关系

### 4.1 SysConfig是独立工具

**关键理解**：
```
SysConfig ≠ SDK的一部分
SysConfig = 跨多个SDK的通用工具
```

**支持的TI产品线**：
```
SysConfig支持：
├─ SimpleLink MCU（MSP432, CC13xx, CC26xx）
├─ Sitara MPU（AM335x, AM437x）
├─ mmWave Radar（AWR/IWR/xWRL系列）⭐
├─ C2000 MCU
└─ 其他TI芯片

每个产品线有自己的SDK：
├─ MMWAVE_L_SDK（雷达SDK）
├─ SimpleLink SDK（无线SDK）
├─ Sitara SDK（处理器SDK）
└─ ...

SysConfig是它们共用的配置工具
```

### 4.2 SysConfig与MMWAVE_L_SDK的集成

#### 集成方式1: CCS项目中使用

**流程**：
```
1. 安装MMWAVE_L_SDK
   └─ C:\ti\MMWAVE_L_SDK_06_01_00_01\

2. 安装SysConfig
   └─ C:\ti\sysconfig_1.20.0\

3. 在CCS中导入SDK示例
   └─ examples/mmw_demo/xwrL684x-evm/

4. 项目中包含example.syscfg
   └─ 双击文件自动打开SysConfig GUI

5. 修改配置并保存
   └─ 自动重新生成C代码

6. 编译项目
   └─ 新配置编译到固件
```

**CCS自动化集成**：
```
CCS编译流程：
1. 预构建步骤
   └─ 调用SysConfig生成代码

2. 编译步骤
   └─ 编译SysConfig生成的代码 + 应用代码

3. 链接步骤
   └─ 生成.out文件

4. 后处理
   └─ 转换为.appimage
```

#### 集成方式2: 命令行使用

**适用场景**：自动化构建、脚本化开发

**命令示例**：
```powershell
# 生成配置代码
C:\ti\sysconfig_1.20.0\nw\sysconfig.exe `
    --script example.syscfg `
    --context c674x `
    --output generated/ `
    --compiler ticlang

# 输出
# generated/
#   ├── ti_drivers_config.c
#   ├── ti_drivers_config.h
#   └── Board.h
```

### 4.3 SysConfig设备数据来源

**问题**：SysConfig如何知道AWRL6844有哪些引脚和外设？

**答案**：设备数据库

**数据流向**：
```
TI芯片设计团队
    ↓ 生成
设备数据文件（.json）
    ↓ 打包到
SysConfig安装包
    ↓ 安装到
C:\ti\sysconfig_1.20.0\products\68xx\xwrl684x\
    ↓ 读取
SysConfig GUI
    ↓ 显示给
开发者
```

**AWRL6844数据文件位置**：
```
C:\ti\sysconfig_1.20.0\products\
└── 68xx\
    └── xwrl684x\
        ├── devices\
        │   └── AWRL6844.json        ← 芯片定义
        ├── pinmux\
        │   └── xwrl684x_pinmux.json ← 引脚复用
        └── peripherals\
            ├── uart.json            ← UART外设定义
            ├── gpio.json            ← GPIO外设定义
            └── ...
```

**AWRL6844.json示例片段**：
```json
{
  "name": "AWRL6844",
  "displayName": "AWRL6844 (xWRL684x)",
  "device": {
    "cpu": ["C674x", "R5F"],
    "memory": {
      "flash": "2MB",
      "sram": "256KB"
    },
    "package": "FCBGA_207"
  },
  "pins": [
    {
      "ball": "A1",
      "name": "GPIO_0",
      "type": "GPIO",
      "mux": [
        {"function": "GPIO", "mode": 0},
        {"function": "UART0_TX", "mode": 1},
        {"function": "SPI0_CLK", "mode": 2}
      ]
    }
    // ... 207个引脚
  ],
  "peripherals": {
    "UART": {
      "instances": ["UART0", "UART1"],
      "features": ["DMA", "FIFO", "IrDA"]
    },
    "SPI": {
      "instances": ["SPI0", "SPI1", "SPI2"],
      "maxSpeed": "50MHz"
    }
    // ... 其他外设
  }
}
```

### 4.4 版本兼容性

**问题**：SysConfig版本需要匹配SDK版本吗？

**答案**：建议匹配，但不强制

**兼容性表**：
| MMWAVE_L_SDK版本 | 推荐SysConfig版本 | 兼容性 |
|-----------------|-----------------|-------|
| 06.01.00.01 | 1.20.0 | ✅ 完全兼容 |
| 06.01.00.01 | 1.18.0 | ⚠️ 部分功能可能缺失 |
| 06.01.00.01 | 1.22.0 | ✅ 向后兼容 |
| 05.03.00.07 | 1.20.0 | ⚠️ 可能不支持新功能 |

**版本检查**：
```powershell
# 查看SysConfig版本
C:\ti\sysconfig_1.20.0\nw\sysconfig.exe --version

# 输出
# SysConfig version 1.20.0 build 3587
```

**SDK推荐版本**：
```
MMWAVE_L_SDK安装时会推荐SysConfig版本
通常在SDK文档中说明：
"This SDK requires SysConfig 1.18.0 or higher"
```

---

## 第五章：实际应用案例

### 案例1: 添加自定义UART端口

**场景**：需要添加第二个UART用于数据输出

#### Step 1: 打开SysConfig

```powershell
# 方式1: 在CCS中打开
# 项目 → 双击 example.syscfg

# 方式2: 独立启动
C:\ti\sysconfig_1.20.0\nw\sysconfig.exe example.syscfg
```

#### Step 2: 添加UART模块

```
1. 左侧面板 → 搜索"UART"
2. 点击"Add" → 添加UART实例
3. 配置参数：
   Name: CONFIG_UART_DATA
   Baud Rate: 921600
   Data Bits: 8
   Stop Bits: 1
   Parity: None
```

#### Step 3: 分配引脚

```
4. 引脚配置区域：
   TX Pin: 选择 GPIO_30
   RX Pin: 选择 GPIO_31

5. SysConfig自动检查：
   ✅ GPIO_30未被占用
   ✅ GPIO_31未被占用
   ✅ UART1可用
```

#### Step 4: 保存并生成代码

```
6. 保存example.syscfg
   → SysConfig自动重新生成：
     ti_drivers_config.c
     ti_drivers_config.h
     Board.h
```

**生成的代码**（ti_drivers_config.h）：
```c
// 自动生成的宏定义
#define CONFIG_UART_DEBUG  0  // 原有的UART0
#define CONFIG_UART_DATA   1  // 新添加的UART1

// 函数声明
extern void Board_initUART(void);
```

#### Step 5: 在应用代码中使用

```c
// main.c
#include "ti_drivers_config.h"

void main(void)
{
    // 初始化UART（自动调用SysConfig生成的代码）
    Board_initUART();
    
    // 打开新添加的UART
    UART_Handle uartData;
    UART_Params uartParams;
    
    UART_Params_init(&uartParams);
    uartParams.baudRate = 921600;
    
    uartData = UART_open(CONFIG_UART_DATA, &uartParams);
    
    // 使用UART发送数据
    char buffer[] = "Hello from UART1!\n";
    UART_write(uartData, buffer, sizeof(buffer));
}
```

#### Step 6: 编译和测试

```
7. 在CCS中编译项目
   → 新的UART配置编译到固件

8. 烧录到AWRL6844
   → 新的UART1端口可用

9. 测试
   → 连接GPIO_30/31到USB转串口
   → 波特率设置为921600
   → 接收到"Hello from UART1!"
```

**时间统计**：
- 手动编写配置代码：2小时
- 使用SysConfig：10分钟
- **节省时间**：92% ⬇️

---

### 案例2: 配置GPIO控制LED

**场景**：添加3个GPIO控制RGB LED

#### SysConfig配置

```javascript
// example.syscfg

// LED红色
const gpio_led_r = GPIO.addInstance();
gpio_led_r.$name = "CONFIG_GPIO_LED_RED";
gpio_led_r.mode = "Output";
gpio_led_r.initialOutputState = "Low";
gpio_led_r.gpioPin.$assign = "GPIO_0";

// LED绿色
const gpio_led_g = GPIO.addInstance();
gpio_led_g.$name = "CONFIG_GPIO_LED_GREEN";
gpio_led_g.mode = "Output";
gpio_led_g.initialOutputState = "Low";
gpio_led_g.gpioPin.$assign = "GPIO_1";

// LED蓝色
const gpio_led_b = GPIO.addInstance();
gpio_led_b.$name = "CONFIG_GPIO_LED_BLUE";
gpio_led_b.mode = "Output";
gpio_led_b.initialOutputState = "Low";
gpio_led_b.gpioPin.$assign = "GPIO_2";
```

#### 应用代码

```c
// led_control.c
#include "ti_drivers_config.h"
#include <ti/drivers/GPIO.h>

void LED_init(void)
{
    // 初始化GPIO（调用SysConfig生成的初始化函数）
    GPIO_init();
}

void LED_setColor(uint8_t red, uint8_t green, uint8_t blue)
{
    // 控制红色LED
    GPIO_write(CONFIG_GPIO_LED_RED, red ? 1 : 0);
    
    // 控制绿色LED
    GPIO_write(CONFIG_GPIO_LED_GREEN, green ? 1 : 0);
    
    // 控制蓝色LED
    GPIO_write(CONFIG_GPIO_LED_BLUE, blue ? 1 : 0);
}

// 使用示例
void main(void)
{
    LED_init();
    
    // 红色
    LED_setColor(1, 0, 0);
    delay_ms(1000);
    
    // 绿色
    LED_setColor(0, 1, 0);
    delay_ms(1000);
    
    // 蓝色
    LED_setColor(0, 0, 1);
    delay_ms(1000);
    
    // 紫色（红+蓝）
    LED_setColor(1, 0, 1);
}
```

---

### 案例3: 修改时钟频率

**场景**：优化功耗，降低系统时钟频率

#### SysConfig配置

```
1. 在SysConfig GUI中：
   左侧面板 → Power Management

2. 修改时钟配置：
   CPU Clock: 200MHz → 100MHz
   Peripheral Clock: 100MHz → 50MHz

3. SysConfig自动：
   ✅ 验证时钟树配置
   ✅ 计算PLL参数
   ✅ 更新所有外设时钟
   ✅ 检查UART波特率是否仍然有效
```

**生成的代码**（自动更新）：
```c
// ti_drivers_config.c

// 时钟配置（自动生成）
#define CPU_CLOCK_HZ     100000000  // 100MHz
#define PERIPH_CLOCK_HZ  50000000   // 50MHz

void Board_initClock(void)
{
    // PLL配置（自动计算）
    PLL_setMultiplier(25);   // 4MHz * 25 = 100MHz
    PLL_setDivider(1);
    
    // 外设时钟分频
    PERIPH_setClockDivider(2);  // 100MHz / 2 = 50MHz
}
```

**影响**：
- ✅ 功耗降低约50%
- ⚠️ 处理性能降低50%
- ✅ UART/SPI等外设仍然正常工作（自动调整分频）

---

## 📝 总结

### SysConfig核心要点

1. **SysConfig = 图形化硬件配置工具**
   - 简化外设配置
   - 自动生成初始化代码
   - 实时冲突检查

2. **独立工具，跨多个SDK**
   - 不是SDK的一部分
   - 支持多个TI产品线
   - 版本独立更新

3. **与固件的关系**
   - `.syscfg`配置文件（源码项目）
   - 生成C代码编译到固件
   - 预编译固件不包含`.syscfg`

4. **与SDK的关系**
   - CCS集成（自动调用）
   - 命令行支持（脚本化）
   - 设备数据由TI提供

5. **节省开发时间**
   - 硬件配置：节省80-90%时间
   - 避免手写寄存器代码
   - 自动检测冲突

### 安装路径总结

```
C:\ti\sysconfig_1.20.0\        ← SysConfig工具
    ├── nw\sysconfig.exe       ← 可执行文件
    └── products\68xx\         ← AWRL6844设备数据

C:\ti\MMWAVE_L_SDK\            ← 固件SDK
    └── examples\
        └── mmw_demo\
            └── example.syscfg ← 配置文件（源码项目）

关系：
SDK提供示例项目（包含.syscfg）
     ↓
SysConfig工具读取.syscfg并生成C代码
     ↓
C代码编译到固件
```

### 适用场景

| 场景 | 需要SysConfig | 原因 |
|-----|--------------|------|
| 从零开发固件 | ✅ 必需 | 配置硬件外设 |
| 修改SDK示例 | ✅ 必需 | 调整GPIO/UART等配置 |
| 使用预编译固件 | ❌ 不需要 | 硬件配置已固定 |
| 纯应用开发（不修改硬件） | ❌ 不需要 | 只需关注应用逻辑 |

### 学习建议

**新手路线**：
```
Day 1: 了解SysConfig基本概念
Day 2: 安装SysConfig并打开示例
Day 3: 尝试修改GPIO配置
Day 4: 添加UART外设
Day 5: 理解生成的代码
```

**进阶路线**：
```
Week 1: 配置所有常用外设（UART/SPI/I2C）
Week 2: 理解时钟树和电源管理
Week 3: 学习引脚复用冲突解决
Week 4: 集成到自己的固件项目
```

---

## 🔗 相关资源

### 官方文档
- **SysConfig用户指南**: `C:\ti\sysconfig_1.20.0\docs\`
- **TI SysConfig主页**: https://www.ti.com/tool/SYSCONFIG
- **视频教程**: TI Training - SysConfig Introduction

### 本项目文档
- [Part1 - SDK基础概念](Part1-SDK基础概念与三目录详解.md)
- [Part2 - 固件校验方法](Part2-固件校验方法完整指南.md)
- [Part3 - SDK与固件关系](Part3-SDK与固件关系及工作流程.md)
- [Part4 - 实践案例FAQ](Part4-实践案例与常见问题.md)

---

**最后更新**：2025-12-25  
**文档作者**：项目开发团队  
**适用版本**：SysConfig 1.20.0 + MMWAVE_L_SDK 06.01.00.01
