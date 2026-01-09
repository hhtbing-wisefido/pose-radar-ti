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
    gHealthDetectMCB.mmWaveHandle = gMmWaveHandle;
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
int32_t RadarControl_config(HealthDetect_CliCfg_t *cliCfg)
{
    if (cliCfg == NULL)
    {
        return -1;
    }

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

    /* Configure TX/RX enable - use fields from CliCfg channelCfg command */
    gMmWaveCfg.txEnbl = cliCfg->txChannelEn;
    gMmWaveCfg.rxEnbl = cliCfg->rxChannelEn;
    
    /* Store APLL frequency (400MHz default, 396MHz if shift enabled) */
    gMmWaveCfg.apllFreqMHz = 400.0f;  /* Default 400MHz */

    gRadarConfigured = 1;

    DebugP_log("RadarControl: Configuration saved (tx=0x%04X, rx=0x%04X)\r\n", 
               gMmWaveCfg.txEnbl, gMmWaveCfg.rxEnbl);

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

    /* Step 2: Configure APLL (like SDK MmwDemo_configAndEnableApll) */
    /* Turn off APLL first */
    retVal = MMWave_FecssDevClockCtrl(&gMmWaveCfg.initCfg, 0, &errCode);  /* 0 = disable */
    if (retVal != 0)
    {
        DebugP_log("RadarControl: APLL disable failed, errCode=%d\r\n", errCode);
        /* Continue anyway - may not be critical */
    }
    
    /* Configure APLL registers for 400MHz */
    retVal = MMWave_ConfigApllReg(400.0f);
    if (retVal != 0)
    {
        DebugP_log("RadarControl: APLL config failed, retVal=%d\r\n", retVal);
        /* Continue anyway */
    }
    
    /* Turn on APLL */
    retVal = MMWave_FecssDevClockCtrl(&gMmWaveCfg.initCfg, 1, &errCode);  /* 1 = enable */
    if (retVal != 0)
    {
        DebugP_log("RadarControl: APLL enable failed, errCode=%d\r\n", errCode);
        /* Continue anyway */
    }
    
    DebugP_log("RadarControl: APLL configured at 400MHz\r\n");

    /* Step 3: Turn on RF power for TX/RX channels */
    retVal = MMWave_FecssRfPwrOnOff(gMmWaveCfg.txEnbl, gMmWaveCfg.rxEnbl, &errCode);
    if (retVal != 0)
    {
        DebugP_log("RadarControl: MMWave_FecssRfPwrOnOff failed, errCode=%d\r\n", errCode);
        return errCode;
    }
    
    DebugP_log("RadarControl: RF power enabled (tx=0x%04X, rx=0x%04X)\r\n", 
               gMmWaveCfg.txEnbl, gMmWaveCfg.rxEnbl);

    /* Step 4: Open mmWave device (if not already open) */
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

    /* Step 5: Configure mmWave */
    if (MMWave_config(gMmWaveHandle, &gMmWaveCfg, &errCode) < 0)
    {
        DebugP_log("RadarControl: MMWave_config failed, errCode=%d\r\n", errCode);
        return errCode;
    }
    DebugP_log("RadarControl: MMWave configured\r\n");

    /* Step 6: Start mmWave sensor */
    if (MMWave_start(gMmWaveHandle, &gMmWaveCfg.strtCfg, &errCode) < 0)
    {
        DebugP_log("RadarControl: MMWave_start failed, errCode=%d\r\n", errCode);
        return errCode;
    }

    DebugP_log("RadarControl: Started successfully!\r\n");

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

    DebugP_log("RadarControl: Stopped\r\n");

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
