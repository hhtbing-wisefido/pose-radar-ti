# AWRL6844 SysConfig 配置文件详解

> **创建日期**: 2025-12-15  
> **SDK版本**: MMWAVE_L_SDK_06_01_00_01  
> **设备型号**: AWRL6844 (xWRL684x-evm)  
> **工具版本**: SysConfig 1.20.0

---

## 📋 目录

- [概述](#概述)
- [SysConfig工具简介](#sysconfig工具简介)
- [配置文件位置](#配置文件位置)
- [HelloWorld项目配置](#helloworld项目配置)
- [常用示例配置](#常用示例配置)
- [配置文件结构](#配置文件结构)
- [使用指南](#使用指南)
- [配置项说明](#配置项说明)

---

## 概述

### 什么是 SysConfig？

**SysConfig** 是TI提供的图形化配置工具，用于：
- ✅ 配置硬件外设（GPIO、UART、SPI、I2C等）
- ✅ 配置引脚复用（Pin Mux）
- ✅ 配置时钟树
- ✅ 配置中断和DMA
- ✅ **自动生成初始化代码**（.c/.h文件）

### `.syscfg` 文件的作用

- 📄 存储硬件配置信息
- 🔄 在编译时自动生成初始化代码
- 🎯 确保软件与硬件配置一致
- 📝 可视化编辑硬件资源

---

## SysConfig工具简介

### 工具位置

```
C:\ti\sysconfig_1.20.0\
├── sysconfig_gui.bat      # 图形界面启动器
├── sysconfig_cli.bat      # 命令行版本
└── dist\                  # 工具主体
```

### 启动方式

**方法1: 图形界面**
```powershell
C:\ti\sysconfig_1.20.0\sysconfig_gui.bat
```

**方法2: 从CCS启动**
- 在CCS项目中双击 `.syscfg` 文件
- 自动在SysConfig编辑器中打开

**方法3: 命令行**
```powershell
C:\ti\sysconfig_1.20.0\sysconfig_cli.bat --product "C:\ti\MMWAVE_L_SDK_06_01_00_01" --device AWR68XX --output generated\ example.syscfg
```

### 配置步骤

1. **选择Software Product**: `MMWAVE_L_SDK_06_01_00_01`
2. **选择Device**: `AWR68XX`
3. **配置外设**: GPIO、UART、SPI等
4. **生成代码**: 保存后自动生成初始化代码

---

## 配置文件位置

### HelloWorld 项目配置文件

| 核心 | 操作系统 | 配置文件路径 | 用途 |
|------|---------|-------------|------|
| **R5F主核** | FreeRTOS | `C:\ti\MMWAVE_L_SDK_06_01_00_01\examples\hello_world\xwrL684x-evm\r5fss0-0_freertos\example.syscfg` | ⭐ 主要开发 |
| R5F主核 | No-RTOS | `C:\ti\MMWAVE_L_SDK_06_01_00_01\examples\hello_world\xwrL684x-evm\r5fss0-0_nortos\example.syscfg` | 裸机开发 |
| C66x DSP | FreeRTOS | `C:\ti\MMWAVE_L_SDK_06_01_00_01\examples\hello_world\xwrL684x-evm\c66ss0_freertos\example.syscfg` | DSP核心 |
| C66x DSP | No-RTOS | `C:\ti\MMWAVE_L_SDK_06_01_00_01\examples\hello_world\xwrL684x-evm\c66ss0_nortos\example.syscfg` | DSP裸机 |

### SBL Bootloader 配置文件

```
C:\ti\MMWAVE_L_SDK_06_01_00_01\examples\drivers\boot\sbl\xwrL684x-evm\r5fss0-0_nortos\example.syscfg
```

**说明**: 
- SBL运行在R5F核心
- 使用No-RTOS（裸机模式）
- 负责从Flash加载应用程序

---

## HelloWorld项目配置

### 推荐配置文件 ⭐

**文件路径**:
```
C:\ti\MMWAVE_L_SDK_06_01_00_01\examples\hello_world\xwrL684x-evm\r5fss0-0_freertos\example.syscfg
```

**推荐理由**:
1. ✅ R5F是主控核心（Cortex-R5F @ 200MHz）
2. ✅ FreeRTOS是常用RTOS
3. ✅ 包含完整外设配置
4. ✅ 适合大多数应用开发

### 配置内容概览

该配置文件包含：

| 配置项 | 说明 | 默认配置 |
|-------|------|---------|
| **时钟** | 系统时钟配置 | PLL: 200MHz |
| **UART** | 调试串口 | UART0, 115200bps |
| **GPIO** | LED控制引脚 | GPIO配置 |
| **中断** | 中断优先级 | 系统中断配置 |
| **内存** | RAM/ROM分配 | TCM + MSRAM |
| **DMA** | DMA通道 | 预留通道 |

### 生成的代码文件

保存 `.syscfg` 后自动生成：

```
r5fss0-0_freertos/
├── ti_drivers_config.c         # 驱动配置实现
├── ti_drivers_config.h         # 驱动配置头文件
├── ti_drivers_open_close.c     # 初始化函数
├── ti_drivers_open_close.h     # 初始化头文件
└── ti_board_config.c           # 板级配置
```

**在应用代码中使用**:
```c
#include "ti_drivers_config.h"
#include "ti_drivers_open_close.h"

int main(void)
{
    /* SysConfig生成的初始化函数 */
    System_init();
    Board_init();
    Drivers_open();
    
    /* 你的应用代码 */
    while(1) {
        // ...
    }
    
    Drivers_close();
    Board_deinit();
}
```

---

## 常用示例配置

### 1. 驱动示例配置

#### GPIO 示例
```
C:\ti\MMWAVE_L_SDK_06_01_00_01\examples\drivers\gpio\gpio_input_interrupt\xwrL684x-evm\r5fss0-0_freertos\example.syscfg
```
- GPIO输入/输出配置
- 中断配置
- 引脚复用

#### UART 示例
```
C:\ti\MMWAVE_L_SDK_06_01_00_01\examples\drivers\uart\uart_echo\xwrL684x-evm\r5fss0-0_freertos\example.syscfg
```
- 串口波特率配置
- DMA模式配置
- 中断模式配置

#### SPI 示例
```
C:\ti\MMWAVE_L_SDK_06_01_00_01\examples\drivers\spi\spi_loopback\xwrL684x-evm\r5fss0-0_freertos\example.syscfg
```
- SPI主从模式
- 时钟频率配置
- 片选信号配置

#### I2C 示例
```
C:\ti\MMWAVE_L_SDK_06_01_00_01\examples\drivers\i2c\i2c_read\xwrL684x-evm\r5fss0-0_freertos\example.syscfg
```
- I2C地址配置
- 速度配置（标准/快速模式）
- 超时配置

### 2. 数据处理示例配置

#### Range Processing (距离处理)
```
C:\ti\MMWAVE_L_SDK_06_01_00_01\examples\datapath\rangeproc\xwrL684x-evm\r5fss0-0_freertos\example.syscfg
```
- EDMA配置
- HWA配置
- FFT参数

#### Doppler Processing (多普勒处理)
```
C:\ti\MMWAVE_L_SDK_06_01_00_01\examples\datapath\dopplerproc\xwrL684x-evm\r5fss0-0_freertos\example.syscfg
```
- DSP协处理器配置
- 信号处理链配置

#### CFAR Detection
```
C:\ti\MMWAVE_L_SDK_06_01_00_01\examples\datapath\cfarproc\xwrL684x-evm\r5fss0-0_freertos\example.syscfg
```
- 检测算法参数
- 阈值配置

### 3. 控制模块示例配置

#### mmWave 控制
```
C:\ti\MMWAVE_L_SDK_06_01_00_01\examples\control\mmwave\xwrL684x-evm\r5fss0-0_freertos\example.syscfg
```
- 雷达前端配置
- Chirp参数
- 帧配置

#### LUT (Look-Up Table)
```
C:\ti\MMWAVE_L_SDK_06_01_00_01\examples\control\lut\xwrL684x-evm\r5fss0-0_freertos\example.syscfg
```
- 校准数据配置
- 查找表管理

### 4. Boot 相关配置

#### SBL (Secondary Bootloader)
```
C:\ti\MMWAVE_L_SDK_06_01_00_01\examples\drivers\boot\sbl\xwrL684x-evm\r5fss0-0_nortos\example.syscfg
```
- Flash接口配置（QSPI）
- 启动模式配置
- 多核启动序列

#### SBL Lite
```
C:\ti\MMWAVE_L_SDK_06_01_00_01\examples\drivers\boot\sbl_lite\xwrL684x-evm\r5fss0-0_nortos\example.syscfg
```
- 精简版Bootloader
- 快速启动配置

---

## 配置文件结构

### `.syscfg` 文件格式

`.syscfg` 文件是JavaScript格式的配置文件：

```javascript
/**
 * These arguments were used when this file was generated. They will be automatically applied on subsequent loads
 * via the GUI or CLI. Run CLI with '--help' for additional information on how to override these arguments.
 */
// @cliArgs --device "AWR68XX" --package "ANC" --part "Default"
// @cliArgs --board "/ti/boards/xwrL684x_evm" --rtos "freertos"
// @cliArgs --product "C:/ti/MMWAVE_L_SDK_06_01_00_01@6.1.0"

/**
 * Import the modules used in this configuration.
 */
const edma       = scripting.addModule("/drivers/edma/edma", {}, false);
const gpio       = scripting.addModule("/drivers/gpio/gpio", {}, false);
const uart       = scripting.addModule("/drivers/uart/uart", {}, false);
const clock      = scripting.addModule("/kernel/freertos/clock");
const debug_log  = scripting.addModule("/kernel/freertos/debug_log");

/**
 * UART Instance Configuration
 */
const uart1 = uart.addInstance();
uart1.$name = "CONFIG_UART0";
uart1.UART.$assign = "UART0";
uart1.baudRate = 115200;
uart1.dataLength = "8 Bits";
uart1.stopBits = "1 Stop Bit";
uart1.parity = "None";

/**
 * GPIO Instance Configuration
 */
const gpio1 = gpio.addInstance();
gpio1.$name = "CONFIG_GPIO_LED";
gpio1.pinDir = "OUTPUT";
gpio1.GPIO.$assign = "GPIO_14";
```

### 关键配置项说明

#### 1. CLI参数
```javascript
// @cliArgs --device "AWR68XX"           // 设备型号
// @cliArgs --package "ANC"              // 封装类型
// @cliArgs --board "/ti/boards/xwrL684x_evm"  // 开发板
// @cliArgs --rtos "freertos"            // 操作系统
// @cliArgs --product "C:/ti/MMWAVE_L_SDK_06_01_00_01@6.1.0"  // SDK路径
```

#### 2. 模块导入
```javascript
const uart = scripting.addModule("/drivers/uart/uart");
const gpio = scripting.addModule("/drivers/gpio/gpio");
const spi  = scripting.addModule("/drivers/spi/spi");
```

#### 3. 实例配置
```javascript
const uart1 = uart.addInstance();
uart1.$name = "CONFIG_UART0";
uart1.UART.$assign = "UART0";
uart1.baudRate = 115200;
```

---

## 使用指南

### 创建新配置

#### 方法1: 从空白开始
1. 启动 SysConfig GUI
2. 点击 "Start a new Design"
3. 选择 Software Product: `MMWAVE_L_SDK_06_01_00_01`
4. 选择 Device: `AWR68XX`
5. 点击 START

#### 方法2: 从示例复制
```powershell
# 复制HelloWorld配置作为模板
Copy-Item "C:\ti\MMWAVE_L_SDK_06_01_00_01\examples\hello_world\xwrL684x-evm\r5fss0-0_freertos\example.syscfg" `
          "D:\MyProject\my_config.syscfg"
```

#### 方法3: 在CCS中创建
1. 右键项目 → New → Other
2. 选择 SysConfig File
3. 自动创建并打开编辑器

### 编辑配置

#### 添加外设
1. 左侧面板选择外设类型（如UART）
2. 点击 "Add" 添加实例
3. 配置实例参数
4. 配置引脚分配

#### 配置时钟
1. 打开 Clock Tree 视图
2. 调整PLL频率
3. 配置外设时钟源
4. 验证时钟约束

#### 引脚复用
1. 打开 Pin Mux 视图
2. 拖拽功能到引脚
3. 解决冲突
4. 锁定关键引脚

### 生成代码

#### 自动生成（推荐）
- 保存 `.syscfg` 文件时自动生成
- 在CCS编译时自动更新

#### 手动生成
```powershell
sysconfig_cli.bat --product "C:\ti\MMWAVE_L_SDK_06_01_00_01" --device AWR68XX --output generated\ example.syscfg
```

---

## 配置项说明

### 时钟配置

#### 系统时钟
```
PLL输入: 40MHz (晶振)
PLL倍频: 50倍
PLL输出: 2000MHz
R5F时钟: 200MHz (除以10)
C66x时钟: 450MHz
```

#### 外设时钟
- UART: 96MHz
- SPI: 96MHz
- I2C: 96MHz
- ADC: 200MHz

### GPIO配置

#### 可用引脚
- GPIO_0 ~ GPIO_15: 通用GPIO
- 部分引脚复用为LED、按键等

#### 配置选项
- 方向: INPUT / OUTPUT
- 上拉/下拉: PULL_UP / PULL_DOWN / NONE
- 中断: RISING / FALLING / BOTH
- 驱动强度: 2mA / 4mA / 8mA

### UART配置

#### 参数
- 波特率: 9600 ~ 3000000 bps
- 数据位: 7位 / 8位
- 停止位: 1位 / 2位
- 校验: 无 / 奇校验 / 偶校验
- 流控: 无 / RTS/CTS

#### DMA模式
- 发送DMA: 提高发送效率
- 接收DMA: 减少CPU中断

### SPI配置

#### 模式
- Master模式（主设备）
- Slave模式（从设备）

#### 参数
- 时钟频率: 100kHz ~ 48MHz
- 数据位宽: 8位 / 16位 / 32位
- 时钟相位: CPHA = 0/1
- 时钟极性: CPOL = 0/1
- 片选: 手动 / 自动

### I2C配置

#### 速度模式
- 标准模式: 100kHz
- 快速模式: 400kHz
- 高速模式: 3.4MHz

#### 地址
- 7位地址模式
- 10位地址模式（可选）

### 中断配置

#### 优先级
- 0 (最高) ~ 15 (最低)
- 建议关键中断使用0-3
- 普通中断使用4-7

#### 中断向量
- 每个外设可配置中断回调函数
- SysConfig自动生成中断注册代码

---

## 常见问题

### Q1: 如何查看所有可用的配置选项？

**A**: 在SysConfig GUI中：
1. 选择外设实例
2. 查看右侧属性面板
3. 悬停在参数上查看帮助信息

### Q2: 生成的代码在哪里？

**A**: 
- CCS项目：`<project>/Debug/syscfg/` 或 `<project>/Release/syscfg/`
- Make项目：`generated/` 目录

### Q3: 修改配置后需要重新编译吗？

**A**: 
- ✅ 是的，需要清理并重新编译项目
- SysConfig生成的代码是编译的一部分

### Q4: 引脚冲突如何解决？

**A**:
1. SysConfig会自动检测冲突
2. 红色标记表示冲突
3. 修改引脚分配或禁用冲突功能

### Q5: 能否手动修改生成的代码？

**A**:
- ❌ 不建议，每次重新生成会覆盖
- ✅ 应该修改 `.syscfg` 文件，重新生成

### Q6: 如何备份配置？

**A**:
```powershell
# 备份syscfg文件
Copy-Item "example.syscfg" "example.syscfg.backup"

# 或提交到Git
git add example.syscfg
git commit -m "保存硬件配置"
```

---

## 最佳实践

### 1. 版本控制

✅ **建议**:
- 将 `.syscfg` 文件提交到版本控制
- 不要提交生成的代码文件

❌ **不建议**:
- 手动修改生成的代码
- 不同分支使用相同配置名称

### 2. 命名规范

```
✅ 好的命名:
CONFIG_UART_DEBUG
CONFIG_GPIO_LED_RED
CONFIG_SPI_FLASH

❌ 不好的命名:
uart1
gpio
instance0
```

### 3. 注释配置

在 `.syscfg` 文件中添加注释：
```javascript
/**
 * Debug UART for console output
 * Connected to XDS110 virtual COM port
 */
const uart1 = uart.addInstance();
uart1.$name = "CONFIG_UART_DEBUG";
```

### 4. 配置分组

按功能组织配置：
```
- 系统配置（时钟、内存）
- 通信接口（UART、SPI、I2C）
- 数据处理（DMA、EDMA）
- 雷达前端（mmWave控制）
```

---

## 参考资料

### 官方文档

1. **SysConfig用户指南**:
   - `C:\ti\sysconfig_1.20.0\docs\SysConfig_Users_Guide.pdf`

2. **SDK文档**:
   - `C:\ti\MMWAVE_L_SDK_06_01_00_01\docs\api_guide_xwrL684x\index.html`

3. **设备技术参考手册**:
   - xWRL68xx Technical Reference Manual (SWRU621)

### 在线资源

- [TI Developer Zone - SysConfig](https://dev.ti.com/sysconfig)
- [TI E2E Forum - mmWave Sensors](https://e2e.ti.com/support/sensors-group/sensors/f/sensors-forum)
- [MMWAVE_L_SDK Documentation](https://dev.ti.com/tirex/explore/node?node=A__AC.I2ocVgQ2eJMaJwg5lKQ__MMWAVE_L_SDK__AyP5vxv__6.1.0)

---

## 总结

### 关键要点

1. ✅ **SysConfig是硬件配置工具**，自动生成初始化代码
2. ✅ **`.syscfg`文件**存储配置，是源代码的一部分
3. ✅ **推荐配置路径**: `r5fss0-0_freertos/example.syscfg`
4. ✅ **生成代码位置**: `syscfg/` 或 `generated/` 目录
5. ✅ **修改配置后**需要重新编译项目

### 下一步

- 📖 阅读SDK API文档
- 🔧 尝试修改示例配置
- 💻 在CCS中创建自己的项目
- 🚀 集成到自定义应用中

---

**文档维护**: Benson@Wisefido  
**更新日期**: 2025-12-15  
**SDK版本**: MMWAVE_L_SDK_06_01_00_01  
**SysConfig版本**: 1.20.0
