/**
 * @file shared_memory.h
 * @brief L3共享RAM内存映射（MSS和DSS共享）
 * 
 * 参考：第3章 3.3.3节 共享RAM使用规划
 * 基于InCabin架构学习
 */

#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <stdint.h>

/**************************************************************************
 * L3 共享RAM基本信息
 **************************************************************************/
#define L3_SRAM_BASE            0x51000000U
#define L3_SRAM_SIZE            (896U * 1024U)    /* 896KB */

/**************************************************************************
 * 共享RAM区域划分（参考第3章图示）
 **************************************************************************/
/* DPC配置区 (4KB) - MSS写，DSS读 */
#define DPC_CONFIG_OFFSET       0x00000U
#define DPC_CONFIG_SIZE         (4U * 1024U)

/* 点云数据区 (8KB) - DSS写，MSS读 */
#define POINT_CLOUD_OFFSET      0x01000U
#define POINT_CLOUD_SIZE        (8U * 1024U)

/* 特征数据区 (4KB) - DSS写，MSS读 🔥本章重点 */
#define FEATURE_DATA_OFFSET     0x03000U
#define FEATURE_DATA_SIZE       (4U * 1024U)

/* DPC结果区 (4KB) - DSS写，MSS读 */
#define DPC_RESULT_OFFSET       0x04000U
#define DPC_RESULT_SIZE         (4U * 1024U)

/* DSS工作缓冲区 (876KB) - DSS内部使用 */
#define SCRATCH_BUFFER_OFFSET   0x05000U
#define SCRATCH_BUFFER_SIZE     (876U * 1024U)

/**************************************************************************
 * 共享RAM绝对地址
 **************************************************************************/
#define DPC_CONFIG_BASE         (L3_SRAM_BASE + DPC_CONFIG_OFFSET)
#define POINT_CLOUD_BASE        (L3_SRAM_BASE + POINT_CLOUD_OFFSET)
#define FEATURE_DATA_BASE       (L3_SRAM_BASE + FEATURE_DATA_OFFSET)
#define DPC_RESULT_BASE         (L3_SRAM_BASE + DPC_RESULT_OFFSET)
#define SCRATCH_BUFFER_BASE     (L3_SRAM_BASE + SCRATCH_BUFFER_OFFSET)

#endif /* SHARED_MEMORY_H */
