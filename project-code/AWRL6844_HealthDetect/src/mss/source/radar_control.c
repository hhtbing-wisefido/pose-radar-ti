/**
 * @file radar_control.c
 * @brief Radar Control Module Implementation (mmWave API Wrapper)
 *
 * Reference: mmw_demo_SDK_reference/source/mmwave_control/
 * Reference: AWRL6844_InCabin_Demos/src/mss/source/mmwave_control/
 * Adapted for: Health Detection three-layer architecture
 *
 * RTOS: FreeRTOS (L-SDK mandatory)
 * Note: L-SDK uses FreeRTOS, NOT TI-RTOS/BIOS
 *
 * Created: 2026-01-08
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

/* Application Includes */
#include <source/radar_control.h>
#include <source/health_detect_main.h>

/**************************************************************************
 *************************** Local Definitions ****************************
 **************************************************************************/

/** @brief AWRL6844 frequency limits */
#define RADAR_FREQ_LIMIT_LOW        (60000U)    /* 60 GHz */
#define RADAR_FREQ_LIMIT_HIGH       (64000U)    /* 64 GHz */

/**************************************************************************
 *************************** Local Variables ******************************
 **************************************************************************/

/** @brief mmWave handle */
static MMWave_Handle gMmWaveHandle = NULL;

/** @brief Radar initialized flag */
static uint8_t gRadarInitialized = 0;

/** @brief Radar opened flag */
static uint8_t gRadarOpened = 0;

/** @brief Radar configured flag */
static uint8_t gRadarConfigured = 0;

/**************************************************************************
 *************************** Local Functions ******************************
 **************************************************************************/

/**
 * @brief mmWave event callback
 */
static void RadarControl_eventCallback(uint32_t event, void *arg)
{
    /* Handle events */
    switch (event)
    {
        case MMWAVE_EVENT_FRAME_START:
            HealthDetect_frameStartCallback(arg);
            break;

        case MMWAVE_EVENT_FRAME_STOP:
            HealthDetect_frameStopCallback(arg);
            break;

        default:
            break;
    }
}

/**
 * @brief mmWave error callback
 */
static void RadarControl_errorCallback(int32_t errCode, void *arg)
{
    DebugP_log("RadarControl: Error callback, code=%d\r\n", errCode);
    gHealthDetectMCB.errorCode = errCode;
}

/**************************************************************************
 *************************** Public Functions *****************************
 **************************************************************************/

/**
 * @brief Initialize radar control module
 */
int32_t RadarControl_init(void)
{
    MMWave_InitCfg initCfg;
    int32_t errCode;

    DebugP_log("RadarControl: Initializing...\r\n");

    /* Initialize mmWave configuration */
    memset(&initCfg, 0, sizeof(MMWave_InitCfg));

    /* Set callbacks */
    initCfg.eventFxn = RadarControl_eventCallback;
    initCfg.errorFxn = RadarControl_errorCallback;

    /* Initialize mmWave */
    gMmWaveHandle = MMWave_init(&initCfg, &errCode);
    if (gMmWaveHandle == NULL)
    {
        DebugP_log("RadarControl: MMWave_init failed, errCode=%d\r\n", errCode);
        return errCode;
    }

    /* Store handle in MCB */
    gHealthDetectMCB.mmWaveHandle = gMmWaveHandle;

    gRadarInitialized = 1;

    DebugP_log("RadarControl: Initialized\r\n");

    return 0;
}

/**
 * @brief Open radar device
 */
int32_t RadarControl_open(void)
{
    MMWave_OpenCfg openCfg;
    int32_t errCode;

    if (!gRadarInitialized)
    {
        DebugP_log("RadarControl: Not initialized\r\n");
        return -1;
    }

    DebugP_log("RadarControl: Opening...\r\n");

    /* Initialize open configuration */
    memset(&openCfg, 0, sizeof(MMWave_OpenCfg));

    /* Set frequency limits for AWRL6844 */
    openCfg.freqLimitLow = RADAR_FREQ_LIMIT_LOW;
    openCfg.freqLimitHigh = RADAR_FREQ_LIMIT_HIGH;

    /* Open mmWave */
    errCode = MMWave_open(gMmWaveHandle, &openCfg);
    if (errCode != 0)
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
 */
int32_t RadarControl_close(void)
{
    int32_t errCode;

    if (!gRadarOpened)
    {
        return 0;
    }

    DebugP_log("RadarControl: Closing...\r\n");

    /* Close mmWave */
    errCode = MMWave_close(gMmWaveHandle);
    if (errCode != 0)
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
 */
int32_t RadarControl_config(HealthDetect_CliCfg_t *cliCfg)
{
    MMWave_ProfileCfg profileCfg;
    MMWave_ChirpCfg chirpCfg;
    MMWave_FrameCfg frameCfg;
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

    /* Configure profile */
    memset(&profileCfg, 0, sizeof(MMWave_ProfileCfg));
    profileCfg.profileId = cliCfg->profileCfg.profileId;
    profileCfg.startFrequency = (uint32_t)(cliCfg->profileCfg.startFreqGHz * 1000000.0f);
    profileCfg.idleTimeConst = (uint32_t)(cliCfg->profileCfg.idleTimeUs * 10.0f);
    profileCfg.adcStartTimeConst = (uint32_t)(cliCfg->profileCfg.adcStartTimeUs * 10.0f);
    profileCfg.rampEndTime = (uint32_t)(cliCfg->profileCfg.rampEndTimeUs * 10.0f);
    profileCfg.freqSlopeConst = (int16_t)(cliCfg->profileCfg.freqSlopeConst);
    profileCfg.numAdcSamples = cliCfg->profileCfg.numAdcSamples;
    profileCfg.digOutSampleRate = cliCfg->profileCfg.digOutSampleRate;
    profileCfg.rxGain = cliCfg->profileCfg.rxGain;

    errCode = MMWave_addProfile(gMmWaveHandle, &profileCfg);
    if (errCode != 0)
    {
        DebugP_log("RadarControl: MMWave_addProfile failed, errCode=%d\r\n", errCode);
        return errCode;
    }

    /* Configure chirp */
    memset(&chirpCfg, 0, sizeof(MMWave_ChirpCfg));
    chirpCfg.chirpStartIdx = cliCfg->chirpCfg.chirpStartIdx;
    chirpCfg.chirpEndIdx = cliCfg->chirpCfg.chirpEndIdx;
    chirpCfg.profileId = cliCfg->chirpCfg.profileId;
    chirpCfg.txEnMask = cliCfg->chirpCfg.txEnable;

    errCode = MMWave_addChirp(gMmWaveHandle, &chirpCfg);
    if (errCode != 0)
    {
        DebugP_log("RadarControl: MMWave_addChirp failed, errCode=%d\r\n", errCode);
        return errCode;
    }

    /* Configure frame */
    memset(&frameCfg, 0, sizeof(MMWave_FrameCfg));
    frameCfg.chirpStartIdx = cliCfg->frameCfg.chirpStartIdx;
    frameCfg.chirpEndIdx = cliCfg->frameCfg.chirpEndIdx;
    frameCfg.numLoops = cliCfg->frameCfg.numLoops;
    frameCfg.numFrames = cliCfg->frameCfg.numFrames;
    frameCfg.framePeriodicity = (uint32_t)(cliCfg->frameCfg.framePeriodMs * 1000000.0f);
    frameCfg.triggerSelect = cliCfg->frameCfg.triggerSelect;

    errCode = MMWave_setFrameCfg(gMmWaveHandle, &frameCfg);
    if (errCode != 0)
    {
        DebugP_log("RadarControl: MMWave_setFrameCfg failed, errCode=%d\r\n", errCode);
        return errCode;
    }

    gRadarConfigured = 1;

    DebugP_log("RadarControl: Configured\r\n");

    return 0;
}

/**
 * @brief Start radar sensor
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

    /* Start mmWave */
    errCode = MMWave_start(gMmWaveHandle);
    if (errCode != 0)
    {
        DebugP_log("RadarControl: MMWave_start failed, errCode=%d\r\n", errCode);
        return errCode;
    }

    DebugP_log("RadarControl: Started\r\n");

    return 0;
}

/**
 * @brief Stop radar sensor
 */
int32_t RadarControl_stop(void)
{
    int32_t errCode;

    DebugP_log("RadarControl: Stopping...\r\n");

    /* Stop mmWave */
    errCode = MMWave_stop(gMmWaveHandle);
    if (errCode != 0)
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
