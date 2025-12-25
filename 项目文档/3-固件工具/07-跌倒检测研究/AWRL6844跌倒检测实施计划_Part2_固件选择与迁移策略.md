# AWRL6844跌倒检测实施计划 - Part2: 固件选择与迁移策略

**创建日期**: 2025-12-25  
**前置要求**: Part1环境准备已完成  
**本章目标**: 选择合适的基础固件并制定迁移策略  

---

## 📋 Part2 概览

本部分内容：
1. 固件方案选择与对比
2. 基于3D People Tracking的迁移方案
3. 从零开始的开发方案
4. 推荐方案与实施步骤
5. 代码结构设计

---

## 1. 固件方案选择

### 1.1 可用方案对比

#### 方案A: 基于TI 3D People Tracking Demo ⭐ 推荐

**位置**：
```
C:\ti\radar_toolbox_3_30_00_06\
└── source\ti\examples\
    └── 3D_people_tracking\
        └── 68xx_3D_people_tracking\
```

**优势**：
```
✅ TI官方维护，代码质量高
✅ 已实现完整的3D人员追踪
✅ 包含点云聚类、目标追踪
✅ 支持AWRL6844硬件
✅ 有详细的实现文档
✅ 包含可视化工具
✅ CLI配置完善
```

**劣势**：
```
❌ 代码复杂度较高（需要学习）
❌ 没有直接的跌倒检测功能
❌ 需要添加跌倒检测逻辑
```

**适用场景**：
- ✅ 有一定C语言基础
- ✅ 希望快速原型开发
- ✅ 需要稳定可靠的基础
- ✅ 可以基于现有代码扩展

#### 方案B: 基于mmWave Demo

**位置**：
```
C:\ti\mmwave_sdk_xxx\
└── examples\
    └── mmwave_demo\
        └── xwr68xx\
```

**优势**：
```
✅ SDK基础示例，结构简单
✅ 易于理解和修改
✅ 包含完整的CLI框架
✅ 文档完善
```

**劣势**：
```
❌ 功能较基础，需要大量开发
❌ 没有人员追踪功能
❌ 需要自己实现检测和追踪
❌ 开发周期长
```

**适用场景**：
- ✅ 需要完全自定义功能
- ✅ 有充足开发时间
- ✅ 熟悉雷达算法开发

#### 方案C: 从零开始开发

**优势**：
```
✅ 完全自主控制
✅ 代码精简
✅ 针对性优化
```

**劣势**：
```
❌ 开发周期长（数月）
❌ 需要深厚技术积累
❌ 调试困难
❌ 不推荐
```

**适用场景**：
- ⚠️ 不推荐用于本项目

### 1.2 方案选择建议

**推荐方案**：**方案A - 基于3D People Tracking Demo** ⭐

**理由**：
1. ✅ **已有完整的人员追踪**：跌倒检测依赖于人员追踪
2. ✅ **代码质量高**：TI官方维护，bug少
3. ✅ **开发效率高**：只需添加跌倒检测逻辑
4. ✅ **文档完善**：有实施指南和调优文档
5. ✅ **社区支持**：E2E论坛有大量讨论

**开发工作量估算**：
```
基于3D People Tracking：
├─ 学习现有代码：3-5天
├─ 添加跌倒检测：5-7天
├─ 调试优化：5-7天
└─ 总计：2-3周

从零开始：
├─ 信号处理链：2周
├─ 目标追踪：2周
├─ 跌倒检测：1周
├─ 调试优化：2周
└─ 总计：7-9周
```

---

## 2. 3D People Tracking Demo 架构分析

### 2.1 目录结构

```
68xx_3D_people_tracking/
├── src/                          # 源代码
│   ├── mss/                      # MSS (R5F) 代码
│   │   ├── mss_main.c           # 主程序入口
│   │   ├── cli.c                # CLI命令解析
│   │   ├── config.c             # 配置管理
│   │   └── tracker_utils.c      # 追踪工具函数
│   │
│   └── dss/                      # DSS (C66x) 代码
│       ├── dss_main.c           # DSP主程序
│       ├── dss_data_path.c      # 数据处理链
│       ├── objectdetection.c    # 目标检测
│       └── tracker/              # 追踪算法
│           ├── EKF_XYZ_Interface.c
│           ├── gtrack.c
│           └── gtrack_utilities.c
│
├── docs/                         # 文档
│   ├── 3D_people_tracking_demo_implementation_guide.pdf
│   ├── 3D_people_tracking_detection_layer_tuning_guide.pdf
│   └── 3D_people_tracking_tracker_layer_tuning_guide.pdf
│
├── matlab/                       # MATLAB工具
│   └── parse_mmw_demo_output.m
│
├── gui/                          # 可视化工具
│   └── mmwave_demo_visualizer.exe
│
└── profiles/                     # 配置文件
    ├── profile_3d_people_tracking.cfg
    └── profile_3d_people_tracking_pcount.cfg
```

### 2.2 软件架构

#### 整体架构

```
┌─────────────────────────────────────────────────┐
│                   CLI Commands                   │  ← 用户配置
│         (UART, 115200 baud, COM_A)              │
└────────────────┬────────────────────────────────┘
                 ↓
┌─────────────────────────────────────────────────┐
│              MSS (R5F Core)                     │
│  ┌──────────────────────────────────────────┐  │
│  │  • CLI解析                                │  │
│  │  • 配置管理                              │  │
│  │  • UART数据输出                           │  │
│  │  • 系统控制                              │  │
│  └──────────────────────────────────────────┘  │
└────────────────┬────────────────────────────────┘
                 ↓ (Mailbox通信)
┌─────────────────────────────────────────────────┐
│              DSS (C66x DSP)                     │
│  ┌──────────────────────────────────────────┐  │
│  │  数据处理链：                             │  │
│  │  1. ADC数据采集                          │  │
│  │  2. Range FFT                            │  │
│  │  3. Doppler FFT                          │  │
│  │  4. CFAR检测                             │  │
│  │  5. Angle FFT (Azimuth/Elevation)       │  │
│  │  6. 点云聚类                             │  │
│  │  7. 目标追踪 (GTRACK)                    │  │
│  └──────────────────────────────────────────┘  │
└────────────────┬────────────────────────────────┘
                 ↓
┌─────────────────────────────────────────────────┐
│         Output Data (UART, COM_B)               │
│  • 目标列表 (ID, 位置, 速度)                     │
│  • 点云数据 (Range, Angle, Doppler)            │
│  • 目标索引                                     │
└─────────────────────────────────────────────────┘
```

#### 数据处理流程

```
ADC数据 (时域)
    ↓
Range FFT (1D FFT)
    ↓
Range-Doppler Map (2D FFT)
    ↓
CFAR检测 (CA-CFAR)
    ↓
Detected Points (Range, Doppler, Antenna)
    ↓
Angle Estimation (Azimuth + Elevation)
    ↓
3D Point Cloud (X, Y, Z, Doppler)
    ↓
Point Cloud Clustering
    ↓
Cluster Centroids
    ↓
GTRACK Tracker (EKF-based)
    ↓
Tracked Targets (ID, Position, Velocity)
    ↓
🎯 [跌倒检测模块] ← 在这里添加
    ↓
Output (UART)
```

### 2.3 关键模块分析

#### 模块1: 目标检测层 (Detection Layer)

**文件**: `dss/objectdetection.c`

**功能**：
- Range FFT
- Doppler FFT
- CFAR检测
- Angle FFT
- 点云生成

**输出**：
```c
typedef struct MmwDemo_detectedObj_t {
    uint16_t rangeIdx;      // Range bin索引
    uint16_t dopplerIdx;    // Doppler bin索引
    uint16_t peakVal;       // 峰值
    int16_t  x;             // X坐标 (mm)
    int16_t  y;             // Y坐标 (mm)
    int16_t  z;             // Z坐标 (mm)
} MmwDemo_detectedObj;
```

#### 模块2: 目标追踪层 (Tracking Layer)

**文件**: `dss/tracker/gtrack.c`

**功能**：
- 扩展卡尔曼滤波 (EKF)
- 数据关联
- 目标管理（创建、更新、删除）
- 轨迹预测

**输出**：
```c
typedef struct GTRACK_targetDesc_t {
    uint32_t uid;           // 目标唯一ID
    float    S[6];          // 状态向量 [x, y, z, vx, vy, vz]
    float    EC[9];         // 误差协方差矩阵
    float    G;             // 增益
    uint8_t  state;         // 目标状态
} GTRACK_targetDesc;
```

#### 模块3: 输出格式

**UART输出结构**：
```c
// TLV (Type-Length-Value) 格式
typedef struct {
    uint32_t type;          // 数据类型
    uint32_t length;        // 数据长度
    // 数据内容
} MmwDemo_output_message_tlv;

// 支持的TLV类型：
#define MMWDEMO_OUTPUT_MSG_DETECTED_POINTS      1
#define MMWDEMO_OUTPUT_MSG_RANGE_PROFILE        2
#define MMWDEMO_OUTPUT_MSG_NOISE_PROFILE        3
#define MMWDEMO_OUTPUT_MSG_AZIMUT_STATIC_HEAT_MAP  4
#define MMWDEMO_OUTPUT_MSG_RANGE_DOPPLER_HEAT_MAP  5
#define MMWDEMO_OUTPUT_MSG_STATS                6
#define MMWDEMO_OUTPUT_MSG_DETECTED_POINTS_SIDE_INFO  7
#define MMWDEMO_OUTPUT_MSG_SPHERICAL_POINTS     8
#define MMWDEMO_OUTPUT_MSG_TRACKERPROC_TARGET_LIST  9
#define MMWDEMO_OUTPUT_MSG_TRACKERPROC_TARGET_INDEX 10
```

---

## 3. 跌倒检测集成方案

### 3.1 添加跌倒检测的位置

**推荐位置**：在GTRACK输出后添加跌倒检测模块

```
GTRACK Tracker
    ↓
Tracked Targets (ID, Position, Velocity)
    ↓
┌─────────────────────────────────────┐
│  🎯 Fall Detection Module (新增)    │
│                                     │
│  Input:                             │
│  • Tracked targets                  │
│  • Historical trajectory            │
│                                     │
│  Process:                           │
│  • Height change detection          │
│  • Velocity threshold check         │
│  • Posture analysis                 │
│  • Stationary detection             │
│                                     │
│  Output:                            │
│  • Fall event flag                  │
│  • Fall confidence score            │
│  • Target ID with fall              │
└─────────────────────────────────────┘
    ↓
Output (UART + Fall Alert)
```

### 3.2 跌倒检测算法设计

#### 算法流程

```
For each tracked target:
    
    1. 获取目标历史轨迹
       ├─ 最近N帧的位置 (x, y, z)
       ├─ 最近N帧的速度 (vx, vy, vz)
       └─ 时间戳
    
    2. 高度变化检测
       ├─ 计算高度差: Δz = z_current - z_prev
       ├─ 判断: Δz < -HEIGHT_THRESHOLD (如 -0.5m)
       └─ 时间窗口: < TIME_WINDOW (如 0.5s)
    
    3. 垂直速度检测
       ├─ 计算垂直速度: vz
       ├─ 判断: vz < -VELOCITY_THRESHOLD (如 -1.5 m/s)
       └─ 持续时间检查
    
    4. 姿态分析
       ├─ 最终高度检查: z_final < HEIGHT_GROUND (如 0.5m)
       ├─ 与初始高度对比: z_final < z_initial * 0.3
       └─ 排除蹲坐动作（速度和时间特征不同）
    
    5. 静止检测
       ├─ 检测跌倒后速度: sqrt(vx² + vy² + vz²)
       ├─ 判断: velocity < STATIONARY_THRESHOLD (如 0.2 m/s)
       ├─ 持续时间: > STATIONARY_TIME (如 3s)
       └─ 确认跌倒事件
    
    6. 输出决策
       ├─ 如果所有条件满足 → Fall Detected
       ├─ 输出目标ID
       ├─ 输出置信度分数
       └─ 触发报警
```

#### 关键参数

```c
// 跌倒检测参数（可通过CLI配置）
typedef struct {
    float heightThreshold;      // 高度变化阈值 (m)，如 -0.5
    float velocityThreshold;    // 垂直速度阈值 (m/s)，如 -1.5
    float groundHeight;         // 地面高度 (m)，如 0.5
    float stationaryVelocity;   // 静止速度阈值 (m/s)，如 0.2
    float stationaryTime;       // 静止持续时间 (s)，如 3.0
    uint32_t historyFrames;     // 历史轨迹帧数，如 30
    float confidenceThreshold;  // 置信度阈值，如 0.8
} FallDetectionParams;
```

### 3.3 代码结构设计

#### 新增文件结构

```
src/dss/
├── falldetection/               # 跌倒检测模块（新增）
│   ├── falldetection.c         # 跌倒检测主逻辑
│   ├── falldetection.h         # 接口定义
│   ├── trajectory_history.c    # 轨迹历史管理
│   └── trajectory_history.h
│
├── dss_main.c                  # 修改：集成跌倒检测
└── dss_data_path.c             # 修改：调用跌倒检测
```

#### 数据结构设计

**1. 目标轨迹历史**

```c
#define MAX_HISTORY_FRAMES 50

// 单帧历史记录
typedef struct {
    float x, y, z;              // 位置 (m)
    float vx, vy, vz;           // 速度 (m/s)
    uint64_t timestamp;         // 时间戳 (ms)
    uint8_t valid;              // 数据有效标志
} TrajectoryPoint;

// 目标轨迹历史
typedef struct {
    uint32_t targetId;          // 目标ID
    TrajectoryPoint history[MAX_HISTORY_FRAMES];
    uint32_t headIdx;           // 环形缓冲区头指针
    uint32_t count;             // 有效数据点数量
} TargetTrajectory;

// 轨迹历史管理器
typedef struct {
    TargetTrajectory targets[GTRACK_MAX_TARGETS];
    uint32_t numTargets;
} TrajectoryManager;
```

**2. 跌倒检测状态**

```c
// 跌倒检测状态
typedef enum {
    FALL_STATE_NORMAL = 0,      // 正常状态
    FALL_STATE_FALLING,         // 正在跌倒
    FALL_STATE_FALLEN,          // 已跌倒
    FALL_STATE_RECOVERY         // 恢复中
} FallState;

// 单个目标的跌倒检测结果
typedef struct {
    uint32_t targetId;          // 目标ID
    FallState state;            // 当前状态
    float confidence;           // 置信度 [0-1]
    uint64_t fallTimestamp;     // 跌倒时间戳
    uint32_t stationaryFrames;  // 静止帧数
    float maxFallHeight;        // 最大下降高度
    float maxFallVelocity;      // 最大下降速度
} FallDetectionResult;

// 跌倒检测输出
typedef struct {
    FallDetectionResult results[GTRACK_MAX_TARGETS];
    uint32_t numResults;
    uint32_t fallEventCount;    // 跌倒事件计数
} FallDetectionOutput;
```

**3. 跌倒检测句柄**

```c
// 跌倒检测模块句柄
typedef struct {
    FallDetectionParams params;         // 配置参数
    TrajectoryManager trajectoryMgr;    // 轨迹管理器
    FallDetectionOutput output;         // 输出结果
    uint32_t frameCount;                // 帧计数器
} FallDetectionHandle;
```

### 3.4 核心函数接口

#### 初始化与配置

```c
/**
 * @brief 初始化跌倒检测模块
 * @param handle 跌倒检测句柄
 * @param params 配置参数
 * @return 0成功，其他失败
 */
int32_t FallDetection_init(FallDetectionHandle *handle, 
                           FallDetectionParams *params);

/**
 * @brief 配置跌倒检测参数（CLI命令）
 * @param handle 跌倒检测句柄
 * @param params 新的配置参数
 * @return 0成功，其他失败
 */
int32_t FallDetection_config(FallDetectionHandle *handle, 
                             FallDetectionParams *params);
```

#### 主处理函数

```c
/**
 * @brief 处理一帧追踪结果，检测跌倒事件
 * @param handle 跌倒检测句柄
 * @param targets 追踪目标列表（GTRACK输出）
 * @param numTargets 目标数量
 * @param timestamp 当前时间戳
 * @return 0成功，其他失败
 */
int32_t FallDetection_process(FallDetectionHandle *handle,
                              GTRACK_targetDesc *targets,
                              uint32_t numTargets,
                              uint64_t timestamp);

/**
 * @brief 获取检测结果
 * @param handle 跌倒检测句柄
 * @param output 输出结果指针
 * @return 0成功，其他失败
 */
int32_t FallDetection_getResults(FallDetectionHandle *handle,
                                 FallDetectionOutput **output);
```

#### 轨迹管理函数

```c
/**
 * @brief 更新目标轨迹历史
 * @param trajectoryMgr 轨迹管理器
 * @param targetId 目标ID
 * @param x, y, z 位置
 * @param vx, vy, vz 速度
 * @param timestamp 时间戳
 * @return 0成功，其他失败
 */
int32_t TrajectoryManager_update(TrajectoryManager *mgr,
                                 uint32_t targetId,
                                 float x, float y, float z,
                                 float vx, float vy, float vz,
                                 uint64_t timestamp);

/**
 * @brief 获取目标轨迹历史
 * @param trajectoryMgr 轨迹管理器
 * @param targetId 目标ID
 * @param trajectory 输出轨迹指针
 * @return 0成功，其他失败
 */
int32_t TrajectoryManager_getTrajectory(TrajectoryManager *mgr,
                                        uint32_t targetId,
                                        TargetTrajectory **trajectory);
```

#### 跌倒检测核心算法

```c
/**
 * @brief 检测高度快速下降
 * @param trajectory 目标轨迹
 * @param params 检测参数
 * @return true表示检测到，false表示未检测到
 */
bool FallDetection_detectHeightDrop(TargetTrajectory *trajectory,
                                    FallDetectionParams *params);

/**
 * @brief 检测垂直速度阈值
 * @param trajectory 目标轨迹
 * @param params 检测参数
 * @return true表示检测到，false表示未检测到
 */
bool FallDetection_detectVerticalVelocity(TargetTrajectory *trajectory,
                                          FallDetectionParams *params);

/**
 * @brief 检测目标静止状态
 * @param trajectory 目标轨迹
 * @param params 检测参数
 * @return true表示静止，false表示运动
 */
bool FallDetection_detectStationary(TargetTrajectory *trajectory,
                                    FallDetectionParams *params);

/**
 * @brief 计算跌倒置信度
 * @param trajectory 目标轨迹
 * @param params 检测参数
 * @return 置信度分数 [0-1]
 */
float FallDetection_calculateConfidence(TargetTrajectory *trajectory,
                                        FallDetectionParams *params);
```

---

## 4. CLI命令扩展

### 4.1 新增CLI命令

需要添加以下CLI命令用于配置跌倒检测参数：

```c
// 跌倒检测配置命令
fallDetectionCfg <enable> <heightThreshold> <velocityThreshold> 
                 <groundHeight> <stationaryVelocity> <stationaryTime>

// 示例：
fallDetectionCfg 1 -0.5 -1.5 0.5 0.2 3.0

// 参数说明：
// enable: 使能跌倒检测 (0=禁用, 1=启用)
// heightThreshold: 高度变化阈值 (m)，负值表示下降
// velocityThreshold: 垂直速度阈值 (m/s)，负值表示向下
// groundHeight: 地面高度阈值 (m)
// stationaryVelocity: 静止速度阈值 (m/s)
// stationaryTime: 静止持续时间 (s)
```

### 4.2 CLI解析实现

在 `mss/cli.c` 中添加：

```c
// CLI命令表中添加
static MmwDemo_CLICmd gMmwDemoCLICmdTable[] = {
    // ... 现有命令
    {
        "fallDetectionCfg",
        MmwDemo_CLIFallDetectionCfg,
        "Fall detection configuration"
    },
};

// CLI命令处理函数
static int32_t MmwDemo_CLIFallDetectionCfg(int32_t argc, char* argv[])
{
    FallDetectionParams params;
    
    if (argc != 7) {
        CLI_write("Error: Invalid number of arguments\n");
        return -1;
    }
    
    // 解析参数
    params.enable = (uint8_t)atoi(argv[1]);
    params.heightThreshold = (float)atof(argv[2]);
    params.velocityThreshold = (float)atof(argv[3]);
    params.groundHeight = (float)atof(argv[4]);
    params.stationaryVelocity = (float)atof(argv[5]);
    params.stationaryTime = (float)atof(argv[6]);
    
    // 配置跌倒检测模块
    if (FallDetection_config(&gFallDetectionHandle, &params) == 0) {
        CLI_write("Fall detection configured successfully\n");
        return 0;
    } else {
        CLI_write("Error: Failed to configure fall detection\n");
        return -1;
    }
}
```

---

## 5. 输出格式扩展

### 5.1 新增TLV类型

```c
// 在 mmw_output.h 中添加新的TLV类型
#define MMWDEMO_OUTPUT_MSG_FALL_DETECTED    11
#define MMWDEMO_OUTPUT_MSG_FALL_TARGET_INFO 12
```

### 5.2 跌倒检测输出结构

```c
// 跌倒事件TLV
typedef struct {
    uint32_t targetId;          // 目标ID
    float confidence;           // 置信度
    uint64_t timestamp;         // 跌倒时间戳
    float x, y, z;              // 跌倒位置
    float fallHeight;           // 下降高度
    float fallVelocity;         // 下降速度
} MmwDemo_output_message_fall_event;

// 输出示例
void outputFallEvent(MmwDemo_output_message_fall_event *event)
{
    // TLV header
    MmwDemo_output_message_tlv tlv;
    tlv.type = MMWDEMO_OUTPUT_MSG_FALL_DETECTED;
    tlv.length = sizeof(MmwDemo_output_message_fall_event);
    
    // 发送TLV header
    UART_writePolling(tlv);
    
    // 发送数据
    UART_writePolling(event);
}
```

---

## 6. 开发实施步骤

### 6.1 Phase 1: 代码准备（1-2天）

**Step 1: 导入3D People Tracking项目**
```powershell
# 在CCS中
File → Import → CCS Projects
Browse → C:\ti\radar_toolbox_xxx\source\ti\examples\3D_people_tracking\68xx_3D_people_tracking\
导入MSS和DSS两个项目
```

**Step 2: 编译验证**
```
1. 右键MSS项目 → Build Project
2. 右键DSS项目 → Build Project
3. 确保编译成功，无错误
```

**Step 3: 创建跌倒检测模块文件**
```
在DSS项目中：
1. 创建 falldetection/ 目录
2. 添加 falldetection.c, falldetection.h
3. 添加 trajectory_history.c, trajectory_history.h
4. 更新CCS项目配置，添加新文件
```

### 6.2 Phase 2: 实现轨迹管理（2-3天）

**Step 1: 实现轨迹历史数据结构**
```c
// trajectory_history.c

// 初始化轨迹管理器
void TrajectoryManager_init(TrajectoryManager *mgr)
{
    memset(mgr, 0, sizeof(TrajectoryManager));
}

// 更新目标轨迹
int32_t TrajectoryManager_update(TrajectoryManager *mgr,
                                 uint32_t targetId,
                                 float x, float y, float z,
                                 float vx, float vy, float vz,
                                 uint64_t timestamp)
{
    // 查找或创建目标轨迹
    TargetTrajectory *trajectory = findOrCreateTrajectory(mgr, targetId);
    
    // 添加新的轨迹点（环形缓冲区）
    uint32_t idx = trajectory->headIdx;
    trajectory->history[idx].x = x;
    trajectory->history[idx].y = y;
    trajectory->history[idx].z = z;
    trajectory->history[idx].vx = vx;
    trajectory->history[idx].vy = vy;
    trajectory->history[idx].vz = vz;
    trajectory->history[idx].timestamp = timestamp;
    trajectory->history[idx].valid = 1;
    
    // 更新指针和计数
    trajectory->headIdx = (idx + 1) % MAX_HISTORY_FRAMES;
    if (trajectory->count < MAX_HISTORY_FRAMES) {
        trajectory->count++;
    }
    
    return 0;
}
```

**Step 2: 测试轨迹管理**
```c
// 在dss_data_path.c中添加测试代码
void testTrajectoryManager()
{
    TrajectoryManager mgr;
    TrajectoryManager_init(&mgr);
    
    // 模拟添加轨迹点
    for (int i = 0; i < 10; i++) {
        TrajectoryManager_update(&mgr, 1, 
                                0.0f, 1.0f, 1.5f - i*0.1f,
                                0.0f, 0.0f, -0.5f,
                                i * 100);
    }
    
    // 验证数据
    TargetTrajectory *traj;
    TrajectoryManager_getTrajectory(&mgr, 1, &traj);
    // 检查traj->count == 10
    // 检查traj->history[0].z == 1.5
}
```

### 6.3 Phase 3: 实现跌倒检测算法（3-4天）

**Step 1: 实现高度下降检测**
```c
// falldetection.c

bool FallDetection_detectHeightDrop(TargetTrajectory *trajectory,
                                    FallDetectionParams *params)
{
    if (trajectory->count < 5) return false;
    
    // 获取最新和最旧的高度
    uint32_t newestIdx = (trajectory->headIdx - 1 + MAX_HISTORY_FRAMES) 
                         % MAX_HISTORY_FRAMES;
    uint32_t oldestIdx = (trajectory->headIdx - trajectory->count + MAX_HISTORY_FRAMES) 
                         % MAX_HISTORY_FRAMES;
    
    float currentHeight = trajectory->history[newestIdx].z;
    float initialHeight = trajectory->history[oldestIdx].z;
    
    // 计算高度变化
    float heightDrop = currentHeight - initialHeight;
    
    // 检查是否超过阈值
    if (heightDrop < params->heightThreshold) {
        // 检查时间窗口
        uint64_t timeDiff = trajectory->history[newestIdx].timestamp -
                           trajectory->history[oldestIdx].timestamp;
        if (timeDiff < 500) {  // 500ms内
            return true;
        }
    }
    
    return false;
}
```

**Step 2: 实现垂直速度检测**
```c
bool FallDetection_detectVerticalVelocity(TargetTrajectory *trajectory,
                                          FallDetectionParams *params)
{
    if (trajectory->count < 3) return false;
    
    // 获取最近几帧的垂直速度
    uint32_t idx = (trajectory->headIdx - 1 + MAX_HISTORY_FRAMES) 
                   % MAX_HISTORY_FRAMES;
    
    float vz = trajectory->history[idx].vz;
    
    // 检查垂直速度是否超过阈值（负值表示向下）
    return (vz < params->velocityThreshold);
}
```

**Step 3: 实现静止检测**
```c
bool FallDetection_detectStationary(TargetTrajectory *trajectory,
                                    FallDetectionParams *params)
{
    if (trajectory->count < 10) return false;
    
    // 检查最近N帧的速度是否都很小
    uint32_t stationaryCount = 0;
    for (uint32_t i = 0; i < 10; i++) {
        uint32_t idx = (trajectory->headIdx - 1 - i + MAX_HISTORY_FRAMES) 
                       % MAX_HISTORY_FRAMES;
        
        float speed = sqrtf(trajectory->history[idx].vx * trajectory->history[idx].vx +
                           trajectory->history[idx].vy * trajectory->history[idx].vy +
                           trajectory->history[idx].vz * trajectory->history[idx].vz);
        
        if (speed < params->stationaryVelocity) {
            stationaryCount++;
        }
    }
    
    // 如果80%以上的帧都静止，认为目标静止
    return (stationaryCount >= 8);
}
```

**Step 4: 实现主检测逻辑**
```c
int32_t FallDetection_process(FallDetectionHandle *handle,
                              GTRACK_targetDesc *targets,
                              uint32_t numTargets,
                              uint64_t timestamp)
{
    // 更新轨迹历史
    for (uint32_t i = 0; i < numTargets; i++) {
        TrajectoryManager_update(&handle->trajectoryMgr,
                                targets[i].uid,
                                targets[i].S[0], targets[i].S[1], targets[i].S[2],
                                targets[i].S[3], targets[i].S[4], targets[i].S[5],
                                timestamp);
    }
    
    // 对每个目标进行跌倒检测
    handle->output.numResults = 0;
    for (uint32_t i = 0; i < numTargets; i++) {
        TargetTrajectory *traj;
        TrajectoryManager_getTrajectory(&handle->trajectoryMgr,
                                       targets[i].uid, &traj);
        
        // 检测跌倒特征
        bool heightDrop = FallDetection_detectHeightDrop(traj, &handle->params);
        bool fastVelocity = FallDetection_detectVerticalVelocity(traj, &handle->params);
        bool stationary = FallDetection_detectStationary(traj, &handle->params);
        
        // 判断跌倒事件
        FallDetectionResult *result = &handle->output.results[handle->output.numResults];
        result->targetId = targets[i].uid;
        
        if (heightDrop && fastVelocity) {
            result->state = FALL_STATE_FALLING;
            result->confidence = 0.7f;
        } else if (heightDrop && fastVelocity && stationary) {
            result->state = FALL_STATE_FALLEN;
            result->confidence = 0.95f;
            handle->output.fallEventCount++;
        } else {
            result->state = FALL_STATE_NORMAL;
            result->confidence = 0.0f;
        }
        
        handle->output.numResults++;
    }
    
    return 0;
}
```

### 6.4 Phase 4: 集成到数据处理链（1-2天）

**Step 1: 在dss_main.c中初始化**
```c
// 全局句柄
FallDetectionHandle gFallDetectionHandle;

// 在main()中初始化
void MmwDemo_dssInitTask(UArg arg0, UArg arg1)
{
    // ... 现有初始化代码
    
    // 初始化跌倒检测
    FallDetectionParams params = {
        .heightThreshold = -0.5f,
        .velocityThreshold = -1.5f,
        .groundHeight = 0.5f,
        .stationaryVelocity = 0.2f,
        .stationaryTime = 3.0f,
        .historyFrames = 30,
        .confidenceThreshold = 0.8f
    };
    FallDetection_init(&gFallDetectionHandle, &params);
    
    // ... 其他初始化
}
```

**Step 2: 在数据处理链中调用**
```c
// 在dss_data_path.c的处理函数中
void MmwDemo_dssDataPathProcessEvents(UArg arg0, UArg arg1)
{
    // ... 现有处理代码
    
    // GTRACK处理后
    GTRACK_targetDesc *targets;
    uint32_t numTargets;
    // ... 获取GTRACK输出
    
    // 🎯 调用跌倒检测
    if (gFallDetectionHandle.params.enable) {
        FallDetection_process(&gFallDetectionHandle,
                             targets,
                             numTargets,
                             gMmwDssMCB.frameCount * 100);  // 时间戳
        
        // 获取结果
        FallDetectionOutput *fallOutput;
        FallDetection_getResults(&gFallDetectionHandle, &fallOutput);
        
        // 输出跌倒事件
        for (uint32_t i = 0; i < fallOutput->numResults; i++) {
            if (fallOutput->results[i].state == FALL_STATE_FALLEN) {
                outputFallEvent(&fallOutput->results[i]);
            }
        }
    }
    
    // ... 其他处理
}
```

---

## 7. 测试与调试

### 7.1 单元测试

**测试1: 轨迹管理器测试**
```c
void test_TrajectoryManager()
{
    // 测试添加轨迹
    // 测试查询轨迹
    // 测试环形缓冲区
}
```

**测试2: 高度检测测试**
```c
void test_HeightDetection()
{
    // 模拟跌倒场景
    // 模拟正常蹲下场景
    // 验证检测准确性
}
```

### 7.2 集成测试

**测试场景**：
```
1. 站立→跌倒→躺地
2. 站立→蹲下→站起（不应误报）
3. 行走→跌倒
4. 多人场景
```

---

**下一部分**：Part3 - 跌倒检测算法优化与测试

> 💡 建议：先完成环境准备和固件迁移，再继续Part3的算法优化。

