# 📁 源代码目录

## 🏗️ 三层架构

```
src/
├── common/     # Layer 1: 共享接口层 (MSS-DSS共用)
├── mss/        # Layer 2: MSS应用层 (R5F + FreeRTOS)
├── dss/        # Layer 3: DSS算法层 (C66x DSP)
└── system/     # Layer 0: 系统配置层 (链接脚本、配置)
```

## 📂 目录说明

### common/ - 共享接口层
- `shared_memory.h` - L3 RAM内存映射定义
- `data_path.h` - DPC配置/结果结构
- `health_detect_types.h` - 健康检测类型定义（🆕新增）
- `mmwave_output.h` - TLV输出格式

### mss/ - MSS应用层
- `health_detect_main.c/h` - 主控程序（FreeRTOS）
- `cli.c/h` - CLI命令处理
- `dpc_control.c/h` - DPC协调（IPC）
- `presence_detect.c/h` - 存在检测（🆕新增）
- `tlv_output.c/h` - TLV数据输出
- `radar_control.c/h` - mmWave API封装

### dss/ - DSS算法层
- `dss_main.c/h` - DSP主程序
- `feature_extract.c/h` - 特征提取（🆕新增）
- `dsp_utils.c/h` - DSP工具函数

### system/ - 系统配置层
- `linker_mss.cmd` - MSS链接脚本
- `linker_dss.cmd` - DSS链接脚本
- `system_config.h` - 系统配置参数

## ⚙️ RTOS说明

### MSS - FreeRTOS

```c
// ✅ 正确使用FreeRTOS API
#include "FreeRTOS.h"
#include "task.h"

gTask = xTaskCreateStatic(taskFunc, "name", stackSize, ...);
vTaskStartScheduler();
```

### DSS - 裸机/DPL

DSS不使用FreeRTOS，运行裸机或使用SDK DPL层。

## 🔗 编译

- **MSS**: TI CLANG 4.0.4.LTS (ARM Cortex-R5F)
- **DSS**: TI C6000 8.5.0.LTS (C66x DSP)

---

> 🔴 参考 mmw_demo 源码学习正确的API用法！
