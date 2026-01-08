# 📋 AWRL6844 Health Detect 项目重建总结

**日期**: 2026-01-08
**最后更新**: 2026-01-08 (编译问题修复)
**状态**: 代码框架创建完成，编译问题已修复，待重新导入CCS验证

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

## 🎯 任务目标

根据失败经验资料，重新创建 AWRL6844 Health Detect 项目代码框架。

**核心要求**：

1. ✅ 保持三层架构设计方向（未改变）
2. ✅ 修正API使用：BIOS API → FreeRTOS API
3. ✅ 严格参考mmw_demo源码的API用法

---

## 🔥 失败教训回顾

### 上次失败的根本原因

| 问题       | 错误做法                         | 正确做法                              |
| ---------- | -------------------------------- | ------------------------------------- |
| RTOS API   | `#include <ti/sysbios/BIOS.h>` | `#include "FreeRTOS.h"`             |
| 任务创建   | `Task_create()`                | `xTaskCreateStatic()`               |
| 调度器启动 | `BIOS_start()`                 | `vTaskStartScheduler()`             |
| 信号量     | `Semaphore_create()`           | `xSemaphoreCreateBinaryStatic()`    |
| SDK标识    | 未明确                           | `COM_TI_MMWAVE_L_SDK_6_INSTALL_DIR` |

### 教训总结

> **"AI在编写代码前必须仔细阅读参考源码，而不是凭'经验'使用其他SDK的API风格。'看代码'比'猜测'更可靠。"**

---

## 📁 创建的文件清单

### 项目根目录 (`project-code/AWRL6844_HealthDetect/`)

| 文件                           | 类型      | 说明                   |
| ------------------------------ | --------- | ---------------------- |
| `README.md`                  | 文档      | 项目主说明文档         |
| `mss_project.projectspec`    | CCS配置   | MSS项目配置（TICLANG） |
| `dss_project.projectspec`    | CCS配置   | DSS项目配置（C6000）   |
| `system_project.projectspec` | CCS配置   | 系统项目配置           |
| `system.syscfg`              | SysConfig | 外设配置               |

### Common层 (`src/common/`) - 共享接口

| 文件                      | 说明                                        |
| ------------------------- | ------------------------------------------- |
| `shared_memory.h`       | L3 RAM内存映射定义（0x51000000基址，896KB） |
| `data_path.h`           | DPC配置/结果结构（CFAR、AOA、点云）         |
| `health_detect_types.h` | 🆕 健康检测特征结构（新增功能）             |
| `mmwave_output.h`       | TLV输出格式（兼容SDK Visualizer）           |
| `README.md`             | 层说明文档                                  |

### MSS层 (`src/mss/`) - R5F应用层

| 文件                     | 说明                                         |
| ------------------------ | -------------------------------------------- |
| `health_detect_main.h` | 主控程序头文件，MCB结构定义                  |
| `health_detect_main.c` | 主控程序实现，**使用正确FreeRTOS API** |
| `cli.h`                | CLI命令接口头文件                            |
| `cli.c`                | CLI命令实现（sensorStart, sensorStop等）     |
| `dpc_control.h`        | DPC控制头文件                                |
| `dpc_control.c`        | DPC协调实现，IPC通信                         |
| `presence_detect.h`    | 🆕 存在检测模块头文件                        |
| `presence_detect.c`    | 🆕 存在检测算法实现                          |
| `tlv_output.h`         | TLV输出模块头文件                            |
| `tlv_output.c`         | TLV数据包构建与发送                          |
| `radar_control.h`      | 雷达控制头文件                               |
| `radar_control.c`      | mmWave API封装                               |
| `README.md`            | 层说明文档                                   |

### DSS层 (`src/dss/`) - C66x算法层

| 文件                  | 说明                                  |
| --------------------- | ------------------------------------- |
| `dss_main.h`        | DSP主程序头文件                       |
| `dss_main.c`        | DSP主程序实现，IPC处理                |
| `feature_extract.h` | 🆕 特征提取模块头文件                 |
| `feature_extract.c` | 🆕 特征提取实现（范围统计、运动能量） |
| `dsp_utils.h`       | DSP工具函数头文件                     |
| `dsp_utils.c`       | DSP工具函数实现                       |
| `README.md`         | 层说明文档                            |

### System层 (`src/system/`) - 系统配置

| 文件                | 说明                                       |
| ------------------- | ------------------------------------------ |
| `linker_mss.cmd`  | MSS链接脚本（R5F内存布局）                 |
| `linker_dss.cmd`  | DSS链接脚本（C66x内存布局）                |
| `system_config.h` | 系统配置参数（任务优先级、堆栈大小等）     |
| `system.xml`      | CCS System项目配置（定义核心和子项目关系） |
| `README.md`       | 层说明文档                                 |

---

## 🏗️ 架构设计

### 三层架构图

```
┌─────────────────────────────────────────────────────────────┐
│                    AWRL6844 Health Detect                    │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌─────────────────────────────────────────────────────┐   │
│  │              Common Layer (共享接口)                  │   │
│  │  shared_memory.h | data_path.h | health_detect_types.h│   │
│  └─────────────────────────────────────────────────────┘   │
│                           │                                  │
│           ┌───────────────┴───────────────┐                 │
│           ▼                               ▼                  │
│  ┌─────────────────────┐      ┌─────────────────────┐      │
│  │   MSS Layer (R5F)   │      │   DSS Layer (C66x)  │      │
│  │     FreeRTOS        │◄────►│     裸机/DPL        │      │
│  │                     │ IPC  │                     │      │
│  │  • CLI命令处理      │      │  • Range/Doppler FFT│      │
│  │  • DPC协调          │      │  • CFAR检测         │      │
│  │  • 存在检测 🆕      │      │  • AOA估计          │      │
│  │  • TLV输出          │      │  • 特征提取 🆕      │      │
│  │  • 雷达控制         │      │                     │      │
│  └─────────────────────┘      └─────────────────────┘      │
│           │                               │                  │
│           └───────────────┬───────────────┘                 │
│                           ▼                                  │
│  ┌─────────────────────────────────────────────────────┐   │
│  │              System Layer (系统配置)                  │   │
│  │    linker_mss.cmd | linker_dss.cmd | system_config.h │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### L3 RAM 内存布局

```
地址            大小    用途
──────────────────────────────────────
0x51000000      4KB     DPC Config
0x51001000      4KB     DPC Result
0x51002000      64KB    Point Cloud
0x51012000      32KB    Range Profile
0x5101A000      4KB     Health Features 🆕
0x5101B000      512KB   ADC Data
0x5109B000      ~276KB  Reserved
──────────────────────────────────────
Total:          896KB   L3 Shared RAM
```

---

## 🆕 新增功能

### 1. 存在检测 (Presence Detection)

**位置**: `src/mss/presence_detect.c`

**功能**：分析点云判断目标存在与运动状态

```c
typedef struct PresenceDetect_Result {
    uint8_t  isPresent;         // 目标存在
    uint8_t  isMoving;          // 目标移动
    uint16_t numPointsInZone;   // 检测区点数
    float    avgRange_m;        // 平均距离
    float    avgVelocity_mps;   // 平均速度
} PresenceDetect_Result_t;
```

**默认配置**：

- 最小点数: 5
- 距离范围: 0.5m - 3.0m
- 速度阈值: 0.1 m/s
- 保持帧数: 10

### 2. 特征提取 (Feature Extraction)

**位置**: `src/dss/feature_extract.c`

**功能**：从点云数据提取健康检测相关特征

```c
typedef struct HealthDetect_Features {
    StatisticsInfo_t rangeStats;      // 距离统计
    StatisticsInfo_t velocityStats;   // 速度统计
    float motionEnergy;               // 运动能量
    float motionEnergySmoothed;       // 平滑运动能量
    float peakSnr_dB;                 // 峰值信噪比
    uint16_t numValidPoints;          // 有效点数
} HealthDetect_Features_t;
```

---

## ⚙️ 编译环境要求

### 工具版本

| 工具         | 版本      | 说明                        |
| ------------ | --------- | --------------------------- |
| CCS          | 12.8.1+   | IDE                         |
| mmWave L-SDK | 6.5.0.0   | **L-SDK** (Low-Power) |
| SysConfig    | 1.21.0+   | 配置工具                    |
| TI CLANG     | 4.0.4.LTS | MSS编译器                   |
| TI C6000     | 8.5.0.LTS | DSS编译器                   |

### 编译选项

**MSS (R5F)**:

```
-mcpu=cortex-r5 -mfloat-abi=hard -mfpu=vfpv3-d16 -mthumb
```

**DSS (C66x)**:

```
-mv6600 --abi=eabi --opt_for_speed=5
```

---

## 🔧 CCS导入问题及解决方案

### 问题1: 设备ID无法识别

**错误信息**：

```
Device 'Cortex R.AWRL6844' is not currently recognized
Device 'TMS320C66XX.AWRL6844' is not currently recognized
```

**原因**：CCS不识别AWRL6844这个设备ID

**解决方案**：修改为AWRL68xx系列ID

```xml
<!-- 错误 -->
deviceId="Cortex R.AWRL6844"
deviceId="TMS320C66XX.AWRL6844"

<!-- 正确 -->
deviceId="Cortex R.AWRL68xx"
deviceId="TMS320C66XX.AWRL68xx"
```

### 问题2: SDK产品无法识别

**错误信息**：

```
Product com.ti.MMWAVE_L_SDK v0.0 is not currently installed and no compatible version is available
```

**原因**：products字段名称错误

**解决方案**：使用正确的SDK产品名称

```xml
<!-- 错误 -->
products="sysconfig;com.ti.MMWAVE_L_SDK"

<!-- 正确 -->
products="sysconfig;MMWAVE-L-SDK-6"
```

### 问题3: 源文件路径无法解析

**错误信息**：

```
Path '../src/mss/health_detect_main.c' cannot be resolved
Path '../src/dss/dss_main.c' cannot be resolved
```

**原因**：projectspec在项目根目录，使用 `../src/`路径不正确

**解决方案**：修正相对路径

```xml
<!-- 错误 -->
<file path="../src/mss/health_detect_main.c" ... />
-I${PROJECT_ROOT}/../src

<!-- 正确 -->
<file path="src/mss/health_detect_main.c" ... />
-I${PROJECT_ROOT}/src
```

### 问题4: System项目无法自动导入MSS/DSS子项目 ⭐⭐⭐

**现象**：

- 在CCS中导入 `system_project.projectspec`后，MSS和DSS项目不会自动导入
- 需要手动分别导入3个projectspec文件

**原因分析**：
❌ **错误用法** - 使用 `<linkedResources>`或 `<buildDependency>`：

```xml
<!-- 这些标签不会触发自动导入 -->
<linkedResources>
    <link>
        <name>mss</name>
        <locationURI>PROJECT_LOC/../health_detect_mss</locationURI>
    </link>
</linkedResources>

<buildDependency>
    <project name="health_detect_mss"/>
</buildDependency>
```

✅ **正确用法** - 使用 `<import>`标签：

```xml
<!-- System项目文件开头，在<project>标签之前 -->
<projectSpec>
    <!-- 自动导入子项目 -->
    <import spec="mss_project.projectspec"/>
    <import spec="dss_project.projectspec"/>
  
    <project name="health_detect_system" ... >
        ...
    </project>
</projectSpec>
```

**关键点**：

- `<import>` 标签必须放在 `<project>` 标签**之前**
- `spec` 属性填写相对于system projectspec的路径
- 导入system项目时，CCS会自动导入spec指定的子项目

**参考示例**：`InCabin_Demos/src/system/demo_in_cabin_sensing_6844_system.projectspec`

```xml
<?xml version="1.0" encoding="UTF-8"?>
<projectSpec>
    <!-- 关键：先import子项目 -->
    <import spec="../mss/.../demo_in_cabin_sensing_6844_mss.projectspec"/>
    <import spec="../dss/.../demo_in_cabin_sensing_6844_dss.projectspec"/>
  
    <project name="demo_in_cabin_sensing_6844_system" ...>
        ...
    </project>
</projectSpec>
```

**修正方案**：

```xml
<!-- 修正前 -->
<projectSpec>
    <applicability>...</applicability>
    <project ...>
        <linkedResources>...</linkedResources>
    </project>
</projectSpec>

<!-- 修正后 -->
<projectSpec>
    <import spec="mss_project.projectspec"/>
    <import spec="dss_project.projectspec"/>
  
    <project ...>
        <!-- 不需要linkedResources -->
    </project>
</projectSpec>
```

### ✅ 导入成功确认

**导入结果**：

- ✅ MSS项目：无错误
- ✅ DSS项目：无错误
- ✅ System项目：无错误

**修正文件清单**：

| 文件                           | 修正内容                                           |
| ------------------------------ | -------------------------------------------------- |
| `mss_project.projectspec`    | deviceId, products, 文件路径                       |
| `dss_project.projectspec`    | deviceId, products, 文件路径, include路径          |
| `system_project.projectspec` | deviceId, products,**添加 `<import>`标签** |
| `src/system/system.xml`      | **新增** - 定义多核系统结构                  |

---

## 🐛 编译问题及解决方案

### 问题1: System项目编译错误 - no input files

**错误信息**：

```
#10009: no input files
```

**原因**：System项目是容器项目，不应编译可执行文件

**解决方案**：修正outputType

```xml
<!-- 错误 -->
<project
    outputFormat="ELF"
    cgtVersion="4.0.4.LTS"
    isLinkable="false"
>

<!-- 正确 -->
<project
    outputType="system"
    toolChain="TICLANG"
>
```

### 问题2: DSS项目编译错误 - 找不到头文件

**错误信息**：

```
#1965: cannot open source file "dsp_utils.h"
#1965: cannot open source file "kernel/dpl/DebugP.h"
```

**原因**：SDK include路径不完整

**解决方案**：补充完整的SDK头文件路径

```xml
<!-- 不足 -->
<compilerBuildOptions>
    -I${PROJECT_ROOT}/src
    -I${SDK_INSTALL_DIR}/source
</compilerBuildOptions>

<!-- 完整 -->
<compilerBuildOptions>
    -I${CG_TOOL_ROOT}/include                    <!-- 编译器头文件 -->
    -I${PROJECT_ROOT}/src/dss                    <!-- 本地头文件 -->
    -I${SDK_INSTALL_DIR}/source                  <!-- SDK根目录 -->
    -I${SDK_INSTALL_DIR}/source/kernel/dpl       <!-- DPL层（DebugP.h等） -->
    -I${SDK_INSTALL_DIR}/source/drivers          <!-- 驱动层 -->
    -I${SDK_INSTALL_DIR}/firmware/mmwave_dfp     <!-- 毫米波DFP -->
</compilerBuildOptions>
```

### 问题3: System项目导入错误 - system.xml文件缺失

**错误信息**：

```
Problems importing projects: Path 'src/system/system.xml' cannot be resolved
```

**原因**：缺少 `system.xml`文件，该文件定义多核系统结构

**解决方案**：创建 `system.xml`文件

```xml
<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<system>
    <!-- MSS Project on Cortex-R5 Core -->
    <project configuration="@match" id="project_0" name="health_detect_mss">
    </project>
    <core id="Cortex_R5_0" project="project_0"/>
  
    <!-- DSS Project on C66x DSP Core -->
    <project configuration="@match" id="project_1" name="health_detect_dss">
    </project>
    <core id="C66xx_DSP" project="project_1"/>
  
    <!-- Pre-build steps -->
    <preBuildSteps>
    </preBuildSteps>
  
    <!-- Post-build steps -->
    <postBuildSteps>
        <step command="echo System build completed"/>
    </postBuildSteps>
</system>
```

**说明**：

- `system.xml`定义了MSS和DSS项目与硬件核心的绑定关系
- CCS通过此文件识别这是一个多核系统项目
- 文件路径：`src/system/system.xml`

### 问题4: MSS项目编译错误 - big endian not supported

**错误信息**：

```
tiarmclang: error: big endian not supported for subtarget.
```

**原因**：MSS projectspec缺少 `endianness="little"`配置，CCS默认使用了大端模式

**解决方案**：在project标签中添加endianness属性

```xml
<!-- 错误 - 缺少endianness -->
<project
    device="Cortex R.AWRL68xx"
    outputFormat="ELF"
>

<!-- 正确 - 指定小端模式 -->
<project
    device="Cortex R.AWRL68xx"
    deviceCore="Cortex_R5_0"
    endianness="little"
    outputFormat="ELF"
    outputType="executable"
    ignoreDefaultCCSSettings="true"
>
```

**关键点**：

- AWRL6844的R5F和C66x核心都使用**小端模式**
- 必须明确指定 `endianness="little"`
- 同时添加 `deviceCore`、`outputType`、`ignoreDefaultCCSSettings`确保CCS正确识别

### 问题5: DSS/MSS编译选项未生效 - include路径丢失

**错误信息**：

```
DSS: cannot open source file "kernel/dpl/DebugP.h"
MSS: 编译选项中出现-mbig-endian
```

**原因**：使用了 `<buildOptions>`嵌套标签，CCS可能无法正确解析

**解决方案**：将编译选项直接写在 `<project>`标签的属性中

```xml
<!-- 错误 - 嵌套在buildOptions中 -->
<project ...>
    <buildOptions>
        <compilerBuildOptions>
            -I${SDK_INSTALL_DIR}/source
            -DSOC_AWRL6844
        </compilerBuildOptions>
    </buildOptions>
</project>

<!-- 正确 - 直接作为project属性 -->
<project
    ...
    compilerBuildOptions="
        -I${SDK_INSTALL_DIR}/source
        -I${SDK_INSTALL_DIR}/source/kernel/dpl
        -DSOC_AWRL6844
    "
    linkerBuildOptions="
        -i${SDK_INSTALL_DIR}/source/drivers/lib
    "
>
</project>
```

**关键点**：

- CCS对projectspec的解析可能因版本而异
- 参考InCabin_Demos的格式，直接将选项作为project属性
- 多行字符串需要正确缩进

### 问题6: SDK_INSTALL_DIR变量无法解析

**错误信息**：

```
Build-variable 'SDK_INSTALL_DIR' cannot be resolved. This project may not build as expected.
```

**原因**：`pathVariable`定义在 `<project>`标签内部，但在 `compilerBuildOptions`属性中就已经使用

**错误的定义方式**：

```xml
<project
    compilerBuildOptions="
        -I${SDK_INSTALL_DIR}/source    <!-- 这里就用了 -->
    "
>
    <!-- 但变量定义在这里 -->
    <pathVariable name="SDK_INSTALL_DIR" pathType="installPath" .../>
</project>
```

**正确的解决方案**：

```xml
<project ...>
    <!-- 变量定义必须在使用之前（文件列表之前） -->
    <pathVariable name="SDK_INSTALL_DIR" path="${COM_TI_MMWAVE_L_SDK_6_INSTALL_DIR}" scope="project"/>
  
    <!-- Source files -->
    <file path="src/..." />
</project>
```

**关键点**：

- 虽然在 `compilerBuildOptions`**属性**中使用了变量，但CCS仍然需要在 `<project>`的**子元素**中定义
- 使用 `path="${...}"`而不是 `pathType="installPath"`
- 添加 `scope="project"`确保项目范围可见
- 参考InCabin_Demos的做法：变量定义在配置标签之后，文件列表之前

### 问题7: DSS/MSS编译找不到本地头文件

**错误信息**：

```
cannot open source file "dsp_utils.h"
cannot open source file "dss_main.h"
cannot open source file "feature_extract.h"
```

**原因**：CCS将源文件导入到工作区根目录，但projectspec没有添加 `action="copy"`指令

**问题分析**：

- 源文件在 `src/dss/dsp_utils.c`
- 源文件中 `#include "dsp_utils.h"`期望头文件在同一目录
- CCS导入时如果没有 `action="copy"`，会创建链接而不是复制文件
- 编译时找不到相对路径的头文件

**解决方案**：添加 `action="copy"`，同时列出头文件

```xml
<!-- 错误 - 没有action，没有列出头文件 -->
<file path="src/dss/dss_main.c" openOnCreation="false" excludeFromBuild="false"/>
<file path="src/dss/dsp_utils.c" openOnCreation="false" excludeFromBuild="false"/>

<!-- 正确 - 添加action="copy"，列出所有.c和.h文件 -->
<file path="src/dss/dss_main.c" openOnCreation="false" excludeFromBuild="false" action="copy"/>
<file path="src/dss/dss_main.h" openOnCreation="false" excludeFromBuild="false" action="copy"/>
<file path="src/dss/dsp_utils.c" openOnCreation="false" excludeFromBuild="false" action="copy"/>
<file path="src/dss/dsp_utils.h" openOnCreation="false" excludeFromBuild="false" action="copy"/>
```

**`action="copy"`的作用**：

- CCS会将文件从原位置复制到项目工作区根目录
- `.c`文件和对应的 `.h`文件会在同一目录，`#include "xxx.h"`能够找到
- 这是TI官方示例项目的标准做法

**修正内容**：

- DSS项目：添加了3对.c/.h文件的 `action="copy"`
- MSS项目：添加了6对.c/.h文件的 `action="copy"`

### 问题8: DSS编译错误 - 未定义类型 `PointCloud_Point_t` 和 `SubFrame_Cfg_t`

**日期**: 2026-01-08

**错误信息**：

```
"../source/feature_extract.h", line 158: error #20: identifier "PointCloud_Point_t" is undefined
"../source/health_detect_dss.h", line 225: error #20: identifier "SubFrame_Cfg_t" is undefined
```

**原因**：

- `feature_extract.c/h` 使用了 `PointCloud_Point_t` 类型，但 `data_path.h` 中只定义了 `PointCloud_Cartesian_t` 和 `PointCloud_Spherical_t`
- `health_detect_dss.c/h` 使用了 `SubFrame_Cfg_t` 类型，但该类型未定义
- InCabin_Demos 参考项目中使用的是 `SubFrameObj_t`（但那是空结构体）

**解决方案**：在 `data_path.h` 中添加缺失的类型定义

**修改文件**: `src/common/data_path.h`

```c
/*===========================================================================*/
/*                         SubFrame Configuration                             */
/*===========================================================================*/

/**
 * @brief SubFrame Configuration Structure
 * Configuration parameters for each subframe
 */
typedef struct SubFrame_Cfg_t
{
    /* Antenna Configuration */
    uint8_t     numTxAntennas;              /**< Number of TX antennas enabled */
    uint8_t     numRxAntennas;              /**< Number of RX antennas enabled */
    uint16_t    numVirtualAntennas;         /**< Number of virtual antennas */
    
    /* Range Configuration */
    uint16_t    numRangeBins;               /**< Number of range bins */
    uint16_t    numAdcSamples;              /**< Number of ADC samples per chirp */
    
    /* Doppler Configuration */
    uint16_t    numDopplerBins;             /**< Number of Doppler bins */
    uint16_t    numChirpsPerFrame;          /**< Total chirps per frame */
    
    /* Frame Timing */
    float       framePeriodMs;              /**< Frame period in milliseconds */
    float       chirpDurationUs;            /**< Single chirp duration in microseconds */
    
    /* Processing Configuration */
    DPC_StaticConfig_t  staticCfg;          /**< Static DPC configuration */
    DPC_DynamicConfig_t dynamicCfg;         /**< Dynamic DPC configuration */
    
    /* Memory Addresses */
    void        *radarCubeAddr;             /**< Radar cube memory address */
    uint32_t    radarCubeSize;              /**< Radar cube size in bytes */
    
    /* Flags */
    uint8_t     isValid;                    /**< Configuration valid flag */
} SubFrame_Cfg_t;

/*===========================================================================*/
/*                         Point Cloud Structures                             */
/*===========================================================================*/

/**
 * @brief Generic Point Cloud Point
 * Generic point structure used for processing (alias to Cartesian)
 */
typedef PointCloud_Cartesian_t PointCloud_Point_t;
```

**添加位置**：
- `SubFrame_Cfg_t` 在 `DPC_Config_t` 之后添加
- `PointCloud_Point_t` 在 `PointCloud_SideInfo_t` 之后、`PointCloud_Output_t` 之前添加

### 问题9: DSS编译错误 - include 路径风格不一致导致找不到头文件

**日期**: 2026-01-08

**错误信息**：

```
"../source/feature_extract.c", line 30: fatal error #5: could not open source file "common/health_detect_types.h"
"../source/feature_extract.c", line 31: fatal error #5: could not open source file "common/data_path.h"
```

**原因分析**：

CCS 使用 `action="copy"` 时的目录结构：

```
CCS_project_dir/
├── feature_extract.c       # 从 src/dss/source/ 复制
├── feature_extract.h       # 从 src/dss/source/ 复制
├── common/                 # targetDirectory="common" 创建
│   ├── data_path.h        # 从 src/common/ 复制
│   ├── health_detect_types.h
│   └── shared_memory.h
```

projectspec 中的配置：

```xml
<!-- common 头文件复制到 common/ 子目录 -->
<file path="${PROJECT_COMMON_PATH}/data_path.h" targetDirectory="common" action="copy"/>
```

因此：
- 源文件使用 `#include "../../common/data_path.h"` → ❌ 错误（相对路径在复制后无效）
- 源文件使用 `#include <common/data_path.h>` → ⚠️ 可能有问题（需要 include path 正确配置）
- 源文件使用 `#include "common/data_path.h"` → ✅ 正确（项目根目录下有 common/ 子目录）

**解决方案**：统一所有文件使用 `"common/xxx.h"` 格式

**修改的文件列表**：

| 文件 | 修改前 | 修改后 |
|------|--------|--------|
| `src/dss/source/feature_extract.h` | `<common/data_path.h>` | `"common/data_path.h"` |
| `src/dss/source/health_detect_dss.h` | `"../../common/data_path.h"` | `"common/data_path.h"` |
| `src/mss/source/health_detect_main.h` | `<common/data_path.h>` | `"common/data_path.h"` |
| `src/mss/source/dpc_control.h` | `<common/data_path.h>` | `"common/data_path.h"` |
| `src/mss/source/dpc_control.c` | `<common/shared_memory.h>` | `"common/shared_memory.h"` |
| `src/mss/source/presence_detect.h` | `<common/...>` | `"common/..."` |
| `src/mss/source/tlv_output.h` | `<common/...>` | `"common/..."` |

**关键教训**：

> ⚠️ **使用 `action="copy"` 时，必须考虑复制后的目录结构！**
> 
> - 源文件中的相对路径 `"../../common/xxx.h"` 在复制后会失效
> - 必须使用与 `targetDirectory` 配置一致的路径
> - 统一使用 `"common/xxx.h"` 格式最可靠

### 问题10: DSS编译错误 - `PointCloud_Point_t` 缺少球坐标和SNR字段

**日期**: 2026-01-08

**错误信息**：

```
"../feature_extract.c", line 254: error #137: struct "PointCloud_Cartesian_t" has no field "range"
"../feature_extract.c", line 255: error #137: struct "PointCloud_Cartesian_t" has no field "snr"
"../feature_extract.c", line 273: error #137: struct "PointCloud_Cartesian_t" has no field "azimuth"
"../feature_extract.c", line 274: error #137: struct "PointCloud_Cartesian_t" has no field "elevation"
```

**原因**：

- `PointCloud_Point_t` 被定义为 `PointCloud_Cartesian_t` 的别名
- `PointCloud_Cartesian_t` 只有 `x`, `y`, `z`, `velocity` 四个字段
- `feature_extract.c` 需要访问 `range`, `azimuth`, `elevation`, `snr` 字段

**解决方案**：将 `PointCloud_Point_t` 改为完整的结构体定义

**修改文件**: `src/common/data_path.h`

```c
/**
 * @brief Generic Point Cloud Point
 * Complete point structure with both Cartesian and Spherical coordinates plus SNR
 * Used for feature extraction and health detection processing
 */
typedef struct PointCloud_Point_t
{
    /* Cartesian Coordinates */
    float       x;                  /**< X coordinate in meters */
    float       y;                  /**< Y coordinate in meters */
    float       z;                  /**< Z coordinate in meters */
    
    /* Spherical Coordinates */
    float       range;              /**< Range in meters */
    float       azimuth;            /**< Azimuth angle in radians */
    float       elevation;          /**< Elevation angle in radians */
    
    /* Velocity */
    float       velocity;           /**< Radial velocity in m/s */
    
    /* Quality */
    float       snr;                /**< Signal-to-noise ratio in dB */
} PointCloud_Point_t;
```

**设计说明**：

- 包含笛卡尔坐标 (x, y, z) 用于质心计算
- 包含球坐标 (range, azimuth, elevation) 用于特征提取
- 包含 SNR 用于质量过滤
- 这是一个完整的点云点结构，适合健康检测处理

### 问题11: DSS编译错误 - 枚举类型初始化和不可达代码

**日期**: 2026-01-08

**错误信息**：

```
"../health_detect_dss.c", line 114: error #190-D: enumerated type mixed with another type
"../health_detect_dss.c", line 619: error #112-D: statement is unreachable
```

**原因分析**：

1. **枚举类型混用**：`HealthDSS_MCB_t gHealthDssMCB = {0};` 中，第一个成员 `currentState` 是枚举类型 `HealthDSS_State_e`，用 `0` 初始化会产生警告（在 `--emit_warnings_as_errors` 模式下变成错误）

2. **不可达代码**：`while(1)` 循环后的代码永远不会执行

**解决方案**：

1. **移除 `= {0}` 初始化器**：依赖 `HealthDSS_init()` 函数中的 `memset()` 来初始化

```c
/* 错误 */
HealthDSS_MCB_t gHealthDssMCB = {0};

/* 正确 */
HealthDSS_MCB_t gHealthDssMCB;
```

2. **用 `#if 0` 包裹不可达代码**：

```c
while (1)
{
    if (xQueueReceive(gHealthDssMCB.eventQueue, &msg, portMAX_DELAY) == pdPASS)
    {
        HealthDSS_handleMessage(&msg);
    }
}

/* Note: Code below is intentionally unreachable - kept for shutdown sequence reference */
#if 0
    SemaphoreP_pend(&gHealthDssMCB.initCompleteSem, SystemP_WAIT_FOREVER);
    Board_driversClose();
    Drivers_close();
#endif
```

**关键教训**：

> ⚠️ **TI C6000 编译器对类型检查非常严格！**
> 
> - 枚举类型不能用整数 `0` 初始化（会产生 #190-D 警告）
> - 使用 `--emit_warnings_as_errors` 时，所有警告都会变成错误
> - 不可达代码会产生 #112-D 警告

---

## 📋 下一步：编译验证

### 编译顺序

1. **DSS项目**（独立编译）
2. **MSS项目**（可能依赖DSS的输出）
3. **System项目**（依赖前两者）

### 预期可能的编译问题

| 问题类型     | 可能原因       | 解决方向        |
| ------------ | -------------- | --------------- |
| 头文件找不到 | SDK路径配置    | 检查include路径 |
| 链接库找不到 | 库文件路径     | 检查linker路径  |
| API未定义    | SDK版本差异    | 参考mmw_demo    |
| 类型不匹配   | 结构体定义差异 | 对齐SDK定义     |

---

## 📊 统计

| 项目               | 数量         |
| ------------------ | ------------ |
| 创建的源文件 (.c)  | 9            |
| 创建的头文件 (.h)  | 10           |
| 创建的配置文件     | 6            |
| 创建的文档         | 6            |
| **总文件数** | **31** |
| **总文件数** | **30** |

---

## ✅ 完成状态

| 阶段                  | 状态      | 说明                           |
| --------------------- | --------- | ------------------------------ |
| 需求文档v2            | ✅ 完成   | 保留三层架构，添加FreeRTOS规范 |
| Common层              | ✅ 完成   | 4个头文件 + 类型定义补充       |
| MSS层                 | ✅ 完成   | 6对.c/.h文件                   |
| DSS层                 | ✅ 完成   | 3对.c/.h文件                   |
| System层              | ✅ 完成   | 链接脚本+配置                  |
| CCS项目配置           | ✅ 完成   | 3个projectspec                 |
| README文档            | ✅ 完成   | 各层+主README                  |
| **类型定义修复** | ✅ 完成   | 添加 `SubFrame_Cfg_t`、`PointCloud_Point_t` (2026-01-08) |
| **Include路径修复** | ✅ 完成 | 统一使用 `"common/xxx.h"` 格式 (2026-01-08) |
| **PointCloud_Point_t完善** | ✅ 完成 | 添加球坐标和SNR字段 (2026-01-08) |
| **枚举初始化修复** | ✅ 完成 | 移除 `= {0}` 和不可达代码 (2026-01-08) |
| **CCS编译验证** | ⏳ 进行中 | DSS编译中，需重新验证       |

---

## 📊 雷达功能对比分析

### 需求文档v2中定义的雷达功能

**需求文档路径**：`项目文档/3-固件工具/09-AWRL6844 从mmw_demo演进Health Detection基础功能项目/AWRL6844_HealthDetect需求文档v2.md`

**明确包含的mmw_demo雷达功能**：

| 功能模块    | 需求文档中的定义                           | mmw_demo来源                           |
| ----------- | ------------------------------------------ | -------------------------------------- |
| 雷达控制    | `radar_control.c/h` - mmWave API封装     | `mmwave_control/` 目录               |
| mmWave API  | 频率配置、Profile/Chirp/Frame配置          | `MMWave_init/open/config/start/stop` |
| CLI配置命令 | `frameCfg`, `profileCfg`, `chirpCfg` | `mmw_cli.c` 的CLI命令                |
| 帧处理循环  | 帧触发、帧处理、帧完成回调                 | `mmwave_demo.c` 的主循环             |

**需求文档明确说明**：

> "根据**第3章演进架构**，将TI mmWave SDK的mmw_demo功能**完整重建**为新的三层架构健康检测项目"
>
> "参考与重建的关系：📚 仅作为参考，学习其功能实现、API调用方式、数据结构"

### 实际创建的AWRL6844_HealthDetect雷达功能

**项目路径**：`project-code/AWRL6844_HealthDetect/`

**已实现的雷达功能**：

| 文件                             | 雷达功能实现                                     | 状态   |
| -------------------------------- | ------------------------------------------------ | ------ |
| `src/mss/radar_control.c`      | ✅ mmWave API封装（init/open/config/start/stop） | 已实现 |
| `src/mss/radar_control.h`      | ✅ 雷达控制接口定义                              | 已实现 |
| `src/mss/cli.c`                | ✅ CLI命令（frameCfg, profileCfg等）             | 已实现 |
| `src/mss/health_detect_main.c` | ✅ 帧处理循环、mmWave回调                        | 已实现 |
| `src/common/data_path.h`       | ✅ 帧配置结构（Frame_Config_t）                  | 已实现 |

**代码证据**：

1. **radar_control.c 包含完整的mmWave API**：

```c
#include <ti/mmwave/mmwave.h>
#include <ti/mmwave/mmwave_error.h>

int32_t RadarControl_open(void)
{
    openCfg.freqLimitLow  = 60000U;   /* 60 GHz */
    openCfg.freqLimitHigh = 64000U;   /* 64 GHz */
    gMmWaveHandle = MMWave_open(...);
}
```

2. **cli.c 包含雷达配置命令**：

```c
static int32_t CLI_cmdFrameCfg(int32_t argc, char* argv[])
{
    Frame_Config_t* cfg = &gHealthDetectMCB.cliCfg.frameCfg;
    cfg->numChirpsPerFrame = atoi(argv[1]);
    cfg->framePeriodMs = (float)atof(argv[3]);
}
```

3. **health_detect_main.c 包含帧处理**：

```c
#include <control/mmwave/mmwave.h>

/* Initialize radar control (mmWave API) */
RadarControl_init();

/* Frame start callback */
static void HealthDetect_frameStartCallback(...)

/* Start radar via mmWave API */
RadarControl_start(&gHealthDetectMCB.cliCfg);
```

### mmw_demo_SDK_reference中的雷达功能

**项目路径**：`project-code/mmw_demo_SDK_reference/`

**包含的雷达功能**：

| 文件                       | 雷达功能                                    |
| -------------------------- | ------------------------------------------- |
| `source/mmwave_demo.c`   | MMWave_init/open/config/start/stop          |
| `source/mmwave_control/` | 雷达配置管理（如果存在）                    |
| `source/mmw_cli.c`       | CLI命令（frameCfg, profileCfg, chirpCfg等） |

**代码证据**：

```c
// mmwave_demo.c
#include <mmwavelink/include/rl_device.h>
#include <mmwavelink/include/rl_sensor.h>

MMWave_open(gMmwMssMCB.ctrlHandle, &gMmwMssMCB.mmWaveCfg, &errCode)
MMWave_config(gMmwMssMCB.ctrlHandle, &gMmwMssMCB.mmWaveCfg, &errCode)
MMWave_stop(gMmwMssMCB.ctrlHandle, ...)
```

### 对比结论

| 对比项               | mmw_demo_SDK_reference | AWRL6844_HealthDetect         | 是否相同？              |
| -------------------- | ---------------------- | ----------------------------- | ----------------------- |
| **雷达初始化** | ✅ MMWave_init/open    | ✅ RadarControl_init/open     | 🟢 功能相同，封装不同   |
| **雷达配置**   | ✅ MMWave_config       | ✅ RadarControl_config        | 🟢 功能相同，封装不同   |
| **雷达启停**   | ✅ MMWave_start/stop   | ✅ RadarControl_start/stop    | 🟢 功能相同，封装不同   |
| **CLI命令**    | ✅ frameCfg/profileCfg | ✅ frameCfg/profileCfg        | 🟢 命令相同             |
| **帧处理循环** | ✅ mmwave_demo.c主循环 | ✅ health_detect_main.c主循环 | 🟢 逻辑相同，代码重写   |
| **API调用**    | ✅ 直接调用mmWave API  | ✅ 通过radar_control封装      | 🟡 间接调用，多一层封装 |
| **代码结构**   | ❌ 单体架构            | ✅ 三层架构                   | 🔴 结构不同             |

### 最终答案

**问题1**: 需求文档v2是否包含mmw_demo的雷达功能？

- ✅ **是的**，需求文档明确要求"完整重建mmw_demo功能"
- ✅ 包含雷达控制、CLI配置、帧处理等核心功能
- ✅ 参考 `mmwave_control/`、`mmwave_demo.c`、`mmw_cli.c`

**问题2**: AWRL6844_HealthDetect是否包含雷达功能？

- ✅ **是的**，已实现完整的雷达控制功能
- ✅ `radar_control.c` 封装了所有mmWave API
- ✅ `cli.c` 实现了雷达配置命令
- ✅ `health_detect_main.c` 实现了帧处理循环

**问题3**: 两者的雷达功能是否相同？

- 🟢 **功能层面相同**：都实现了雷达初始化、配置、启动、停止、帧处理
- 🟢 **API层面相同**：都使用TI mmWave L-SDK的API
- 🟡 **调用方式不同**：HealthDetect通过 `radar_control`模块封装，mmw_demo直接调用
- 🔴 **架构完全不同**：HealthDetect是三层架构，mmw_demo是单体架构

### 建议后续对比验证

虽然代码已创建，但**强烈建议**对比验证以下内容：

1. **Profile/Chirp/Frame配置参数**

   - mmw_demo的默认配置值
   - HealthDetect的配置值是否合理
2. **回调函数注册**

   - mmw_demo如何注册frameStart/frameStop回调
   - HealthDetect是否正确注册
3. **错误处理**

   - mmw_demo的错误处理流程
   - HealthDetect是否覆盖所有错误情况
4. **频率范围**

   - mmw_demo: 60-64GHz（AWRL6844规格）
   - HealthDetect: 已配置60-64GHz ✅

---

> 🎯 **目标**: 编译通过 → 能烧录 → 能运行 → 功能验证
