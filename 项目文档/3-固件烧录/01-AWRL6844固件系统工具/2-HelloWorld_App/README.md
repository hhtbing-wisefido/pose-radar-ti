# 📱 HelloWorld 应用程序固件

> **多核演示应用 - R5F + C66x DSP + RF固件**

---

## 📍 SDK来源路径

**官方SDK位置**:
```
C:\ti\MMWAVE_L_SDK_06_01_00_01\examples\hello_world\xwrL684x-evm\
├── r5fss0-0_freertos\               # R5F主核（FreeRTOS）
│   ├── example.syscfg               # SysConfig配置文件
│   └── Release\*.appimage
├── c66ss0_freertos\                 # C66x DSP核
│   ├── example.syscfg
│   └── Release\*.appimage
└── system\                          # 系统级多核项目
    └── Release\hello_world_system.release.appimage  # 最终固件 ⭐
```

**设备型号**: AWRL6844 (xWRL684x-evm)

---

## 📄 文件说明

### hello_world_system.release.appimage

- **文件类型**: 可烧录固件文件
- **大小**: 约218KB
- **来源**: TI SDK编译生成
- **源码路径**: `C:\ti\MMWAVE_L_SDK_06_01_00_01\examples\hello_world`
- **烧录地址**: 0x42000 (Flash偏移270336字节)
- **可用空间**: 最大1.78MB (SBL_MAX_METAIMAGE_SIZE)

---

## 🎯 应用功能

### HelloWorld演示

这是TI官方的多核演示程序，验证以下功能：

1. **R5F主核** (ARM Cortex-R5F)
   - 系统初始化
   - 串口通信
   - 打印 "Hello World from r5f0-0!"

2. **C66x DSP核**
   - DSP核心启动
   - 打印 "Hello World from c66ss0!"

3. **RF固件核** (R5F从核)
   - 射频子系统初始化
   - 打印 "Hello World from r5f0-1!"

---

## 🔄 启动流程

```
SBL启动完成
   ↓
读取App Meta Header (0x42000)
   ↓
解析多核镜像结构
   ↓
加载R5F主核到RAM
   ↓
加载C66x DSP核到RAM
   ↓
加载RF固件核到RAM
   ↓
按顺序启动各核
   ↓
各核运行HelloWorld
```

---

## 📦 固件结构

### .appimage文件组成

```
hello_world_system.release.appimage:
  ├── Meta Header (512字节)
  │   ├── Magic Number
  │   ├── 镜像数量: 3
  │   ├── 各核加载地址
  │   └── 各核入口点
  │
  ├── R5F主核镜像 (~80KB)
  │   └── ARM Cortex-R5F代码
  │
  ├── C66x DSP镜像 (~100KB)
  │   └── TI C66x DSP代码
  │
  └── RF固件镜像 (~30KB)
      └── R5F从核代码
```

---

## 🚀 烧录方法

### 前提条件

**⚠️ 必须先烧录SBL！**

如果是首次烧录或SBL不存在，必须先烧录SBL到0x2000地址。

### 方法1: 使用Python工具（推荐）⭐

```powershell
cd ..\5-Scripts
python flash_tool.py
# 选择"完整烧录"或"仅烧录App"
```

### 方法2: 直接使用arprog命令

```powershell
cd ..\3-Tools

# 一次性烧录SBL+App（推荐）
.\arprog_cmdline_6844.exe -p COM3 ^
  -f1 "..\1-SBL_Bootloader\sbl.release.appimage" ^
  -f2 "..\2-HelloWorld_App\hello_world_system.release.appimage" ^
  -s SFLASH -c -cf

# 或仅烧录App（SBL已存在）
.\arprog_cmdline_6844.exe -p COM3 ^
  -f1 "..\2-HelloWorld_App\hello_world_system.release.appimage" ^
  -of1 270336 ^
  -s SFLASH -c
```

**参数说明**：
- `-p COM3`: 串口号
- `-f1 file`: 要烧录的文件
- `-of1 270336`: Flash起始地址（270336 = 0x42000）
- `-s SFLASH`: 目标存储（SPI Flash）
- `-c`: 擦除后写入

---

## 🖥️ 查看运行结果

### 硬件设置

1. **SOP开关设置**: SOP_MODE2 = [0011] (Debug UART模式)
2. **串口连接**: 连接COM4（Debug UART）
3. **串口参数**: 115200-8-N-1

### 期望输出

```
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
All tests have passed!!
```

---

## ⚠️ 重要说明

### .appimage可直接烧录

- ✅ SDK编译时已生成完整的.appimage文件
- ✅ 包含完整的Meta Header和多核镜像
- ✅ arprog会自动处理Flash写入
- ❌ **不需要**使用buildImage_creator提取
- ❌ **不需要**使用metaImage_creator生成meta

### 必须先有SBL

- 应用程序无法自己启动
- 必须由SBL从Flash加载
- 确保SBL已烧录到0x2000

---

## 💾 Flash地址分配

| 地址 | 内容 | 大小 |
|------|------|------|
| 0x00000 - 0x01FFF | Flash Header | 8KB |
| 0x02000 - 0x41FFF | SBL | 256KB (预留) |
| **0x42000 - 0x1FFFFF** | **应用程序** | **最大1.78MB** |

---

## 🔧 源码位置

```
MMWAVE_L_SDK_06_01_00_01/
└── examples/
    └── hello_world/
        └── awrl6844_hello_world_system/
            ├── r5fss0-0_nortos/    # R5F主核
            ├── c66ss0/             # DSP核
            └── r5fss0-1_nortos/    # RF核
```

---

## 📚 相关文档

- [../README.md](../README.md) - 项目总览
- [../操作指南.md](../操作指南.md) - 详细烧录步骤
- [../Flash布局说明.md](../Flash布局说明.md) - Flash内存布局
- [../5-Scripts/README.md](../5-Scripts/README.md) - 脚本使用说明

---

## 🐛 常见问题

### Q: 为什么烧录后没有输出？

**A**: 检查：
1. SOP开关是否为SOP_MODE2 = [0011]
2. 串口是否连接到COM4（Debug UART）
3. 串口参数是否正确（115200-8-N-1）
4. SBL是否已正确烧录
5. 是否按下复位按钮

### Q: 可以自定义HelloWorld程序吗？

**A**: 可以！修改SDK源码后重新编译：
1. 修改 `examples/hello_world/` 目录下的源文件
2. 使用CCS或Make重新编译
3. 会生成新的 `hello_world_system.release.appimage`
4. 烧录新固件到0x42000

### Q: 如何添加更多功能？

**A**: 参考SDK中的其他示例：
- `examples/drivers/` - 外设驱动示例
- `examples/mmwave_dfp/` - 雷达数据处理
- 修改后重新编译生成.appimage文件

### Q: 应用程序最大可以多大？

**A**: 
- 最大Meta Image大小: 1.78MB (SBL_MAX_METAIMAGE_SIZE)
- 包含Meta Header和所有核的代码
- 超过此大小会烧录失败

---

**更新日期**: 2025-12-15  
**SDK版本**: 06.01.00.01
