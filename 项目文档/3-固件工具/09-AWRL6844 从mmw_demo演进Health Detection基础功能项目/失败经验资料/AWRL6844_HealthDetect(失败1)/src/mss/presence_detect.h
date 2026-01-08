/**
 * @file presence_detect.h
 * @brief Presence Detection Module Header
 */

#ifndef PRESENCE_DETECT_H
#define PRESENCE_DETECT_H

#include <stdint.h>
#include "../common/health_detect_types.h"

/* Presence Detection API */
int32_t PresenceDetect_init(void);

PresenceState_e PresenceDetect_processFrame(
    const HealthDetect_PointCloudFeatures_t* features,
    uint32_t frameNum);

PresenceState_e PresenceDetect_getState(void);

uint32_t PresenceDetect_getTimeSinceMotion(uint32_t currentFrame);

void PresenceDetect_reset(void);

#endif /* PRESENCE_DETECT_H */
