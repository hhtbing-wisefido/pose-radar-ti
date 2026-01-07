# 🔬 AWRL6844雷达健康检测-05-第2章-标准Demo验证

> **目标**: 使用SDK参考工程(mmw_demo_SDK_reference)验证开发环境正常，建立可靠基线
> **创建日期**: 2026-01-07
> **状态**: ✅ 已生成
> **前置文档**: `AWRL6844雷达健康检测-04-第1章-环境搭建.md`

---

> ⚠️ **重要说明**：
> - **使用工程**：`project-code\mmw_demo_SDK_reference`（已复制好）
> - **作用**：仅用于验证环境，**不做任何修改**
> - **参考基准**：SDK原始状态的工程，用于对比和学习
> - **新项目**：第3章规划后才会创建 `AWRL6844_HealthDetect`

---

## 📊 本章概览

```
┌─────────────────────────────────────────────────────────────────┐
│                    第2章 标准Demo验证 路线图                      │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  2.1 复制工程    2.2 CCS导入      2.3 编译       2.4 烧录       │
│  ─────────────  ─────────────    ────────────   ────────────    │
│  从SDK复制       导入到CCS        编译检查       烧录固件       │
│  mmw_demo工程    配置路径         生成镜像       发送配置       │
│  到项目目录      不重命名         验证无错       启动雷达       │
│      ↓               ↓               ↓              ↓           │
│  [工程复制]      [工程导入]      [固件就绪]      [验证通过]      │
│                                                                 │
│                     2.5 配置加载    2.6 数据验证                 │
│                    ────────────    ────────────                 │
│                    发送.cfg文件    检查点云输出                 │
│                    执行sensorStart  可视化验证                  │
│                    雷达启动        验证检测功能                 │
│                        ↓               ↓                        │
│                    [雷达运行]      [功能正常]                    │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

> ⚠️ **前置条件**：必须完成第1章的环境搭建！

---

## 📋 目录

- [2.1 复制SDK标准Demo工程](#21-复制sdk标准demo工程)
- [2.2 在CCS中导入工程](#22-在ccs中导入工程)
- [2.3 编译验证](#23-编译验证)
- [2.4 固件烧录](#24-固件烧录)
- [2.5 雷达配置加载](#25-雷达配置加载)
- [2.6 数据输出验证](#26-数据输出验证)
- [2.7 常见问题排查](#27-常见问题排查)
- [2.8 完成检查清单](#28-完成检查清单)

---

## 2.1 复制SDK标准Demo工程

> 🎯 **目标**：从SDK复制mmw_demo工程到项目目录，作为验证基准
> 📍 **目标位置**：`D:\7.project\TI_Radar_Project\project-code\mmw_demo_SDK_reference\`

### 2.1.1 源工程位置

**SDK标准mmw_demo工程**：

```
C:\ti\MMWAVE_L_SDK_06_01_00_01\examples\mmw_demo\mmwave_demo\
```

**完整目录结构**：

```
mmwave_demo/
├── prebuilt_binaries/                    # 预编译固件
│   └── xwrL684x-evm/
│       └── mmwave_demo.release.appimage  # 可直接烧录的固件
│
├── profiles/                             # 雷达配置文件
│   ├── monitors.cfg                      # 监控配置
│   ├── profile_2T4R_bpm.cfg             # 2发4收BPM配置
│   ├── profile_3T4R_tdm.cfg             # 3发4收TDM配置
│   └── profile_4T4R_tdm.cfg             # 4发4收TDM配置
│
├── source/                               # 源代码目录
│   ├── mmwave_demo.c                     # 主程序实现
│   ├── mmwave_demo.h                     # 主程序头文件
│   ├── mmw_cli.c                         # CLI命令实现
│   ├── mmw_cli.h                         # CLI命令头文件
│   ├── mmw_res.h                         # 资源定义
│   │
│   ├── calibrations/                     # 校准相关
│   │   ├── factory_cal.c/.h             # 工厂校准
│   │   ├── mmw_flash_cal.c/.h           # Flash校准
│   │   └── range_phase_bias_measurement.c/.h  # 距离相位偏差测量
│   │
│   ├── dpc/                              # Data Path Chain (DPC)
│   │   ├── dpc.c                         # DPC实现
│   │   └── dpc.h                         # DPC头文件
│   │
│   ├── lvds_streaming/                   # LVDS数据流
│   │   ├── mmw_lvds_stream.c
│   │   └── mmw_lvds_stream.h
│   │
│   ├── mmwave_control/                   # 毫米波控制
│   │   ├── interrupts.c/.h              # 中断处理
│   │   ├── mmwave_control_config.c      # 毫米波配置
│   │   └── monitors.c/.h                # 监控功能
│   │
│   ├── power_management/                 # 电源管理
│   │   ├── power_management.c
│   │   └── power_management.h
│   │
│   └── test/                             # 测试相关
│       ├── ADC_testbuf.c                # ADC测试缓冲
│       └── ADC_testbuf.h
│
└── xwrL684x-evm/                         # 设备特定工程
    └── r5fss0-0_freertos/               # R5F FreeRTOS工程
        ├── example.syscfg               # SysConfig配置文件（硬件/引脚配置）
        ├── main.c                        # 主程序入口
        │
        └── ti-arm-clang/                # TI ARM编译器工程目录
            ├── example.projectspec       # CCS工程配置文件
            ├── linker.cmd                # 链接脚本（内存布局）
            ├── makefile                  # Make构建脚本
            ├── makefile_ccs_bootimage_gen  # 生成启动镜像的Make脚本
            ├── memory_hex.cmd            # 内存十六进制配置
            ├── mmwave_demo.release.out   # 编译输出（ELF格式）
            ├── mmwave_demo.release.map   # 内存映射文件
            ├── mmwave_demo.release.appimage  # 可烧录固件（和prebuilt中的相同）
            ├── syscfg_c.rov.xs          # ROV（Runtime Object Viewer）配置
            │
            └── config/                   # 配置文件目录
                ├── metaimage_cfg.debug.json      # Debug模式元镜像配置
                ├── metaimage_cfg.release.json    # Release模式元镜像配置
                ├── metaimage_cfg_hs.debug.json   # HS (High Security) Debug配置
                └── metaimage_cfg_hs.release.json # HS Release配置
```

---

### 2.1.2 复制步骤

#### 方法1：使用Windows资源管理器

```
步骤1: 打开SDK目录
        Windows资源管理器 → 导航到：
        C:\ti\MMWAVE_L_SDK_06_01_00_01\examples\mmw_demo\

步骤2: 复制mmwave_demo文件夹
        选中 mmwave_demo 文件夹 → Ctrl+C 复制

步骤3: 粘贴到项目目录
        导航到：D:\7.project\TI_Radar_Project\project-code\
        Ctrl+V 粘贴

步骤4: 重命名
        将粘贴的文件夹重命名为：mmw_demo_SDK_reference
```

#### 方法2：使用PowerShell命令（推荐）

```powershell
# 复制整个工程
Copy-Item -Path "C:\ti\MMWAVE_L_SDK_06_01_00_01\examples\mmw_demo\mmwave_demo" `
          -Destination "D:\7.project\TI_Radar_Project\project-code\mmw_demo_SDK_reference" `
          -Recurse -Force
```

> 💡 **推荐使用方法2**：PowerShell命令更快，且不会漏掉隐藏文件

---

### 2.1.3 验证复制结果

#### 检查主要目录

```powershell
# 检查主目录是否存在
Test-Path "D:\7.project\TI_Radar_Project\project-code\mmw_demo_SDK_reference"
# 应返回: True

# 检查4个主要子目录
Get-ChildItem "D:\7.project\TI_Radar_Project\project-code\mmw_demo_SDK_reference" | Select-Object Name
# 应返回:
# Name
# ----
# prebuilt_binaries
# profiles
# source
# xwrL684x-evm
```

#### 检查关键文件

```powershell
# 检查预编译固件
Test-Path "D:\7.project\TI_Radar_Project\project-code\mmw_demo_SDK_reference\prebuilt_binaries\xwrL684x-evm\mmwave_demo.release.appimage"
# 应返回: True

# 检查配置文件
Test-Path "D:\7.project\TI_Radar_Project\project-code\mmw_demo_SDK_reference\profiles\profile_4T4R_tdm.cfg"
# 应返回: True

# 检查主程序源码
Test-Path "D:\7.project\TI_Radar_Project\project-code\mmw_demo_SDK_reference\source\mmwave_demo.c"
# 应返回: True

# 检查CCS工程文件
Test-Path "D:\7.project\TI_Radar_Project\project-code\mmw_demo_SDK_reference\xwrL684x-evm\r5fss0-0_freertos\ti-arm-clang\example.projectspec"
# 应返回: True
```

#### 验证目录完整性

```powershell
# 统计文件总数（应该有30+个文件）
(Get-ChildItem "D:\7.project\TI_Radar_Project\project-code\mmw_demo_SDK_reference" -Recurse -File).Count
```

> ⚠️ **重要**：
> - 这个工程仅用于**验证环境**，不做任何修改
> - 作为SDK原始状态的参考基准
> - 实际开发在第3章规划后创建的 `AWRL6844_HealthDetect` 目录进行

---

## 2.2 在CCS中导入工程

> 💡 **工程位置**：`D:\7.project\TI_Radar_Project\project-code\mmw_demo_SDK_reference\`
> ⚠️ **说明**：此工程仅用于验证环境，**不做任何修改**

### 2.2.1 配置CCS产品发现路径

> ⚠️ **重要**：导入工程前必须先配置产品发现路径！

#### 打开设置

```
CCS菜单：Window → Preferences → Code Composer Studio → Products
```

#### 添加产品发现路径

在 `Product discovery-path` 中添加SDK路径：

```
C:\ti\MMWAVE_L_SDK_06_01_00_01
```

> 💡 **注意**：mmw_demo是单核R5F工程，不需要添加C66x的mathlib和dsplib路径

#### 验证产品已发现

在 `Products` 页面应显示：

| 产品名称                      | 版本      | 状态 |
| ----------------------------- | --------- | ---- |
| mmWave low-power SDK xWRL68xx | 06.01.00.01 | ✅   |

---

### 2.2.2 导入工程到CCS

#### 导入步骤

```
步骤1: 打开CCS

步骤2: Project → Import CCS Projects...

步骤3: 选择搜索目录
        点击 Browse... 按钮
        选择：D:\7.project\TI_Radar_Project\project-code\mmw_demo_SDK_reference\xwrL684x-evm\r5fss0-0_freertos

步骤4: CCS会自动发现1个工程：
        ☑ mmwave_demo_xwrL684x-evm_r5fss0-0_freertos_ti-arm-clang

步骤5: 保持默认选项：
        ☑ Copy projects into workspace (可选，推荐不勾选，直接使用原位置)

步骤6: 点击 Finish
```

> 💡 **工程说明**：
> - 这是**单核R5F工程**（AWRL6844只有R5F，没有C66x DSS）
> - 使用**FreeRTOS**操作系统
> - 使用**TI ARM Clang**编译器

> ⚠️ **不要重命名工程** - 保持SDK原始名称，便于识别和参考

---

## 2.3 编译验证

### 2.3.1 编译前检查

```
检查清单：
├── ✅ CCS版本正确 (20.4.0 或更高)
├── ✅ SDK路径配置正确 (Product discovery-path)
├── ✅ 工程无红色错误标记
└── ✅ 工程已成功导入
```

---

### 2.3.2 执行编译

#### 编译步骤

```
步骤1: 选择R5F工程
        在Project Explorer中选择：
        mmwave_demo_xwrL684x-evm_r5fss0-0_freertos_ti-arm-clang

步骤2: 右键 → Build Project

步骤3: 等待编译完成（首次编译需要3-5分钟）

步骤4: 检查Console输出
```

#### 编译成功标志

```
**** Build of configuration Release for project mmwave_demo_xwrL684x-evm_r5fss0-0_freertos_ti-arm-clang ****

...编译过程...
making directory: Release
...
Building file: "../main.c"
...
Building target: "mmwave_demo.release.out"
...

**** Build Finished ****

✅ 0 errors, 0 warnings (或少量warning可忽略)
```

---

### 2.3.3 编译输出

#### 输出文件位置

```
project-code/mmw_demo_SDK_reference/xwrL684x-evm/r5fss0-0_freertos/ti-arm-clang/Release/
├── mmwave_demo.release.out         ← ELF可执行文件
├── mmwave_demo.release.map         ← 内存映射文件
└── mmwave_demo.release.appimage    ← 可烧录固件（最终产物）
```

> 💡 **说明**：这个 `.appimage` 文件就是可烧录的固件，包含了MSS和DSS的代码。

#### 输出文件说明

| 文件           | 说明                       |
| -------------- | -------------------------- |
| `*.appimage` | 合并的可烧录固件（烧录用） |
| `*.out`      | ELF调试文件（CCS调试用）   |
| `*.map`      | 内存映射文件（分析用）     |

---

### 2.3.4 常见编译错误处理

#### 错误1: SDK路径未找到

```
错误信息: fatal error: ti/xxx.h: No such file or directory

解决方法:
1. 检查Product discovery-path是否添加SDK路径
2. 重启CCS
3. Clean Project 后重新编译
```

#### 错误2: 库文件未找到

```
错误信息: Product ti.mathlib.c66x v0.0 is not currently installed

解决方法:
1. 检查Product discovery-path是否添加mathlib和dsplib路径
2. 在Settings → Products 中确认已识别
3. 重新导入工程
```

---

## 2.4 固件烧录

### 2.4.1 烧录前准备

#### 硬件设置（SOP跳线）

```
⚠️ 烧录前必须设置为Flash编程模式：
- S7: OFF（Flash模式）
- S8: OFF（Flash模式）
```

#### 确认COM端口

```powershell
# 从第1章记录的COM端口号，例如：
# CLI端口: COM3
# 数据端口: COM4
```

---

### 2.4.2 使用SDK Visualizer烧录（推荐）

#### 启动Visualizer

```powershell
Start-Process "C:\ti\MMWAVE_L_SDK_06_01_00_01\tools\visualizer\visualizer.exe"
```

#### 烧录步骤

```
步骤1: 切换到 Flash 标签页

步骤2: 选择COM端口
        - 自动检测或手动选择CLI端口（如COM3）

步骤3: 选择设备
        - Device: AWRL6844

步骤4: 设置SOP开关（按软件提示）
        - 切换到 FLASHING MODE (S7-OFF, S8-OFF)
        - 按S2复位键

步骤5: 选择固件文件
        - 点击 "Browse..." 按钮
        - 选择编译输出的固件：
          project-code/mmw_demo_SDK_reference/xwrL684x-evm/r5fss0-0_freertos/ti-arm-clang/Release/mmwave_demo.release.appimage
        
        或使用预编译固件：
          project-code/mmw_demo_SDK_reference/prebuilt_binaries/xwrL684x-evm/mmwave_demo.release.appimage

步骤6: 点击 FLASH 按钮
        - 等待烧录完成（显示进度条）

步骤7: 恢复SOP开关（按软件提示）
        - 切换回 FUNCTIONAL MODE (S7-ON, S8-ON)
        - 按S2复位键
```

#### 烧录成功标志

```
✅ 显示进度条到100%
✅ 显示 "Flashing completed successfully"
✅ 串口有启动信息输出
```

---

### 2.4.3 使用命令行烧录（备选）

#### 简单烧录（不加偏移量）

```powershell
cd C:\ti\MMWAVE_L_SDK_06_01_00_01\tools\FlashingTool

# 使用编译输出的固件
.\arprog_cmdline_6844.exe -p COM3 `
  -f1 "D:\7.project\TI_Radar_Project\project-code\mmw_demo_SDK_reference\xwrL684x-evm\r5fss0-0_freertos\ti-arm-clang\Release\mmwave_demo.release.appimage" `
  -s SFLASH `
  -c

# 或使用预编译固件
.\arprog_cmdline_6844.exe -p COM3 `
  -f1 "D:\7.project\TI_Radar_Project\project-code\mmw_demo_SDK_reference\prebuilt_binaries\xwrL684x-evm\mmwave_demo.release.appimage" `
  -s SFLASH `
  -c
```

> 💡 **推荐使用不加偏移量的简单烧录**，成功率更高。

---

## 2.5 雷达配置加载

> 🚨 **关键步骤**：固件烧录后，**必须发送配置文件才能启动雷达**！

### 2.5.1 配置文件的作用

**雷达启动流程**：

```
烧录固件 → 发送配置文件(.cfg) → 执行sensorStart → 雷达输出数据
```

**没有配置文件，雷达不会工作！**

---

### 2.5.2 标准配置文件位置

#### 项目中的配置文件（推荐）

```
D:\7.project\TI_Radar_Project\project-code\mmw_demo_SDK_reference\profiles\
├── profile_2T4R_bpm.cfg      # 2发4收BPM模式
├── profile_3T4R_tdm.cfg      # 3发4收TDM模式
├── profile_4T4R_tdm.cfg      # 4发4收TDM模式（✅ 推荐）
└── monitors.cfg              # 带硬件监控的配置
```

> 💡 **推荐使用**: `profile_4T4R_tdm.cfg` - 4发4收TDM模式，性能最佳

#### 配置文件详细对比分析

两个主要配置文件的详细对比请参见：

📋 **[附录A - 配置文件对比分析](./AWRL6844雷达健康检测-附录A-配置文件对比分析.md)**

该附录详细说明：
- 📊 两个配置文件的逐行对比
- 🔧 monitors.cfg独有的12种硬件监控功能
  - PLL电压监控
  - 8路TX功率监控
  - 4路基带功率监控
  - DC信号监控
  - TX-RX环路测试
  - 高通滤波器监控
  - 时钟监控
- 🎯 应用场景建议（Demo验证 vs 生产测试）
- 📈 性能影响分析
- 🔍 故障诊断示例

**快速结论**：
- `profile_4T4R_tdm.cfg` → **第2章验证推荐**（简单高效）
- `monitors.cfg` → 生产测试、硬件诊断（功能全面，性能开销+15%）

#### SDK原始配置（备选）

```
C:\ti\MMWAVE_L_SDK_06_01_00_01\examples\mmw_demo\mmwave_demo\profiles\profile_4T4R_tdm.cfg
```

---

### 2.5.3 使用SDK Visualizer发送配置

#### 发送步骤

```
步骤1: 启动SDK Visualizer（如已启动则继续）

步骤2: 切换到 Sensor Config 标签页

步骤3: 选择COM端口
        - CLI Port: COM3 (命令端口)
        - Data Port: COM4 (数据输出端口)

步骤4: 连接设备
        - 点击 "Connect" 按钮
        - 应看到连接成功提示

步骤5: 加载配置文件
        - 点击 "Browse..." 按钮
        - 选择配置文件：
          D:\7.project\TI_Radar_Project\project-code\mmw_demo_SDK_reference\profiles\profile_4T4R_tdm.cfg

步骤6: 发送配置
        - 点击 "Send Config" 按钮
        - 等待所有命令执行完成
        - 每行命令应返回 "Done"

步骤7: 启动雷达
        - 配置发送成功后，雷达会自动启动
        - 或手动点击 "Start Sensor"
```

---

### 2.5.4 配置文件内容说明

**标准mmw_demo配置包含**：

| 配置项      | CLI命令         | 作用          |
| ----------- | --------------- | ------------- |
| 通道配置    | `channelCfg`  | TX/RX通道使能 |
| ADC配置     | `adcCfg`      | ADC采样参数   |
| Profile配置 | `profileCfg`  | Chirp参数     |
| Chirp配置   | `chirpCfg`    | 具体Chirp定义 |
| 帧配置      | `frameCfg`    | 帧结构        |
| CFAR配置    | `cfarCfg`     | 目标检测参数  |
| 传感器启动  | `sensorStart` | 启动雷达      |

---

## 2.6 数据输出验证

### 2.6.1 CLI端口验证

#### 使用串口工具连接

**连接参数**：

- 端口：CLI端口（COM3）
- 波特率：115200
- 数据位：8
- 停止位：1
- 校验位：None

#### 验证CLI响应

```
发送命令: help

应看到输出:
Available commands:
- sensorStop
- sensorStart
- profileCfg
- ...
```

---

### 2.6.2 数据端口验证

#### 在SDK Visualizer中查看

```
步骤1: 切换到 Plots 标签页

步骤2: 应看到实时数据显示：
        - Range-Azimuth Heat Map（距离-方位热图）
        - Detected Objects（检测到的目标点）
        - Statistics（统计信息）

步骤3: 测试场景：
        - 静态环境 → 无点云或极少点云
        - 人员走动 → 有明显移动点云
        - 手摆动 → 能检测到手部运动
```

---

### 2.6.3 功能验证测试

#### 测试场景清单

| 测试场景           | 预期结果           | 验证标志      |
| ------------------ | ------------------ | ------------- |
| **静态环境** | 无点云或极少静态点 | ✅ 无虚警     |
| **人员走动** | 检测到移动点云     | ✅ 能跟踪     |
| **人员静止** | 点云稳定           | ✅ 位置准确   |
| **手部摆动** | 检测到小目标运动   | ✅ 灵敏度正常 |

#### 验证记录

```
验证日期：________
测试人员：________

测试结果：
├── CLI命令响应： □ 正常  □ 异常
├── 数据端口输出： □ 正常  □ 异常
├── 点云可视化：   □ 正常  □ 异常
└── 目标检测：     □ 正常  □ 异常

备注：_______________________________________________
```

---

## 2.7 常见问题排查

### 2.7.1 问题：编译找不到头文件

```
错误: fatal error: ti/common/mmwave_error.h: No such file or directory

排查步骤:
1. 检查SDK是否正确安装
2. 检查Product discovery-path中是否添加SDK路径
3. 重启CCS
4. Clean Project 后重新编译
```

---

### 2.7.2 问题：烧录失败

```
错误: Connect to Device Timeout

排查步骤:
1. 确认SOP开关设置正确（S7-OFF, S8-OFF）
2. 确认使用正确的COM端口
3. 按S2复位键后立即执行烧录
4. 检查USB连接是否稳定
5. 尝试使用命令行烧录（不加偏移量）
```

---

### 2.7.3 问题：配置发送失败

```
错误: CLI命令响应超时或错误

排查步骤:
1. 确认固件已正确烧录
2. 确认SOP开关已恢复到FUNCTIONAL模式（S7-ON, S8-ON）
3. 确认连接的是CLI端口（波特率115200）
4. 重启EVM后重试
5. 检查配置文件是否与固件匹配
```

---

### 2.7.4 问题：无数据输出

```
现象: CLI命令正常，但无点云数据

排查步骤:
1. 确认配置文件已发送成功（所有命令返回"Done"）
2. 确认执行了sensorStart命令
3. 检查数据端口波特率（必须是1250000）
4. 重新发送配置文件
5. 查看Visualizer的Console输出是否有错误
```

---

## 2.8 完成检查清单

### 工程复制检查

| 序号 | 检查项                               | 状态 |
| ---- | ------------------------------------ | ---- |
| 1    | 已从SDK复制mmw_demo工程              | ⬜   |
| 2    | 已重命名为mmw_demo_SDK_reference     | ⬜   |
| 3    | 目录结构完整（src/docs等）           | ⬜   |

### 工程导入和编译检查

| 序号 | 检查项                       | 状态 |
| ---- | ---------------------------- | ---- |
| 1    | 已在CCS中导入3个工程         | ⬜   |
| 2    | Product discovery-path已配置 | ⬜   |
| 3    | 编译无错误                   | ⬜   |
| 4    | 已生成.appimage文件          | ⬜   |

### 固件烧录检查

| 序号 | 检查项                      | 状态 |
| ---- | --------------------------- | ---- |
| 1    | 固件烧录成功                | ⬜   |
| 2    | SOP开关已恢复FUNCTIONAL模式 | ⬜   |
| 3    | 串口有启动信息输出          | ⬜   |

### 配置加载检查

| 序号 | 检查项                    | 状态 |
| ---- | ------------------------- | ---- |
| 1    | 配置文件发送成功          | ⬜   |
| 2    | 所有CLI命令返回"Done"     | ⬜   |
| 3    | 雷达已启动（sensorStart） | ⬜   |

### 数据验证检查

| 序号 | 检查项             | 状态 |
| ---- | ------------------ | ---- |
| 1    | CLI命令响应正常    | ⬜   |
| 2    | 数据端口有输出     | ⬜   |
| 3    | 可视化工具显示点云 | ⬜   |
| 4    | 能检测到人员运动   | ⬜   |

---

## 📊 本章总结

### ✅ 完成标志

当以上所有检查项都打✅时，第2章完成！

**标准Demo验证完成后，你应该拥有**：

- ✅ mmw_demo_SDK_reference工程（SDK参考工程）
- ✅ 能编译通过的代码
- ✅ 能烧录成功的固件
- ✅ 能正常工作的雷达（配置+数据输出）
- ✅ 开发环境正常工作

### 产出文档

建议创建：

```
项目文档/2-开发记录/YYYY-MM-DD/
└── YYYY-MM-DD_标准Demo验证报告.md
```

记录：

- 编译结果
- 烧录方法
- 配置文件
- 测试场景和结果
- 遇到的问题和解决方法

---

### 下一步

**进入第3章：架构演进规划**

- 深入学习DPC机制
- 研究InCabin的多核架构（附录C）
- 设计AWRL6844_HealthDetect的架构
- 规划健康检测功能的部署

> 💡 **重要**：只有验证了标准Demo能正常工作，才能在此基础上进行功能扩展！

---

## 2.9 硬件安装配置

> 📋 **实际安装参数**：见[附录E - 设备安装配置信息](./AWRL6844雷达健康检测-附录E-设备安装配置信息.md)

本章验证使用的硬件配置：
- **雷达位置**: 墙面安装，高度1.78m，俯角-40°
- **检测区域**: 4m宽 × 5m深 × 2m高
- **配置文件**: profile_4T4R_tdm.cfg

详细的安装参数、区域定义、配置文件说明请参考附录E。

---

## 📚 参考资源

### 项目相关文档

| 文档                  | 位置                            | 用途             |
| --------------------- | ------------------------------- | ---------------- |
| 附录C-InCabin架构参考 | 同目录                          | 学习多核架构设计 |
| SDK固件研究           | 3-固件工具/06-SDK固件研究/      | 理解mmw_demo代码 |
| 烧录测试报告          | 3-固件工具/03-固件烧录测试/     | 烧录问题排查     |
| 配置参数研究          | 3-固件工具/05-雷达配置参数研究/ | 配置文件调优     |

### TI官方文档

| 文档               | 位置                                                                        |
| ------------------ | --------------------------------------------------------------------------- |
| mmw_demo用户指南   | `C:\ti\MMWAVE_L_SDK_06_01_00_01\docs\`                                    |
| API参考手册        | `C:\ti\MMWAVE_L_SDK_06_01_00_01\docs\api_guide_xwrL64xx\`                 |
| Visualizer用户指南 | `C:\ti\MMWAVE_L_SDK_06_01_00_01\docs\Low_Power_Visualizer_User_Guide.pdf` |

---

> 🎯 **第2章核心目标**：验证mmw_demo_SDK_reference工程，确保开发环境正常工作！
