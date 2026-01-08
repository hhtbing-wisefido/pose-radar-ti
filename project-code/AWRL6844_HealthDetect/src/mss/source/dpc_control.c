/**
 * @file dpc_control.c
 * @brief DPC (Data Path Chain) Control Implementation
 *
 * Reference: mmw_demo_SDK_reference/source/dpc/dpc.c
 * Reference: AWRL6844_InCabin_Demos/src/mss/source/dpc/dpc_mss.c
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
#include <kernel/dpl/CacheP.h>
#include <kernel/dpl/ClockP.h>

/* Application Includes */
#include <source/dpc_control.h>
#include <source/health_detect_main.h>
#include "common/shared_memory.h"

/**************************************************************************
 *************************** Local Definitions ****************************
 **************************************************************************/

/** @brief DPC State */
typedef enum DPC_State_e
{
    DPC_STATE_UNINIT = 0,
    DPC_STATE_INIT,
    DPC_STATE_CONFIGURED,
    DPC_STATE_RUNNING,
    DPC_STATE_MAX
} DPC_State_e;

/**************************************************************************
 *************************** Local Variables ******************************
 **************************************************************************/

/** @brief DPC state */
static DPC_State_e gDpcState = DPC_STATE_UNINIT;

/** @brief DPC configuration copy */
static DPC_Config_t gDpcConfig;

/** @brief Point cloud storage */
static PointCloud_Cartesian_t gPointCloudData[SHARED_MAX_DETECTED_POINTS];

/** @brief Side info storage */
static PointCloud_SideInfo_t gSideInfoData[SHARED_MAX_DETECTED_POINTS];

/**************************************************************************
 *************************** Public Functions *****************************
 **************************************************************************/

/**
 * @brief Initialize DPC module
 */
int32_t DPC_init(void)
{
    DebugP_log("DPC: Initializing...\r\n");

    /* Clear configuration */
    memset(&gDpcConfig, 0, sizeof(DPC_Config_t));

    /* Clear point cloud storage */
    memset(gPointCloudData, 0, sizeof(gPointCloudData));
    memset(gSideInfoData, 0, sizeof(gSideInfoData));

    /* Update state */
    gDpcState = DPC_STATE_INIT;

    DebugP_log("DPC: Initialized\r\n");

    return 0;
}

/**
 * @brief Configure DPC with given parameters
 */
int32_t DPC_config(DPC_Config_t *config)
{
    if (config == NULL)
    {
        DebugP_log("DPC: Error - NULL config\r\n");
        return -1;
    }

    if (gDpcState == DPC_STATE_UNINIT)
    {
        DebugP_log("DPC: Error - Not initialized\r\n");
        return -1;
    }

    DebugP_log("DPC: Configuring...\r\n");

    /* Copy configuration */
    memcpy(&gDpcConfig, config, sizeof(DPC_Config_t));

    /* TODO: Configure HWA for range processing */
    /* TODO: Configure HWA for Doppler processing */
    /* TODO: Configure HWA for CFAR detection */
    /* TODO: Configure HWA for AOA estimation */
    /* TODO: Setup EDMA channels */

    /* Update state */
    gDpcState = DPC_STATE_CONFIGURED;

    DebugP_log("DPC: Configured - Range bins: %d, Doppler bins: %d\r\n",
               gDpcConfig.staticCfg.numRangeBins,
               gDpcConfig.staticCfg.numDopplerBins);

    return 0;
}

/**
 * @brief Execute DPC processing for one frame
 */
int32_t DPC_execute(DPC_Result_t *result)
{
    uint32_t startTimeUs;

    if (result == NULL)
    {
        return -1;
    }

    if (gDpcState != DPC_STATE_CONFIGURED && gDpcState != DPC_STATE_RUNNING)
    {
        DebugP_log("DPC: Error - Not configured\r\n");
        return -1;
    }

    /* Record start time */
    startTimeUs = ClockP_getTimeUsec();

    /* Update state */
    gDpcState = DPC_STATE_RUNNING;

    /* Initialize result */
    memset(result, 0, sizeof(DPC_Result_t));
    result->frameNum = gHealthDetectMCB.frameNum;
    result->pointCloud.points = gPointCloudData;
    result->pointCloud.sideInfo = gSideInfoData;

    /* TODO: Trigger HWA range processing */
    /* TODO: Wait for range processing complete */
    /* TODO: Trigger HWA Doppler processing */
    /* TODO: Wait for Doppler processing complete */
    /* TODO: Run CFAR detection */
    /* TODO: Run AOA estimation */
    /* TODO: Generate point cloud */

    /* For now, generate dummy data for testing */
    /* This will be replaced with actual HWA processing */
    result->pointCloud.numDetectedPoints = 0;
    result->errorCode = 0;

    /* Calculate statistics */
    result->stats.frameStartTimeUs = startTimeUs;
    result->stats.interFrameTimeUs = ClockP_getTimeUsec() - startTimeUs;

    return 0;
}

/**
 * @brief Reconfigure DPC during runtime
 */
int32_t DPC_reconfig(DPC_DynamicConfig_t *dynamicCfg)
{
    if (dynamicCfg == NULL)
    {
        return -1;
    }

    DebugP_log("DPC: Reconfiguring...\r\n");

    /* Update dynamic configuration */
    memcpy(&gDpcConfig.dynamicCfg, dynamicCfg, sizeof(DPC_DynamicConfig_t));

    /* TODO: Reconfigure CFAR thresholds */
    /* TODO: Reconfigure AOA parameters */

    DebugP_log("DPC: Reconfigured\r\n");

    return 0;
}

/**
 * @brief Deinitialize DPC module
 */
int32_t DPC_deinit(void)
{
    DebugP_log("DPC: Deinitializing...\r\n");

    /* TODO: Release HWA resources */
    /* TODO: Release EDMA channels */

    /* Clear state */
    gDpcState = DPC_STATE_UNINIT;

    DebugP_log("DPC: Deinitialized\r\n");

    return 0;
}
