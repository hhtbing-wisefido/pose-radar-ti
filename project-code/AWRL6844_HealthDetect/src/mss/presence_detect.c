/**
 * @file presence_detect.c
 * @brief Presence Detection Module - High-Level Feature Analysis
 * 
 * Analyzes features from DSS to determine presence state.
 * Uses clustering, motion tracking, and temporal filtering.
 */

#include "presence_detect.h"
#include "../common/shared_memory.h"
#include "../common/health_detect_types.h"
#include <string.h>
#include <math.h>

/*----------------------------------------------------------------------------*/
/* Presence Detection Configuration                                          */
/*----------------------------------------------------------------------------*/

#define PRESENCE_TIMEOUT_FRAMES     50  /* 50 frames @ 10fps = 5 seconds */
#define MOTION_VELOCITY_THRESHOLD   0.1f /* m/s */
#define MIN_POINTS_FOR_PRESENCE     3

/*----------------------------------------------------------------------------*/
/* Module State                                                              */
/*----------------------------------------------------------------------------*/

typedef struct {
    PresenceState_e currentState;
    uint32_t lastMotionFrame;
    uint32_t lastPresenceFrame;
    uint32_t consecutiveAbsentFrames;
    
    /* Feature history for temporal filtering */
    float lastCenterX;
    float lastCenterY;
    float lastCenterZ;
    
} PresenceDetect_State_t;

static PresenceDetect_State_t gPresenceState = {
    .currentState = PRESENCE_ABSENT,
    .lastMotionFrame = 0,
    .lastPresenceFrame = 0,
    .consecutiveAbsentFrames = 0,
};

/*----------------------------------------------------------------------------*/
/* Private Function Declarations                                             */
/*----------------------------------------------------------------------------*/

static bool PresenceDetect_hasSignificantMotion(
    const HealthDetect_PointCloudFeatures_t* features);
    
static float PresenceDetect_calculateDistance3D(
    float x1, float y1, float z1,
    float x2, float y2, float z2);

/*----------------------------------------------------------------------------*/
/* Public API Implementation                                                 */
/*----------------------------------------------------------------------------*/

/**
 * @brief Initialize presence detection module
 */
int32_t PresenceDetect_init(void)
{
    memset(&gPresenceState, 0, sizeof(gPresenceState));
    gPresenceState.currentState = PRESENCE_ABSENT;
    
    return 0;
}

/**
 * @brief Process frame features and update presence state
 * 
 * @param features Feature data from DSS (in L3 RAM)
 * @param frameNum Current frame number
 * @return Updated presence state
 */
PresenceState_e PresenceDetect_processFrame(
    const HealthDetect_PointCloudFeatures_t* features,
    uint32_t frameNum)
{
    if (features == NULL) {
        return gPresenceState.currentState;
    }
    
    /* Check if valid features exist */
    bool hasValidFeatures = (features->numPoints >= MIN_POINTS_FOR_PRESENCE);
    
    /* State Machine */
    switch (gPresenceState.currentState) {
        case PRESENCE_ABSENT:
            if (hasValidFeatures) {
                /* Transition to PRESENT */
                gPresenceState.currentState = PRESENCE_PRESENT;
                gPresenceState.lastPresenceFrame = frameNum;
                gPresenceState.consecutiveAbsentFrames = 0;
                
                /* Record initial position */
                gPresenceState.lastCenterX = features->centerX;
                gPresenceState.lastCenterY = features->centerY;
                gPresenceState.lastCenterZ = features->centerZ;
            }
            break;
            
        case PRESENCE_PRESENT:
            if (hasValidFeatures) {
                /* Check for motion */
                if (PresenceDetect_hasSignificantMotion(features)) {
                    /* Transition to MOTION */
                    gPresenceState.currentState = PRESENCE_MOTION;
                    gPresenceState.lastMotionFrame = frameNum;
                }
                
                gPresenceState.lastPresenceFrame = frameNum;
                gPresenceState.consecutiveAbsentFrames = 0;
                
                /* Update last position */
                gPresenceState.lastCenterX = features->centerX;
                gPresenceState.lastCenterY = features->centerY;
                gPresenceState.lastCenterZ = features->centerZ;
            } else {
                /* No features detected */
                gPresenceState.consecutiveAbsentFrames++;
                
                if (gPresenceState.consecutiveAbsentFrames > PRESENCE_TIMEOUT_FRAMES) {
                    /* Timeout - return to ABSENT */
                    gPresenceState.currentState = PRESENCE_ABSENT;
                }
            }
            break;
            
        case PRESENCE_MOTION:
            if (hasValidFeatures) {
                if (!PresenceDetect_hasSignificantMotion(features)) {
                    /* Motion stopped - return to PRESENT */
                    gPresenceState.currentState = PRESENCE_PRESENT;
                } else {
                    /* Continue tracking motion */
                    gPresenceState.lastMotionFrame = frameNum;
                }
                
                gPresenceState.lastPresenceFrame = frameNum;
                gPresenceState.consecutiveAbsentFrames = 0;
                
                /* Update position */
                gPresenceState.lastCenterX = features->centerX;
                gPresenceState.lastCenterY = features->centerY;
                gPresenceState.lastCenterZ = features->centerZ;
            } else {
                /* Lost tracking */
                gPresenceState.consecutiveAbsentFrames++;
                
                if (gPresenceState.consecutiveAbsentFrames > PRESENCE_TIMEOUT_FRAMES) {
                    gPresenceState.currentState = PRESENCE_ABSENT;
                } else {
                    /* Brief occlusion - stay in MOTION */
                }
            }
            break;
    }
    
    return gPresenceState.currentState;
}

/**
 * @brief Get current presence state
 */
PresenceState_e PresenceDetect_getState(void)
{
    return gPresenceState.currentState;
}

/**
 * @brief Get time since last motion (frames)
 */
uint32_t PresenceDetect_getTimeSinceMotion(uint32_t currentFrame)
{
    if (gPresenceState.lastMotionFrame == 0) {
        return 0xFFFFFFFF; /* Never detected */
    }
    
    return currentFrame - gPresenceState.lastMotionFrame;
}

/**
 * @brief Reset presence detection state
 */
void PresenceDetect_reset(void)
{
    gPresenceState.currentState = PRESENCE_ABSENT;
    gPresenceState.lastMotionFrame = 0;
    gPresenceState.lastPresenceFrame = 0;
    gPresenceState.consecutiveAbsentFrames = 0;
}

/*----------------------------------------------------------------------------*/
/* Private Helper Functions                                                  */
/*----------------------------------------------------------------------------*/

/**
 * @brief Check if features indicate significant motion
 * 
 * Criteria:
 * - Velocity magnitude > threshold
 * - OR position change > threshold
 */
static bool PresenceDetect_hasSignificantMotion(
    const HealthDetect_PointCloudFeatures_t* features)
{
    /* Check velocity magnitude */
    float velocityMag = sqrtf(
        features->velocityX * features->velocityX +
        features->velocityY * features->velocityY +
        features->velocityZ * features->velocityZ
    );
    
    if (velocityMag > MOTION_VELOCITY_THRESHOLD) {
        return true;
    }
    
    /* Check position change */
    float positionChange = PresenceDetect_calculateDistance3D(
        features->centerX, features->centerY, features->centerZ,
        gPresenceState.lastCenterX, 
        gPresenceState.lastCenterY,
        gPresenceState.lastCenterZ
    );
    
    if (positionChange > 0.2f) { /* 20cm threshold */
        return true;
    }
    
    return false;
}

/**
 * @brief Calculate 3D Euclidean distance
 */
static float PresenceDetect_calculateDistance3D(
    float x1, float y1, float z1,
    float x2, float y2, float z2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dz = z2 - z1;
    
    return sqrtf(dx*dx + dy*dy + dz*dz);
}
