/**
 * @file feature_extract.h
 * @brief Feature Extraction Module Header
 *
 * Reference: New functionality for Health Detection project
 * Adapted for: Health Detection three-layer architecture - DSS Layer
 *
 * RTOS: FreeRTOS (L-SDK mandatory)
 * Note: L-SDK uses FreeRTOS, NOT TI-RTOS/BIOS
 *
 * This module extracts health-related features from radar point cloud data
 * running on the C66x DSP for optimal performance.
 *
 * Created: 2026-01-08
 */

#ifndef FEATURE_EXTRACT_H
#define FEATURE_EXTRACT_H

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************
 *************************** Include Files ********************************
 **************************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <common/health_detect_types.h>
#include <common/data_path.h>

/**************************************************************************
 *************************** Macros ***************************************
 **************************************************************************/

/** Maximum number of points for feature extraction */
#define FEATURE_EXTRACT_MAX_POINTS          (256U)

/** Smoothing filter alpha for motion energy */
#define FEATURE_EXTRACT_SMOOTHING_ALPHA     (0.1f)

/** Minimum points required for valid statistics */
#define FEATURE_EXTRACT_MIN_POINTS          (3U)

/** Invalid SNR value */
#define FEATURE_EXTRACT_INVALID_SNR         (-100.0f)

/**************************************************************************
 *************************** Type Definitions *****************************
 **************************************************************************/

/**
 * @brief Feature Extraction Configuration
 */
typedef struct FeatureExtract_Config_t
{
    float       rangeMin_m;             /**< Minimum range for feature extraction */
    float       rangeMax_m;             /**< Maximum range for feature extraction */
    float       velocityThreshold_mps;  /**< Velocity threshold for motion detection */
    float       snrThreshold_dB;        /**< SNR threshold for valid points */
    float       smoothingAlpha;         /**< Smoothing filter coefficient */
    uint16_t    minPointsForStats;      /**< Minimum points for valid statistics */
} FeatureExtract_Config_t;

/**
 * @brief Extracted Features Structure
 * Contains all health-related features extracted from point cloud
 */
typedef struct FeatureExtract_Output_t
{
    /* Range Statistics */
    StatisticsInfo_t    rangeStats;         /**< Range statistics (min/max/mean/std) */
    
    /* Velocity Statistics */
    StatisticsInfo_t    velocityStats;      /**< Velocity statistics */
    
    /* Azimuth Statistics */
    StatisticsInfo_t    azimuthStats;       /**< Azimuth angle statistics */
    
    /* Elevation Statistics */
    StatisticsInfo_t    elevationStats;     /**< Elevation angle statistics */
    
    /* SNR Statistics */
    StatisticsInfo_t    snrStats;           /**< SNR statistics */
    
    /* Motion Features */
    float               motionEnergy;       /**< Instantaneous motion energy */
    float               motionEnergySmoothed; /**< Smoothed motion energy (IIR filtered) */
    float               motionIndex;        /**< Motion activity index (0-1) */
    
    /* Peak Features */
    float               peakRange_m;        /**< Range of strongest point */
    float               peakVelocity_mps;   /**< Velocity of strongest point */
    float               peakSnr_dB;         /**< Peak SNR value */
    
    /* Point Count Features */
    uint16_t            numValidPoints;     /**< Number of valid points processed */
    uint16_t            numMovingPoints;    /**< Number of points with |velocity| > threshold */
    uint16_t            numStaticPoints;    /**< Number of static points */
    
    /* Centroid */
    float               centroidX_m;        /**< X centroid of point cloud */
    float               centroidY_m;        /**< Y centroid of point cloud */
    float               centroidZ_m;        /**< Z centroid of point cloud */
    
    /* Spread (spatial extent) */
    float               spreadX_m;          /**< X spread (max - min) */
    float               spreadY_m;          /**< Y spread */
    float               spreadZ_m;          /**< Z spread */
    
    /* Timing */
    uint32_t            processingCycles;   /**< DSP cycles for feature extraction */
    
    /* Validity */
    uint8_t             isValid;            /**< 1 if features are valid */
    
} FeatureExtract_Output_t;

/**
 * @brief Feature Extraction Handle (opaque)
 */
typedef struct FeatureExtract_Handle_t
{
    FeatureExtract_Config_t     config;             /**< Configuration */
    FeatureExtract_Output_t     output;             /**< Latest output */
    float                       prevMotionEnergy;   /**< Previous motion energy for smoothing */
    uint32_t                    frameCount;         /**< Frame counter */
    uint8_t                     isInitialized;      /**< Initialization flag */
} FeatureExtract_Handle_t;

/**************************************************************************
 *************************** Function Prototypes **************************
 **************************************************************************/

/**
 * @brief Initialize feature extraction module
 *
 * @param[out] handle   Pointer to handle structure
 * @param[in]  config   Pointer to configuration (NULL for defaults)
 *
 * @return 0 on success, negative on error
 */
int32_t FeatureExtract_init(FeatureExtract_Handle_t *handle,
                            const FeatureExtract_Config_t *config);

/**
 * @brief Process point cloud and extract features
 *
 * @param[in,out] handle      Feature extraction handle
 * @param[in]     pointCloud  Pointer to point cloud data
 * @param[in]     numPoints   Number of points in point cloud
 * @param[out]    output      Pointer to output structure (can be NULL)
 *
 * @return 0 on success, negative on error
 */
int32_t FeatureExtract_process(FeatureExtract_Handle_t *handle,
                               const PointCloud_Point_t *pointCloud,
                               uint16_t numPoints,
                               FeatureExtract_Output_t *output);

/**
 * @brief Get latest extracted features
 *
 * @param[in]  handle   Feature extraction handle
 * @param[out] output   Pointer to output structure
 *
 * @return 0 on success, negative on error
 */
int32_t FeatureExtract_getOutput(const FeatureExtract_Handle_t *handle,
                                 FeatureExtract_Output_t *output);

/**
 * @brief Reset feature extraction state
 *
 * @param[in,out] handle   Feature extraction handle
 *
 * @return 0 on success, negative on error
 */
int32_t FeatureExtract_reset(FeatureExtract_Handle_t *handle);

/**
 * @brief Set feature extraction configuration
 *
 * @param[in,out] handle   Feature extraction handle
 * @param[in]     config   New configuration
 *
 * @return 0 on success, negative on error
 */
int32_t FeatureExtract_setConfig(FeatureExtract_Handle_t *handle,
                                 const FeatureExtract_Config_t *config);

/**
 * @brief Get default configuration
 *
 * @param[out] config   Pointer to configuration structure to fill
 */
void FeatureExtract_getDefaultConfig(FeatureExtract_Config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* FEATURE_EXTRACT_H */
