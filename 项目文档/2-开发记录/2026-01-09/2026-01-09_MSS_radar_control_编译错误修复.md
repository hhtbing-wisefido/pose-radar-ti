# MSS radar_control.c 编译错误修复 (问题19)

**日期**: 2026-01-09  
**错误类型**: L-SDK 6.x API结构体字段不匹配（问题18回归）  
**文件**: `radar_control.c` (MSS项目)

## 🔴 错误信息

```
../radar_control.c:235:30: error: no member named 'startFreqGHz' in 'struct MMWave_ProfileComCfg_t'
../radar_control.c:236:30: error: no member named 'digOutSampleRateMHz' in 'struct MMWave_ProfileComCfg_t'
../radar_control.c:237:30: error: no member named 'numAdcSamples' in 'struct MMWave_ProfileComCfg_t'
../radar_control.c:238:30: error: no member named 'rxGain' in 'struct MMWave_ProfileComCfg_t'
../radar_control.c:241:31: error: no member named 'idleTimeus' in 'struct MMWave_ProfileTimeCfg_t'
../radar_control.c:242:31: error: no member named 'adcStartTimeus' in 'struct MMWave_ProfileTimeCfg_t'
../radar_control.c:243:31: error: no member named 'rampEndTimeus' in 'struct MMWave_ProfileTimeCfg_t'
../radar_control.c:244:31: error: no member named 'freqSlopeConst' in 'struct MMWave_ProfileTimeCfg_t'
../radar_control.c:254:33: error: no member named 'channelCfg' in 'struct HealthDetect_CliCfg_t'
```

**共9个错误**

## 🔍 根本原因

**问题**: CCS workspace中的`radar_control.c`是旧文件，使用了错误的SDK结构体字段名称

**位置判断**:
- 编译日志显示: `../radar_control.c`
- CCS工作目录: `C:/Users/Administrator/workspace_ccstheia/health_detect_6844_mss/Release/`
- 实际文件: `C:/Users/Administrator/workspace_ccstheia/health_detect_6844_mss/radar_control.c`
- **不是**项目代码目录的文件: `D:/7.project/TI_Radar_Project/project-code/AWRL6844_HealthDetect/src/mss/source/radar_control.c`

## ✅ 修复步骤

### 步骤1: 在CCS中打开文件

1. 在CCS Project Explorer中找到`health_detect_6844_mss`项目
2. 双击打开`radar_control.c`文件

### 步骤2: 定位错误代码

找到**第230-260行**左右的配置代码段

### 步骤3: 替换为正确代码

将原来的代码（有9个错误的版本）替换为以下**正确的SDK 6.x代码**：

```c
    DebugP_log("RadarControl: Configuring...\r\n");

    /* Configure profile common parameters - match SDK struct fields */
    /* SDK: digOutputSampRate (uint8_t), numOfAdcSamples (uint16_t) */
    gMmWaveCfg.profileComCfg.digOutputSampRate = (uint8_t)(cliCfg->profileCfg.digOutSampleRate / 1000);
    gMmWaveCfg.profileComCfg.numOfAdcSamples = cliCfg->profileCfg.numAdcSamples;
    gMmWaveCfg.profileComCfg.digOutputBitsSel = 0; /* 0: 16-bit, 2: 14-bit */
    gMmWaveCfg.profileComCfg.dfeFirSel = 0;
    gMmWaveCfg.profileComCfg.chirpRampEndTimeus = cliCfg->profileCfg.rampEndTimeUs;
    gMmWaveCfg.profileComCfg.chirpRxHpfSel = 0; /* 0: 175kHz corner */
    gMmWaveCfg.profileComCfg.chirpTxMimoPatSel = 0;

    /* Configure profile timing parameters - match SDK struct fields */
    /* SDK: chirpIdleTimeus, chirpAdcStartTime (uint16_t), chirpSlope, startFreqGHz */
    gMmWaveCfg.profileTimeCfg.chirpIdleTimeus = cliCfg->profileCfg.idleTimeUs;
    gMmWaveCfg.profileTimeCfg.chirpAdcStartTime = (uint16_t)cliCfg->profileCfg.adcStartTimeUs;
    gMmWaveCfg.profileTimeCfg.chirpTxStartTimeus = 0.0f; /* Typically 0 */
    gMmWaveCfg.profileTimeCfg.chirpSlope = cliCfg->profileCfg.freqSlopeConst;
    gMmWaveCfg.profileTimeCfg.startFreqGHz = cliCfg->profileCfg.startFreqGHz;

    /* Configure frame parameters */
    gMmWaveCfg.frameCfg.numOfChirpsInBurst = (cliCfg->frameCfg.chirpEndIdx - cliCfg->frameCfg.chirpStartIdx + 1);
    gMmWaveCfg.frameCfg.numOfBurstsInFrame = cliCfg->frameCfg.numLoops;
    gMmWaveCfg.frameCfg.numOfFrames = cliCfg->frameCfg.numFrames;
    gMmWaveCfg.frameCfg.framePeriodicityus = (uint32_t)(cliCfg->frameCfg.framePeriodMs * 1000.0f);

    /* Configure TX/RX enable - use fields from CliCfg */
    gMmWaveCfg.txEnbl = cliCfg->chirpCfg.txEnable;
    gMmWaveCfg.rxEnbl = cliCfg->rxChannelEn;
```

### 步骤4: 保存文件

在CCS中按`Ctrl+S`保存文件

### 步骤5: Clean并重新编译

1. 右键`health_detect_6844_mss`项目 → Clean Project
2. 右键`health_detect_6844_mss`项目 → Build Project
3. 检查编译输出，应该没有错误

## 📊 修改对照表

| 错误代码 | 正确代码 | 所属结构 | 说明 |
|---------|---------|---------|------|
| `profileComCfg.startFreqGHz` | `profileTimeCfg.startFreqGHz` | ProfileTimeCfg | ✅ 字段在TimeCfg中 |
| `profileComCfg.digOutSampleRateMHz` | `profileComCfg.digOutputSampRate` | ProfileComCfg | ✅ 字段名不同+类型uint8_t |
| `profileComCfg.numAdcSamples` | `profileComCfg.numOfAdcSamples` | ProfileComCfg | ✅ 字段名多了"Of" |
| `profileComCfg.rxGain` | ❌ 删除此行 | N/A | SDK中不存在 |
| `profileTimeCfg.idleTimeus` | `profileTimeCfg.chirpIdleTimeus` | ProfileTimeCfg | ✅ 字段名多了"chirp" |
| `profileTimeCfg.adcStartTimeus` | `profileTimeCfg.chirpAdcStartTime` | ProfileTimeCfg | ✅ 字段名+类型uint16_t |
| `profileTimeCfg.rampEndTimeus` | `profileComCfg.chirpRampEndTimeus` | ProfileComCfg | ✅ 字段在ComCfg中 |
| `profileTimeCfg.freqSlopeConst` | `profileTimeCfg.chirpSlope` | ProfileTimeCfg | ✅ 字段名不同 |
| `cliCfg->channelCfg.rxChannelEn` | `cliCfg->rxChannelEn` | CliCfg | ✅ 直接访问，无channelCfg |

## 🎯 验证

修复后，编译应该通过，没有这9个错误。

## 📝 文档同步

修复完成后需要：
1. ✅ 更新`HealthDetect项目重建总结.md`，添加"问题19"
2. ✅ 提交到Git
3. ✅ 推送到GitHub

---

> 📌 **注意**: 此问题是问题18的回归，原因是CCS workspace中的文件与项目代码目录不同步。
