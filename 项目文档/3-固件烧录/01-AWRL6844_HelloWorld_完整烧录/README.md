# 🚀 AWRL6844 HelloWorld 完整烧录项目

> **项目目标**: 从空白板子到运行HelloWorld，实现完整的QSPI Flash烧录流程

## 📋 项目说明

本项目提供AWRL6844从**完全空白板子**到**成功运行HelloWorld示例**的完整烧录方案，包含：

1. ✅ **SBL Bootloader** - 二级引导程序
2. ✅ **HelloWorld应用** - 最简单的验证程序
3. ✅ **配套配置文件** - 所有必需的.json/.cfg文件
4. ✅ **烧录工具** - arprog_cmdline_6844.exe
5. ✅ **完整文档** - 分步操作指南

---

## 📁 目录结构

```
01-AWRL6844_HelloWorld_完整烧录/
├── 📄 README.md                          # 本文件
├── 📄 操作指南.md                         # 详细烧录步骤
├── 📄 Flash布局说明.md                    # QSPI Flash内存布局
│
├── 📂 1-SBL_Bootloader/                  # SBL引导程序
│   ├── sbl.release.appimage              # SBL固件（必须）
│   ├── metaimage_cfg.release.json        # SBL Meta配置
│   └── README.md                         # SBL说明
│
├── 📂 2-HelloWorld_App/                  # HelloWorld应用
│   ├── hello_world_system.release.appimage  # 应用固件
│   ├── metaimage_cfg.release.json           # 应用Meta配置
│   └── README.md                            # 应用说明
│
├── 📂 3-Tools/                           # 烧录工具
│   ├── arprog_cmdline_6844.exe           # 串口烧录工具
│   ├── buildImage_creator.exe            # 镜像提取工具
│   ├── metaImage_creator.exe             # Meta镜像生成工具
│   └── README.md                         # 工具说明
│
├── 📂 4-Generated/                       # 生成文件目录（执行后产生）
│   ├── sbl_meta.bin                      # SBL Meta Image
│   ├── hello_world_meta.bin              # App Meta Image
│   └── README.md                         # 说明
│
└── 📂 5-Scripts/                         # 自动化脚本
    ├── 1_generate_sbl_meta.bat           # 生成SBL Meta Image
    ├── 2_generate_app_meta.bat           # 生成App Meta Image
    ├── 3_flash_sbl.bat                   # 烧录SBL
    ├── 4_flash_app.bat                   # 烧录应用
    ├── clean_generated.bat               # 清理生成文件
    ├── full_flash.bat                    # 完整烧录流程（推荐）
    └── README.md                         # 脚本说明
```

---

## 🎯 快速开始

### 前置条件

- [x] AWRL6844EVM开发板
- [x] USB数据线（Type-C或Micro-USB）
- [x] Windows PC（已安装驱动）
- [x] 串口调试工具（推荐：TeraTerm、PuTTY）

### 方式1: 完整自动烧录（推荐新手）

```bash
cd 5-Scripts
full_flash.bat COM3
```

**执行内容**:
1. ✅ 清理旧文件
2. ✅ 生成SBL Meta Image
3. ✅ 生成App Meta Image
4. ✅ 烧录SBL到0x2000
5. ✅ 烧录App到0x42000
6. ✅ 自动验证

**耗时**: 约2分钟

---

### 方式2: 分步手动烧录（推荐调试）

```bash
cd 5-Scripts

# Step 1: 生成Meta Images
1_generate_sbl_meta.bat
2_generate_app_meta.bat

# Step 2: 烧录SBL到Flash
3_flash_sbl.bat COM3

# Step 3: 烧录HelloWorld应用
4_flash_app.bat COM3
```

### 验证成功

1. 打开串口终端（115200 8N1）
2. 按复位按钮
3. 应看到输出：
   ```
   ***** SBL Starting *****
   Loading Application...
   Hello World!
   ```

---

## 📊 QSPI Flash 布局

| 地址范围 | 大小 | 内容 | 说明 |
|---------|------|------|------|
| `0x000000 - 0x00001FFF` | 8KB | Flash Header & 预留 | ROM Header + 对齐 |
| `0x00002000 - 0x00041FFF` | ~248KB | SBL Bootloader | `M_META_SBL_OFFSET` |
| `0x00042000 - 0x001FFFFF` | ≤1.784MB | HelloWorld Meta | `M_META_IMAGE_OFFSET` |

详细说明见：[Flash布局说明.md](./Flash布局说明.md)

---

## 🔧 烧录流程详解

### Phase 1: 准备Meta Images

```bash
# 1.1 从SBL的.appimage提取核心镜像
buildImage_creator.exe -i 1-SBL_Bootloader/sbl.release.appimage

# 1.2 生成SBL Meta Image
metaImage_creator.exe -config 1-SBL_Bootloader/metaimage_cfg.release.json
```

### Phase 2: 烧录SBL到Flash

```bash
# 2.1 设置开发板为SOP_MODE1（QSPI刷写模式）
# 2.2 连接串口
# 2.3 执行烧录
arprog_cmdline_6844.exe -p COM3 -f 1-SBL_Bootloader/sbl.release.appimage -o 0x2000
```

### Phase 3: 烧录应用

```bash
# 3.1 生成App Meta Image
buildImage_creator.exe -i 2-HelloWorld_App/hello_world_system.release.appimage
metaImage_creator.exe -config 2-HelloWorld_App/metaimage_cfg.release.json

# 3.2 烧录App
arprog_cmdline_6844.exe -p COM3 -f 2-HelloWorld_App/hello_world_system.release.appimage -o 0x42000
```

### Phase 4: 验证运行

```bash
# 4.1 切换到SOP_MODE2（应用/功能模式）
# 4.2 复位开发板
# 4.3 查看串口输出
```

完整步骤见：[操作指南.md](./操作指南.md)

---

## ❓ 常见问题

### Q1: 为什么需要先烧录SBL？

**A**: SBL是二级引导程序，负责从Flash加载应用程序。没有SBL，应用无法启动。

### Q2: Flash Header在哪里？

**A**: Flash Header包含在SBL Meta Image的前256字节，由`metaImage_creator.exe`自动生成。

### Q3: 可以只烧录应用吗？

**A**: 不可以。首次烧录必须包含SBL。后续更新可以只更新应用部分（地址0x42000）。

### Q4: 串口没有输出？

**A**: 检查：
1. SOP开关是否设置为SOP_MODE2 (01)（应用模式）
2. 串口参数：115200 8N1
3. 是否按下复位按钮

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
