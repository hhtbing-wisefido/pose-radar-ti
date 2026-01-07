/**
 * @file radar_control.c
 * @brief Radar Control Module - mmWave API Wrapper
 * 
 * Encapsulates TI mmWave SDK API calls for radar configuration.
 * Reference: mmw_demo/mmwave_control/
 * Simplified for: Health detection use case
 */

#include "radar_control.h"
#include <ti/control/mmwave/mmwave.h>
#include <ti/sysbios/knl/Semaphore.h>

/*----------------------------------------------------------------------------*/
/* Radar Configuration                                                       */
/*----------------------------------------------------------------------------*/

/* Health Detection Optimized Chirp Configuration */
#define RADAR_START_FREQ_GHZ        60.0f   /* 60 GHz start frequency */
#define RADAR_IDLE_TIME_US          100.0f  /* Idle time between chirps */
#define RADAR_ADC_START_TIME_US     6.0f    /* ADC sampling start time */
#define RADAR_RAMP_END_TIME_US      60.0f   /* Chirp duration */
#define RADAR_FREQ_SLOPE_MHZ_US     70.0f   /* Frequency slope */
#define RADAR_TX_POWER_BACKOFF_DB   0       /* Max TX power */
#define RADAR_ADC_SAMPLES           256     /* ADC samples per chirp */
#define RADAR_SAMPLE_RATE_KSPS      5000    /* 5 Msps */

/* Frame Configuration */
#define RADAR_CHIRPS_PER_FRAME      128     /* 128 chirps per frame */
#define RADAR_FRAME_PERIODICITY_MS  100     /* 100ms = 10 fps */

/*----------------------------------------------------------------------------*/
/* Module State                                                              */
/*----------------------------------------------------------------------------*/

typedef struct {
    MMWave_Handle mmwaveHandle;
    bool initialized;
    bool sensorStarted;
    RadarControl_FrameCallback_t frameCallback;
    Semaphore_Handle configSem;
} RadarControl_State_t;

static RadarControl_State_t gRadarState = {
    .mmwaveHandle = NULL,
    .initialized = false,
    .sensorStarted = false,
    .frameCallback = NULL,
};

/*----------------------------------------------------------------------------*/
/* Private Function Declarations                                             */
/*----------------------------------------------------------------------------*/

static void RadarControl_eventCallback(uint16_t msgId, uint16_t sbId, uint16_t sbLen, uint8_t *payload);
static int32_t RadarControl_configureChirp(void);
static int32_t RadarControl_configureFrame(void);

/*----------------------------------------------------------------------------*/
/* Public API Implementation                                                 */
/*----------------------------------------------------------------------------*/

/**
 * @brief Initialize radar control module
 * 
 * Initializes mmWave SDK and configures radar
 */
int32_t RadarControl_init(void)
{
    MMWave_InitCfg initCfg;
    int32_t errCode;
    Semaphore_Params semParams;
    
    /* Create semaphore for config completion */
    Semaphore_Params_init(&semParams);
    semParams.mode = Semaphore_Mode_BINARY;
    gRadarState.configSem = Semaphore_create(0, &semParams, NULL);
    
    /* Initialize mmWave SDK */
    memset(&initCfg, 0, sizeof(MMWave_InitCfg));
    initCfg.domain = MMWave_Domain_MSS;
    initCfg.socHandle = NULL; /* TODO: Get SOC handle */
    initCfg.eventFxn = RadarControl_eventCallback;
    initCfg.linkCRCCfg.useCRCDriver = 1;
    initCfg.linkCRCCfg.crcChannel = 0;
    
    /* TODO: Initialize mmWave module
     * gRadarState.mmwaveHandle = MMWave_init(&initCfg, &errCode);
     * if (gRadarState.mmwaveHandle == NULL) {
     *     return -1;
     * }
     */
    
    gRadarState.initialized = true;
    
    return 0;
}

/**
 * @brief Configure radar parameters
 * 
 * Sets up chirp profile and frame configuration
 */
int32_t RadarControl_config(void)
{
    int32_t retVal;
    
    if (!gRadarState.initialized) {
        return -1;
    }
    
    /* Configure chirp profile */
    retVal = RadarControl_configureChirp();
    if (retVal < 0) {
        return retVal;
    }
    
    /* Configure frame */
    retVal = RadarControl_configureFrame();
    if (retVal < 0) {
        return retVal;
    }
    
    return 0;
}

/**
 * @brief Start radar sensor
 * 
 * Begins continuous frame acquisition
 */
int32_t RadarControl_start(void)
{
    int32_t errCode;
    
    if (!gRadarState.initialized) {
        return -1;
    }
    
    if (gRadarState.sensorStarted) {
        return 0; /* Already started */
    }
    
    /* TODO: Start mmWave sensor
     * errCode = MMWave_start(gRadarState.mmwaveHandle, NULL);
     * if (errCode < 0) {
     *     return -1;
     * }
     */
    
    gRadarState.sensorStarted = true;
    
    return 0;
}

/**
 * @brief Stop radar sensor
 */
int32_t RadarControl_stop(void)
{
    int32_t errCode;
    
    if (!gRadarState.sensorStarted) {
        return 0; /* Already stopped */
    }
    
    /* TODO: Stop mmWave sensor
     * errCode = MMWave_stop(gRadarState.mmwaveHandle, NULL);
     * if (errCode < 0) {
     *     return -1;
     * }
     */
    
    gRadarState.sensorStarted = false;
    
    return 0;
}

/**
 * @brief Register frame callback
 * 
 * Callback is invoked when new frame data is available
 */
int32_t RadarControl_registerFrameCallback(RadarControl_FrameCallback_t callback)
{
    gRadarState.frameCallback = callback;
    return 0;
}

/*----------------------------------------------------------------------------*/
/* Private Configuration Functions                                           */
/*----------------------------------------------------------------------------*/

/**
 * @brief Configure chirp profile
 * 
 * Reference: mmw_demo chirp configuration
 */
static int32_t RadarControl_configureChirp(void)
{
    /* TODO: Implement chirp configuration
     * 
     * Steps:
     * 1. Create rlProfileCfg_t structure
     * 2. Set frequency, slope, samples, etc.
     * 3. Call rlSetProfileConfig()
     * 
     * Example (pseudocode):
     * rlProfileCfg_t profileCfg;
     * profileCfg.profileId = 0;
     * profileCfg.startFreqConst = RADAR_START_FREQ_GHZ * 1e9 / 53.644;
     * profileCfg.idleTimeConst = RADAR_IDLE_TIME_US * 1000 / 10;
     * profileCfg.adcStartTimeConst = RADAR_ADC_START_TIME_US * 1000 / 10;
     * profileCfg.rampEndTime = RADAR_RAMP_END_TIME_US * 1000 / 10;
     * profileCfg.freqSlopeConst = RADAR_FREQ_SLOPE_MHZ_US * (1<<26) / 3.6;
     * profileCfg.numAdcSamples = RADAR_ADC_SAMPLES;
     * profileCfg.digOutSampleRate = RADAR_SAMPLE_RATE_KSPS;
     * rlSetProfileConfig(0, 1, &profileCfg);
     */
    
    return 0;
}

/**
 * @brief Configure frame timing
 * 
 * Reference: mmw_demo frame configuration
 */
static int32_t RadarControl_configureFrame(void)
{
    /* TODO: Implement frame configuration
     * 
     * Steps:
     * 1. Create rlFrameCfg_t structure
     * 2. Set chirps per frame, frame periodicity
     * 3. Call rlSetFrameConfig()
     * 
     * Example (pseudocode):
     * rlFrameCfg_t frameCfg;
     * frameCfg.chirpStartIdx = 0;
     * frameCfg.chirpEndIdx = RADAR_CHIRPS_PER_FRAME - 1;
     * frameCfg.numLoops = 1;
     * frameCfg.numFrames = 0; // Continuous
     * frameCfg.framePeriodicity = RADAR_FRAME_PERIODICITY_MS * 1000000 / 5;
     * rlSetFrameConfig(0, &frameCfg);
     */
    
    return 0;
}

/*----------------------------------------------------------------------------*/
/* Event Callbacks                                                           */
/*----------------------------------------------------------------------------*/

/**
 * @brief mmWave event callback
 * 
 * Called by mmWave SDK for various events
 */
static void RadarControl_eventCallback(uint16_t msgId, uint16_t sbId, uint16_t sbLen, uint8_t *payload)
{
    /* TODO: Handle mmWave events
     * 
     * Important events:
     * - MMWAVE_EVT_FRAME_START: New frame started
     * - MMWAVE_EVT_CHIRP_EVENT: Chirp completed
     * - MMWAVE_EVT_BSS_FAULT: Fault detected
     */
    
    /* Frame start event */
    if (msgId == 0x0001) { /* MMWAVE_EVT_FRAME_START */
        uint32_t frameNum = *(uint32_t*)payload;
        
        /* Call user callback */
        if (gRadarState.frameCallback != NULL) {
            gRadarState.frameCallback(frameNum);
        }
    }
}
