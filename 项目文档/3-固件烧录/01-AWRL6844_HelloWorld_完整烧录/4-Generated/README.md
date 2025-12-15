# 📁 临时文件目录

> **用于存放调试和验证过程中的临时文件**

---

## 📍 相关SDK工具路径

**调试工具位置**（如使用）:
```
C:\ti\MMWAVE_L_SDK_06_01_00_01\tools\FlashingTool\
├── buildImage_creator.exe           # 镜像分析工具（用于解析.appimage）
└── arprog_cmdline_6844.exe          # Flash读回验证（-r 参数）
```

**设备型号**: AWRL6844 (xWRL684x-evm)

---

## 📋 目录用途

此目录**不是必需的**，仅在以下场景使用：

1. **Flash读回验证** - 从FLASH读取内容进行对比
2. **调试工具输出** - buildImage_creator等调试工具的输出
3. **测试文件** - 临时测试用的二进制文件

---

## 📂 目录结构

```
4-Generated/
├── README.md           # 本文件
└── (临时文件)          # 脚本运行时动态生成
    ├── *.bin          # 二进制文件
    └── temp/          # 临时提取的文件
```

---

## 🔄 文件生命周期

### 自动生成

某些操作会在此目录生成临时文件：

```powershell
# Flash读回验证（如果使用）
arprog_cmdline_6844.exe -p COM3 -r 0x2000 -s 130000 -o 4-Generated/sbl_readback.bin
arprog_cmdline_6844.exe -p COM3 -r 0x42000 -s 218000 -o 4-Generated/app_readback.bin
```

### 手动清理

手动删除临时文件：

```powershell
# 删除所有.bin文件（如果存在）
Remove-Item "4-Generated\*.bin" -Force -ErrorAction SilentlyContinue

# 删除temp目录（如果存在）
Remove-Item "4-Generated\temp" -Recurse -Force -ErrorAction SilentlyContinue
```

---

## ⚠️ 重要说明

### 正常烧录不需要此目录

- ✅ `.appimage` 文件可直接烧录
- ✅ `arprog -cf` 参数自动创建Flash Header
- ❌ **不需要**在此目录生成中间文件
- ❌ **不需要**Meta Image生成步骤

### 何时会用到

1. **验证烧录** - 读回Flash内容对比
2. **调试固件** - 使用buildImage_creator分析.appimage结构
3. **高级操作** - 手动提取和组合固件

---

## 🧹 清理建议

定期清理此目录以节省空间：

```powershell
# 删除所有临时文件
Remove-Item "4-Generated\*.bin" -Force -ErrorAction SilentlyContinue
Remove-Item "4-Generated\temp" -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item "4-Generated\readback" -Recurse -Force -ErrorAction SilentlyContinue
```

---

## 📚 相关文档

- [../README.md](../README.md) - 项目总览
- [../5-Scripts/README.md](../5-Scripts/README.md) - 脚本说明
- [../操作指南.md](../操作指南.md) - 烧录步骤

---

**提示**: 此目录可以保持为空，不影响正常烧录操作。

**更新日期**: 2025-12-15
