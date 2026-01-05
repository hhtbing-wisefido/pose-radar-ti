# 📡 雷达配置文件目录

> **用途**: 存放AWRL6844健康检测项目专用的雷达配置文件
> **创建日期**: 2026-01-05

---

## 📊 配置文件说明

### 🚨 重要提醒

**雷达工作流程**：
```
烧录固件(.appimage) → 发送配置文件(.cfg) → sensorStart → 雷达开始输出数据
```

没有配置文件，雷达无法正常工作！

---

## 📁 文件列表

| 文件 | 用途 | 状态 |
|------|------|------|
| `profile_health.cfg` | 健康检测专用雷达chirp配置 | 🔲 待创建 |
| `zone_default.cfg` | 默认区域划分配置 | 🔲 待创建 |

---

## 📚 SDK配置文件参考

开发前可参考以下SDK配置文件：

### 通用配置文件

| 文件 | 路径 | 用途 |
|------|------|------|
| `6844_profile_4T4R_tdm.cfg` | `C:\ti\MMWAVE_L_SDK_06_01_00_01\tools\visualizer\tmp\` | SDK Visualizer默认配置 ✅ |
| `6844_profile_4T4R_tdm.cfg` | `C:\ti\radar_toolbox_3_30_00_06\tools\mmwave_data_recorder\src\cfg\` | 数据采集工具配置 ✅ |

### InCabin Demo专用配置

| 文件 | 路径 | 用途 |
|------|------|------|
| `cpd.cfg` | `chirpConfigs6844\` | 儿童存在检测(CPD) |
| `sbr.cfg` | `chirpConfigs6844\` | 安全带提醒(SBR) |
| `intrusion_detection.cfg` | `chirpConfigs6844\` | 入侵检测 |
| `intrusion_detection_LP.cfg` | `chirpConfigs6844\` | 低功耗入侵检测 |

> **完整路径**: `C:\ti\radar_toolbox_3_30_00_06\tools\visualizers\AWRL6844_Incabin_GUI\src\chirpConfigs6844\`

---

## 🛠️ 配置工具

### 1. SDK Visualizer（首选）

```powershell
# 启动工具
Start-Process "C:\ti\MMWAVE_L_SDK_06_01_00_01\tools\visualizer\visualizer.exe"
```

**功能**:
- ✅ 发送配置文件
- ✅ 实时数据可视化
- ✅ 固件烧录

### 2. 雷达配置参数研究工具

**位置**: `项目文档/3-固件工具/05-雷达配置参数研究/`

**内容**:
- `radar_test_gui.py` - 配置文件测试GUI
- `雷达配置文件深度分析_v2.0_Part1.md` - 参数详细解释
- `雷达配置文件深度分析_v2.0_Part2.md` - 配置模板和最佳实践
- `TI_6844_profile_4T4R_tdm.cfg` - 标准TI配置文件样本

---

## 📝 配置文件格式

配置文件是纯文本格式，包含一系列CLI命令：

```
% 注释行
sensorStop
flushCfg
channelCfg 15 15 0
profileCfg 0 60.25 7 7 160 0 0 60 0.5 512 6000 0 0 40
chirpCfg 0 0 0 0 0 0 0 1
chirpCfg 1 1 0 0 0 0 0 2
chirpCfg 2 2 0 0 0 0 0 4
chirpCfg 3 3 0 0 0 0 0 8
frameCfg 0 3 32 0 100 1 0
...
sensorStart
```

---

## 🔗 相关文档

- **参数详解**: `项目文档/3-固件工具/05-雷达配置参数研究/雷达配置文件深度分析_v2.0_Part1.md`
- **配置模板**: `项目文档/3-固件工具/05-雷达配置参数研究/雷达配置文件深度分析_v2.0_Part2.md`
- **实测结果**: `项目文档/4-测试实验结果/2.雷达配置实际测试结果.md`
- **固件匹配**: `项目文档/2-开发记录/2025-12-23/2025-12-23_固件与雷达配置匹配关系报告.md`

---

> 📅 **创建日期**: 2026-01-05
