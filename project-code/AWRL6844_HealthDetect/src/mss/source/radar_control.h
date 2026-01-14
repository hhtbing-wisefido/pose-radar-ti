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
 * @param pMCB Pointer to HealthDetect MCB (contains cliCfg)
 * @return 0 on success, error code on failure
 */
int32_t RadarControl_config(HealthDetect_MCB_t *pMCB);

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

/**
 * @brief Get mmWave configuration pointer
 * @return Pointer to MMWave_Cfg structure
 */
MMWave_Cfg* RadarControl_getCfg(void);

/**
 * @brief Configure and Enable APLL (SDK Standard)
 * 参考: mmw_demo_SDK_reference/source/mmwave_demo.c line 395-450
 * 
 * 🔴 关键修复（问题36）：
 * - 完整的APLL配置流程
 * - 支持校准数据保存/恢复
 * - 支持396MHz/400MHz频率切换
 * 
 * @param apllFreqMHz APLL frequency in MHz (396.0 or 400.0)
 * @param saveRestoreCalData 0=RESTORE校准数据, 1=SAVE校准数据
 * @return 0 on success, error code on failure
 */
int32_t RadarControl_configAndEnableApll(float apllFreqMHz, uint8_t saveRestoreCalData);

/**
 * @brief Check if radar is initialized
 * @return 1 if initialized, 0 otherwise
 */
uint8_t RadarControl_isInitialized(void);

/**
 * @brief Check if radar is opened
 * @return 1 if opened, 0 otherwise
 */
uint8_t RadarControl_isOpened(void);

/**
 * @brief Check if radar is configured
 * @return 1 if configured, 0 otherwise
 */
uint8_t RadarControl_isConfigured(void);

#ifdef __cplusplus
}
#endif

#endif /* RADAR_CONTROL_H */
