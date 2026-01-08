/**
 * @file data_path.h
 * @brief Data Path Chain (DPC) Definitions for MSS-DSS Coordination
 * 
 * This file defines DPC structures learned from mmw_demo but adapted
 * for the three-layer architecture with DSS feature extraction.
 * 
 * Reference: mmw_demo/source/dpc/dpc.c, dpc.h
 * Adapted for: Health Detection multi-core architecture
 */

#ifndef DATA_PATH_H
#define DATA_PATH_H

#include <stdint.h>
#include <stdbool.h>

/*----------------------------------------------------------------------------*/
/* DPC Configuration Structures (MSS → DSS)                                  */
/*----------------------------------------------------------------------------*/

/**
 * @brief CFAR Configuration for Range/Doppler Detection
 * Learned from: mmw_demo DPU_CFARCAProc_CfarCfg
 */
typedef struct DPC_CfarCfg_t {
    uint8_t  averageMode;       /* 0=CFAR-CA, 1=CFAR-CASO */
    uint8_t  winLen;            /* Guard+Reference cells */
    uint8_t  guardLen;          /* Guard cells */
    uint8_t  noiseDivShift;     /* Noise threshold shift */
    uint16_t cyclicMode;        /* 0=disabled, 1=enabled */
    float    thresholdScale;    /* Detection threshold */
} DPC_CfarCfg_t;

/**
 * @brief DOA (Direction of Arrival) Configuration
 * Learned from: mmw_demo AOA processing
 */
typedef struct DPC_DOACfg_t {
    uint8_t  numVirtualAntennas; /* Virtual antenna count */
    uint8_t  numAntAzim;         /* Azimuth antennas */
    uint8_t  numAntElev;         /* Elevation antennas */
    float    estResolution;      /* Angle resolution (degrees) */
    float    gamma;              /* Peak search threshold */
    bool     multiPeakEn;        /* Enable multi-peak detection */
} DPC_DOACfg_t;

/**
 * @brief DPC Main Configuration Structure
 * Written by MSS, read by DSS from L3 DPC_CONFIG_BASE
 */
typedef struct DPC_Config_t {
    /* Frame Configuration */
    uint32_t frameCount;         /* Current frame number */
    uint16_t numChirpsPerFrame;
    uint16_t numRangeBins;
    uint16_t numDopplerBins;
    
    /* Detection Parameters */
    DPC_CfarCfg_t rangeCfar;
    DPC_CfarCfg_t dopplerCfar;
    DPC_DOACfg_t  doaCfg;
    
    /* Feature Extraction Control */
    bool     featureExtractEn;   /* Enable DSS feature extraction */
    float    minClusterSize;     /* Minimum points for valid cluster */
    
} DPC_Config_t;

/*----------------------------------------------------------------------------*/
/* DPC Results (DSS → MSS)                                                   */
/*----------------------------------------------------------------------------*/

/**
 * @brief DPC Execution Statistics
 * Written by DSS, read by MSS from L3 DPC_RESULT_BASE
 */
typedef struct DPC_Result_t {
    uint32_t frameCount;         /* Frame number */
    uint32_t interFrameStartTime;/* SOC_XWR68XX_DSS_CPU_CLK_CYCLES timestamp */
    uint32_t interFrameEndTime;
    uint32_t activeFrameTime;    /* DPC execution time (cycles) */
    
    /* Detection Results */
    uint16_t numDetectedObjects; /* Point cloud count */
    uint16_t numFeatures;        /* Feature extraction count */
    
    /* Error Flags */
    uint32_t errorCode;          /* 0=success, non-zero=error */
    
} DPC_Result_t;

/*----------------------------------------------------------------------------*/
/* Point Cloud Structure (DSS → MSS)                                         */
/*----------------------------------------------------------------------------*/

/**
 * @brief Single Detected Object in Cartesian Coordinates
 * Reference: DPIF_PointCloudCartesian from mmw_demo
 */
typedef struct DPC_PointCloudCartesian_t {
    float x;                     /* Range (meters) - forward */
    float y;                     /* Azimuth (meters) - left/right */
    float z;                     /* Elevation (meters) - up/down */
    float velocity;              /* Doppler velocity (m/s) */
} DPC_PointCloudCartesian_t;

/**
 * @brief Point Cloud Array in L3 Shared RAM
 * Section: .point_cloud
 */
typedef struct DPC_PointCloud_t {
    uint32_t numPoints;          /* Valid point count */
    DPC_PointCloudCartesian_t points[256]; /* Up to 256 objects */
} DPC_PointCloud_t;

/*----------------------------------------------------------------------------*/
/* DPC State Machine                                                         */
/*----------------------------------------------------------------------------*/

typedef enum {
    DPC_STATE_IDLE = 0,
    DPC_STATE_CONFIG,
    DPC_STATE_RUNNING,
    DPC_STATE_STOPPED,
    DPC_STATE_ERROR
} DPC_State_e;

#endif /* DATA_PATH_H */
