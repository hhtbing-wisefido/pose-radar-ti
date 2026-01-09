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
 * and calling MMWave_config(handle, MMWave_Cfg*, errCode*)
 * 
 * Note: In L-SDK 6.x, there are no separate MMWave_addProfile, MMWave_addChirp,
 * MMWave_setFrameCfg functions. Configuration is done through MMWave_Cfg structure.
 */
int32_t RadarControl_config(HealthDetect_CliCfg_t *cliCfg)
{
    int32_t errCode;

    if (cliCfg == NULL)
    {
        return -1;
    }

    if (!gRadarOpened)
    {
        /* Open radar first */
        errCode = RadarControl_open();
        if (errCode != 0)
        {
            return errCode;
        }
    }

    DebugP_log("RadarControl: Configuring...\r\n");

    /* Configure profile common parameters */
    gMmWaveCfg.profileComCfg.startFreqGHz = cliCfg->profileCfg.startFreqGHz;
    gMmWaveCfg.profileComCfg.digOutSampleRateMHz = (float)cliCfg->profileCfg.digOutSampleRate / 1000.0f;
    gMmWaveCfg.profileComCfg.numAdcSamples = cliCfg->profileCfg.numAdcSamples;
    gMmWaveCfg.profileComCfg.rxGain = cliCfg->profileCfg.rxGain;

    /* Configure profile timing parameters */
    gMmWaveCfg.profileTimeCfg.idleTimeus = cliCfg->profileCfg.idleTimeUs;
    gMmWaveCfg.profileTimeCfg.adcStartTimeus = cliCfg->profileCfg.adcStartTimeUs;
    gMmWaveCfg.profileTimeCfg.rampEndTimeus = cliCfg->profileCfg.rampEndTimeUs;
    gMmWaveCfg.profileTimeCfg.freqSlopeConst = cliCfg->profileCfg.freqSlopeConst;

    /* Configure frame parameters */
    gMmWaveCfg.frameCfg.numOfChirpsInBurst = (cliCfg->frameCfg.chirpEndIdx - cliCfg->frameCfg.chirpStartIdx + 1);
    gMmWaveCfg.frameCfg.numOfBurstsInFrame = cliCfg->frameCfg.numLoops;
    gMmWaveCfg.frameCfg.numOfFrames = cliCfg->frameCfg.numFrames;
    gMmWaveCfg.frameCfg.framePeriodicityus = (uint32_t)(cliCfg->frameCfg.framePeriodMs * 1000.0f);

    /* Configure TX/RX enable */
    gMmWaveCfg.txEnbl = cliCfg->chirpCfg.txEnable;
    gMmWaveCfg.rxEnbl = cliCfg->channelCfg.rxChannelEn;

    /* Apply configuration using L-SDK 6.x API */
    if (MMWave_config(gMmWaveHandle, &gMmWaveCfg, &errCode) < 0)
    {
        DebugP_log("RadarControl: MMWave_config failed, errCode=%d\r\n", errCode);
        return errCode;
    }

    gRadarConfigured = 1;

    DebugP_log("RadarControl: Configured\r\n");

    return 0;
}

/**
 * @brief Start radar sensor
 * 
 * L-SDK 6.x: Uses MMWave_start(handle, MMWave_StrtCfg*, errCode*)
 */
int32_t RadarControl_start(void)
{
    int32_t errCode;

    if (!gRadarConfigured)
    {
        DebugP_log("RadarControl: Not configured\r\n");
        return -1;
    }

    DebugP_log("RadarControl: Starting...\r\n");

    /* Start mmWave sensor using L-SDK 6.x API */
    if (MMWave_start(gMmWaveHandle, &gMmWaveCfg.strtCfg, &errCode) < 0)
    {
        DebugP_log("RadarControl: MMWave_start failed, errCode=%d\r\n", errCode);
        return errCode;
    }

    DebugP_log("RadarControl: Started\r\n");

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
