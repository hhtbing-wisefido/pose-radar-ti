# 🛠️ Radar Toolbox 开发环境说明

**日期**: 2025-12-04  
**版本**: radar_toolbox_3_30_00_06  
**适用芯片**: xWRL6844

---

## 📌 核心问题回答

### ❓ 能否在 VS Code 中开发？

**✅ 可以！但有限制**

| 开发内容 | VS Code | CCS IDE | 推荐方案 |
|---------|---------|---------|----------|
| **Python GUI 可视化工具** | ✅ 完全支持 | ❌ 不适用 | **VS Code** |
| **算法开发和原型验证** | ✅ 完全支持 | ❌ 不适用 | **VS Code** |
| **固件二次开发** | ⚠️ 部分支持 | ✅ 完全支持 | **CCS IDE** |
| **雷达配置和调试** | ❌ 不支持 | ✅ 完全支持 | **mmWave Studio** |

### 🎯 推荐的开发流程

```
阶段1: 评估和学习 (VS Code + Python)
  ├── 使用 Python 可视化工具
  ├── 运行 Demo 应用
  ├── 分析雷达数据
  └── 算法原型开发

阶段2: 固件定制 (CCS IDE)
  ├── 导入 Radar Toolbox 工程
  ├── 修改固件代码
  ├── 编译和烧录
  └── 硬件测试

阶段3: 产品化 (混合使用)
  ├── Python: 上位机软件
  ├── CCS: 固件优化
  └── mmWave Studio: 配置调优
```

---

## 📦 Radar Toolbox 目录结构

### 本地路径
```
知识库\硬件资料\雷达模组资料\
└── TI 60G 4T4R车规雷达模块资料\
    └── 雷达模块\
        └── RADAR-TOOLBOX\
            └── radar_toolbox_3_30_00_06\
```

### 目录内容

```
radar_toolbox_3_30_00_06/
├── .metadata/                    # Eclipse 工作空间元数据
├── applications/                 # 应用Demo
│   ├── automotive/              # 汽车应用
│   ├── industrial/              # 工业应用
│   ├── personal_electronics/    # 个人电子设备应用
│   └── applications_overview.html
├── getting_started/              # 入门文档 ⭐
│   ├── getting_started_overview.html
│   ├── Getting_Started_With_xWRL1432.html
│   ├── Getting_Started_With_xWRL6432.html
│   └── Getting_Started_With_xWRL6844.html  ← 你的芯片
├── hardware_docs/                # 硬件文档
├── software_docs/                # 软件文档
├── source/                       # 源代码 ⭐⭐⭐
│   ├── third_party/             # 第三方库
│   └── ti/                      # TI 源代码
│       ├── alg/                 # 算法库
│       ├── common/              # 通用代码
│       ├── custom_sdk_files/    # 自定义SDK文件
│       ├── examples/            # 示例代码
│       └── utils/               # 工具代码
├── tests_and_experiments/        # 测试和实验
├── toolbox_docs/                 # Toolbox 文档
└── tools/                        # 工具集
```

---

## 🔧 开发环境配置

### 方案 A: VS Code 开发 (推荐用于算法和上位机)

#### 1. Python 环境配置

```powershell
# 创建虚拟环境
cd "D:\BaiduSyncdisk\0.0 Qsync-HN\0google云盘\Benson@Wisefido\7.项目资料\Ti雷达项目"
python -m venv .venv

# 激活虚拟环境
.\.venv\Scripts\Activate.ps1

# 安装依赖
pip install numpy matplotlib pyserial
pip install pyqt5  # 如果使用 GUI 工具
```

#### 2. VS Code 插件

**必装插件**:
- Python (Microsoft)
- Pylance (Microsoft)
- Jupyter (Microsoft)

**推荐插件**:
- C/C++ (Microsoft) - 查看固件代码
- Remote Development (Microsoft) - 如果需要远程开发

#### 3. 可以做什么？

**✅ 数据分析和可视化**
- 读取雷达数据文件
- 使用 Python 进行信号处理
- 绘制点云、热图、轨迹
- 算法原型开发

**✅ 上位机软件开发**
- 使用 Python/PyQt5 开发 GUI
- 串口通信控制雷达
- 数据实时显示
- 配置文件管理

**✅ 示例代码学习**
- 查看 `source/ti/examples/` 中的代码
- 理解算法实现
- 移植到 Python 原型

**❌ 不能做**
- 无法编译固件
- 无法烧录固件到芯片
- 无法使用 TI 专用调试工具

---

### 方案 B: CCS IDE 开发 (固件开发必需)

#### 1. 导入 Radar Toolbox 工程

**步骤**:
1. 打开 CCS Theia (已安装在 `C:\ti\ccs2040\`)
2. File → Import → General → Existing Projects into Workspace
3. 选择路径:
   ```
   D:\BaiduSyncdisk\0.0 Qsync-HN\0google云盘\Benson@Wisefido\7.项目资料\Ti雷达项目\知识库\硬件资料\雷达模组资料\TI 60G 4T4R车规雷达模块资料\雷达模块\RADAR-TOOLBOX\radar_toolbox_3_30_00_06
   ```
4. 选择要导入的项目(通常在 `source/ti/examples/`)

#### 2. 可以做什么？

**✅ 固件开发**
- 修改雷达信号处理算法
- 自定义数据输出格式
- 优化 DSP 性能
- 添加新功能

**✅ 编译和调试**
- 完整的编译工具链
- 硬件调试器支持
- 性能分析工具
- Flash 烧录

**✅ 项目管理**
- 版本控制集成
- 多项目管理
- 依赖管理

---

### 方案 C: 混合开发 (推荐 ⭐⭐⭐)

```
┌─────────────────────────────────────────┐
│  阶段1: VS Code (Python)                │
│  ├── 学习 Demo                          │
│  ├── 数据分析                           │
│  ├── 算法原型                           │
│  └── 验证可行性                         │
└─────────────────────────────────────────┘
              ↓
┌─────────────────────────────────────────┐
│  阶段2: CCS IDE (固件)                  │
│  ├── 导入 Toolbox 工程                  │
│  ├── 修改 C 代码                        │
│  ├── 编译固件                           │
│  └── 烧录到芯片                         │
└─────────────────────────────────────────┘
              ↓
┌─────────────────────────────────────────┐
│  阶段3: mmWave Studio (配置)            │
│  ├── GUI 参数配置                       │
│  ├── 数据采集                           │
│  └── 性能测试                           │
└─────────────────────────────────────────┘
              ↓
┌─────────────────────────────────────────┐
│  阶段4: VS Code (上位机)                │
│  ├── Python GUI 应用                    │
│  ├── 实时数据可视化                     │
│  └── 产品化应用                         │
└─────────────────────────────────────────┘
```

---

## 💡 实践建议

### 初学者路线 (全程 VS Code)

**Week 1-2: 学习和评估**
```powershell
# 1. 查看入门文档
Start "D:\...\radar_toolbox_3_30_00_06\getting_started\Getting_Started_With_xWRL6844.html"

# 2. 运行 Demo (使用预编译固件)
# 连接硬件
# 使用 mmWave Studio 运行 out-of-box demo

# 3. 分析数据 (Python)
cd "项目根目录"
python analyze_radar_data.py
```

**不需要 CCS**：前期只是学习和评估，使用预编译的 Demo 固件即可

---

### 进阶开发 (VS Code + CCS)

**Week 3-4: 固件定制**
```
1. 在 VS Code 中研究源代码 (source/ti/)
   - 理解算法逻辑
   - 确定修改点

2. 在 CCS IDE 中修改固件
   - 导入工程
   - 修改代码
   - 编译烧录

3. 在 VS Code 中验证
   - Python 读取新数据
   - 分析效果
```

---

## 🔌 VS Code 的具体能力

### ✅ 完全支持的工作

#### 1. Python 开发
```python
# 示例: 读取雷达数据
import numpy as np
import matplotlib.pyplot as plt

# 读取点云数据
data = np.loadtxt('radar_points.csv', delimiter=',')

# 可视化
plt.scatter(data[:, 0], data[:, 1])
plt.xlabel('X (m)')
plt.ylabel('Y (m)')
plt.title('Radar Point Cloud')
plt.show()
```

#### 2. 代码查看和学习
```
在 VS Code 中打开:
source/ti/examples/
├── occupancy_detection/
├── people_counting/
├── traffic_monitoring/
└── ...

功能:
- 语法高亮
- 函数跳转
- 代码搜索
- Git 集成
```

#### 3. 数据处理
```python
# 信号处理
from scipy import signal
import numpy as np

# FFT 分析
fft_result = np.fft.fft(radar_signal)
```

### ⚠️ 部分支持的工作

#### 1. C 代码查看 (只读)
```c
// 可以在 VS Code 查看固件代码
// 但不能编译
void radarDataProc(void) {
    // 算法实现
}
```

- ✅ 语法高亮
- ✅ 代码导航
- ❌ 无法编译
- ❌ 无法调试

#### 2. 配置文件编辑
```lua
-- 可以编辑 .cfg 配置文件
-- 但不能验证语法
profileCfg 0 60 7 7 60.25 0 0
```

### ❌ 不支持的工作

1. **固件编译**
   - 需要 TI ARM 编译器
   - 需要 CCS 构建系统
   - VS Code 无法直接编译

2. **硬件烧录**
   - 需要 Uniflash 或 CCS
   - 需要 XDS 调试器驱动

3. **实时调试**
   - 需要 CCS + JTAG
   - VS Code 无硬件调试功能

---

## 🎯 最佳实践

### 工作流程示例

#### 场景 1: 只做算法研究 (纯 VS Code)
```
目标: 研究雷达算法,不修改固件

工具:
- VS Code + Python
- mmWave Studio (运行 Demo)

流程:
1. 使用 mmWave Studio 采集数据
2. 在 VS Code 用 Python 分析
3. 开发算法原型
4. 验证效果

✅ 不需要 CCS!
```

#### 场景 2: 固件定制开发 (VS Code + CCS)
```
目标: 修改雷达固件,添加自定义功能

工具:
- VS Code (查看代码, Python 验证)
- CCS IDE (编译固件)
- mmWave Studio (测试)

流程:
1. VS Code: 查看 source/ti/ 源代码
2. CCS: 导入工程并修改
3. CCS: 编译并烧录
4. mmWave Studio: 测试
5. VS Code: Python 分析结果

⚠️ 必须使用 CCS!
```

#### 场景 3: 产品化应用 (VS Code)
```
目标: 开发上位机软件

工具:
- VS Code + Python/PyQt5

流程:
1. 使用固定的雷达固件
2. 开发 GUI 应用
3. 串口通信
4. 数据可视化
5. 打包发布

✅ 不需要 CCS!
```

---

## 📝 总结对比

### IDE 功能对比

| 功能 | VS Code | CCS IDE | 备注 |
|------|---------|---------|------|
| **Python 开发** | ⭐⭐⭐⭐⭐ | ❌ | VS Code 完胜 |
| **查看 C 代码** | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | VS Code 可查看 |
| **编译固件** | ❌ | ⭐⭐⭐⭐⭐ | 必须 CCS |
| **烧录固件** | ❌ | ⭐⭐⭐⭐⭐ | 必须 CCS |
| **硬件调试** | ❌ | ⭐⭐⭐⭐⭐ | 必须 CCS |
| **数据分析** | ⭐⭐⭐⭐⭐ | ⭐ | VS Code 完胜 |
| **GUI 开发** | ⭐⭐⭐⭐⭐ | ❌ | VS Code 完胜 |
| **Git 集成** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | VS Code 更好 |
| **AI 辅助** | ⭐⭐⭐⭐ | ⭐ | VS Code 有 Copilot |

### 建议配置

**方案 1: 纯软件开发者 (无固件修改)**
```
主力IDE: VS Code
工具: Python, mmWave Studio
成本: 免费
```

**方案 2: 固件开发者 (需修改底层)**
```
主力IDE: CCS IDE
辅助IDE: VS Code (Python 分析)
工具: mmWave Studio
成本: CCS 免费版
```

**方案 3: 全栈开发者 (推荐)**
```
双 IDE: VS Code + CCS IDE
- VS Code: 算法、上位机、数据分析
- CCS: 固件修改、编译、调试
工具: mmWave Studio
成本: 全免费
```

---

## 🚀 快速开始

### 立即可以在 VS Code 做的事

```powershell
# 1. 打开项目
cd "D:\BaiduSyncdisk\0.0 Qsync-HN\0google云盘\Benson@Wisefido\7.项目资料\Ti雷达项目"
code .

# 2. 查看 Radar Toolbox 源代码
code "知识库\硬件资料\雷达模组资料\TI 60G 4T4R车规雷达模块资料\雷达模块\RADAR-TOOLBOX\radar_toolbox_3_30_00_06\source\ti\examples"

# 3. 创建 Python 分析脚本
New-Item -ItemType File -Path "scripts\analyze_radar.py"
```

---

## ⚠️ 注意事项

1. **VS Code 的局限性**
   - 不能替代 CCS 编译固件
   - 不能替代 mmWave Studio 配置雷达
   - 只适合上层应用和算法开发

2. **CCS 的必要性**
   - 如果需要修改固件,必须使用 CCS
   - 如果只用 Demo,可以不用 CCS

3. **最佳组合**
   - VS Code: 日常开发、学习、分析
   - CCS: 固件定制
   - mmWave Studio: 配置和测试

---

## 📚 相关文档

- [TI-mmWave-完整资源导航.md](../../知识库/TI-mmWave-完整资源导航.md)
- [Getting Started with xWRL6844](../../知识库/硬件资料/雷达模组资料/TI 60G 4T4R车规雷达模块资料/雷达模块/RADAR-TOOLBOX/radar_toolbox_3_30_00_06/getting_started/Getting_Started_With_xWRL6844.html)

---

**结论**: 
- ✅ **可以用 VS Code**,但主要用于 Python 开发、数据分析、上位机软件
- ⚠️ **固件开发必须用 CCS**,这是 TI 工具链的要求
- 🎯 **推荐双 IDE 配合**,发挥各自优势
