/**
 * @file health_detect_main.h
 * @brief Health Detection Main Application Header
 *
 * Reference: mmw_demo_SDK_reference/source/mmwave_demo.h
 * Reference: AWRL6844_InCabin_Demos/src/mss/source/mmwave_demo_mss.h
 * Adapted for: Health Detection three-layer architecture
 *
 * RTOS: FreeRTOS (L-SDK mandatory)
 * Note: L-SDK uses FreeRTOS, NOT TI-RTOS/BIOS
 *
 * Created: 2026-01-08
 */

#ifndef HEALTH_DETECT_MAIN_H
#define HEALTH_DETECT_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/* FreeRTOS Includes */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

/* SDK DPL Includes */
#include <kernel/dpl/SemaphoreP.h>
#include <kernel/dpl/ClockP.h>
#include <kernel/dpl/DebugP.h>

/* mmWave SDK Includes */
#include <control/mmwave/mmwave.h>
#include <drivers/uart.h>
#include <drivers/hwa.h>
#include <drivers/edma.h>

/* Common Includes */
#include "common/data_path.h"
#include "common/health_detect_types.h"
#include "common/mmwave_output.h"

/*===========================================================================*/
/*                         Task Configuration                                 */
/*===========================================================================*/

/** @brief CLI Task Priority */
#define CLI_TASK_PRIORITY           (1U)

/** @brief DPC Task Priority */
#define DPC_TASK_PRIORITY           (5U)

/** @brief TLV Output Task Priority */
#define TLV_TASK_PRIORITY           (3U)

/** @brief Main Task Priority */
#define MAIN_TASK_PRIORITY          (configMAX_PRIORITIES - 1)

/** @brief CLI Task Stack Size */
#define CLI_TASK_STACK_SIZE         (4096U)

/** @brief DPC Task Stack Size */
#define DPC_TASK_STACK_SIZE         (8192U)

/** @brief TLV Task Stack Size */
#define TLV_TASK_STACK_SIZE         (2048U)

/** @brief Main Task Stack Size */
#define MAIN_TASK_STACK_SIZE        (16384U / sizeof(StackType_t))

/*===========================================================================*/
/*                         System State                                       */
/*===========================================================================*/

/**
 * @brief System State Enumeration
 */
typedef enum HealthDetect_State_e
{
    HEALTH_DETECT_STATE_INIT = 0,       /**< Initialization state */
    HEALTH_DETECT_STATE_IDLE,           /**< Idle, waiting for config */
    HEALTH_DETECT_STATE_CONFIGURED,     /**< Configured, ready to start */
    HEALTH_DETECT_STATE_RUNNING,        /**< Sensor running */
    HEALTH_DETECT_STATE_STOPPED,        /**< Sensor stopped */
    HEALTH_DETECT_STATE_ERROR,          /**< Error state */
    HEALTH_DETECT_STATE_MAX
} HealthDetect_State_e;

/*===========================================================================*/
/*                         CLI Configuration                                  */
/*===========================================================================*/

/**
 * @brief CLI Configuration Structure
 * Holds all configuration received via CLI commands
 */
typedef struct HealthDetect_CliCfg_t
{
    /* Radar Configuration */
    Profile_Config_t        profileCfg;         /**< Profile configuration */
    Chirp_Config_t          chirpCfg;           /**< Chirp configuration */
    Frame_Config_t          frameCfg;           /**< Frame configuration */

    /* Detection Configuration */
    CFAR_Range_Config_t     cfarRangeCfg;       /**< CFAR range config */
    CFAR_Doppler_Config_t   cfarDopplerCfg;     /**< CFAR Doppler config */
    AOA_Config_t            aoaCfg;             /**< AOA config */

    /* Health Detection Configuration */
    PresenceDetect_Config_t presenceCfg;        /**< Presence detection config */
    DetectionZone_t         zones[HEALTH_DETECT_MAX_ZONES]; /**< Detection zones */

    /* Channel Configuration */
    uint8_t                 rxChannelEn;        /**< RX channel enable mask */
    uint8_t                 txChannelEn;        /**< TX channel enable mask */

    /* Configuration Flags */
    uint8_t                 isProfileCfgPending;
    uint8_t                 isChirpCfgPending;
    uint8_t                 isFrameCfgPending;
    uint8_t                 isCfarCfgPending;
} HealthDetect_CliCfg_t;

/*===========================================================================*/
/*                         Master Control Block                               */
/*===========================================================================*/

/**
 * @brief Health Detection Master Control Block
 * Main application control structure
 */
typedef struct HealthDetect_MCB_t
{
    /* System State */
    HealthDetect_State_e    state;              /**< Current system state */
    uint32_t                frameNum;           /**< Current frame number */
    uint32_t                errorCode;          /**< Last error code */

    /* mmWave Control */
    MMWave_Handle           mmWaveHandle;       /**< mmWave control handle */
    uint8_t                 isSensorStarted;    /**< Sensor started flag */
    uint8_t                 isSensorStopped;    /**< Sensor stopped flag */

    /* Driver Handles */
    UART_Handle             uartHandle;         /**< UART handle for CLI/output */
    UART_Handle             uartLogHandle;      /**< UART handle for logging */
    HWA_Handle              hwaHandle;          /**< HWA handle */
    EDMA_Handle             edmaHandle;         /**< EDMA handle */

    /* Configuration */
    HealthDetect_CliCfg_t   cliCfg;             /**< CLI configuration */

    /* DPC Results */
    DPC_Result_t            dpcResult;          /**< DPC processing result */
    HealthDetect_Features_t healthFeatures;     /**< Health detection features */
    PresenceDetect_Result_t presenceResult;     /**< Presence detection result */

    /* Statistics */
    uint32_t                framePeriodUs;      /**< Measured frame period (us) */
    uint32_t                interFrameTimeUs;   /**< Inter-frame processing time */
    uint32_t                transmitTimeUs;     /**< Data transmit time */

    /* FreeRTOS Objects */
    TaskHandle_t            dpcTaskHandle;      /**< DPC task handle */
    TaskHandle_t            tlvTaskHandle;      /**< TLV task handle */
    SemaphoreHandle_t       dpcSemHandle;       /**< DPC semaphore handle */
    SemaphoreHandle_t       tlvSemHandle;       /**< TLV semaphore handle */

    /* Memory Buffers */
    uint8_t                 *l3RamPtr;          /**< L3 RAM pointer */
    uint32_t                l3RamSize;          /**< L3 RAM size */
    uint8_t                 *localRamPtr;       /**< Local RAM pointer */
    uint32_t                localRamSize;       /**< Local RAM size */

} HealthDetect_MCB_t;

/*===========================================================================*/
/*                         Global Variables                                   */
/*===========================================================================*/

/** @brief Global Master Control Block */
extern HealthDetect_MCB_t gHealthDetectMCB;

/*===========================================================================*/
/*                         Function Prototypes                                */
/*===========================================================================*/

/**
 * @brief Main application entry point (called from FreeRTOS main task)
 * @param args Task arguments (unused)
 */
void health_detect_main(void *args);

/**
 * @brief Initialize the Health Detection application
 * @return 0 on success, error code on failure
 */
int32_t HealthDetect_init(void);

/**
 * @brief Start the sensor
 * @return 0 on success, error code on failure
 */
int32_t HealthDetect_sensorStart(void);

/**
 * @brief Stop the sensor
 * @return 0 on success, error code on failure
 */
int32_t HealthDetect_sensorStop(void);

/**
 * @brief DPC processing task
 * @param args Task arguments (unused)
 */
void HealthDetect_dpcTask(void *args);

/**
 * @brief TLV output task
 * @param args Task arguments (unused)
 */
void HealthDetect_tlvTask(void *args);

/**
 * @brief Frame start callback
 * @param arg Callback argument
 */
void HealthDetect_frameStartCallback(void *arg);

/**
 * @brief Frame stop callback
 * @param arg Callback argument
 */
void HealthDetect_frameStopCallback(void *arg);

/**
 * @brief Assert handler for debug
 * @param expression Expression that failed
 * @param file File name
 * @param line Line number
 */
void HealthDetect_debugAssert(int32_t expression, const char *file, int32_t line);

/*===========================================================================*/
/*                         Debug Macros                                       */
/*===========================================================================*/

/** @brief Debug assert macro */
#define HealthDetect_assert(expr) \
    HealthDetect_debugAssert((expr), __FILE__, __LINE__)

#ifdef __cplusplus
}
#endif

#endif /* HEALTH_DETECT_MAIN_H */
