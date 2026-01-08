/**
 * @file radar_control.h
 * @brief Radar Control Module Header (mmWave API Wrapper)
 *
 * Reference: mmw_demo_SDK_reference/source/mmwave_control/
 * Reference: AWRL6844_InCabin_Demos/src/mss/source/mmwave_control/
 * Adapted for: Health Detection three-layer architecture
 *
 * RTOS: FreeRTOS (L-SDK mandatory)
 * Note: L-SDK uses FreeRTOS, NOT TI-RTOS/BIOS
 *
 * Created: 2026-01-08
 */

#ifndef RADAR_CONTROL_H
#define RADAR_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <control/mmwave/mmwave.h>
#include <source/health_detect_main.h>

/*===========================================================================*/
/*                         Function Prototypes                                */
/*===========================================================================*/

/**
 * @brief Initialize radar control module
 * @return 0 on success, error code on failure
 */
int32_t RadarControl_init(void);

/**
 * @brief Open radar device
 * @return 0 on success, error code on failure
 */
int32_t RadarControl_open(void);

/**
 * @brief Close radar device
 * @return 0 on success, error code on failure
 */
int32_t RadarControl_close(void);

/**
 * @brief Configure radar with CLI settings
 * @param cliCfg Pointer to CLI configuration
 * @return 0 on success, error code on failure
 */
int32_t RadarControl_config(HealthDetect_CliCfg_t *cliCfg);

/**
 * @brief Start radar sensor
 * @return 0 on success, error code on failure
 */
int32_t RadarControl_start(void);

/**
 * @brief Stop radar sensor
 * @return 0 on success, error code on failure
 */
int32_t RadarControl_stop(void);

/**
 * @brief Get mmWave handle
 * @return mmWave handle
 */
MMWave_Handle RadarControl_getHandle(void);

#ifdef __cplusplus
}
#endif

#endif /* RADAR_CONTROL_H */
