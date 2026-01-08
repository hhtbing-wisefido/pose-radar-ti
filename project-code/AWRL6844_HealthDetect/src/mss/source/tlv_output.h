/**
 * @file tlv_output.h
 * @brief TLV Output Module Header
 *
 * Reference: mmw_demo_SDK_reference/source/mmwave_demo.c (TLV output section)
 * Adapted for: Health Detection three-layer architecture
 *
 * RTOS: FreeRTOS (L-SDK mandatory)
 * Note: L-SDK uses FreeRTOS, NOT TI-RTOS/BIOS
 *
 * Created: 2026-01-08
 */

#ifndef TLV_OUTPUT_H
#define TLV_OUTPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <drivers/uart.h>
#include <common/data_path.h>
#include <common/health_detect_types.h>
#include <common/mmwave_output.h>

/*===========================================================================*/
/*                         Function Prototypes                                */
/*===========================================================================*/

/**
 * @brief Initialize TLV output module
 * @param uartHandle UART handle for output
 * @return 0 on success, error code on failure
 */
int32_t TLV_init(UART_Handle uartHandle);

/**
 * @brief Send output data via UART
 * @param frameNum Current frame number
 * @param dpcResult Pointer to DPC result
 * @param presenceResult Pointer to presence detection result
 * @param healthFeatures Pointer to health features
 * @return 0 on success, error code on failure
 */
int32_t TLV_sendOutput(uint32_t frameNum,
                       DPC_Result_t *dpcResult,
                       PresenceDetect_Result_t *presenceResult,
                       HealthDetect_Features_t *healthFeatures);

/**
 * @brief Set UART handle for output
 * @param uartHandle UART handle
 */
void TLV_setUartHandle(UART_Handle uartHandle);

/**
 * @brief Enable/disable TLV types
 * @param tlvType TLV type to enable/disable
 * @param enable 1 to enable, 0 to disable
 */
void TLV_setEnabled(MmwDemo_output_message_type_e tlvType, uint8_t enable);

#ifdef __cplusplus
}
#endif

#endif /* TLV_OUTPUT_H */
