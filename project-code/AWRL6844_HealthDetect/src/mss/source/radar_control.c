/**
 * @file radar_control.c
 * @brief Radar Control Module Implementation (mmWave API Wrapper)
 *
 * Reference: AWRL6844_InCabin_Demos/src/mss/source/mmwave_demo_mss.c
 * Adapted for: Health Detection three-layer architecture
 *
 * RTOS: FreeRTOS (L-SDK mandatory)
 * Note: L-SDK uses FreeRTOS, NOT TI-RTOS/BIOS
 *
 * L-SDK 6.x API Usage:
 * - MMWave_init(MMWave_InitCfg*, errCode) - Initialize mmWave control
 * - MMWave_open(handle, MMWave_Cfg*, errCode) - Open mmWave device
 * - MMWave_config(handle, MMWave_Cfg*, errCode) - Configure mmWave
 * - MMWave_start(handle, MMWave_StrtCfg*, errCode) - Start sensor
 * - MMWave_stop(handle, MMWave_StrtCfg*, errCode) - Stop sensor
 * - MMWave_close(handle, errCode) - Close mmWave device
 *
 * Created: 2026-01-08
 * Updated: 2026-01-08 - Fixed API for L-SDK 6.x
 */

/**************************************************************************
 *************************** Include Files ********************************
 **************************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* SDK DPL Includes */
#include <kernel/dpl/DebugP.h>
#include <kernel/dpl/ClockP.h>

/* mmWave Control Includes */
#include <control/mmwave/mmwave.h>
#include <mmwavelink/include/rl_device.h>
#include <mmwavelink/include/rl_sensor.h>
#include <common/sys_common_xwrL684x.h>

/* Driver Includes */
#include <drivers/adcbuf.h>

/* Application Includes */
#include <source/radar_control.h>
#include <source/health_detect_main.h>

/* SysConfig Generated Includes */
#include "ti_drivers_config.h"

/**************************************************************************
 *************************** Local Definitions ****************************
 **************************************************************************/

/* None - frequency limits are in MMWave_Cfg, not separate defines */

/**************************************************************************
 *************************** Local Variables ******************************
 **************************************************************************/

/** @brief mmWave handle */
static MMWave_Handle gMmWaveHandle = NULL;

/** @brief mmWave configuration */
static MMWave_Cfg gMmWaveCfg;

/** @brief ADC Buffer handle */
static ADCBuf_Handle gAdcBufHandle = NULL;

/** @brief Radar initialized flag */
static uint8_t gRadarInitialized = 0;

/** @brief Radar opened flag */
static uint8_t gRadarOpened = 0;

/** @brief Radar configured flag */
static uint8_t gRadarConfigured = 0;

/**************************************************************************
 *************************** Public Functions *****************************
 **************************************************************************/

/**
 * @brief Initialize radar control module
 * 
 * L-SDK 6.x: Uses MMWave_init(MMWave_InitCfg*, errCode*)
 */
int32_t RadarControl_init(void)
{
    int32_t errCode;
    int32_t retVal = 0;
    ADCBuf_Params adcBuffParams;
    MMWave_ErrorLevel errorLevel;
    int16_t mmWaveErrorCode;
    int16_t subsysErrorCode;

    DebugP_log("RadarControl: Initializing...\r\n");

    /* Initialize the mmWave configuration */
    memset(&gMmWaveCfg, 0, sizeof(MMWave_Cfg));

    /* Initialize the mmWave control init configuration */
    memset(&gMmWaveCfg.initCfg, 0, sizeof(MMWave_InitCfg));
    gMmWaveCfg.initCfg.iswarmstart = false;

    /* Open ADCBuf driver */
    ADCBuf_Params_init(&adcBuffParams);
    gAdcBufHandle = ADCBuf_open(CONFIG_ADCBUF0, &adcBuffParams);
    if (gAdcBufHandle == NULL)
    {
        DebugP_log("RadarControl: ADCBuf_open failed\r\n");
        retVal = -1;
        goto exit;
    }

    /* Initialize mmWave control module */
    gMmWaveHandle = MMWave_init(&gMmWaveCfg.initCfg, &errCode);
    if (gMmWaveHandle == NULL)
    {
        MMWave_decodeError(errCode, &errorLevel, &mmWaveErrorCode, &subsysErrorCode);
        DebugP_log("RadarControl: MMWave_init failed [errCode=%d, errorLevel=%d, mmWaveErr=%d, subsysErr=%d]\r\n",
                   errCode, errorLevel, mmWaveErrorCode, subsysErrorCode);
        retVal = errCode;
        goto exit;
    }

    /* Store handle in MCB */
    gHealthDetectMCB.ctrlHandle = gMmWaveHandle;
    gRadarInitialized = 1;

    DebugP_log("RadarControl: Initialized\r\n");

exit:
    return retVal;
}

/**
 * @brief Open radar device
 * 
 * L-SDK 6.x: Uses MMWave_open(handle, MMWave_Cfg*, errCode*)
 */
int32_t RadarControl_open(void)
{
    int32_t errCode;

    if (!gRadarInitialized)
    {
        DebugP_log("RadarControl: Not initialized\r\n");
        return -1;
    }

    if (gRadarOpened)
    {
        DebugP_log("RadarControl: Already opened\r\n");
        return 0;
    }

    DebugP_log("RadarControl: Opening...\r\n");

    /* Open mmWave device using L-SDK 6.x API */
    if (MMWave_open(gMmWaveHandle, &gMmWaveCfg, &errCode) < 0)
    {
        DebugP_log("RadarControl: MMWave_open failed, errCode=%d\r\n", errCode);
        return errCode;
    }

    gRadarOpened = 1;

    DebugP_log("RadarControl: Opened\r\n");

    return 0;
}

/**
 * @brief Close radar device
 * 
 * L-SDK 6.x: Uses MMWave_close(handle, errCode*)
 */
int32_t RadarControl_close(void)
{
    int32_t errCode;

    if (!gRadarOpened)
    {
        return 0;
    }

    DebugP_log("RadarControl: Closing...\r\n");

    /* Close mmWave device using L-SDK 6.x API */
    if (MMWave_close(gMmWaveHandle, &errCode) < 0)
    {
        DebugP_log("RadarControl: MMWave_close failed, errCode=%d\r\n", errCode);
        return errCode;
    }

    gRadarOpened = 0;
    gRadarConfigured = 0;

    DebugP_log("RadarControl: Closed\r\n");

    return 0;
}

/**
 * @brief Configure radar with CLI settings
 * 
 * L-SDK 6.x: Configuration is done by filling MMWave_Cfg structure
 * MMWave_open/config/start will be called in RadarControl_start()
 * 
 * Note: In L-SDK 6.x, there are no separate MMWave_addProfile, MMWave_addChirp,
 * MMWave_setFrameCfg functions. Configuration is done through MMWave_Cfg structure.
 */
int32_t RadarControl_config(HealthDetect_MCB_t *pMCB)
{
    if (pMCB == NULL)
    {
        return -1;
    }

    DebugP_log("RadarControl: Configuring...\r\n");

    /* Configure profile common parameters - match SDK struct fields */
    /* SDK: digOutputSampRate (uint8_t), numOfAdcSamples (uint16_t) */
    gMmWaveCfg.profileComCfg.digOutputSampRate = (uint8_t)(pMCB->cliCfg.profileCfg.digOutSampleRate / 1000);
    gMmWaveCfg.profileComCfg.numOfAdcSamples = pMCB->cliCfg.profileCfg.numAdcSamples;
    gMmWaveCfg.profileComCfg.digOutputBitsSel = 0; /* 0: 16-bit, 2: 14-bit */
    gMmWaveCfg.profileComCfg.dfeFirSel = 0;
    gMmWaveCfg.profileComCfg.chirpRampEndTimeus = pMCB->cliCfg.profileCfg.rampEndTimeUs;
    gMmWaveCfg.profileComCfg.chirpRxHpfSel = 0; /* 0: 175kHz corner */
    gMmWaveCfg.profileComCfg.chirpTxMimoPatSel = 0;

    /* Configure profile timing parameters - match SDK struct fields */
    /* SDK: chirpIdleTimeus, chirpAdcStartTime (uint16_t), chirpSlope, startFreqGHz */
    gMmWaveCfg.profileTimeCfg.chirpIdleTimeus = pMCB->cliCfg.profileCfg.idleTimeUs;
    gMmWaveCfg.profileTimeCfg.chirpAdcStartTime = (uint16_t)pMCB->cliCfg.profileCfg.adcStartTimeUs;
    gMmWaveCfg.profileTimeCfg.chirpTxStartTimeus = 0.0f; /* Typically 0 */
    gMmWaveCfg.profileTimeCfg.chirpSlope = pMCB->cliCfg.profileCfg.freqSlopeConst;
    gMmWaveCfg.profileTimeCfg.startFreqGHz = pMCB->cliCfg.profileCfg.startFreqGHz;

    /* Configure frame parameters */
    gMmWaveCfg.frameCfg.numOfChirpsInBurst = (pMCB->cliCfg.frameCfg.chirpEndIdx - pMCB->cliCfg.frameCfg.chirpStartIdx + 1);
    gMmWaveCfg.frameCfg.numOfBurstsInFrame = pMCB->cliCfg.frameCfg.numLoops;
    gMmWaveCfg.frameCfg.numOfFrames = pMCB->cliCfg.frameCfg.numFrames;
    gMmWaveCfg.frameCfg.framePeriodicityus = (uint32_t)(pMCB->cliCfg.frameCfg.framePeriodMs * 1000.0f);

    /* Configure TX/RX enable - use fields from CliCfg channelCfg command */
    gMmWaveCfg.txEnbl = pMCB->cliCfg.txChannelEn;
    gMmWaveCfg.rxEnbl = pMCB->cliCfg.rxChannelEn;
    
    /* Store APLL frequency (400MHz default, 396MHz if shift enabled) */
    gMmWaveCfg.apllFreqMHz = 400.0f;  /* Default 400MHz */

    gRadarConfigured = 1;

    DebugP_log("RadarControl: Configuration saved (tx=0x%04X, rx=0x%04X)\r\n", 
               gMmWaveCfg.txEnbl, gMmWaveCfg.rxEnbl);

    return 0;
}

/**
 * @brief Configure and Enable APLL (SDK Standard)
 * 参考: mmw_demo_SDK_reference/source/mmwave_demo.c line 395-450
 * 
 * 🔴 关键修复（问题36）：
 * - 完整实现SDK的APLL配置流程
 * - 支持校准数据保存/恢复（SAVE_APLL_CALIB_DATA/RESTORE_APLL_CALIB_DATA）
 * - 支持396MHz和400MHz频率
 * - 错误处理和恢复机制
 * 
 * APLL配置3种场景：
 * 1. 冷启动+频率偏移：saveRestoreCalData=SAVE, apllFreqMHz=396
 * 2. 热启动（恢复校准）：saveRestoreCalData=RESTORE, apllFreqMHz=396/400
 * 3. 冷启动+无偏移：saveRestoreCalData=SAVE, apllFreqMHz=400
 * 
 * @param apllFreqMHz APLL frequency in MHz (396.0 or 400.0)
 * @param saveRestoreCalData 0=RESTORE校准数据, 1=SAVE校准数据
 * @return 0 on success, error code on failure
 */
int32_t RadarControl_configAndEnableApll(float apllFreqMHz, uint8_t saveRestoreCalData)
{
    int32_t retVal = 0;
    int32_t errCode;
    APLL_CalResult* ptrApllCalRes = NULL;
    
    DebugP_log("RadarControl: Configuring APLL at %.1f MHz (saveRestore=%d)...\r\n", 
               apllFreqMHz, saveRestoreCalData);

    /* Step 1: 关闭APLL (SDK Standard) */
    retVal = MMWave_FecssDevClockCtrl(&gMmWaveCfg.initCfg, 
                                       MMWAVE_APLL_CLOCK_DISABLE, &errCode);
    if (retVal != 0)
    {
        DebugP_log("Error: APLL disable failed [errCode=%d]\r\n", errCode);
        return -1;
    }

    /* Step 2: 配置APLL寄存器 (SDK Standard) */
    retVal = MMWave_ConfigApllReg(apllFreqMHz);
    if (retVal != 0)
    {
        DebugP_log("Error: APLL register config failed [retVal=%d]\r\n", retVal);
        return -1;
    }

    /* Step 3: 处理校准数据 (SDK Standard) */
    if (saveRestoreCalData == 0)  /* RESTORE模式 */
    {
        /* 根据频率选择校准数据 */
        if (apllFreqMHz == 396.0f)
        {
            ptrApllCalRes = &gHealthDetectMCB.downShiftedApllCalRes;
            DebugP_log("RadarControl: Restoring 396MHz calibration data\r\n");
        }
        else  /* 400.0f */
        {
            ptrApllCalRes = &gHealthDetectMCB.defaultApllCalRes;
            DebugP_log("RadarControl: Restoring 400MHz calibration data\r\n");
        }

        /* 恢复校准数据到APLL (SDK Standard API: MMWave_SetApllCalResult) */
        retVal = MMWave_SetApllCalResult(ptrApllCalRes);  /* ptrApllCalRes is now uint32_t* */
        if (retVal != 0)
        {
            DebugP_log("Error: APLL restore calibration failed [retVal=%d]\r\n", retVal);
            return -1;
        }
    }
    else  /* SAVE模式 */
    {
        DebugP_log("RadarControl: Will save calibration after APLL enable\r\n");
    }

    /* Step 4: 启用APLL (SDK Standard) */
    retVal = MMWave_FecssDevClockCtrl(&gMmWaveCfg.initCfg, 
                                       MMWAVE_APLL_CLOCK_ENABLE, &errCode);
    if (retVal != 0)
    {
        DebugP_log("Error: APLL enable failed [errCode=%d]\r\n", errCode);
        return -1;
    }

    /* Step 5: 如果是SAVE模式，保存校准数据 (SDK Standard) */
    if (saveRestoreCalData == 1)
    {
        /* 根据频率选择存储位置 */
        if (apllFreqMHz == 396.0f)
        {
            ptrApllCalRes = &gHealthDetectMCB.downShiftedApllCalRes;
            DebugP_log("RadarControl: Saving 396MHz calibration data\r\n");
        }
        else  /* 400.0f */
        {
            ptrApllCalRes = &gHealthDetectMCB.defaultApllCalRes;
            DebugP_log("RadarControl: Saving 400MHz calibration data\r\n");
        }

        /* 保存APLL校准数据 (SDK Standard API: MMWave_GetApllCalResult) */
        retVal = MMWave_GetApllCalResult(ptrApllCalRes);  /* ptrApllCalRes is now uint32_t* */
        if (retVal != 0)
        {
            DebugP_log("Error: APLL save calibration failed [retVal=%d]\r\n", retVal);
            return -1;
        }
    }

    DebugP_log("RadarControl: APLL configured and enabled successfully\r\n");
    
    return 0;
}

/**
 * @brief Start radar sensor
 * 
 * L-SDK 6.x: Uses MMWave_start(handle, MMWave_StrtCfg*, errCode*)
 * Must configure ADCBuf and RF power before starting
 */
int32_t RadarControl_start(void)
{
    int32_t errCode;
    int32_t retVal;
    uint8_t channel;
    uint16_t offset = 0;
    uint32_t chanDataSize;
    uint32_t chanDataSizeAligned16;
    ADCBuf_RxChanConf rxChanConf;

    if (!gRadarConfigured)
    {
        DebugP_log("RadarControl: Not configured\r\n");
        return -1;
    }

    DebugP_log("RadarControl: Starting (SDK flow)...\r\n");

    /* Step 1: Configure ADC Buffer channels (like SDK MmwDemo_ADCBufConfig) */
    chanDataSize = gMmWaveCfg.profileComCfg.numOfAdcSamples * 2;  /* Complex samples */
    chanDataSizeAligned16 = ((chanDataSize + 15) / 16) * 16;
    
    memset(&rxChanConf, 0, sizeof(ADCBuf_RxChanConf));
    
    for (channel = 0; channel < SYS_COMMON_NUM_RX_CHANNEL; channel++)
    {
        if ((gMmWaveCfg.rxEnbl & (3 << (channel * 2))) != 0)
        {
            rxChanConf.channel = channel;
            rxChanConf.offset = offset;
            ADCBuf_control(gAdcBufHandle, ADCBufMMWave_CMD_CHANNEL_ENABLE, (void *)&rxChanConf);
            offset += chanDataSizeAligned16;
        }
    }
    
    DebugP_log("RadarControl: ADCBuf configured, %d bytes per channel\r\n", chanDataSizeAligned16);

    /* Step 2: Configure APLL (SDK Standard - 问题36修复) */
    /* 使用新的RadarControl_configAndEnableApll()函数 */
    float apllFreq;
    uint8_t saveRestoreMode;
    
    /* 确定APLL频率和校准模式 */
    if (gHealthDetectMCB.apllFreqShiftEnable == 1)
    {
        /* 启用频率偏移：使用396MHz */
        apllFreq = 396.0f;
        /* 如果是第一次配置，SAVE校准数据；否则RESTORE */
        saveRestoreMode = (gHealthDetectMCB.oneTimeConfigDone == 0) ? 1 : 0;
    }
    else
    {
        /* 不启用频率偏移：使用400MHz */
        apllFreq = 400.0f;
        /* 如果是第一次配置，SAVE校准数据；否则RESTORE */
        saveRestoreMode = (gHealthDetectMCB.oneTimeConfigDone == 0) ? 1 : 0;
    }
    
    retVal = RadarControl_configAndEnableApll(apllFreq, saveRestoreMode);
    if (retVal != 0)
    {
        DebugP_log("RadarControl: APLL configuration failed [retVal=%d]\r\n", retVal);
        return retVal;
    }
    
    DebugP_log("RadarControl: APLL configured at %.1f MHz\r\n", apllFreq);

    /* Step 3: Factory Calibration (SDK Standard - 关键步骤！) */
    /* 参考：mmw_demo/calibrations/factory_cal.c 的 mmwDemo_factoryCal() */
    /* 工厂校准必须在MMWave_open之前调用 */
    /* 注意：使用本地变量判断，因为oneTimeConfigDone还未设置 */
    if (saveRestoreMode == 1)  /* SAVE模式 = 第一次启动 = 需要校准 */
    {
        /* 🟢 第9轮修复：检查calibCfg是否通过CLI命令配置 */
        /* flashOffset非0表示用户发送了factoryCalibCfg命令 */
        if (gHealthDetectMCB.calibCfg.flashOffset != 0)
        {
            /* 已配置：执行工厂校准 */
            DebugP_log("RadarControl: Performing factory calibration...\r\n");
            
            /* 设置校准配置到gMmWaveCfg（SDK标准流程）*/
            gMmWaveCfg.calibCfg.saveEnable = gHealthDetectMCB.calibCfg.saveEnable;
            gMmWaveCfg.calibCfg.restoreEnable = gHealthDetectMCB.calibCfg.restoreEnable;
            gMmWaveCfg.calibCfg.rxGain = gHealthDetectMCB.calibCfg.rxGain;
            gMmWaveCfg.calibCfg.txBackoffSel = gHealthDetectMCB.calibCfg.txBackoffSel;
            gMmWaveCfg.calibCfg.flashOffset = gHealthDetectMCB.calibCfg.flashOffset;
            gMmWaveCfg.calibCfg.monitorsFlashOffset = gHealthDetectMCB.calibCfg.monitorsFlashOffset;
            
            /* 设置工厂校准数据缓冲区指针（SDK要求）*/
            /* MMWave_factoryCalib需要此指针来存储/恢复校准结果 */
            gMmWaveCfg.calibCfg.ptrFactoryCalibData = &gHealthDetectMCB.factoryCalibData;
            
            DebugP_log("RadarControl: CalibCfg - saveEnable=%d, restoreEnable=%d, rxGain=%d, flashOffset=0x%x\r\n",
                       gMmWaveCfg.calibCfg.saveEnable, 
                       gMmWaveCfg.calibCfg.restoreEnable,
                       gMmWaveCfg.calibCfg.rxGain,
                       gMmWaveCfg.calibCfg.flashOffset);
            
            retVal = MMWave_factoryCalib(gMmWaveHandle, &gMmWaveCfg, &errCode);
            if (retVal != 0)
            {
                DebugP_log("RadarControl: MMWave_factoryCalib failed, errCode=%d\r\n", errCode);
                /* 解码错误用于调试 */
                MMWave_ErrorLevel errorLevel;
                int16_t mmWaveErrorCode;
                int16_t subsysErrorCode;
                MMWave_decodeError(errCode, &errorLevel, &mmWaveErrorCode, &subsysErrorCode);
                DebugP_log("  errorLevel=%d, mmWaveErrorCode=%d, subsysErrorCode=%d\r\n", 
                           errorLevel, mmWaveErrorCode, subsysErrorCode);
                return errCode;
            }
            DebugP_log("RadarControl: Factory calibration completed\r\n");
        }
        else
        {
            /* 🟢 未配置：跳过工厂校准（用户未发送factoryCalibCfg命令）*/
            DebugP_log("RadarControl: Factory calibration skipped (not configured via CLI)\r\n");
            DebugP_log("  Note: Use 'factoryCalibCfg' command if calibration is required\r\n");
        }
    }
    else
    {
        DebugP_log("RadarControl: Factory calibration skipped (warm start)\r\n");
    }

    /* 标记已完成一次配置（后续可以RESTORE校准数据） */
    gHealthDetectMCB.oneTimeConfigDone = 1;

    /* Step 4: Turn on RF power for TX/RX channels */
    retVal = MMWave_FecssRfPwrOnOff(gMmWaveCfg.txEnbl, gMmWaveCfg.rxEnbl, &errCode);
    if (retVal != 0)
    {
        DebugP_log("RadarControl: MMWave_FecssRfPwrOnOff failed, errCode=%d\r\n", errCode);
        return errCode;
    }
    
    DebugP_log("RadarControl: RF power enabled (tx=0x%04X, rx=0x%04X)\r\n", 
               gMmWaveCfg.txEnbl, gMmWaveCfg.rxEnbl);

    /* Step 5: Monitor Configuration (SDK Standard - 可选) */
    /* 注意：监控器配置在L-SDK中是可选的，通过CLI命令配置 */
    /* 如果需要，使用：MMWave_configMonitors() */
    DebugP_log("RadarControl: Monitor configuration skipped (configured via CLI if needed)\r\n");

    /* Step 6: Open mmWave device (if not already open) */
    if (!gRadarOpened)
    {
        if (MMWave_open(gMmWaveHandle, &gMmWaveCfg, &errCode) < 0)
        {
            DebugP_log("RadarControl: MMWave_open failed, errCode=%d\r\n", errCode);
            return errCode;
        }
        gRadarOpened = 1;
        DebugP_log("RadarControl: MMWave opened\r\n");
    }

    /* Step 7: Configure mmWave */
    if (MMWave_config(gMmWaveHandle, &gMmWaveCfg, &errCode) < 0)
    {
        DebugP_log("RadarControl: MMWave_config failed, errCode=%d\r\n", errCode);
        return errCode;
    }
    DebugP_log("RadarControl: MMWave configured\r\n");

    /* Step 8: Start mmWave sensor */
    if (MMWave_start(gMmWaveHandle, &gMmWaveCfg.strtCfg, &errCode) < 0)
    {
        DebugP_log("RadarControl: MMWave_start failed, errCode=%d\r\n", errCode);
        return errCode;
    }

    /* 增加传感器启动计数 */
    gHealthDetectMCB.sensorStartCount++;

    DebugP_log("RadarControl: Started successfully! (count=%d)\r\n", gHealthDetectMCB.sensorStartCount);
    
    return 0;
}

/**
 * @brief Stop radar sensor
 * 
 * L-SDK 6.x: Uses MMWave_stop(handle, MMWave_StrtCfg*, errCode*)
 */
int32_t RadarControl_stop(void)
{
    int32_t errCode;

    DebugP_log("RadarControl: Stopping...\r\n");

    /* Stop mmWave sensor using L-SDK 6.x API */
    if (MMWave_stop(gMmWaveHandle, &gMmWaveCfg.strtCfg, &errCode) < 0)
    {
        DebugP_log("RadarControl: MMWave_stop failed, errCode=%d\r\n", errCode);
        return errCode;
    }

    /* 增加传感器停止计数 */
    gHealthDetectMCB.sensorStopCount++;

    DebugP_log("RadarControl: Stopped (count=%d)\r\n", gHealthDetectMCB.sensorStopCount);

    return 0;
}

/**
 * @brief Get mmWave handle
 */
MMWave_Handle RadarControl_getHandle(void)
{
    return gMmWaveHandle;
}

/**
 * @brief Get mmWave configuration pointer
 */
MMWave_Cfg* RadarControl_getCfg(void)
{
    return &gMmWaveCfg;
}

/**
 * @brief Check if radar is initialized
 */
uint8_t RadarControl_isInitialized(void)
{
    return gRadarInitialized;
}

/**
 * @brief Check if radar is opened
 */
uint8_t RadarControl_isOpened(void)
{
    return gRadarOpened;
}

/**
 * @brief Check if radar is configured
 */
uint8_t RadarControl_isConfigured(void)
{
    return gRadarConfigured;
}
