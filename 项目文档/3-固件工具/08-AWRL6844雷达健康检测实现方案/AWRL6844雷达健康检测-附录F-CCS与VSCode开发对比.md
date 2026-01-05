# 🔧 AWRL6844固件开发：CCS vs VS Code 全维度对比

> **创建日期**: 2026-01-05
> **适用项目**: AWRL6844雷达健康检测
> **目的**: 帮助开发者选择合适的开发工具

---

## 📊 对比总览

```
┌────────────────────────────────────────────────────────────────────┐
│                    开发工具选择决策树                               │
├────────────────────────────────────────────────────────────────────┤
│                                                                    │
│  需要断点调试？ ──是──→ 使用 CCS                                   │
│       │                                                            │
│       否                                                           │
│       ↓                                                            │
│  熟悉命令行编译？ ──是──→ 可以用 VS Code + gmake                   │
│       │                                                            │
│       否                                                           │
│       ↓                                                            │
│  首次接触TI开发？ ──是──→ 建议先用 CCS（学习曲线低）               │
│       │                                                            │
│       否                                                           │
│       ↓                                                            │
│  根据个人偏好选择                                                   │
│                                                                    │
└────────────────────────────────────────────────────────────────────┘
```

---

## 🆚 核心对比表

| 维度 | CCS (Code Composer Studio) | VS Code + gmake |
|------|---------------------------|-----------------|
| **类型** | 完整IDE | 编辑器 + 命令行工具 |
| **TI官方支持** | ⭐⭐⭐⭐⭐ 官方IDE | ⭐⭐ 非官方但可行 |
| **启动速度** | 慢（30秒+） | 快（3秒） |
| **内存占用** | 高（1-2GB） | 低（300MB） |
| **断点调试** | ✅ 完整支持 | ❌ 不支持（需CCS调试） |
| **代码编辑** | ⭐⭐⭐ 一般 | ⭐⭐⭐⭐⭐ 优秀 |
| **智能提示** | ⭐⭐⭐ 基础 | ⭐⭐⭐⭐⭐ 强大（C/C++插件） |
| **工程配置** | 图形化向导 | 手动编写配置 |
| **编译速度** | 正常 | 正常（同样的编译器） |
| **学习成本** | ⭐⭐ 低（开箱即用） | ⭐⭐⭐⭐ 中高（需配置） |
| **多项目管理** | ⭐⭐⭐ 一般 | ⭐⭐⭐⭐⭐ 优秀 |
| **Git集成** | ⭐⭐ 基础 | ⭐⭐⭐⭐⭐ 优秀 |
| **扩展生态** | ⭐⭐ 有限 | ⭐⭐⭐⭐⭐ 丰富 |

---

## 📁 工具链依赖关系

```
┌─────────────────────────────────────────────────────────────────┐
│                        CCS安装包                                 │
│  C:\ti\ccs2040\                                                 │
│  ├── ccs\                                                       │
│  │   ├── eclipse\ccstudio.exe          # CCS IDE程序            │
│  │   ├── utils\bin\gmake.exe           # ⭐ GNU Make工具        │
│  │   └── tools\compiler\                                        │
│  │       ├── ti-cgt-armllvm_4.0.4.LTS\ # ⭐ R5F编译器(ARM)      │
│  │       └── ti-cgt-c6000_8.5.0.LTS\   # ⭐ DSP编译器(C66x)     │
│  └── ...                                                        │
└─────────────────────────────────────────────────────────────────┘
                              ↓
              ┌───────────────┴───────────────┐
              ↓                               ↓
    ┌─────────────────┐             ┌─────────────────┐
    │    使用CCS      │             │  使用VS Code    │
    │                 │             │                 │
    │ 打开ccstudio.exe │             │ 只调用:         │
    │ 图形化操作       │             │ - gmake.exe     │
    │                 │             │ - tiarmclang    │
    │                 │             │ - ti-cgt-c6000  │
    └─────────────────┘             └─────────────────┘
```

**关键理解**：
- ✅ CCS安装后，编译器和gmake作为**独立工具**存在
- ✅ VS Code只需调用这些工具，**不需要运行CCS程序**
- ✅ 两种方式使用**完全相同的编译器**，生成**完全相同的固件**

---

## 🛠️ 方案A：使用CCS开发

### A.1 完整开发流程

```
步骤1: 启动CCS
       C:\ti\ccs2040\ccs\eclipse\ccstudio.exe
              ↓
步骤2: 导入工程
       File → Import → CCS Projects
       选择: AWRL6844_InCabin_Demos\src\system
              ↓
步骤3: 配置路径变量（首次）
       Window → Preferences → Build Variables
       设置SDK、Toolbox路径
              ↓
步骤4: 编写/修改代码
       在CCS编辑器中修改源文件
              ↓
步骤5: 编译
       右键工程 → Build Project
       或 Ctrl+B
              ↓
步骤6: 生成固件
       Release\demo_in_cabin_sensing_6844_system.release.appimage
              ↓
步骤7: 烧录（可选在CCS中）
       Run → Debug 或使用外部烧录工具
              ↓
步骤8: 调试
       设置断点 → F5运行 → 查看变量
```

### A.2 CCS工程结构

```
AWRL6844_InCabin_Demos/src/
├── system/                              # 系统工程（导入这个）
│   ├── .project                         # CCS工程文件
│   ├── .ccsproject                      # CCS配置
│   └── makefile_system_ccs_bootimage_gen
├── mss/                                 # MSS子工程（自动关联）
│   └── xwrL684x-evm/r5fss0-0_freertos/ti-arm-clang/
│       ├── .project
│       └── makefile
└── dss/                                 # DSS子工程（自动关联）
    └── xwrL684x-evm/c66ss0_freertos/ti-c6000/
        ├── .project
        └── makefile
```

### A.3 CCS优势场景

| 场景 | 为什么CCS更好 |
|------|--------------|
| **断点调试** | 唯一支持XDS110在线调试的方式 |
| **查看寄存器** | 可视化查看R5F/C66x寄存器 |
| **内存监视** | 实时查看RAM/Flash内容 |
| **首次开发** | 工程配置已预设好，开箱即用 |
| **SysConfig配置** | 图形化配置外设引脚 |

---

## 🛠️ 方案B：使用VS Code + gmake开发

### B.1 环境配置（一次性）

#### B.1.1 安装VS Code扩展

```
必装扩展：
├── C/C++ (Microsoft)              # 代码智能提示
├── C/C++ Extension Pack           # 扩展包
└── Makefile Tools (可选)          # Makefile支持

推荐扩展：
├── GitLens                        # Git增强
├── Todo Tree                      # TODO管理
└── Error Lens                     # 错误高亮
```

#### B.1.2 配置环境变量

**方式1: 系统环境变量（推荐）**

```powershell
# 添加到系统PATH
$env:Path += ";C:\ti\ccs2040\ccs\utils\bin"
$env:Path += ";C:\ti\ccs2040\ccs\tools\compiler\ti-cgt-armllvm_4.0.4.LTS\bin"
$env:Path += ";C:\ti\ccs2040\ccs\tools\compiler\ti-cgt-c6000_8.5.0.LTS\bin"

# 设置SDK路径
[System.Environment]::SetEnvironmentVariable("MMWAVE_L_SDK_INSTALL_DIR", "C:\ti\MMWAVE_L_SDK_06_01_00_01", "User")
[System.Environment]::SetEnvironmentVariable("RADAR_TOOLBOX_PATH", "C:\ti\radar_toolbox_3_30_00_06", "User")
[System.Environment]::SetEnvironmentVariable("CGT_TI_ARM_CLANG_PATH", "C:\ti\ccs2040\ccs\tools\compiler\ti-cgt-armllvm_4.0.4.LTS", "User")
[System.Environment]::SetEnvironmentVariable("CGT_TI_C6000_PATH", "C:\ti\ccs2040\ccs\tools\compiler\ti-cgt-c6000_8.5.0.LTS", "User")
```

**方式2: VS Code工作区配置**

创建 `.vscode/settings.json`：

```json
{
    "terminal.integrated.env.windows": {
        "MMWAVE_L_SDK_INSTALL_DIR": "C:/ti/MMWAVE_L_SDK_06_01_00_01",
        "RADAR_TOOLBOX_PATH": "C:/ti/radar_toolbox_3_30_00_06",
        "CGT_TI_ARM_CLANG_PATH": "C:/ti/ccs2040/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS",
        "CGT_TI_C6000_PATH": "C:/ti/ccs2040/ccs/tools/compiler/ti-cgt-c6000_8.5.0.LTS",
        "PATH": "${env:PATH};C:/ti/ccs2040/ccs/utils/bin"
    }
}
```

#### B.1.3 配置编译任务

创建 `.vscode/tasks.json`：

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build MSS (R5F)",
            "type": "shell",
            "command": "gmake",
            "args": [
                "-j4",
                "PROFILE=release"
            ],
            "options": {
                "cwd": "C:/ti/radar_toolbox_3_30_00_06/source/ti/examples/Automotive_InCabin_Security_and_Safety/AWRL6844_InCabin_Demos/src/mss/xwrL684x-evm/r5fss0-0_freertos/ti-arm-clang"
            },
            "group": "build",
            "problemMatcher": ["$gcc"]
        },
        {
            "label": "Build DSS (C66x)",
            "type": "shell",
            "command": "gmake",
            "args": [
                "-j4",
                "PROFILE=release"
            ],
            "options": {
                "cwd": "C:/ti/radar_toolbox_3_30_00_06/source/ti/examples/Automotive_InCabin_Security_and_Safety/AWRL6844_InCabin_Demos/src/dss/xwrL684x-evm/c66ss0_freertos/ti-c6000"
            },
            "group": "build",
            "problemMatcher": ["$gcc"]
        },
        {
            "label": "Build All & Generate AppImage",
            "type": "shell",
            "command": "gmake",
            "args": [
                "-f", "makefile_system_ccs_bootimage_gen",
                "PROFILE=release"
            ],
            "options": {
                "cwd": "C:/ti/radar_toolbox_3_30_00_06/source/ti/examples/Automotive_InCabin_Security_and_Safety/AWRL6844_InCabin_Demos/src/system"
            },
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": ["$gcc"]
        },
        {
            "label": "Clean All",
            "type": "shell",
            "command": "gmake",
            "args": ["clean"],
            "options": {
                "cwd": "C:/ti/radar_toolbox_3_30_00_06/source/ti/examples/Automotive_InCabin_Security_and_Safety/AWRL6844_InCabin_Demos/src/mss/xwrL684x-evm/r5fss0-0_freertos/ti-arm-clang"
            },
            "group": "build",
            "problemMatcher": []
        }
    ]
}
```

#### B.1.4 配置C/C++智能提示

创建 `.vscode/c_cpp_properties.json`：

```json
{
    "configurations": [
        {
            "name": "AWRL6844",
            "includePath": [
                "${workspaceFolder}/**",
                "C:/ti/MMWAVE_L_SDK_06_01_00_01/source/**",
                "C:/ti/MMWAVE_L_SDK_06_01_00_01/kernel/freertos/FreeRTOS-Kernel/include/**",
                "C:/ti/radar_toolbox_3_30_00_06/source/**",
                "C:/ti/ccs2040/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS/include/**"
            ],
            "defines": [
                "SOC_XWRL6844",
                "SUBSYS_MSS",
                "_DEBUG"
            ],
            "compilerPath": "C:/ti/ccs2040/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS/bin/tiarmclang.exe",
            "cStandard": "c17",
            "cppStandard": "c++17",
            "intelliSenseMode": "gcc-arm"
        }
    ],
    "version": 4
}
```

### B.2 完整开发流程

```
步骤1: 打开VS Code
       打开SDK工程目录或自己的项目目录
              ↓
步骤2: 配置环境（首次）
       创建.vscode/settings.json、tasks.json、c_cpp_properties.json
              ↓
步骤3: 编写/修改代码
       在VS Code编辑器中修改源文件
       享受智能提示、代码跳转等功能
              ↓
步骤4: 编译
       Ctrl+Shift+B 或 Terminal → Run Build Task
       选择 "Build All & Generate AppImage"
              ↓
步骤5: 查看编译输出
       Terminal窗口显示编译进度和错误
              ↓
步骤6: 生成固件
       同样生成 .appimage 文件
              ↓
步骤7: 烧录
       使用SDK Visualizer或flash_tool.py（外部工具）
              ↓
步骤8: 调试（需要时）
       如需断点调试，切换到CCS进行
```

### B.3 VS Code优势场景

| 场景 | 为什么VS Code更好 |
|------|------------------|
| **代码编辑** | 更强大的编辑器功能、多光标、代码片段 |
| **Git操作** | 内置Git支持、GitLens扩展 |
| **多语言项目** | 同时编辑C/Python/Markdown等 |
| **轻量快速** | 启动快、占用资源少 |
| **远程开发** | 支持SSH远程开发 |
| **扩展生态** | 丰富的扩展可用 |

---

## 📋 开发流程对比

### 日常开发流程对比

| 步骤 | CCS | VS Code |
|------|-----|---------|
| **启动** | 30秒+ | 3秒 |
| **打开文件** | 工程树双击 | Ctrl+P快速打开 |
| **代码跳转** | F3 | F12（更快） |
| **查找引用** | Ctrl+Shift+G | Shift+F12 |
| **全局搜索** | Ctrl+H | Ctrl+Shift+F（更强） |
| **编译** | Ctrl+B / 右键Build | Ctrl+Shift+B |
| **查看错误** | Problems视图 | Problems视图 |
| **Git提交** | 需要插件或外部工具 | 内置完整支持 |

### 编译命令对比

**CCS编译**：
```
右键工程 → Build Project
或菜单 Project → Build All
```

**VS Code编译**：
```powershell
# 终端中直接执行
cd C:\ti\radar_toolbox_3_30_00_06\source\ti\examples\Automotive_InCabin_Security_and_Safety\AWRL6844_InCabin_Demos\src\mss\xwrL684x-evm\r5fss0-0_freertos\ti-arm-clang

gmake -j4 PROFILE=release

# 或使用配置好的Task
Ctrl+Shift+B → 选择任务
```

---

## 🔧 混合使用策略（推荐）

### 最佳实践：结合两者优势

```
┌─────────────────────────────────────────────────────────────────┐
│                     推荐工作流程                                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  日常开发（90%时间）                                             │
│  ─────────────────                                              │
│  使用 VS Code:                                                  │
│  ├── 编写代码（智能提示强）                                      │
│  ├── 代码审查（Git集成好）                                       │
│  ├── 编译构建（gmake快捷）                                       │
│  └── 烧录测试（外部工具）                                        │
│                                                                 │
│  调试阶段（10%时间）                                             │
│  ─────────────────                                              │
│  切换到 CCS:                                                    │
│  ├── 设置断点                                                   │
│  ├── 单步执行                                                   │
│  ├── 查看变量/寄存器                                             │
│  └── 内存分析                                                   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 文件共享

**两个IDE可以同时打开同一工程目录**：
- VS Code编辑的文件，CCS可以立即看到变化
- CCS编译的结果，VS Code终端可以查看
- 不冲突，不需要切换工作区

---

## ⚠️ 注意事项

### CCS注意事项

| 问题 | 解决方案 |
|------|---------|
| 工程路径太长导致编译失败 | 将工程放在较短路径如`C:\ti\projects\` |
| 路径变量未生效 | Clean后重新编译 |
| 编译器版本不匹配 | 在工程属性中指定正确版本 |

### VS Code注意事项

| 问题 | 解决方案 |
|------|---------|
| gmake找不到 | 确认PATH包含`C:\ti\ccs2040\ccs\utils\bin` |
| 编译器找不到 | 检查环境变量`CGT_TI_ARM_CLANG_PATH` |
| 智能提示不工作 | 检查`c_cpp_properties.json`的includePath |
| 中文路径问题 | 避免在路径中使用中文 |

---

## 📊 总结：如何选择？

### 快速决策表

| 如果你... | 推荐使用 |
|----------|---------|
| 首次接触TI嵌入式开发 | **CCS**（学习曲线低） |
| 需要频繁断点调试 | **CCS**（唯一支持） |
| 主要写代码，偶尔调试 | **VS Code + 偶尔CCS** |
| 熟悉Makefile/命令行 | **VS Code** |
| 同时开发多语言项目 | **VS Code** |
| 追求编辑器体验 | **VS Code** |
| 需要最稳定的官方支持 | **CCS** |

### 本项目建议

```
AWRL6844雷达健康检测项目 开发建议：
─────────────────────────────────────

第1-2章（环境搭建+验证）：
  └─ 不需要编译，使用预编译固件
  └─ 任选工具都可以

第3-10章（代码开发）：
  └─ 推荐：VS Code编写代码 + gmake编译
  └─ 调试时：切换到CCS进行断点调试

烧录测试：
  └─ 使用SDK Visualizer（与IDE无关）
```

---

## 📚 相关资源

### VS Code配置模板位置

> 本项目已提供配置模板，可直接复制使用

```
project-code/AWRL6844_HealthDetect/.vscode/（待创建）
├── settings.json
├── tasks.json
├── c_cpp_properties.json
└── launch.json
```

### 参考文档

| 文档 | 位置 |
|------|------|
| CCS用户指南 | `C:\ti\ccs2040\ccs\doc\` |
| SDK编译说明 | `C:\ti\MMWAVE_L_SDK_06_01_00_01\docs\` |
| VS Code C++配置 | https://code.visualstudio.com/docs/cpp |

---

> **最终建议**：两个工具都安装好，根据当前任务灵活切换，发挥各自优势！
