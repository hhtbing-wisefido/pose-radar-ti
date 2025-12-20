# AWRL6844EVM 固件智能管理系统

## 📖 项目简介

专门为 **AWRL6844评估板** 设计的固件扫描、筛选、匹配工具。基于深度学习TI官方文档规律，实现智能化的固件管理和配置推荐。

**🎯 最新更新（v1.6.0）**：改进智能匹配算法，大幅提升InCabin Demo等固件的配置文件推荐准确性！详见 [智能匹配算法改进方案.md](./智能匹配算法改进方案.md)

### ✨ 核心功能

1. **🔍 智能扫描**
   - 自动扫描TI SDK目录
   - 精准识别AWRL6844兼容固件
   - 区分应用固件、SBL固件、雷达配置文件

2. **📊 多层级筛选**
   - 基于目录层级的动态筛选
   - 基于文件名模式的精确匹配
   - 支持关键词搜索

3. **🎯 智能匹配（v2改进版）** ⭐
   - 基于SDK路径关联的智能推荐
   - 配置文件名语义解析
   - 自动推荐匹配的SBL固件
   - 根据应用场景推荐雷达配置
   - 显示匹配度评分（最高200+分）

4. **📦 详细信息展示**
   - 完整文件路径（可一键复制）
   - 固件功能详解
   - 应用场景说明

---

## 🚀 快速开始

### 环境要求

- **Python**: 3.8+
- **依赖**: PyQt6

### 安装步骤

```powershell
# 1. 安装依赖
pip install PyQt6

# 2. 运行程序
cd "D:\7.项目资料\Ti雷达项目\project-code\code_search"
python awrl6844_gui_app.py
```

---

## 📚 使用指南

### 步骤1: 配置扫描目录

默认已配置以下目录：
- `C:\ti\MMWAVE_L_SDK_06_01_00_01`
- `C:\ti\radar_toolbox_3_30_00_06`

可以添加/删除自定义目录。

### 步骤2: 开始扫描

点击"开始扫描"按钮，系统将：
- 递归扫描所有子目录
- 识别`.appimage`固件文件
- 识别`.cfg`配置文件
- 自动过滤非6844固件

### 步骤3: 查看结果

#### 📦 应用固件标签页
- 查看所有AWRL6844应用固件
- 按类别、处理器筛选
- 关键词搜索
- 查看详细信息（路径、类别、处理器、版本等）

#### 🔧 SBL固件标签页
- 查看所有SBL启动固件
- 标准版/轻量版/镜像选择
- Flash地址和大小信息
- 功能说明

#### ⚙️ 雷达配置标签页
- 查看所有雷达参数配置文件
- 按应用场景、模式、功耗筛选
- TX/RX通道信息
- 检测距离、工作模式详情

#### 🎯 智能匹配标签页
- 选择应用固件
- 自动推荐Top 3 SBL固件
- 自动推荐Top 5 雷达配置文件
- 显示匹配度评分

---

## 🔍 筛选规则详解

### 应用固件筛选原则

基于 `2025-12-17_固件目录与文件命名规律分析.md`：

✅ **必须包含（MMWAVE_L_SDK）**
```
/xwrL684x-evm/    ← 官方平台标识
```

✅ **必须包含（radar_toolbox）**
```
路径包含: AWRL6844, 6844, _6844_
文件名包含: xWRL6844, _6844_, L6844
```

❌ **必须排除**
```
xwrl1432, L1432, xwrl6432, L6432
awr2944, awr2544, awr29xx, iwrl6432
```

### SBL固件筛选原则

基于 `2025-12-17_SBL固件规律分析.md`：

✅ **路径特征**
```
/drivers/boot/sbl/      ← MMWAVE_L_SDK标准路径
/Fundamentals/SBL_*/    ← radar_toolbox路径
```

✅ **文件名特征**
```
sbl.release.appimage
sbl_lite.release.appimage
sbl_image_select.Release.appimage
```

✅ **处理器配置**
```
只使用: r5fss0-0_nortos    ← SBL专用配置
```

### 雷达配置文件筛选原则

基于 `2025-12-17_AWRL6844EVM雷达参数配置文件清单.md`：

✅ **路径特征**
```
/chirp_configs/         ← 配置文件目录
/config_file/
```

✅ **文件名特征**
```
*.cfg                   ← 扩展名
包含: 6844, 68xx, xwrl68, 6843
```

❌ **排除系统配置**
```
.syscfg, ti_*, board_*, rtos.cfg
```

---

## 🎯 匹配算法

### SBL匹配评分规则

| 条件 | 评分 | 说明 |
|-----|------|------|
| 同一SDK | +50分 | MMWAVE_L_SDK优先 |
| 标准版 | +30分 | 完整功能推荐 |
| 轻量版 | +20分 | 快速启动 |
| xwrL684x平台 | +20分 | 硬件匹配 |

### 配置文件匹配评分规则（v2改进版）⭐

**2025-12-20更新：改进算法，提高InCabin Demo等固件的配置推荐准确性**

详见：[智能匹配算法改进方案.md](./智能匹配算法改进方案.md)

#### 新评分体系（总分可达200+）

| 条件 | 评分 | 说明 |
|-----|------|------|
| **同Demo目录** | +100分 | 固件与配置在同一Demo项目下（最强关联） |
| **同SDK关联目录** | +80分 | 例如：固件在source/ti/examples，配置在tools/visualizers |
| **配置文件名语义匹配** | +60分 | InCabin专用：cpd.cfg、sbr.cfg、intrusion_detection.cfg |
| 应用场景文本匹配 | +20分 | 原40分降至20分，降低宽泛匹配权重 |
| 芯片型号匹配 | +20分 | 6844专用配置（原30分降至20分） |
| 检测距离合理性 | +15分 | 短距离≤10m、中距离≤50m、长距离>50m |
| 功耗模式匹配 | +5分 | 低功耗/_LP后缀匹配 |

#### 改进要点

1. **新增SDK路径关联**：同一SDK内的固件和配置获得高分（180分）
2. **新增文件名语义解析**：识别cpd、sbr、intrusion等关键词（60分）
3. **降低宽泛文本匹配**：避免误匹配（40分→20分）
4. **InCabin Demo特殊优化**：精准推荐专用配置文件

---

## 📂 项目结构

```
code_search/
├── awrl6844_firmware_matcher.py   # 核心匹配引擎
├── awrl6844_gui_app.py            # GUI主程序
└── README.md                      # 本文档
```

---

## 🔧 核心类说明

### AWRL6844FirmwareMatcher

**功能**: 固件扫描和匹配核心引擎

**主要方法**:
- `scan_directory(directory, recursive)` - 扫描目录
- `match_sbl_for_firmware(firmware)` - 匹配SBL
- `match_configs_for_firmware(firmware)` - 匹配配置文件

**数据结构**:
- `FirmwareInfo` - 应用固件信息
- `SBLInfo` - SBL固件信息
- `ConfigInfo` - 雷达配置文件信息

### AWRL6844GUI

**功能**: PyQt6图形界面

**主要标签页**:
- **扫描与管理** - 目录管理和扫描控制
- **应用固件** - 应用固件浏览和筛选
- **SBL固件** - SBL固件浏览
- **雷达配置** - 配置文件浏览和筛选
- **智能匹配** - 固件配置推荐

---

## 📊 目录层级解析

### MMWAVE_L_SDK (7层结构)

```
C:/ti/MMWAVE_L_SDK_06_01_00_01/
└── [SDK重复层]/                    可选
    └── examples/                   示例根目录
        └── [功能类别]/             control/datapath/drivers/kernel/mmw_demo
            └── [具体功能]/         gpio/uart/i2c/hwa/edma...
                └── xwrL684x-evm/       硬件平台 ⭐
                    └── [处理器_OS]/    r5fss0-0_freertos/nortos
                        └── ti-arm-clang/   编译器
                            └── [固件].appimage
```

**筛选使用的层级**:
1. **第2层**: examples
2. **第3层**: 功能类别（中文选项）
3. **第4层**: 具体功能（中文选项）
4. **第5层**: xwrL684x-evm（固定）
5. **第6层**: 处理器配置（中文选项）
6. **第7层**: 编译器（中文选项）

### radar_toolbox (5-6层结构)

```
C:/ti/radar_toolbox_3_30_00_06/
└── source/ti/examples/
    └── [应用场景大类]/             Automotive/Industrial/Fundamentals
        └── [具体Demo]/             People_Tracking/Area_Scanner...
            └── prebuilt_binaries/  预编译目录
                └── [芯片型号]/     可选，6844/6843/6432
                    └── [固件].appimage
```

**筛选使用的层级**:
1. **第2层**: source/ti/examples
2. **第3层**: 应用场景大类（中文选项）
3. **第4层**: 具体Demo（中文选项）
4. **第5层**: prebuilt_binaries
5. **第6层**: 芯片型号（可选）

---

## 📝 文件命名模式

### 应用固件命名

**MMWAVE_L_SDK格式**:
```
[驱动名]_[功能].[版本].appimage
示例: gpio_led_blink.release.appimage
```

**radar_toolbox格式**:
```
[功能]_demo[_变体].[版本].appimage
示例: motion_and_presence_detection_demo.Release.appimage
```

### SBL固件命名

```
sbl[_变体].release.appimage
sbl_lite.release.appimage
sbl_image_select.Release.appimage
```

### 配置文件命名

```
[应用]_[参数]_[模式].cfg
示例: vod_6843_aop_overhead_2row.cfg
      xWRL6844_4T4R_tdm.cfg
```

---

## 🎓 学习资源

本系统基于深度学习以下文档规律：

### 必读文档（项目文档/2-开发记录/2025-12-17/）

1. **芯片命名规则**
   - `2025-12-17_TI毫米波雷达芯片命名规则详解.md`
   - 理解6844的命名含义和系列归属

2. **固件列表**
   - `2025-12-17_TI_SDK固件列表.md`
   - 354个固件的完整清单

3. **目录规律**
   - `2025-12-17_固件目录与文件命名规律分析.md`
   - 7层和5-6层目录结构详解

4. **SBL规律**
   - `2025-12-17_SBL固件规律分析.md`
   - SBL的作用、类型、选择

5. **配置文件关系**
   - `2025-12-17_TI雷达固件配置文件关系详解.md`
   - 理解.syscfg、RTOS .cfg、雷达.cfg的区别

6. **配置文件清单**
   - `2025-12-17_AWRL6844EVM雷达参数配置文件清单.md`
   - 45个配置文件的分类和应用

---

## 🔄 版本历史

### v1.6.0 (2025-12-20) ⭐ **算法改进版**
- ✅ **改进智能匹配算法v2**
  - 新增SDK路径关联评分（100分+80分）
  - 新增配置文件名语义解析（60分）
  - 降低宽泛文本匹配权重（40分→20分）
  - InCabin Demo配置推荐准确性大幅提升
- ✅ **新增辅助方法**
  - `_extract_sdk_root()` - SDK根目录提取
  - `_is_same_demo_directory()` - 同Demo目录判断
  - `_is_related_in_sdk()` - SDK内关联目录判断
  - `_parse_config_filename()` - 文件名语义解析
  - `_is_short_range_app()` - 短距离应用判断
- ✅ **完善文档**
  - 创建《智能匹配算法改进方案.md》
  - 详细说明改进逻辑和评分规则
  - 提供InCabin Demo匹配示例

### v1.0 (2025-12-17)
- ✅ 初始版本发布
- ✅ 实现智能扫描功能
- ✅ 实现多层级筛选
- ✅ 实现智能匹配算法
- ✅ PyQt6图形界面
- ✅ 支持AWRL6844专用筛选规则

---

## 📞 技术支持

如有问题，请参考：
- TI官方文档: `知识库/雷达模块/技术文档/`
- 项目文档: `项目文档/2-开发记录/2025-12-17/`

---

## 📄 许可证

本项目专为AWRL6844EVM开发，仅供学习和开发使用。

---

**专为AWRL6844EVM设计 | 基于TI官方SDK规律 | 智能化固件管理** 🚀
