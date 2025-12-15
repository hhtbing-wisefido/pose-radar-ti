# 📦 生成文件目录

> **工具链自动生成的中间和最终文件**

---

## 目录说明

此目录用于存放：
- 🔧 **中间文件**: buildImage_creator提取的.rig文件
- 📤 **最终文件**: metaImage_creator生成的Meta Images
- ✅ **验证文件**: Flash读回的数据

**注意**: 此目录下的文件都是自动生成的，可随时删除重新生成。

---

## 文件类型

### 1. Build Images (.rig文件)

**来源**: buildImage_creator.exe提取
**用途**: metaImage_creator的输入文件

**文件示例**:
```
temp/
  ├── sbl_r5fss0-0_nortos.release.rig    (SBL R5F核心镜像)
  ├── hello_world_r5fss0-0.release.rig   (App R5F核心镜像)
  ├── hello_world_c66ss0.release.rig     (App DSP核心镜像)
  └── hello_world_r5fss0-1.release.rig   (App RF核心镜像)
```

**特点**:
- 二进制格式
- 包含核心标识（R5F/C66/RF）
- 大小通常几百KB

---

### 2. Meta Images (.bin文件)

**来源**: metaImage_creator.exe生成
**用途**: 直接烧录到Flash

**文件示例**:
```
├── sbl_meta.bin                 (SBL Meta Image, ~130KB)
└── hello_world_meta.bin         (App Meta Image, ~220KB)
```

**结构**:
```
Meta Image (.bin):
  ├─ Meta Header              (~1KB)
  │  ├─ Magic Number
  │  ├─ Version
  │  ├─ Build Images数量
  │  └─ Checksum
  ├─ Flash Header (SBL)       (256B, 仅SBL包含)
  ├─ Build Image 1
  │  ├─ Header
  │  └─ Executable Code
  ├─ Build Image 2
  │  ├─ Header
  │  └─ Executable Code
  └─ Build Image 3
     ├─ Header
     └─ Executable Code
```

---

### 3. 验证文件

**来源**: arprog_cmdline读回
**用途**: 验证Flash内容

**文件示例**:
```
readback/
  ├── sbl_readback.bin         (从Flash 0x2000读回)
  └── app_readback.bin         (从Flash 0x42000读回)
```

---

## 生成流程

### Phase 1: 生成SBL Meta Image

```bash
cd 1-SBL_Bootloader

# 步骤1: 提取Build Images
..\3-Tools\buildImage_creator.exe -i sbl.release.appimage
# 输出: temp/sbl_r5fss0-0_nortos.release.rig

# 步骤2: 生成Meta Image
..\3-Tools\metaImage_creator.exe -config metaimage_cfg.release.json
# 输出: ..\4-Generated\sbl_meta.bin
```

**生成的文件**:
- 📁 `temp/sbl_r5fss0-0_nortos.release.rig` (中间文件)
- 📤 `4-Generated/sbl_meta.bin` (最终文件)

---

### Phase 2: 生成App Meta Image

```bash
cd 2-HelloWorld_App

# 步骤1: 提取Build Images
..\3-Tools\buildImage_creator.exe -i hello_world_system.release.appimage
# 输出:
#   temp/hello_world_r5fss0-0.release.rig
#   temp/hello_world_c66ss0.release.rig
#   temp/hello_world_r5fss0-1.release.rig

# 步骤2: 生成Meta Image
..\3-Tools\metaImage_creator.exe -config metaimage_cfg.release.json
# 输出: ..\4-Generated\hello_world_meta.bin
```

**生成的文件**:
- 📁 `temp/hello_world_r5fss0-0.release.rig` (R5F主核)
- 📁 `temp/hello_world_c66ss0.release.rig` (DSP核)
- 📁 `temp/hello_world_r5fss0-1.release.rig` (RF核)
- 📤 `4-Generated/hello_world_meta.bin` (最终文件)

---

## 文件命名规范

### 配置文件中的命名

**SBL配置** (`1-SBL_Bootloader/metaimage_cfg.release.json`):
```json
{
  "metaImageFile": "..\\4-Generated\\sbl_meta.bin",
  "buildImages": [
    {
      "imageFile": "temp/sbl_r5fss0-0_nortos.release.rig",
      "core": "r5fss0-0"
    }
  ]
}
```

**App配置** (`2-HelloWorld_App/metaimage_cfg.release.json`):
```json
{
  "metaImageFile": "..\\4-Generated\\hello_world_meta.bin",
  "buildImages": [
    {
      "imageFile": "temp/hello_world_r5fss0-0.release.rig",
      "core": "r5fss0-0"
    },
    {
      "imageFile": "temp/hello_world_c66ss0.release.rig",
      "core": "c66ss0"
    },
    {
      "imageFile": "temp/hello_world_r5fss0-1.release.rig",
      "core": "r5fss0-1"
    }
  ]
}
```

---

## 文件验证

### 方法1: 工具自动验证

```bash
# 烧录时带验证
arprog_cmdline_6844.exe -p COM3 -f sbl_meta.bin -o 0x2000 -v
```

### 方法2: 手动读回对比

```bash
# 读回SBL
arprog_cmdline_6844.exe -p COM3 -r 0x2000 -s 130000 -o 4-Generated\sbl_readback.bin

# 读回App
arprog_cmdline_6844.exe -p COM3 -r 0x42000 -s 220000 -o 4-Generated\app_readback.bin

# 对比文件
fc /b 4-Generated\sbl_meta.bin 4-Generated\sbl_readback.bin
```

### 方法3: 解析Meta Image

```bash
# 查看Meta Image内容（需自定义工具）
python parse_meta_image.py 4-Generated/sbl_meta.bin
```

**解析输出示例**:
```
Meta Image: sbl_meta.bin
  Size: 130KB
  Magic: 0x12345678
  Version: 1.0
  Build Images: 1
    [0] R5F (r5fss0-0): 125KB @ 0x0100
  Flash Header: Present
  Checksum: 0xABCDEF01 (Valid)
```

---

## 常见问题

### Q1: temp/目录找不到？

**A**: 
- buildImage_creator会自动创建temp/目录
- 如果不存在，手动创建：`mkdir temp`

---

### Q2: Meta Image生成失败？

**A**: 检查：
1. ✅ 是否先运行buildImage_creator？
2. ✅ temp/目录下是否有.rig文件？
3. ✅ metaimage_cfg.json中路径是否正确？
4. ✅ Build Images数量是否匹配？

---

### Q3: 为什么SBL只有1个Build Image？

**A**: 
- SBL仅在R5F主核运行
- 不需要多核协同
- HelloWorld需要R5F + DSP + RF三核

---

### Q4: .rig文件可以复用吗？

**A**: 
- ✅ **可以**，如果不修改代码
- ⚠️ 修改代码后必须重新生成
- ⚠️ 不同项目的.rig不能混用

---

### Q5: 如何清理生成文件？

**A**: 
```bash
# Windows
rmdir /s /q temp
del 4-Generated\*.bin

# PowerShell
Remove-Item temp -Recurse -Force
Remove-Item 4-Generated\*.bin
```

---

## 文件大小参考

| 文件 | 类型 | 大小 | 备注 |
|------|------|------|------|
| sbl.release.appimage | 源文件 | ~130KB | 打包格式 |
| sbl_r5fss0-0.release.rig | 中间 | ~125KB | 可执行镜像 |
| sbl_meta.bin | 最终 | ~130KB | 添加了Headers |
| hello_world_system.release.appimage | 源文件 | ~220KB | 打包格式 |
| hello_world_r5fss0-0.release.rig | 中间 | ~80KB | R5F镜像 |
| hello_world_c66ss0.release.rig | 中间 | ~100KB | DSP镜像 |
| hello_world_r5fss0-1.release.rig | 中间 | ~30KB | RF镜像 |
| hello_world_meta.bin | 最终 | ~220KB | 添加了Headers |

---

## 目录结构示例

**完整生成后的目录**:
```
4-Generated/
  ├── README.md                        (本文档)
  ├── sbl_meta.bin                     (SBL Meta Image)
  ├── hello_world_meta.bin             (App Meta Image)
  ├── temp/                            (临时目录)
  │   ├── sbl_r5fss0-0_nortos.release.rig
  │   ├── hello_world_r5fss0-0.release.rig
  │   ├── hello_world_c66ss0.release.rig
  │   └── hello_world_r5fss0-1.release.rig
  └── readback/                        (验证目录，可选)
      ├── sbl_readback.bin
      └── app_readback.bin
```

---

## 相关文档

- [3-Tools/README.md](../3-Tools/README.md) - 工具详细说明
- [操作指南.md](../操作指南.md) - 完整操作流程
- [README.md](../README.md) - 项目概述

---

**更新日期**: 2025-12-12  
**SDK版本**: 06.01.00.01
