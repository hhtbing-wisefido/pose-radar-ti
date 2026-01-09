# 📚 Part 15: CCS System项目自动依赖编译机制

> **创建日期**: 2026-01-09  
> **适用工具**: Code Composer Studio (CCS) 20.x  
> **SDK版本**: MMWAVE_L_SDK 06.01.00.01  
> **文档状态**: ✅ 完整

---

## 🎯 核心问题

在TI多核雷达项目（如AWRL6844）中，开发者经常遇到这样的问题：

> ❓ **为什么只编译System项目会失败？**  
> ❓ **必须按MSS→DSS→System的顺序手动编译吗？**  
> ❓ **如何让CCS自动处理编译依赖？**

---

## 📚 官方文档说明

### 来源：TI MMWAVE-L-SDK BUILD_GUIDE.html

**官方路径**: `C:\ti\MMWAVE_L_SDK_06_01_00_01\docs\api_guide_xwrL684x\BUILD_GUIDE.html`

#### 关键说明1：System项目自动导入依赖

> System projects in CCS contain references to individual core projects:
> 1. Import the system project:
>    - Example: `uart_echo_xwrL684x-evm_system_freertos`
> 2. **This automatically imports referenced core projects:**
>    - R5F project: `uart_echo_xwrL684x-evm_r5fss0-0_freertos_ti-arm-clang`
>    - C66 project: `uart_echo_xwrL684x-evm_c66ss0_freertos_ti-c6000-clang`

#### 关键说明2：编译System自动编译子项目

> Building the system project:
> - Right-click on the system project
> - Select "Build Project"
> - **This automatically builds all referenced core projects**
> - Individual .out files are created in their respective projects
> - **The combined .appimage is created in the system project**

#### 关键说明3：故障排除

> **Problem**: System project doesn't build all cores  
> **Solution**:
> - Make sure all referenced projects are imported
> - Check project dependencies in system project properties
> - **Try building individual core projects first, then the system project**

---

## 🔧 自动依赖编译的工作机制

### 1. projectspec中的`<import>`标签

System项目的`.projectspec`文件必须包含`<import>`标签来声明依赖：

```xml
<?xml version="1.0" encoding="UTF-8"?>
<projectSpec>
    <!-- 这两行是关键！告诉CCS需要导入哪些子项目 -->
    <import spec="../mss/xwrL684x-evm/r5fss0-0_freertos/ti-arm-clang/xxx_mss.projectspec"/>
    <import spec="../dss/xwrL684x-evm/c66ss0_freertos/ti-c6000/xxx_dss.projectspec"/>
    
    <project
        name="xxx_system"
        outputType="system"
        ...
    >
        ...
    </project>
</projectSpec>
```

### 2. system.xml中的项目引用

`system.xml`文件定义了多核项目的组成：

```xml
<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<system>
    <!-- MSS项目引用 -->
    <project configuration="@match" id="project_0" name="xxx_mss">
    </project>
    <core id="Cortex_R5_0" project="project_0"/>
    
    <!-- DSS项目引用 -->
    <project configuration="@match" id="project_1" name="xxx_dss">
    </project>
    <core id="C66xx_DSP" project="project_1"/>
    
    <postBuildSteps>
        <step command="$(MAKE) -f makefile_system_ccs_bootimage_gen ..."/>
    </postBuildSteps>
</system>
```

### 3. CCS自动编译流程

当正确配置后，CCS按以下顺序自动处理：

```
┌──────────────────────────────────────────────────────────────┐
│  用户操作：右键System项目 → Build Project                    │
└────────────────────────┬─────────────────────────────────────┘
                         ↓
┌──────────────────────────────────────────────────────────────┐
│  CCS自动步骤1：检测system.xml中的项目依赖                     │
│  → 发现需要 xxx_mss 和 xxx_dss                                │
└────────────────────────┬─────────────────────────────────────┘
                         ↓
┌──────────────────────────────────────────────────────────────┐
│  CCS自动步骤2：编译 xxx_mss 项目                              │
│  → 生成 xxx_mss.out → 生成 xxx_mss.rig                        │
└────────────────────────┬─────────────────────────────────────┘
                         ↓
┌──────────────────────────────────────────────────────────────┐
│  CCS自动步骤3：编译 xxx_dss 项目                              │
│  → 生成 xxx_dss.out → 生成 xxx_dss.rig                        │
└────────────────────────┬─────────────────────────────────────┘
                         ↓
┌──────────────────────────────────────────────────────────────┐
│  CCS自动步骤4：执行System的post-build                        │
│  → 复制MSS/DSS的.rig文件                                      │
│  → 合并生成 xxx_system.appimage                               │
└──────────────────────────────────────────────────────────────┘
```

---

## ⚠️ 常见问题：为什么自动编译没生效？

### 问题1：分别导入了3个项目

**错误做法**：
```
File → Import → CCS Projects
  选择 mss.projectspec → Finish
File → Import → CCS Projects  
  选择 dss.projectspec → Finish
File → Import → CCS Projects
  选择 system.projectspec → Finish
```

**问题**：这样导入时，CCS不会自动识别项目间的依赖关系！

**正确做法**：
```
File → Import → CCS Projects
  只选择 system.projectspec → Finish
  (CCS自动解析<import>标签，自动导入MSS和DSS)
```

### 问题2：项目名称不匹配

`system.xml`中的项目名称必须与实际项目名称**完全一致**：

```xml
<!-- system.xml -->
<project configuration="@match" id="project_0" name="health_detect_6844_mss">
                                                      ^^^^^^^^^^^^^^^^^^^^^^^^
                                                      必须与CCS中的项目名一致
```

### 问题3：相对路径错误

`<import spec="...">`中的路径必须是相对于system.projectspec的正确路径：

```
src/
├── mss/
│   └── xwrL684x-evm/r5fss0-0_freertos/ti-arm-clang/
│       └── xxx_mss.projectspec
├── dss/
│   └── xwrL684x-evm/c66ss0_freertos/ti-c6000/
│       └── xxx_dss.projectspec
└── system/
    ├── xxx_system.projectspec  ← 这个文件中
    │   <import spec="../mss/xwrL684x-evm/r5fss0-0_freertos/ti-arm-clang/xxx_mss.projectspec"/>
    │                  ↑↑↑↑↑ 必须是正确的相对路径
```

---

## ✅ 正确的项目导入流程

### 步骤1：清理现有项目（如有）

1. 在CCS Project Explorer中
2. 右键每个项目 → Delete
3. **不要勾选** "Delete project contents on disk"

### 步骤2：只导入System项目

```
File → Import → CCS Projects
Browse to: <项目路径>/src/system/
只选择: xxx_system.projectspec
点击 Finish
```

CCS会自动：
- ✅ 解析 `<import>` 标签
- ✅ 自动导入 MSS 项目
- ✅ 自动导入 DSS 项目
- ✅ 设置项目间依赖关系

### 步骤3：验证项目依赖

1. 右键 System 项目 → Properties
2. 展开 Build → Dependencies
3. 确认 MSS 和 DSS 项目被列为依赖

### 步骤4：编译

```
右键 System 项目 → Build Project
```

CCS会自动按正确顺序编译所有项目。

---

## 📋 检查清单

使用以下清单验证项目配置是否正确：

| 检查项 | 要求 | 状态 |
|-------|------|------|
| system.projectspec有`<import>`标签 | 引用MSS和DSS的projectspec | □ |
| `<import>`路径正确 | 相对路径可以正确解析 | □ |
| system.xml中的项目名称匹配 | 与MSS/DSS项目名一致 | □ |
| 只从system.projectspec导入 | 不要分别导入3个项目 | □ |
| Project Explorer显示3个项目 | MSS、DSS、System都存在 | □ |
| System项目的Dependencies设置正确 | 列出MSS和DSS依赖 | □ |

---

## 📚 参考文档

1. **TI官方文档**: `C:\ti\MMWAVE_L_SDK_06_01_00_01\docs\api_guide_xwrL684x\BUILD_GUIDE.html`
2. **CCS项目使用指南**: `C:\ti\MMWAVE_L_SDK_06_01_00_01\docs\api_guide_xwrL684x\CCS_PROJECTS_PAGE.html`
3. **参考项目**: `C:\ti\radar_toolbox_3_30_00_06\source\ti\examples\Automotive_InCabin_Security_and_Safety\AWRL6844_InCabin_Demos\src\system\demo_in_cabin_sensing_6844_system.projectspec`

---

## 🔗 相关文档

- [Part1-SDK基础概念与三目录详解](Part1-SDK基础概念与三目录详解.md)
- [Part3-SDK与固件关系及工作流程](Part3-SDK与固件关系及工作流程.md)
- [Part10-MMWAVE_L_SDK深度解析](Part10-MMWAVE_L_SDK深度解析.md)
