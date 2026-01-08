# 📁 MSS Layer - 主处理器应用层

## 🎯 层职责

MSS (Main Subsystem) 层运行在 ARM Cortex-R5F 核心上，负责：

- 🔧 **系统初始化** - FreeRTOS任务、外设驱动
- 📡 **CLI命令处理** - UART命令解析与执行
- 🔄 **DPC协调** - MSS-DSS多核通信
- 📊 **TLV输出** - 格式化数据发送到上位机
- 🎯 **存在检测** - 点云分析算法（🆕新增功能）

## 📂 文件列表

| 文件 | 描述 |
|------|------|
| `health_detect_main.h` | 主控程序头文件，MCB结构定义 |
| `health_detect_main.c` | 主控程序实现，FreeRTOS任务 |
| `cli.h` | CLI命令接口头文件 |
| `cli.c` | CLI命令实现 |
| `dpc_control.h` | DPC控制头文件 |
| `dpc_control.c` | DPC协调实现，IPC通信 |
| `presence_detect.h` | 存在检测模块头文件 |
| `presence_detect.c` | 存在检测算法实现（🆕新增） |
| `tlv_output.h` | TLV输出模块头文件 |
| `tlv_output.c` | TLV数据包构建与发送 |
| `radar_control.h` | 雷达控制头文件 |
| `radar_control.c` | mmWave API封装 |

## ⚙️ RTOS说明

**关键：使用 FreeRTOS API，不是 TI-RTOS/BIOS！**

```c
// ✅ 正确的FreeRTOS API
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

TaskHandle_t task = xTaskCreateStatic(taskFunc, "name", ...);
vTaskStartScheduler();
SemaphoreHandle_t sem = xSemaphoreCreateBinaryStatic(&semObj);
```

```c
// ❌ 错误的BIOS API（禁止使用）
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

Task_create(...);  // 错误！
BIOS_start();      // 错误！
```

## 🔗 依赖关系

```
MSS Layer
    ├── common/           # 共享头文件
    ├── FreeRTOS          # 操作系统
    ├── SDK DPL           # 驱动移植层
    └── mmWave API        # 雷达控制API
```

## 📝 编译器

- **编译器**: TI CLANG 4.0.4.LTS
- **目标**: ARM Cortex-R5F
- **选项**: `-mcpu=cortex-r5 -mfloat-abi=hard -mfpu=vfpv3-d16 -mthumb`

---

> 🔴 **注意**: 所有代码必须参考 mmw_demo 源码的API用法，禁止凭经验猜测！
