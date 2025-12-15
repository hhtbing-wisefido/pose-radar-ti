# ⚡ 自动化脚本

> **一键执行完整烧录流程（基于官方SDK流程）**

---

## 脚本清单

| 脚本 | 功能 | 执行时间 |
|------|------|----------|
| `full_flash.bat` | 完整烧录流程（推荐）⭐ | ~2-3分钟 |
| `3_flash_sbl.bat` | 单独烧录SBL | ~1分钟 |
| `4_flash_app.bat` | 单独烧录应用 | ~1-2分钟 |

**已删除的脚本**（不再需要）：
- ❌ `1_generate_sbl_meta.bat` - SDK编译已生成.appimage
- ❌ `2_generate_app_meta.bat` - SDK编译已生成.appimage

---

## 快速开始

### 方式1: 完整自动烧录（推荐）⭐

```bash
# 一键完成所有步骤
full_flash.bat COM3
```

**执行流程**:
1. ✅ 检查固件文件（.appimage）
2. ✅ 使用arprog一次性烧录SBL和App（使用-cf自动创建Flash Header）
3. ✅ 显示完成信息

**特点**：
- 使用官方推荐的`-cf`参数
- 无需手动生成Meta Image
- 自动创建Flash Header
- 一条命令完成全部烧录

---

### 方式2: 分步手动执行（调试用）

```bash
# Step 1: 烧录SBL到0x2000
3_flash_sbl.bat COM3

# Step 2: 烧录App到0x42000
4_flash_app.bat COM3
```

**适用场景**：
- 单独更新SBL或应用
- 调试特定组件
- 学习烧录流程

---

## 脚本详细说明

### 1. 完整烧录流程 ⭐

**文件**: `full_flash.bat`

**用法**: `full_flash.bat <COM端口>`

**功能**:
- 检查固件文件是否存在
- 使用arprog一次性烧录SBL和App
- 自动创建Flash Header（-cf参数）

**执行示例**:
```powershell
PS C:\> .\full_flash.bat COM3

========================================
 AWRL6844 完整烧录脚本（官方推荐方式）
========================================

[检查1/2] 检查SBL固件文件...
✓ 找到: ..\1-SBL_Bootloader\sbl.release.appimage (~130KB)

[检查2/2] 检查应用固件文件...
✓ 找到: ..\2-HelloWorld_App\hello_world_system.release.appimage (~218KB)

========================================
[烧录] 使用官方-cf参数一次性烧录
========================================

执行命令:
arprog_cmdline_6844.exe -p COM3 ^
  -f1 "..\1-SBL_Bootloader\sbl.release.appimage" ^
  -f2 "..\2-HelloWorld_App\hello_world_system.release.appimage" ^
  -s SFLASH -c -cf

[arprog输出]
Connecting to device on COM3...
Erasing Flash sectors...
Programming file 1 at 0x2000... (~130KB)
Programming file 2 at 0x42000... (~218KB)
Creating Flash Header automatically...
Verifying...
Done!

========================================
✅ 烧录完成！总耗时: ~2分30秒
========================================

下一步:
1. 切换SOP_MODE2 = [0011] (Debug UART模式)
2. 打开串口工具 (COM4, 115200-8-N-1)
3. 复位板子
4. 查看串口输出

期望看到:
  [BOOTLOADER] Boot Media: FLASH
  [BOOTLOADER] Booting Cores...
  Hello World from r5f0-0!
  Hello World from c66ss0!
```

**关键命令**：
```batch
arprog_cmdline_6844.exe -p %COM_PORT% ^
  -f1 "..\1-SBL_Bootloader\sbl.release.appimage" ^
  -f2 "..\2-HelloWorld_App\hello_world_system.release.appimage" ^
  -s SFLASH -c -cf
```

**参数说明**：
- `-p COM3`: 串口号
- `-f1 file`: 第一个文件（SBL）
- `-f2 file`: 第二个文件（App）
- `-s SFLASH`: 目标存储（SPI Flash）
- `-c`: 擦除后写入
- `-cf`: 自动创建Flash Header ⭐

---

### 2. 单独烧录SBL

**文件**: `3_flash_sbl.bat`

**用法**: `3_flash_sbl.bat <COM端口>`

**功能**:
- 检查SBL文件是否存在
- 烧录SBL到0x2000
- 使用新参数格式

**执行示例**:
```powershell
PS C:\> .\3_flash_sbl.bat COM3

========================================
 AWRL6844 烧录 SBL Bootloader
========================================

[检查] SBL固件文件...
✓ 找到: ..\1-SBL_Bootloader\sbl.release.appimage (~130KB)

[烧录] 目标地址: 0x2000 (8192字节)

执行命令:
arprog_cmdline_6844.exe -p COM3 ^
  -f1 "..\1-SBL_Bootloader\sbl.release.appimage" ^
  -of1 8192 -s SFLASH -c

[arprog输出]
Programming... 100%
Done!

========================================
✅ SBL烧录完成！
========================================

下一步: 运行 4_flash_app.bat COM3 烧录应用
```

**关键命令**：
```batch
arprog_cmdline_6844.exe -p %COM_PORT% ^
  -f1 "..\1-SBL_Bootloader\sbl.release.appimage" ^
  -of1 8192 ^
  -s SFLASH -c
```

**参数说明**：
- `-f1 file`: 要烧录的文件
- `-of1 8192`: 起始地址（8192 = 0x2000）
- `-s SFLASH`: SPI Flash
- `-c`: 擦除后写入

---

### 3. 单独烧录应用

**文件**: `4_flash_app.bat`

**用法**: `4_flash_app.bat <COM端口>`

**功能**:
- 检查App文件是否存在
- 烧录App到0x42000
- 使用新参数格式

**执行示例**:
```powershell
PS C:\> .\4_flash_app.bat COM3

========================================
 AWRL6844 烧录 HelloWorld 应用
========================================

[检查] 应用固件文件...
✓ 找到: ..\2-HelloWorld_App\hello_world_system.release.appimage (~218KB)

[烧录] 目标地址: 0x42000 (270336字节)

执行命令:
arprog_cmdline_6844.exe -p COM3 ^
  -f1 "..\2-HelloWorld_App\hello_world_system.release.appimage" ^
  -of1 270336 -s SFLASH -c

[arprog输出]
Programming... 100%
Done!

========================================
✅ 应用烧录完成！
========================================

下一步:
1. 切换SOP_MODE2 = [0011] (Debug UART模式)
2. 打开串口工具 (COM4, 115200-8-N-1)
3. 复位板子
4. 查看串口输出
```

**关键命令**：
```batch
arprog_cmdline_6844.exe -p %COM_PORT% ^
  -f1 "..\2-HelloWorld_App\hello_world_system.release.appimage" ^
  -of1 270336 ^
  -s SFLASH -c
```

**参数说明**：
- `-f1 file`: 要烧录的文件
- `-of1 270336`: 起始地址（270336 = 0x42000）
- `-s SFLASH`: SPI Flash
- `-c`: 擦除后写入

---

## 工作流程对比

### 官方推荐流程（当前使用）✅

```
SDK编译
   ↓
.appimage文件
   ↓
full_flash.bat（使用-cf参数）
   ↓
一次性烧录完成
```

**优点**：
- ✅ 简单快速
- ✅ 符合官方文档
- ✅ 自动处理Flash Header
- ✅ 不依赖buildImage/metaImage工具

### 旧流程（已废弃）❌

```
SDK编译
   ↓
.appimage文件
   ↓
buildImage_creator（提取）
   ↓
metaImage_creator（生成meta）
   ↓
arprog烧录
   ↓
完成
```

**为什么废弃**：
- ❌ 步骤繁琐（多余的提取和生成）
- ❌ 不符合官方文档说明
- ❌ arprog的-cf参数已能自动创建Header

---

## 常见问题

### Q1: 为什么删除了1_generate_sbl_meta.bat和2_generate_app_meta.bat？

**A**: 
- SDK编译时已经生成了完整的.appimage文件
- .appimage可以直接烧录，无需再生成meta
- arprog的`-cf`参数会自动创建Flash Header
- 这符合官方SDK文档的说明

---

### Q2: 脚本找不到工具？

**A**: 
- 检查`3-Tools/`目录是否存在`arprog_cmdline_6844.exe`
- 脚本必须在`5-Scripts/`目录运行
- 或确保相对路径正确

---

### Q3: 如何修改默认串口？

**A**: 编辑脚本，修改：
```batch
SET COM_PORT=COM3
```

或运行时指定：
```batch
full_flash.bat COM5
```

---

### Q4: 烧录失败怎么办？

**A**: 检查：
1. SOP开关是否设置为SOP_MODE0 = [0000]（功能模式）
2. USB驱动是否正确安装
3. 串口是否被其他程序占用
4. 固件文件是否存在且完整
5. 板子是否正常上电

---

### Q5: 可以只烧录应用吗？

**A**: 
- 首次烧录：必须先烧录SBL，再烧录应用
- 后续更新：可以只运行`4_flash_app.bat`更新应用

---

## 脚本依赖

| 脚本 | 依赖工具 | 依赖文件 |
|------|---------|---------|
| full_flash.bat | arprog_cmdline_6844.exe | sbl.release.appimage, hello_world_system.release.appimage |
| 3_flash_sbl.bat | arprog_cmdline_6844.exe | sbl.release.appimage |
| 4_flash_app.bat | arprog_cmdline_6844.exe | hello_world_system.release.appimage |

---

## 相关文档

- [../README.md](../README.md) - 项目概述
- [../操作指南.md](../操作指南.md) - 详细步骤
- [../3-Tools/README.md](../3-Tools/README.md) - 工具说明
- [../Flash布局说明.md](../Flash布局说明.md) - Flash内存布局

---

**更新日期**: 2025-12-15  
**SDK版本**: 06.01.00.01  
**变更记录**: 删除meta生成脚本，改用官方-cf参数流程
