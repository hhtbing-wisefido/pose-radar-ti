# 📊 Part14: TLV数据格式与工具兼容性完整指南

> **创建日期**: 2026-01-09
> **适用范围**: AWRL6844 mmWave Demo / InCabin Demo / 自定义固件
> **文档状态**: ✅ 完整（综合TLV快速参考与InCabin对比分析）

---

## 🎯 本章目标

本章深入分析TI雷达固件的TLV（Type-Length-Value）数据格式，回答以下核心问题：

1. ❓ **什么是TLV数据格式？**
2. ❓ **标准mmWave Demo与InCabin Demo的TLV有什么区别？**
3. ❓ **为什么SDK Visualizer无法显示InCabin的点云？**
4. ❓ **开发自定义固件时如何选择TLV Type ID？**

---

## 📑 目录

- [1. TLV数据格式基础](#1-tlv数据格式基础)
- [2. 标准Demo vs InCabin Demo TLV对比](#2-标准demo-vs-incabin-demo-tlv对比)
- [3. 工具兼容性分析](#3-工具兼容性分析)
- [4. 为什么InCabin使用独有格式](#4-为什么incabin使用独有格式)
- [5. 数据流对比](#5-数据流对比)
- [6. 自定义固件TLV设计指南](#6-自定义固件tlv设计指南)
- [7. 快速诊断指南](#7-快速诊断指南)
- [8. 源码参考](#8-源码参考)
- [9. 总结与最佳实践](#9-总结与最佳实践)

---

## 1. TLV数据格式基础

### 1.1 什么是TLV？

TLV (Type-Length-Value) 是雷达通过UART发送数据时使用的格式：

```
┌─────────────────────────────────────────┐
│  帧头 (Frame Header)                     │
├─────────────────────────────────────────┤
│  TLV 1: Type=XXX, Length=YYY            │
│         Data (YYY bytes)                │
├─────────────────────────────────────────┤
│  TLV 2: Type=ZZZ, Length=WWW            │
│         Data (WWW bytes)                │
├─────────────────────────────────────────┤
│  ...更多TLV块...                         │
└─────────────────────────────────────────┘
```

**关键在于`Type`字段** - 不同Demo使用不同的Type ID！

### 1.2 TLV结构详解

```c
typedef struct {
    uint32_t type;    // 数据类型ID
    uint32_t length;  // 数据长度（字节）
} MmwDemo_output_message_tl;
```

接收端根据`type`字段决定如何解析`data`内容。

---

## 2. 标准Demo vs InCabin Demo TLV对比

### 2.1 TLV Type ID对照表

| 数据类型 | 标准mmWave Demo | InCabin Demo | 兼容性 | 说明 |
|---------|----------------|--------------|--------|------|
| **点云数据** | Type = 1<br/>`DETECTED_POINTS` | Type = 3001<br/>`POINT_CLOUD` | ❌ 不兼容 | ⭐ 关键差异 |
| **Range Profile** | Type = 2<br/>`RANGE_PROFILE` | Type = 2<br/>`RANGE_PROFILE` | ✅ 兼容 | 相同 |
| **Noise Profile** | Type = 3<br/>`NOISE_PROFILE` | ❌ 无 | - | 标准Demo独有 |
| **Stats统计** | Type = 6<br/>`STATS` | Type = 6<br/>`STATS` | ✅ 兼容 | 相同 |
| **Side Info** | Type = 7<br/>`SIDE_INFO` | ❌ 无 | - | 标准Demo独有 |
| **占用特征** | ❌ 无 | Type = 3002<br/>`OCCUPANCY_FEATURES` | - | InCabin独有 |
| **分类结果** | ❌ 无 | Type = 1041<br/>`CLASSIFICATION_RES` | - | InCabin独有 |
| **身高估计** | ❌ 无 | Type = 1042<br/>`HEIGHT_ESTIMATION` | - | InCabin独有 |
| **入侵检测** | ❌ 无 | Type = 12, 13<br/>`INTRUSION_DET_*` | - | InCabin独有 |

### 2.2 源码定义对比

**标准mmWave Demo**
```c
// 源文件: C:\ti\mmwave_l_sdk_06_01_00_01\examples\mmw_demo\mmwave_demo\source\mmwave_demo.h

typedef enum MmwDemo_output_message_type_e
{
    MMWDEMO_OUTPUT_MSG_DETECTED_POINTS = 1,           // ← Type = 1
    MMWDEMO_OUTPUT_MSG_RANGE_PROFILE = 2,
    MMWDEMO_OUTPUT_MSG_NOISE_PROFILE = 3,
    MMWDEMO_OUTPUT_MSG_STATS = 6,
    MMWDEMO_OUTPUT_MSG_DETECTED_POINTS_SIDE_INFO = 7,
    // ... 更多标准类型
} MmwDemo_output_message_type;
```

**InCabin Demo**
```c
// 源文件: project-code\AWRL6844_InCabin_Demos\src\mss\source\mmwave_demo_mss.h

typedef enum mmwLab_output_message_type_e
{
    MMWDEMO_OUTPUT_MSG_RANGE_PROFILE = 2,
    MMWDEMO_OUTPUT_MSG_POINT_CLOUD = 3001,              // ← Type = 3001 ⭐ 不同！
    MMWDEMO_OUTPUT_MSG_OCCUPANCY_FEATURES = 3002,       // ← InCabin独有
    MMWDEMO_OUTPUT_MSG_OCCUPANCY_CLASSIFICATION_RES = 1041,
    MMWDEMO_OUTPUT_MSG_OCCUPANCY_HEIGHT_RES = 1042,
    MMWDEMO_OUTPUT_MSG_STATS = 6,
    MMWDEMO_OUTPUT_MSG_INTRUSION_DET_INFO = 12,
    MMWDEMO_OUTPUT_MSG_INTRUSION_DET_3D_DET_MAT = 13,
} mmwLab_output_message_type;
```

---

## 3. 工具兼容性分析

### 3.1 工具兼容性矩阵

| 固件 | 配置文件 | SDK Visualizer | InCabin GUI | 说明 |
|-----|---------|----------------|-------------|------|
| **mmwave_demo.release.appimage** | 6844_profile_4T4R_tdm.cfg | ✅ 能用 | ❌ 不能 | 标准Demo使用Type=1 |
| **InCabin固件** | incabin_compatible.cfg | ❌ 不能 | ✅ 能用 | InCabin使用Type=3001 |
| **自定义固件(基于标准Demo)** | 自定义.cfg | ✅ 能用 | ❌ 不能 | 遵循标准Demo TLV协议 |
| **自定义固件(基于InCabin)** | 自定义.cfg | ❌ 不能 | ✅ 能用 | 遵循InCabin TLV协议 |

### 3.2 工具路径

| 工具 | 路径 | 适用固件 |
|-----|------|---------|
| **SDK Visualizer** | `C:\ti\MMWAVE_L_SDK_06_01_00_01\tools\visualizer\` | 标准Demo |
| **InCabin GUI** | `C:\ti\radar_toolbox_3_30_00_06\tools\visualizers\AWRL6844_Incabin_GUI\src\occupancy_demo_gui.exe` | InCabin Demo |

### 3.3 SDK Visualizer解析逻辑

```python
def parse_tlv(tlv_type, tlv_data):
    if tlv_type == 1:      # DETECTED_POINTS（标准Demo）
        parse_point_cloud()  # ✅ 能解析
        display_points()     # ✅ 显示点云
        
    elif tlv_type == 2:    # RANGE_PROFILE
        return parse_range_profile(tlv_data)
    
    elif tlv_type == 6:    # STATS
        return parse_stats(tlv_data)
    
    elif tlv_type == 3001: # InCabin的点云
        skip()               # ❌ 不认识这个ID
                             # ❌ 跳过这个TLV块
                             # ❌ 导致Points Detected = 0
    else:
        return None  # 未知类型，跳过
```

**这就是为什么**：
- SDK Visualizer收到InCabin数据包 (WebSocket正常)
- 但看到Type = 3001不认识
- 跳过点云TLV块
- UI显示 Points Detected = 0

---

## 4. 为什么InCabin使用独有格式

### 4.1 应用场景差异

| 维度 | 标准Demo | InCabin Demo |
|-----|---------|--------------|
| **应用场景** | 通用雷达应用 | 汽车座舱监测 |
| **输出内容** | 基础点云 (X,Y,Z,V,SNR) | 点云 + AI分类结果 |
| **目标识别** | 无 | 婴儿/成人/空座 |
| **检测距离** | 灵活配置 | 0.5-2m (车内) |
| **多区域** | 不支持 | 支持多座位监测 |

### 4.2 数据处理流程差异

**标准Demo流程**：
```
雷达原始数据 → CFAR检测 → 点云 → UART输出
                                    ↓
                            Type = 1 (点云)
```

**InCabin Demo流程**：
```
雷达原始数据 → CFAR检测 → 点云 → 特征提取 → CNN分类器 → UART输出
                                ↓           ↓            ↓
                         Type = 3001   Type = 3002  Type = 1041
                         (点云)        (特征)       (分类结果)
```

### 4.3 数据结构差异

**标准Demo点云结构** (简单，20字节/点)：

```c
typedef struct {
    float x;        // X坐标 (m)
    float y;        // Y坐标 (m)
    float z;        // Z坐标 (m)
    float velocity; // 速度 (m/s)
    float snr;      // 信噪比 (dB)
} StandardPointCloud;
```

**InCabin点云结构** (量化，8字节/点)：

```c
typedef struct {
    int8_t  azimuth;    // 方位角 (量化后)
    int8_t  elevation;  // 俯仰角 (量化后)
    uint16_t range;     // 距离 (量化后)
    int16_t doppler;    // 多普勒 (量化后)
    uint16_t snr;       // SNR (Q8格式)
} InCabinPointCloud;

// 单位转换信息
typedef struct {
    float elevationUnit;  // 例如: (π/2)/127
    float azimuthUnit;    // 例如: (π/2)/127
    float rangeUnit;      // 例如: 0.00025m
    float dopplerUnit;    // 例如: 0.00028m/s
    float snrUnit;        // 例如: 1/256
} PointCloudUnits;
```

**为什么量化？**
- 节省UART带宽 (8字节 vs 20字节)
- InCabin需要5fps高帧率，数据量大
- 接收端用`pointUnit`恢复浮点数

### 4.4 InCabin独有输出

**占用特征 (Type=3002)**：
```c
if (gMmwMssMCB.guiMonSel.occupancyDetFeaturesInfo)
{
    gMmwMssMCB.featuresToUart.messageTL.type = MMWDEMO_OUTPUT_MSG_OCCUPANCY_FEATURES;
    memcpy(gMmwMssMCB.featuresToUart.features, 
           &gMmwMssMCB.classifierResult.featOut.featsPerZone, ...);
}
```

**分类结果 (Type=1041)**：
```c
if (gMmwMssMCB.guiMonSel.occupancyDetClassInfo)
{
    gMmwMssMCB.classResToUart.messageTL.type = MMWDEMO_OUTPUT_MSG_OCCUPANCY_CLASSIFICATION_RES;
    // 输出每个区域的占用概率 (0-100%)
    for (i = 0; i < numZones * numClasses; i++)
    {
        gMmwMssMCB.classResToUart.predictions[i] = 
            (uint8_t) lroundf(oneQ7float * gMmwMssMCB.classifierResult.zonesPredictions[i]);
    }
}
```

**身高估计 (Type=1042, 仅CPD模式)**：
```c
if(gMmwMssMCB.runningMode == RUNNING_MODE_CPD)
{
    gMmwMssMCB.heightEstToUart.messageTL.type = MMWDEMO_OUTPUT_MSG_OCCUPANCY_HEIGHT_RES;
    for (i = 0; i < numZones; i++)
    {
        gMmwMssMCB.heightEstToUart.heightEst[i] = 
            gMmwMssMCB.classifierResult.heightEstimations[i];
    }
}
```

---

## 5. 数据流对比

### 5.1 标准mmWave Demo + SDK Visualizer ✅

```
雷达硬件
   ↓
标准mmwave_demo固件
   ↓
UART输出: Type=1 (点云), Type=2 (Range Profile), Type=6 (Stats)
   ↓
SDK Visualizer Python后端
   ↓ (识别Type=1)
解析点云数据
   ↓
WebSocket发送到浏览器
   ↓
JavaScript渲染点云 ✅ 成功！
```

### 5.2 InCabin Demo + SDK Visualizer ❌

```
雷达硬件
   ↓
InCabin固件
   ↓
UART输出: Type=3001 (点云), Type=3002 (特征), Type=1041 (分类)
   ↓
SDK Visualizer Python后端
   ↓ (不认识Type=3001)
跳过点云TLV ❌
   ↓
WebSocket发送: {pointCloud: [], detectedObjects: 0}
   ↓
JavaScript渲染: Points Detected = 0 ❌ 失败！
```

### 5.3 InCabin Demo + InCabin GUI ✅

```
雷达硬件
   ↓
InCabin固件
   ↓
UART输出: Type=3001 (点云), Type=3002 (特征), Type=1041 (分类)
   ↓
InCabin GUI Python后端
   ↓ (识别Type=3001, 3002, 1041)
解析所有InCabin专用数据
   ↓
GUI渲染
   ↓
点云 + 占用概率 + 分类结果 + 身高 ✅ 成功！
```

---

## 6. 自定义固件TLV设计指南

### 6.1 🔴 关键原则：兼容标准Demo格式

**推荐做法**：使用标准mmWave Demo的TLV Type ID

```c
// ✅ 推荐：使用标准格式
MMWDEMO_OUTPUT_MSG_DETECTED_POINTS = 1,           // 点云
MMWDEMO_OUTPUT_MSG_RANGE_PROFILE = 2,             // Range Profile
MMWDEMO_OUTPUT_MSG_STATS = 6,                     // 统计
MMWDEMO_OUTPUT_MSG_DETECTED_POINTS_SIDE_INFO = 7, // SNR信息
```

**好处**：
- ✅ SDK Visualizer直接可用
- ✅ 开发调试方便
- ✅ 官方文档可参考
- ✅ 后续维护简单

### 6.2 扩展TLV设计

**自定义TLV从Type=1000开始，避开标准范围**

```c
// 健康检测专用TLV（从1000开始）
#define MMWDEMO_OUTPUT_MSG_PRESENCE_DETECT      1000  // 人存检测结果
#define MMWDEMO_OUTPUT_MSG_HEALTH_FEATURES      1001  // 健康特征向量
#define MMWDEMO_OUTPUT_MSG_VITAL_SIGNS          1002  // 生命体征
#define MMWDEMO_OUTPUT_MSG_POSTURE_RESULT       1003  // 姿态检测结果
#define MMWDEMO_OUTPUT_MSG_FALL_DETECTION       1004  // 跌倒检测告警
```

### 6.3 Type ID范围规划

| 范围 | 用途 | 说明 |
|-----|------|------|
| 1-99 | 标准Demo类型 | TI官方定义 |
| 100-299 | 标准Demo扩展 | TI保留 |
| 300-399 | 官方Demo扩展 | TI保留 |
| 1000-1999 | 用户自定义 | 推荐范围 |
| 3000-3999 | InCabin专用 | TI InCabin Demo |

### 6.4 设计检查清单

- [ ] 核心TLV（点云、Range Profile、Stats）使用标准Type ID？
- [ ] 自定义TLV从1000开始？
- [ ] 没有与标准范围冲突？
- [ ] SDK Visualizer能正常显示核心数据？

---

## 7. 快速诊断指南

### 7.1 症状：雷达运行，SDK Visualizer显示Points Detected = 0

**检查步骤**：

1. ✅ **确认固件类型**
   ```
   如果是InCabin固件 → 必须用InCabin GUI ⚠️
   如果是标准Demo固件 → 可以用SDK Visualizer ✅
   如果是自定义固件 → 检查TLV Type ID
   ```

2. ✅ **查看WebSocket日志**
   ```
   如果看到大量数据包 → 数据在传输，但工具无法解析
   如果无数据包 → 硬件/配置问题
   ```

3. ✅ **检查Range Profile**
   ```
   如果Range Profile有峰值 → 雷达工作正常，是工具兼容性问题
   如果Range Profile平坦 → 硬件或CFAR参数问题
   ```

4. ✅ **检查固件TLV Type**
   ```c
   // 在固件头文件中检查
   如果MMWDEMO_OUTPUT_MSG_DETECTED_POINTS = 1 → ✅ 正确
   如果MMWDEMO_OUTPUT_MSG_POINT_CLOUD = 3001 → ❌ 需修改或换工具
   ```

### 7.2 诊断流程图

```
SDK Visualizer无点云？
       ↓
是否有WebSocket数据？
    ↙        ↘
   是          否
   ↓            ↓
检查TLV Type   检查串口/硬件
   ↓            
Type=1？ ──是→ 其他问题
   ↓否
使用InCabin GUI
或修改固件
```

---

## 8. 源码参考

### 8.1 标准Demo

| 文件 | 路径 | 说明 |
|-----|------|------|
| TLV定义 | `C:\ti\mmwave_l_sdk_06_01_00_01\examples\mmw_demo\mmwave_demo\source\mmwave_demo.h` | Line 1296-1350 |
| 点云输出 | `mmwave_demo.c` | TLV打包逻辑 |

### 8.2 InCabin Demo

| 文件 | 路径 | 说明 |
|-----|------|------|
| TLV定义 | `project-code\AWRL6844_InCabin_Demos\src\mss\source\mmwave_demo_mss.h` | Line 1533-1574 |
| 点云打包 | `mmwave_demo_mss.c` | Line 1138-1270 |

---

## 9. 总结与最佳实践

### 9.1 核心要点

| 要点 | 说明 |
|-----|------|
| **TLV Type ID是关键** | 决定工具是否能解析 |
| **标准Demo = Type 1** | SDK Visualizer识别 |
| **InCabin = Type 3001** | 需要InCabin GUI |
| **自定义固件用1000+** | 避免冲突 |

### 9.2 工具选择指南

| 你的固件 | 应该用的工具 |
|---------|------------|
| 标准mmWave Demo | SDK Visualizer |
| InCabin Demo | InCabin GUI |
| 自定义固件(基于标准Demo) | SDK Visualizer |
| 自定义固件(基于InCabin) | InCabin GUI |

### 9.3 开发建议

1. ✅ **新项目基于标准Demo开发** - SDK Visualizer可用
2. ✅ **核心TLV保持标准格式** - 点云用Type=1
3. ✅ **扩展TLV从1000开始** - 避免冲突
4. ✅ **测试前确认工具匹配** - 避免调试浪费时间

### 9.4 常见问题FAQ

**Q: 为什么InCabin固件不直接用Type=1？**
A: InCabin的点云结构与标准Demo不同（量化数据），使用不同Type ID避免解析器误解析。

**Q: 能否修改SDK Visualizer支持InCabin？**
A: 技术上可行，但不推荐。InCabin GUI是专门设计的，功能更完整。

**Q: 自定义固件必须兼容标准格式吗？**
A: 不是必须，但强烈推荐。否则需要开发专用解析工具。

---

## 📚 相关文档

- [Part3-SDK与固件关系及工作流程](./Part3-SDK与固件关系及工作流程.md)
- [Part13-SDK对比与RTOS深度解析](./Part13-SDK对比与RTOS深度解析.md)
- [附录F-TLV数据格式兼容性要求](../08-AWRL6844雷达健康检测实现方案/AWRL6844雷达健康检测-附录F-TLV数据格式兼容性要求.md) - HealthDetect项目专用

---

**结论**: 🎯 **工具选择必须匹配固件类型！开发自定义固件时优先使用标准TLV格式！**
