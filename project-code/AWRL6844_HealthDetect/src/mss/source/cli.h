/**
 * @file cli.h
 * @brief CLI Command Interface Header
 *
 * Reference: mmw_demo_SDK_reference/source/mmw_cli.h
 * Reference: AWRL6844_InCabin_Demos/src/mss/source/mmw_cli.h
 * Adapted for: Health Detection three-layer architecture
 *
 * RTOS: FreeRTOS (L-SDK mandatory)
 * Note: L-SDK uses FreeRTOS, NOT TI-RTOS/BIOS
 *
 * Created: 2026-01-08
 */

#ifndef CLI_H
#define CLI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* mmWave SDK Includes */
#include <control/mmwave/mmwave.h>

/* Driver Includes */
#include <drivers/uart.h>

/*===========================================================================*/
/*                         Forward Declarations                              */
/*===========================================================================*/

/**
 * @brief CLI Configuration Structure (Forward Declaration)
 * 完整定义在cli.c中 - 遵循SDK封装原则
 */
typedef struct CLI_Cfg_t CLI_Cfg;

/*===========================================================================*/
/*                         CLI Configuration                                  */
/*===========================================================================*/

/** @brief Maximum command line length */
#define CLI_MAX_CMD_LINE_LEN        (256U)

/** @brief Maximum number of command arguments */
#define CLI_MAX_ARGS                (32U)

/** @brief CLI prompt string - Must match mmw_demo for SDK Visualizer compatibility */
#define CLI_PROMPT                  "mmwDemo:/>"

/*===========================================================================*/
/*                         Function Prototypes                                */
/*===========================================================================*/

/**
 * @brief Initialize CLI module
 * @return 0 on success, error code on failure
 */
int32_t CLI_init(void);

/**
 * @brief Open CLI with configuration (SDK Standard)
 * 参考: mmw_demo_SDK_reference/source/mmw_cli.h
 * 
 * 🔴 关键修复（问题36）：
 * - SDK标准的CLI_open()接口
 * - 支持mmWave扩展命令配置
 * - SDK Visualizer兼容性必需
 * 
 * @param ptrCLICfg Pointer to CLI configuration
 * @return 0 on success, error code on failure
 */
int32_t CLI_open(CLI_Cfg* ptrCLICfg);

/**
 * @brief Run CLI (blocking)
 * This function enters CLI loop and processes commands
 */
void CLI_run(void);

/**
 * @brief Write output to CLI console
 * @param format Printf-style format string
 * @param ... Variable arguments
 * @return Number of characters written
 */
int32_t CLI_write(const char *format, ...);

/**
 * @brief Open radar configuration
 * @return 0 on success, error code on failure
 */
int32_t CLI_openRadar(void);

/**
 * @brief Close radar configuration
 * @return 0 on success, error code on failure
 */
int32_t CLI_closeRadar(void);

#ifdef __cplusplus
}
#endif

#endif /* CLI_H */
