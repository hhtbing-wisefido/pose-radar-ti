/**
 * @file shared_memory.h
 * @brief L3 RAM Shared Memory Layout Definition
 *
 * Reference: AWRL6844_InCabin_Demos/src/common_mss_dss/dpif_mss_dss.h
 * Adapted for: Health Detection three-layer architecture
 *
 * RTOS: FreeRTOS (L-SDK mandatory)
 * Note: L-SDK uses FreeRTOS, NOT TI-RTOS/BIOS
 *
 * Created: 2026-01-08
 */

#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/*===========================================================================*/
/*                         Memory Region Definitions                          */
/*===========================================================================*/

/**
 * @brief L3 RAM Base Address (DSS view)
 * This is the shared L3 RAM accessible by both MSS (R5F) and DSS (C66x)
 */
#define L3_RAM_BASE_ADDR            (0x88000000U)

/**
 * @brief L3 RAM Total Size: 1408KB
 * AWRL6844 has 1408KB L3 RAM
 */
#define L3_RAM_TOTAL_SIZE           (0x160000U)   /* 1408 KB */

/**
 * @brief IPC Mailbox Region (1KB)
 * Used for MSS-DSS inter-processor communication
 */
#define L3_IPC_MBOX_OFFSET          (0x00000000U)
#define L3_IPC_MBOX_SIZE            (0x00000400U)  /* 1 KB */
#define L3_IPC_MBOX_ADDR            (L3_RAM_BASE_ADDR + L3_IPC_MBOX_OFFSET)

/**
 * @brief MSS L3 Region (704KB + 448KB = 1152KB)
 * Used by MSS for radar cube, detection matrix, point cloud
 */
#define L3_MSS_OFFSET               (L3_IPC_MBOX_SIZE)
#define L3_MSS_SIZE                 (0x120000U)   /* 1152 KB (0xB0000 + 0x70000) */
#define L3_MSS_ADDR                 (L3_RAM_BASE_ADDR + L3_MSS_OFFSET)

/**
 * @brief DSS L3 Region (remaining ~255KB)
 * Used by DSS for algorithm processing
 */
#define L3_DSS_OFFSET               (L3_MSS_OFFSET + L3_MSS_SIZE)
#define L3_DSS_SIZE                 (L3_RAM_TOTAL_SIZE - L3_DSS_OFFSET)
#define L3_DSS_ADDR                 (L3_RAM_BASE_ADDR + L3_DSS_OFFSET)

/*===========================================================================*/
/*                     Health Detect Shared Memory Layout                     */
/*===========================================================================*/

/**
 * @brief DPC Configuration Buffer (4KB)
 * Contains radar configuration parameters
 */
#define SHARED_DPC_CONFIG_OFFSET    (0x00000000U)
#define SHARED_DPC_CONFIG_SIZE      (0x00001000U)  /* 4 KB */
#define SHARED_DPC_CONFIG_ADDR      (L3_MSS_ADDR + SHARED_DPC_CONFIG_OFFSET)

/**
 * @brief DPC Result Buffer (4KB)
 * Contains processing results from DSP
 */
#define SHARED_DPC_RESULT_OFFSET    (SHARED_DPC_CONFIG_OFFSET + SHARED_DPC_CONFIG_SIZE)
#define SHARED_DPC_RESULT_SIZE      (0x00001000U)  /* 4 KB */
#define SHARED_DPC_RESULT_ADDR      (L3_MSS_ADDR + SHARED_DPC_RESULT_OFFSET)

/**
 * @brief Point Cloud Buffer (64KB)
 * Contains detected point cloud data
 */
#define SHARED_POINT_CLOUD_OFFSET   (SHARED_DPC_RESULT_OFFSET + SHARED_DPC_RESULT_SIZE)
#define SHARED_POINT_CLOUD_SIZE     (0x00010000U)  /* 64 KB */
#define SHARED_POINT_CLOUD_ADDR     (L3_MSS_ADDR + SHARED_POINT_CLOUD_OFFSET)

/**
 * @brief Range Profile Buffer (32KB)
 * Contains range FFT output
 */
#define SHARED_RANGE_PROFILE_OFFSET (SHARED_POINT_CLOUD_OFFSET + SHARED_POINT_CLOUD_SIZE)
#define SHARED_RANGE_PROFILE_SIZE   (0x00008000U)  /* 32 KB */
#define SHARED_RANGE_PROFILE_ADDR   (L3_MSS_ADDR + SHARED_RANGE_PROFILE_OFFSET)

/**
 * @brief Health Features Buffer (4KB)
 * Contains extracted health detection features
 */
#define SHARED_HEALTH_FEAT_OFFSET   (SHARED_RANGE_PROFILE_OFFSET + SHARED_RANGE_PROFILE_SIZE)
#define SHARED_HEALTH_FEAT_SIZE     (0x00001000U)  /* 4 KB */
#define SHARED_HEALTH_FEAT_ADDR     (L3_MSS_ADDR + SHARED_HEALTH_FEAT_OFFSET)

/**
 * @brief ADC Data Buffer (512KB)
 * Contains raw ADC samples for processing
 */
#define SHARED_ADC_DATA_OFFSET      (SHARED_HEALTH_FEAT_OFFSET + SHARED_HEALTH_FEAT_SIZE)
#define SHARED_ADC_DATA_SIZE        (0x00080000U)  /* 512 KB */
#define SHARED_ADC_DATA_ADDR        (L3_MSS_ADDR + SHARED_ADC_DATA_OFFSET)

/**
 * @brief Radar Cube Buffer (remaining MSS L3 space)
 * Contains range-doppler processed data
 */
#define SHARED_RADAR_CUBE_OFFSET    (SHARED_ADC_DATA_OFFSET + SHARED_ADC_DATA_SIZE)
#define SHARED_RADAR_CUBE_SIZE      (L3_MSS_SIZE - SHARED_RADAR_CUBE_OFFSET)
#define SHARED_RADAR_CUBE_ADDR      (L3_MSS_ADDR + SHARED_RADAR_CUBE_OFFSET)

/*===========================================================================*/
/*                         Maximum Limits                                     */
/*===========================================================================*/

/** @brief Maximum number of detected points */
#define SHARED_MAX_DETECTED_POINTS  (500U)

/** @brief Maximum number of range bins */
#define SHARED_MAX_RANGE_BINS       (256U)

/** @brief Maximum number of Doppler bins */
#define SHARED_MAX_DOPPLER_BINS     (64U)

/** @brief Maximum number of TX antennas */
#define SHARED_MAX_TX_ANTENNAS      (4U)

/** @brief Maximum number of RX antennas */
#define SHARED_MAX_RX_ANTENNAS      (4U)

/** @brief Maximum number of virtual antennas (TX * RX) */
#define SHARED_MAX_VIRTUAL_ANTENNAS (SHARED_MAX_TX_ANTENNAS * SHARED_MAX_RX_ANTENNAS)

/*===========================================================================*/
/*                         Memory Validation Macros                           */
/*===========================================================================*/

/**
 * @brief Validate shared memory layout at compile time
 */
#define SHARED_MEMORY_LAYOUT_VALID  \
    ((SHARED_RADAR_CUBE_OFFSET + SHARED_RADAR_CUBE_SIZE) <= L3_MSS_SIZE)

#ifdef __cplusplus
}
#endif

#endif /* SHARED_MEMORY_H */
