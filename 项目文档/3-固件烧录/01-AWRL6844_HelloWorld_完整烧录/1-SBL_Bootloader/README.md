# 📦 SBL (Secondary Bootloader)

> **二级引导程序 - 从Flash启动应用的关键组件**

---

## 文件说明

### 1. sbl.release.appimage

**文件大小**: ~130KB  
**来源**: `MMWAVE_L_SDK_06_01_00_01/examples/drivers/boot/sbl/`  
**用途**: SBL Bootloader的原始应用镜像

**包含内容**:
- R5F Core可执行代码
- 初始化代码（时钟、外设）
- Flash读取逻辑
- Meta Image解析器
- 多核加载器

---

### 2. metaimage_cfg.release.json

**用途**: SBL Meta Image生成配置文件

**关键配置项**:
```json
{
  "securityType": "gp",           // General Purpose (非安全启动)
  "flashIndex": "1",             // QSPI Flash
  "metaImageType": "multi",      // 多核镜像
  "buildImages": [                // 核心镜像列表
    {
      "buildImagePath": "sbl_r5_img.release.rig",
      "encryptEnable": "no"        // 不加密
    }
  ],
  "metaImageFile": "sbl.release.appimage"  // 输出文件名
}
```

---

## SBL工作原理

### 启动流程

```
┌─────────────┐
│  Power On   │
└──────┬──────┘
       │
       ▼
┌─────────────────────────┐
│  ROM Bootloader         │
│  - 读取Flash Header     │
│  - 验证Magic Number     │
│  - 获取SBL信息          │
└──────┬──────────────────┘
       │
       ▼
┌─────────────────────────┐
│  加载SBL到SRAM          │
│  - 从0x2000读取         │
│  - 加载到0x00000000     │
│  - 验证校验和            │
└──────┬──────────────────┘
       │
       ▼
┌─────────────────────────┐
│  执行SBL                │
│  - 初始化SOC            │
│  - 初始化QSPI Flash     │
│  - 读取App Meta Image   │
│  - 解析多核镜像          │
│  - 加载到RAM            │
│  - 启动各核             │
└──────┬──────────────────┘
       │
       ▼
┌─────────────────────────┐
│  应用程序运行            │
└─────────────────────────┘
```

---

## 生成SBL Meta Image

### Step 1: 提取Build Images

```bash
..\3-Tools\buildImage_creator.exe -i sbl.release.appimage
```

**生成文件**:
- `temp/sbl_r5_img.release.rig`

---

### Step 2: 创建Meta Image

```bash
..\3-Tools\metaImage_creator.exe -config metaimage_cfg.release.json
```

**生成文件**:
- `sbl.release.appimage` (包含Flash Header + SBL)

**文件结构**:
```
sbl.release.appimage:
  ├── Flash Header (~8KB @ 0x0)
  │   ├── Magic: 0x544F4F42
  │   ├── Image Size
  │   ├── Load Address
  │   └── Entry Point
  └── SBL Code (~130KB @ 0x2000)
```

---

## 烧录到Flash

### 烧录命令

```bash
cd ..\3-Tools
.\arprog_cmdline_6844.exe -p COM3 -f ..\1-SBL_Bootloader\sbl.release.appimage -o 0x2000
```

### 参数说明

- `-p COM3`: 串口号
- `-f sbl.release.appimage`: SBL Meta Image文件
- `-o 0x2000`: Flash偏移地址（与SDK宏`M_META_SBL_OFFSET`一致）

**为什么是0x2000？**
- Flash Header占用0x0-0x1FFF（8KB）
- SBL代码从0x2000开始
- ROM Bootloader会读取0x0的Header，然后从0x2000加载SBL

---

## 串口输出示例

### SBL启动日志

```
**********************************************
*        AWRL6844 Secondary Bootloader      *
*             Version: 1.0.0                *
**********************************************

[SBL] SOC Initialize...
[SBL]   PLL Config: 200 MHz
[SBL]   DDR Init: 533 MHz
[SBL]   QSPI Init: 80 MHz
[SBL] SOC Initialize... Done

[SBL] Loading Application...
[SBL]   Flash Address: 0x00042000
[SBL]   Reading Meta Header...
[SBL]   Meta Magic: 0x4D535452 (OK)
[SBL]   Image Count: 2

[SBL] Image 1: R5F Core
[SBL]   Load Address: 0x00000000
[SBL]   Entry Point: 0x00000100
[SBL]   Size: 102,400 bytes
[SBL]   Loading... Done

[SBL] Image 2: DSP Core
[SBL]   Load Address: 0x21000000
[SBL]   Size: 51,200 bytes
[SBL]   Loading... Done

[SBL] Starting R5F Core...
[SBL] Starting DSP Core...
[SBL] Jump to Application Entry
```

---

## SBL配置选项

### metaimage_cfg.release.json详解

| 配置项 | 值 | 说明 |
|--------|---|------|
| `securityType` | `"gp"` | General Purpose (非安全) |
| `flashIndex` | `"1"` | QSPI Flash索引 |
| `metaImageType` | `"multi"` | 多核镜像支持 |
| `pbistEnablecontrol` | `"0"` | 不启用PBIST自检 |
| `sharedRamAllocationControl` | `"0"` | 共享RAM自动分配 |

### 安全选项（HS版本）

如需启用安全启动，修改配置：
```json
{
  "securityType": "hs",
  "imageEncryptionParam": {
    "iv": "...",
    "keyFile": "config_keys/mek.txt"
  },
  "CertificateParams": {
    "certSigningKeyFileRSA": "config_keys/mpk.pem"
  }
}
```

---

## 常见问题

### Q1: SBL大小有限制吗？

**A**: 建议不超过256KB。超过需要修改Flash布局。

### Q2: 可以自定义SBL吗？

**A**: 可以。源码位置：
```
MMWAVE_L_SDK_06_01_00_01/examples/drivers/boot/sbl/
```

### Q3: SBL损坏会怎样？

**A**: 设备无法启动。需要重新烧录SBL。

### Q4: 如何更新SBL？

**A**: 只能通过UART烧录模式（SOP4）更新。正常运行时无法OTA更新SBL。

---

## 相关文档

- [README.md](../README.md) - 项目概述
- [Flash布局说明.md](../Flash布局说明.md) - 内存布局
- [操作指南.md](../操作指南.md) - 烧录步骤

---

**更新日期**: 2025-12-12  
**SDK版本**: 06.01.00.01
