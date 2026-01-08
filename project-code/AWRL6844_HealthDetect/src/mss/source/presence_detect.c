/**
 * @file presence_detect.c
 * @brief Presence Detection Module Implementation
 *
 * Reference: New functionality for Health Detection project
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
#include <math.h>

/* SDK DPL Includes */
#include <kernel/dpl/DebugP.h>

/* Application Includes */
#include <source/presence_detect.h>

/**************************************************************************
 *************************** Local Variables ******************************
 **************************************************************************/

/** @brief Presence detection configuration */
static PresenceDetect_Config_t gPresenceConfig;

/** @brief Presence detection result */
static PresenceDetect_Result_t gPresenceResult;

/** @brief Initialized flag */
static uint8_t gPresenceInitialized = 0;

/**************************************************************************
 *************************** Local Functions ******************************
 **************************************************************************/

/**
 * @brief Calculate range from Cartesian coordinates
 */
static float calculateRange(float x, float y, float z)
{
    return sqrtf(x * x + y * y + z * z);
}

/**
 * @brief Check if point is within detection zone
 */
static uint8_t isPointInZone(PointCloud_Cartesian_t *point)
{
    float range = calculateRange(point->x, point->y, point->z);

    /* Check range limits */
    if (range < gPresenceConfig.minRange_m || range > gPresenceConfig.maxRange_m)
    {
        return 0;
    }

    return 1;
}

/**************************************************************************
 *************************** Public Functions *****************************
 **************************************************************************/

/**
 * @brief Initialize presence detection module
 */
int32_t PresenceDetect_init(PresenceDetect_Config_t *config)
{
    DebugP_log("PresenceDetect: Initializing...\r\n");

    /* Clear state */
    memset(&gPresenceResult, 0, sizeof(PresenceDetect_Result_t));

    /* Apply configuration if provided */
    if (config != NULL)
    {
        memcpy(&gPresenceConfig, config, sizeof(PresenceDetect_Config_t));
    }
    else
    {
        /* Use default configuration */
        gPresenceConfig.minPointsForPresence = 5;
        gPresenceConfig.minRange_m = 0.5f;
        gPresenceConfig.maxRange_m = 3.0f;
        gPresenceConfig.minVelocityThresh_mps = 0.1f;
        gPresenceConfig.presenceHoldFrames = 10;
        gPresenceConfig.snrThreshold_dB = 10.0f;
    }

    gPresenceInitialized = 1;

    DebugP_log("PresenceDetect: Initialized\r\n");

    return 0;
}

/**
 * @brief Configure presence detection parameters
 */
int32_t PresenceDetect_config(PresenceDetect_Config_t *config)
{
    if (config == NULL)
    {
        return -1;
    }

    memcpy(&gPresenceConfig, config, sizeof(PresenceDetect_Config_t));

    DebugP_log("PresenceDetect: Configured - Range [%.2f, %.2f]m\r\n",
               gPresenceConfig.minRange_m, gPresenceConfig.maxRange_m);

    return 0;
}

/**
 * @brief Process point cloud for presence detection
 */
int32_t PresenceDetect_process(PointCloud_Cartesian_t *points,
                                uint32_t numPoints,
                                PresenceDetect_Result_t *result)
{
    uint32_t i;
    uint16_t pointsInZone = 0;
    float sumRange = 0.0f;
    float sumVelocity = 0.0f;
    float sumAzimuth = 0.0f;
    float sumElevation = 0.0f;
    uint8_t hasMovingPoints = 0;

    if (!gPresenceInitialized)
    {
        return -1;
    }

    if (result == NULL)
    {
        return -1;
    }

    /* Process each point */
    for (i = 0; i < numPoints; i++)
    {
        if (isPointInZone(&points[i]))
        {
            float range = calculateRange(points[i].x, points[i].y, points[i].z);
            float azimuth = atan2f(points[i].x, points[i].y) * (180.0f / 3.14159f);
            float elevation = 0.0f;

            if (range > 0.01f)
            {
                elevation = asinf(points[i].z / range) * (180.0f / 3.14159f);
            }

            pointsInZone++;
            sumRange += range;
            sumVelocity += points[i].velocity;
            sumAzimuth += azimuth;
            sumElevation += elevation;

            /* Check for motion */
            if (fabsf(points[i].velocity) > gPresenceConfig.minVelocityThresh_mps)
            {
                hasMovingPoints = 1;
            }
        }
    }

    /* Calculate averages */
    if (pointsInZone > 0)
    {
        result->avgRange_m = sumRange / pointsInZone;
        result->avgVelocity_mps = sumVelocity / pointsInZone;
        result->avgAzimuth_deg = sumAzimuth / pointsInZone;
        result->avgElevation_deg = sumElevation / pointsInZone;
    }
    else
    {
        result->avgRange_m = 0.0f;
        result->avgVelocity_mps = 0.0f;
        result->avgAzimuth_deg = 0.0f;
        result->avgElevation_deg = 0.0f;
    }

    result->numPointsInZone = pointsInZone;

    /* Determine presence */
    if (pointsInZone >= gPresenceConfig.minPointsForPresence)
    {
        result->isPresent = 1;
        result->presenceCounter++;
        result->absenceCounter = 0;

        /* Determine if moving */
        result->isMoving = hasMovingPoints ? 1 : 0;

        /* Update state */
        if (result->isMoving)
        {
            if (result->avgVelocity_mps > 0)
            {
                result->state = PRESENCE_STATE_APPROACHING;
            }
            else
            {
                result->state = PRESENCE_STATE_DEPARTING;
            }
        }
        else
        {
            result->state = PRESENCE_STATE_PRESENT;
        }
    }
    else
    {
        /* Check hold time */
        result->absenceCounter++;

        if (result->absenceCounter > gPresenceConfig.presenceHoldFrames)
        {
            result->isPresent = 0;
            result->isMoving = 0;
            result->presenceCounter = 0;
            result->state = PRESENCE_STATE_EMPTY;
        }
        else
        {
            /* Hold previous state */
        }
    }

    /* Copy to global result */
    memcpy(&gPresenceResult, result, sizeof(PresenceDetect_Result_t));

    return 0;
}

/**
 * @brief Reset presence detection state
 */
int32_t PresenceDetect_reset(void)
{
    memset(&gPresenceResult, 0, sizeof(PresenceDetect_Result_t));
    gPresenceResult.state = PRESENCE_STATE_EMPTY;

    DebugP_log("PresenceDetect: Reset\r\n");

    return 0;
}

/**
 * @brief Get current presence state
 */
PresenceState_e PresenceDetect_getState(void)
{
    return gPresenceResult.state;
}
