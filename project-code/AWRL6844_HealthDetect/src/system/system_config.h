/**
 * @file system_config.h
 * @brief System Configuration Header
 * 
 * Compile-time configuration for Health Detect project
 * Created: 2026-01-08
 */

#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

/*===========================================================================
 * Target Platform
 *===========================================================================*/

#define PLATFORM_AWRL6844       1
#define PLATFORM_FREQUENCY_MHZ  400     /* R5F clock */
#define DSP_FREQUENCY_MHZ       450     /* C66x clock */

/*===========================================================================
 * SDK Version
 *===========================================================================*/

#define MMWAVE_L_SDK_VERSION_MAJOR  6
#define MMWAVE_L_SDK_VERSION_MINOR  5
#define MMWAVE_L_SDK_VERSION_PATCH  0

/*===========================================================================
 * RTOS Configuration
 *===========================================================================*/

/* FreeRTOS on MSS */
#define MSS_FREERTOS_TICK_RATE_HZ       1000
#define MSS_FREERTOS_MAX_PRIORITIES     10
#define MSS_FREERTOS_MINIMAL_STACK_SIZE 256

/*===========================================================================
 * Task Priorities (higher number = higher priority)
 *===========================================================================*/

#define TASK_PRI_INIT           5
#define TASK_PRI_CLI            4
#define TASK_PRI_DPC            6
#define TASK_PRI_OUTPUT         3
#define TASK_PRI_IDLE           1

/*===========================================================================
 * Task Stack Sizes (words, not bytes)
 *===========================================================================*/

#define INIT_TASK_STACK_SIZE    1024
#define CLI_TASK_STACK_SIZE     2048
#define DPC_TASK_STACK_SIZE     2048
#define OUTPUT_TASK_STACK_SIZE  1024

/*===========================================================================
 * UART Configuration
 *===========================================================================*/

#define CLI_UART_INSTANCE       0       /* UART0 for CLI */
#define CLI_UART_BAUDRATE       115200
#define DATA_UART_INSTANCE      1       /* UART1 for data output */
#define DATA_UART_BAUDRATE      921600

/*===========================================================================
 * Radar Configuration Limits
 *===========================================================================*/

#define MAX_NUM_CHIRPS          256
#define MAX_NUM_RANGE_BINS      512
#define MAX_NUM_DOPPLER_BINS    128
#define MAX_NUM_DETECTED_POINTS 1024
#define MAX_NUM_TRACKS          20

/*===========================================================================
 * L3 RAM Allocation
 *===========================================================================*/

#define L3_RAM_BASE_ADDR        0x51000000
#define L3_RAM_TOTAL_SIZE       (896 * 1024)    /* 896KB */

/* Section sizes */
#define DPC_CONFIG_ALLOC_SIZE   (4 * 1024)      /* 4KB */
#define DPC_RESULT_ALLOC_SIZE   (4 * 1024)      /* 4KB */
#define POINT_CLOUD_ALLOC_SIZE  (64 * 1024)     /* 64KB */
#define RANGE_PROFILE_ALLOC_SIZE (32 * 1024)    /* 32KB */
#define HEALTH_FEATURES_ALLOC_SIZE (4 * 1024)   /* 4KB */
#define ADC_DATA_ALLOC_SIZE     (512 * 1024)    /* 512KB */

/*===========================================================================
 * Health Detection Parameters
 *===========================================================================*/

#define PRESENCE_MIN_POINTS     5
#define PRESENCE_RANGE_MIN_M    0.5f
#define PRESENCE_RANGE_MAX_M    3.0f
#define PRESENCE_VELOCITY_THR   0.1f    /* m/s */
#define PRESENCE_HOLD_FRAMES    10

/*===========================================================================
 * Debug Configuration
 *===========================================================================*/

#define ENABLE_DEBUG_LOGS       1
#define ENABLE_STATS_LOGGING    1
#define STATS_LOG_INTERVAL_MS   1000

#endif /* SYSTEM_CONFIG_H */
