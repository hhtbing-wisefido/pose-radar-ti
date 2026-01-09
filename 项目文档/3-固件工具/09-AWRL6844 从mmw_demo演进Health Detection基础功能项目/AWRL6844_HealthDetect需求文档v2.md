# 📋 AWRL6844 Health Detection 项目需求文档 v2.4

**项目路径**: `D:\7.project\TI_Radar_Project\project-code\AWRL6844_HealthDetect`
**创建日期**: 2026-01-08
**更新日期**: 2026-01-09
**版本**: v2.4（添加CCS自动依赖编译机制）

---

## 🔴 重要：参考项目路径选择

### ⚠️ 必须参考本地项目，不要参考radar_toolbox

| 来源                  | 路径                                                                             | 是否推荐       |
| --------------------- | -------------------------------------------------------------------------------- | -------------- |
| **✅ 本地项目** | `D:\7.project\TI_Radar_Project\project-code\AWRL6844_InCabin_Demos\`           | **推荐** |
| ❌ radar_toolbox      | `C:\ti\radar_toolbox_3_30_00_06\source\ti\examples\...\AWRL6844_InCabin_Demos` | 不推荐         |

### 原因说明

**从radar_toolbox导入会出现版本警告**：

```
Product SysConfig v1.23.0 is not currently installed. A compatible version 1.26.0 will be used.
Product mmWave low-power SDK xWRL68xx v6.0.5.01 is not currently installed. A compatible version 6.1.0.01 will be used.
```

**从本地项目导入无任何错误**：

- `D:\7.project\TI_Radar_Project\project-code\AWRL6844_InCabin_Demos\src\mss\xwrL684x-evm\` → ✅ 无错误
- `D:\7.project\TI_Radar_Project\project-code\AWRL6844_InCabin_Demos\src\dss\xwrL684x-evm\` → ✅ 无错误
- `D:\7.project\TI_Radar_Project\project-code\AWRL6844_InCabin_Demos\src\system\` → ✅ 无错误

### 结论

> 📌 **参考InCabin_Demos时，始终使用本地项目路径**：
>
> ```
> D:\7.project\TI_Radar_Project\project-code\AWRL6844_InCabin_Demos\
> ```
>
> **不要使用**：
>
> ```
> C:\ti\radar_toolbox_3_30_00_06\source\ti\examples\...
> ```

---

## ⚠️ 重要：失败教训与修正（2026-01-08）

### 🔴 v1.0版本失败的具体原因

**失败点**：代码编写时**错误使用了BIOS/TI-RTOS风格的API**

| 错误代码（v1失败）                        | 正确代码（mmw_demo实际使用）                |
| ----------------------------------------- | ------------------------------------------- |
| `#include <ti/sysbios/BIOS.h>`          | `#include "FreeRTOS.h"`                   |
| `#include <ti/sysbios/knl/Task.h>`      | `#include "task.h"`                       |
| `Task_create()`, `Task_Params_init()` | `xTaskCreateStatic()` / `xTaskCreate()` |
| `BIOS_start()`                          | `vTaskStartScheduler()`                   |
| `Semaphore_create()`                    | `xSemaphoreCreateBinaryStatic()`          |

**错误根源**：

- ❌ 没有仔细阅读 mmw_demo 的 `main.c` 和 `mmwave_demo.c`
- ❌ 凭"TI SDK经验"使用了其他SDK（如标准mmWave SDK 3.x）的API风格
- ❌ 不了解 **L-SDK（mmWave Low-Power SDK）本身就是FreeRTOS**

### 🟢 v1.0版本正确的部分（保留）

**以下内容完全正确，继续沿用**：

- ✅ **三层架构设计**：common/mss/dss/system
- ✅ **从零重建的方向**：不复制mmw_demo，参考学习后重写
- ✅ **功能规划**：主控、CLI、DPC协调、TLV输出、特征提取
- ✅ **文件结构规划**：所有模块划分都正确
- ✅ **Milestone规划**：架构重建→编译验证→功能实现→硬件测试

### 🔧 v2.0修正内容

**本版本仅修正**：

1. 添加 **FreeRTOS API规范章节**（强制）
2. 添加 **失败教训说明**（警示）
3. 强调 **必须先读mmw_demo源码再编码**
4. 其他内容保持v1.0不变

---

## 📋 核心需求说明（保持v1.0）

### 1. 项目目标

根据**第3章演进架构**，将TI mmWave SDK的mmw_demo功能**完整重建**为新的三层架构健康检测项目。

**关键要求**：

- ✅ **不是复制粘贴**mmw_demo源代码
- ✅ **不是简单修改**mmw_demo
- ✅ **是从零重建**，参考mmw_demo的功能和API用法
- ✅ **实现同等功能**，但代码结构符合第3章架构
- ✅ **一口气完成**所有模块，不中途询问

### 2. 参考与重建的关系

**mmw_demo_SDK_reference的定位**：

- 📚 **仅作为参考**：学习其功能实现、API调用方式、数据结构
- 📚 **学习对象**：理解DPC工作流程、CLI命令设计、TLV输出格式
- 🔴 **必须学习其RTOS API用法**：FreeRTOS任务创建、信号量、调度器
- ❌ **禁止复制**：不能直接复制其源代码到新项目
- ❌ **禁止照搬**：不能保留其目录结构和文件命名

---

## 🔴 FreeRTOS API规范（新增 - 强制）

### 1. 必须使用的头文件

```c
// ✅ 正确：FreeRTOS头文件
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

// ✅ 正确：SDK DPL抽象层
#include <kernel/dpl/SemaphoreP.h>
#include <kernel/dpl/ClockP.h>
#include <kernel/dpl/DebugP.h>
#include <kernel/dpl/HwiP.h>
#include <kernel/dpl/CacheP.h>
#include <kernel/dpl/AddrTranslateP.h>

// ❌ 禁止：BIOS/TI-RTOS头文件（L-SDK中不存在！）
// #include <ti/sysbios/BIOS.h>
// #include <ti/sysbios/knl/Task.h>
// #include <ti/sysbios/knl/Semaphore.h>
// #include <ti/sysbios/knl/Clock.h>
```

### 2. 任务创建API

```c
// ✅ 正确：FreeRTOS任务创建（静态分配，推荐）
StackType_t gTaskStack[TASK_STACK_SIZE] __attribute__((aligned(32)));
StaticTask_t gTaskObj;
TaskHandle_t gTask;

gTask = xTaskCreateStatic(
    TaskFunction,           // 任务函数
    "TaskName",             // 任务名称
    TASK_STACK_SIZE,        // 栈大小
    NULL,                   // 参数
    TASK_PRIORITY,          // 优先级
    gTaskStack,             // 栈指针
    &gTaskObj               // 任务对象
);

// ✅ 正确：FreeRTOS任务创建（动态分配）
xTaskCreate(TaskFunction, "TaskName", stackSize, NULL, priority, &taskHandle);

// ❌ 禁止：BIOS任务创建
// Task_Params taskParams;
// Task_Params_init(&taskParams);
// Task_create(TaskFunction, &taskParams, NULL);
```

### 3. 调度器启动

```c
// ✅ 正确：FreeRTOS调度器启动
vTaskStartScheduler();

// ❌ 禁止：BIOS启动
// BIOS_start();
```

### 4. 信号量API

```c
// ✅ 正确：FreeRTOS信号量（静态分配）
StaticSemaphore_t gSemObj;
SemaphoreHandle_t gSem;
gSem = xSemaphoreCreateBinaryStatic(&gSemObj);
xSemaphoreGive(gSem);
xSemaphoreTake(gSem, portMAX_DELAY);

// ✅ 正确：SDK DPL抽象层信号量
SemaphoreP_Object semObj;
SemaphoreP_constructBinary(&semObj, 0);
SemaphoreP_pend(&semObj, SystemP_WAIT_FOREVER);
SemaphoreP_post(&semObj);

// ❌ 禁止：BIOS信号量
// Semaphore_create(0, NULL, NULL);
// Semaphore_pend(sem, BIOS_WAIT_FOREVER);
```

### 5. 延时API

```c
// ✅ 正确：SDK DPL延时
ClockP_usleep(1000);      // 微秒延时
ClockP_sleep(1);          // 秒延时

// ✅ 正确：FreeRTOS延时
vTaskDelay(pdMS_TO_TICKS(100));  // 毫秒延时

// ❌ 禁止：BIOS延时
// Task_sleep(1000);
```

### 6. 调试输出

```c
// ✅ 正确：SDK调试输出
DebugP_log("Message: %d\r\n", value);
DebugP_assert(condition);

// ✅ 正确：CLI输出
CLI_write("Message: %d\r\n", value);
```

### 7. 参考mmw_demo中的实际用法

**必须阅读的文件**：

| 文件                                      | 内容         | 必须学习的API                                       |
| ----------------------------------------- | ------------ | --------------------------------------------------- |
| `xwrL684x-evm/r5fss0-0_freertos/main.c` | FreeRTOS入口 | `xTaskCreateStatic`, `vTaskStartScheduler`      |
| `source/mmwave_demo.c` 1-200行          | 任务定义     | 任务栈、任务对象、信号量定义                        |
| `source/mmwave_demo.c` 全局变量         | FreeRTOS对象 | `TaskHandle_t`, `StaticTask_t`, `StackType_t` |

---

## 🏗️ 第3章三层架构要求（保持v1.0）

### 架构层次划分

```
AWRL6844_HealthDetect/
├── src/
│   ├── common/          # Layer 1: 共享接口层
│   ├── mss/             # Layer 2: MSS应用层 (R5F)
│   ├── dss/             # Layer 3: DSS算法层 (C66x)
│   └── system/          # Layer 0: 系统配置层
├── mss_project.projectspec
├── dss_project.projectspec
├── system_project.projectspec
└── README.md
```

### Layer 1: Common (共享接口层)

**职责**：定义MSS与DSS之间的共享数据结构和内存映射

**必须包含**：

- `shared_memory.h` - L3 RAM内存映射（896KB）
- `data_path.h` - DPC结构定义（Config/Result/PointCloud）
- `mmwave_output.h` - TLV输出格式定义
- `health_detect_types.h` - 健康检测特征数据结构

### Layer 2: MSS (应用层 - R5F @ 200MHz)

**职责**：主控制、配置管理、高层决策、数据输出

**必须包含的模块**：

| 模块     | 文件                       | 功能                     | 参考            |
| -------- | -------------------------- | ------------------------ | --------------- |
| 主控程序 | `health_detect_main.c/h` | FreeRTOS任务框架、帧循环 | mmwave_demo.c   |
| DPC协调  | `dpc_control.c/h`        | MSS-DSS通信              | dpc/dpc.c       |
| CLI命令  | `cli.c/h`                | 命令行接口               | mmw_cli.c       |
| 存在检测 | `presence_detect.c/h`    | 🆕 特征分析、状态机      | 新功能          |
| TLV输出  | `tlv_output.c/h`         | 数据包构建、UART发送     | 多个文件        |
| 雷达控制 | `radar_control.c/h`      | mmWave API封装           | mmwave_control/ |

### Layer 3: DSS (算法层 - C66x @ 450MHz)

**职责**：信号处理、特征提取（实现40% DSP利用率）

**必须包含的模块**：

| 模块      | 文件                    | 功能                |
| --------- | ----------------------- | ------------------- |
| DSP主程序 | `dss_main.c/h`        | IPC消息循环         |
| 特征提取  | `feature_extract.c/h` | 🆕 点云特征计算     |
| DSP工具   | `dsp_utils.c/h`       | Cache操作、周期计数 |

### Layer 0: System (系统配置层)

**必须包含**：

- `linker_mss.cmd` - MSS链接脚本
- `linker_dss.cmd` - DSS链接脚本
- `shared_memory.ld` - 共享内存区域定义
- `system.xml` - 多核系统描述文件
- `makefile_system_ccs_bootimage_gen` - 打包脚本
- `config/*.json` - metaimage配置文件

---

## 🔄 功能对照表（保持v1.0）

| mmw_demo功能       | mmw_demo文件    | 新架构文件           | 层级 | 状态    |
| ------------------ | --------------- | -------------------- | ---- | ------- |
| 主控程序           | mmwave_demo.c   | health_detect_main.c | MSS  | ✅ 重写 |
| DPC协调            | dpc/dpc.c       | dpc_control.c        | MSS  | ✅ 重写 |
| CLI命令            | mmw_cli.c       | cli.c                | MSS  | ✅ 重写 |
| TLV输出            | (多个文件)      | tlv_output.c         | MSS  | ✅ 重写 |
| 雷达控制           | mmwave_control/ | radar_control.c      | MSS  | ✅ 封装 |
| DSP处理            | ❌ 无           | dss_main.c           | DSS  | 🆕 新增 |
| **特征提取** | ❌ 无           | feature_extract.c    | DSS  | 🆕 新增 |
| 存在检测           | ❌ 无           | presence_detect.c    | MSS  | 🆕 新增 |

---

## 📝 编码规范要求（更新）

### 文件头注释模板（更新）

```c
/**
 * @file xxx.c
 * @brief 模块功能简述
 * 
 * Reference: mmw_demo参考文件路径
 * Adapted for: 三层架构具体用途
 * 
 * RTOS: FreeRTOS (L-SDK mandatory)
 * Note: L-SDK uses FreeRTOS, NOT TI-RTOS/BIOS
 * 
 * Created: 2026-01-08
 */
```

### 编码前强制检查清单（🔴 新增）

**在编写任何代码前，必须：**

- [ ] 是否阅读了 `mmw_demo/xwrL684x-evm/r5fss0-0_freertos/main.c`？
- [ ] 是否阅读了 `mmw_demo/source/mmwave_demo.c` 的头文件包含和全局变量？
- [ ] 是否确认要使用的API在mmw_demo中有使用？
- [ ] 是否确认没有使用任何BIOS/TI-RTOS API？

**如果任何一项是"否"，停止编码，先去阅读mmw_demo源码！**

### 命名规范（保持v1.0）

- **模块前缀**：`HealthDetect_`, `DPC_`, `TLV_`, `RadarControl_`
- **私有函数**：`static` + 模块前缀
- **全局变量**：`g` + 驼峰命名
- **宏定义**：全大写 + 下划线

---

## 🎯 交付标准（保持v1.0，更新M2）

### Milestone 1: 架构重建

**目标**：按第3章架构从零重建所有代码

**交付物清单**：

- [ ] src/common/*.h (4个文件)
- [ ] src/system/* (7个文件，含config目录)
- [ ] src/mss/*.c/h (12个文件)
- [ ] src/dss/*.c/h (6个文件)
- [ ] mss_project.projectspec
- [ ] dss_project.projectspec
- [ ] system_project.projectspec
- [ ] README.md

### Milestone 2: 编译验证（🔴 更新验收标准）

**目标**：在CCS中编译通过，生成.appimage固件

**验收标准**：

- [ ] 导入CCS项目成功（MSS/DSS/System三个项目）
- [ ] MSS项目编译0错误
- [ ] DSS项目编译0错误
- [ ] System项目编译成功，生成.appimage
- [ ] **🔴 代码中没有任何BIOS/TI-RTOS API调用**
- [ ] **🔴 所有任务使用FreeRTOS API创建**

### Milestone 3: 功能实现

**目标**：完成TODO部分，实现完整功能

### Milestone 4: 硬件测试

**目标**：在AWRL6844 EVM上验证

---

## 🚫 禁止的行为（更新）

### 绝对不允许

1. ❌ **复制粘贴mmw_demo源代码**
2. ❌ **照搬mmw_demo目录结构**
3. ❌ **使用BIOS/TI-RTOS API**（🔴 新增强调）

   - 禁止 `#include <ti/sysbios/*.h>`
   - 禁止 `Task_create()`, `BIOS_start()`
   - 禁止 `Semaphore_create()`, `Semaphore_pend()`
   - L-SDK中这些API**根本不存在**！
4. ❌ **不读源码就编写代码**（🔴 新增）

   - 必须先阅读mmw_demo的main.c和mmwave_demo.c
   - 必须确认API用法正确
   - 凭"经验"猜测 = 失败

---

## 🔧 编译打包说明（保持v1.0）

### 方式1: 分别编译（开发调试）

```
CCS操作：
├─ Build MSS Project → health_detect_mss.out
└─ Build DSS Project → health_detect_dss.out

烧录：
└─ UniFlash分别烧录两个.out文件

优点：✅ 开发快、调试方便
缺点：⚠️ 版本可能不一致、烧录两次
适用：开发调试 ✅（仅作为辅助手段）
```

### ⭐ 方式2: 系统打包（正式方式 - 🔴 必须实现）

```
项目配置：
├─ mss_project.projectspec
├─ dss_project.projectspec
└─ system_project.projectspec ← ✅ 协调上面两个

CCS操作：
└─ Build System Project
    ├─ 自动编译MSS → mss.rig
    ├─ 自动编译DSS → dss.rig
    └─ 打包成 .appimage (单文件包含所有)

烧录：
└─ UniFlash一次烧录 .appimage

优点：✅ 一次烧录、版本一致、完整发布
缺点：⚠️ 打包稍慢（可接受）
适用：所有阶段 ✅（开发+发布）
状态：🔴 必须完成，不是可选的
```

### 🔧 CCS自动依赖编译机制（🆕 v2.4新增）

> 📎 **详细技术分析**: [Part15-CCS System项目自动依赖编译机制](../06-SDK固件研究/Part15-CCS System项目自动依赖编译机制.md)

#### 官方文档说明

**来源**: `C:\ti\MMWAVE_L_SDK_06_01_00_01\docs\api_guide_xwrL684x\BUILD_GUIDE.html`

> "Building the system project... **This automatically builds all referenced core projects**"

#### 工作原理

CCS通过`<import>`标签实现自动依赖编译：

```xml
<!-- system.projectspec -->
<projectSpec>
    <import spec="../mss/.../xxx_mss.projectspec"/>   <!-- 自动导入MSS -->
    <import spec="../dss/.../xxx_dss.projectspec"/>   <!-- 自动导入DSS -->
    ...
</projectSpec>
```

#### ✅ 正确的导入方式（自动触发依赖编译）

```
步骤1: File → Import → CCS Projects
步骤2: Browse to: .../src/system/
步骤3: 只选择: health_detect_6844_system.projectspec
步骤4: 点击 Finish

CCS自动：
  ✅ 解析 <import> 标签
  ✅ 自动导入 MSS 项目
  ✅ 自动导入 DSS 项目
  ✅ 设置项目间依赖关系

编译时自动：
  ✅ 先编译 MSS → 生成 .rig
  ✅ 再编译 DSS → 生成 .rig
  ✅ 最后执行 System post-build → 生成 .appimage
```

#### ❌ 错误的导入方式（导致手动编译）

```
错误做法：分别导入3个项目
  File → Import → 选择 mss.projectspec → Finish
  File → Import → 选择 dss.projectspec → Finish
  File → Import → 选择 system.projectspec → Finish

问题：CCS不会自动识别项目间依赖！
结果：编译System会报错 "找不到.rig文件"
```

#### 项目配置检查清单

| 检查项 | 要求 | 验证方法 |
|-------|------|---------|
| system.projectspec有`<import>`标签 | 引用MSS和DSS | 查看XML文件 |
| `<import>`路径正确 | 相对路径可解析 | 路径存在 |
| system.xml项目名匹配 | 与MSS/DSS项目名一致 | 对比名称 |
| 导入方式正确 | 只从system导入 | 3个项目同时出现 |
| Dependencies设置 | System依赖MSS/DSS | Project Properties |

**🔴 重要说明**：

- ✅ **系统打包是必选项**，不是可选的
- ✅ **系统打包才是完整的**项目形态
- ✅ **方式1仅供开发调试**参考，不作为交付方式
- ✅ **最终交付必须是.appimage格式**

### .appimage文件结构

```
health_detect_system.release.appimage
├─────────────────────────────────
│ Meta Header (元信息)
├─────────────────────────────────
│ MSS Image
│  ├─ MSS.out (ARM代码)
│  └─ 加载地址: 0x00000000
├─────────────────────────────────
│ DSS Image
│  ├─ DSS.out (DSP代码)
│  └─ 加载地址: 0x00800000
├─────────────────────────────────
│ RF Firmware Patch
└─────────────────────────────────
```

### Milestone策略

**Milestone 1-2（开发阶段）**：

- ✅ 已完成MSS/DSS/System项目配置
- ✅ 先验证分别编译成功（快速迭代）
- 🔄 然后验证系统打包成功（Milestone 2必须完成）

**Milestone 3+（发布阶段）**：

- ✅ 只使用系统打包方式
- ✅ 交付.appimage文件

---

## 🔥 固件烧录与验证流程（新增）

> ⚠️ **重要**：编译生成.appimage后，必须完成以下步骤验证固件正常工作！
> 📎 **详细操作**：参见[第2章-标准Demo验证](../08-AWRL6844雷达健康检测实现方案/AWRL6844雷达健康检测-05-第2章-标准Demo验证.md)

### 1. 固件烧录

#### 1.1 硬件准备

**SOP跳线设置（烧录模式）**：
```
烧录模式: S7-OFF, S8-OFF
运行模式: S7-ON, S8-ON
```

**COM端口确认**：
```powershell
# 设备管理器查看端口
# CLI端口: COMx（如COM3）- 用于命令/配置
# 数据端口: COMy（如COM4）- 用于点云数据
```

#### 1.2 使用SDK Visualizer烧录（推荐）

```
步骤1: 启动Visualizer
       C:\ti\MMWAVE_L_SDK_06_01_00_01\tools\visualizer\visualizer.exe

步骤2: 切换到 Flash 标签页

步骤3: 设置SOP跳线为烧录模式（S7-OFF, S8-OFF）
       按S2复位键

步骤4: 选择固件文件
       project-code/AWRL6844_HealthDetect/.../health_detect_system.release.appimage

步骤5: 点击 FLASH 按钮，等待进度条到100%

步骤6: 恢复SOP跳线为运行模式（S7-ON, S8-ON）
       按S2复位键
```

#### 1.3 使用命令行烧录（备选）

```powershell
cd C:\ti\MMWAVE_L_SDK_06_01_00_01\tools\FlashingTool

.\arprog_cmdline_6844.exe -p COM3 `
  -f1 "D:\7.project\TI_Radar_Project\project-code\AWRL6844_HealthDetect\...\health_detect_system.release.appimage" `
  -s SFLASH `
  -c
```

#### 1.4 烧录成功标志

```
✅ 进度条到100%
✅ 显示 "Flashing completed successfully"
✅ 串口有启动信息输出（波特率115200）
```

---

### 2. 雷达配置加载

> 🚨 **关键步骤**：固件烧录后，**必须发送配置文件才能启动雷达**！
> 📎 **配置文件分析**：参见[附录G-雷达配置文件对比分析](../08-AWRL6844雷达健康检测实现方案/AWRL6844雷达健康检测-附录G-雷达配置文件对比分析.md)

#### 2.1 配置文件工作原理

```
烧录固件 → 发送配置文件(.cfg) → 执行sensorStart → 雷达输出数据
```

**没有配置文件，雷达不会工作！**

#### 2.2 标准配置文件位置

| 配置文件 | 路径 | 说明 |
|---------|------|------|
| **profile_4T4R_tdm.cfg** | `mmw_demo_SDK_reference/profiles/` | ✅ 推荐，4TX 4RX TDM模式 |
| **6844_profile_4T4R_tdm.cfg** | `MMWAVE_L_SDK/.../visualizer/tmp/` | SDK Visualizer默认 |
| **健康检测专用配置** | `AWRL6844_HealthDetect/cfg/` | 项目自定义配置（待开发） |

#### 2.3 标准配置文件关键参数

```properties
% 核心配置（mmw_demo标准格式）
channelCfg 153 255 0              % 4TX 4RX TDM模式
chirpComnCfg 8 0 0 256 1 13.1 3   % Chirp参数
frameCfg 64 0 1358 1 100 0        % 10 FPS帧率
cfarProcCfg 0 2 8 4 3 0 9.0 0     % CFAR检测
aoaProcCfg 64 64                  % AOA处理
aoaFovCfg -60 60 -60 60           % 视场角±60°
antGeometryBoard xWRL6844EVM      % 天线配置（简化）
sensorStart 0 0 0 0               % 启动雷达
```

#### 2.4 设备安装配置

> 📎 **详细安装信息**：参见[附录E-设备安装配置信息](../08-AWRL6844雷达健康检测实现方案/AWRL6844雷达健康检测-附录E-设备安装配置信息.md)

**实际安装参数**：
```
雷达位置: 墙面壁挂安装
离地高度: 1.78m
俯仰角度: -40°（向下俯视）
检测区域: 4m宽 × 5m深 × 2m高
```

**配置文件中的位置参数**（如需自定义）：
```properties
% 传感器位置配置（InCabin格式，标准mmw_demo不需要）
sensorPosition 0 0 1.78 0 -40
% 格式：X Y Z azimuthTilt elevationTilt

% 检测区域定义（InCabin格式，标准mmw_demo不需要）
cuboidDef 0 0  -2.0  2.0  0.2  5.0  0.0  2.0
% 格式：zoneId subZoneId xMin xMax yMin yMax zMin zMax
```

> ⚠️ **注意**：`sensorPosition`和`cuboidDef`是InCabin专用CLI命令，标准mmw_demo固件**不识别**！
> 如需区域定义功能，需要在HealthDetect固件中实现这些CLI命令。

#### 2.5 使用SDK Visualizer发送配置

```
步骤1: 启动SDK Visualizer（如已启动则继续）

步骤2: 切换到 Sensor Config 标签页

步骤3: 选择COM端口
        - CLI Port: COM3 (命令端口)
        - Data Port: COM4 (数据端口)

步骤4: 连接设备 → 点击 "Connect"

步骤5: 加载配置文件 → 点击 "Browse..."
        选择：mmw_demo_SDK_reference/profiles/profile_4T4R_tdm.cfg

步骤6: 发送配置 → 点击 "Send Config"
        每行命令应返回 "Done"

步骤7: 雷达自动启动，或手动点击 "Start Sensor"
```

#### 2.6 配置加载成功标志

```
✅ 所有CLI命令返回 "Done"
✅ 无 "Error" 或 "Invalid command" 提示
✅ sensorStart执行成功
✅ 雷达开始输出数据
```

---

### 3. 数据输出验证

#### 3.1 CLI端口验证

**连接参数**：
```
端口: CLI端口（如COM3）
波特率: 115200
数据位: 8, 停止位: 1, 校验: None
```

**验证命令响应**：
```
发送: help
预期: 显示可用命令列表

发送: version
预期: 显示固件版本信息
```

#### 3.2 数据端口验证（SDK Visualizer）

```
步骤1: 切换到 Plots 标签页

步骤2: 验证实时数据显示：
        - Range-Azimuth Heat Map（距离-方位热图）
        - Detected Objects（检测到的目标点）
        - Statistics（统计信息）

步骤3: 测试场景验证：
        - 静态环境 → 无点云或极少静态点
        - 人员走动 → 有明显移动点云
        - 手部摆动 → 能检测到小目标运动
```

#### 3.3 功能验证测试场景

| 测试场景 | 预期结果 | 验证标志 |
|---------|---------|---------|
| **静态环境** | 无点云或极少静态点 | ✅ 无虚警 |
| **人员走动** | 检测到移动点云 | ✅ 能跟踪 |
| **人员静止** | 点云稳定 | ✅ 位置准确 |
| **手部摆动** | 检测到小目标 | ✅ 灵敏度正常 |
| **多人场景** | 多个点云簇 | ✅ 可区分 |

#### 3.4 TLV数据格式验证

**验证点云数据使用标准格式**：
```
点云TLV: Type = 1 (MMWDEMO_OUTPUT_MSG_DETECTED_POINTS)
         ✅ SDK Visualizer能正常显示

自定义TLV: Type = 1000+ (健康检测扩展)
         ✅ SDK Visualizer安全忽略，不影响显示
```

> 📎 **TLV格式规范**：参见[附录F-TLV数据格式兼容性要求](../08-AWRL6844雷达健康检测实现方案/AWRL6844雷达健康检测-附录F-TLV数据格式兼容性要求.md)

---

### 4. 验证完成检查清单

| 类别 | 检查项 | 状态 |
|------|--------|------|
| **烧录** | 固件烧录成功（进度100%） | ⬜ |
| **烧录** | SOP已恢复运行模式 | ⬜ |
| **烧录** | 串口有启动信息 | ⬜ |
| **配置** | 配置文件发送成功 | ⬜ |
| **配置** | 所有CLI命令返回Done | ⬜ |
| **配置** | sensorStart执行成功 | ⬜ |
| **验证** | CLI命令响应正常 | ⬜ |
| **验证** | 数据端口有输出 | ⬜ |
| **验证** | Visualizer显示点云 | ⬜ |
| **验证** | 能检测到人员运动 | ⬜ |

**✅ 所有检查项通过后，固件验证完成！**

---

### 5. 常见问题排查

#### 问题1: 烧录超时

```
错误: Connect to Device Timeout

解决:
1. 确认SOP跳线正确（S7-OFF, S8-OFF）
2. 按S2复位键后立即执行烧录
3. 检查USB连接稳定性
4. 尝试命令行烧录
```

#### 问题2: 配置发送失败

```
错误: CLI命令响应超时或错误

解决:
1. 确认SOP已恢复运行模式（S7-ON, S8-ON）
2. 确认连接的是CLI端口（波特率115200）
3. 重启EVM后重试
4. 检查配置文件编码（必须是ASCII，不能UTF-8带BOM）
```

#### 问题3: 无点云输出

```
现象: CLI正常，但无点云数据

解决:
1. 确认sensorStart已执行
2. 检查数据端口波特率（必须是1250000）
3. 检查天线朝向（正面朝向监测区域）
4. 检查检测区域内是否有目标
```

---

## 📚 参考资源

### 必读的mmw_demo文件（🔴 编码前必须阅读）

| 文件                                      | 内容             | 必须学习   |
| ----------------------------------------- | ---------------- | ---------- |
| `xwrL684x-evm/r5fss0-0_freertos/main.c` | FreeRTOS入口     | ⭐⭐⭐⭐⭐ |
| `source/mmwave_demo.c` 1-200行          | 头文件、全局变量 | ⭐⭐⭐⭐⭐ |
| `source/mmwave_demo.c` 全部             | 主应用逻辑       | ⭐⭐⭐⭐   |
| `source/mmw_cli.c`                      | CLI命令          | ⭐⭐⭐⭐   |
| `source/dpc/dpc.c`                      | DPC处理          | ⭐⭐⭐     |

### SDK技术深度分析（推荐阅读）

> 📎 以下文档位于 `项目文档/3-固件工具/06-SDK固件研究/`

| 文档 | 内容 | 推荐程度 |
| ---- | ---- | -------- |
| [Part10-MMWAVE_L_SDK深度解析](../06-SDK固件研究/Part10-MMWAVE_L_SDK深度解析.md) | SDK完整架构、目录结构、组件分析 | ⭐⭐⭐⭐ |
| [Part13-SDK对比与RTOS深度解析](../06-SDK固件研究/Part13-SDK对比与RTOS深度解析.md) | L-SDK vs 标准SDK对比、FreeRTOS vs BIOS详解 | ⭐⭐⭐⭐⭐ |
| [Part14-TLV数据格式与工具兼容性完整指南](../06-SDK固件研究/Part14-TLV数据格式与工具兼容性完整指南.md) | TLV格式详解、工具兼容性分析 | ⭐⭐⭐⭐⭐ |
| [Part3-SDK与固件关系及工作流程](../06-SDK固件研究/Part3-SDK与固件关系及工作流程.md) | SDK与固件关系、编译流程 | ⭐⭐⭐ |

### 失败经验资料

- 位置：`项目文档/3-固件工具/09-.../失败经验资料/`
- 内容：v1.0的代码和文档，供对比参考
- 用途：避免重复错误

---

## ✅ 自检清单

### 编写代码前

- [ ] 阅读了mmw_demo/main.c？
- [ ] 阅读了mmw_demo/mmwave_demo.c的头文件部分？
- [ ] 确认要使用的API是FreeRTOS还是BIOS？
- [ ] 确认L-SDK使用FreeRTOS，不是BIOS？

### 编译前

- [ ] 检查是否有 `#include <ti/sysbios/...>` ？（必须为0）
- [ ] 检查是否有 `BIOS_start()` ？（必须为0）
- [ ] 检查是否有 `Task_create()` ？（必须为0）

### 编译后

- [ ] MSS项目0 Errors？
- [ ] DSS项目0 Errors？
- [ ] System项目生成.appimage？

---

## 📎 附录A：TLV数据格式兼容性要求

### 🔴 关键原则：必须兼容标准mmWave Demo TLV格式

**AWRL6844_HealthDetect 项目必须使用标准 mmWave Demo 的 TLV Type ID**，以确保与 SDK Visualizer 等官方测试工具兼容。

> ⭐ **最高优先级要求**：点云数据**必须使用Type=1**（标准DETECTED_POINTS格式），禁止使用Type=3001（InCabin私有格式）。
>
> 📎 **完整技术分析**: [附录F：TLV数据格式兼容性要求](../08-AWRL6844雷达健康检测实现方案/AWRL6844雷达健康检测-附录F-TLV数据格式兼容性要求.md)

### 快速参考表

| 数据类型 | 标准mmWave Demo | InCabin Demo | HealthDetect选择 |
|---------|----------------|--------------|------------------|
| **点云数据** | Type = 1 | Type = 3001 | **Type = 1** ✅ |
| **Range Profile** | Type = 2 | Type = 2 | **Type = 2** ✅ |
| **Stats统计** | Type = 6 | Type = 6 | **Type = 6** ✅ |
| **健康检测扩展** | ❌ 无 | ❌ 无 | **Type = 1000+** ✅ |

### 扩展TLV设计（从1000开始）

```c
// 健康检测专用TLV（从1000开始，避开官方范围）
#define MMWDEMO_OUTPUT_MSG_PRESENCE_DETECT      1000  // 人存检测结果
#define MMWDEMO_OUTPUT_MSG_HEALTH_FEATURES      1001  // 健康特征向量
#define MMWDEMO_OUTPUT_MSG_VITAL_SIGNS          1002  // 生命体征
#define MMWDEMO_OUTPUT_MSG_POSTURE_RESULT       1003  // 姿态检测结果
#define MMWDEMO_OUTPUT_MSG_FALL_DETECTION       1004  // 跌倒检测告警
```

### 设计原则

| 原则 | 说明 | 结果 |
|-----|------|------|
| **核心TLV完全兼容** | Type 1-12使用标准格式 | SDK Visualizer正常显示 |
| **扩展TLV不冲突** | Type 1000+自定义 | SDK Visualizer安全忽略 |
| **点云必须Type=1** | 最关键的兼容性要求 | 官方工具可用 |

---

## 📝 版本历史

| 版本 | 日期       | 说明                        |
| ---- | ---------- | --------------------------- |
| v1.0 | 2026-01-07 | 初版，架构和功能规划正确    |
|      |            | ❌ 代码使用了错误的BIOS API |
| v2.0 | 2026-01-08 | 添加FreeRTOS API规范        |
|      |            | 添加失败教训说明            |
|      |            | 强调必须先读源码            |
|      |            | 其他内容保持v1.0            |
| v2.1 | 2026-01-09 | 添加TLV格式兼容性附录       |
|      |            | 明确必须使用标准Demo格式    |
| v2.2 | 2026-01-09 | 添加SDK技术深度分析引用     |
|      |            | 链接Part10/Part13/Part14    |
| v2.3 | 2026-01-09 | 🆕 添加固件烧录验证流程     |
|      |            | 🆕 添加配置加载步骤         |
|      |            | 🆕 添加数据验证检查清单     |
|      |            | 🆕 引用附录E/附录G/第2章    |
| v2.4 | 2026-01-09 | 🆕 添加CCS自动依赖编译章节  |
|      |            | 🆕 链接Part15技术文档       |
|      |            | 🆕 添加项目配置检查清单     |

---

> ⚠️ **核心教训**：L-SDK使用FreeRTOS，不是TI-RTOS/BIOS！编写代码前必须阅读mmw_demo源码！

> 📌 **大方向正确**：三层架构、从零重建、功能规划都是正确的，只是API选择错误。
