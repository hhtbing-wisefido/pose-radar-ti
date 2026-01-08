/**
 * @file radar_control.h
 * @brief Radar Control Module Header
 * 
 * Wraps mmWave API for RF initialization and control
 * Reference: mmw_demo/source/control/control.h
 * RTOS: FreeRTOS (L-SDK mandatory)
 * Created: 2026-01-08
 */

#ifndef RADAR_CONTROL_H
#define RADAR_CONTROL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Radar state enumeration
 */
typedef enum RadarControl_State
{
    RADAR_STATE_UNINIT = 0,
    RADAR_STATE_INIT,
    RADAR_STATE_CONFIGURED,
    RADAR_STATE_STARTED,
    RADAR_STATE_STOPPED,
    RADAR_STATE_ERROR
} RadarControl_State_e;

/**
 * @brief Frame configuration
 */
typedef struct RadarControl_FrameCfg
{
    uint16_t chirpStartIdx;
    uint16_t chirpEndIdx;
    uint16_t numLoops;
    uint16_t numFrames;
    float    framePeriodicity_ms;
    float    triggerDelay_us;
    uint8_t  triggerSelect;           /* 1=SW, 2=HW */
} RadarControl_FrameCfg_t;

/**
 * @brief Initialize radar control module
 * @return 0 on success, negative on error
 */
int32_t RadarControl_init(void);

/**
 * @brief Open mmWave device
 * @return 0 on success, negative on error
 */
int32_t RadarControl_open(void);

/**
 * @brief Close mmWave device
 * @return 0 on success, negative on error
 */
int32_t RadarControl_close(void);

/**
 * @brief Start radar sensing
 * @return 0 on success, negative on error
 */
int32_t RadarControl_start(void);

/**
 * @brief Stop radar sensing
 * @return 0 on success, negative on error
 */
int32_t RadarControl_stop(void);

/**
 * @brief Configure frame parameters
 * @param frameCfg Frame configuration
 * @return 0 on success, negative on error
 */
int32_t RadarControl_configFrame(const RadarControl_FrameCfg_t* frameCfg);

/**
 * @brief Get current radar state
 * @return Current state
 */
RadarControl_State_e RadarControl_getState(void);

/**
 * @brief Get radar temperature (for monitoring)
 * @param tempDegC Output: temperature in degrees Celsius
 * @return 0 on success, negative on error
 */
int32_t RadarControl_getTemperature(float* tempDegC);

#ifdef __cplusplus
}
#endif

#endif /* RADAR_CONTROL_H */
