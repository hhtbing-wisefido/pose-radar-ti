# 🔧 烧录工具目录

> **AWRL6844 固件烧录工具集**

---

## 📍 SDK来源路径

**官方SDK工具位置**:
```
C:\ti\MMWAVE_L_SDK_06_01_00_01\tools\FlashingTool\
├── arprog_cmdline_6844.exe          # 烧录工具 ⭐
├── buildImage_creator.exe           # 镜像分析工具
├── metaImage_creator.exe            # Meta镜像生成工具
├── flashHeader_creator.exe          # Flash头生成工具
└── Readme_arprog_cmdline_6844.html  # 工具说明文档
```

**设备型号**: AWRL6844 (xWRL684x-evm)

---

## 📂 工具清单

| 工具 | 大小 | 用途 | 是否必需 |
|------|------|------|----------|
| `arprog_cmdline_6844.exe` | ~2MB | **串口烧录工具** | ✅ **必需** |
| `buildImage_creator.exe` | ~6MB | 调试工具（分析.appimage） | ❌ 可选 |
| `metaImage_creator.exe` | ~6MB | 调试工具（高级定制） | ❌ 可选 |
| `flashHeader_creator.exe` | ~5MB | 调试工具（独立创建头） | ❌ 可选 |

---

## 🎯 核心工具：arprog_cmdline_6844.exe

### 工具信息

- **版本**: 3.5
- **来源**: TI SDK官方工具
- **路径**: `MMWAVE_L_SDK_06_01_00_01/tools/FlashingTool/`
- **功能**: 通过XDS110串口烧录固件到QSPI Flash

### 支持的操作

- ✅ Flash烧录（Programming）
- ✅ Flash读取（Read）
- ✅ Flash擦除（Erase）
- ✅ Flash验证（Verify）
- ✅ 自动创建Flash Header（`-cf`参数）⭐

---

## 🚀 正常烧录流程（官方推荐）

### 方法1: 完整一键烧录（推荐）⭐

```powershell
# 使用-cf参数，自动创建Flash Header
arprog_cmdline_6844.exe -p COM3 ^
  -f1 "..\1-SBL_Bootloader\sbl.release.appimage" ^
  -f2 "..\2-HelloWorld_App\hello_world_system.release.appimage" ^
  -s SFLASH -c -cf
```

**优势**：
- ✅ 一条命令同时烧录SBL和应用
- ✅ 自动计算Flash地址
- ✅ 自动创建Flash Header
- ✅ 无需任何中间步骤

### 方法2: 分步烧录

```powershell
# 步骤1: 烧录SBL到0x2000
arprog_cmdline_6844.exe -p COM3 ^
  -f1 "..\1-SBL_Bootloader\sbl.release.appimage" ^
  -of1 8192 ^
  -s SFLASH -c

# 步骤2: 烧录应用到0x42000
arprog_cmdline_6844.exe -p COM3 ^
  -f1 "..\2-HelloWorld_App\hello_world_system.release.appimage" ^
  -of1 270336 ^
  -s SFLASH -c
```

**地址说明**：
- 8192 = 0x2000 (SBL起始地址)
- 270336 = 0x42000 (Application起始地址)

---

## 📋 arprog 参数详解

### 必需参数

| 参数 | 说明 | 示例 |
|------|------|------|
| `-p` | 串口号 | `-p COM3` |
| `-f1` | 第一个文件 | `-f1 sbl.appimage` |
| `-s` | 存储类型 | `-s SFLASH` |

### 重要可选参数

| 参数 | 说明 | 用途 |
|------|------|------|
| `-c` | 初始化时发送break | 建立连接 |
| `-cf` | 自动创建Flash Header | ⭐推荐使用 |
| `-of1` | 手动指定偏移 | 与-cf互斥 |
| `-f2` ~ `-f8` | 额外文件 | 多文件烧录 |
| `-v` | 验证模式 | 烧录后验证 |

### 读取/擦除参数

| 参数 | 说明 | 示例 |
|------|------|------|
| `-r` | 读取起始地址 | `-r 0x2000` |
| `-s` | 读取大小（字节） | `-s 130000` |
| `-o` | 输出文件 | `-o output.bin` |
| `-e` | 全片擦除 | `-e` |

---

## 🔍 调试工具说明（可选）

### buildImage_creator.exe

**⚠️ 此工具不是正常烧录所需**

**用途**：分析.appimage文件内部结构

```bash
# 提取.appimage中的各核镜像
buildImage_creator.exe -i hello_world.appimage

# 输出：temp/目录中的.rig文件
# - R5F核心镜像
# - DSP核心镜像
# - RF核心镜像
```

**何时使用**：
- 🔍 调试固件结构
- 🔍 学习.appimage格式
- 🔍 分析各核代码大小

**不需要用于**：
- ❌ 正常烧录流程
- ❌ 生成可烧录文件

---

### metaImage_creator.exe

**⚠️ 此工具不是正常烧录所需**

**用途**：高级定制Meta Image（专家用户）

```bash
# 使用自定义配置生成Meta Image
metaImage_creator.exe -config custom_config.json
```

**何时使用**：
- 🔧 自定义Meta Header参数
- 🔧 手动组合多个.rig文件
- 🔧 添加签名/加密（高级功能）

**不需要用于**：
- ❌ 正常烧录流程
- ❌ SDK编译的标准固件

---

### flashHeader_creator.exe

**⚠️ 此工具不是正常烧录所需**

**用途**：独立创建Flash Header（极少使用）

```bash
flashHeader_creator.exe -config flash_header_cfg.json
```

**何时使用**：
- 🔬 研究Flash Header格式
- 🔬 特殊场景的Header定制

**不需要用于**：
- ❌ 正常烧录（arprog -cf自动创建）

---

## ❓ 常见问题

### Q1: .appimage可以直接烧录吗？

**A**: ✅ **可以！**

- SDK编译直接生成可烧录的.appimage文件
- 使用arprog的`-cf`参数会自动创建Flash Header
- **不需要**提取、转换或生成meta

**错误理解**：
- ❌ "需要用buildImage_creator提取"
- ❌ "需要用metaImage_creator生成meta"
- ❌ ".appimage不能直接烧录"

### Q2: 什么是Flash Header？为什么需要它？

**A**: Flash Header是ROM Bootloader识别固件的标记。

- ROM启动后读取Flash 0x0位置
- Flash Header包含固件位置、大小等信息
- arprog的`-cf`参数会**自动创建**

**你不需要关心细节**：
- ✅ 使用`-cf`参数即可
- ✅ 工具自动计算和写入

### Q3: 何时使用调试工具？

**A**: 仅在以下场景：

| 场景 | 工具 | 原因 |
|------|------|------|
| 分析固件结构 | buildImage_creator | 查看各核代码 |
| 学习.appimage格式 | buildImage_creator | 教学目的 |
| 高级定制（专家） | metaImage_creator | 特殊需求 |
| 研究Flash格式 | flashHeader_creator | 开发工具 |

**99%的用户不需要这些工具！**

### Q4: 烧录失败怎么办？

**A**: 检查清单：

1. **硬件检查**
   - [ ] SOP开关：SOP_MODE0 = [0000] (功能模式)
   - [ ] USB连接：XDS110端口
   - [ ] 板子上电

2. **软件检查**
   - [ ] COM口正确（设备管理器查看）
   - [ ] 驱动安装（XDS110驱动）
   - [ ] 串口未被占用（关闭其他串口工具）

3. **文件检查**
   - [ ] .appimage文件存在
   - [ ] 文件路径正确（相对路径或绝对路径）

4. **重试步骤**
   - 按下复位按钮
   - 重新上电
   - 重新运行烧录命令

### Q5: 如何验证烧录成功？

**A**: 三种方法：

```powershell
# 方法1: 烧录时加-v参数（推荐）
arprog_cmdline_6844.exe -p COM3 -f1 file.appimage -of1 8192 -s SFLASH -c -v

# 方法2: 读回Flash内容对比
arprog_cmdline_6844.exe -p COM3 -r 0x2000 -s 130000 -o sbl_readback.bin
# 然后用工具对比sbl_readback.bin和原文件

# 方法3: 实际运行测试（最可靠）
# 1. 切换SOP_MODE2 = [0011]
# 2. 打开串口工具（COM4, 115200）
# 3. 复位板子
# 4. 查看串口输出
```

### Q6: 为什么有些文档说要生成meta？

**A**: 那些是**过时或错误的理解**。

**官方SDK文档明确说明**：
- BUILD_GUIDE.html: 编译生成.appimage用于烧录
- EVM_SETUP_AND_FLASHING.html: 使用arprog -cf烧录

**来源可能**：
- 老版本SDK（流程已更新）
- 第三方教程（理解错误）
- 调试场景（误当作标准流程）

**正确流程**：
```
SDK编译 → .appimage → arprog -cf → 完成
```

---

## 📊 性能参考

| 操作 | 耗时 | 说明 |
|------|------|------|
| Flash擦除 | ~30秒 | 按Sector擦除 |
| 烧录SBL (130KB) | ~45秒 | 115200波特率 |
| 烧录App (220KB) | ~60秒 | 115200波特率 |
| 验证 | ~20秒 | 读回并对比 |
| **完整流程** | **~2-3分钟** | 擦除+烧录+验证 |

---

## 🔗 相关资源

### 官方文档（SDK中）

- `docs/api_guide_xwrL684x/BUILD_GUIDE.html` - 编译指南
- `docs/api_guide_xwrL684x/EVM_SETUP_AND_FLASHING.html` - 烧录指南
- `tools/FlashingTool/Readme_arprog_cmdline_6844.html` - 工具手册

### 项目文档

- [../README.md](../README.md) - 项目概述
- [../操作指南.md](../操作指南.md) - 详细步骤
- [../5-Scripts/README.md](../5-Scripts/README.md) - 自动化脚本

---

## 📝 快速参考

### 最常用命令

```powershell
# 完整烧录（推荐）
cd 3-Tools
.\arprog_cmdline_6844.exe -p COM3 ^
  -f1 "..\1-SBL_Bootloader\sbl.release.appimage" ^
  -f2 "..\2-HelloWorld_App\hello_world_system.release.appimage" ^
  -s SFLASH -c -cf

# 或使用脚本
cd ..\5-Scripts
.\full_flash.bat COM3
```

### 记住这些

- ✅ `.appimage` 可以直接烧录
- ✅ 使用 `-cf` 参数自动创建Flash Header
- ✅ 调试工具（buildImage/metaImage）不是必需的
- ✅ 推荐使用自动化脚本（5-Scripts目录）

---

**更新日期**: 2025-12-15  
**SDK版本**: 06.01.00.01  
**工具版本**: arprog_cmdline_6844 v3.5
