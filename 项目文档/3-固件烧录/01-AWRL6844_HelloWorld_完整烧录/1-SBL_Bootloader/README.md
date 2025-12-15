# 📦 SBL Bootloader 固件

> **二级引导程序（Secondary Bootloader）**

---

## 📄 文件说明

### sbl.release.appimage

- **文件类型**: 可烧录固件文件
- **大小**: 约130KB
- **来源**: TI SDK编译生成
- **烧录地址**: 0x2000 (Flash偏移8192字节)
- **预留空间**: 256KB (0x40000)

---

## 🔍 SBL功能

### 主要职责

1. **硬件初始化**
   - 配置时钟系统
   - 初始化DDR内存
   - 设置外设接口

2. **加载应用程序**
   - 从Flash读取应用Meta Image
   - 解析多核镜像结构
   - 加载各核代码到RAM

3. **启动应用**
   - 验证应用完整性
   - 配置各核启动参数
   - 跳转到应用入口点

---

## 🔄 启动流程

```
上电/复位
   ↓
ROM Bootloader (芯片固化)
   ↓
读取Flash Header (0x0)
   ↓
加载SBL到RAM (从0x2000)
   ↓
执行SBL
   ↓
SBL加载应用 (从0x42000)
   ↓
应用程序运行
```

---

## 💾 Flash布局

| 地址范围 | 大小 | 内容 | 说明 |
|---------|------|------|------|
| 0x0000 - 0x1FFF | 8KB | Flash Header | ROM识别标记 |
| 0x2000 - 0x41FFF | 256KB | SBL | 实际~130KB，预留256KB |
| 0x42000 - 0x1FFFFF | ~1.78MB | Application | 应用程序空间 |

---

## 🚀 烧录方法

### 方法1: 使用完整烧录脚本（推荐）

```powershell
cd ..\5-Scripts
.\full_flash.bat COM3
```

### 方法2: 单独烧录SBL

```powershell
cd ..\5-Scripts
.\3_flash_sbl.bat COM3
```

### 方法3: 直接使用arprog命令

```powershell
cd ..\3-Tools
.\arprog_cmdline_6844.exe -p COM3 ^
  -f1 "..\1-SBL_Bootloader\sbl.release.appimage" ^
  -of1 8192 ^
  -s SFLASH -c
```

**参数说明**：
- `-p COM3`: 串口号
- `-f1 file`: 要烧录的文件
- `-of1 8192`: Flash起始地址（8192 = 0x2000）
- `-s SFLASH`: 目标存储（SPI Flash）
- `-c`: 擦除后写入

---

## ⚠️ 重要说明

### .appimage可直接烧录

- ✅ SDK编译时已生成完整的.appimage文件
- ✅ 包含所有必要的Meta Header信息
- ✅ arprog会自动处理Flash Header
- ❌ **不需要**使用buildImage_creator提取
- ❌ **不需要**使用metaImage_creator生成meta

### 首次烧录必须包含SBL

- ROM Bootloader固定从Flash加载SBL
- 没有SBL，应用无法启动
- SBL负责加载和启动应用程序

---

## 🔧 源码位置

```
MMWAVE_L_SDK_06_01_00_01/
└── examples/
    └── drivers/
        └── boot/
            └── sbl/
                ├── sbl_main.c          # SBL主程序
                ├── sbl_qspi.c          # QSPI Flash驱动
                └── sbl.h               # 地址定义
```

**关键地址定义**（见sbl.h）：
```c
#define M_META_SBL_OFFSET      0x2000U              // SBL起始地址
#define M_META_IMAGE_OFFSET    (M_META_SBL_OFFSET + (256U * 1024U))  // 0x42000
```

---

## 📚 相关文档

- [../README.md](../README.md) - 项目总览
- [../操作指南.md](../操作指南.md) - 详细烧录步骤
- [../Flash布局说明.md](../Flash布局说明.md) - Flash内存布局
- [../5-Scripts/README.md](../5-Scripts/README.md) - 脚本使用说明

---

## 🐛 常见问题

### Q: 为什么SBL预留256KB但实际只有130KB？

**A**: 为SBL功能扩展预留空间。未来SDK更新可能增加SBL功能，256KB确保有足够空间。应用起始地址固定在0x42000，保证兼容性。

### Q: 可以只烧录应用不烧录SBL吗？

**A**: 首次烧录必须包含SBL。后续更新应用时，如果SBL没有变化，可以只烧录应用（0x42000地址）。

### Q: 如何验证SBL是否正确烧录？

**A**: 
1. 使用串口工具（COM4, 115200-8-N-1）
2. 设置SOP_MODE2 = [0011] (Debug UART模式)
3. 复位板子
4. 应看到：`[BOOTLOADER] Boot Media: FLASH`

---

**更新日期**: 2025-12-15  
**SDK版本**: 06.01.00.01
