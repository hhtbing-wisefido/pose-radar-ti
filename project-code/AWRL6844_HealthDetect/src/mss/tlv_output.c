/**
 * @file tlv_output.c
 * @brief TLV (Tag-Length-Value) Output Module - UART Data Transmission
 * 
 * Formats and transmits detection results via UART.
 * Reference: mmw_demo TLV output implementation
 * Rewritten for: Three-layer architecture
 */

#include "tlv_output.h"
#include <mmwave_output.h>
#include <data_path.h>
#include <health_detect_types.h>
#include <drivers/uart.h>
#include <string.h>

/*----------------------------------------------------------------------------*/
/* TLV Output Configuration                                                  */
/*----------------------------------------------------------------------------*/

#define TLV_MAX_PACKET_SIZE     4096  /* Maximum UART packet size */
#define TLV_UART_TIMEOUT_MS     100

/*----------------------------------------------------------------------------*/
/* Module State                                                              */
/*----------------------------------------------------------------------------*/

typedef struct {
    UART_Handle uartHandle;
    uint8_t packetBuffer[TLV_MAX_PACKET_SIZE];
    uint32_t bufferOffset;
    bool initialized;
} TLV_Output_State_t;

static TLV_Output_State_t gTLV_State = {
    .uartHandle = NULL,
    .bufferOffset = 0,
    .initialized = false,
};

/*----------------------------------------------------------------------------*/
/* Private Function Declarations                                             */
/*----------------------------------------------------------------------------*/

static int32_t TLV_writeHeader(
    uint32_t frameNum,
    uint32_t numObjects,
    uint32_t numTLVs);

static int32_t TLV_writeDetectedPoints(
    const DPC_PointCloud_t* pointCloud);

static int32_t TLV_writeHealthFeatures(
    const HealthDetect_PointCloudFeatures_t* features);

static int32_t TLV_writeStats(
    const DPC_Result_t* dpcResult);

static int32_t TLV_transmitPacket(void);

/*----------------------------------------------------------------------------*/
/* Public API Implementation                                                 */
/*----------------------------------------------------------------------------*/

/**
 * @brief Initialize TLV output module
 * 
 * Opens UART port for data transmission
 */
int32_t TLV_Output_init(uint32_t uartInstance)
{
    UART_Params uartParams;
    
    /* Configure UART - high speed for data streaming */
    UART_Params_init(&uartParams);
    uartParams.baudRate = 921600;  /* 921.6 kbps */
    uartParams.writeMode = UART_MODE_BLOCKING;
    uartParams.writeTimeout = TLV_UART_TIMEOUT_MS * 1000; /* microseconds */
    
    /* Open UART instance */
    gTLV_State.uartHandle = UART_open(uartInstance, &uartParams);
    
    if (gTLV_State.uartHandle == NULL) {
        return -1;
    }
    
    gTLV_State.initialized = true;
    gTLV_State.bufferOffset = 0;
    
    return 0;
}

/**
 * @brief Send frame results via UART TLV format
 * 
 * Builds complete TLV packet and transmits
 * 
 * @param frameNum Frame number
 * @param pointCloud Point cloud data from DSS
 * @param features Feature data from DSS
 * @param dpcResult DPC execution statistics
 */
int32_t TLV_Output_sendFrame(
    uint32_t frameNum,
    const DPC_PointCloud_t* pointCloud,
    const HealthDetect_PointCloudFeatures_t* features,
    const DPC_Result_t* dpcResult)
{
    int32_t retVal;
    uint32_t numTLVs = 0;
    
    if (!gTLV_State.initialized) {
        return -1;
    }
    
    /* Reset buffer */
    gTLV_State.bufferOffset = 0;
    
    /* Count TLVs to include */
    if (pointCloud != NULL && pointCloud->numPoints > 0) {
        numTLVs++; /* TLV_TYPE_DETECTED_POINTS */
    }
    if (features != NULL && features->numPoints > 0) {
        numTLVs++; /* TLV_TYPE_HEALTH_FEATURES */
    }
    if (dpcResult != NULL) {
        numTLVs++; /* TLV_TYPE_STATS */
    }
    
    /* Write packet header */
    uint32_t numObjects = (pointCloud != NULL) ? pointCloud->numPoints : 0;
    retVal = TLV_writeHeader(frameNum, numObjects, numTLVs);
    if (retVal < 0) {
        return retVal;
    }
    
    /* Write TLV: Detected Points */
    if (pointCloud != NULL && pointCloud->numPoints > 0) {
        retVal = TLV_writeDetectedPoints(pointCloud);
        if (retVal < 0) {
            return retVal;
        }
    }
    
    /* Write TLV: Health Features */
    if (features != NULL && features->numPoints > 0) {
        retVal = TLV_writeHealthFeatures(features);
        if (retVal < 0) {
            return retVal;
        }
    }
    
    /* Write TLV: Statistics */
    if (dpcResult != NULL) {
        retVal = TLV_writeStats(dpcResult);
        if (retVal < 0) {
            return retVal;
        }
    }
    
    /* Transmit packet via UART */
    retVal = TLV_transmitPacket();
    
    return retVal;
}

/**
 * @brief Close TLV output module
 */
void TLV_Output_deinit(void)
{
    if (gTLV_State.uartHandle != NULL) {
        UART_close(gTLV_State.uartHandle);
        gTLV_State.uartHandle = NULL;
    }
    
    gTLV_State.initialized = false;
}

/*----------------------------------------------------------------------------*/
/* Private TLV Builder Functions                                             */
/*----------------------------------------------------------------------------*/

/**
 * @brief Write packet header
 * 
 * Format: MmwaveOutput_Header_t
 */
static int32_t TLV_writeHeader(
    uint32_t frameNum,
    uint32_t numObjects,
    uint32_t numTLVs)
{
    MmwaveOutput_Header_t* header;
    
    if (gTLV_State.bufferOffset + sizeof(MmwaveOutput_Header_t) > TLV_MAX_PACKET_SIZE) {
        return -1; /* Buffer overflow */
    }
    
    header = (MmwaveOutput_Header_t*)&gTLV_State.packetBuffer[gTLV_State.bufferOffset];
    
    header->magicWord = MMWAVE_OUTPUT_MAGIC_WORD;
    header->version = 0x01000000; /* Version 1.0.0 */
    header->totalPacketLen = 0; /* Will be updated before transmission */
    header->platform = 0x6844; /* AWRL6844 */
    header->frameNumber = frameNum;
    header->timeCpuCycles = 0; /* TODO: Get SOC timestamp */
    header->numDetectedObjects = numObjects;
    header->numTLVs = numTLVs;
    header->subFrameNumber = 0;
    
    gTLV_State.bufferOffset += sizeof(MmwaveOutput_Header_t);
    
    return 0;
}

/**
 * @brief Write TLV Type 1: Detected Points
 */
static int32_t TLV_writeDetectedPoints(
    const DPC_PointCloud_t* pointCloud)
{
    TLV_Header_t* tlvHeader;
    TLV_DetectedPoint_t* tlvPoints;
    uint32_t i;
    uint32_t tlvPayloadSize;
    
    if (pointCloud == NULL || pointCloud->numPoints == 0) {
        return 0;
    }
    
    tlvPayloadSize = pointCloud->numPoints * sizeof(TLV_DetectedPoint_t);
    
    if (gTLV_State.bufferOffset + sizeof(TLV_Header_t) + tlvPayloadSize > TLV_MAX_PACKET_SIZE) {
        return -1; /* Buffer overflow */
    }
    
    /* Write TLV header */
    tlvHeader = (TLV_Header_t*)&gTLV_State.packetBuffer[gTLV_State.bufferOffset];
    tlvHeader->type = TLV_TYPE_DETECTED_POINTS;
    tlvHeader->length = tlvPayloadSize;
    gTLV_State.bufferOffset += sizeof(TLV_Header_t);
    
    /* Write point data */
    tlvPoints = (TLV_DetectedPoint_t*)&gTLV_State.packetBuffer[gTLV_State.bufferOffset];
    
    for (i = 0; i < pointCloud->numPoints; i++) {
        tlvPoints[i].x = pointCloud->points[i].x;
        tlvPoints[i].y = pointCloud->points[i].y;
        tlvPoints[i].z = pointCloud->points[i].z;
        tlvPoints[i].velocity = pointCloud->points[i].velocity;
        tlvPoints[i].snr = 0.0f; /* TODO: Add SNR from detection */
    }
    
    gTLV_State.bufferOffset += tlvPayloadSize;
    
    return 0;
}

/**
 * @brief Write TLV Type 100: Health Features (Custom)
 */
static int32_t TLV_writeHealthFeatures(
    const HealthDetect_PointCloudFeatures_t* features)
{
    TLV_Header_t* tlvHeader;
    HealthDetect_PointCloudFeatures_t* tlvFeatures;
    uint32_t tlvPayloadSize;
    
    if (features == NULL || features->numPoints == 0) {
        return 0;
    }
    
    tlvPayloadSize = sizeof(HealthDetect_PointCloudFeatures_t);
    
    if (gTLV_State.bufferOffset + sizeof(TLV_Header_t) + tlvPayloadSize > TLV_MAX_PACKET_SIZE) {
        return -1;
    }
    
    /* Write TLV header */
    tlvHeader = (TLV_Header_t*)&gTLV_State.packetBuffer[gTLV_State.bufferOffset];
    tlvHeader->type = TLV_TYPE_HEALTH_FEATURES;
    tlvHeader->length = tlvPayloadSize;
    gTLV_State.bufferOffset += sizeof(TLV_Header_t);
    
    /* Write feature data */
    tlvFeatures = (HealthDetect_PointCloudFeatures_t*)&gTLV_State.packetBuffer[gTLV_State.bufferOffset];
    memcpy(tlvFeatures, features, sizeof(HealthDetect_PointCloudFeatures_t));
    
    gTLV_State.bufferOffset += tlvPayloadSize;
    
    return 0;
}

/**
 * @brief Write TLV Type 6: Statistics
 */
static int32_t TLV_writeStats(
    const DPC_Result_t* dpcResult)
{
    TLV_Header_t* tlvHeader;
    TLV_Stats_t* tlvStats;
    uint32_t tlvPayloadSize;
    
    if (dpcResult == NULL) {
        return 0;
    }
    
    tlvPayloadSize = sizeof(TLV_Stats_t);
    
    if (gTLV_State.bufferOffset + sizeof(TLV_Header_t) + tlvPayloadSize > TLV_MAX_PACKET_SIZE) {
        return -1;
    }
    
    /* Write TLV header */
    tlvHeader = (TLV_Header_t*)&gTLV_State.packetBuffer[gTLV_State.bufferOffset];
    tlvHeader->type = TLV_TYPE_STATS;
    tlvHeader->length = tlvPayloadSize;
    gTLV_State.bufferOffset += sizeof(TLV_Header_t);
    
    /* Write statistics */
    tlvStats = (TLV_Stats_t*)&gTLV_State.packetBuffer[gTLV_State.bufferOffset];
    tlvStats->interFrameProcessingTime = dpcResult->activeFrameTime;
    tlvStats->transmitOutputTime = 0; /* TODO */
    tlvStats->interFrameProcessingMargin = 0;
    tlvStats->interChirpProcessingMargin = 0;
    tlvStats->activeFrameCpuLoad = 0;
    tlvStats->interFrameCpuLoad = 0;
    
    gTLV_State.bufferOffset += tlvPayloadSize;
    
    return 0;
}

/**
 * @brief Transmit packet buffer via UART
 */
static int32_t TLV_transmitPacket(void)
{
    MmwaveOutput_Header_t* header;
    int32_t retVal;
    
    if (gTLV_State.bufferOffset == 0) {
        return 0; /* Nothing to send */
    }
    
    /* Update total packet length in header */
    header = (MmwaveOutput_Header_t*)&gTLV_State.packetBuffer[0];
    header->totalPacketLen = gTLV_State.bufferOffset;
    
    /* Transmit via UART */
    retVal = UART_write(
        gTLV_State.uartHandle,
        gTLV_State.packetBuffer,
        gTLV_State.bufferOffset
    );
    
    if (retVal != gTLV_State.bufferOffset) {
        return -1; /* Transmission error */
    }
    
    return 0;
}
