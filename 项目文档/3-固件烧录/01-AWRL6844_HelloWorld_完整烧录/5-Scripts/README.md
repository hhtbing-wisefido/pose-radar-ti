# ⚡ 自动化脚本

> **一键执行完整烧录流程**

---

## 脚本清单

| 脚本 | 功能 | 执行时间 |
|------|------|---------|
| `1_generate_sbl_meta.bat` | 生成SBL Meta Image | ~5秒 |
| `2_generate_app_meta.bat` | 生成App Meta Image | ~8秒 |
| `3_flash_sbl.bat` | 烧录SBL到Flash | ~45秒 |
| `4_flash_app.bat` | 烧录App到Flash | ~60秒 |
| `5_verify_flash.bat` | 验证Flash内容 | ~30秒 |
| `clean_generated.bat` | 清理生成文件 | ~1秒 |
| `full_flash.bat` | 完整烧录流程 | ~2分钟 |

---

## 快速开始

### 方式1: 完整自动烧录（推荐新手）

```bash
# 一键完成所有步骤
full_flash.bat COM3
```

**执行流程**:
1. ✅ 清理旧文件
2. ✅ 生成SBL Meta Image
3. ✅ 生成App Meta Image
4. ✅ 烧录SBL到0x2000
5. ✅ 烧录App到0x42000
6. ✅ 验证烧录结果

---

### 方式2: 分步手动执行（推荐调试）

```bash
# Step 1: 生成SBL Meta Image
1_generate_sbl_meta.bat

# Step 2: 生成App Meta Image
2_generate_app_meta.bat

# Step 3: 烧录SBL
3_flash_sbl.bat COM3

# Step 4: 烧录App
4_flash_app.bat COM3

# Step 5: 验证（可选）
5_verify_flash.bat COM3
```

---

## 脚本详细说明

### 1. 生成SBL Meta Image

**文件**: `1_generate_sbl_meta.bat`

**功能**:
- 进入SBL目录
- 运行buildImage_creator
- 运行metaImage_creator
- 生成`sbl.release.appimage`

**执行示例**:
```bash
C:\> 1_generate_sbl_meta.bat

[INFO] 开始生成SBL Meta Image...
[INFO] 当前目录: D:\Ti雷达项目\项目文档\3-固件烧录\01-AWRL6844_HelloWorld_完整烧录\5-Scripts
[INFO] 进入SBL目录...
[INFO] 提取Build Images...
buildImage_creator v2.0
  Input: sbl.release.appimage
  Output: temp/sbl_r5fss0-0_nortos.release.rig
  Status: SUCCESS (125KB)

[INFO] 生成Meta Image...
metaImage_creator v2.0
  Config: metaimage_cfg.release.json
  Output: sbl.release.appimage
  Build Images: 1
    [0] r5fss0-0: 125KB
  Flash Header: Generated
  Status: SUCCESS (130KB)

[SUCCESS] SBL Meta Image生成完成！
[OUTPUT] 文件: ..\1-SBL_Bootloader\sbl.release.appimage (约130KB)
```

---

### 2. 生成App Meta Image

**文件**: `2_generate_app_meta.bat`

**功能**:
- 进入App目录
- 运行buildImage_creator
- 运行metaImage_creator
- 生成`hello_world_system.release.appimage`

**执行示例**:
```bash
C:\> 2_generate_app_meta.bat

[INFO] 开始生成App Meta Image...
[INFO] 当前目录: D:\Ti雷达项目\项目文档\3-固件烧录\01-AWRL6844_HelloWorld_完整烧录\5-Scripts
[INFO] 进入App目录...
[INFO] 提取Build Images...
buildImage_creator v2.0
  Input: hello_world_system.release.appimage
  Output: temp/hello_world_r5fss0-0.release.rig (80KB)
  Output: temp/hello_world_c66ss0.release.rig (100KB)
  Output: temp/hello_world_r5fss0-1.release.rig (30KB)
  Status: SUCCESS (3 images)

[INFO] 生成Meta Image...
metaImage_creator v2.0
  Config: metaimage_cfg.release.json
  Output: hello_world_system.release.appimage
  Build Images: 3
    [0] r5fss0-0: 80KB (R5F主核)
    [1] c66ss0: 100KB (DSP核)
    [2] r5fss0-1: 30KB (RF核)
  Flash Header: Not included
  Status: SUCCESS (约200KB)

[SUCCESS] App Meta Image生成完成！
[OUTPUT] 文件: ..\2-HelloWorld_App\hello_world_system.release.appimage (约200KB)
```

---

### 3. 烧录SBL

**文件**: `3_flash_sbl.bat`

**用法**: `3_flash_sbl.bat <COM端口>`

**功能**:
- 检查`sbl.release.appimage`是否存在
- 烧录到Flash 0x2000
- 自动验证

**执行示例**:
```bash
C:\> 3_flash_sbl.bat COM3

[INFO] 开始烧录SBL Bootloader...
[INFO] COM端口: COM3
[INFO] Meta Image: ..\1-SBL_Bootloader\sbl.release.appimage (约130KB)
[INFO] Flash地址: 0x00002000

[WARN] 请确认硬件设置：
  1. SOP开关设置为SOP_MODE1 (00) - QSPI刷写模式
  2. USB连接到COM3
  3. 板子已上电

按任意键继续...

[INFO] 开始烧录...
arprog_cmdline_6844 v3.5
  Port: COM3
  File: ../1-SBL_Bootloader/sbl.release.appimage
  Address: 0x2000
  Size: ~130KB
  
  Progress: [##########] 100%
  Time: 45s
  Status: SUCCESS

[INFO] 验证烧录...
  Readback: ~130KB
  Compare: MATCH
  Status: VERIFIED

[SUCCESS] SBL烧录完成！
[INFO] 下一步: 运行 4_flash_app.bat COM3 烧录应用
```

**错误处理**:
```bash
[ERROR] 找不到sbl.release.appimage！
[INFO] 请先运行: 1_generate_sbl_meta.bat

[ERROR] 无法打开COM3！
[INFO] 检查：
  1. COM端口号是否正确
  2. 其他程序是否占用串口
  3. USB驱动是否安装
  4. 板子是否上电

[ERROR] 烧录失败！
[INFO] 尝试：
  1. 按下复位按钮
  2. 重新上电
  3. 检查SOP开关配置 (S7/S8)
```

---

### 4. 烧录App

**文件**: `4_flash_app.bat`

**用法**: `4_flash_app.bat <COM端口>`

**功能**:
- 检查`hello_world_system.release.appimage`是否存在
- 烧录到Flash 0x42000
- 自动验证

**执行示例**:
```bash
C:\> 4_flash_app.bat COM3

[INFO] 开始烧录HelloWorld应用...
[INFO] COM端口: COM3
[INFO] Meta Image: ..\2-HelloWorld_App\hello_world_system.release.appimage (约200KB)
[INFO] Flash地址: 0x00042000

[INFO] 开始烧录...
arprog_cmdline_6844 v3.5
  Port: COM3
  File: ../2-HelloWorld_App/hello_world_system.release.appimage
  Address: 0x42000
  Size: ~200KB
  
  Progress: [##########] 100%
  Time: 60s
  Status: SUCCESS

[INFO] 验证烧录...
  Readback: ~200KB
  Compare: MATCH
  Status: VERIFIED

[SUCCESS] HelloWorld应用烧录完成！
[INFO] 下一步：
  1. 关闭板子电源
  2. 将SOP开关改为SOP_MODE2 (01) - 应用/功能模式
  3. 重新上电
  4. 打开串口终端（115200 8N1）
  5. 查看输出
```

---

### 5. 验证Flash

**文件**: `5_verify_flash.bat`

**用法**: `5_verify_flash.bat <COM端口>`

**功能**:
- 读回SBL区域（0x2000）
- 读回App区域（0x42000）
- 对比原始文件

**执行示例**:
```bash
C:\> 5_verify_flash.bat COM3

[INFO] 开始验证Flash内容...

[INFO] 验证SBL区域...
  Address: 0x2000
  Size: ~130KB
  Readback: ../4-Generated/sbl_readback.bin
  Compare with: ../1-SBL_Bootloader/sbl.release.appimage
  Result: MATCH ✓

[INFO] 验证App区域...
  Address: 0x42000
  Size: ~200KB
  Readback: ../4-Generated/app_readback.bin
  Compare with: ../2-HelloWorld_App/hello_world_system.release.appimage
  Result: MATCH ✓

[SUCCESS] Flash内容验证通过！
[INFO] 所有文件已正确烧录到Flash
```

---

### 6. 清理生成文件

**文件**: `clean_generated.bat`

**功能**:
- 删除temp/目录
- 删除所有.bin文件
- 保留源文件和配置

**执行示例**:
```bash
C:\> clean_generated.bat

[WARN] 将删除以下内容：
  - 4-Generated/temp/
  - 4-Generated/*.bin
  - 1-SBL_Bootloader/temp/
  - 2-HelloWorld_App/temp/

是否继续？(Y/N): Y

[INFO] 清理中...
  Removed: 4-Generated/temp/ (5 files)
  Removed: 4-Generated/*.bin (4 files)
  Removed: 1-SBL_Bootloader/temp/ (1 file)
  Removed: 2-HelloWorld_App/temp/ (3 files)

[SUCCESS] 清理完成！
[INFO] 总计删除: 13个文件
```

---

### 7. 完整烧录流程

**文件**: `full_flash.bat`

**用法**: `full_flash.bat <COM端口>`

**功能**: 自动执行完整流程

**执行示例**:
```bash
C:\> full_flash.bat COM3

╔════════════════════════════════════════════════════════════╗
║    AWRL6844 HelloWorld 完整烧录流程                        ║
║    COM端口: COM3                                           ║
╚════════════════════════════════════════════════════════════╝

[1/6] 清理旧文件...
  └─ 完成 (1s)

[2/6] 生成SBL Meta Image...
  └─ 完成 (5s) - sbl.release.appimage (约130KB)

[3/6] 生成App Meta Image...
  └─ 完成 (8s) - hello_world_system.release.appimage (约200KB)

[4/6] 烧录SBL Bootloader...
  └─ 地址: 0x2000
  └─ 完成 (45s)

[5/6] 烧录HelloWorld应用...
  └─ 地址: 0x42000
  └─ 完成 (60s)

[6/6] 验证Flash内容...
  └─ SBL区域: VERIFIED ✓
  └─ App区域: VERIFIED ✓
  └─ 完成 (30s)

╔════════════════════════════════════════════════════════════╗
║    烧录完成！总耗时: 2分29秒                               ║
╚════════════════════════════════════════════════════════════╝

[INFO] 后续操作：
  1. 断电
  2. SOP开关改为SOP_MODE2 (01)
  3. 重新上电
  4. 打开串口终端查看输出

[INFO] 期望输出：
  ----------------------------------------
  [BOOTLOADER_PROFILE] Boot Media       : FLASH
  [BOOTLOADER_PROFILE] Boot Image Size  : 220 KB
  [BOOTLOADER_PROFILE] Cores present    :
  r5f0-0
  c66ss0
  r5f0-1
  [BOOTLOADER] Booting Cores ...
  Hello World from r5f0-0 !
  Hello World from c66ss0 !
  Hello World from r5f0-1 !
  ----------------------------------------
```

---

## 常见问题

### Q1: 脚本找不到工具？

**A**: 
- 检查目录结构是否正确
- 工具必须在`3-Tools/`目录
- 脚本必须在`5-Scripts/`目录

---

### Q2: 脚本在哪个目录运行？

**A**: 
- 推荐在`5-Scripts/`目录运行
- 脚本会自动切换到正确目录
- 也可以在项目根目录运行

---

### Q3: 如何修改默认串口？

**A**: 编辑脚本，修改：
```batch
SET COM_PORT=COM3
```

---

### Q4: 脚本支持参数吗？

**A**: 
- 烧录脚本需要串口参数：`3_flash_sbl.bat COM3`
- 生成脚本无需参数：`1_generate_sbl_meta.bat`
- 完整流程需要串口参数：`full_flash.bat COM3`

---

### Q5: 脚本失败后如何恢复？

**A**: 
```bash
# 清理后重新开始
clean_generated.bat
full_flash.bat COM3
```

---

## 脚本依赖

| 脚本 | 依赖工具 | 依赖文件 |
|------|---------|---------|
| 1_generate_sbl_meta | buildImage_creator, metaImage_creator | sbl.release.appimage, metaimage_cfg.release.json |
| 2_generate_app_meta | buildImage_creator, metaImage_creator | hello_world_system.release.appimage, metaimage_cfg.release.json |
| 3_flash_sbl | arprog_cmdline_6844 | sbl_meta.bin |
| 4_flash_app | arprog_cmdline_6844 | hello_world_meta.bin |
| 5_verify_flash | arprog_cmdline_6844 | sbl_meta.bin, hello_world_meta.bin |
| clean_generated | - | - |
| full_flash | 所有工具 | 所有文件 |

---

## 相关文档

- [README.md](../README.md) - 项目概述
- [操作指南.md](../操作指南.md) - 详细步骤
- [3-Tools/README.md](../3-Tools/README.md) - 工具说明

---

**更新日期**: 2025-12-12  
**SDK版本**: 06.01.00.01
