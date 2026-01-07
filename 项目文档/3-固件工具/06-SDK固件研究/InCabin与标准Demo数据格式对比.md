# InCabin Demo vs 标准mmWave Demo 数据格式对比

## 📋 问题回答

**你的问题**：
1. InCabin固件使用什么数据格式？
2. 标准Demo (mmwave_demo.release.appimage) 使用什么数据格式？
3. 两者有什么区别？
4. 为什么InCabin要使用独有格式？

---

## 🔍 核心发现：TLV类型ID不同！

### TLV (Type-Length-Value) 数据格式

雷达通过UART发送数据时使用TLV格式：

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

---

## 📊 数据格式对比表

| 数据类型 | 标准mmWave Demo | InCabin Demo | 说明 |
|---------|----------------|--------------|------|
| **点云数据** | `Type = 1` (DETECTED_POINTS) | `Type = 3001` (POINT_CLOUD) | ⭐ ID完全不同 |
| **Range Profile** | `Type = 2` (RANGE_PROFILE) | `Type = 2` (RANGE_PROFILE) | ✅ 相同 |
| **Stats** | `Type = 6` (STATS) | `Type = 6` (STATS) | ✅ 相同 |
| **Occupancy Features** | ❌ 无 | `Type = 3002` | ⭐ InCabin独有 |
| **Classification Result** | ❌ 无 | `Type = 1041` | ⭐ InCabin独有 |
| **Height Estimation** | ❌ 无 | `Type = 1042` | ⭐ InCabin独有 |
| **Intrusion Detection** | ❌ 无 | `Type = 12, 13` | ⭐ InCabin独有 |

---

## 🔴 关键区别：点云TLV Type ID

### 标准mmWave Demo
**源文件**: `C:\ti\mmwave_l_sdk_06_01_00_01\examples\mmw_demo\mmwave_demo\source\mmwave_demo.h`

```c
typedef enum MmwDemo_output_message_type_e
{
    /*! @brief   List of detected points */
    MMWDEMO_OUTPUT_MSG_DETECTED_POINTS = 1,  // ← Type = 1

    /*! @brief   Range profile */
    MMWDEMO_OUTPUT_MSG_RANGE_PROFILE = 2,

    /*! @brief   Noise floor profile */
    MMWDEMO_OUTPUT_MSG_NOISE_PROFILE = 3,
    
    /*! @brief   Stats information */
    MMWDEMO_OUTPUT_MSG_STATS = 6,
    
    // ... 更多标准类型
} MmwDemo_output_message_type;
```

### InCabin Demo
**源文件**: `d:\7.project\TI_Radar_Project\project-code\AWRL6844_InCabin_Demos\src\mss\source\mmwave_demo_mss.h`

```c
typedef enum mmwLab_output_message_type_e
{
    /*! @brief   Range profile */
    MMWDEMO_OUTPUT_MSG_RANGE_PROFILE = 2,

    /*! @brief   Point Cloud */
    MMWDEMO_OUTPUT_MSG_POINT_CLOUD = 3001,  // ← Type = 3001 ⭐ 不同！

    /*! @brief   SBR/CPD features */
    MMWDEMO_OUTPUT_MSG_OCCUPANCY_FEATURES = 3002,  // ← InCabin独有
    
    /*! @brief   Occupancy classification result */
    MMWDEMO_OUTPUT_MSG_OCCUPANCY_CLASSIFICATION_RES = 1041,  // ← InCabin独有
    
    /*! @brief   Occupancy height result */
    MMWDEMO_OUTPUT_MSG_OCCUPANCY_HEIGHT_RES = 1042,  // ← InCabin独有

    /*! @brief   Stats information */
    MMWDEMO_OUTPUT_MSG_STATS = 6,

    MMWDEMO_OUTPUT_MSG_INTRUSION_DET_INFO = 12,  // ← InCabin独有
    MMWDEMO_OUTPUT_MSG_INTRUSION_DET_3D_DET_MAT = 13,  // ← InCabin独有
    
    // ... 更多调试类型
} mmwLab_output_message_type;
```

---

## 💡 为什么你的标准Demo能用SDK Visualizer？

### 测试配置对比

| 配置 | 固件 | 点云TLV Type | SDK Visualizer |
|-----|------|-------------|---------------|
| `6844_profile_4T4R_tdm.cfg` | **标准mmwave_demo** | Type = 1 | ✅ 能识别 |
| `incabin_compatible.cfg` | **InCabin Demo** | Type = 3001 | ❌ 无法识别 |

**原因**：
1. **SDK Visualizer内置的TLV解析器只识别标准Demo的Type ID**
2. 标准Demo点云: Type = 1 → SDK Visualizer知道如何解析
3. InCabin点云: Type = 3001 → SDK Visualizer不认识这个ID
4. SDK Visualizer看到Type = 3001 → 跳过这个TLV块 → 无法显示点云

---

## 🔧 SDK Visualizer的解析逻辑

### Python后端解析代码逻辑 (推测)

```python
def parse_tlv(tlv_type, tlv_data):
    if tlv_type == 1:  # DETECTED_POINTS
        # 解析点云数据
        points = parse_point_cloud(tlv_data)
        return {"pointCloud": points}
    
    elif tlv_type == 2:  # RANGE_PROFILE
        # 解析Range Profile
        return parse_range_profile(tlv_data)
    
    elif tlv_type == 6:  # STATS
        # 解析统计信息
        return parse_stats(tlv_data)
    
    elif tlv_type == 3001:  # ← InCabin的点云
        # ❌ SDK Visualizer不知道如何处理！
        # 可能跳过，或者尝试用错误的解析器
        return None  # 导致无数据显示
    
    else:
        # 未知类型，跳过
        return None
```

**这就是为什么**：
- SDK Visualizer收到InCabin数据包 (WebSocket正常)
- 但看到Type = 3001不认识
- 跳过点云TLV块
- UI显示 Points Detected = 0

---

## 🎯 为什么InCabin使用独有格式？

### 原因1: 应用场景特殊化

**标准Demo**：
- 通用雷达应用
- 只输出基础点云 (X, Y, Z, Velocity, SNR)
- 适用于工业、交通、安防等多种场景

**InCabin Demo**：
- 专门用于**汽车座舱监测**
- 需要识别：婴儿 vs 成人 vs 空座
- 需要输出：占用概率、身高估计、分类结果
- 需要多区域检测 (驾驶座、副驾驶、后排)

### 原因2: 额外的AI处理输出

InCabin固件包含**机器学习分类器**：

```
标准Demo流程：
雷达原始数据 → CFAR检测 → 点云 → UART输出
                                    ↓
                            Type = 1 (点云)

InCabin Demo流程：
雷达原始数据 → CFAR检测 → 点云 → 特征提取 → CNN分类器 → UART输出
                                ↓           ↓            ↓
                         Type = 3001   Type = 3002  Type = 1041
                         (点云)        (特征)       (分类结果)
```

**固件代码证据** (`mmwave_demo_mss.c` 1220-1260行)：

```c
/* 存储特征到UART */
if (gMmwMssMCB.guiMonSel.occupancyDetFeaturesInfo)
{
    /* Features */
    gMmwMssMCB.featuresToUart.messageTL.type = MMWDEMO_OUTPUT_MSG_OCCUPANCY_FEATURES;  // Type = 3002
    gMmwMssMCB.featuresToUart.messageTL.length = ...;
    memcpy(gMmwMssMCB.featuresToUart.features, &gMmwMssMCB.classifierResult.featOut.featsPerZone, ...);
}

if (gMmwMssMCB.guiMonSel.occupancyDetClassInfo)
{
    /* 分类结果 */
    gMmwMssMCB.classResToUart.messageTL.type = MMWDEMO_OUTPUT_MSG_OCCUPANCY_CLASSIFICATION_RES;  // Type = 1041
    gMmwMssMCB.classResToUart.messageTL.length = ...;
    // 输出每个区域的占用概率 (0-100%)
    for (i = 0; i < numZones * numClasses; i++)
    {
        gMmwMssMCB.classResToUart.predictions[i] = (uint8_t) lroundf(oneQ7float * gMmwMssMCB.classifierResult.zonesPredictions[i]);
    }
    
    /* 身高估计 (仅CPD模式) */
    if(gMmwMssMCB.runningMode == RUNNING_MODE_CPD)
    {
        gMmwMssMCB.heightEstToUart.messageTL.type = MMWDEMO_OUTPUT_MSG_OCCUPANCY_HEIGHT_RES;  // Type = 1042
        gMmwMssMCB.heightEstToUart.messageTL.length = ...;
        for (i = 0; i < numZones; i++)
        {
            gMmwMssMCB.heightEstToUart.heightEst[i] = gMmwMssMCB.classifierResult.heightEstimations[i];
        }
    }
}
```

### 原因3: 数据结构更复杂

**标准Demo点云结构** (简单)：

```c
typedef struct {
    float x;        // X坐标 (m)
    float y;        // Y坐标 (m)
    float z;        // Z坐标 (m)
    float velocity; // 速度 (m/s)
    float snr;      // 信噪比 (dB)
} StandardPointCloud;
```

**InCabin点云结构** (包含更多信息)：

```c
typedef struct {
    int8_t  azimuth;    // 方位角 (量化后)
    int8_t  elevation;  // 俯仰角 (量化后)
    uint16_t range;     // 距离 (量化后)
    int16_t doppler;    // 多普勒 (量化后)
    uint16_t snr;       // SNR (Q8格式)
} InCabinPointCloud;

// 加上单位转换信息
typedef struct {
    float elevationUnit;  // 例如: (π/2)/127
    float azimuthUnit;    // 例如: (π/2)/127
    float rangeUnit;      // 例如: 0.00025m
    float dopplerUnit;    // 例如: 0.00028m/s
    float snrUnit;        // 例如: 1/256
} PointCloudUnits;
```

**为什么量化**？
- 节省UART带宽 (5字节 vs 20字节)
- InCabin需要5fps高帧率，数据量大
- 接收端可以用`pointUnit`恢复浮点数：
  ```c
  real_azimuth = quantized_azimuth * azimuthUnit;
  real_range = quantized_range * rangeUnit;
  ```

---

## 🔧 InCabin GUI为什么能正确解析？

### InCabin GUI的优势

**专门设计用于InCabin Demo**：

1. **内置InCabin TLV解析器**
   ```python
   def parse_incabin_tlv(tlv_type, tlv_data):
       if tlv_type == 3001:  # POINT_CLOUD
           return parse_incabin_point_cloud(tlv_data)
       
       elif tlv_type == 3002:  # OCCUPANCY_FEATURES
           return parse_occupancy_features(tlv_data)
       
       elif tlv_type == 1041:  # CLASSIFICATION_RES
           return parse_classification_result(tlv_data)
       
       elif tlv_type == 1042:  # HEIGHT_ESTIMATION
           return parse_height_estimation(tlv_data)
   ```

2. **专用UI组件**
   - Occupancy Probability显示 (0-100%)
   - Zone Status显示 (Empty/Adult/Child)
   - Height Estimation显示
   - 多区域同时监控

3. **量化数据自动恢复**
   - 读取`pointUnit`信息
   - 自动将量化值转换为物理单位
   - 正确显示坐标和速度

---

## 📊 完整数据流对比

### 标准mmWave Demo + SDK Visualizer ✅

```
雷达硬件
   ↓
标准mmwave_demo.bin固件
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

### InCabin Demo + SDK Visualizer ❌

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

### InCabin Demo + InCabin GUI ✅

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

## 🎓 总结

### 1. 数据格式区别

| 项目 | 标准Demo | InCabin Demo |
|-----|---------|--------------|
| **点云TLV Type** | 1 | 3001 |
| **数据结构** | 浮点数 | 量化整数 + 单位 |
| **额外输出** | 无 | 特征(3002) + 分类(1041) + 身高(1042) |
| **应用场景** | 通用 | 汽车座舱监测 |

### 2. 为什么InCabin使用独有格式？

- ✅ **特殊应用需求** - 需要输出分类、身高、多区域信息
- ✅ **AI处理结果** - CNN分类器输出需要专用TLV类型
- ✅ **数据优化** - 量化数据节省带宽，适合高帧率应用
- ✅ **避免冲突** - 使用3000+范围的Type ID，不与标准Demo冲突

### 3. 为什么SDK Visualizer不工作？

- ❌ SDK Visualizer只认识标准Demo的TLV类型 (Type 1-10)
- ❌ 看到Type = 3001不知道如何解析
- ❌ 跳过InCabin的点云数据
- ❌ 导致UI显示Points Detected = 0

### 4. 正确的测试方法

| Demo类型 | 固件 | 可视化工具 |
|---------|------|-----------|
| 标准mmWave Demo | mmwave_demo.release.appimage | ✅ SDK Visualizer |
| InCabin Demo | InCabin固件 | ✅ InCabin GUI (`occupancy_demo_gui.exe`) |
| InCabin Demo | InCabin固件 | ❌ SDK Visualizer (不兼容) |

---

## 🔍 验证你的发现

你提到的现象完全符合上述分析：

1. **标准Demo能用SDK Visualizer**
   - `mmwave_demo.release.appimage` + `6844_profile_4T4R_tdm.cfg`
   - 输出Type = 1的点云
   - SDK Visualizer认识并正确显示 ✅

2. **InCabin不能用SDK Visualizer**
   - InCabin固件 + `incabin_compatible.cfg`
   - 输出Type = 3001的点云
   - SDK Visualizer不认识，无法显示 ❌

3. **WebSocket日志显示数据在传输**
   - `totalPacketLen: 3360` - 数据包确实发送了
   - `subFrameNum: 4294967295` - InCabin使用的帧号格式
   - 但SDK Visualizer无法解析TLV内容

---

## 📚 参考源码位置

### 标准Demo
- TLV定义: `C:\ti\mmwave_l_sdk_06_01_00_01\examples\mmw_demo\mmwave_demo\source\mmwave_demo.h` (line 1296-1350)

### InCabin Demo
- TLV定义: `d:\7.project\TI_Radar_Project\project-code\AWRL6844_InCabin_Demos\src\mss\source\mmwave_demo_mss.h` (line 1533-1574)
- 点云打包: `d:\7.project\TI_Radar_Project\project-code\AWRL6844_InCabin_Demos\src\mss\source\mmwave_demo_mss.c` (line 1138-1270)

---

**结论**: InCabin Demo必须使用InCabin GUI，SDK Visualizer只能用于标准Demo！🎯
