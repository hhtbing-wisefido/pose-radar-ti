/**
 * @file health_detect_types.h
 * @brief Health Detection Feature Types and Structures
 *
 * Reference: New functionality for Health Detection project
 * Adapted for: Health Detection three-layer architecture
 *
 * RTOS: FreeRTOS (L-SDK mandatory)
 * Note: L-SDK uses FreeRTOS, NOT TI-RTOS/BIOS
 *
 * Created: 2026-01-08
 */

#ifndef HEALTH_DETECT_TYPES_H
#define HEALTH_DETECT_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/*===========================================================================*/
/*                         Health Detection States                            */
/*===========================================================================*/

/**
 * @brief Presence Detection State
 */
typedef enum PresenceState_e
{
    PRESENCE_STATE_EMPTY = 0,       /**< No presence detected */
    PRESENCE_STATE_PRESENT,         /**< Presence detected, stationary */
    PRESENCE_STATE_MOVING,          /**< Presence detected, moving */
    PRESENCE_STATE_APPROACHING,     /**< Target approaching */
    PRESENCE_STATE_DEPARTING,       /**< Target departing */
    PRESENCE_STATE_MAX
} PresenceState_e;

/**
 * @brief Health Monitoring State
 */
typedef enum HealthState_e
{
    HEALTH_STATE_UNKNOWN = 0,       /**< State unknown/initializing */
    HEALTH_STATE_NORMAL,            /**< Normal health indicators */
    HEALTH_STATE_IRREGULAR,         /**< Irregular pattern detected */
    HEALTH_STATE_ALERT,             /**< Alert condition */
    HEALTH_STATE_MAX
} HealthState_e;

/*===========================================================================*/
/*                         Statistics Structures                              */
/*===========================================================================*/

/**
 * @brief Statistics Information Structure
 * Basic statistics for a data set
 */
typedef struct StatisticsInfo_t
{
    float       min;                /**< Minimum value */
    float       max;                /**< Maximum value */
    float       mean;               /**< Mean value */
    float       variance;           /**< Variance */
    float       stdDev;             /**< Standard deviation */
} StatisticsInfo_t;

/**
 * @brief Histogram Structure
 */
typedef struct Histogram_t
{
    uint16_t    numBins;            /**< Number of histogram bins */
    float       binWidth;           /**< Width of each bin */
    float       minValue;           /**< Minimum value (bin 0 start) */
    uint32_t    *counts;            /**< Array of bin counts */
} Histogram_t;

/*===========================================================================*/
/*                         Feature Extraction Structures                      */
/*===========================================================================*/

/**
 * @brief Point Cloud Features
 * Features extracted from point cloud data
 */
typedef struct PointCloudFeatures_t
{
    uint16_t    numPoints;          /**< Number of valid points */
    StatisticsInfo_t rangeStats;    /**< Range statistics */
    StatisticsInfo_t velocityStats; /**< Velocity statistics */
    StatisticsInfo_t azimuthStats;  /**< Azimuth statistics */
    StatisticsInfo_t elevationStats;/**< Elevation statistics */
    StatisticsInfo_t snrStats;      /**< SNR statistics */
    float       centroidX;          /**< Centroid X coordinate */
    float       centroidY;          /**< Centroid Y coordinate */
    float       centroidZ;          /**< Centroid Z coordinate */
    float       spreadX;            /**< Spread in X direction */
    float       spreadY;            /**< Spread in Y direction */
    float       spreadZ;            /**< Spread in Z direction */
} PointCloudFeatures_t;

/**
 * @brief Motion Features
 * Features related to target motion
 */
typedef struct MotionFeatures_t
{
    float       motionEnergy;       /**< Total motion energy */
    float       motionEnergySmoothed; /**< Smoothed motion energy */
    float       velocityMagnitude;  /**< Velocity magnitude */
    float       accelerationEst;    /**< Estimated acceleration */
    float       peakVelocity;       /**< Peak velocity in frame */
    uint8_t     motionDirection;    /**< Motion direction indicator */
} MotionFeatures_t;

/**
 * @brief Breathing Features (for vital signs detection)
 * Features related to breathing pattern
 */
typedef struct BreathingFeatures_t
{
    float       breathingRate;      /**< Breathing rate (breaths/min) */
    float       breathingDepth;     /**< Breathing depth (mm) */
    float       breathingVariability; /**< Breathing variability */
    float       peakPower;          /**< Peak power at breathing frequency */
    float       confidence;         /**< Detection confidence (0-1) */
    uint8_t     isValid;            /**< Valid detection flag */
} BreathingFeatures_t;

/**
 * @brief Heart Rate Features (for vital signs detection)
 * Features related to heart rate pattern
 */
typedef struct HeartRateFeatures_t
{
    float       heartRate;          /**< Heart rate (bpm) */
    float       heartRateVariability; /**< Heart rate variability */
    float       peakPower;          /**< Peak power at heart rate frequency */
    float       confidence;         /**< Detection confidence (0-1) */
    uint8_t     isValid;            /**< Valid detection flag */
} HeartRateFeatures_t;

/*===========================================================================*/
/*                         Complete Health Features                           */
/*===========================================================================*/

/**
 * @brief Complete Health Detection Features
 * All features extracted for health monitoring
 */
typedef struct HealthDetect_Features_t
{
    /* Timestamp */
    uint32_t    frameNum;           /**< Frame number */
    uint32_t    timestamp_ms;       /**< Timestamp in milliseconds */

    /* Point Cloud Features */
    PointCloudFeatures_t pointCloudFeatures;

    /* Motion Features */
    MotionFeatures_t motionFeatures;

    /* Vital Signs Features */
    BreathingFeatures_t breathingFeatures;
    HeartRateFeatures_t heartRateFeatures;

    /* SNR and Quality */
    float       peakSnr_dB;         /**< Peak SNR in dB */
    float       avgSnr_dB;          /**< Average SNR in dB */
    float       signalQuality;      /**< Overall signal quality (0-1) */

    /* State Information */
    PresenceState_e presenceState;  /**< Current presence state */
    HealthState_e healthState;      /**< Current health state */

    /* Validity */
    uint8_t     isValid;            /**< Features valid flag */
} HealthDetect_Features_t;

/*===========================================================================*/
/*                         Presence Detection Structures                      */
/*===========================================================================*/

/**
 * @brief Presence Detection Configuration
 */
typedef struct PresenceDetect_Config_t
{
    uint16_t    minPointsForPresence;   /**< Minimum points to declare presence */
    float       minRange_m;             /**< Minimum detection range (m) */
    float       maxRange_m;             /**< Maximum detection range (m) */
    float       minVelocityThresh_mps;  /**< Velocity threshold for motion (m/s) */
    uint16_t    presenceHoldFrames;     /**< Frames to hold presence after detection */
    float       snrThreshold_dB;        /**< Minimum SNR for valid point */
} PresenceDetect_Config_t;

/**
 * @brief Presence Detection Result
 */
typedef struct PresenceDetect_Result_t
{
    uint8_t     isPresent;              /**< Target present flag */
    uint8_t     isMoving;               /**< Target moving flag */
    uint16_t    numPointsInZone;        /**< Number of points in detection zone */
    float       avgRange_m;             /**< Average target range (m) */
    float       avgVelocity_mps;        /**< Average target velocity (m/s) */
    float       avgAzimuth_deg;         /**< Average azimuth angle (deg) */
    float       avgElevation_deg;       /**< Average elevation angle (deg) */
    uint16_t    presenceCounter;        /**< Frames since presence started */
    uint16_t    absenceCounter;         /**< Frames since presence ended */
    PresenceState_e state;              /**< Current presence state */
} PresenceDetect_Result_t;

/*===========================================================================*/
/*                         Zone Definition                                    */
/*===========================================================================*/

/**
 * @brief Detection Zone Definition
 * Defines a 3D zone for presence detection
 */
typedef struct DetectionZone_t
{
    uint8_t     enabled;            /**< Zone enable flag */
    float       minX;               /**< Minimum X coordinate (m) */
    float       maxX;               /**< Maximum X coordinate (m) */
    float       minY;               /**< Minimum Y coordinate (m) */
    float       maxY;               /**< Maximum Y coordinate (m) */
    float       minZ;               /**< Minimum Z coordinate (m) */
    float       maxZ;               /**< Maximum Z coordinate (m) */
    char        name[16];           /**< Zone name */
} DetectionZone_t;

/**
 * @brief Maximum number of detection zones
 */
#define HEALTH_DETECT_MAX_ZONES     (4U)

/*===========================================================================*/
/*                         Feature History Buffer                             */
/*===========================================================================*/

/**
 * @brief Feature history length for temporal analysis
 */
#define HEALTH_DETECT_HISTORY_LEN   (64U)

/**
 * @brief Feature History Buffer
 * Stores historical feature data for trend analysis
 */
typedef struct FeatureHistory_t
{
    float       motionEnergyHistory[HEALTH_DETECT_HISTORY_LEN];
    float       rangeHistory[HEALTH_DETECT_HISTORY_LEN];
    float       velocityHistory[HEALTH_DETECT_HISTORY_LEN];
    uint16_t    numPointsHistory[HEALTH_DETECT_HISTORY_LEN];
    uint16_t    writeIdx;           /**< Current write index */
    uint16_t    count;              /**< Number of valid entries */
} FeatureHistory_t;

#ifdef __cplusplus
}
#endif

#endif /* HEALTH_DETECT_TYPES_H */
