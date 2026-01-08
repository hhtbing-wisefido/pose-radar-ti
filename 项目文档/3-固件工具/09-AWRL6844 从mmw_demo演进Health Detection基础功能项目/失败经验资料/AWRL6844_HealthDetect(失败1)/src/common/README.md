/**
 * @file README.md
 * @brief common/ 共享接口层说明
 * 
 * 参考：第3章 3.4.2节 src/common/ 关键目录说明
 */

# common/ 共享接口层

## 🎯 作用

定义MSS（R5F）和DSS（C66x）**都需要使用**的数据结构和接口。

这是第3章架构的核心：通过共享头文件，MSS和DSS使用统一的数据定义，通过共享RAM传递数据。

---

## 📁 文件说明

| 文件 | 作用 | 参考 |
|------|------|------|
| `shared_memory.h` | L3共享RAM地址映射 | 第3章 3.3.3节 |
| `health_detect_types.h` | 健康检测专用类型 | 第3章 3.4.2节 |

---

## 🔄 数据流设计

```
MSS (R5F) 侧                     DSS (C66x) 侧
──────────                       ─────────────

【配置阶段】
写入 DPC_CONFIG_BASE     →      读取 DPC_CONFIG_BASE
通过 Mailbox 通知       →

【执行阶段】
                        ←      HWA执行FFT/CFAR
                        ←      C66x计算特征
                        ←      写入 FEATURE_DATA_BASE
                        ←      通过 Mailbox 通知完成

【结果阶段】
读取 FEATURE_DATA_BASE   ←
执行应用算法（人存/姿态/跌倒）
TLV输出
```

---

## 🔥 关键设计点

### 1. 共享RAM区域（896KB）

```
0x51000000 - 0x51001000  (4KB)   DPC配置区
0x51001000 - 0x51003000  (8KB)   点云数据区
0x51003000 - 0x51004000  (4KB)   特征数据区 🔥
0x51004000 - 0x51005000  (4KB)   DPC结果区
0x51005000 - 0x510E0000  (876KB) DSS工作缓冲区
```

### 2. 特征数据结构

`HealthDetect_PointCloudFeatures_t` 包含：
- **质心**：centerX/Y/Z
- **分布**：spreadXY/Z
- **速度**：avgVelocity/maxVelocity
- **统计**：numPoints

这些特征由 **DSS/C66x计算**，供 **MSS/R5F决策** 使用。

---

## 📋 使用方法

### MSS侧使用

```c
#include "common/shared_memory.h"
#include "common/health_detect_types.h"

// 读取DSS计算的特征
HealthDetect_PointCloudFeatures_t *features = 
    (HealthDetect_PointCloudFeatures_t *)FEATURE_DATA_BASE;

// 基于特征做决策
if (features->numPoints > 10) {
    // 人存检测逻辑
}
```

### DSS侧使用

```c
#include "common/shared_memory.h"
#include "common/health_detect_types.h"

// 计算特征并写入共享RAM
HealthDetect_PointCloudFeatures_t *features = 
    (HealthDetect_PointCloudFeatures_t *)FEATURE_DATA_BASE;

features->centerX = ...;  // 计算质心
features->spreadXY = ...; // 计算分布
```

---

> 📚 详细架构设计见：`项目文档/3-固件工具/08-AWRL6844雷达健康检测实现方案/AWRL6844雷达健康检测-06-第3章-架构演进规划.md`
