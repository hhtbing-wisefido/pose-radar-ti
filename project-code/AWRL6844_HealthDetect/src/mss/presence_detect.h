/**
 * @file presence_detect.h
 * @brief Presence Detection Module Header
 * 
 * 🆕 Health Detect project new feature
 * Analyzes point cloud to detect target presence and basic motion
 * 
 * Reference: mmw_demo DPC output processing
 * RTOS: FreeRTOS (L-SDK mandatory)
 * Created: 2026-01-08
 */

#ifndef PRESENCE_DETECT_H
#define PRESENCE_DETECT_H

#include <stdint.h>
#include <health_detect_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Presence detection configuration
 */
typedef struct PresenceDetect_Config
{
    uint16_t minPointsForPresence;    /* Minimum points to confirm presence */
    float    rangeMin_m;               /* Detection zone: minimum range (m) */
    float    rangeMax_m;               /* Detection zone: maximum range (m) */
    float    velocityThreshold_mps;    /* Threshold for motion detection (m/s) */
    uint16_t presenceHoldFrames;       /* Frames to hold presence after detection */
} PresenceDetect_Config_t;

/**
 * @brief Presence detection result
 */
typedef struct PresenceDetect_Result
{
    uint8_t  isPresent;               /* 1 = target present, 0 = no target */
    uint8_t  isMoving;                /* 1 = target moving, 0 = stationary */
    uint16_t numPointsInZone;         /* Number of points in detection zone */
    float    avgRange_m;              /* Average range of detected points */
    float    avgVelocity_mps;         /* Average velocity of detected points */
} PresenceDetect_Result_t;

/**
 * @brief Initialize presence detection module
 * @param config Configuration parameters
 * @return 0 on success, negative on error
 */
int32_t PresenceDetect_init(const PresenceDetect_Config_t* config);

/**
 * @brief Process point cloud for presence detection
 * @param pointCloud Pointer to point cloud data
 * @param numPoints Number of points in cloud
 * @param result Output: detection result
 * @return 0 on success, negative on error
 */
int32_t PresenceDetect_process(
    const HealthDetect_PointCloud_t* pointCloud,
    uint16_t numPoints,
    PresenceDetect_Result_t* result
);

/**
 * @brief Update configuration at runtime
 * @param config New configuration
 * @return 0 on success, negative on error
 */
int32_t PresenceDetect_setConfig(const PresenceDetect_Config_t* config);

/**
 * @brief Get default configuration
 * @param config Output: default config
 */
void PresenceDetect_getDefaultConfig(PresenceDetect_Config_t* config);

#ifdef __cplusplus
}
#endif

#endif /* PRESENCE_DETECT_H */
