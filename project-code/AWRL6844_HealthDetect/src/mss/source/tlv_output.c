/**
 * @file tlv_output.c
 * @brief TLV Output Module Implementation
 *
 * Reference: mmw_demo_SDK_reference/source/mmwave_demo.c (TLV output section)
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
#include <kernel/dpl/CacheP.h>

/* Driver Includes */
#include <drivers/uart.h>

/* Application Includes */
#include <source/tlv_output.h>
#include <source/health_detect_main.h>

/**************************************************************************
 *************************** Local Definitions ****************************
 **************************************************************************/

/** @brief Output buffer size */
#define TLV_OUTPUT_BUF_SIZE         (MMWAVE_OUTPUT_MAX_PACKET_SIZE)

/**************************************************************************
 *************************** Local Variables ******************************
 **************************************************************************/

/** @brief UART handle */
static UART_Handle gTlvUartHandle = NULL;

/** @brief Output buffer */
static uint8_t gTlvOutputBuf[TLV_OUTPUT_BUF_SIZE] __attribute__((aligned(32)));

/** @brief Initialized flag */
static uint8_t gTlvInitialized = 0;

/** @brief TLV enable flags */
static uint8_t gTlvEnabled[MMWDEMO_OUTPUT_MSG_MAX] = {0};

/**************************************************************************
 *************************** Local Functions ******************************
 **************************************************************************/

/**
 * @brief Build message header
 */
static uint32_t TLV_buildHeader(uint8_t *buf, uint32_t frameNum, 
                                 uint32_t numObjects, uint32_t numTLVs)
{
    MmwDemo_output_message_header_t *header = (MmwDemo_output_message_header_t *)buf;

    /* Set magic word */
    header->magicWord[0] = MMWAVE_OUTPUT_MAGIC_WORD_0;
    header->magicWord[1] = MMWAVE_OUTPUT_MAGIC_WORD_1;
    header->magicWord[2] = MMWAVE_OUTPUT_MAGIC_WORD_2;
    header->magicWord[3] = MMWAVE_OUTPUT_MAGIC_WORD_3;

    /* Set header fields */
    header->version = 0x01000000; /* Version 1.0.0.0 */
    header->totalPacketLen = 0;   /* Will be updated later */
    header->platform = MMWAVE_OUTPUT_PLATFORM_AWRL6844;
    header->frameNumber = frameNum;
    header->timeCpuCycles = ClockP_getTimeUsec();
    header->numDetectedObj = numObjects;
    header->numTLVs = numTLVs;
    header->subFrameNumber = 0;

    return sizeof(MmwDemo_output_message_header_t);
}

/**
 * @brief Add point cloud TLV
 */
static uint32_t TLV_addPointCloud(uint8_t *buf, PointCloud_Output_t *pointCloud)
{
    MmwDemo_output_message_tl_t *tlHeader;
    MmwDemo_output_message_point_unit_t *pointUnit;
    MmwDemo_output_message_UARTpoint_t *points;
    uint32_t offset = 0;
    uint32_t i;

    if (pointCloud->numDetectedPoints == 0)
    {
        return 0;
    }

    /* TLV header */
    tlHeader = (MmwDemo_output_message_tl_t *)buf;
    tlHeader->type = MMWDEMO_OUTPUT_MSG_DETECTED_POINTS;
    offset += sizeof(MmwDemo_output_message_tl_t);

    /* Point units */
    pointUnit = (MmwDemo_output_message_point_unit_t *)(buf + offset);
    pointUnit->xyzUnit = 0.001f;      /* 1mm resolution */
    pointUnit->dopplerUnit = 0.01f;   /* 0.01 m/s resolution */
    pointUnit->snrUnit = 0.1f;        /* 0.1 dB resolution */
    pointUnit->noiseUnit = 0.1f;      /* 0.1 dB resolution */
    pointUnit->numDetectedPoints = (uint16_t)pointCloud->numDetectedPoints;
    pointUnit->reserved = 0;
    offset += sizeof(MmwDemo_output_message_point_unit_t);

    /* Points */
    points = (MmwDemo_output_message_UARTpoint_t *)(buf + offset);
    for (i = 0; i < pointCloud->numDetectedPoints; i++)
    {
        points[i].x = (int16_t)(pointCloud->points[i].x / pointUnit->xyzUnit);
        points[i].y = (int16_t)(pointCloud->points[i].y / pointUnit->xyzUnit);
        points[i].z = (int16_t)(pointCloud->points[i].z / pointUnit->xyzUnit);
        points[i].doppler = (int16_t)(pointCloud->points[i].velocity / pointUnit->dopplerUnit);

        if (pointCloud->sideInfo != NULL)
        {
            points[i].snr = (uint8_t)(pointCloud->sideInfo[i].snr / 10);
            points[i].noise = (uint8_t)(pointCloud->sideInfo[i].noise / 10);
        }
        else
        {
            points[i].snr = 0;
            points[i].noise = 0;
        }
    }
    offset += pointCloud->numDetectedPoints * sizeof(MmwDemo_output_message_UARTpoint_t);

    /* Update TLV length */
    tlHeader->length = offset - sizeof(MmwDemo_output_message_tl_t);

    return offset;
}

/**
 * @brief Add presence detection TLV
 */
static uint32_t TLV_addPresence(uint8_t *buf, PresenceDetect_Result_t *presence)
{
    MmwDemo_output_message_tl_t *tlHeader;
    MmwDemo_output_message_presence_t *presenceData;
    uint32_t offset = 0;

    /* TLV header */
    tlHeader = (MmwDemo_output_message_tl_t *)buf;
    tlHeader->type = MMWDEMO_OUTPUT_MSG_PRESENCE_DETECT;
    tlHeader->length = sizeof(MmwDemo_output_message_presence_t);
    offset += sizeof(MmwDemo_output_message_tl_t);

    /* Presence data */
    presenceData = (MmwDemo_output_message_presence_t *)(buf + offset);
    presenceData->isPresent = presence->isPresent;
    presenceData->isMoving = presence->isMoving;
    presenceData->presenceState = (uint8_t)presence->state;
    presenceData->reserved = 0;
    presenceData->numPointsInZone = presence->numPointsInZone;
    presenceData->presenceCounter = presence->presenceCounter;
    presenceData->avgRange_m = presence->avgRange_m;
    presenceData->avgVelocity_mps = presence->avgVelocity_mps;
    presenceData->avgAzimuth_deg = presence->avgAzimuth_deg;
    presenceData->avgElevation_deg = presence->avgElevation_deg;
    offset += sizeof(MmwDemo_output_message_presence_t);

    return offset;
}

/**
 * @brief Add health features TLV
 */
static uint32_t TLV_addHealthFeatures(uint8_t *buf, HealthDetect_Features_t *features)
{
    MmwDemo_output_message_tl_t *tlHeader;
    MmwDemo_output_message_health_features_t *featData;
    uint32_t offset = 0;

    /* TLV header */
    tlHeader = (MmwDemo_output_message_tl_t *)buf;
    tlHeader->type = MMWDEMO_OUTPUT_MSG_HEALTH_FEATURES;
    tlHeader->length = sizeof(MmwDemo_output_message_health_features_t);
    offset += sizeof(MmwDemo_output_message_tl_t);

    /* Health features data */
    featData = (MmwDemo_output_message_health_features_t *)(buf + offset);
    featData->frameNum = features->frameNum;
    featData->motionEnergy = features->motionFeatures.motionEnergy;
    featData->motionEnergySmoothed = features->motionFeatures.motionEnergySmoothed;
    featData->peakSnr_dB = features->peakSnr_dB;
    featData->avgSnr_dB = features->avgSnr_dB;
    featData->signalQuality = features->signalQuality;
    featData->numValidPoints = features->pointCloudFeatures.numPoints;
    featData->healthState = (uint8_t)features->healthState;
    featData->reserved = 0;
    offset += sizeof(MmwDemo_output_message_health_features_t);

    return offset;
}

/**
 * @brief Add statistics TLV
 */
static uint32_t TLV_addStats(uint8_t *buf, DPC_Stats_t *stats)
{
    MmwDemo_output_message_tl_t *tlHeader;
    MmwDemo_output_message_stats_t *statsData;
    uint32_t offset = 0;

    /* TLV header */
    tlHeader = (MmwDemo_output_message_tl_t *)buf;
    tlHeader->type = MMWDEMO_OUTPUT_MSG_STATS;
    tlHeader->length = sizeof(MmwDemo_output_message_stats_t);
    offset += sizeof(MmwDemo_output_message_tl_t);

    /* Stats data */
    statsData = (MmwDemo_output_message_stats_t *)(buf + offset);
    statsData->interFrameProcessingTimeUs = stats->interFrameTimeUs;
    statsData->transmitOutputTimeUs = stats->transmitTimeUs;
    memset(statsData->powerMeasured, 0, sizeof(statsData->powerMeasured));
    memset(statsData->tempReading, 0, sizeof(statsData->tempReading));
    offset += sizeof(MmwDemo_output_message_stats_t);

    return offset;
}

/**************************************************************************
 *************************** Public Functions *****************************
 **************************************************************************/

/**
 * @brief Initialize TLV output module
 */
int32_t TLV_init(UART_Handle uartHandle)
{
    DebugP_log("TLV: Initializing...\r\n");

    /* Store UART handle */
    gTlvUartHandle = uartHandle;

    /* Clear output buffer */
    memset(gTlvOutputBuf, 0, sizeof(gTlvOutputBuf));

    /* Enable default TLVs */
    gTlvEnabled[MMWDEMO_OUTPUT_MSG_DETECTED_POINTS] = 1;
    gTlvEnabled[MMWDEMO_OUTPUT_MSG_STATS] = 1;
    gTlvEnabled[MMWDEMO_OUTPUT_MSG_PRESENCE_DETECT] = 1;
    gTlvEnabled[MMWDEMO_OUTPUT_MSG_HEALTH_FEATURES] = 1;

    gTlvInitialized = 1;

    DebugP_log("TLV: Initialized\r\n");

    return 0;
}

/**
 * @brief Send output data via UART
 */
int32_t TLV_sendOutput(uint32_t frameNum,
                       DPC_Result_t *dpcResult,
                       PresenceDetect_Result_t *presenceResult,
                       HealthDetect_Features_t *healthFeatures)
{
    uint32_t offset = 0;
    uint32_t numTLVs = 0;
    MmwDemo_output_message_header_t *header;

    if (!gTlvInitialized)
    {
        return -1;
    }

    /* Build header (will update later) */
    offset = TLV_buildHeader(gTlvOutputBuf, frameNum, 
                              dpcResult ? dpcResult->pointCloud.numDetectedPoints : 0,
                              0); /* numTLVs updated later */

    /* Add point cloud TLV */
    if (gTlvEnabled[MMWDEMO_OUTPUT_MSG_DETECTED_POINTS] && dpcResult != NULL)
    {
        uint32_t tlvLen = TLV_addPointCloud(gTlvOutputBuf + offset, &dpcResult->pointCloud);
        if (tlvLen > 0)
        {
            offset += tlvLen;
            numTLVs++;
        }
    }

    /* Add presence detection TLV */
    if (gTlvEnabled[MMWDEMO_OUTPUT_MSG_PRESENCE_DETECT] && presenceResult != NULL)
    {
        offset += TLV_addPresence(gTlvOutputBuf + offset, presenceResult);
        numTLVs++;
    }

    /* Add health features TLV */
    if (gTlvEnabled[MMWDEMO_OUTPUT_MSG_HEALTH_FEATURES] && healthFeatures != NULL)
    {
        offset += TLV_addHealthFeatures(gTlvOutputBuf + offset, healthFeatures);
        numTLVs++;
    }

    /* Add statistics TLV */
    if (gTlvEnabled[MMWDEMO_OUTPUT_MSG_STATS] && dpcResult != NULL)
    {
        offset += TLV_addStats(gTlvOutputBuf + offset, &dpcResult->stats);
        numTLVs++;
    }

    /* Update header */
    header = (MmwDemo_output_message_header_t *)gTlvOutputBuf;
    header->totalPacketLen = offset;
    header->numTLVs = numTLVs;

    /* Send via UART */
    if (gTlvUartHandle != NULL && offset > 0)
    {
        UART_write(gTlvUartHandle, gTlvOutputBuf, offset, NULL);
    }

    return 0;
}

/**
 * @brief Set UART handle for output
 */
void TLV_setUartHandle(UART_Handle uartHandle)
{
    gTlvUartHandle = uartHandle;
}

/**
 * @brief Enable/disable TLV types
 */
void TLV_setEnabled(MmwDemo_output_message_type_e tlvType, uint8_t enable)
{
    if (tlvType < MMWDEMO_OUTPUT_MSG_MAX)
    {
        gTlvEnabled[tlvType] = enable ? 1 : 0;
    }
}
