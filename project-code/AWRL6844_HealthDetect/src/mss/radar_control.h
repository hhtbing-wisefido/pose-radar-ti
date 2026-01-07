/**
 * @file radar_control.h
 * @brief Radar Control Module Header
 */

#ifndef RADAR_CONTROL_H
#define RADAR_CONTROL_H

#include <stdint.h>

/* Frame callback function type */
typedef void (*RadarControl_FrameCallback_t)(uint32_t frameNum);

/* Radar Control API */
int32_t RadarControl_init(void);
int32_t RadarControl_config(void);
int32_t RadarControl_start(void);
int32_t RadarControl_stop(void);
int32_t RadarControl_registerFrameCallback(RadarControl_FrameCallback_t callback);

#endif /* RADAR_CONTROL_H */
