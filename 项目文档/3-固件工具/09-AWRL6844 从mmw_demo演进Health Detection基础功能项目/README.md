# 📁 09-AWRL6844 从mmw_demo演进Health Detection基础功能项目

**创建日期**: 2026-01-08  
**项目状态**: 🔄 v2.0重新实施中

---

## 📋 目录说明

本目录存放 AWRL6844 Health Detection 项目的相关文档。

v1.0版本因代码编写时**错误使用了BIOS API**（而非FreeRTOS API）而失败，相关资料已移至 `失败经验资料/` 子目录供参考。

**重要**：v1.0的**三层架构设计、功能规划都是正确的**，只是API选择错误。v2.0继续沿用架构设计，修正API使用。

---

## 📂 目录结构

```
09-AWRL6844 从mmw_demo演进Health Detection基础功能项目/
├── README.md                           # 本文件
├── AWRL6844_HealthDetect需求文档v2.md  # ✅ 当前版本需求文档
│
└── 失败经验资料/                        # v1.0资料（架构设计可参考）
    ├── AWRL6844_HealthDetect(失败1)/   # v1.0代码（API错误）
    ├── AWRL6844 Health Detection 项目需求文档(旧).md  # v1.0需求（架构正确）
    ├── AWRL6844雷达健康检测-附录B-SDK源码架构分析.md
    ├── AWRL6844雷达健康检测-附录C-InCabin架构学习参考.md
    ├── Part13-SDK对比与RTOS深度解析.md
    └── ...
```

---

## 📝 文档索引

| 文档 | 说明 | 状态 |
|------|------|------|
| **AWRL6844_HealthDetect需求文档v2.md** | v2.0需求文档（添加FreeRTOS API规范） | ✅ 当前版本 |
| 失败经验资料/AWRL6844 Health Detection 项目需求文档(旧).md | v1.0需求文档（架构设计正确可参考） | 📚 参考 |
| 失败经验资料/AWRL6844_HealthDetect(失败1)/ | v1.0代码（API错误，架构可参考） | 📚 参考 |

---

## 🔴 v1.0失败原因（仅API错误）

### ❌ 失败点：代码使用了错误的API

| 错误代码（v1.0） | 正确代码 |
|-----------------|---------|
| `#include <ti/sysbios/BIOS.h>` | `#include "FreeRTOS.h"` |
| `Task_create()` | `xTaskCreateStatic()` |
| `BIOS_start()` | `vTaskStartScheduler()` |

### ✅ 正确的部分（保留）

- ✅ **三层架构设计**：common/mss/dss/system - **完全正确**
- ✅ **从零重建方向**：参考mmw_demo重写 - **完全正确**
- ✅ **功能规划**：主控、CLI、DPC、特征提取 - **完全正确**
- ✅ **Milestone规划**：架构→编译→功能→测试 - **完全正确**

---

## 🎯 v2.0 当前目标

继续按照**第3章三层架构**从零重建，修正API使用：

| Milestone | 目标 | 状态 |
|-----------|------|------|
| M1 | 架构重建（使用正确的FreeRTOS API） | 🔄 进行中 |
| M2 | CCS导入+编译通过 | ⏳ |
| M3 | 功能实现 | ⏳ |
| M4 | 硬件测试 | ⏳ |

---

## 📚 相关资源

| 资源 | 路径 | 用途 |
|------|------|------|
| mmw_demo参考源码 | `project-code/mmw_demo_SDK_reference/` | **必须阅读**，学习正确API |
| 新项目目录 | `project-code/AWRL6844_HealthDetect/` | 代码实现位置 |
| 08方案文档 | `项目文档/3-固件工具/08-AWRL6844雷达健康检测实现方案/` | 第3章架构设计 |

---

## 🔴 关键提醒

> **L-SDK使用FreeRTOS，不是TI-RTOS/BIOS！**
> 
> 编写代码前必须阅读 mmw_demo 的 `main.c` 和 `mmwave_demo.c`，学习正确的API用法。

---

> 📌 **核心策略**：保持v1.0的三层架构设计，使用正确的FreeRTOS API从零重建代码
