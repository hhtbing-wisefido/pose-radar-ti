/**
 * @file presence_detect.c
 * @brief Presence Detection Module Implementation
 * 
 * 🆕 Health Detect project new feature
 * Analyzes point cloud to detect target presence and basic motion
 * 
 * Algorithm:
 * 1. Filter points within detection zone (range min/max)
 * 2. Count valid points
 * 3. If points >= threshold, presence = true
 * 4. Analyze velocity to determine motion state
 * 
 * Reference: mmw_demo point cloud processing
 * RTOS: FreeRTOS (L-SDK mandatory)
 * Created: 2026-01-08
 */

#include <stdint.h>
#include <string.h>
#include <math.h>

#include <kernel/dpl/DebugP.h>

#include "presence_detect.h"

/*===========================================================================
 * Local Variables
 *===========================================================================*/

static PresenceDetect_Config_t gConfig;
static uint16_t gPresenceHoldCounter = 0;
static uint8_t gPreviousPresenceState = 0;

/*===========================================================================
 * Default Configuration
 *===========================================================================*/

static const PresenceDetect_Config_t gDefaultConfig = {
    .minPointsForPresence = 5,         /* At least 5 points */
    .rangeMin_m = 0.5f,                /* Ignore points closer than 0.5m */
    .rangeMax_m = 3.0f,                /* Ignore points further than 3m */
    .velocityThreshold_mps = 0.1f,     /* 10 cm/s motion threshold */
    .presenceHoldFrames = 10           /* Hold presence for 10 frames */
};

/*===========================================================================
 * Implementation
 *===========================================================================*/

/**
 * @brief Get default configuration
 */
void PresenceDetect_getDefaultConfig(PresenceDetect_Config_t* config)
{
    if (config != NULL)
    {
        memcpy(config, &gDefaultConfig, sizeof(PresenceDetect_Config_t));
    }
}

/**
 * @brief Initialize presence detection module
 */
int32_t PresenceDetect_init(const PresenceDetect_Config_t* config)
{
    if (config == NULL)
    {
        memcpy(&gConfig, &gDefaultConfig, sizeof(PresenceDetect_Config_t));
    }
    else
    {
        memcpy(&gConfig, config, sizeof(PresenceDetect_Config_t));
    }
    
    gPresenceHoldCounter = 0;
    gPreviousPresenceState = 0;
    
    DebugP_log("PresenceDetect: Initialized (range=%.1f-%.1fm)\r\n",
               gConfig.rangeMin_m, gConfig.rangeMax_m);
    return 0;
}

/**
 * @brief Update configuration at runtime
 */
int32_t PresenceDetect_setConfig(const PresenceDetect_Config_t* config)
{
    if (config == NULL)
    {
        return -1;
    }
    memcpy(&gConfig, config, sizeof(PresenceDetect_Config_t));
    return 0;
}

/**
 * @brief Process point cloud for presence detection
 */
int32_t PresenceDetect_process(
    const HealthDetect_PointCloud_t* pointCloud,
    uint16_t numPoints,
    PresenceDetect_Result_t* result)
{
    uint16_t pointsInZone = 0;
    float sumRange = 0.0f;
    float sumVelocity = 0.0f;
    uint16_t movingPoints = 0;
    
    if (result == NULL)
    {
        return -1;
    }
    
    /* Clear result */
    memset(result, 0, sizeof(PresenceDetect_Result_t));
    
    /* If no points, check hold counter */
    if (pointCloud == NULL || numPoints == 0)
    {
        if (gPresenceHoldCounter > 0)
        {
            gPresenceHoldCounter--;
            result->isPresent = gPreviousPresenceState;
        }
        return 0;
    }
    
    /* Analyze each point */
    for (uint16_t i = 0; i < numPoints; i++)
    {
        float range = pointCloud[i].range_m;
        float velocity = pointCloud[i].velocity_mps;
        
        /* Check if point is in detection zone */
        if (range >= gConfig.rangeMin_m && range <= gConfig.rangeMax_m)
        {
            pointsInZone++;
            sumRange += range;
            sumVelocity += velocity;
            
            /* Check if point is moving */
            if (fabsf(velocity) > gConfig.velocityThreshold_mps)
            {
                movingPoints++;
            }
        }
    }
    
    /* Fill result */
    result->numPointsInZone = pointsInZone;
    
    if (pointsInZone > 0)
    {
        result->avgRange_m = sumRange / (float)pointsInZone;
        result->avgVelocity_mps = sumVelocity / (float)pointsInZone;
    }
    
    /* Determine presence state */
    if (pointsInZone >= gConfig.minPointsForPresence)
    {
        result->isPresent = 1;
        gPresenceHoldCounter = gConfig.presenceHoldFrames;
    }
    else if (gPresenceHoldCounter > 0)
    {
        gPresenceHoldCounter--;
        result->isPresent = gPreviousPresenceState;
    }
    else
    {
        result->isPresent = 0;
    }
    
    /* Determine motion state */
    if (result->isPresent && movingPoints > 0)
    {
        result->isMoving = 1;
    }
    else
    {
        result->isMoving = 0;
    }
    
    /* Save state for next frame */
    gPreviousPresenceState = result->isPresent;
    
    return 0;
}
