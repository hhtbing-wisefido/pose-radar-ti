/**
 * @file dpc_control.h
 * @brief DPC (Data Path Chain) Control Header
 *
 * Reference: mmw_demo_SDK_reference/source/dpc/dpc.c
 * Reference: AWRL6844_InCabin_Demos/src/mss/source/dpc/dpc_mss.c
 * Adapted for: Health Detection three-layer architecture
 *
 * RTOS: FreeRTOS (L-SDK mandatory)
 * Note: L-SDK uses FreeRTOS, NOT TI-RTOS/BIOS
 *
 * Created: 2026-01-08
 */

#ifndef DPC_CONTROL_H
#define DPC_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "common/data_path.h"

/*===========================================================================*/
/*                         Function Prototypes                                */
/*===========================================================================*/

/**
 * @brief Initialize DPC module
 * @return 0 on success, error code on failure
 */
int32_t DPC_init(void);

/**
 * @brief Configure DPC with given parameters
 * @param config Pointer to DPC configuration
 * @return 0 on success, error code on failure
 */
int32_t DPC_config(DPC_Config_t *config);

/**
 * @brief Execute DPC processing for one frame
 * @param result Pointer to store results
 * @return 0 on success, error code on failure
 */
int32_t DPC_execute(DPC_Result_t *result);

/**
 * @brief Reconfigure DPC during runtime
 * @param dynamicCfg Pointer to dynamic configuration
 * @return 0 on success, error code on failure
 */
int32_t DPC_reconfig(DPC_DynamicConfig_t *dynamicCfg);

/**
 * @brief Deinitialize DPC module
 * @return 0 on success, error code on failure
 */
int32_t DPC_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* DPC_CONTROL_H */
