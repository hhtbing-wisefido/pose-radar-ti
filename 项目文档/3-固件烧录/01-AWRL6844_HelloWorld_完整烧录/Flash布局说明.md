# 📊 AWRL6844 QSPI Flash 布局说明

> **基于**: TI官方SDK文档和SBL源码分析

---

## 🗺️ 完整内存布局

```
┌─────────────────────────────────────────────────────────────┐
│            AWRL6844 QSPI Flash 内存布局 (2MB)                │
├──────────────┬──────────────┬─────────────┬──────────────────┤
│   地址范围    │    大小      │    内容     │      说明         │
├──────────────┼──────────────┼─────────────┼──────────────────┤
│ 0x00000000   │   0x2000     │ Flash Header│ ROM/Flash头+保留  │
│ 0x00001FFF   │   (8 KB)     │ & 保留区    │ (包含SBL头部)     │
├──────────────┼──────────────┼─────────────┼──────────────────┤
│ 0x00002000   │   ~0x3E000   │ SBL Meta    │ 二级引导程序      │
│ 0x00041FFF   │   (~248 KB)  │ Image       │ (M_META_SBL_OFFSET)|
├──────────────┼──────────────┼─────────────┼──────────────────┤
│ 0x00042000   │   ≤0x1BE000  │ Application │ 应用Meta Image    │
│ 0x001FFFFF   │   (≤1.784MB) │ Meta Image  │ (M_META_IMAGE_OFFSET)
└──────────────┴──────────────┴─────────────┴──────────────────┘
```

---

## 📍 关键地址定义

### 1. Flash Header & 预留区 (0x00000000)

- **位置**: 0x0 - 0x1FFF（8KB，包含Flash Header+预留）
- **作用**: 由工具生成的Flash Header，供ROM Bootloader识别；后续区域预留给SBL头部和对齐需求。
- **来源**: `metaImage_creator.exe`在生成`sbl.release.appimage`时自动写入。

---

### 2. SBL Bootloader (0x00002000)

- **位置**: 0x2000 - 0x41FFF（约248KB）
- **宏定义**: `M_META_SBL_OFFSET`（见`sbl.h`）
- **文件**: `sbl.release.appimage`（由`sbl_r5_img.release.rig`生成的meta image）

**功能**:
- ✅ 初始化硬件（时钟、DDR、外设）
- ✅ 解析Meta Image格式
- ✅ 从Flash加载应用到RAM
- ✅ 验证应用完整性
- ✅ 跳转到应用入口点

**启动流程**:
```
ROM Bootloader → 读取Flash Header → 加载SBL到RAM → 执行SBL
```

**源码位置**:
```
MMWAVE_L_SDK_06_01_00_01/examples/drivers/boot/sbl/
```

---

### 3. Application Meta Image (0x00042000)

- **位置**: 0x42000 - 0x1FFFFF（最大1.784MB）
- **宏定义**: `M_META_IMAGE_OFFSET = 0x42000`（由`M_META_SBL_OFFSET`加256KB计算）
- **大小上限**: `SBL_MAX_METAIMAGE_SIZE = 1784 * 1024`（见`sbl.h`）
- **文件**: `hello_world_system.release.appimage`（由`metaimage_cfg.release.json`生成）

**结构**（`SBL_METAHEADER_SIZE = 512B`）:
```
Meta Image = Meta Header(512B) + R5F Core + DSP Core + RF Firmware
```

**各部分说明**:

| 组件 | 大小 | 功能 |
|------|------|------|
| Meta Header | ~1KB | 描述多核镜像结构 |
| R5F Core Image | ~100KB | ARM R5F主控代码 |
| DSP Core Image | ~50KB | C66x DSP信号处理 |
| RF Firmware | ~20KB | 射频前端固件 |

**加载过程**:
```
SBL读取Meta Header → 解析各核镜像地址 → 依次加载到RAM → 启动各核
```

---

## 🔄 启动流程详解

### 完整启动链

```
┌─────────────┐
│ Power On /  │
│   Reset     │
└──────┬──────┘
       │
       ▼
┌─────────────────────────────┐
│  ROM Bootloader (片上ROM)    │
│  - 初始化最小系统             │
│  - 检测启动模式(SOP)          │
│  - 读取Flash Header(0x0)     │
└──────┬──────────────────────┘
       │
       ▼
┌─────────────────────────────┐
│  加载SBL到SRAM (Flash偏移0x2000) │
│  - 根据Header中的size读取    │
│  - 按镜像中的loadAddress放入TCMA/TCMB │
└──────┬──────────────────────┘
       │
       ▼
┌─────────────────────────────┐
│  执行SBL (entryPoint)        │
│  - 初始化外设                │
│  - 解析App Meta Image        │
│  - 加载各核镜像到RAM         │
└──────┬──────────────────────┘
       │
       ▼
┌─────────────────────────────┐
│  启动Application             │
│  - R5F Core 开始执行         │
│  - DSP Core 并行运行         │
│  - RF 初始化                 │
└─────────────────────────────┘
```

---

## 🔧 SOP启动模式

### SOP跳线配置（官方EVM文档）

| SOP模式 | 名称 | 跳线(SOP0,SOP1) | 用途 |
|---------|------|-----------------|------|
| **SOP_MODE1** | Flashing | OFF, OFF | 烧录模式（UART） |
| **SOP_MODE2** | Application | OFF, ON | 正常从Flash启动 |
| **SOP_MODE4** | Debug | ON, ON | 调试模式 |

**重要**:
- 🔧 **烧录时**: SOP_MODE1（OFF, OFF）
- ⭐ **运行时**: SOP_MODE2（OFF, ON）
- ⚠️ 切换SOP前先断电，切换后再上电/复位

---

## 📦 Meta Image 格式

### Meta Image结构

```
┌────────────────────────────────────────────┐
│            Meta Header (512B)              │
│  - Magic Number (验证标识)                  │
│  - Image Count (镜像数量)                  │
│  - Image Info[] (各镜像描述)               │
├────────────────────────────────────────────┤
│            Build Image 1 (R5F)             │
│  - Load Address                            │
│  - Entry Point                             │
│  - Image Size                              │
│  - Binary Data                             │
├────────────────────────────────────────────┤
│            Build Image 2 (DSP)             │
│  - Load Address                            │
│  - Entry Point                             │
│  - Image Size                              │
│  - Binary Data                             │
├────────────────────────────────────────────┤
│            Build Image 3 (RF)              │
│  - Load Address                            │
│  - Image Size                              │
│  - Binary Data                             │
└────────────────────────────────────────────┘
```

### 关键Magic Numbers

| Magic | 值 | 含义 |
|-------|---|------|
| Meta Header Start | `0x5254534D` | Meta Image标识（见`sbl.h`字段`metaHeaderStart`） |
| R5F Core Magic | `0xA95316AD` | APPSS (R5F) |
| DSP Core Magic | `0xD59246F1` | DSS (C66x) |
| RF Core Magic | `0xFECB36DE` | FECSS (RF) |

---

## 🛠️ 烧录地址对照表

### 烧录命令参考

| 烧录内容 | 文件 | 地址 | 命令示例 |
|---------|------|------|----------|
| SBL Bootloader | `sbl.release.appimage` | `0x2000` | `arprog_cmdline_6844.exe -p COM3 -f sbl.release.appimage -o 0x2000` |
| HelloWorld App | `hello_world_system.release.appimage` | `0x42000` | `arprog_cmdline_6844.exe -p COM3 -f hello_world_system.release.appimage -o 0x42000` |

**注意**:
- Flash Header (0x0) **不需要单独烧录**，包含在`*.appimage`里
- 应用起始地址由SBL宏决定：`M_META_IMAGE_OFFSET = 0x42000`

---

## 🔍 Flash容量规划

### AWRL6844标准配置

**QSPI Flash**: S25FL128S (16MB)

| 分区 | 起始地址 | 大小 | 百分比 |
|------|---------|------|--------|
| SBL区 | 0x000000 | 256KB | 1.56% |
| App区 | 0x040000 | 512KB | 3.13% |
| **可用空间** | 0x0C0000 | **15.3MB** | **95.31%** |

### 应用大小参考

| 应用类型 | 典型大小 | 适合容量 |
|---------|---------|---------|
| HelloWorld | ~200KB | ✅ 充足 |
| mmWave Demo | ~350KB | ✅ 充足 |
| InCabin Demo | ~980KB | ✅ 充足 |
| 复杂AI应用 | 2-5MB | ✅ 可用 |

---

## 📚 官方文档参考

### SDK文档位置

```
MMWAVE_L_SDK_06_01_00_01/
├── docs/api_guide_xwrL684x/
│   ├── sbl_8md.html              # SBL用户指南
│   ├── EXAMPLES_DRIVERS_SBL.html # SBL示例文档
│   └── bootloader_architecture.html # 启动架构
│
└── examples/drivers/boot/
    ├── sbl/                      # SBL源码
    ├── sbl_lite/                 # 精简版SBL
    └── spibooting/               # SPI启动示例
```

### 内存布局定义文件（源码中的权威宏）

```c
// MMWAVE_L_SDK_06_01_00_01/examples/drivers/boot/sbl/sbl.h

#define M_META_SBL_OFFSET       (uint32_t)(0x02000U)         // SBL写入Flash的起始偏移
#define M_META_IMAGE_OFFSET     (uint32_t)(0x042000U)        // 应用Meta Image绝对偏移
#define SBL_METAHEADER_SIZE     (512U)                       // Meta Header固定512B
#define SBL_MAX_METAIMAGE_SIZE  (1784U * 1024U)              // 应用meta最大尺寸（SBL会按此擦除）
// Magic words: APPSS=0xA95316AD, DSS=0xD59246F1, FECSS=0xFECB36DE, CERT=0xCE97F1C3
```

---

## ⚠️ 注意事项

### 1. 地址对齐要求
- Flash操作必须256字节对齐
- 镜像大小建议4KB对齐

### 2. 烧录顺序
1. **先烧录SBL** (地址0x2000)
2. 再烧录应用Meta Image (地址0x42000)
3. 顺序错误会导致无法启动

### 3. Flash擦除
- 烧录工具会按照`SBL_MAX_METAIMAGE_SIZE`覆盖范围擦除（约1.784MB，从0x42000开始）
- 擦除粒度取决于Flash器件`blockSize`（可在`Flash_getAttrs`获取）
- 全片擦除会清除所有内容

### 4. 验证方法
```bash
# 读取SBL区域验证（示例）
arprog_cmdline_6844.exe -p COM3 -r 0x2000 -s 0x200 -o verify_sbl.bin
```

---

## 🔗 相关文档

- [README.md](./README.md) - 项目概述
- [操作指南.md](./操作指南.md) - 烧录步骤
- [1-SBL_Bootloader/README.md](./1-SBL_Bootloader/README.md) - SBL详解

---

**更新日期**: 2025-12-12  
**SDK版本**: 06.01.00.01  
**参考来源**: TI官方SDK文档 + 源码分析
