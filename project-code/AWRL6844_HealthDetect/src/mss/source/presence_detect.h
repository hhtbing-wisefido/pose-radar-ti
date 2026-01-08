/**
 * @file presence_detect.h
 * @brief Presence Detection Module Header
 *
 * Reference: New functionality for Health Detection project
 * Adapted for: Health Detection three-layer architecture
 *
 * RTOS: FreeRTOS (L-SDK mandatory)
 * Note: L-SDK uses FreeRTOS, NOT TI-RTOS/BIOS
 *
 * Created: 2026-01-08
 */

#ifndef PRESENCE_DETECT_H
#define PRESENCE_DETECT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <common/data_path.h>
#include <common/health_detect_types.h>

/*===========================================================================*/
/*                         Function Prototypes                                */
/*===========================================================================*/

/**
 * @brief Initialize presence detection module
 * @param config Pointer to configuration
 * @return 0 on success, error code on failure
 */
int32_t PresenceDetect_init(PresenceDetect_Config_t *config);

/**
 * @brief Configure presence detection parameters
 * @param config Pointer to configuration
 * @return 0 on success, error code on failure
 */
int32_t PresenceDetect_config(PresenceDetect_Config_t *config);

/**
 * @brief Process point cloud for presence detection
 * @param points Pointer to point cloud data
 * @param numPoints Number of detected points
 * @param result Pointer to store result
 * @return 0 on success, error code on failure
 */
int32_t PresenceDetect_process(PointCloud_Cartesian_t *points,
                                uint32_t numPoints,
                                PresenceDetect_Result_t *result);

/**
 * @brief Reset presence detection state
 * @return 0 on success, error code on failure
 */
int32_t PresenceDetect_reset(void);

/**
 * @brief Get current presence state
 * @return Current presence state
 */
PresenceState_e PresenceDetect_getState(void);

#ifdef __cplusplus
}
#endif

#endif /* PRESENCE_DETECT_H */
