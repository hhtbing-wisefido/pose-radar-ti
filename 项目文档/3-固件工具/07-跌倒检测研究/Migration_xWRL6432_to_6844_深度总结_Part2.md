# Migration深度总结 - 第二部分：通道配置与Per-Chirp LUT

## 第三部分（续）：TX/RX通道掩码配置

### 3.3 TX通道配置 🔴 关键变更

#### TX通道架构差异

**xWRL6432 TX架构**：
```
2个TX通道：
├─ TX0: 1个信号源 + 1个PA
└─ TX1: 1个信号源 + 1个PA

TX使能掩码：2位
例如：0b11 = 使能TX0和TX1
```

**xWRL6844 TX架构** ⭐：
```
4个TX通道，8个功率放大器：
├─ TX0AB: 2个PA（PA0A + PA0B）
├─ TX1AB: 2个PA（PA1A + PA1B）
├─ TX2AB: 2个PA（PA2A + PA2B）
└─ TX3AB: 2个PA（PA3A + PA3B）

TX使能掩码：8位（每个PA一位）

⚠️ 关键规则：
使能一个TX通道 = 必须使能对应的2个PA
```

#### TX通道使能掩码详解

**掩码位定义**：
```
TX Enable Mask (8-bit):
┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
│ Bit7│ Bit6│ Bit5│ Bit4│ Bit3│ Bit2│ Bit1│ Bit0│
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│PA3B │PA3A │PA2B │PA2A │PA1B │PA1A │PA0B │PA0A │
└─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘

使能示例：
├─ 使能TX0AB: 0b00000011 = 0x03
├─ 使能TX1AB: 0b00001100 = 0x0C
├─ 使能TX2AB: 0b00110000 = 0x30
├─ 使能TX3AB: 0b11000000 = 0xC0
└─ 使能所有TX: 0b11111111 = 0xFF
```

**常用配置**：
```c
// 使能TX0和TX1（类似6432的2TX配置）
TX_MASK = 0b00001111 = 0x0F

// 使能TX0,TX1,TX2,TX3（完整4TX配置）
TX_MASK = 0b11111111 = 0xFF

// TDMA模式：依次使能每个TX
Chirp 0: TX_MASK = 0x03  // TX0
Chirp 1: TX_MASK = 0x0C  // TX1
Chirp 2: TX_MASK = 0x30  // TX2
Chirp 3: TX_MASK = 0xC0  // TX3
```

**⚠️ 错误示例**：
```c
// ❌ 错误：只使能一个PA
TX_MASK = 0b00000001  // 只使能PA0A，TX0无法工作

// ✅ 正确：使能两个PA
TX_MASK = 0b00000011  // 使能PA0A和PA0B，TX0正常工作
```

### 3.4 RX通道配置 🔴 关键变更

#### RX通道架构

**xWRL6432 RX架构**：
```
3个RX通道：
├─ RX0
├─ RX1  
└─ RX2

RX使能掩码：3位
例如：0b111 = 使能所有RX
```

**xWRL6844 RX架构**：
```
4个RX通道，4个基带处理链：
├─ RX0A: 独立基带链
├─ RX1A: 独立基带链
├─ RX2A: 独立基带链
└─ RX3A: 独立基带链

RX使能掩码：8位（为兼容xWRL6888预留）
```

#### RX通道使能掩码详解 ⭐ 特殊格式

**掩码位定义**：
```
RX Enable Mask (8-bit):
┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
│ Bit7│ Bit6│ Bit5│ Bit4│ Bit3│ Bit3│ Bit1│ Bit0│
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│RX3B │ N/A │RX2B │ N/A │RX1B │RX3A │RX1A │RX0A │
│(预留)│     │(预留)│     │(预留)│     │     │     │
└─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘

⚠️ 有效位：只有Bit0, Bit1, Bit3, Bit7
其他位预留给未来的xWRL6888
```

**使能示例**：
```c
// 使能所有4个RX通道（标准配置）
RX_MASK = 0b10011001 = 0x99  ← 官方推荐！

// 分解说明：
Bit7=1: RX3A使能
Bit3=1: RX2A使能
Bit1=1: RX1A使能
Bit0=1: RX0A使能

// ❌ 错误示例：
RX_MASK = 0b11111111 = 0xFF  // 错误！包含了无效位
RX_MASK = 0b00001111 = 0x0F  // 错误！只使能了RX0A,RX1A,RX2A
```

**常用配置**：
```c
// 使能RX0A,RX1A,RX2A（3RX，类似6432）
RX_MASK = 0b00001011 = 0x0B

// 使能所有RX（4RX，完整配置）
RX_MASK = 0b10011001 = 0x99  ← 推荐

// 只使能RX0A（单通道测试）
RX_MASK = 0b00000001 = 0x01
```

### 3.5 通道配置迁移对比

#### CLI命令对比

**xWRL6432配置**：
```bash
# channelCfg <rxChannelEn> <txChannelEn> <cascading>
channelCfg 7 3 0

解释：
rxChannelEn = 7 = 0b111     → 使能RX0,RX1,RX2
txChannelEn = 3 = 0b11      → 使能TX0,TX1
cascading = 0               → 无级联
```

**xWRL6844配置**：
```bash
# channelCfg <rxChannelEn> <txChannelEn> <cascading>
channelCfg 15 15 0

解释：
rxChannelEn = 15 = 0b1111   → 实际映射到0x99（RX0A,RX1A,RX2A,RX3A）
txChannelEn = 15 = 0b1111   → 实际映射到0xFF（TX0AB,TX1AB,TX2AB,TX3AB）
cascading = 0               → 无级联

⚠️ SDK内部会自动转换为正确的8位掩码
```

**⚠️ 迁移注意事项**：
```
🔴 如果直接使用mmWaveLink API（不通过SDK）：
├─ 必须使用正确的8位掩码
├─ TX: 每个通道2位（必须同时使能两个PA）
├─ RX: 使用特殊的0x99格式
└─ 参考ICD文档第3章

✅ 如果使用SDK CLI：
├─ SDK会自动处理掩码转换
├─ 使用简化的参数即可
└─ 保持与6432类似的配置方式
```

---

## 第四部分：Per-Chirp LUT方法

### 4.1 Per-Chirp LUT概述 ⭐ 新特性

#### 什么是Per-Chirp LUT？

```
Per-Chirp LUT (Look-Up Table)：
├─ 为每个chirp独立配置参数
├─ 参数存储在专用内存（16KB PER_CHIRP_RAM）
├─ 硬件自动从LUT更新参数
└─ 实现TDMA、BPM等高级模式

支持的参数：
├─ TX使能（tx_en）
├─ BPM相位（tx_bpm）
├─ 频率偏移
├─ 相位偏移
└─ 其他chirp参数
```

#### xWRL6432 vs xWRL6844对比

```
xWRL6432:
├─ TDMA/BPM通过Profile API配置
├─ 配置简单但灵活性有限
└─ mmwaveLink提供TDMA/BPM选项

xWRL6844:
├─ 必须使用Per-Chirp LUT配置TDMA/BPM
├─ mmwaveLink不再提供TDMA/BPM选项
├─ 更灵活但配置更复杂
└─ SDK内部封装，提供CLI接口
```

**⚠️ 迁移影响**：
```
如果使用SDK CLI：
✅ 无需修改，SDK自动处理Per-Chirp LUT

如果直接使用mmWaveLink API：
🔴 必须学习Per-Chirp LUT配置
🔴 参考ICD文档 section 2.4.5 ~ 2.4.9
```

### 4.2 TDMA模式配置示例

#### 示例1：基础TDMA模式（4 Chirps）

**需求**：
```
4个chirp，TX使能顺序：TX0 → TX1 → TX2 → TX3
```

**Per-Chirp LUT配置**：

**tx_en.txt文件内容**：
```
// Chirp 0: 使能TX0AB
0x03  // 0b00000011

// Chirp 1: 使能TX1AB  
0x0C  // 0b00001100

// Chirp 2: 使能TX2AB
0x30  // 0b00110000

// Chirp 3: 使能TX3AB
0xC0  // 0b11000000
```

**API调用**：
```c
// 1. 设置Per-Chirp TX使能
rlRfPerChirpPhShifterCfg_t txEnCfg;
txEnCfg.numOfChirps = 4;
txEnCfg.txOutPowerBackoffCode = {0x03, 0x0C, 0x30, 0xC0};

rlRfSetPerChirpPhShifterCfg(deviceMap, &txEnCfg);

// 2. 在Profile配置中关联
rlChirpCommonCfg_t commonCfg;
commonCfg.chirpTxMimoPatSel = 1;  // 使能TDMA
rlRfSetChirpCommonConfig(deviceMap, &commonCfg);
```

**内存格式**：
```
加载后，PER_CHIRP_RAM中的数据：
地址     | 数据     | 说明
---------|----------|----------
0x0000   | 0xc0300c03 | 4个chirp的TX使能，小端格式
         |          | = {0x03, 0x0C, 0x30, 0xC0}
```

#### 示例2：TDMA with Variation（8 Chirps）

**需求**：
```
8个chirp，TX使能顺序：
TX0, TX0, TX1, TX1, TX2, TX2, TX3, TX3
每个TX重复2次
```

**优化方案**：
```c
// 复用示例1的LUT文件
// 只需修改重复计数

rlRfPerChirpPhShifterCfg_t txEnCfg;
txEnCfg.numOfChirps = 4;  // LUT中4个值
txEnCfg.repetitionCount = 2;  // 每个值重复2次
txEnCfg.txOutPowerBackoffCode = {0x03, 0x0C, 0x30, 0xC0};

// 结果：生成8个chirp
// Chirp 0,1: TX0 (0x03)
// Chirp 2,3: TX1 (0x0C)  
// Chirp 4,5: TX2 (0x30)
// Chirp 6,7: TX3 (0xC0)
```

### 4.3 BPM模式配置示例

#### 示例3：BPM模式（4 Chirps）

**需求**：
```
4个chirp，TX使能顺序：
Chirp 0: TX0+TX1 (同相)
Chirp 1: TX0-TX1 (TX1反相180°)
Chirp 2: TX2+TX3 (同相)
Chirp 3: TX2-TX3 (TX3反相180°)
```

**Per-Chirp LUT配置（需要2个文件）**：

**tx_en.txt**（TX使能）：
```
// Chirp 0: 使能TX0和TX1
0x0F  // 0b00001111

// Chirp 1: 使能TX0和TX1（TX1相位在tx_bpm中控制）
0x0F  // 0b00001111

// Chirp 2: 使能TX2和TX3
0xF0  // 0b11110000

// Chirp 3: 使能TX2和TX3
0xF0  // 0b11110000
```

**tx_bpm.txt**（BPM相位控制）：
```
// Chirp 0: TX0+TX1同相
0x00  // 无相位翻转

// Chirp 1: TX1反相180°
0x0C  // 0b00001100（TX1AB的2个PA都反相）

// Chirp 2: TX2+TX3同相
0x00  // 无相位翻转

// Chirp 3: TX3反相180°
0xC0  // 0b11000000（TX3AB的2个PA都反相）
```

**API调用**：
```c
// 1. 设置TX使能LUT
rlRfPerChirpPhShifterCfg_t txEnCfg;
txEnCfg.numOfChirps = 4;
txEnCfg.txOutPowerBackoffCode = {0x0F, 0x0F, 0xF0, 0xF0};
rlRfSetPerChirpPhShifterCfg(deviceMap, &txEnCfg);

// 2. 设置BPM相位LUT
rlRfPerChirpBpmCfg_t bpmCfg;
bpmCfg.numOfChirps = 4;
bpmCfg.bpmPhaseConfig = {0x00, 0x0C, 0x00, 0xC0};
rlRfSetPerChirpBpmConfig(deviceMap, &bpmCfg);

// 3. 在Profile配置中启用BPM
rlChirpCommonCfg_t commonCfg;
commonCfg.chirpTxMimoPatSel = 4;  // 使能BPM
rlRfSetChirpCommonConfig(deviceMap, &commonCfg);
```

### 4.4 SDK CLI抽象（推荐使用）

**SDK简化配置**：
```bash
# chirpComnCfg中的chirpTxMimoPatSel参数
# SDK自动生成Per-Chirp LUT

# TDMA模式
chirpComnCfg 80 0 0 128 1 63.0 1
                          ↑
                    chirpTxMimoPatSel = 1

# BPM模式  
chirpComnCfg 80 0 0 128 4 63.0 1
                          ↑
                    chirpTxMimoPatSel = 4
```

**⚠️ 迁移建议**：
```
如果可能，优先使用SDK CLI：
✅ 自动处理Per-Chirp LUT配置
✅ 与6432保持相似的配置方式
✅ 减少出错机会

只在以下情况直接使用API：
├─ 需要自定义chirp模式
├─ 不使用SDK框架
└─ 需要更精细的控制
```

---

## 第五部分：SDK CLI配置迁移

### 5.1 通道配置对比

#### 配置命令格式

```bash
channelCfg <rxChannelEn> <txChannelEn> <cascading>
```

#### 实际配置对比

| 参数 | xWRL6432 | xWRL6844 | 说明 |
|------|----------|----------|------|
| **rxChannelEn** | 7 | 15 | SDK自动转换为正确掩码 |
| **txChannelEn** | 3 | 15 | SDK自动转换为正确掩码 |
| **cascading** | 0 | 0 | 级联模式（通常为0） |

**示例**：
```bash
# xWRL6432（2TX3RX）
channelCfg 7 3 0

# xWRL6844（4TX4RX）
channelCfg 15 15 0
```

### 5.2 低功耗配置

```bash
lowPowerCfg <lpAdcMode>

# 使能片间和帧间动态功耗节省
lowPowerCfg 1
```

**说明**：xWRL6432和xWRL6844相同，无需修改。

### 5.3 Chirp通用配置

#### 命令格式

```bash
chirpComnCfg <digOutputSampRate> <dfeDataOutputMode> <chirpTxMimoPatSel> 
             <numLoops> <numRXChannelGroups> <totalRampTimeInUs> <hpfCornerFreq>
```

#### 关键参数对比

| 参数 | xWRL6432 | xWRL6844 | 变化说明 |
|------|----------|----------|---------|
| **digOutputSampRate** | 40 | 80 | ⚠️ 翻倍以保持相同采样率 |
| **dfeDataOutputMode** | 0 | 0 | 无变化 |
| **chirpTxMimoPatSel** | 1或4 | 1或4 | 1=TDMA, 4=BPM |
| **numLoops** | 128 | 128 | Range FFT点数 |
| **numRXChannelGroups** | 4 | 4 | 无变化 |
| **totalRampTimeInUs** | 63.0 | 63.0 | 无变化 |
| **hpfCornerFreq** | 1 | 1 | 高通滤波器 |

**示例**：
```bash
# xWRL6432
chirpComnCfg 40 0 0 128 4 63.0 1
             ↑
      采样率除数=40
      实际采样率=100MHz/40=2.5Msps

# xWRL6844（保持相同采样率）
chirpComnCfg 80 0 0 128 4 63.0 1
             ↑
      采样率除数=80（翻倍）
      实际采样率=200MHz/80=2.5Msps
```

