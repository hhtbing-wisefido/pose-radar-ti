# 📎 附录F：TLV数据格式兼容性要求

> **创建日期**: 2026-01-09
> **版本**: v1.0
> **目的**: 统一TLV数据格式规范，确保与官方工具兼容

---

## 🔴 核心原则（最高优先级）

### ⭐ 必须兼容标准 mmWave Demo TLV 格式

**AWRL6844_HealthDetect 项目必须使用标准 mmWave Demo 的 TLV Type ID**，以确保：

1. ✅ **SDK Visualizer 可用** - 开发调试必备工具
2. ✅ **官方测试工具兼容** - 方便问题排查
3. ✅ **后续维护简单** - 不需要开发专用解析工具
4. ✅ **文档参考一致** - 可直接使用TI官方文档

**🚫 禁止采用 InCabin Demo 的私有格式（Type=3001）**

---

## 📊 标准Demo vs InCabin Demo TLV Type ID对照表

| 数据类型 | 标准mmWave Demo | InCabin Demo | 兼容性 | HealthDetect选择 |
|---------|----------------|--------------|--------|------------------|
| **点云数据** | Type = 1<br/>`DETECTED_POINTS` | Type = 3001<br/>`POINT_CLOUD` | ❌ 不兼容 | **Type = 1** ✅ |
| **Range Profile** | Type = 2<br/>`RANGE_PROFILE` | Type = 2<br/>`RANGE_PROFILE` | ✅ 兼容 | **Type = 2** ✅ |
| **Noise Profile** | Type = 3<br/>`NOISE_PROFILE` | ❌ 无 | - | **Type = 3** ✅ |
| **Stats统计** | Type = 6<br/>`STATS` | Type = 6<br/>`STATS` | ✅ 兼容 | **Type = 6** ✅ |
| **Side Info** | Type = 7<br/>`SIDE_INFO` | ❌ 无 | - | **Type = 7** ✅ |
| **占用特征** | ❌ 无 | Type = 3002<br/>`OCCUPANCY_FEATURES` | - | ❌ 不使用 |
| **分类结果** | ❌ 无 | Type = 1041<br/>`CLASSIFICATION_RES` | - | ❌ 不使用 |
| **身高估计** | ❌ 无 | Type = 1042<br/>`HEIGHT_ESTIMATION` | - | ❌ 不使用 |
| **入侵检测** | ❌ 无 | Type = 12, 13<br/>`INTRUSION_DET_*` | - | ❌ 不使用 |

---

## 🎯 工具兼容性矩阵

| 固件 | 配置文件 | SDK Visualizer | InCabin GUI | 说明 |
|-----|---------|----------------|-------------|------|
| **标准mmwave_demo** | 6844_profile_4T4R_tdm.cfg | ✅ 能用 | ❌ 不能 | 标准Demo使用Type=1 |
| **InCabin固件** | incabin_compatible.cfg | ❌ 不能 | ✅ 能用 | InCabin使用Type=3001 |
| **AWRL6844_HealthDetect** | 自定义.cfg | ✅ 能用 | ❌ 不能 | **必须遵循标准格式** |

### 工具路径

| 工具 | 路径 | 适用固件 |
|-----|------|---------|
| SDK Visualizer | `C:\ti\MMWAVE_L_SDK_06_01_00_01\tools\visualizer\` | 标准Demo / HealthDetect |
| InCabin GUI | `C:\ti\radar_toolbox_3_30_00_06\tools\visualizers\AWRL6844_Incabin_GUI\src\occupancy_demo_gui.exe` | InCabin Demo |

---

## 🔧 AWRL6844_HealthDetect TLV 格式规范

### 核心TLV（兼容标准Demo）

```c
typedef enum MmwDemo_output_message_type_e
{
    /*! @brief 点云数据 - 必须使用Type=1 */
    MMWDEMO_OUTPUT_MSG_DETECTED_POINTS = 1,
    
    /*! @brief Range Profile */
    MMWDEMO_OUTPUT_MSG_RANGE_PROFILE = 2,
    
    /*! @brief Noise Profile */
    MMWDEMO_OUTPUT_MSG_NOISE_PROFILE = 3,
    
    /*! @brief Stats统计 */
    MMWDEMO_OUTPUT_MSG_STATS = 6,
    
    /*! @brief 点云SNR/Noise Side Info */
    MMWDEMO_OUTPUT_MSG_DETECTED_POINTS_SIDE_INFO = 7,
    
    // ... 其他标准类型参考mmwave_demo.h
    
} MmwDemo_output_message_type;
```

### 扩展TLV（健康检测专用）

**从Type=1000开始，避开标准范围(1-299)和官方扩展范围(300-399)**

```c
// 健康检测专用TLV（从1000开始）
#define MMWDEMO_OUTPUT_MSG_PRESENCE_DETECT      1000  // 人存检测结果
#define MMWDEMO_OUTPUT_MSG_HEALTH_FEATURES      1001  // 健康特征向量
#define MMWDEMO_OUTPUT_MSG_VITAL_SIGNS          1002  // 生命体征
#define MMWDEMO_OUTPUT_MSG_POSTURE_RESULT       1003  // 姿态检测结果
#define MMWDEMO_OUTPUT_MSG_FALL_DETECTION       1004  // 跌倒检测告警
```

### 设计原则

| 原则 | 说明 | 影响 |
|-----|------|------|
| **核心TLV完全兼容** | Type 1-12使用标准格式 | SDK Visualizer正常显示 |
| **扩展TLV不冲突** | Type 1000+自定义 | SDK Visualizer安全忽略 |
| **点云必须Type=1** | 最关键的兼容性要求 | 决定工具是否可用 |

---

## 🔍 为什么InCabin不能用SDK Visualizer？

### SDK Visualizer解析逻辑

```python
def parse_tlv(tlv_type, tlv_data):
    if tlv_type == 1:      # 标准Demo的点云
        parse_point_cloud()  # ✅ 能解析
        display_points()     # ✅ 显示点云
        
    elif tlv_type == 3001: # InCabin的点云
        skip()               # ❌ 不认识这个ID
                             # ❌ 跳过这个TLV块
                             # ❌ 导致Points Detected = 0
```

### 现象说明

| 症状 | 原因 | 解决方案 |
|-----|------|---------|
| SDK Visualizer显示Points=0 | InCabin使用Type=3001 | 使用InCabin GUI |
| WebSocket有数据但无显示 | TLV Type不识别 | 检查固件类型 |
| Range Profile正常但无点云 | 只有点云Type不同 | 确认点云使用Type=1 |

---

## 📊 数据流对比

### 标准mmWave Demo + SDK Visualizer ✅（正确）

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

### InCabin Demo + SDK Visualizer ❌（错误）

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

### AWRL6844_HealthDetect + SDK Visualizer ✅（目标）

```
雷达硬件
   ↓
HealthDetect固件
   ↓
UART输出: Type=1 (点云), Type=6 (Stats), Type=1000+ (健康数据)
   ↓
SDK Visualizer Python后端
   ↓ (识别Type=1, 忽略Type=1000+)
解析点云数据（健康数据被安全忽略）
   ↓
点云正常显示 ✅
```

---

## 💡 为什么InCabin使用独有格式？

### 原因分析

| 原因 | 说明 |
|-----|------|
| **AI处理输出** | 需要传输CNN分类器的结果（Type=1041） |
| **多区域监控** | 需要传输每个座位的占用状态（Type=3002） |
| **身高估计** | CPD模式需要区分婴儿/成人（Type=1042） |
| **数据优化** | 量化数据节省UART带宽（8字节点vs20字节点） |
| **专用GUI** | TI为InCabin开发了专用可视化工具 |

### 为什么HealthDetect不采用InCabin格式

| 考虑因素 | InCabin格式 | 标准格式 | HealthDetect选择 |
|---------|------------|---------|------------------|
| SDK Visualizer | ❌ 不兼容 | ✅ 兼容 | ✅ 标准 |
| 开发调试便利性 | ❌ 需专用工具 | ✅ 官方工具可用 | ✅ 标准 |
| 文档资料 | ⚠️ 有限 | ✅ 丰富 | ✅ 标准 |
| 后续维护 | ❌ 复杂 | ✅ 简单 | ✅ 标准 |
| 功能需求 | 汽车座舱 | 通用 | 健康检测 |

---

## 📝 快速诊断指南

### 症状：雷达运行，但SDK Visualizer显示Points Detected = 0

**检查步骤**：

1. ✅ **确认固件类型**
   ```
   如果是InCabin固件 → 必须用InCabin GUI ⚠️
   如果是标准Demo固件 → 可以用SDK Visualizer ✅
   如果是HealthDetect固件 → 可以用SDK Visualizer ✅
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
   // 在mmwave_output.h中检查
   如果MMWDEMO_OUTPUT_MSG_DETECTED_POINTS = 1 → ✅ 正确
   如果MMWDEMO_OUTPUT_MSG_POINT_CLOUD = 3001 → ❌ 需要修改
   ```

---

## 📚 参考资料

### 源码位置

| 文件 | 路径 | 说明 |
|-----|------|------|
| 标准Demo TLV定义 | `C:\ti\mmwave_l_sdk_06_01_00_01\examples\mmw_demo\mmwave_demo\source\mmwave_demo.h` | Line 1296-1350 |
| InCabin TLV定义 | `project-code\AWRL6844_InCabin_Demos\src\mss\source\mmwave_demo_mss.h` | Line 1533-1574 |
| HealthDetect TLV定义 | `project-code\AWRL6844_HealthDetect\mss\include\mmwave_output.h` | 新建文件 |

### 相关文档

- [TLV数据格式快速参考.md](./TLV数据格式快速参考.md) - 快速查阅表
- [Part14-TLV数据格式与工具兼容性完整指南](../06-SDK固件研究/Part14-TLV数据格式与工具兼容性完整指南.md) - 详细技术分析

---

## 🎓 总结

### 核心要点

| 要点 | 说明 |
|-----|------|
| **点云必须Type=1** | 这是最关键的兼容性要求 |
| **扩展从Type=1000开始** | 避免与标准/官方扩展冲突 |
| **SDK Visualizer可用** | 开发调试的必要条件 |
| **不使用InCabin格式** | 尽管功能更丰富，但工具不兼容 |

### 决策依据

```
InCabin格式优点：数据紧凑、功能丰富
InCabin格式缺点：SDK Visualizer不可用、需要专用工具

标准格式优点：官方工具兼容、文档丰富、维护简单
标准格式缺点：需要自定义扩展Type

HealthDetect选择：标准格式 + 自定义扩展（Type 1000+）
理由：开发调试便利性 > 数据紧凑性
```

---

**结论**: 🎯 **工具选择必须匹配固件类型，HealthDetect必须使用标准Demo TLV格式！**
