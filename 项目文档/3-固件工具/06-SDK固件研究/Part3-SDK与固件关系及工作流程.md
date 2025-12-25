# 🔗 SDK与固件关系及完整工作流程

> **文档版本**: v1.0  
> **创建日期**: 2025-12-25  
> **适用硬件**: AWRL6844-EVM  
> **前置文档**: [Part1](Part1-SDK基础概念与三目录详解.md) | [Part2](Part2-固件校验方法完整指南.md)

---

## 📋 目录

- [第一章：SDK与固件的本质关系](#第一章sdk与固件的本质关系)
- [第二章：固件完整生命周期](#第二章固件完整生命周期)
- [第三章：标准工作流程](#第三章标准工作流程)
- [第四章：配置文件与固件的关系](#第四章配置文件与固件的关系)

---

## 第一章：SDK与固件的本质关系

### 1.1 从源码到固件的转换

**核心关系**：
```
SDK（工具 + 源码）
    ↓ 编译
固件（二进制.appimage）
    ↓ 烧录
Flash存储器
    ↓ 启动
芯片运行
```

### 1.2 类比理解

| 概念 | 类比 | 说明 |
|-----|------|------|
| **SDK** | 厨房（工具+食材+菜谱） | 提供开发环境 |
| **源码** | 食材（原料） | C/C++代码 |
| **编译** | 烹饪（加工） | 转换为机器码 |
| **固件** | 成品菜（食物） | 可执行的二进制文件 |
| **烧录** | 装盘上菜（传递） | 写入Flash |
| **运行** | 食用（消费） | 芯片执行代码 |

### 1.3 依赖关系图

```
📦 MMWAVE_L_SDK (开发环境)
    ├─ 📚 包含：源码 + 库 + 工具链
    ├─ 🔧 编译器：ti-arm-clang
    ├─ 🛠️ 构建脚本：makefile
    └─ 📄 输出：.appimage固件
         ↓
    🔥 固件（独立二进制）
         ├─ ✅ 可独立运行（不需要SDK）
         ├─ ✅ 可拷贝到其他电脑
         ├─ ✅ 可批量烧录到设备
         └─ ❌ 不包含源码（已编译）
              ↓
         💾 Flash存储器
              ↓
         🎯 芯片执行
```

### 1.4 关键问题解答

#### Q1: 烧录固件后还需要SDK吗？

**答案**：❌ **不需要**

**原因**：
- 固件是**完整的可执行程序**
- 所有代码已编译为机器码
- 芯片直接从Flash读取并执行
- SDK只在**开发和编译阶段**需要

**类比**：
```
SDK = 工厂（生产软件）
固件 = 产品（可独立使用）

产品出厂后不需要工厂
固件烧录后不需要SDK
```

#### Q2: 固件包含什么？

**固件包含**：
- ✅ 所有编译后的代码（MSS + DSS）
- ✅ 必要的驱动和库
- ✅ 启动引导程序（SBL，Multi-Image时）
- ✅ 配置数据（如果编译时内嵌）

**固件不包含**：
- ❌ 源代码（.c/.h文件）
- ❌ 编译工具（编译器）
- ❌ 文档和注释
- ❌ 开发工具（调试器等）

#### Q3: 为什么SDK有多个目录？

**原因**：不同SDK服务于**不同阶段**和**不同用户**

```
开发阶段：
MMWAVE_L_SDK → 编写代码 → 编译固件

测试阶段：
radar_toolbox → 加载配置 → 测试功能

硬件验证：
mmwave_studio → RF测试 → 性能验证

生产阶段：
MMWAVE_L_SDK → 批量烧录 → 质量检测
```

---

## 第二章：固件完整生命周期

### 2.1 固件生命周期图

```
┌─────────────────────────────────────────────────────────────────┐
│                     固件完整生命周期                              │
└─────────────────────────────────────────────────────────────────┘

📝 阶段1: 开发（使用MMWAVE_L_SDK）
    ├─ 编写源码（C/C++）
    ├─ 配置项目（选择功能模块）
    └─ 添加自定义算法
         ↓
🔨 阶段2: 编译（使用ti-arm-clang）
    ├─ 预处理（宏展开）
    ├─ 编译（生成.obj目标文件）
    ├─ 链接（合并为.out可执行文件）
    └─ 转换（生成.appimage固件）
         ↓
✅ 阶段3: 验证（校验固件）
    ├─ Meta魔数检查（0x5254534D）
    ├─ 设备ID验证（xWRL684x）
    ├─ 格式检测（Multi-Image/Single-Image）
    └─ 完整性校验
         ↓
🔥 阶段4: 烧录（使用烧录工具）
    ├─ 连接设备（JTAG/UART）
    ├─ 擦除Flash
    ├─ 写入固件（到指定偏移量）
    └─ 验证烧录
         ↓
🚀 阶段5: 运行（芯片自启动）
    ├─ ROM Bootloader启动
    ├─ 加载SBL（如果Multi-Image）
    ├─ SBL加载Application
    └─ Application运行
         ↓
⚙️ 阶段6: 配置（通过CLI串口）
    ├─ 发送配置命令（.cfg文件）
    ├─ 设置雷达参数
    └─ 启动传感器（sensorStart）
         ↓
📊 阶段7: 数据采集（实时运行）
    ├─ 检测目标
    ├─ 输出数据（点云/目标信息）
    └─ 可视化显示
         ↓
🔄 阶段8: 维护（版本管理）
    ├─ 固件更新（新功能/修复bug）
    ├─ 重新烧录
    └─ 回归测试
```

### 2.2 各阶段使用的工具

| 阶段 | 工具 | SDK来源 | 用户角色 |
|-----|------|---------|---------|
| 开发 | Code Composer Studio | MMWAVE_L_SDK | 固件开发者 |
| 编译 | ti-arm-clang | MMWAVE_L_SDK | 固件开发者 |
| 验证 | 校验脚本 | 自定义 | 测试工程师 |
| 烧录 | arprog_cmdline_6844 | MMWAVE_L_SDK | 生产工程师 |
| 运行 | - | - | 最终用户 |
| 配置 | 串口工具/visualizer | radar_toolbox | 应用工程师 |
| 采集 | visualizer | MMWAVE_L_SDK/radar_toolbox | 应用工程师 |
| 维护 | 版本控制系统 | - | 项目经理 |

### 2.3 时间线估算

**首次开发**：
```
学习SDK         2-5天
修改示例代码     3-7天
编译调试        1-3天
固件验证        1天
烧录测试        1天
配置调试        2-5天
----------------------------
总计：         10-26天
```

**后续开发**：
```
修改代码        1-2天
编译烧录        1天
测试验证        1-2天
----------------------------
总计：         3-5天
```

**生产烧录**（单个设备）：
```
连接设备        1分钟
烧录固件        2-5分钟
功能测试        3-5分钟
----------------------------
总计：         6-11分钟
```

---

## 第三章：标准工作流程

### 3.1 场景1：使用标准固件（最简单） ⭐⭐⭐

**目标**：快速评估AWRL6844功能

**步骤**：

#### Step 1: 准备环境
```powershell
# 安装必需工具
1. MMWAVE_L_SDK_06_01_00_01  ← 固件和烧录工具
2. radar_toolbox_3_30_00_06  ← 配置文件

# 硬件连接
1. AWRL6844-EVM连接电源
2. USB连接PC（UART + 数据端口）
3. 确认COM端口（设备管理器查看）
```

#### Step 2: 烧录标准固件
```powershell
# 进入烧录工具目录
cd C:\ti\MMWAVE_L_SDK_06_01_00_01\tools\FlashingTool

# 烧录Multi-Image固件（推荐）
.\arprog_cmdline_6844.exe `
    -i "C:\ti\MMWAVE_L_SDK_06_01_00_01\examples\mmw_demo\xwrL684x-evm\mmwave_demo.release.appimage" `
    -d xwrl684x `
    -o 0x0

# 等待烧录完成（约2-5分钟）
```

#### Step 3: 配置雷达参数
```powershell
# 打开可视化工具
C:\ti\radar_toolbox_3_30_00_06\tools\visualizers\Applications_Visualizer\Industrial_Visualizer\Industrial_Visualizer.exe

# 或使用MMWAVE_L_SDK的visualizer
C:\ti\MMWAVE_L_SDK_06_01_00_01\mmwave_l_sdk_06_01_00_01\tools\visualizer\visualizer.exe

# 在可视化工具中：
1. 连接串口（CLI: COM3@115200, Data: COM4@1250000）
2. 加载配置文件
   C:\ti\radar_toolbox_3_30_00_06\tools\Adc_Data_Capture_Tool_DCA1000_CLI\chirp_configs\xWRL6844_4T4R_tdm.cfg
3. 发送配置（点击"Send Config"）
4. 启动雷达（sensorStart）
```

#### Step 4: 观察数据
```
可视化工具会显示：
- 检测到的目标（点云）
- 距离-多普勒热图
- 目标坐标信息
- 帧统计信息
```

**总耗时**：约15-30分钟（首次）

---

### 3.2 场景2：开发自定义固件 ⭐⭐

**目标**：修改固件功能或算法

**步骤**：

#### Step 1: 安装开发环境
```
1. 安装Code Composer Studio (CCS)
   下载地址：https://www.ti.com/tool/CCSTUDIO
   
2. 安装MMWAVE_L_SDK
   下载地址：https://www.ti.com/tool/MMWAVE-L-SDK
   
3. 配置CCS
   - 导入SDK路径
   - 配置ti-arm-clang编译器
```

#### Step 2: 导入示例项目
```
CCS步骤：
1. File → Import → CCS Projects
2. 选择目录：
   C:\ti\MMWAVE_L_SDK_06_01_00_01\examples\mmw_demo\xwrL684x-evm
3. 导入两个项目：
   - mmw_demo_mss（MSS核心）
   - mmw_demo_dss（DSS核心）
```

#### Step 3: 修改代码
```c
// 示例：添加自定义CLI命令

// 文件：examples/mmw_demo/mss/mmw_cli.c

// 添加命令定义
static int32_t MmwDemo_CLIMyCustomCmd(int32_t argc, char* argv[])
{
    // 处理自定义命令
    CLI_write("Custom command executed!\n");
    return 0;
}

// 注册命令
CLI_Cmd myCustomCmd = {
    "myCommand",
    MmwDemo_CLIMyCustomCmd
};

// 在初始化函数中注册
CLI_addCmd(&myCustomCmd);
```

#### Step 4: 编译固件
```
CCS操作：
1. 选择项目：mmw_demo_mss
2. Project → Build Project
3. 选择项目：mmw_demo_dss
4. Project → Build Project
5. 等待编译完成
```

#### Step 5: 生成appimage
```powershell
# 使用buildImage_creator工具
cd C:\ti\MMWAVE_L_SDK_06_01_00_01\tools\buildImage_creator

# 生成Multi-Image固件
.\out2rprc.exe mss_binary.xer6 mss.tmp
.\out2rprc.exe dss_binary.xe674 dss.tmp
.\multiImageGen.exe meta.bin mss.tmp dss.tmp custom_firmware.appimage

# 输出：custom_firmware.appimage
```

#### Step 6: 烧录测试
```powershell
# 烧录自定义固件
.\arprog_cmdline_6844.exe `
    -i custom_firmware.appimage `
    -d xwrl684x `
    -o 0x0

# 连接串口测试
# 发送新命令：myCommand
```

**总耗时**：3-7天（包括学习和调试）

---

### 3.3 场景3：批量生产烧录 ⭐⭐⭐

**目标**：批量烧录设备

**步骤**：

#### Step 1: 准备烧录环境
```powershell
# 创建烧录脚本：flash_production.ps1

param(
    [string]$FirmwarePath = "C:\production\firmware\mmwave_demo.appimage",
    [int]$DeviceCount = 10
)

$FlasherPath = "C:\ti\MMWAVE_L_SDK_06_01_00_01\tools\FlashingTool\arprog_cmdline_6844.exe"

for ($i = 1; $i -le $DeviceCount; $i++) {
    Write-Host "烧录设备 $i / $DeviceCount"
    
    # 提示连接设备
    Write-Host "请连接设备并按回车继续..."
    Read-Host
    
    # 烧录固件
    & $FlasherPath -i $FirmwarePath -d xwrl684x -o 0x0
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ 设备 $i 烧录成功" -ForegroundColor Green
    } else {
        Write-Host "❌ 设备 $i 烧录失败" -ForegroundColor Red
    }
    
    Write-Host "--------------------"
}

Write-Host "批量烧录完成！"
```

#### Step 2: 执行烧录
```powershell
# 运行烧录脚本
.\flash_production.ps1 -FirmwarePath "firmware.appimage" -DeviceCount 50
```

#### Step 3: 功能测试（可选）
```powershell
# 自动化测试脚本：test_device.ps1

$Port = "COM3"
$BaudRate = 115200

# 打开串口
$serialPort = New-Object System.IO.Ports.SerialPort $Port, $BaudRate
$serialPort.Open()

# 发送基本配置
$commands = @(
    "sensorStop",
    "channelCfg 15 7 0",
    "sensorStart"
)

foreach ($cmd in $commands) {
    $serialPort.WriteLine($cmd)
    Start-Sleep -Milliseconds 500
}

# 检查响应
$response = $serialPort.ReadExisting()
if ($response -match "sensorStart Ignored") {
    Write-Host "✅ 设备功能正常"
} else {
    Write-Host "❌ 设备响应异常"
}

$serialPort.Close()
```

**总耗时**：6-11分钟/设备

---

## 第四章：配置文件与固件的关系

### 4.1 配置文件的作用

**核心概念**：
```
固件 = 程序逻辑（代码）
配置文件 = 运行参数（数据）

固件定义"能做什么"
配置文件定义"怎么做"
```

**类比**：
```
固件 = 汽车（功能）
配置文件 = 驾驶方式（参数）

同一辆车可以：
- 高速行驶（配置A）
- 慢速行驶（配置B）
- 停车（配置C）
```

### 4.2 固件与配置的依赖关系

#### 关系1：固件决定支持的命令

**示例**：mmwave_demo固件支持22个命令

```cfg
# mmwave_demo.appimage支持的命令（22个）
sensorStop
channelCfg
chirpComnCfg
chirpTimingCfg
frameCfg
...（22个命令）
sensorStart
```

**其他固件**：可能只支持部分命令

```cfg
# 简化版固件可能只支持10个命令
sensorStop
channelCfg
frameCfg
sensorStart
```

#### 关系2：配置文件必须匹配固件

**✅ 正确匹配**：
```
固件：mmwave_demo.appimage（支持22命令）
配置：xWRL6844_4T4R_tdm.cfg（使用22命令）
结果：✅ 正常工作
```

**❌ 错误匹配**：
```
固件：simple_demo.appimage（只支持10命令）
配置：xWRL6844_4T4R_tdm.cfg（使用22命令）
结果：❌ 部分命令无效
```

### 4.3 配置文件的工作流程

```
┌─────────────────────────────────────────┐
│   配置文件（.cfg）在PC上                  │
│   - 纯文本文件                           │
│   - 22条CLI命令                          │
│   - 参数值（频率、帧率等）                │
└─────────────────────────────────────────┘
          ↓ 通过串口发送（115200波特率）
┌─────────────────────────────────────────┐
│   CLI端口（COM3）                        │
│   - 接收命令                             │
│   - 解析参数                             │
│   - 验证有效性                           │
└─────────────────────────────────────────┘
          ↓ 固件内部处理
┌─────────────────────────────────────────┐
│   固件执行                               │
│   - 配置寄存器                           │
│   - 设置雷达参数                         │
│   - 启动ADC/DSP                          │
└─────────────────────────────────────────┘
          ↓ 雷达运行
┌─────────────────────────────────────────┐
│   数据输出（数据端口COM4）                │
│   - 点云数据                             │
│   - 目标信息                             │
│   - 统计信息                             │
└─────────────────────────────────────────┘
```

### 4.4 配置文件的类型

#### 类型1：基础配置（Basic Configuration）

**用途**：基本功能测试

**示例**：`xWRL6844_2T4R_profile.cfg`
```cfg
% 基础配置（10命令）
sensorStop
channelCfg 7 3 0              % 2TX 2RX
chirpComnCfg 0 0 0 0 0 0 0 1
frameCfg 0 0 16 0 50 1 0
sensorStart
```

#### 类型2：标准配置（Standard Configuration）

**用途**：完整功能演示

**示例**：`xWRL6844_4T4R_tdm.cfg`
```cfg
% 标准配置（22命令）
sensorStop
channelCfg 15 7 0             % 4TX 3RX
chirpComnCfg 0 0 0 0 0 0 0 1
chirpTimingCfg 1 1 0 0
adcDataDitherCfg 1
frameCfg 0 0 32 0 50 1 0
gpAdcMeasConfig 0 1
...（22个命令）
sensorStart
```

#### 类型3：应用配置（Application Configuration）

**用途**：特定场景优化

**示例**：`people_tracking_6844.cfg`
```cfg
% 人员跟踪配置
sensorStop
channelCfg 15 7 0
frameCfg 0 0 64 0 33 1 0      % 提高帧率到33fps
cfarProcCfg_Range 0 2 4 4 4 16 16 4 2 30.00 0  % 优化距离检测
aoaProcCfg 64 64 0 0 1 1      % 启用角度估计
sensorStart
```

### 4.5 如何选择配置文件

**决策树**：
```
你的需求是什么？
    ├─ 快速测试功能 → 基础配置（10命令）
    ├─ 完整功能演示 → 标准配置（22命令）
    └─ 特定应用场景 → 应用配置
         ├─ 人员跟踪 → people_tracking_*.cfg
         ├─ 占用检测 → occupancy_*.cfg
         ├─ 手势识别 → gesture_*.cfg
         └─ 自定义需求 → 修改现有配置
```

---

## 📝 总结

### 核心关系图

```
┌────────────────────────────────────────────────────────┐
│                  SDK与固件生态系统                      │
└────────────────────────────────────────────────────────┘

MMWAVE_L_SDK（开发SDK）
    ├─ 📝 源码（C/C++）
    ├─ 🔧 工具链（ti-arm-clang）
    ├─ 🛠️ 构建脚本
    └─ 📦 输出
         ↓ 编译
    🔥 固件（.appimage）← 独立可执行
         ├─ 定义：支持哪些命令
         ├─ 包含：所有程序逻辑
         └─ 烧录：到Flash存储器
              ↓ 运行时配置
         ⚙️ 配置文件（.cfg）
              ├─ 定义：运行参数
              ├─ 来源：radar_toolbox
              └─ 发送：通过CLI串口
                   ↓
              📊 雷达运行
                   ├─ 检测目标
                   ├─ 输出数据
                   └─ 可视化显示
```

### 关键要点

1. **SDK ≠ 固件**
   - SDK是开发工具
   - 固件是编译产物

2. **固件独立运行**
   - 烧录后不需要SDK
   - 可拷贝到其他设备

3. **配置文件是参数**
   - 不修改固件本身
   - 只改变运行行为

4. **三个SDK各有分工**
   - MMWAVE_L_SDK：开发+烧录
   - radar_toolbox：配置+可视化
   - mmwave_studio：RF测试

### 下一步

- ➡️ 继续阅读：[Part4-实践案例与常见问题.md](Part4-实践案例与常见问题.md)
- ➡️ 返回目录：[README.md](README.md)

---

**最后更新**：2025-12-25  
**文档作者**：项目开发团队
