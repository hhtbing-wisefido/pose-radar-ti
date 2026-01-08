# 附录C：InCabin Demo架构学习参考

> **目标**：为第3章架构演进规划提供参考  
> **创建日期**：2026-01-07  
> **适用章节**：第3章 架构演进规划

---

## 📋 核心问题

1. InCabin Demo是否针对AWRL6844？
2. InCabin源代码目录架构是否值得学习？
3. 为什么InCabin作为基础工程失败，但架构仍值得学习？

---

## 1. InCabin Demo基本信息

### 1.1 是否针对AWRL6844？

**✅ 是的，100%专为AWRL6844设计！**

**证据**：
- 工程名称：`AWRL6844_InCabin_Demos`
- SDK路径：`Automotive_InCabin_Security_and_Safety/AWRL6844_InCabin_Demos/`
- 固件名：`demo_in_cabin_sensing_6844_system.release.appimage`
- 专用GUI：`AWRL6844_Incabin_GUI`
- 应用场景：车载座舱监测（Occupancy Detection、Seat Belt Reminder）

### 1.2 设计目标

InCabin Demo是TI专门为AWRL6844开发的**车载座舱监测**解决方案：
- 人员占位检测（Occupancy Detection）
- 安全带提醒（Seat Belt Reminder）
- 入侵检测（Intrusion Detection）

---

## 2. InCabin目录架构分析

### 2.1 完整目录结构

```
AWRL6844_InCabin_Demos/
├── src/
│   ├── common_mss_dss/          # 🔥 MSS和DSS共享代码
│   │   ├── data_path.h         # DPC数据结构定义
│   │   ├── shared_memory.h     # 共享RAM接口定义
│   │   ├── mmwave_output.h     # TLV输出格式
│   │   └── utils.h             # 通用工具函数
│   │
│   ├── mss/                     # 🔥 R5F (APPSS/MSS) 代码
│   │   ├── mmwave_demo_mss.c   # 主控逻辑、CLI、DPC协调
│   │   ├── occupancyClassifier.c   # 人存分类器（CNN）
│   │   ├── cli.c               # CLI命令解析
│   │   ├── sensor_mgmt.c       # 传感器管理
│   │   └── ...
│   │
│   ├── dss/                     # 🔥 C66x DSP (DSS) 代码
│   │   ├── mmwave_demo_dss.c   # DSP侧数据处理入口
│   │   ├── capon_processing.c  # Capon波束成形算法
│   │   ├── feature_extract.c   # 特征提取（点云特征）
│   │   ├── dpu/                # Data Processing Unit
│   │   │   ├── rangeproc/     # Range FFT处理
│   │   │   ├── dopplerproc/   # Doppler FFT处理
│   │   │   └── cfar/          # CFAR检测
│   │   └── ...
│   │
│   └── system/                  # 系统配置
│       ├── linker_mss.cmd      # MSS链接脚本
│       ├── linker_dss.cmd      # DSS链接脚本
│       └── shared_memory.ld    # 共享RAM映射
│
├── docs/                        # 文档
├── prebuilt_binaries/          # 预编译固件
└── firmware/                    # 固件配置
```

### 2.2 关键目录说明

| 目录 | 作用 | 学习价值 |
|------|------|---------|
| **common_mss_dss/** | MSS和DSS共享的头文件和接口 | ⭐⭐⭐⭐⭐ 最重要 |
| **mss/** | R5F主控代码 | ⭐⭐⭐⭐ 学习主控逻辑 |
| **dss/** | C66x DSP算法代码 | ⭐⭐⭐⭐⭐ 学习DSS扩展 |
| **system/** | 链接脚本、内存映射 | ⭐⭐⭐ 学习内存规划 |

---

## 3. 架构设计亮点

### 3.1 多核协同设计 🔥

**InCabin正确利用了AWRL6844的四子系统架构**：

```
┌─────────────────────────────────────────────────────────────┐
│                InCabin多核协同架构                           │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  RF前端 (FECSS)                                              │
│     ↓                                                       │
│  ADC数据                                                     │
│     ↓                                                       │
│  ┌────────────────────────────────────────────────────┐    │
│  │  DSS (C66x DSP @ 450MHz + HWA)                     │    │
│  │  -------------------------------------------------- │    │
│  │  [HWA] Range FFT → Doppler FFT → CFAR             │    │
│  │     ↓                                               │    │
│  │  [C66x DSP]                                         │    │
│  │   ├── Capon波束成形（2D DoA）                       │    │
│  │   ├── 点云特征提取                                   │    │
│  │   └── 结果写入共享RAM                               │    │
│  └────────────────────────────────────────────────────┘    │
│            ↓ (共享RAM: 896KB)                              │
│  ┌────────────────────────────────────────────────────┐    │
│  │  MSS (R5F @ 200MHz)                                │    │
│  │  -------------------------------------------------- │    │
│  │  ├── DPC协调（调度DSS任务）                         │    │
│  │  ├── 从共享RAM读取DSS结果                           │    │
│  │  ├── occupancyClassifier（CNN分类）                │    │
│  │  ├── 人存/姿态决策逻辑                              │    │
│  │  ├── TLV格式化输出                                  │    │
│  │  └── CLI/UART/CAN通信                              │    │
│  └────────────────────────────────────────────────────┘    │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### 3.2 代码组织方式 🔥

#### common_mss_dss/ 的作用

**这是InCabin架构的核心设计！**

```c
// common_mss_dss/data_path.h - MSS和DSS共享的数据结构

typedef struct DPC_ObjectDetection_ExecuteResult
{
    uint32_t numObjOut;              // 检测到的目标数量
    DPIF_PointCloudCartesian *objOut; // 点云数据（共享RAM地址）
    DPIF_PointCloudSideInfo *objOutSideInfo; // 点云附加信息
    // ... 更多字段
} DPC_ObjectDetection_ExecuteResult_t;
```

```c
// common_mss_dss/shared_memory.h - 共享RAM接口

#define SHARED_RAM_BASE_ADDR    0x...
#define SHARED_RAM_SIZE         (896 * 1024)  // 896KB

// MSS侧写入，DSS侧读取
#define DPC_CONFIG_BASE         (SHARED_RAM_BASE_ADDR + 0x0000)
// DSS侧写入，MSS侧读取
#define DPC_RESULT_BASE         (SHARED_RAM_BASE_ADDR + 0x1000)
```

**这正是你第3章需要设计的！**

### 3.3 算法部署策略 🔥

| 算法模块 | 部署位置 | 原因 |
|---------|---------|------|
| **Range FFT** | HWA | 硬件加速，释放CPU |
| **Doppler FFT** | HWA | 硬件加速 |
| **CFAR检测** | HWA | 硬件加速 |
| **Capon波束成形** | C66x DSP | 复杂矩阵运算，需要DSP算力 |
| **点云特征提取** | C66x DSP | 浮点运算，450MHz算力 |
| **CNN分类器** | R5F | 推理较快，可在主核 |
| **决策逻辑** | R5F | 状态机、规则判断 |
| **CLI/通信** | R5F | 外设丰富 |

**核心思路**：
- **计算密集型** → DSS（C66x DSP @ 450MHz）
- **控制逻辑型** → MSS（R5F @ 200MHz）
- **硬件加速型** → HWA

---

## 4. 对比：标准mmw_demo vs InCabin

### 4.1 架构对比

| 项目 | 标准mmw_demo | InCabin Demo | 你的目标架构 |
|------|-------------|--------------|-------------|
| **R5F使用** | 100%负载 ❌ | 主控+决策 ✅ | 学InCabin ✅ |
| **DSS使用** | 0%（闲置）❌ | 特征提取、算法 ✅ | 学InCabin ✅ |
| **共享RAM** | 未使用 ❌ | 高效数据交换 ✅ | 学InCabin ✅ |
| **代码组织** | 单一mss/ ❌ | mss/ + dss/ + common/ ✅ | 学InCabin ✅ |
| **算法复杂度** | 基础点云 | 高级算法（Capon、CNN） | 学InCabin ✅ |
| **点云可靠性** | ✅ 稳定 | ❌ 需相位校准 | 用标准Demo ✅ |

### 4.2 性能对比

**标准mmw_demo**：
- R5F @ 200MHz：100%使用
- C66x DSP @ 450MHz：0%使用（闲置）
- 算力浪费：60%

**InCabin Demo**：
- R5F @ 200MHz：50%使用（主控）
- C66x DSP @ 450MHz：80%使用（算法）
- 算力利用率：90%+

---

## 5. InCabin失败原因分析

### 5.1 为什么作为基础工程失败？

**实战测试5次全部失败（Points=0）**，原因：

1. **相位校准要求极高** ❌
   - Capon算法对相位精度要求极高（误差<1°）
   - 标准校准流程无法满足
   - 需要专业设备和环境

2. **需要DCA1000** ❌
   - InCabin设计时假设有DCA1000采集原始数据
   - 用于相位校准和算法调试
   - 我们没有此设备

3. **TLV格式不兼容** ❌
   - InCabin使用专有TLV格式（Type=3001）
   - SDK Visualizer无法解析
   - 调试困难

4. **杂波抑制过于激进** ❌
   - 为车载场景优化（金属座椅、车体反射）
   - 室内场景可能把人也过滤掉

**详见**：[02-方案确认.md](./AWRL6844雷达健康检测-02-方案确认.md) - 第2部分实战测试

### 5.2 为什么架构仍值得学习？

**InCabin的架构设计是成功的！** ✅

| 设计方面 | 评价 | 学习价值 |
|---------|------|---------|
| **多核协同思路** | ✅ 优秀 | 必须学习 |
| **代码组织方式** | ✅ 清晰 | 直接参考 |
| **共享RAM使用** | ✅ 高效 | 完全可复用 |
| **算法部署策略** | ✅ 合理 | 必须学习 |
| **DPC扩展方法** | ✅ 标准 | 必须学习 |

**结论**：
- ❌ 不用InCabin的固件和具体算法（Capon）
- ✅ **学习InCabin的架构设计思路**
- ✅ 用标准mmw_demo做基础（点云可靠）
- ✅ **参考InCabin的多核组织方式扩展**

---

## 6. 第3章应该如何参考InCabin

### 6.1 Phase 3A：学习InCabin架构

**必读文件**（按优先级）：

| 文件 | 路径 | 学习重点 |
|------|------|---------|
| 1️⃣ data_path.h | common_mss_dss/ | DPC数据结构定义 |
| 2️⃣ shared_memory.h | common_mss_dss/ | 共享RAM接口 |
| 3️⃣ mmwave_demo_mss.c | mss/ | 主控逻辑、DPC协调 |
| 4️⃣ mmwave_demo_dss.c | dss/ | DSS侧处理流程 |
| 5️⃣ feature_extract.c | dss/ | 特征提取实现 |

**学习任务**：
1. 理解 `common_mss_dss/` 如何定义共享接口
2. 理解DPC如何协调MSS和DSS
3. 理解共享RAM的数据传递机制
4. 理解DSS算法的组织方式

### 6.2 Phase 3B：设计你的架构

**参考InCabin设计你的目录结构**：

```
你的目标架构（参考InCabin）：

AWRL6844_HealthDetect/src/
├── common/                      # 🔥 学习 common_mss_dss/
│   ├── data_path.h             # DPC数据结构（学InCabin）
│   ├── shared_memory.h         # 共享RAM接口（学InCabin）
│   └── health_detect_types.h   # 健康检测专用类型
│
├── mss/                         # 🔥 学习 InCabin/mss/
│   ├── mmwave_demo_mss.c       # 主控（扩展标准Demo）
│   ├── presence_detect.c       # 人存检测
│   ├── pose_detect.c           # 姿态检测（决策逻辑）
│   ├── fall_detect.c           # 跌倒检测
│   └── health_monitor.c        # 主健康监测逻辑
│
└── dss/                         # 🔥 学习 InCabin/dss/
    ├── mmwave_demo_dss.c       # DSS入口（扩展标准Demo）
    ├── feature_extract.c       # 特征提取（学InCabin）
    └── dsp_utils.c             # DSP工具函数
```

### 6.3 设计重点（学InCabin）

#### 重点1：共享接口设计

```c
// common/data_path.h - 学习InCabin的接口定义方式

typedef struct HealthDetect_PointCloudFeatures
{
    float centerX, centerY, centerZ;  // 质心
    float spreadXY, spreadZ;          // 分布
    float avgVelocity;                // 平均速度
    uint32_t numPoints;               // 点云数量
} HealthDetect_PointCloudFeatures_t;

// DSS计算特征 → 写入共享RAM
// MSS读取特征 → 姿态分类
```

#### 重点2：算法部署策略

| 算法 | 部署位置 | 理由（学InCabin） |
|------|---------|-----------------|
| **点云特征提取** | DSS/C66x | 浮点运算多，需450MHz算力 |
| **规则分类器** | MSS/R5F | 简单逻辑，R5F够用 |
| **状态机** | MSS/R5F | 控制逻辑，适合R5F |

#### 重点3：共享RAM规划

```
共享RAM (896KB) 使用规划：

0x00000000 - 0x00001000 (4KB)   : DPC配置区 (MSS写，DSS读)
0x00001000 - 0x00002000 (4KB)   : 点云数据 (DSS写，MSS读)
0x00002000 - 0x00003000 (4KB)   : 特征数据 (DSS写，MSS读)
0x00003000 - 0x000E0000 (844KB) : 中间缓存

（参考InCabin的shared_memory.h）
```

---

## 7. 关键文件清单

### 7.1 InCabin源代码位置

| 类型 | 路径 |
|------|------|
| **项目源码** | `project-code/AWRL6844_InCabin_Demos/src/` |
| **SDK源码** | `C:\ti\radar_toolbox_3_30_00_06\source\ti\examples\Automotive_InCabin_Security_and_Safety\AWRL6844_InCabin_Demos\` |

### 7.2 重要参考文档

| 文档 | 位置 | 用途 |
|------|------|------|
| **Point Cloud Tuning Guide** | docs/ | 理解Capon算法 |
| **User Guide** | docs/ | 理解整体流程 |
| **TLV格式对比** | ../06-SDK固件研究/ | 理解数据格式 |

---

## 8. 实施建议

### 8.1 第3章Phase 3A任务

**Day 1-2：阅读InCabin代码**
- [ ] 阅读 `common_mss_dss/data_path.h`
- [ ] 阅读 `common_mss_dss/shared_memory.h`
- [ ] 理解共享数据结构定义

**Day 3：对比MSS和DSS代码**
- [ ] 阅读 `mss/mmwave_demo_mss.c` 主控逻辑
- [ ] 阅读 `dss/mmwave_demo_dss.c` DSS处理
- [ ] 理解DPC协调机制

### 8.2 第3章Phase 3B任务

**基于InCabin设计你的架构**：
- [ ] 创建 `common/` 目录结构
- [ ] 定义健康检测专用数据结构
- [ ] 规划共享RAM使用
- [ ] 决定算法部署策略（MSS vs DSS）
- [ ] 设计DSS特征提取接口

### 8.3 输出文档

**参考InCabin完成以下文档**：
- [ ] 《DPC机制学习笔记》（含InCabin对比）
- [ ] 《DSS算法扩展架构设计》（参考InCabin）
- [ ] 《共享RAM使用规范》（参考InCabin）
- [ ] 《目录结构规划》（对比InCabin）

---

## 9. 常见问题

### Q1：InCabin的Capon算法需要学吗？

**A**：不需要。Capon算法太复杂，且需要相位校准。你用标准Demo的基础点云就够了。

**但要学习**：
- ✅ Capon如何在DSS实现（代码组织）
- ✅ 特征提取的思路（centerX/Y/Z、spread等）
- ✅ DSS和MSS的数据传递机制

### Q2：InCabin的occupancyClassifier要用吗？

**A**：看情况。

- **Phase 1**：先用规则分类器（简单、风险低）
- **Phase 2**：如果规则准确率不够，再研究CNN

**但要学习**：
- ✅ CNN分类器如何集成到系统
- ✅ 特征如何从DSS传递到MSS
- ✅ 分类结果如何输出

### Q3：必须完全按照InCabin的架构吗？

**A**：不必完全一样，但核心思路要学。

**必须学的**：
- ✅ common/ 共享接口的设计思路
- ✅ MSS和DSS的分工原则
- ✅ 共享RAM的使用方式

**可以调整的**：
- 具体算法实现（规则 vs CNN）
- 目录命名（common vs common_mss_dss）
- 数据结构细节（根据需求调整）

---

## 10. 总结

### 核心要点

1. **InCabin是AWRL6844专用Demo** ✅
2. **InCabin正确利用了多核架构** ✅
3. **InCabin的代码组织方式值得学习** ✅
4. **InCabin作为基础工程失败，但架构设计成功** ✅

### 实施策略

```
你的方案 = 标准mmw_demo的基础 + InCabin的架构思路

├── 基础（标准mmw_demo）
│   ├── ✅ 可靠的点云输出
│   ├── ✅ 无需相位校准
│   └── ✅ SDK Visualizer支持
│
└── 架构（参考InCabin）
    ├── ✅ common/ 共享接口
    ├── ✅ MSS/DSS分工
    ├── ✅ 共享RAM使用
    └── ✅ DSS算法部署
```

### 给第3章的建议

**Phase 3A（学习）**：
- 深入分析InCabin的 `common_mss_dss/` 和 `dss/`
- 理解多核协同的实现机制
- 学习共享RAM的使用方式

**Phase 3B（设计）**：
- 参考InCabin设计你的目录结构
- 明确算法在MSS和DSS的部署
- 定义共享数据结构和接口

**不要做的**：
- ❌ 复制InCabin的Capon算法
- ❌ 使用InCabin的固件作为基础
- ❌ 盲目照搬所有代码

**要做的**：
- ✅ 学习架构设计思路
- ✅ 参考代码组织方式
- ✅ 在标准Demo基础上扩展

---

## 11. InCabin的编译打包机制 🔥

### 11.1 为什么InCabin能"系统编译"生成.appimage？

**核心发现**：InCabin看起来是"系统编译"，但实际是：

```
真相：分别编译 + 后处理打包

Step 1: 分别编译MSS和DSS ✅ (和标准mmw_demo一样)
Step 2: 使用TI工具打包成.appimage ✅ (额外的一步)
```

### 11.2 系统项目配置分析

#### system.xml - 多核项目协调

```xml
<system>
    <!-- 项目1: MSS (R5F) -->
    <project id="project_0" name="demo_in_cabin_sensing_6844_mss">
    </project>
    <core id="Cortex_R5_0" project="project_0"/>
    
    <!-- 项目2: DSS (C66x) -->
    <project id="project_1" name="demo_in_cabin_sensing_6844_dss">
    </project>
    <core id="C66xx_DSP" project="project_1"/>
    
    <!-- 后处理步骤：打包成.appimage -->
    <postBuildSteps>
        <step command="$(MAKE) -f makefile_system_ccs_bootimage_gen"/>
    </postBuildSteps>
</system>
```

**翻译成人话**：
1. 先用ARM编译器编译MSS项目 → 生成 `mss.out` (ARM指令集)
2. 再用C6000编译器编译DSS项目 → 生成 `dss.out` (DSP指令集)
3. 执行后处理脚本 → 打包成单个 `.appimage` 文件

### 11.3 .appimage文件结构

```
demo_in_cabin_sensing_6844_system.release.appimage
├─────────────────────────────────────────────────
│ Meta Header (元信息)
│  ├─ 镜像类型: multi-core
│  ├─ 安全类型: GP (General Purpose)
│  └─ Flash索引: 1
├─────────────────────────────────────────────────
│ MSS Image (.rig格式)
│  ├─ MSS.out (ARM R5F代码)
│  ├─ 加载地址: 0x00000000
│  └─ 大小: ~500KB
├─────────────────────────────────────────────────
│ DSS Image (.rig格式)
│  ├─ DSS.out (C66x DSP代码)
│  ├─ 加载地址: 0x00800000
│  └─ 大小: ~200KB
├─────────────────────────────────────────────────
│ RF Firmware Patch
│  ├─ mmwave_rfs_patch.rig
│  └─ 雷达前端固件
├─────────────────────────────────────────────────
│ Certificate (可选)
│  └─ 用于安全启动验证
└─────────────────────────────────────────────────

总大小: ~750KB
烧录位置: Flash起始地址
一次烧录: ✅ 包含所有内容
```

### 11.4 metaImage_creator工具原理

```bash
# makefile_system_ccs_bootimage_gen关键步骤

# Step 1: 复制MSS和DSS的.rig文件到临时目录
COPY demo_in_cabin_sensing_6844_mss_img.release.rig → temp/
COPY demo_in_cabin_sensing_6844_dss_img.release.rig → temp/

# Step 2: 准备配置文件（JSON格式）
CREATE metaimage_cfg.release.json
  {
    "buildImages": [
      {"buildImagePath": "temp/mss_img.rig"},   // MSS固件
      {"buildImagePath": "temp/dss_img.rig"},   // DSS固件
      {"buildImagePath": "mmwave_rfs_patch.rig"} // RF固件
    ],
    "metaImageFile": "system.release.appimage"   // 输出文件
  }

# Step 3: 调用TI官方打包工具
cd $(MCU_PLUS_SDK_PATH)/tools/MetaImageGen
./metaImage_creator --complete_metaimage metaimage_cfg.json

# Step 4: 生成最终.appimage
Output: demo_in_cabin_sensing_6844_system.release.appimage ✅
```

### 11.5 为什么需要分别编译？

#### 技术原因（必须）

| 原因 | MSS | DSS |
|-----|-----|-----|
| **CPU架构** | ARM Cortex-R5F | C66x DSP |
| **指令集** | ARM指令集 | TI C6000指令集 |
| **编译器** | TI ARM Compiler (armcl) | TI C6000 Compiler (cl6x) |
| **二进制格式** | ARM COFF/ELF | C6000 COFF/ELF |
| **内存视角** | L3 @ 0x51000000 | L3 @ 0xC0000000 |

**结论**：❌ 不可能用一个编译器编译两种不同的CPU架构！

#### 工程优势（更好）

```
分别编译的好处：
├─ ✅ 可以单独调试MSS或DSS
├─ ✅ 可以单独更新某个核心的固件
├─ ✅ 编译速度快（并行编译）
├─ ✅ 模块化开发更清晰
└─ ✅ 便于版本管理和回滚

系统打包的好处：
├─ ✅ 一个文件包含所有内容
├─ ✅ 版本一致性保证
├─ ✅ 一次烧录完成
└─ ✅ 发布和部署方便
```

### 11.6 启动流程

```
上电启动流程：

1. ROM Bootloader (固化在芯片)
   ↓
2. 读取Flash中的.appimage
   ├─ 解析Meta Header
   ├─ 验证签名（如果启用）
   └─ 提取各个镜像
   ↓
3. 加载MSS Image
   ├─ 复制MSS.out到MSS内存 (TCMA, L3)
   ├─ 设置MSS启动地址
   └─ 启动R5F核心
   ↓
4. MSS启动后自己加载DSS Image
   ├─ 复制DSS.out到DSS内存 (L2, L3)
   ├─ 配置DSS启动地址
   └─ 通过IPC Mailbox唤醒C66x核心
   ↓
5. 两个核心并行运行
   ├─ MSS: 主控制、配置、通信
   └─ DSS: 信号处理、算法计算
```

### 11.7 对比两种方式

#### 方式1: 传统分别编译烧录

```
开发阶段使用：
├─ Build MSS Project → MSS.out
├─ Build DSS Project → DSS.out
└─ UniFlash分别烧录
    ├─ 烧录MSS.out到地址0x00000000
    └─ 烧录DSS.out到地址0x00100000

优点：
✅ 简单直接，快速迭代
✅ 调试方便（可以单独更新某个核心）
✅ 适合开发阶段

缺点：
❌ 需要两次烧录操作
❌ 容易版本不匹配
❌ 发布时需要管理多个文件
```

#### 方式2: 系统项目打包

```
发布阶段使用：
└─ Build System Project
    ├─ 自动编译MSS → mss.rig
    ├─ 自动编译DSS → dss.rig
    ├─ 打包RF固件 → rfs.rig
    └─ 生成.appimage (单文件)

烧录：
└─ UniFlash一次烧录.appimage

优点：
✅ 一次编译完成所有核心
✅ 一次烧录完成
✅ 版本一致性保证
✅ 发布和部署方便
✅ 可以包含签名和加密

缺点：
❌ 打包过程较慢
❌ 调试时需要重新打包
❌ 配置较复杂
```

### 11.8 类比理解

就像一台电脑的操作系统：

```
MSS.out = Windows操作系统 (x86)
DSS.out = 显卡驱动程序 (GPU代码)

你不能用x86编译器编译GPU代码！
但你可以把它们打包成一个安装包（.appimage）
```

### 11.9 学习要点

**对于AWRL6844_HealthDetect项目**：

**Phase 1 - 开发阶段**（当前）：
- ✅ 保持分别编译MSS/DSS
- ✅ 使用两个.projectspec文件
- ✅ 快速调试和迭代
- ⏭️ 暂不需要系统打包

**Phase 2 - 发布阶段**（将来）：
- ✅ 添加system项目配置
- ✅ 创建system.xml和makefile
- ✅ 配置metaimage_cfg.json
- ✅ 生成.appimage发布

**关键文件参考**（从InCabin学习）：
```
参考InCabin的文件：
├─ src/system/demo_in_cabin_sensing_6844_system.projectspec
├─ src/system/system.xml
├─ src/system/makefile_system_ccs_bootimage_gen
└─ src/system/config/metaimage_cfg.release.json

复制到你的项目（将来）：
├─ src/system/health_detect_system.projectspec
├─ src/system/system.xml
├─ src/system/makefile_system_ccs_bootimage_gen
└─ src/system/config/metaimage_cfg.release.json
```

---

## 相关文档

- [02-方案确认](./AWRL6844雷达健康检测-02-方案确认.md) - InCabin方案为何失败
- [03-实施目录大纲](./AWRL6844雷达健康检测-03-实施目录大纲.md) - 第3章详细规划
- [附录B-SDK源码架构分析](./AWRL6844雷达健康检测-附录B-SDK源码架构分析.md) - 标准Demo架构
- [InCabin与标准Demo数据格式对比](../06-SDK固件研究/InCabin与标准Demo数据格式对比.md) - TLV格式差异

---

> 📌 **关键提示**：InCabin虽然作为基础工程失败了，但它展示了如何正确利用AWRL6844的多核架构。这正是你第3章最需要学习的！
