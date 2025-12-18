# 🚀 AWRL6844 固件系统工具

> **项目目标**: 从空白板子到运行HelloWorld，实现完整的QSPI Flash烧录流程

## 📍 SDK来源总结

**本项目所有资料均来自TI官方SDK**:

```
C:\ti\MMWAVE_L_SDK_06_01_00_01\    # SDK根目录
├── examples\drivers\boot\sbl\xwrL684x-evm\           # SBL固件源码
├── examples\hello_world\xwrL684x-evm\              # HelloWorld源码
└── tools\FlashingTool\                              # 烧录工具
```

**设备型号**: AWRL6844 (xWRL684x-evm)

**SDK版本**: MMWAVE_L_SDK_06_01_00_01

**重要说明**:
- ✅ 所有固件均针对 xWRL684x-evm 平台
- ✅ 所有工具均为TI官方版本
- ✅ .appimage文件可直接烧录（SDK编译输出）
- ✅ 详细源码路径请查看各子目录README.md

---

## 📋 项目说明

本工具提供AWRL6844固件烧录和管理的完整解决方案，包含：

1. ✅ **SBL Bootloader** - 二级引导程序
2. ✅ **HelloWorld应用** - 最简单的验证程序
3. ✅ **烧录工具** - arprog_cmdline_6844.exe
4. ✅ **Python GUI工具** - flash_tool.py v1.4.1 ⭐
5. ✅ **完整文档** - 分步操作指南

---

## 📁 目录结构

```
01-AWRL6844固件系统工具/
├── 📄 README.md                          # 本文件
├── 📄 操作指南.md                         # 详细烧录步骤
├── 📄 Flash布局说明.md                    # QSPI Flash内存布局
│
├── 📂 1-SBL_Bootloader/                  # SBL固件
│   ├── sbl.release.appimage              # SBL固件（可直接烧录）
│   └── README.md                         # SBL说明
│
├── 📂 2-HelloWorld_App/                  # 应用程序
│   ├── hello_world_system.release.appimage  # 应用固件（可直接烧录）
│   └── README.md                         # 应用说明
│
├── 📂 3-Tools/                           # 烧录工具
│   ├── arprog_cmdline_6844.exe           # 串口烧录工具（必需）⭐
│   ├── buildImage_creator.exe            # Meta Header生成工具
│   ├── flashHeader_creator.exe           # Flash Header生成工具
│   ├── metaImage_creator.exe             # Meta Image生成工具
│   └── README.md                         # 工具说明
│
├── 📂 4-Generated/                       # 临时文件目录
│   └── README.md                         # 预留给固件生成工具使用
│
└── 📂 5-Scripts/                         # Python烧录工具 ⭐⭐⭐
    ├── flash_tool.py                     # GUI烧录工具 v1.4.1
    ├── tabs/                             # 标签页模块
    │   ├── tab_basic.py                  # 基本烧录
    │   ├── tab_advanced.py               # 高级功能
    │   ├── tab_firmware_manager.py       # 固件管理（智能匹配）
    │   ├── tab_monitor.py                # 串口监视
    │   └── tab_ports.py                  # 端口管理
    ├── awrl6844_firmware_matcher.py      # 固件智能匹配引擎
    ├── config/                           # 配置文件
    │   └── scan_paths.json               # 固件扫描路径
    ├── requirements.txt                  # Python依赖
    ├── pyrightconfig.json                # Python类型检查配置
    ├── README.md                         # 详细使用说明
    └── 烧录操作指南.md                    # 操作指南
```

---

## 🎯 快速开始

### 前置条件

- [x] AWRL6844EVM开发板
- [x] USB数据线
- [x] Windows PC（已安装XDS110驱动）
- [x] Python 3.8+ (推荐使用Python GUI工具)

---

### Python GUI工具（推荐）⭐⭐⭐

```bash
cd 5-Scripts

# 首次运行安装依赖
pip install -r requirements.txt

# 启动GUI工具
python flash_tool.py
```

**flash_tool.py v1.4.1 特性**:

1. ✅ **5个功能标签页**
   - 基本烧录：完整/SBL/App烧录，自动扫描端口
   - 高级功能：固件分析、SBL检测、分步烧录
   - 固件管理：智能匹配SBL和App，一键添加
   - 串口监视：实时监控串口输出
   - 端口管理：COM口测试和管理

2. ✅ **智能固件匹配引擎**
   - 自动扫描多个目录查找固件
   - 智能匹配SBL和App配对
   - 置信度评分和推荐

3. ✅ **友好的图形界面**
   - 实时日志输出
   - 进度提示
   - 错误提醒

4. ✅ **完整的烧录功能**
   - 完整烧录（SBL + App）
   - 仅烧录SBL
   - 仅烧录App
   - SBL存在性检测

**详细使用说明**: 查看 `5-Scripts/README.md`

**耗时**: 约2-3分钟

**说明**: 固件文件(.appimage)由SDK编译生成，**可直接烧录**，无需生成meta或其他中间步骤。

---

### 手动使用arprog命令（高级）

```bash
cd 3-Tools

# 完整烧录（一次性烧录SBL+App）
.\arprog_cmdline_6844.exe -p COM3 ^
  -f1 "..\1-SBL_Bootloader\sbl.release.appimage" ^
  -f2 "..\2-HelloWorld_App\hello_world_system.release.appimage" ^
  -s SFLASH -c -cf

# 或分步烧录：
# 步骤1: 烧录SBL到0x2000
.\arprog_cmdline_6844.exe -p COM3 ^
  -f1 "..\1-SBL_Bootloader\sbl.release.appimage" ^
  -of1 8192 -s SFLASH -c

# 步骤2: 烧录App到0x42000
.\arprog_cmdline_6844.exe -p COM3 ^
  -f1 "..\2-HelloWorld_App\hello_world_system.release.appimage" ^
  -of1 270336 -s SFLASH -c
```

---

### 验证成功

1. 设置SOP开关为SOP_MODE2 (SOP0=OFF, SOP1=ON)
2. 打开串口终端（115200 8N1）
3. 按复位按钮
4. 应看到输出：
   ```
   [BOOTLOADER_PROFILE] Boot Media : FLASH
   Hello World from r5fss0-0 !
   Hello World from c66ss0 !
   All tests have passed!!
   ```

---

## 📊 QSPI Flash 布局

| 地址范围 | 大小 | 内容 | 说明 |
|---------|------|------|------|
| `0x000000 - 0x00001FFF` | 8KB | Reserved | 保留区域 |
| `0x00002000 - 0x00041FFF` | 256KB | SBL Bootloader | 实际~130KB，预留256KB |
| `0x00042000 - 0x001FFFFF` | ≤1.784MB | HelloWorld Meta | `M_META_IMAGE_OFFSET` |

详细说明见：[Flash布局说明.md](./Flash布局说明.md)

---

## 🔧 烧录流程详解

### 方式2：分步烧录（了解细节）

#### 阶段1：准备工作

```powershell
# 1. 检查固件文件
Test-Path "1-SBL_Bootloader\sbl.release.appimage"
Test-Path "2-HelloWorld_App\hello_world_system.release.appimage"

# 2. 检查串口（应看到两个连续COM口）
Get-WmiObject Win32_SerialPort | Select Name, DeviceID

# 3. 设置硬件模式（SOP_MODE0 = [0000] = 功能模式）
```

#### 阶段2：执行烧录

**选项A：使用脚本（推荐）**
```powershell
cd 5-Scripts
.\3_flash_sbl.bat    # 烧录SBL（2-3分钟）
.\4_flash_app.bat    # 烧录App（2-3分钟）
```

**选项B：直接使用arprog命令**
```powershell
# 烧录SBL到0x2000
cd 3-Tools
.\arprog_cmdline_6844.exe -p COM3 -f1 "..\1-SBL_Bootloader\sbl.release.appimage" -of1 8192 -s SFLASH -c

# 烧录App到0x42000
.\arprog_cmdline_6844.exe -p COM3 -f1 "..\2-HelloWorld_App\hello_world_system.release.appimage" -of1 270336 -s SFLASH -c
```

**参数说明**：
- `-p COM3`：串口号（根据实际情况调整）
- `-f1 file`：要烧录的文件
- `-of1 8192`：起始地址（8192=0x2000，270336=0x42000）
- `-s SFLASH`：目标存储为SPI Flash
- `-c`：擦除后写入

#### 阶段3：验证运行

```powershell
# 1. 切换到运行模式（SOP_MODE2 = [0011] = Debug UART模式）
# 2. 打开串口工具（COM4, 115200-8-N-1）
# 3. 复位板子，应看到：
#    - SBL启动信息
#    - 跳转到应用
#    - HelloWorld输出
```

完整步骤见：[操作指南.md](./操作指南.md)

---

## ❓ 常见问题

### Q1: 为什么需要先烧录SBL？
**A**: SBL是二级引导程序，负责从Flash加载应用程序。ROM Bootloader → SBL → Application是固定的启动顺序。

### Q2: .appimage文件可以直接烧录吗？
**A**: 可以！.appimage由SDK编译生成，arprog工具使用`-cf`参数会自动创建Flash Header，无需手动生成meta。

### Q3: 可以只烧录应用吗？
**A**: 不可以。首次烧录必须包含SBL。后续更新可以只更新应用部分（地址0x42000）。

### Q4: 串口没有输出？
**A**: 检查：
1. SOP开关是否设置为SOP_MODE2 = [0011]（Debug UART模式）
2. 串口参数：115200 8N1
3. 是否使用了正确的串口：
   - **COM3 (User UART)** - 烧录端口
   - **COM4 (Auxiliary Data Port)** - 雷达数据输出端口
   - HelloWorld程序输出可能在COM3或COM4，取决于程序配置
4. 是否按下复位按钮

---

## 📚 相关文档

- [操作指南.md](./操作指南.md) - 详细操作步骤
- [Flash布局说明.md](./Flash布局说明.md) - 内存布局详解
- [1-SBL_Bootloader/README.md](./1-SBL_Bootloader/README.md) - SBL详解
- [2-HelloWorld_App/README.md](./2-HelloWorld_App/README.md) - 应用详解
- [3-Tools/README.md](./3-Tools/README.md) - 工具使用说明

---

## 📝 技术支持

### 官方资源

- **SDK文档**: `MMWAVE_L_SDK_06_01_00_01/docs/api_guide_xwrL684x/`
- **SBL文档**: `sbl_8md.html`
- **示例代码**: `examples/drivers/boot/sbl/`

### 项目维护

- **创建日期**: 2025-12-12
- **SDK版本**: 06.01.00.01
- **芯片型号**: AWRL6844 (xWRL684x)
- **硬件版本**: AWRL6844EVM

---

## ⚠️ 重要说明

1. **备份原始固件**: 如果板子已有固件，建议先备份
2. **电压检查**: 确保供电电压正确（3.3V/5V）
3. **开关设置**: 烧录前务必检查SOP开关 (S7/S8)
4. **串口驱动**: 确保XDS110驱动正确安装

---

## 📊 项目状态

- [x] 目录结构创建
- [ ] 文件收集（SBL + HelloWorld）
- [ ] 配置文件适配
- [ ] 脚本编写
- [ ] 文档完善
- [ ] 烧录测试
- [ ] 功能验证

---

**下一步**: 查看 [操作指南.md](./操作指南.md) 开始烧录
