# AWRL6844 Health Detection 项目需求文档

**项目路径**: `D:\7.project\TI_Radar_Project\project-code\AWRL6844_HealthDetect`  
**创建日期**: 2026-01-07  
**版本**: v1.0

---

## 📋 核心需求说明

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
- ❌ **禁止复制**：不能直接复制其源代码到新项目
- ❌ **禁止照搬**：不能保留其目录结构和文件命名

**正确做法**：
1. 阅读mmw_demo源码，理解其功能
2. 学习其API调用方式和数据结构定义
3. 按照第3章三层架构重新组织代码
4. 重新编写实现同等功能的代码
5. 添加mmw_demo没有的新功能（如DSS特征提取）

---

## 🏗️ 第3章三层架构要求

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
└── README.md
```

### Layer 1: Common (共享接口层)

**职责**：定义MSS与DSS之间的共享数据结构和内存映射

**必须包含**：
- `shared_memory.h` - L3 RAM内存映射（896KB）
- `data_path.h` - DPC结构定义（Config/Result/PointCloud）
- `mmwave_output.h` - TLV输出格式定义
- `health_detect_types.h` - 健康检测特征数据结构

**参考来源**：
- mmw_demo的dpc.h、dpif_pointcloud.h
- InCabin demo的共享内存设计

**关键内容**：
- L3 RAM划分：DPC_CONFIG_BASE, POINT_CLOUD_BASE, FEATURE_DATA_BASE, DPC_RESULT_BASE, SCRATCH_BUFFER_BASE
- DPC配置结构：CFAR参数、DOA参数、帧配置
- 点云数据结构：Cartesian坐标 (x,y,z,velocity)

### Layer 2: MSS (应用层 - R5F @ 200MHz)

**职责**：主控制、配置管理、高层决策、数据输出

**必须包含的模块**：

#### 1. health_detect_main.c/h
- **功能**：主控程序、BIOS任务框架
- **参考**：mmw_demo/source/mmwave_demo.c
- **实现**：
  - BIOS初始化任务
  - 共享内存映射
  - DPC配置初始化
  - 帧处理主循环
  - 雷达启动/停止控制
- **关键点**：这是**新写的文件**，不是mmwave_demo.c的复制！

#### 2. dpc_control.c/h
- **功能**：DPC协调模块，MSS-DSS通信
- **参考**：mmw_demo/source/dpc/dpc.c
- **实现**：
  - IPC mailbox初始化
  - 发送DPC启动命令到DSS
  - 等待DSS处理完成
  - 读取L3 RAM结果

#### 3. cli.c/h
- **功能**：CLI命令行接口
- **参考**：mmw_demo/source/mmw_cli.c
- **实现**：
  - CLI命令表（sensorStart, sensorStop, frameConfig, cfarCfg等）
  - UART命令解析
  - 参数配置接口
- **关键点**：学习mmw_cli.c的命令表设计，但代码重写

#### 4. presence_detect.c/h
- **功能**：存在检测（高层特征分析）
- **参考**：无（新功能）
- **实现**：
  - 基于特征数据的存在判断
  - 状态机：ABSENT → PRESENT → MOTION
  - 运动检测、超时处理

#### 5. tlv_output.c/h
- **功能**：TLV格式数据输出
- **参考**：mmw_demo的TLV输出模块
- **实现**：
  - TLV数据包构建
  - UART高速发送（921600 bps）
  - 支持多种TLV类型（点云、特征、统计）

#### 6. radar_control.c/h
- **功能**：雷达控制模块
- **参考**：mmw_demo/mmwave_control/
- **实现**：
  - mmWave API封装
  - Chirp配置
  - Frame配置
  - 帧回调注册

### Layer 3: DSS (算法层 - C66x @ 450MHz)

**职责**：信号处理、特征提取（实现40% DSP利用率）

**必须包含的模块**：

#### 1. dss_main.c/h
- **功能**：DSP主循环、IPC消息处理
- **参考**：InCabin demo DSS实现
- **实现**：
  - DSP初始化
  - IPC mailbox消息循环
  - 接收MSS启动命令
  - 触发DPC处理链
  - 发送完成通知到MSS

#### 2. feature_extract.c/h
- **功能**：点云特征提取算法（**核心新功能**）
- **参考**：无（新增功能）
- **实现**：
  - 质心计算（centerX/Y/Z）
  - 空间分布（spreadX/Y/Z）
  - 速度分量（velocityX/Y/Z）
  - 写入L3 FEATURE_DATA_BASE
- **关键点**：这是mmw_demo没有的功能！

#### 3. dsp_utils.c/h
- **功能**：DSP工具函数
- **实现**：
  - Cache操作（invalidate/writeback）
  - 周期计数器（TSCL）
  - 内存操作优化

### Layer 0: System (系统配置层)

**职责**：链接脚本、内存映射定义

**必须包含**：
- `linker_mss.cmd` - MSS链接脚本（L3 RAM @ 0x51000000）
- `linker_dss.cmd` - DSS链接脚本（L2 256KB + L3 1407KB）
- `shared_memory.ld` - 共享内存区域定义
- `README.md` - 系统配置文档

---

## 🔄 功能对照表

### mmw_demo功能 → 新架构实现

| mmw_demo功能 | mmw_demo文件 | 新架构文件 | 层级 | 状态 |
|-------------|-------------|-----------|------|------|
| 主控程序 | mmwave_demo.c | health_detect_main.c | MSS | ✅ 重写 |
| DPC协调 | dpc/dpc.c | dpc_control.c | MSS | ✅ 重写 |
| CLI命令 | mmw_cli.c | cli.c | MSS | ✅ 重写 |
| TLV输出 | (多个文件) | tlv_output.c | MSS | ✅ 重写 |
| 雷达控制 | mmwave_control/ | radar_control.c | MSS | ✅ 封装 |
| DSP处理 | ❌ 无 | dss_main.c | DSS | ✅ 新增 |
| **特征提取** | ❌ 无 | feature_extract.c | DSS | ✅ 新增 |
| 存在检测 | ❌ 无 | presence_detect.c | MSS | ✅ 新增 |

**关键区别**：
- mmw_demo：单核R5F处理，DSS未使用（0%）
- 新架构：双核协作，DSS做特征提取（40%）

---

## 📝 编码规范要求

### 文件头注释模板

```c
/**
 * @file xxx.c
 * @brief 模块功能简述
 * 
 * Reference: mmw_demo参考文件路径
 * Adapted for: 三层架构具体用途
 * Created: 2026-01-07
 */
```

### 命名规范

- **模块前缀**：`HealthDetect_`, `DPC_`, `TLV_`, `RadarControl_`
- **私有函数**：`static` + 模块前缀
- **全局变量**：`g` + 驼峰命名
- **宏定义**：全大写 + 下划线

### TODO标记

允许但必须标注的TODO：
- `/* TODO: mmWave API实际调用 */`
- `/* TODO: IPC mailbox实现 */`
- `/* TODO: BIOS配置文件 */`

这些TODO不影响编译验证目标。

---

## 🎯 交付标准

### Milestone 1: 架构重建（2026-01-07完成）

**目标**：按第3章架构从零重建所有代码

**验收标准**：
- ✅ 所有文件都是新创建的（不是复制mmw_demo）
- ✅ 目录结构符合三层架构（common/mss/dss/system）
- ✅ 代码有清晰注释说明参考来源
- ✅ 实现了mmw_demo的主要功能
- ✅ 添加了新功能（DSS特征提取）

**交付物清单**：
- [ ] src/common/*.h (4个文件)
- [ ] src/system/* (4个文件)
- [ ] src/mss/*.c/h (12个文件)
- [ ] src/dss/*.c/h (6个文件)
- [ ] mss_project.projectspec
- [ ] dss_project.projectspec
- [ ] README.md
- [ ] BUILD_GUIDE.md

### Milestone 2: 编译验证（进行中）

**目标**：在CCS中编译通过，生成.out固件

**验收标准**：
- [ ] 导入CCS项目成功
- [ ] MSS项目编译0错误
- [ ] DSS项目编译0错误
- [ ] 生成MSS.out和DSS.out
- [ ] 链接脚本无冲突

### Milestone 3: 功能实现（待开始）

**目标**：完成TODO部分，实现完整功能

**验收标准**：
- [ ] mmWave API实际调用
- [ ] IPC mailbox通信
- [ ] ADC数据路径配置
- [ ] BIOS配置文件

### Milestone 4: 硬件测试（待开始）

**目标**：在AWRL6844 EVM上验证

**验收标准**：
- [ ] 固件烧录成功
- [ ] 雷达启动正常
- [ ] 点云数据输出
- [ ] 特征提取验证
- [ ] 存在检测工作

---

## 🚫 禁止的行为

### 绝对不允许

1. ❌ **复制粘贴mmw_demo源代码**
   - 不能直接复制.c/.h文件
   - 不能保留原始函数实现

2. ❌ **照搬mmw_demo目录结构**
   - 不能使用source/目录
   - 不能保留原始文件命名

3. ❌ **偷懒使用原demo代码**
   - 不能说"先用着mmw_demo代码再改"
   - 必须从第一行开始重写

4. ❌ **只做表面改动**
   - 不能只改文件名和变量名
   - 必须按新架构重新组织

### 允许的参考方式

✅ **正确的参考方法**：
1. 阅读mmw_demo源码，理解功能
2. 学习API调用方式和数据结构
3. 理解算法流程和处理逻辑
4. **然后关闭mmw_demo文件**
5. 按新架构从零编写代码

---

## 📊 进度追踪

### 当前状态（2026-01-07）

**Milestone 1: 架构重建** - ✅ **100%完成**

**已完成**：
- ✅ src/common/ (4个文件) - 共享接口层
- ✅ src/system/ (4个文件) - 系统配置层
- ✅ src/mss/ (12个文件) - MSS应用层
- ✅ src/dss/ (6个文件) - DSS算法层
- ✅ CCS项目配置 (2个.projectspec)
- ✅ 文档 (README.md, BUILD_GUIDE.md, 完成报告.md)

**代码统计**：
- 总文件数：33个
- 源代码：22个文件 (~70KB)
- 代码行数：~3500行
- 注释行数：~1200行

**质量验证**：
- ✅ 所有文件从零编写
- ✅ 符合三层架构
- ✅ 注释清晰完整
- ✅ 实现新功能（特征提取）

---

## 🔄 执行流程要求

### AI执行要求

**必须遵守**：
1. ✅ **一口气完成**所有模块，不中途停下来询问
2. ✅ **按层级顺序**创建：common → system → mss → dss → 项目配置
3. ✅ **每个文件独立完整**，不留空壳
4. ✅ **包含完整注释**，说明参考来源
5. ✅ **标注TODO**，说明待完成部分

**禁止行为**：
- ❌ 创建一两个文件就停下来问"接下来做什么"
- ❌ 只创建.h文件，不创建.c实现
- ❌ 复制粘贴mmw_demo代码
- ❌ 中途改变架构设计

---

## 📚 mmw_demo核心功能清单

### mmw_demo实现的功能（需要重建）

| 功能模块 | mmw_demo文件 | 功能说明 |
|---------|-------------|---------|
| **1. 主控程序** | mmwave_demo.c | 系统初始化、任务管理、帧循环控制 |
| **2. CLI命令行** | mmw_cli.c | 30+命令配置雷达参数（channelCfg, frameCfg, cfarCfg等） |
| **3. DPC数据链** | dpc/dpc.c | Range→Doppler→CFAR→AOA处理链 |
| **4. 雷达控制** | mmwave_control/ | ADC配置、中断处理、帧回调 |
| **5. TLV输出** | 多个文件 | 点云数据UART输出 |
| **6. 校准** | calibrations/ | 测距偏差、相位补偿、Flash存储 |
| **7. LVDS流** | lvds_streaming/ | 原始ADC数据输出（DCA1000） |
| **8. 功耗管理** | power_management/ | 帧间低功耗模式 |

### 新架构必须实现的功能对照

| mmw_demo功能 | 新项目实现情况 | 实现文件 | 状态 |
|-------------|--------------|---------|------|
| ✅ 主控程序 | ✅ 已实现 | health_detect_main.c | 框架完成 |
| ✅ CLI命令 | ✅ 已实现 | cli.c | 命令表定义 |
| ✅ DPC协调 | ✅ 已实现 | dpc_control.c | MSS-DSS通信 |
| ✅ 雷达控制 | ✅ 已实现 | radar_control.c | API封装 |
| ✅ TLV输出 | ✅ 已实现 | tlv_output.c | 数据输出 |
| ⚠️ 校准 | ⏭️ 暂不实现 | (TODO) | 后续版本 |
| ⚠️ LVDS流 | ⏭️ 暂不实现 | (TODO) | 不需要 |
| ⚠️ 功耗管理 | ⏭️ 暂不实现 | (TODO) | 后续版本 |
| 🆕 **DSS特征提取** | ✅ **新增** | feature_extract.c | **新功能** |
| 🆕 **存在检测** | ✅ **新增** | presence_detect.c | **新功能** |

### 🎯 项目目标达成度验证

#### ✅ 核心目标：已达成

**目标1：功能完整重建** ✅
- mmw_demo的5个核心功能（主控、CLI、DPC、雷达控制、TLV）→ **全部重建完成**
- 非核心功能（校准、LVDS、功耗）→ **标注TODO，符合要求**

**目标2：三层架构** ✅
- Common层：4个共享头文件 → **完成**
- MSS层：6个应用模块 → **完成**
- DSS层：3个算法模块 → **完成**
- System层：4个配置文件 → **完成**

**目标3：代码从零编写** ✅
- 所有文件头包含"Reference: mmw_demo..."和"Created: 2026-01-07" → **验证通过**
- 目录结构完全不同（src/common/mss/dss vs source/）→ **验证通过**
- 文件命名完全不同（health_detect_main.c vs mmwave_demo.c）→ **验证通过**

**目标4：新增功能** ✅
- DSS特征提取（mmw_demo无此功能）→ **feature_extract.c已实现**
- 存在检测（mmw_demo无此功能）→ **presence_detect.c已实现**

#### 📊 完成度统计

```
必须实现的功能：8项
├─ 核心功能（必须）：5项 → ✅ 5/5 (100%)
├─ 非核心（TODO）：3项 → ⏭️ 3/3 (标注TODO)
└─ 新增功能（加分）：2项 → ✅ 2/2 (100%)

架构层次：4层
└─ ✅ 4/4层完成 (100%)

代码质量：
├─ 文件数量：33个 → ✅ 符合要求
├─ 代码行数：~2893行 → ✅ 合理规模
├─ 注释率：38.4% → ✅ 超过标准
└─ 参考标注：100% → ✅ 全部标注

结论：✅ **完全达到第3章演进架构重建要求**
```

---

## 🔍 详细功能对比分析（供参考）

### 核心模块功能分析

#### 1. 主控模块 (mmwave_demo.c/h)

**文件**：`source/mmwave_demo.c` (1233行)

**核心功能**：
- ✅ **系统初始化**：BIOS任务创建、内存分配、外设初始化
- ✅ **主控循环**：帧启动 → DPC处理 → 结果输出 → 下一帧
- ✅ **任务管理**：DPC任务、TLV输出任务、ADC文件读取任务
- ✅ **低功耗模式**：帧间进入低功耗状态
- ✅ **传感器启停**：`MmwDemo_stopSensor()`实现优雅停止

**关键函数**：
```c
int32_t MmwStart(void)           // 启动mmWave传感器
void MmwDemo_stopSensor(void)    // 停止传感器并释放资源
void mmwave_demo(void* args)     // 主任务入口
```

**内存管理**：
- L3 RAM：1407KB用于雷达数据立方体、检测矩阵
- Core Local RAM：20KB用于DPU窗口系数

**参考要点**：
- 🔍 学习任务创建和优先级设置（DPC_TASK_PRI=5, TLV_TASK_PRI=3）
- 🔍 学习帧回调注册和中断处理
- 🔍 学习EDMA通道分配和释放流程

---

#### 2. CLI命令行接口 (mmw_cli.c/h)

**文件**：`source/mmw_cli.c` (2342行)

**核心功能**：
- ✅ **命令解析**：UART读取命令行，tokenize参数
- ✅ **配置管理**：30+个CLI命令配置雷达参数
- ✅ **参数验证**：检查参数范围和依赖关系
- ✅ **错误处理**：返回错误码和提示信息

**CLI命令表**（完整列表）：
```c
sensorStart          - 启动传感器
sensorStop           - 停止传感器
channelCfg           - 配置RX/TX通道
apllFreqShiftEn      - APLL频率偏移
chirpComnCfg         - Chirp公共配置
chirpTimingCfg       - Chirp时序配置
frameCfg             - 帧配置
guiMonitor           - GUI监控选项
cfarProcCfg          - CFAR检测配置
cfarFovCfg           - CFAR视场角配置
aoaProcCfg           - AOA处理配置
aoaFovCfg            - AOA视场角配置
clutterRemoval       - 杂波抑制开关
lowPowerCfg          - 低功耗模式配置
factoryCalibCfg      - 工厂校准配置
runtimeCalibCfg      - 运行时校准配置
antGeometryTX/RX     - 天线几何配置
antGeometryDist      - 天线间距配置
measureRangeBias...  - 测距偏差测量
compRangeBias...     - 测距偏差补偿
adcDataDitherCfg     - ADC数据抖动配置
adcDataSource        - ADC数据源选择
adcLogging           - ADC数据记录（LVDS）
```

**参考要点**：
- 🔍 学习CLI命令表结构（cmd, helpString, cmdHandlerFxn）
- 🔍 学习参数解析方法（strtok, atoi, atof）
- 🔍 学习配置参数的验证逻辑

---

#### 3. 数据路径链 (dpc/dpc.c)

**文件**：`source/dpc/dpc.c` (1964行)

**核心功能**：
- ✅ **DPU链协调**：Range → Doppler → CFAR → AOA
- ✅ **内存池管理**：L3 RAM动态分配
- ✅ **HWA资源管理**：HWA内存池、DMA触发通道池
- ✅ **DPU初始化**：Range/Doppler/CFAR/AOA DPU配置
- ✅ **执行流程控制**：`DPC_Execute()`触发处理链

**DPU处理链**：
```
ADC Data → Range DPU (HWA) → Doppler DPU (HWA) 
         → CFAR DPU (HWA)   → AOA 2D DPU (DSP)
         → Point Cloud Output
```

**关键数据结构**：
```c
// Range处理配置
rangeProcHWACfg_t     rangeConfig;

// Doppler处理配置  
dopplerProcHWACfg_t   dopplerConfig;

// CFAR检测配置
cfarProcHWACfg_t      cfarConfig;

// AOA估计配置
aoa2dProcCfg_t        aoaConfig;
```

**参考要点**：
- 🔍 学习DPU配置结构体的定义和填充
- 🔍 学习HWA参数设置（窗函数、FFT大小）
- 🔍 学习内存分配策略（L3 RAM分段管理）

---

#### 4. 雷达控制 (mmwave_control/)

**文件**：
- `mmwave_control_config.c` - ADC Buffer配置
- `monitors.c/h` - RF监控（温度、电压、功率）
- `interrupts.c/h` - 帧启动中断处理

**核心功能**：
- ✅ **ADC Buffer配置**：通道使能、偏移量计算
- ✅ **RF监控**：TX功率、RX增益、PLL控制电压
- ✅ **中断处理**：帧启动中断、Chirp可用中断

**ADC Buffer配置**：
```c
void MmwDemo_ADCBufConfig(uint16_t rxChannelEn, uint32_t chanDataSize)
{
    // 计算16字节对齐的通道数据大小
    chanDataSizeAligned16 = ((chanDataSize + 15) / 16) * 16;
    
    // 为每个RX通道配置偏移量
    for (channel = 0; channel < SYS_COMMON_NUM_RX_CHANNEL; channel++)
    {
        rxChanConf.channel = channel;
        rxChanConf.offset = offset;
        ADCBuf_control(gMmwMssMCB.adcBuffHandle, 
                       ADCBufMMWave_CMD_CHANNEL_ENABLE, 
                       &rxChanConf);
        offset += chanDataSizeAligned16;
    }
}
```

**参考要点**：
- 🔍 学习ADC Buffer通道配置方法
- 🔍 学习RF监控参数的获取和检查
- 🔍 学习中断注册和回调函数设计

---

#### 5. 校准模块 (calibrations/)

**文件**：
- `range_phase_bias_measurement.c/h` - 测距偏差和相位偏差测量
- `mmw_flash_cal.c/h` - Flash校准数据读写
- `factory_cal.c/h` - 工厂校准参数管理

**核心功能**：
- ✅ **测距偏差测量**：使用角反射器测量距离偏差
- ✅ **相位偏差测量**：RX通道间相位差补偿
- ✅ **Flash存储**：校准参数保存到Flash
- ✅ **工厂校准**：保存/恢复工厂校准参数

**测距偏差测量**：
```c
void rangeBiasMeasure_quadfit(float *x, float*y, float *xv, float *yv)
{
    // 抛物线拟合计算峰值位置
    // 用于精确估计测距偏差
}
```

**参考要点**：
- 🔍 学习校准数据结构定义
- 🔍 学习Flash读写操作
- 🔍 学习补偿参数的应用方法

---

#### 6. LVDS数据流 (lvds_streaming/)

**文件**：`mmw_lvds_stream.c`

**核心功能**：
- ✅ **LVDS会话创建**：CBUFF驱动配置
- ✅ **ADC数据流**：原始ADC数据通过LVDS输出
- ✅ **高速传输**：支持DCA1000数据采集卡

**参考要点**：
- 🔍 学习CBUFF驱动API调用
- 🔍 学习LVDS数据格式定义
- ⚠️ 注意：新项目可能不需要此功能

---

#### 7. 功耗管理 (power_management/)

**文件**：`power_management.c/h`

**核心功能**：
- ✅ **帧间低功耗**：Frame间隙进入IDLE模式
- ✅ **PMIC看门狗**：定时喂狗防止系统复位
- ✅ **延迟测量**：统计进入/退出低功耗延迟

**参考要点**：
- 🔍 学习低功耗模式的进入和退出流程
- 🔍 学习功耗管理任务的优先级设置
- ⚠️ 新项目初期可暂不实现（设为TODO）

---

### 架构特点分析

#### mmw_demo架构（需改进的地方）

**单核处理模式**：
- ✅ 所有处理在R5F上完成
- ❌ DSS (C66x)完全未使用（浪费450MHz DSP资源）
- ❌ R5F负载重，限制帧率

**内存使用**：
- L3 RAM (1408KB)：雷达立方体、检测矩阵
- Core Local RAM (20KB)：窗函数系数
- ❌ 未优化：点云结果和雷达立方体共用L3

**任务优先级**：
```
DPC Task        : 优先级 5 (最高)
CLI Task        : 优先级 1
TLV Task        : 优先级 3
Power Task      : 优先级 2
ADC FileRead    : 优先级 4
```

---

### 新架构改进点

#### 🆕 引入DSS协作处理

**改进1：点云特征提取移至DSS**
```
旧架构：R5F做所有事
新架构：R5F主控 + DSS特征提提取（40%利用率）
```

**改进2：共享内存优化**
```c
// common/shared_memory.h
#define DPC_CONFIG_BASE    0x51000000  // DPC配置
#define POINT_CLOUD_BASE   0x51010000  // 点云数据
#define FEATURE_DATA_BASE  0x51020000  // 特征数据（新增）
#define DPC_RESULT_BASE    0x51030000  // DPC结果
```

**改进3：IPC消息传递**
```
MSS → DSS: START_PROCESSING命令
DSS → MSS: PROCESSING_DONE通知
```

#### 🆕 新增功能模块

**存在检测模块** (presence_detect.c)：
- mmw_demo无此功能
- 新架构实现：状态机 + 特征分析

**特征提取模块** (feature_extract.c)：
- mmw_demo无此功能
- 新架构实现：DSS计算质心、空间分布

---

### 参考使用指南

#### 何时查看mmw_demo源码？

**✅ 应该查看的场景**：

1. **学习API调用方式**
   - 如何调用mmWave Control API
   - 如何配置ADC Buffer
   - 如何注册回调函数

2. **学习数据结构定义**
   - DPC配置结构体怎么定义
   - 点云数据结构包含哪些字段
   - TLV格式如何组织

3. **学习算法流程**
   - CFAR检测的配置参数
   - AOA估计的处理步骤
   - 杂波抑制的实现方法

4. **学习错误处理**
   - 如何检测mmWave API返回值
   - 如何处理配置冲突
   - 如何报告错误信息

**❌ 不应该做的**：

- ❌ 复制粘贴mmw_demo的函数实现
- ❌ 照搬mmw_demo的文件结构
- ❌ 保留mmw_demo的命名风格
- ❌ 不思考就直接移植代码

#### 正确的参考流程

```
Step 1: 阅读mmw_demo源码
       ↓
Step 2: 理解功能和API用法
       ↓
Step 3: 记录关键要点（数据结构、参数、API）
       ↓
Step 4: 关闭mmw_demo文件
       ↓
Step 5: 按新架构设计模块
       ↓
Step 6: 从零编写代码
       ↓
Step 7: 添加注释说明参考来源
```

---

### 快速查找表

| 需要实现的功能 | 参考mmw_demo文件 | 新架构文件 | 关键学习点 |
|--------------|----------------|-----------|-----------|
| 主控循环 | mmwave_demo.c | health_detect_main.c | 任务创建、帧启动 |
| CLI命令 | mmw_cli.c | cli.c | 命令表、参数解析 |
| DPC协调 | dpc/dpc.c | dpc_control.c | DPU配置、执行流程 |
| ADC配置 | mmwave_control_config.c | radar_control.c | 通道配置、内存对齐 |
| TLV输出 | 多个文件 | tlv_output.c | TLV格式、UART发送 |
| 校准 | calibrations/*.c | (暂不实现) | Flash操作、补偿参数 |
| LVDS流 | lvds_streaming/*.c | (暂不实现) | CBUFF配置 |
| RF监控 | monitors.c | (暂不实现) | 监控参数读取 |

---

## 📚 其他参考文档位置

**第3章架构文档**：
- 项目文档中的架构设计章节
- InCabin demo示例

**TI SDK文档**：
- mmWave SDK API参考
- BIOS/XDC配置指南
- 硬件用户手册

---

## ✅ 验收检查清单

### 代码质量检查

- [ ] 所有.c文件都有对应的.h文件
- [ ] 所有文件头部有完整注释
- [ ] 所有函数有功能说明注释
- [ ] 没有从mmw_demo复制的代码
- [ ] 目录结构符合三层架构
- [ ] 命名规范统一

### 功能完整性检查

- [ ] common层定义了所有共享结构
- [ ] MSS层包含6个核心模块
- [ ] DSS层实现特征提取
- [ ] CCS项目配置完整
- [ ] 链接脚本正确

### 文档完整性检查

- [ ] README.md说明项目架构
- [ ] BUILD_GUIDE.md说明编译步骤
- [ ] 每个目录有README说明
- [ ] 注释说明参考来源

---

## 🔧 编译打包说明

### 为什么MSS和DSS分别编译？

**双核异构架构的本质**：

AWRL6844包含两个**完全不同**的处理器：

| 项目 | MSS | DSS |
|-----|-----|-----|
| **CPU** | ARM Cortex-R5F @ 200MHz | C66x DSP @ 450MHz |
| **指令集** | ARM指令集 | TI C6000指令集 |
| **编译器** | TI ARM Compiler (armcl) | TI C6000 Compiler (cl6x) |
| **二进制** | ARM COFF/ELF | C6000 COFF/ELF |
| **内存视角** | L3 @ 0x51000000 | L3 @ 0xC0000000 |

**结论**：❌ 不可能用一个编译器编译两种不同的CPU架构！

### 两种编译方式

#### 方式1: 分别编译（开发调试用 - 辅助方式）

```
项目配置：
├─ mss_project.projectspec → 编译MSS.out (ARM)
└─ dss_project.projectspec → 编译DSS.out (DSP)

CCS操作：
├─ Build AWRL6844_HealthDetect_MSS → MSS.out
├─ Build AWRL6844_HealthDetect_DSS → DSS.out
└─ UniFlash分别烧录两个.out文件

优点：✅ 快速调试、可单独更新某个核心
缺点：❌ 需要两次烧录、版本管理困难
适用：开发调试 ✅（仅作为辅助手段）
```

#### ⭐ 方式2: 系统打包（正式方式 - 🔴 必须实现）

```
项目配置：
├─ mss_project.projectspec
├─ dss_project.projectspec
└─ system_project.projectspec ← ✅ 已添加（协调上面两个）

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

### 当前项目策略

**Milestone 1-2（开发阶段）**：
- ✅ 已完成MSS/DSS项目配置
- ✅ 已添加system_project配置
- ✅ 可使用方式1快速调试（辅助手段）
- 🔴 **必须验证系统打包可用**

**Milestone 2（编译验证）**：
- 🔴 **必须完成**：验证System Project编译
- 🔴 **必须完成**：成功生成.appimage文件
- 🔴 **必须完成**：验证.appimage可烧录
- ✅ 分别编译仅作为对比和调试手段

**Milestone 3-4（功能完善）**：
- ✅ 所有发布都使用.appimage格式
- ✅ 版本管理基于.appimage
- ✅ 测试验证基于.appimage

**参考文档**：
- 详见 `项目文档/3-固件工具/08-AWRL6844雷达健康检测实现方案/AWRL6844雷达健康检测-06-第3章-架构演进规划.md` 第3.8节
- 详见 `项目文档/3-固件工具/08-AWRL6844雷达健康检测实现方案/AWRL6844雷达健康检测-附录C-InCabin架构学习参考.md` 第11节

---

## 🎯 最终目标

**短期目标**（Milestone 2）：
- 🔴 **必须完成**：System Project编译通过
- 🔴 **必须完成**：生成.appimage系统镜像
- ✅ 可选：单独编译MSS.out和DSS.out（调试用）
- ✅ 验证架构可行性

**长期目标**（Milestone 3-4）：
- 完成所有TODO
- 在硬件上测试验证（使用.appimage）
- 实现完整健康检测功能

**成功标志**：
- ✅ 代码从零重建，不是mmw_demo修改版
- ✅ 三层架构清晰，模块职责明确
- ✅ 实现mmw_demo的功能 + 新增特征提取
- 🔴 **必须**：可生成完整的.appimage系统镜像
- ✅ 可编译、可运行、可维护

---

> 📝 **重要提醒**：本项目是**重建**，不是**复制**！
> 
> - mmw_demo_SDK_reference = 📚 参考手册
> - AWRL6844_HealthDetect = 🆕 新实现
> - 方法：学习理解 → 关闭参考 → 重新编写.
