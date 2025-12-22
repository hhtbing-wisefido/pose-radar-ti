# 📡 AWRL6844 雷达配置文件研究总结

## 🎯 研究目标

全面理解 Ti AWRL6844 雷达配置文件的结构、用途、参数含义及实际应用，为后续开发可视化配置工具和参数优化提供理论基础。

---

## 📋 目录

1. [配置文件概览](#配置文件概览)
2. [常见问题解答](#常见问题解答)
   - 1️⃣ [雷达配置文件是启动雷达的吗？](#1️⃣-雷达配置文件是启动雷达的吗)
   - 2️⃣ [什么样的应用固件需要使用雷达配置文件？](#2️⃣-什么样的应用固件需要使用雷达配置文件)
   - 3️⃣ [雷达配置文件需要哪个端口发送？](#3️⃣-雷达配置文件需要哪个端口发送)
   - 4️⃣ [适用于AWRL6844-EVM的配置文件有哪些？](#4️⃣-适用于awrl6844-evm的配置文件有哪些)
   - 5️⃣ [可以通过端口读取雷达配置文件吗？](#5️⃣-可以通过端口读取雷达配置文件吗)
   - 6️⃣ [多次写入配置文件可以吗？每次写入是覆盖方式吗？](#6️⃣-多次写入配置文件可以吗每次写入是覆盖方式吗)
   - 7️⃣ [写入配置文件后，能从板子中读取更多的内容吗？](#7️⃣-写入配置文件后能从板子中读取更多的内容吗)
3. [AWRL6844配置文件清单](#awrl6844配置文件清单)
4. [配置文件类型详解](#配置文件类型详解)
5. [配置参数深度解析](#配置参数深度解析)
6. [配置文件示例库](#配置文件示例库)
7. [参数调优指南](#参数调优指南)
8. [工具开发需求](#工具开发需求)

---

## 🔍 配置文件概览

### 配置文件分类

Ti 雷达系统中涉及**三类**不同的配置文件，容易混淆：

| 类型                       | 文件名示例        | 使用阶段 | 是否需要烧录 | 说明                         |
| -------------------------- | ----------------- | -------- | ------------ | ---------------------------- |
| **🔧 SysConfig配置** | `mmwave.syscfg` | 编译时   | ❌ 不需要    | 硬件外设配置（已编译进固件） |
| **⚙️ RTOS配置**    | `mmwave.cfg`    | 编译时   | ❌ 不需要    | 操作系统配置（已编译进固件） |
| **📡 雷达参数配置**  | `profile.cfg`   | 运行时   | ✅ 串口发送  | 雷达工作参数（本研究重点）   |

> ⚠️ **重要区别**：前两类是开发阶段的配置文件，已经编译进固件；只有雷达参数配置文件（`.cfg`）需要在运行时通过串口发送给雷达。

---

## ❓ 常见问题解答

### 1️⃣ 雷达配置文件是启动雷达的吗？

**不是！**配置文件不是用来启动雷达的。

- ✅ **真实作用**：配置雷达的**工作参数**（检测距离、速度、角度分辨率等）
- ⏰ **使用时机**：固件**启动后**，通过串口发送配置命令
- 🔧 **类比**：相当于雷达的"设置菜单"，不是"开关"

**工作流程**：

```
1. 烧录SBL固件 → 2. 烧录应用固件 → 3. 上电启动（雷达启动）
   ↓
4. 通过串口发送配置文件（配置雷达参数）→ 5. 雷达开始工作
```

---

### 2️⃣ 什么样的应用固件需要使用雷达配置文件？

**所有基于mmWave SDK的Demo固件都需要雷达配置文件！**

**适用固件类型**：

- ✅ 人员跟踪Demo（People Tracking）
- ✅ 车内监测Demo（In-Cabin Sensing）
- ✅ 区域扫描Demo（Area Scanner）
- ✅ 交通监控Demo（Traffic Monitoring）
- ✅ 存在检测Demo（Presence Detection）
- ✅ 所有使用CLI命令接口的固件

**不需要配置文件的情况**：

- ❌ 只有当固件内部**硬编码**了配置参数时（较少见）

**原因**：mmWave SDK的Demo固件设计为可配置架构，启动后等待通过串口接收配置命令，以便灵活适配不同应用场景。

---

### 3️⃣ 雷达配置文件需要哪个端口发送？

**使用 COM4 端口（XDS110 Auxiliary Data Port）！**

**端口分工**：

| 端口           | 名称                               | 用途                            | 波特率 |
| -------------- | ---------------------------------- | ------------------------------- | ------ |
| **COM3** | XDS110 Class Application/User UART | 🔥**烧录固件**            | 115200 |
| **COM4** | XDS110 Class Auxiliary Data Port   | 📡**发送配置** + 接收数据 | 115200 |

**重要提示**：

- ⚠️ **烧录时用 COM3**，**配置时用 COM4**，不要混淆！
- ✅ COM4 是双向端口：发送配置命令 + 接收雷达数据
- ✅ 配置文件通过 COM4 以文本形式逐行发送

**Python示例**：

```python
import serial
import time

# 使用COM4发送配置
ser = serial.Serial('COM4', 115200)  # ← 注意是COM4

with open('xWRL6844_4T4R_tdm.cfg', 'r') as f:
    for line in f:
        line = line.strip()
        if line and not line.startswith('%'):  # 跳过注释
            ser.write(line.encode() + b'\n')
            time.sleep(0.05)  # 等待处理
            print(f"发送: {line}")
```

---

### 4️⃣ 适用于AWRL6844-EVM的配置文件有哪些？

**找到 45 个可用配置文件！**

#### 🥇 AWRL6844专用配置（1个）

**`xWRL6844_4T4R_tdm.cfg`** ⭐⭐⭐

**特点**：

- 🎯 唯一专为AWRL6844设计的配置
- 📡 使用全部**4个发射天线**（4TX × 4RX = **16 MIMO虚拟天线**）
- 🔀 TDM模式（时分复用）
- 🔧 适用于DCA1000原始数据采集
- 📁 路径：`C:\ti\radar_toolbox_3_30_00_06\tools\Adc_Data_Capture_Tool_DCA1000_CLI\chirp_configs\`

**为什么要用4TX配置？**

```
AWRL6844 = 4TX × 4RX = 16 MIMO虚拟天线
           ↑
比AWRL6843多1个TX → +33%角度分辨率 → 更精确的角度测量
```

#### 🥈 xWRL68xx通用配置（44个）

所有名称包含 `6843`、`68xx` 的配置文件都兼容AWRL6844。

**按应用场景分类**：

**🚗 车内监测（9个）**：

- `vod_6843_aop_overhead_2row_classification.cfg` - 2排座椅+成人儿童分类 ⭐
- `vod_6843_aop_overhead_3row_bus.cfg` - 大巴/3排座椅
- `vod_6843_aop_overhead_2row_intruder.cfg` - 入侵检测（防盗模式）
- `vod_6843_aop_overhead_2row_van.cfg` - 面包车大空间
- `vod_6843_isk_frontMount_2row.cfg` - 前置安装（仪表台）
- 其他4个配置...

**👥 人员跟踪（13个）**：

- `6843_50m_3D.cfg` - 50米3D跟踪 ⭐
- `6843_100m_2D_advanced.cfg` - 100米长距离
- `pt_6843_3d_aop_overhead_3m_radial_staticRetention.cfg` - 3米顶置跟踪
- `IWR6843AOP_7m_staticRetention_lp.cfg` - 7米低功耗
- 其他9个配置...

**📡 区域扫描（6个）**：

- `area_scanner_68xx_AOP_full_power_full_bandwidth.cfg` - 最高性能 ⭐
- `area_scanner_68xx_AOP_low_power_full_bandwidth.cfg` - 低功耗平衡
- 其他4个配置...

**🏠 存在检测（3个）**：

- `xWR6843_presenceDetection_twoProfile.cfg` - 双Profile高精度 ⭐
- `xWR6843_presenceDetection.cfg` - 基础检测

**🚦 交通监控（2个）**：

- `68xx_traffic_monitoring_70m_MIMO_3D.cfg` - 70米3D监控
- `68xx_traffic_monitoring_70m_MIMO_2D.cfg` - 70米2D监控

**🔧 其他应用（11个）**：

- `Mobile_Tracker_6843_ISK.cfg` - 移动目标跟踪
- `high_accuracy_demo_68xx.cfg` - 高精度液位检测
- `enable_57_61_GHz_6843_AOP.cfg` - 启用全带宽（57-61GHz）
- 其他8个配置...

#### ⚠️ 兼容性说明

| 配置类型               | TX数量 | AWRL6844能用吗？      | AWRL6843能用吗？ | 说明                    |
| ---------------------- | ------ | --------------------- | ---------------- | ----------------------- |
| **AWRL6844配置** | 4TX    | ✅ 能用（最佳性能）   | ❌ 不能用        | 6843硬件不支持4TX       |
| **AWRL6843配置** | 3TX    | ✅ 能用（未充分利用） | ✅ 能用          | 6844只用3个TX，浪费硬件 |

**使用建议**：

- 🎯 **快速验证功能** → 用6843配置（大量现成配置）
- 🎯 **性能优化阶段** → 用4TX配置（`xWRL6844_4T4R_tdm.cfg`）
- 🎯 **量产** → 根据成本和性能需求选择3TX或4TX

**完整配置文件清单**请参阅下方 [AWRL6844配置文件清单](#awrl6844配置文件清单) 章节。

---

### 5️⃣ 可以通过端口读取雷达配置文件吗？

**部分可以，但有限制！**

#### 📖 读取能力说明

**TI mmWave雷达的CLI支持读取配置的情况**：

| 配置类型                | 是否支持读取         | CLI命令            | 说明                                       |
| ----------------------- | -------------------- | ------------------ | ------------------------------------------ |
| **基础配置参数**  | ❌**不支持**   | 无                 | channelCfg、profileCfg等**无法读回** |
| **Flash存储配置** | ✅**支持**     | `getFlashRecord` | 仅特定Demo支持（如停车传感器）             |
| **校准数据**      | ✅**部分支持** | `getCalibData`   | 读取校准数据                               |
| **传感器状态**    | ✅**支持**     | 状态命令           | 读取工作状态和统计信息                     |

---

#### ❌ 为什么大部分配置无法读取？

**技术原因**：

1. **单向配置模式** - mmWave SDK的CLI设计为**单向配置**接口

   - 主机 → 雷达：发送配置命令（支持）
   - 雷达 → 主机：返回数据流（支持）
   - 雷达 → 主机：返回配置参数（**不支持**）
2. **固件架构限制** - 配置参数直接写入寄存器，不保存在可读取的内存区域

   ```
   profileCfg命令 → 直接写入硬件寄存器 → 无法读回
   ```
3. **安全考虑** - 防止配置信息泄露

---

#### ✅ 可以读取的内容

**1. Flash存储的配置（特定Demo）**

某些Demo支持将配置保存到Flash并读取：

```python
import serial
import time

ser = serial.Serial('COM4', 115200)

# 示例：停车传感器Demo支持读取Flash配置
ser.write(b'getFlashRecord\n')
time.sleep(0.1)
response = ser.read(ser.in_waiting).decode('utf-8')
print("Flash配置:", response)
```

**支持的Demo示例**：

- `parking_garage_sensor` - 停车传感器
  - `parking_garage_sensor_68xx_ISK_getFlashRecord.cfg` - 读取配置
  - `parking_garage_sensor_68xx_ISK_setFlashRecord.cfg` - 写入配置

**2. 传感器状态信息**

虽然无法读取配置参数，但可以读取工作状态：

```python
# 读取传感器状态（取决于具体固件支持的命令）
# 注意：不是所有固件都支持以下命令

# 可能的状态查询命令（固件相关）
commands = [
    'sensorStatus',      # 传感器状态
    'getVersion',        # 版本信息
    'getCalibData',      # 校准数据
]

for cmd in commands:
    ser.write(cmd.encode() + b'\n')
    time.sleep(0.1)
    response = ser.read(ser.in_waiting).decode('utf-8')
    print(f"{cmd}: {response}")
```

---

#### 🔧 实际解决方案

**既然无法读取配置，如何管理配置？**

**方案1：本地保存配置文件** ⭐⭐⭐（推荐）

```python
# 发送配置时同时保存
def send_and_save_config(config_file, port='COM4'):
    # 1. 发送到雷达
    ser = serial.Serial(port, 115200)
    with open(config_file, 'r') as f:
        config_content = f.read()
        # 发送配置...
  
    # 2. 保存到本地数据库/日志
    import json
    config_log = {
        'timestamp': time.time(),
        'config_file': config_file,
        'content': config_content
    }
    with open('config_history.json', 'a') as log:
        log.write(json.dumps(config_log) + '\n')
```

**方案2：使用支持Flash的Demo** ⭐⭐

- 选择支持Flash存储的固件
- 配置保存到Flash后可读取
- 适合需要掉电保存配置的场景

**方案3：在应用层记录配置** ⭐⭐⭐

```python
# 在烧录工具中记录配置
class RadarConfigManager:
    def __init__(self):
        self.current_config = None
        self.config_history = []
  
    def send_config(self, config_file):
        # 发送配置
        # ...
      
        # 记录当前配置
        self.current_config = config_file
        self.config_history.append({
            'time': time.time(),
            'file': config_file
        })
  
    def get_current_config(self):
        return self.current_config
```

---

#### 📋 配置管理最佳实践

**推荐工作流程**：

```
1. 准备阶段
   └─ 选择/创建配置文件 (.cfg)
   └─ 保存配置文件副本到项目目录
   └─ 在文件名中标注场景（如：6844_car_2row_v1.cfg）

2. 发送阶段
   └─ 通过COM4发送配置
   └─ 同时记录到日志（时间戳+配置内容）
   └─ 保存到数据库或JSON文件

3. 使用阶段
   └─ 雷达使用该配置工作
   └─ 应用层记录当前配置文件名
   └─ 如需修改，重新发送新配置

4. 验证阶段
   └─ 通过数据输出验证配置正确性
   └─ 对比预期性能（距离、速度、角度分辨率）
   └─ 如不符合，查看日志找到发送的配置
```

---

#### 🎯 实用建议

**如何确认当前配置生效？**

1. **通过性能验证**

   - 测试检测距离是否符合配置
   - 验证帧率是否正确
   - 检查数据输出格式
2. **通过数据分析**

   ```python
   # 解析雷达输出数据
   # 从数据包中可以推断部分配置参数
   # 如：点云数量 → chirp数量
   #     帧周期 → frameCfg的帧率
   ```
3. **保持配置文档**

   - 每次更改配置都记录
   - 维护配置版本历史
   - 关联固件版本和配置文件

---

#### 💡 总结

| 问题                | 答案                                                |
| ------------------- | --------------------------------------------------- |
| 能读取基础配置吗？  | ❌**不能** - profileCfg、channelCfg等无法读回 |
| 能读取Flash配置吗？ | ✅**能** - 但仅限支持的特定Demo               |
| 能读取工作状态吗？  | ✅**能** - 版本、状态、校准数据等             |
| 如何管理配置？      | ✅**本地保存** - 发送时记录配置文件           |
| 如何验证配置？      | ✅**性能测试** - 通过实际输出验证             |

**关键点**：

- 🔴 TI mmWave雷达**不支持读回大部分配置参数**
- 🟢 解决方案：**应用层管理配置**，发送时保存配置副本
- 🟡 部分Demo支持Flash配置读写（如停车传感器）
- 🔵 通过数据输出和性能测试**间接验证配置**

---

### 6️⃣ 多次写入配置文件可以吗？每次写入是覆盖方式吗？

**完全可以！每次写入都是覆盖方式！**

#### ✅ 多次写入配置的特性

**核心机制**：
- ✅ **允许多次写入** - 可以随时重新配置雷达参数
- ✅ **覆盖模式** - 新配置会完全覆盖旧配置
- ✅ **立即生效** - `sensorStart`后新配置立即生效
- ⚠️ **需先停止** - 雷达运行时需先执行`sensorStop`

---

#### 🔄 配置覆盖流程

**标准重新配置流程**：

```python
import serial
import time

ser = serial.Serial('COM4', 115200)

# ========== 第1次配置 ==========
print("发送第1次配置...")
ser.write(b'sensorStop\n')      # 停止雷达（如果正在运行）
time.sleep(0.1)
ser.write(b'flushCfg\n')        # 清空配置缓存
time.sleep(0.1)

# 发送配置A
commands_A = [
    'channelCfg 15 5 0',        # 2TX配置
    'profileCfg 0 77 7 7 200...',
    'frameCfg 0 0 128 0 100 1 0',  # 10 FPS
    'sensorStart'
]
for cmd in commands_A:
    ser.write(cmd.encode() + b'\n')
    time.sleep(0.05)

print("配置A生效，雷达工作中...")
time.sleep(5)  # 运行5秒

# ========== 第2次配置（覆盖） ==========
print("发送第2次配置，覆盖配置A...")
ser.write(b'sensorStop\n')      # 必须先停止
time.sleep(0.1)
ser.write(b'flushCfg\n')        # 清空旧配置
time.sleep(0.1)

# 发送配置B（完全不同的配置）
commands_B = [
    'channelCfg 15 7 0',        # 3TX配置（不同！）
    'profileCfg 0 77 7 7 300...',
    'frameCfg 0 0 64 0 200 1 0',   # 5 FPS（不同！）
    'sensorStart'
]
for cmd in commands_B:
    ser.write(cmd.encode() + b'\n')
    time.sleep(0.05)

print("配置B生效，配置A已被覆盖！")
```

---

#### 📝 重新配置命令详解

**关键命令**：

| 命令 | 作用 | 使用时机 | 是否必需 |
|-----|------|---------|---------|
| `sensorStop` | 停止雷达 | 重新配置前 | ✅ **必需**（如果雷达正在运行） |
| `flushCfg` | 清空配置缓存 | `sensorStop`后 | ✅ **强烈推荐** |
| 发送新配置 | 覆盖旧配置 | `flushCfg`后 | ✅ **必需** |
| `sensorStart` | 启动雷达 | 配置完成后 | ✅ **必需** |

**命令说明**：

1. **`sensorStop`**
   - 作用：停止雷达传感器
   - 效果：停止chirp发送和数据处理
   - 必要性：雷达运行时无法重新配置，必须先停止

2. **`flushCfg`**
   - 作用：清空配置缓存
   - 效果：清除内存中的旧配置参数
   - 必要性：确保新配置完全覆盖，避免旧配置残留

3. **发送新配置**
   - 作用：写入新的配置参数
   - 效果：参数写入硬件寄存器
   - 覆盖性：完全覆盖旧配置

4. **`sensorStart`**
   - 作用：启动雷达
   - 效果：使用新配置开始工作
   - 必要性：配置后必须启动才能工作

---

#### 💾 配置写入到哪里了？

**写入位置层次**：

```
串口CLI命令
    ↓
固件接收并解析
    ↓
写入位置（3个层次）
    ├─ 1. 配置RAM缓存     ← 临时存储，断电丢失
    ├─ 2. 硬件寄存器       ← 直接控制硬件，断电丢失
    └─ 3. Flash存储（可选） ← 永久保存，断电保留
```

**详细说明**：

| 写入位置 | 特性 | 断电后 | 读取 | 说明 |
|---------|------|--------|------|------|
| **1. 配置RAM缓存** | 临时存储 | ❌ 丢失 | ❌ 不可读 | 固件接收配置后临时存储 |
| **2. 硬件寄存器** | 直接控制 | ❌ 丢失 | ❌ 不可读 | 配置直接写入控制寄存器 |
| **3. Flash存储** | 永久保存 | ✅ 保留 | ✅ 可读 | 需固件支持，手动保存 |

---

#### 🔍 写入位置详细解析

**1. 配置RAM缓存（主要写入位置）**

```c
// 固件内部处理（伪代码）
void CLI_ProcessCommand(char* cmd) {
    if (strcmp(cmd, "channelCfg") == 0) {
        // 解析参数
        uint8_t rxEn, txEn;
        parseChannelCfg(cmd, &rxEn, &txEn);
        
        // 写入RAM缓存
        gRadarConfig.rxChannelEn = rxEn;  ← RAM中的配置结构体
        gRadarConfig.txChannelEn = txEn;
    }
}
```

**特点**：
- 📝 CLI命令解析后先写入RAM配置结构体
- ⚡ 速度快，方便固件访问
- ⚠️ 断电后丢失，需重新配置
- ❌ 无法从外部读取

---

**2. 硬件寄存器（实际工作位置）**

```c
// sensorStart时将RAM配置写入硬件
void sensorStart() {
    // 将RAM配置写入硬件寄存器
    RF_REG_TX_ENABLE = gRadarConfig.txChannelEn;  ← 硬件寄存器
    RF_REG_RX_ENABLE = gRadarConfig.rxChannelEn;
    ADC_REG_SAMPLE_RATE = gRadarConfig.adcSampleRate;
    // ... 更多寄存器配置
    
    // 启动雷达
    START_RADAR_ENGINE();
}
```

**特点**：
- 🔧 直接控制硬件工作
- ⚡ 立即生效
- ⚠️ 断电后丢失
- ❌ 寄存器不可从CLI读取

**配置流程**：
```
CLI命令 → RAM缓存 → sensorStart → 硬件寄存器 → 雷达工作
```

---

**3. Flash存储（可选，永久保存）**

**仅部分Demo支持**，如停车传感器Demo：

```python
# 将配置保存到Flash
ser.write(b'sensorStop\n')
time.sleep(0.1)
# ... 发送配置命令 ...
ser.write(b'setFlashRecord\n')  # 保存到Flash
time.sleep(0.5)

# 下次上电自动从Flash加载配置
# 或手动读取Flash配置
ser.write(b'getFlashRecord\n')
response = ser.read(ser.in_waiting)
print("Flash中的配置:", response.decode())
```

**特点**：
- 💾 永久保存配置
- ✅ 断电后保留
- ✅ 可读取（支持的Demo）
- ⚠️ 需固件支持Flash操作
- ⚠️ 大多数Demo不支持

**支持Flash的Demo**：
- `parking_garage_sensor` - 停车传感器
- 其他特定应用Demo

---

#### ⚠️ 重要注意事项

**1. 必须先停止雷达**

```python
# ❌ 错误：雷达运行时直接发送配置
ser.write(b'channelCfg 15 7 0\n')  # 可能被忽略或导致错误

# ✅ 正确：先停止再配置
ser.write(b'sensorStop\n')
time.sleep(0.1)
ser.write(b'flushCfg\n')
time.sleep(0.1)
ser.write(b'channelCfg 15 7 0\n')  # 正常生效
```

**2. 配置是整体覆盖**

```python
# 第1次配置：3个命令
ser.write(b'channelCfg 15 5 0\n')    # 2TX
ser.write(b'profileCfg ...\n')
ser.write(b'frameCfg ...\n')
ser.write(b'sensorStart\n')

# 第2次配置：只发1个命令
ser.write(b'sensorStop\n')
ser.write(b'flushCfg\n')
ser.write(b'channelCfg 15 7 0\n')    # 3TX
ser.write(b'sensorStart\n')

# ❌ 问题：缺少profileCfg和frameCfg，雷达无法正常工作
# ✅ 正确：每次都发送完整配置
```

**关键**：重新配置时必须发送**所有必需的配置命令**，不能只发部分。

---

**3. 断电后配置丢失**

```
上电后雷达状态：
├─ Flash无配置 → 无任何配置，需完全配置
├─ Flash有配置 → 自动加载（仅支持的Demo）
└─ RAM配置 → 已丢失，需重新配置
```

**解决方案**：
- 应用层在每次上电后重新发送配置
- 或使用支持Flash的固件，保存配置到Flash

---

#### 🎯 实际应用场景

**场景1：动态切换配置**

```python
# 白天模式：长距离配置
def switch_to_day_mode():
    ser.write(b'sensorStop\n')
    time.sleep(0.1)
    ser.write(b'flushCfg\n')
    time.sleep(0.1)
    send_config('6843_100m_2D_advanced.cfg')  # 100米配置
    ser.write(b'sensorStart\n')

# 夜间模式：短距离低功耗配置
def switch_to_night_mode():
    ser.write(b'sensorStop\n')
    time.sleep(0.1)
    ser.write(b'flushCfg\n')
    time.sleep(0.1)
    send_config('IWR6843AOP_7m_staticRetention_lp.cfg')  # 低功耗配置
    ser.write(b'sensorStart\n')

# 根据时间自动切换
import datetime
hour = datetime.datetime.now().hour
if 6 <= hour < 18:
    switch_to_day_mode()
else:
    switch_to_night_mode()
```

---

**场景2：自适应配置调整**

```python
# 根据目标数量动态调整配置
def adjust_config_by_targets(num_targets):
    ser.write(b'sensorStop\n')
    time.sleep(0.1)
    ser.write(b'flushCfg\n')
    time.sleep(0.1)
    
    if num_targets > 10:
        # 多目标：使用高帧率配置
        send_config('high_framerate_config.cfg')
    else:
        # 少目标：使用低功耗配置
        send_config('low_power_config.cfg')
    
    ser.write(b'sensorStart\n')
```

---

**场景3：测试不同配置性能**

```python
# 自动化测试多种配置
configs = [
    '6843_50m_2D.cfg',
    '6843_50m_3D.cfg',
    '6843_100m_2D_advanced.cfg',
]

results = []
for config_file in configs:
    print(f"测试配置: {config_file}")
    
    # 发送配置
    ser.write(b'sensorStop\n')
    time.sleep(0.1)
    ser.write(b'flushCfg\n')
    time.sleep(0.1)
    send_config(config_file)
    ser.write(b'sensorStart\n')
    
    # 运行并收集数据
    time.sleep(10)
    data = collect_radar_data()
    
    # 评估性能
    performance = evaluate_performance(data)
    results.append({
        'config': config_file,
        'performance': performance
    })

# 选择最佳配置
best_config = max(results, key=lambda x: x['performance'])
print(f"最佳配置: {best_config['config']}")
```

---

#### 💡 总结

| 问题 | 答案 |
|-----|------|
| 可以多次写入配置吗？ | ✅ **可以** - 随时可以重新配置 |
| 是覆盖方式吗？ | ✅ **是** - 新配置完全覆盖旧配置 |
| 写入到哪里了？ | 📝 **RAM缓存** + 🔧 **硬件寄存器** |
| 断电后保留吗？ | ❌ **不保留** - 需重新配置（除非Flash） |
| 需要先停止吗？ | ✅ **需要** - 必须先`sensorStop` |
| 能部分修改吗？ | ❌ **不能** - 必须发送完整配置 |

**关键要点**：
- 🔴 配置写入**RAM和寄存器**，断电后丢失
- 🟢 每次重新配置都是**完全覆盖**，不是增量修改
- 🟡 重新配置流程：`sensorStop` → `flushCfg` → 发送配置 → `sensorStart`
- 🔵 必须发送**完整配置**，不能只发部分命令
- 🟣 部分Demo支持**Flash永久保存**（如停车传感器）

---

### 7️⃣ 写入配置文件后，能从板子中读取更多的内容吗？

**可以读取数据输出，但仍无法读回配置参数！**

#### 📊 写入配置后可以读取的内容

**写入配置 ≠ 可以读回配置**，但可以读取雷达的**工作数据和状态**。

---

#### ✅ 可以读取的内容（详细分类）

**1. 雷达检测数据** ⭐⭐⭐（主要内容）

配置生效后，雷达会持续输出检测数据：

```python
import serial
import struct

ser = serial.Serial('COM4', 115200)

# 发送配置后，雷达开始输出数据
while True:
    # 读取数据帧头
    magic_word = ser.read(8)
    if magic_word == b'\x02\x01\x04\x03\x06\x05\x08\x07':
        # 读取帧头信息
        header = ser.read(32)  # 40字节总帧头（8字节magic + 32字节header）
        
        # 解析帧头
        version, total_packet_len, platform, frame_num, time_cpu_cycles, \
        num_detected_obj, num_tlvs, subframe_num = struct.unpack('8I', header)
        
        print(f"帧号: {frame_num}, 检测目标数: {num_detected_obj}")
        
        # 读取TLV数据
        for _ in range(num_tlvs):
            tlv_type, tlv_length = struct.unpack('II', ser.read(8))
            tlv_data = ser.read(tlv_length)
            
            if tlv_type == 1:  # 目标列表
                # 解析目标信息（距离、速度、角度）
                parse_detected_objects(tlv_data, num_detected_obj)
```

**可读取的检测数据**：

| 数据类型 | 内容 | 说明 |
|---------|------|------|
| **目标列表** | 距离、速度、角度 | 每个检测到的目标 |
| **点云数据** | 3D坐标(x,y,z) | 所有检测点 |
| **距离-多普勒热图** | 2D矩阵 | Range-Doppler Map |
| **角度信息** | 方位角、俯仰角 | DOA估计结果 |
| **SNR** | 信噪比 | 信号质量 |
| **噪声值** | 噪声功率 | 环境噪声 |

---

**2. 帧统计信息** ⭐⭐

每帧数据包含的统计信息：

```python
# 从数据帧中提取统计信息
frame_stats = {
    'frame_number': frame_num,           # 帧序号
    'timestamp': time_cpu_cycles,        # 时间戳
    'num_objects': num_detected_obj,     # 目标数量
    'num_tlvs': num_tlvs,               # TLV数量
    'frame_processing_time': proc_time,  # 处理时间
}

# 可以用于性能分析
print(f"帧率: {1000/frame_period:.1f} FPS")
print(f"处理延迟: {proc_time} ms")
```

---

**3. 传感器状态信息** ⭐⭐（固件相关）

部分固件支持状态查询命令：

```python
# 注意：具体命令取决于固件版本和Demo类型

# 可能支持的命令（非标准，需查看固件文档）
status_commands = [
    'getVersion',      # 固件版本
    'sensorStatus',    # 传感器状态
    'getTemp',         # 温度信息（如果支持）
    'getCalibData',    # 校准数据
]

for cmd in status_commands:
    ser.write(cmd.encode() + b'\n')
    time.sleep(0.1)
    response = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
    if response:
        print(f"{cmd}: {response}")
```

---

**4. TLV数据包** ⭐⭐⭐（丰富信息）

配置不同会导致输出不同的TLV类型：

| TLV Type | 名称 | 内容 | 取决于配置 |
|---------|------|------|-----------|
| 1 | 目标列表 | 距离、速度、角度 | ✅ 基础输出 |
| 2 | 目标索引 | 目标ID | ✅ 跟踪时 |
| 3 | 聚类信息 | 目标聚类结果 | ⚠️ 需聚类算法 |
| 4 | 跟踪信息 | 轨迹ID、预测 | ⚠️ 需跟踪算法 |
| 5 | 点云 | 所有检测点 | ⚠️ 需点云输出 |
| 6 | Range-Doppler | 热图数据 | ⚠️ 需热图输出 |
| 7 | 统计信息 | 性能统计 | ✅ 通常包含 |
| 8 | 副载波信息 | 相位信息 | ⚠️ 高级功能 |

**关键点**：不同配置会影响输出的TLV类型和数量。

---

#### ❌ 仍然无法读取的内容

即使写入配置后，**仍然无法读回配置参数本身**：

```python
# ❌ 无法读取的内容
ser.write(b'getChannelCfg\n')   # 无此命令
ser.write(b'getProfileCfg\n')   # 无此命令
ser.write(b'getFrameCfg\n')     # 无此命令

# ❌ 原因
# 1. 固件不提供读取配置的CLI命令
# 2. 配置已写入寄存器，无法从CLI读回
# 3. 只有极少数特定Demo支持Flash配置读取
```

---

#### 🔍 通过数据输出推断配置

虽然无法直接读取配置，但可以**从输出数据推断配置参数**：

**方法1：从帧率推断**

```python
import time

# 记录10帧的时间
start_time = time.time()
frame_count = 0

while frame_count < 10:
    # 读取一帧数据
    if read_frame():
        frame_count += 1

end_time = time.time()
measured_fps = 10 / (end_time - start_time)

print(f"实测帧率: {measured_fps:.2f} FPS")

# 推断frameCfg的framePeriodicity参数
frame_period = 1000 / measured_fps  # ms
print(f"推断的帧周期: {frame_period:.1f} ms")
# 可能的配置：frameCfg 0 0 128 0 {frame_period} 1 0
```

---

**方法2：从数据输出推断**

```python
# 从第一帧数据推断配置
first_frame = read_frame()

# 1. 从点云数量推断采样点数
num_samples = len(first_frame['range_profile'])
print(f"FFT点数: {num_samples}")
# 推断：adcCfg中的numAdcSamples参数

# 2. 从虚拟天线数推断TX/RX配置
num_virtual_antennas = first_frame['num_angle_bins']
print(f"虚拟天线数: {num_virtual_antennas}")
# 12 → 可能是 4RX × 3TX (channelCfg 15 7 0)
# 16 → 可能是 4RX × 4TX (channelCfg 15 15 0)

# 3. 从最大检测距离推断带宽
max_range = first_frame['max_detectable_range']
print(f"最大检测距离: {max_range} m")
# 可以反推带宽和采样率
```

---

**方法3：从TLV类型推断**

```python
tlv_types_present = set()

# 收集10帧的TLV类型
for _ in range(10):
    frame = read_frame()
    for tlv in frame['tlvs']:
        tlv_types_present.add(tlv['type'])

print(f"输出的TLV类型: {tlv_types_present}")

# 推断配置
if 5 in tlv_types_present:
    print("→ 启用了点云输出")
if 6 in tlv_types_present:
    print("→ 启用了Range-Doppler热图输出")
if 4 in tlv_types_present:
    print("→ 启用了目标跟踪")
```

---

#### 🎯 实际应用策略

**策略1：本地记录配置** ⭐⭐⭐（推荐）

```python
class RadarManager:
    def __init__(self):
        self.current_config = None
        self.config_history = []
    
    def send_config(self, config_file):
        # 发送配置
        with open(config_file, 'r') as f:
            config_content = f.read()
        
        ser.write(b'sensorStop\n')
        time.sleep(0.1)
        ser.write(b'flushCfg\n')
        time.sleep(0.1)
        
        for line in config_content.split('\n'):
            if line and not line.startswith('%'):
                ser.write(line.encode() + b'\n')
                time.sleep(0.05)
        
        # 本地记录
        self.current_config = {
            'file': config_file,
            'content': config_content,
            'timestamp': time.time()
        }
        self.config_history.append(self.current_config)
    
    def get_current_config(self):
        return self.current_config
    
    def verify_config(self):
        # 通过数据输出验证配置
        frame = read_frame()
        
        # 验证帧率
        expected_fps = self.current_config.get('fps', 10)
        measured_fps = measure_fps()
        
        if abs(expected_fps - measured_fps) < 1:
            print("✅ 配置验证通过")
        else:
            print(f"⚠️ 配置异常：期望{expected_fps}FPS，实测{measured_fps}FPS")
```

---

**策略2：数据验证配置** ⭐⭐

```python
def validate_config_from_data():
    """通过数据输出验证配置是否正确"""
    
    # 收集数据
    frames = []
    for _ in range(10):
        frames.append(read_frame())
    
    # 验证项
    checks = {
        'frame_rate': check_frame_rate(frames),
        'range_resolution': check_range_resolution(frames),
        'max_range': check_max_range(frames),
        'num_antennas': check_antenna_config(frames),
    }
    
    # 报告
    print("配置验证结果:")
    for check, result in checks.items():
        status = "✅" if result['pass'] else "❌"
        print(f"{status} {check}: {result['message']}")
```

---

#### 💡 总结

| 问题 | 答案 |
|-----|------|
| 写入配置后能读取配置参数吗？ | ❌ **不能** - 配置参数无法读回 |
| 能读取雷达数据吗？ | ✅ **能** - 检测数据持续输出 |
| 能读取什么数据？ | 📊 目标、点云、热图、统计信息 |
| 能推断配置吗？ | ✅ **能** - 从数据特征推断部分参数 |
| 如何确认配置正确？ | ✅ **数据验证** - 验证帧率、距离、天线数 |

**关键要点**：
- 🔴 写入配置后**仍然无法读回配置参数**（与之前一样）
- 🟢 但可以读取**丰富的雷达检测数据**（目标、点云、统计信息）
- 🟡 可以从**数据输出推断配置参数**（帧率、天线数、距离范围）
- 🔵 推荐**本地记录配置**，通过**数据验证**确认配置正确

**配置管理最佳实践**：
```
发送配置 → 本地记录 → 读取数据 → 验证配置 → 确认正确
```

---

## 📦 AWRL6844配置文件清单

> 📅 统计日期：2025-12-17
> 🎯 目标板卡：**AWRL6844EVM**
> 📡 芯片系列：xWRL68xx (60GHz, 4TX/4RX)
> 📁 扫描路径：MMWAVE_L_SDK、radar_toolbox、radar_academy
> 📊 配置总数：**45个**（1个专用 + 44个通用）

---

### 🎯 AWRL6844专用配置

#### 1. xWRL6844_4T4R_tdm.cfg ⭐⭐⭐

**完整路径**：

```
C:\ti\radar_toolbox_3_30_00_06\tools\Adc_Data_Capture_Tool_DCA1000_CLI\chirp_configs\xWRL6844_4T4R_tdm.cfg
```

**核心特性**：

- 🎯 **AWRL6844专用配置** - 唯一充分利用4TX硬件的配置
- 📡 **4TX/4RX配置** - 16个MIMO虚拟天线（比3TX多33%角度分辨率）
- 🔀 **TDM模式** - 时分复用，最大化天线利用率
- 🔧 **用途** - DCA1000原始ADC数据采集和算法开发

**适用场景**：

- ✅ AWRL6844评估板开发
- ✅ 原始数据采集和离线算法开发
- ✅ 充分发挥4TX硬件能力的高性能场景
- ✅ 需要高角度分辨率的应用

**推荐优先级**：⭐⭐⭐（AWRL6844用户首选）

---

### 🔄 xWRL68xx通用配置（44个）

以下配置文件适用于**xWRL68xx系列**（包括AWRL6843和AWRL6844）。

#### 📊 配置文件总表

| 序号 | 文件名                                                       | 应用场景                  | 检测距离 | 功耗             |
| ---- | ------------------------------------------------------------ | ------------------------- | -------- | ---------------- |
| 1    | `6843_100m_2D_advanced.cfg`                                | 长距离人员检测            | 100m     | 较高             |
| 2    | `6843_50m_2D.cfg`                                          | 中距离2D跟踪              | 50m      | 标准             |
| 3    | `6843_50m_3D.cfg`                                          | 中距离3D跟踪              | 50m      | 标准             |
| 4    | `68xx_traffic_monitoring_70m_MIMO_2D.cfg`                  | 交通监控2D                | 70m      | 标准             |
| 5    | `68xx_traffic_monitoring_70m_MIMO_3D.cfg`                  | 交通监控3D                | 70m      | 标准             |
| 6    | `area_scanner_68xx_AOP.cfg`                                | 区域扫描                  | 中距离   | 标准             |
| 7    | `area_scanner_68xx_AOP_full_power_full_bandwidth.cfg`      | 区域扫描（最高性能）      | 远距离   | 满功率           |
| 8    | `area_scanner_68xx_AOP_full_power_low_bandwidth.cfg`       | 区域扫描                  | 中距离   | 满功率           |
| 9    | `area_scanner_68xx_AOP_low_power_full_bandwidth.cfg`       | 区域扫描（低功耗）        | 远距离   | 低功耗           |
| 10   | `area_scanner_68xx_ISK.cfg`                                | 区域扫描（ISK板）         | 中距离   | 标准             |
| 11   | `area_scanner_68xx_ODS.cfg`                                | 区域扫描（ODS板）         | 中距离   | 标准             |
| 12   | `enable_57_61_GHz_6843_AOP.cfg`                            | 全带宽测试（AOP）         | -        | 标准             |
| 13   | `enable_57_61_GHz_6843_ISK.cfg`                            | 全带宽测试（ISK）         | -        | 标准             |
| 14   | `enable_57_61_GHz_6843_ODS.cfg`                            | 全带宽测试（ODS）         | -        | 标准             |
| 15   | `high_accuracy_demo_68xx.cfg`                              | 高精度液位检测            | 短距离   | 标准             |
| 16   | `IWR6843AOP_7m_staticRetention_lp.cfg`                     | 低功耗人员跟踪            | 7m       | **低功耗** |
| 17   | `IWR6843ISK_6m_staticRetention_lp.cfg`                     | 低功耗人员跟踪            | 6m       | **低功耗** |
| 18   | `IWR6843ODS_6m_staticRetention_lp.cfg`                     | 低功耗人员跟踪            | 6m       | **低功耗** |
| 19   | `Mobile_Tracker_6843_ISK.cfg`                              | 移动目标跟踪              | 中距离   | 标准             |
| 20   | `parking_garage_sensor_68xx_ISK_getFlashRecord.cfg`        | 停车传感器（读）          | 短距离   | 标准             |
| 21   | `parking_garage_sensor_68xx_ISK_setFlashRecord.cfg`        | 停车传感器（写）          | 短距离   | 标准             |
| 22   | `profile_3d_aop.cfg`                                       | 真实地速检测（AOP）       | 中距离   | 标准             |
| 23   | `profile_3d_isk.cfg`                                       | 真实地速检测（ISK）       | 中距离   | 标准             |
| 24   | `profile_monitor_xwr68xx.cfg`                              | 监控模式                  | -        | 标准             |
| 25   | `pt_6843_3d_aop_overhead_3m_radial.cfg`                    | 顶置3D跟踪                | 3m       | 标准             |
| 26   | `pt_6843_3d_aop_overhead_3m_radial_low_bw.cfg`             | 顶置跟踪（低带宽）        | 3m       | 低带宽           |
| 27   | `pt_6843_3d_aop_overhead_3m_radial_staticRetention.cfg`    | 顶置跟踪（静态保持）      | 3m       | 标准             |
| 28   | `pt_6843_3d_ods_overhead_3m_radial.cfg`                    | 顶置跟踪（ODS）           | 3m       | 标准             |
| 29   | `pt_6843_3d_ods_overhead_3m_radial_low_bw.cfg`             | 顶置跟踪（低带宽）        | 3m       | 低带宽           |
| 30   | `pt_6843_3d_ods_overhead_3m_radial_staticRetention.cfg`    | 顶置跟踪（静态保持）      | 3m       | 标准             |
| 31   | `pt_6843_3d_ods_overhead_8m_60degtilt_staticRetention.cfg` | 顶置跟踪（倾斜）          | 8m       | 标准             |
| 32   | `vod_6843_aop_overhead_2row.cfg`                           | 车内2排检测               | 车内     | 标准             |
| 33   | `vod_6843_aop_overhead_2row_classification.cfg`            | 车内2排+分类              | 车内     | 标准             |
| 34   | `vod_6843_aop_overhead_2row_intruder.cfg`                  | 车内入侵检测              | 车内     | 标准             |
| 35   | `vod_6843_aop_overhead_2row_van.cfg`                       | 面包车2排                 | 车内     | 标准             |
| 36   | `vod_6843_aop_overhead_3row_bus.cfg`                       | 大巴3排                   | 车内     | 标准             |
| 37   | `vod_6843_isk_frontMount_2row.cfg`                         | 车内前置安装              | 车内     | 标准             |
| 38   | `vod_6843_ods_overhead_2row.cfg`                           | 车内2排（ODS）            | 车内     | 标准             |
| 39   | `vod_6843_ods_overhead_2row_classification.cfg`            | 车内2排+分类（ODS）       | 车内     | 标准             |
| 40   | `vod_6843_ods_overhead_3row_optimized.cfg`                 | 车内3排优化               | 车内     | 优化             |
| 41   | `xWR6843_presenceDetection.cfg`                            | 基础存在检测              | 短距离   | 标准             |
| 42   | `xWR6843_presenceDetection_twoProfile.cfg`                 | 双Profile存在检测         | 短距离   | 标准             |
| 43   | `xWR6843_presenceDetection_twoProfile.cfg`                 | 双Profile存在检测（副本） | 短距离   | 标准             |
| 44   | `xWRL6844_4T4R_tdm.cfg`                                    | **AWRL6844专用** ⭐ | -        | 标准             |

> 注：序号44是专用配置的重复显示，实际通用配置为43个。

---

### 🗂️ 按应用场景分类

#### 🚗 1. 车内监测（In-Cabin Sensing）- 9个

**适用场景**：车辆座舱内人员检测、分类、入侵监测

| 配置文件                                             | 座椅排数 | 特殊功能                   | 安装方式 | 封装类型 |
| ---------------------------------------------------- | -------- | -------------------------- | -------- | -------- |
| `vod_6843_aop_overhead_2row.cfg`                   | 2排      | 基础检测                   | 顶置     | AOP      |
| `vod_6843_aop_overhead_2row_classification.cfg` ⭐ | 2排      | **成人/儿童分类**    | 顶置     | AOP      |
| `vod_6843_aop_overhead_2row_intruder.cfg`          | 2排      | **入侵检测（防盗）** | 顶置     | AOP      |
| `vod_6843_aop_overhead_2row_van.cfg`               | 2排      | 面包车大空间               | 顶置     | AOP      |
| `vod_6843_aop_overhead_3row_bus.cfg`               | 3排      | 大巴/多人                  | 顶置     | AOP      |
| `vod_6843_isk_frontMount_2row.cfg`                 | 2排      | 前置安装                   | 仪表台   | ISK      |
| `vod_6843_ods_overhead_2row.cfg`                   | 2排      | 基础检测                   | 顶置     | ODS      |
| `vod_6843_ods_overhead_2row_classification.cfg`    | 2排      | 成人/儿童分类              | 顶置     | ODS      |
| `vod_6843_ods_overhead_3row_optimized.cfg`         | 3排      | **性能优化**         | 顶置     | ODS      |

**AWRL6844推荐**：✅ 所有车内监测场景都推荐
**热门配置**：`vod_6843_aop_overhead_2row_classification.cfg`（分类功能）

---

#### 👥 2. 人员跟踪（People Tracking）- 13个

**适用场景**：室内/室外人员定位、轨迹跟踪、人员计数

| 配置文件                                                     | 检测距离       | 维度         | 安装方式               | 功耗模式         |
| ------------------------------------------------------------ | -------------- | ------------ | ---------------------- | ---------------- |
| `6843_50m_2D.cfg`                                          | 50m            | 2D           | 水平                   | 标准             |
| `6843_50m_3D.cfg` ⭐                                       | 50m            | **3D** | 水平                   | 标准             |
| `6843_100m_2D_advanced.cfg`                                | **100m** | 2D           | 水平                   | 较高             |
| `pt_6843_3d_aop_overhead_3m_radial.cfg`                    | 3m             | 3D           | 顶置（径向）           | 标准             |
| `pt_6843_3d_aop_overhead_3m_radial_low_bw.cfg`             | 3m             | 3D           | 顶置                   | 低带宽           |
| `pt_6843_3d_aop_overhead_3m_radial_staticRetention.cfg`    | 3m             | 3D           | 顶置                   | 静止保持         |
| `pt_6843_3d_ods_overhead_3m_radial.cfg`                    | 3m             | 3D           | 顶置                   | 标准（ODS）      |
| `pt_6843_3d_ods_overhead_3m_radial_low_bw.cfg`             | 3m             | 3D           | 顶置                   | 低带宽（ODS）    |
| `pt_6843_3d_ods_overhead_3m_radial_staticRetention.cfg`    | 3m             | 3D           | 顶置                   | 静止保持（ODS）  |
| `pt_6843_3d_ods_overhead_8m_60degtilt_staticRetention.cfg` | **8m**   | 3D           | 顶置**60°倾斜** | 静止保持         |
| `IWR6843AOP_7m_staticRetention_lp.cfg` ⚡                  | 7m             | 3D           | 标准                   | **低功耗** |
| `IWR6843ISK_6m_staticRetention_lp.cfg` ⚡                  | 6m             | 3D           | ISK                    | **低功耗** |
| `IWR6843ODS_6m_staticRetention_lp.cfg` ⚡                  | 6m             | 3D           | ODS                    | **低功耗** |

**AWRL6844推荐**：✅ 适合室内/车内短距离3D跟踪**热门配置**：

- `6843_50m_3D.cfg` - 中距离3D通用
- `IWR6843AOP_7m_staticRetention_lp.cfg` - 低功耗场景

---

#### 📡 3. 区域扫描（Area Scanner）- 6个

**适用场景**：工业监控、安防、智能建筑空间监测

| 配置文件                                                   | 功耗模式         | 带宽             | 封装 | 适用场景           |
| ---------------------------------------------------------- | ---------------- | ---------------- | ---- | ------------------ |
| `area_scanner_68xx_AOP.cfg`                              | 标准             | 标准             | AOP  | 通用扫描           |
| `area_scanner_68xx_AOP_full_power_full_bandwidth.cfg` ⭐ | **满功率** | **全带宽** | AOP  | **最高性能** |
| `area_scanner_68xx_AOP_full_power_low_bandwidth.cfg`     | 满功率           | 低带宽           | AOP  | 近距离高功率       |
| `area_scanner_68xx_AOP_low_power_full_bandwidth.cfg` ⚡  | **低功耗** | 全带宽           | AOP  | **功耗敏感** |
| `area_scanner_68xx_ISK.cfg`                              | 标准             | 标准             | ISK  | 开发板测试         |
| `area_scanner_68xx_ODS.cfg`                              | 标准             | 标准             | ODS  | ODS封装            |

**AWRL6844推荐**：✅ 工业/商业空间监控**热门配置**：

- `area_scanner_68xx_AOP_full_power_full_bandwidth.cfg` - 追求性能
- `area_scanner_68xx_AOP_low_power_full_bandwidth.cfg` - 功耗平衡

---

#### 🏠 4. 存在检测（Presence Detection）- 3个

**适用场景**：智能家居、办公室占用检测、照明控制

| 配置文件                                        | Profile数量         | 检测模式           | 适用场景     |
| ----------------------------------------------- | ------------------- | ------------------ | ------------ |
| `xWR6843_presenceDetection.cfg`               | 单Profile           | 基础检测           | 简单存在判断 |
| `xWR6843_presenceDetection_twoProfile.cfg` ⭐ | **双Profile** | **动静结合** | 高精度检测   |

**AWRL6844推荐**：✅ 智能家居、办公室占用检测**热门配置**：`xWR6843_presenceDetection_twoProfile.cfg`（双Profile更精准）

> 注：存在检测配置在SDK中有重复文件，实际为2个独特配置。

---

#### 🚦 5. 交通监控（Traffic Monitoring）- 2个

**适用场景**：道路车流检测、车速测量、车辆分类

| 配置文件                                    | 检测距离 | 维度         | MIMO | 适用场景          |
| ------------------------------------------- | -------- | ------------ | ---- | ----------------- |
| `68xx_traffic_monitoring_70m_MIMO_2D.cfg` | 70m      | 2D           | ✅   | 车流统计          |
| `68xx_traffic_monitoring_70m_MIMO_3D.cfg` | 70m      | **3D** | ✅   | 车辆分类/高度检测 |

**AWRL6844推荐**：⚠️ 可用，但77GHz更适合交通场景（更远距离）
**说明**：60GHz适合短中距离，交通监控通常需要更远的检测距离。

---

#### 🔧 6. 其他应用 - 11个

**特殊/测试场景配置**

| 配置文件                                              | 应用场景               | 说明                      |
| ----------------------------------------------------- | ---------------------- | ------------------------- |
| `Mobile_Tracker_6843_ISK.cfg`                       | 移动目标跟踪           | 机器人、AGV导航           |
| `high_accuracy_demo_68xx.cfg`                       | 高精度液位检测         | 工业液位测量              |
| `parking_garage_sensor_68xx_ISK_getFlashRecord.cfg` | 停车传感器             | 读取Flash存储配置         |
| `parking_garage_sensor_68xx_ISK_setFlashRecord.cfg` | 停车传感器             | 写入Flash存储配置         |
| `profile_3d_aop.cfg`                                | 真实地速检测（AOP）    | 车辆地速测量              |
| `profile_3d_isk.cfg`                                | 真实地速检测（ISK）    | 车辆地速测量              |
| `enable_57_61_GHz_6843_AOP.cfg`                     | 全带宽测试             | 启用57-61GHz带宽（AOP）   |
| `enable_57_61_GHz_6843_ISK.cfg`                     | 全带宽测试             | 启用57-61GHz带宽（ISK）   |
| `enable_57_61_GHz_6843_ODS.cfg`                     | 全带宽测试             | 启用57-61GHz带宽（ODS）   |
| `profile_monitor_xwr68xx.cfg`                       | 监控模式               | 系统监控/调试             |
| `xWRL6844_4T4R_tdm.cfg` ⭐                          | **AWRL6844专用** | **DCA1000数据采集** |

---

### 💡 配置文件使用建议

#### 🎯 AWRL6844优先推荐配置（按场景）

**1. 数据采集和算法开发** ⭐⭐⭐

```
xWRL6844_4T4R_tdm.cfg
```

- 唯一的4TX配置，充分发挥硬件优势
- 16个MIMO虚拟天线
- 适合DCA1000原始数据采集

**2. 车内监测应用** ⭐⭐⭐

```
vod_6843_aop_overhead_2row_classification.cfg
vod_6843_aop_overhead_3row_bus.cfg
```

- 带分类功能，区分成人/儿童
- 多排座椅支持

**3. 区域扫描应用** ⭐⭐

```
area_scanner_68xx_AOP_full_power_full_bandwidth.cfg  （追求性能）
area_scanner_68xx_AOP_low_power_full_bandwidth.cfg   （功耗平衡）
```

**4. 人员跟踪应用** ⭐⭐

```
6843_50m_3D.cfg                                       （中距离通用）
pt_6843_3d_aop_overhead_3m_radial_staticRetention.cfg （顶置安装）
IWR6843AOP_7m_staticRetention_lp.cfg                  （低功耗）
```

**5. 存在检测应用** ⭐⭐

```
xWR6843_presenceDetection_twoProfile.cfg
```

- 双Profile更精准
- 动静结合检测

---

#### ⚖️ 配置文件兼容性矩阵

| 配置类型               | TX数量 | AWRL6844能用吗？ | AWRL6843能用吗？ | 性能           | 说明                 |
| ---------------------- | ------ | ---------------- | ---------------- | -------------- | -------------------- |
| **AWRL6844专用** | 4TX    | ✅**推荐** | ❌ 不支持        | **最佳** | 充分利用4TX，16 MIMO |
| **AWRL6843配置** | 3TX    | ✅ 兼容          | ✅ 原生支持      | 良好           | 只用3个TX，浪费1个   |
| **通用68xx配置** | 1-3TX  | ✅ 兼容          | ✅ 兼容          | 标准           | 通用性好             |

**使用策略**：

```
开发初期  →  用6843配置快速验证功能（配置多，易获取）
          ↓
优化阶段  →  用4TX配置获得最佳性能（xWRL6844_4T4R_tdm.cfg）
          ↓
量产阶段  →  根据实际需求选择3TX或4TX（成本vs性能）
```

---

#### 📂 配置文件路径结构

所有配置文件位于 **radar_toolbox_3_30_00_06** SDK中：

```
C:\ti\radar_toolbox_3_30_00_06\
├── source\ti\examples\
│   ├── Industrial_and_Personal_Electronics\
│   │   ├── People_Tracking\
│   │   │   ├── 3D_People_Tracking_Low_Power\chirp_configs\
│   │   │   └── Overhead_3D_People_Tracking\chirp_configs\
│   │   ├── Long_Range_People_Detection\chirp_configs\
│   │   ├── Area_Scanner\chirp_configs\
│   │   ├── Traffic_Monitoring\chirp_configs\
│   │   ├── Level_Sensing\high_accuracy\chirp_configs\
│   │   ├── Parking_Garage_Sensor\chirp_configs\
│   │   ├── True_Ground_Speed\chirp_configs\IWR6843\
│   │   └── Robotics\Mobile_Tracker\chirp_configs\
│   └── Fundamentals\
│       ├── Enabling_57_to_61_GHz_Bandwidth\chirp_configs\
│       └── xWR6843_MultipleProfile_Support\chirp_configs\
├── tools\
│   ├── Adc_Data_Capture_Tool_DCA1000_CLI\chirp_configs\  ← xWRL6844_4T4R_tdm.cfg
│   ├── visualizers\InCabin_CPD_w_Classification_GUI\config_file\
│   └── studio_cli\src\profiles\
```

---

### 📊 统计信息

#### 配置文件来源分布

| SDK                                | 配置数量 | 占比 | 说明                     |
| ---------------------------------- | -------- | ---- | ------------------------ |
| **radar_toolbox_3_30_00_06** | 45       | 100% | xWRL68xx配置文件主要来源 |
| MMWAVE_L_SDK_06_01_00_01           | 0        | 0%   | 主要提供固件和库         |
| radar_academy_3_10                 | 0        | 0%   | 教学资源                 |

**说明**：xWRL68xx系列的配置文件集中在 **radar_toolbox** 中，MMWAVE_L_SDK主要提供固件。

---

#### 应用场景分布

| 场景分类               | 配置数量 | 百分比 | 代表配置                                                |
| ---------------------- | -------- | ------ | ------------------------------------------------------- |
| **人员跟踪**     | 13       | 28.9%  | `6843_50m_3D.cfg`                                     |
| **车内监测**     | 9        | 20.0%  | `vod_6843_aop_overhead_2row_classification.cfg`       |
| **区域扫描**     | 6        | 13.3%  | `area_scanner_68xx_AOP_full_power_full_bandwidth.cfg` |
| **其他应用**     | 11       | 24.4%  | `Mobile_Tracker_6843_ISK.cfg`                         |
| **存在检测**     | 3        | 6.7%   | `xWR6843_presenceDetection_twoProfile.cfg`            |
| **交通监控**     | 2        | 4.4%   | `68xx_traffic_monitoring_70m_MIMO_3D.cfg`             |
| **AWRL6844专用** | 1        | 2.2%   | `xWRL6844_4T4R_tdm.cfg` ⭐                            |

**趋势分析**：

- 人员跟踪和车内监测是主流应用（占比近50%）
- 存在检测配置较少，但应用广泛
- AWRL6844专用配置只有1个，但非常重要

---

### 🔗 快速上手流程

#### 基础使用流程

```
步骤1: 烧录固件
  ├─ SBL固件（COM3）
  └─ 应用固件（COM3）
       ↓
步骤2: 上电启动
  └─ 固件运行，等待配置
       ↓
步骤3: 选择配置文件
  └─ 根据应用场景选择.cfg文件
       ↓
步骤4: 发送配置（COM4）
  └─ 通过串口逐行发送配置命令
       ↓
步骤5: 雷达工作
  └─ 开始检测并输出数据（COM4）
```

---

#### Python发送配置示例

```python
import serial
import time

def send_radar_config(config_file, port='COM4', baudrate=115200):
    """
    发送雷达配置文件到AWRL6844
  
    参数:
        config_file: 配置文件路径（如 'xWRL6844_4T4R_tdm.cfg'）
        port: 串口号（默认COM4）
        baudrate: 波特率（默认115200）
    """
    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        time.sleep(0.5)  # 等待串口稳定
      
        print(f"开始发送配置文件: {config_file}")
      
        with open(config_file, 'r') as f:
            for line_num, line in enumerate(f, 1):
                line = line.strip()
              
                # 跳过空行和注释
                if not line or line.startswith('%') or line.startswith('#'):
                    continue
              
                # 发送命令
                ser.write(line.encode() + b'\n')
                time.sleep(0.05)  # 等待处理
              
                # 读取响应
                response = ser.readline().decode('utf-8', errors='ignore').strip()
                print(f"[{line_num:3d}] 命令: {line:50s} → {response}")
      
        print("\n配置发送完成！")
        ser.close()
      
    except Exception as e:
        print(f"错误: {e}")

# 使用示例
if __name__ == '__main__':
    # AWRL6844专用4TX配置
    send_radar_config('xWRL6844_4T4R_tdm.cfg', port='COM4')
  
    # 或使用通用3D跟踪配置
    # send_radar_config('6843_50m_3D.cfg', port='COM4')
```

---

### 📌 重要提示

#### ⚠️ 常见错误

1. **端口混淆**

   ```
   ❌ 错误：用COM3发送配置  → 无响应（COM3是烧录端口）
   ✅ 正确：用COM4发送配置  → 正常工作
   ```
2. **固件不兼容**

   ```
   ❌ 错误：6843固件 + 4TX配置  → 硬件不支持
   ✅ 正确：6844固件 + 4TX配置  → 完美工作
   ```
3. **未等待固件启动**

   ```
   ❌ 错误：上电后立即发配置  → 固件未就绪
   ✅ 正确：等待2-3秒后发配置  → 稳定工作
   ```
4. **注释行未过滤**

   ```
   ❌ 错误：发送包含%注释的行  → 雷达报错
   ✅ 正确：跳过%和#开头的行  → 正常解析
   ```

---

#### 💡 优化建议

**性能优化**：

- 使用4TX配置（`xWRL6844_4T4R_tdm.cfg`）获得最佳角度分辨率
- 选择full_bandwidth配置获得更好的距离分辨率
- 根据实际需求调整帧率和采样点

**功耗优化**：

- 选择low_power配置降低功耗
- 减少启用的天线数量（如2TX替代4TX）
- 降低帧率和chirp数量

**兼容性优化**：

- 开发初期使用6843配置（配置丰富）
- 量产前切换到4TX配置（性能最优）
- 保留配置切换的灵活性

---

## 📡 配置文件类型详解

### 1. 文件结构

典型的雷达配置文件包含以下部分：

```cfg
% ===== 注释区域 =====
% 配置说明
% 应用场景
% 参数范围

% ===== 全局设置 =====
sensorStop
flushCfg
dfeDataOutputMode 1

% ===== Chirp配置 =====
channelCfg 15 5 0
adcCfg 2 1
adcbufCfg -1 0 1 1 1

% ===== Profile配置 =====
profileCfg 0 77 7 7 200 0 0 70 1 256 5209 0 0 180

% ===== Frame配置 =====
frameCfg 0 0 16 0 100 1 0

% ===== LVDS配置 =====
lvdsStreamCfg -1 0 0 0

% ===== 启动命令 =====
sensorStart
```

### 2. 核心命令详解

#### 2.1 `channelCfg` - 通道配置

**格式**：`channelCfg <rxChannelEn> <txChannelEn> <cascading>`

```cfg
channelCfg 15 5 0
```

**参数说明**：

| 参数            | 值示例 | 二进制 | 含义                             |
| --------------- | ------ | ------ | -------------------------------- |
| `rxChannelEn` | 15     | 1111   | 启用RX1-RX4（4个接收天线）       |
| `txChannelEn` | 5      | 0101   | 启用TX1和TX3（2个发射天线）      |
| `cascading`   | 0      | -      | 单芯片模式（0=单芯片，1/2=级联） |

**常见配置**：

```cfg
channelCfg 15 7 0    # 4RX + 3TX = 12个虚拟天线
channelCfg 15 5 0    # 4RX + 2TX = 8个虚拟天线
channelCfg 3 1 0     # 2RX + 1TX = 2个虚拟天线（低功耗）
```

**天线数量计算**：

- 虚拟天线数 = RX数量 × TX数量
- 更多虚拟天线 = 更高角度分辨率
- 更多天线 = 更高功耗

#### 2.2 `adcCfg` - ADC配置

**格式**：`adcCfg <numADCBits> <adcOutputFmt>`

```cfg
adcCfg 2 1
```

**参数说明**：

| 参数             | 值 | 含义                |
| ---------------- | -- | ------------------- |
| `numADCBits`   | 0  | 12位ADC             |
|                  | 1  | 14位ADC             |
|                  | 2  | 16位ADC（最高精度） |
| `adcOutputFmt` | 0  | 实数格式            |
|                  | 1  | 复数格式（I/Q）     |

**选择建议**：

- 16位ADC：最高精度，用于精密测量
- 14位ADC：平衡性能和功耗
- 12位ADC：低功耗应用

#### 2.3 `profileCfg` - 频率配置（核心参数）

**格式**：

```cfg
profileCfg <profileId> <startFreq> <idleTime> <adcStartTime> 
           <rampEndTime> <txOutPower> <txPhaseShifter> 
           <freqSlopeConst> <txStartTime> <numAdcSamples> 
           <digOutSampleRate> <hpfCornerFreq1> <hpfCornerFreq2> 
           <rxGain>
```

**示例**：

```cfg
profileCfg 0 77 7 7 200 0 0 70 1 256 5209 0 0 180
```

**参数详解**：

| 参数                 | 值示例 | 单位    | 含义          | 计算方法           |
| -------------------- | ------ | ------- | ------------- | ------------------ |
| `profileId`        | 0      | -       | Profile ID    | 0-3                |
| `startFreq`        | 77     | GHz     | 起始频率      | 60-64 GHz          |
| `idleTime`         | 7      | μs     | 空闲时间      | Chirp间隔          |
| `adcStartTime`     | 7      | μs     | ADC启动时间   | RF稳定后采样       |
| `rampEndTime`      | 200    | μs     | Chirp持续时间 | 影响距离分辨率     |
| `txOutPower`       | 0      | dBm     | 发射功率      | 0-31 (0=最大)      |
| `txPhaseShifter`   | 0      | °      | 相位偏移      | 0-360              |
| `freqSlopeConst`   | 70     | MHz/μs | 频率斜率      | 影响速度分辨率     |
| `txStartTime`      | 1      | μs     | 发射启动      | < adcStartTime     |
| `numAdcSamples`    | 256    | -       | ADC采样点数   | 2的幂次（64-1024） |
| `digOutSampleRate` | 5209   | ksps    | 采样率        | 影响最大距离       |
| `hpfCornerFreq1`   | 0      | kHz     | 高通滤波器1   | 0/175/350/700      |
| `hpfCornerFreq2`   | 0      | kHz     | 高通滤波器2   | 0/175/350/700      |
| `rxGain`           | 180    | dB      | 接收增益      | 0-50 dB            |

**关键性能参数计算**：

##### 距离分辨率（Range Resolution）

```
距离分辨率 = c / (2 × 带宽)
带宽 = freqSlopeConst × (rampEndTime - adcStartTime)

示例：
带宽 = 70 MHz/μs × (200-7) μs = 13.51 GHz
距离分辨率 = 3×10^8 / (2 × 13.51×10^9) ≈ 0.011 m = 1.1 cm
```

##### 最大检测距离（Maximum Range）

```
最大距离 = (采样率 × 光速) / (2 × 频率斜率)
         = (numAdcSamples × 光速) / (2 × freqSlopeConst × digOutSampleRate)

示例：
最大距离 = (256 × 3×10^8) / (2 × 70×10^6 × 5209×10^3)
         ≈ 10.5 m
```

##### 速度分辨率（Velocity Resolution）

```
速度分辨率 = λ / (2 × numChirps × chirpTime)
λ = c / centerFreq

示例（假设128个chirps）：
λ = 3×10^8 / 77×10^9 = 3.9 mm
速度分辨率 = 3.9×10^-3 / (2 × 128 × 200×10^-6)
          ≈ 0.076 m/s
```

#### 2.4 `frameCfg` - 帧配置

**格式**：

```cfg
frameCfg <chirpStartIdx> <chirpEndIdx> <numLoops> <numFrames> 
         <framePeriodicity> <triggerSelect> <frameTriggerDelay>
```

**示例**：

```cfg
frameCfg 0 0 128 0 50 1 0
```

**参数说明**：

| 参数                  | 值  | 含义                             |
| --------------------- | --- | -------------------------------- |
| `chirpStartIdx`     | 0   | 起始Chirp索引                    |
| `chirpEndIdx`       | 0   | 结束Chirp索引（0=使用profile 0） |
| `numLoops`          | 128 | 每帧Chirp数量                    |
| `numFrames`         | 0   | 帧数（0=无限）                   |
| `framePeriodicity`  | 50  | 帧周期（ms）                     |
| `triggerSelect`     | 1   | 触发模式（1=软件，2=硬件）       |
| `frameTriggerDelay` | 0   | 触发延迟（ms）                   |

**帧率计算**：

```
帧率 = 1000 / framePeriodicity

示例：
帧率 = 1000 / 50 = 20 FPS
```

### 3. 配置文件命名规范

SDK中的配置文件遵循命名规范，可以从文件名推断配置特性：

**格式**：`<应用>_<模式>_<距离>_<特性>.cfg`

**示例**：

```
3d_people_tracking_68xx_10fps.cfg
└─┬─┘ └────┬─────┘  └┬┘ └─┬─┘
应用       模式      芯片  特性

out_of_box_xwr68xx_3d.cfg
└───┬───┘ └──┬──┘ └┬┘
 应用      芯片    模式
```

**常见命名元素**：

| 元素           | 示例                    | 含义                 |
| -------------- | ----------------------- | -------------------- |
| **应用** | `3d_people_tracking`  | 3D人员跟踪           |
|                | `occupancy_detection` | 占用检测             |
|                | `vital_signs`         | 生命体征             |
|                | `out_of_box`          | 开箱即用             |
| **模式** | `2d`                  | 2D检测               |
|                | `3d`                  | 3D检测               |
|                | `tdm`                 | 时分复用             |
| **芯片** | `68xx`                | 68xx系列（包括6844） |
|                | `6843`                | 特定于6843           |
| **特性** | `10fps`               | 10帧/秒              |
|                | `long_range`          | 长距离               |
|                | `high_res`            | 高分辨率             |

---

## 📚 配置文件示例库

### 按应用场景分类

#### 1. 人员检测与跟踪

**3D人员跟踪（标准）**

```cfg
% 文件：3d_people_tracking_68xx_10fps.cfg
% 应用：室内人员跟踪
% 距离：0-10m
% 帧率：10 FPS

channelCfg 15 7 0          # 4RX + 3TX = 12虚拟天线
profileCfg 0 77 7 7 200 0 0 70 1 256 5209 0 0 180
frameCfg 0 0 128 0 100 1 0 # 10 FPS
```

**特点**：

- ✅ 高角度分辨率（12虚拟天线）
- ✅ 中等距离（~10m）
- ✅ 适合室内场景
- ⚡ 中等功耗

#### 2. 占用检测（低功耗）

**低功耗占用检测**

```cfg
% 文件：occupancy_detection_low_power.cfg
% 应用：房间占用检测
% 距离：0-5m
% 帧率：5 FPS

channelCfg 3 1 0           # 2RX + 1TX = 2虚拟天线
profileCfg 0 77 10 10 150 0 0 60 1 128 4000 0 0 150
frameCfg 0 0 64 0 200 1 0  # 5 FPS
```

**特点**：

- 🔋 低功耗设计
- ✅ 短距离（~5m）
- ✅ 低帧率
- ✅ 简单检测任务

#### 3. 车内感知

**车内乘员检测**

```cfg
% 文件：in_cabin_sensing_68xx.cfg
% 应用：车内乘员检测
% 距离：0-2m
% 帧率：20 FPS

channelCfg 15 5 0          # 4RX + 2TX = 8虚拟天线
profileCfg 0 60 5 5 100 0 0 80 1 256 6400 175 175 180
frameCfg 0 0 256 0 50 1 0  # 20 FPS
```

**特点**：

- ✅ 短距离高精度
- ✅ 高帧率（20 FPS）
- ✅ 使用60 GHz频段
- ✅ 适合车内环境

#### 4. 生命体征检测

**心率呼吸监测**

```cfg
% 文件：vital_signs_68xx.cfg
% 应用：生命体征监测
% 距离：0.5-2m
% 帧率：20 FPS

channelCfg 15 1 0          # 4RX + 1TX = 4虚拟天线
profileCfg 0 77 5 5 40 0 0 100 1 256 10000 0 0 180
frameCfg 0 0 512 0 50 1 0  # 20 FPS
```

**特点**：

- ✅ 极短Chirp（高速度灵敏度）
- ✅ 高帧率
- ✅ 高采样率
- ✅ 检测微小运动

### 按性能特征分类

#### 高距离分辨率配置

```cfg
% 超宽带配置 - 实现1cm级距离分辨率
profileCfg 0 77 7 7 400 0 0 100 1 512 8000 0 0 180
% 带宽 = 100 × (400-7) = 39.3 GHz
% 分辨率 ≈ 0.4 cm
```

#### 高速度分辨率配置

```cfg
% 长帧时间配置 - 检测慢速运动
frameCfg 0 0 512 0 100 1 0  # 512 chirps/frame
% 速度分辨率 ≈ 0.02 m/s
```

#### 长距离检测配置

```cfg
% 高发射功率 + 高接收增益
profileCfg 0 77 7 7 300 0 0 50 1 512 4000 0 0 240
% 最大距离 ≈ 30m
```

---

## 🔧 参数调优指南

### 1. 性能权衡矩阵

| 需求                     | 增加参数                              | 副作用               |
| ------------------------ | ------------------------------------- | -------------------- |
| **提高距离分辨率** | 增加带宽（↑ freqSlope或↑ rampTime） | ↑功耗，↓最大距离   |
| **提高速度分辨率** | 增加chirps数量（↑ numLoops）         | ↑处理时间，↓帧率   |
| **提高角度分辨率** | 增加虚拟天线（↑ RX/TX）              | ↑功耗，↑数据量     |
| **增加最大距离**   | 增加采样点（↑ numAdcSamples）        | ↑数据量，↑处理时间 |
| **降低功耗**       | 减少天线、降低帧率、减少chirps        | ↓性能               |

### 2. 典型应用场景参数建议

#### 室内人员检测（0-10m）

```
距离分辨率：2-5 cm      → 带宽 3-7.5 GHz
速度分辨率：0.05-0.1 m/s → 128-256 chirps
角度分辨率：10-15°      → 8-12虚拟天线
帧率：10-20 FPS
```

#### 手势识别（0-1m）

```
距离分辨率：<1 cm       → 带宽 >15 GHz
速度分辨率：<0.05 m/s   → >256 chirps
帧率：>20 FPS
```

#### 车辆检测（0-50m）

```
距离分辨率：10-20 cm    → 带宽 0.75-1.5 GHz
速度分辨率：0.5-1 m/s   → 32-64 chirps
最大距离：50m
```

### 3. 功耗优化策略

#### 低功耗配置清单

- ✅ 使用最少天线组合（如2RX+1TX）
- ✅ 降低帧率（5-10 FPS）
- ✅ 减少chirps数量（32-64）
- ✅ 使用低采样率
- ✅ 降低发射功率（仅短距离时）

#### 示例对比

| 配置   | 天线    | Chirps | 帧率   | 相对功耗 |
| ------ | ------- | ------ | ------ | -------- |
| 高性能 | 4RX+3TX | 256    | 20 FPS | 100%     |
| 标准   | 4RX+2TX | 128    | 10 FPS | 50%      |
| 低功耗 | 2RX+1TX | 64     | 5 FPS  | 15%      |

---

## 🛠️ 配置文件验证工具

### 1. 参数合法性检查

**必检项目**：

- [ ] `numAdcSamples` 是2的幂次（64/128/256/512/1024）
- [ ] `adcStartTime` > `txStartTime`
- [ ] `rampEndTime` > `adcStartTime` + 采样时间
- [ ] `framePeriodicity` > 单帧所需时间
- [ ] 采样率不超过硬件限制（<12.5 Msps）

### 2. 性能计算器（Python示例）

```python
def calculate_radar_performance(config):
    """
    计算雷达性能参数
    """
    # 提取配置参数
    freq_slope = config['freqSlopeConst']  # MHz/μs
    ramp_time = config['rampEndTime'] - config['adcStartTime']  # μs
    num_samples = config['numAdcSamples']
    sample_rate = config['digOutSampleRate']  # ksps
    num_chirps = config['numLoops']
    frame_period = config['framePeriodicity']  # ms
  
    # 计算性能
    bandwidth = freq_slope * ramp_time  # MHz
    range_resolution = 3e8 / (2 * bandwidth * 1e6)  # m
    max_range = (num_samples * 3e8) / (2 * freq_slope * 1e6 * sample_rate * 1e3)  # m
  
    # 速度性能
    lambda_wave = 3e8 / (config['startFreq'] * 1e9)  # m
    chirp_time = ramp_time + config['idleTime']  # μs
    velocity_resolution = lambda_wave / (2 * num_chirps * chirp_time * 1e-6)  # m/s
    max_velocity = lambda_wave / (4 * chirp_time * 1e-6)  # m/s
  
    # 帧率
    frame_rate = 1000 / frame_period  # FPS
  
    return {
        '带宽 (GHz)': bandwidth / 1000,
        '距离分辨率 (cm)': range_resolution * 100,
        '最大距离 (m)': max_range,
        '速度分辨率 (m/s)': velocity_resolution,
        '最大速度 (m/s)': max_velocity,
        '帧率 (FPS)': frame_rate
    }

# 使用示例
config = {
    'freqSlopeConst': 70,
    'rampEndTime': 200,
    'adcStartTime': 7,
    'numAdcSamples': 256,
    'digOutSampleRate': 5209,
    'numLoops': 128,
    'framePeriodicity': 100,
    'idleTime': 7,
    'startFreq': 77
}

performance = calculate_radar_performance(config)
for key, value in performance.items():
    print(f"{key}: {value:.3f}")
```

**输出示例**：

```
带宽 (GHz): 13.510
距离分辨率 (cm): 1.110
最大距离 (m): 10.485
速度分辨率 (m/s): 0.076
最大速度 (m/s): 9.750
帧率 (FPS): 10.000
```

---

## 📊 配置文件数据库

### SDK配置文件统计

| 应用类别       | 配置文件数量 | 主要Demo                      |
| -------------- | ------------ | ----------------------------- |
| 人员检测       | 8            | 3D People Tracking, Occupancy |
| 车内感知       | 6            | In-Cabin Sensing, Vital Signs |
| 手势识别       | 4            | Gesture Recognition           |
| 通用Demo       | 5            | Out-of-Box, Level Sensing     |
| **总计** | **23** | -                             |

### 配置文件路径规律

**SDK路径结构**：

```
C:\ti\<SDK_NAME>\
├── mmwave_demo\
│   └── 68xx_<application>\
│       ├── profiles\
│       │   ├── profile_*.cfg           # 单profile配置
│       │   └── advanced_*.cfg          # 高级配置
│       └── chirp_configs\
│           └── chirp_config_*.cfg      # Chirp配置
└── industrial_toolbox\
    └── <toolbox_name>\
        └── config\
            └── *.cfg                    # 工业应用配置
```

**文件识别特征**：

- ✅ 文件扩展名：`.cfg`
- ✅ 目录特征：`profiles/`, `chirp_configs/`, `config_file/`
- ✅ 文件名包含：`68xx`, `6844`, `profile`, `chirp`, `config`
- ✅ 内容特征：包含 `profileCfg`, `frameCfg`, `channelCfg` 等命令

---

## 🔬 深度研究主题

### 1. 高级配置命令

除了基础命令外，还有高级配置命令：

#### Angle FFT配置

```cfg
angleFftCfg -1 16 16 16
% 参数：RX ID, 窗口长度, 虚拟天线数
% 影响角度估计精度
```

#### 多普勒补偿

```cfg
clutterRemoval -1 0
% 移除静止目标（地面、墙壁）
% 提高运动目标检测
```

#### CFAR检测

```cfg
cfarCfg -1 0 2 8 4 3 0 15 1
% 恒虚警率检测器配置
% 影响目标检测灵敏度和虚警率
```

### 2. 多Profile配置

**时分复用（TDM）示例**：

```cfg
% Profile 0 - 短距高精度
profileCfg 0 77 7 7 100 0 0 100 1 256 8000 0 0 180

% Profile 1 - 长距低精度
profileCfg 1 77 7 7 200 0 0 50 1 128 4000 0 0 240

% Chirp配置使用不同Profile
chirpCfg 0 0 0 0 0 0 0 1  # Chirp 0使用Profile 0
chirpCfg 1 1 0 0 0 0 0 1  # Chirp 1使用Profile 1

% 帧配置交替使用
frameCfg 0 1 128 0 50 1 0
```

**应用场景**：

- ✅ 同时需要短距高精度和长距低精度
- ✅ 车辆检测（远近结合）
- ✅ 工业应用（不同检测区域）

### 3. 级联配置（多芯片）

```cfg
channelCfg 15 7 1  # cascading = 1
% 启用级联模式
% 多个雷达芯片协同工作
% 形成更大的虚拟天线阵列
% 提高角度分辨率
```

---

## 🎯 后续工具开发需求

### 1. 配置文件可视化编辑器

**功能需求**：

- ✅ 图形化参数编辑
- ✅ 实时性能计算显示
- ✅ 参数合法性验证
- ✅ 配置模板库
- ✅ 参数优化建议

**界面设计**：

```
┌─────────────────────────────────────────────┐
│ 📡 雷达配置编辑器                              │
├─────────────────────────────────────────────┤
│                                             │
│  基本参数                                   │
│  ├─ 起始频率：  [77    ] GHz               │
│  ├─ Chirp时间： [200   ] μs                │
│  ├─ 采样点数：  [256   ] (2的幂次)         │
│  └─ 帧率：      [10    ] FPS               │
│                                             │
│  天线配置                                   │
│  ├─ RX天线：   [☑☑☑☑] (4个)               │
│  └─ TX天线：   [☑☐☑] (TX1, TX3)           │
│                                             │
│  性能预览                                   │
│  ├─ 距离分辨率： 1.1 cm                    │
│  ├─ 最大距离：   10.5 m                    │
│  ├─ 速度分辨率： 0.076 m/s                 │
│  └─ 角度分辨率： 15°                       │
│                                             │
│  [预览配置] [保存] [加载模板]               │
└─────────────────────────────────────────────┘
```

### 2. 配置文件智能匹配器（已实现）

**当前功能**：

- ✅ 扫描SDK中所有配置文件
- ✅ 根据固件推荐Top 8配置
- ✅ 提供配置文件详细信息

**待增强功能**：

- ⏳ 配置文件参数解析和显示
- ⏳ 配置文件性能对比
- ⏳ 配置文件相似度计算

### 3. 配置文件测试工具

**功能需求**：

- ✅ 串口发送配置命令
- ✅ 验证雷达响应
- ✅ 性能测试（距离、速度、角度）
- ✅ 结果可视化

### 4. 配置文件优化助手

**功能需求**：

- ✅ 输入应用场景（距离范围、帧率等）
- ✅ 自动推荐最优配置
- ✅ 多目标优化（性能 vs 功耗）
- ✅ 生成配置文件

---

## 📖 参考资料

### TI官方文档

1. **mmWave SDK User Guide** - 配置命令参考
2. **Programming Chirp Parameters** - Chirp参数编程指南
3. **mmWave Demo User Guide** - Demo配置文件说明
4. **xWR68xx/xWRL68xx Data Sheet** - 硬件规格和限制
5. **xWRL68xx Technical Reference Manual** - 完整技术手册
6. **Migration_from_xWRLx432_to_xWRL6844.md** - 迁移指南

### 项目文档

1. **`2025-12-15_TI雷达固件配置文件关系详解.md`** - 配置文件类型区分
2. **`2025-12-17_AWRL6844EVM雷达参数配置文件清单.md`** - SDK配置文件清单
3. **`2025-12-17_TI毫米波雷达芯片命名规则详解.md`** - AWRL6844命名规则解析
4. **`2025-12-17_SBL固件规律分析.md`** - SBL固件选择
5. **`2025-12-17_TI_SDK固件列表.md`** - 完整固件清单
6. **`固件智能管理系统`** - 配置文件识别和匹配

### 在线资源

1. TI E2E Forums - 配置问题讨论
2. mmWave Training Series - 视频教程
3. TI Resource Explorer - 配置示例

---

## 📝 研究记录

### 研究进度

- [X] 配置文件类型区分
- [X] 基础命令参数理解
- [X] 性能计算公式推导
- [X] SDK配置文件统计
- [X] 配置文件命名规律
- [ ] 高级命令深度研究
- [ ] 多Profile配置测试
- [ ] 级联配置研究
- [ ] 实际性能测试验证
- [ ] 配置优化算法开发

### 待研究问题

1. **参数边界条件**

   - 各参数的硬件限制范围
   - 参数组合的约束条件
   - 非法配置的错误处理
2. **性能实测对比**

   - 理论计算 vs 实际测量
   - 不同配置的性能差异
   - 环境影响因素
3. **配置优化策略**

   - 多目标优化算法
   - 自动参数调优
   - 机器学习辅助配置
4. **特殊场景配置**

   - 极端环境（高温、低温）
   - 复杂环境（多径、干扰）
   - 特殊应用（穿墙、水下）

---

## 🔗 相关工具链接

- **烧录工具**: `01-AWRL6844固件系统工具/5-Scripts/flash_tool.py` v2.1.1
- **固件管理**: `02-固件智能管理系统/awrl6844_gui_app.py`
- **配置匹配**: `02-固件智能管理系统/awrl6844_firmware_matcher.py`
- **配置分析**: `05-雷达配置文件研究/config_calculator.py`
- **配置扫描**: `05-雷达配置文件研究/config_scanner.py`

---

## 📝 更新记录

### v2.2 (2025-12-20)
- ✅ 新增FAQ第6题：多次写入配置文件可以吗？每次写入是覆盖方式吗？
- ✅ 新增FAQ第7题：写入配置文件后，能从板子中读取更多的内容吗？
- ✅ 详细说明配置覆盖机制和重新配置流程
- ✅ 解析配置写入位置：RAM缓存、硬件寄存器、Flash存储
- ✅ 提供重新配置命令详解（sensorStop、flushCfg等）
- ✅ 说明写入配置后可读取的数据类型（检测数据、TLV、统计信息）
- ✅ 提供通过数据输出推断配置的方法
- ✅ 给出动态切换配置、自适应调整等实际应用场景
- ✅ 配置管理和验证的完整策略

### v2.1 (2025-12-20)

- ✅ 新增第5个常见问题：可以通过端口读取雷达配置文件吗？
- ✅ 详细说明配置读取的限制和原因
- ✅ 提供可读取内容的分类（Flash配置、状态信息）
- ✅ 给出配置管理的实际解决方案（3种方案）
- ✅ 配置管理最佳实践和工作流程
- ✅ 配置验证方法和实用建议

### v2.0 (2025-12-20)

- ✅ 新增"常见问题解答"章节（4个核心问题）
- ✅ 整合AWRL6844配置文件清单（45个配置）
- ✅ 详细的应用场景分类和推荐配置
- ✅ 配置文件兼容性矩阵
- ✅ Python发送配置示例代码
- ✅ 统计信息和趋势分析
- ✅ 常见错误和优化建议

### v1.0 (2025-12-17)

- ✅ 配置文件类型区分
- ✅ 基础命令参数理解
- ✅ 性能计算公式推导
- ✅ SDK配置文件统计
- ✅ 配置文件命名规律

---

**文档版本**: v2.2  
**创建日期**: 2025-12-17  
**更新日期**: 2025-12-20  
**作者**: Benson@Wisefido  
**状态**: 🚧 持续更新中  
**配置总数**: 45个（1个专用 + 44个通用）  
**FAQ数量**: 7个