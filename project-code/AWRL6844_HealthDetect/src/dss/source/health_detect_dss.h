/*
 * Copyright (C) 2024 Texas Instruments Incorporated
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 *   Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the
 *   distribution.
 *
 *   Neither the name of Texas Instruments Incorporated nor the names of
 *   its contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file health_detect_dss.h
 * @brief Health Detection DSS Main Header
 *
 * Reference: AWRL6844_InCabin_Demos/src/dss/source/mmwave_demo_dss.h
 * Adapted for: Health Detection three-layer architecture
 *
 * This file defines the DSS (C66 DSP) subsystem structures and interfaces
 * for radar signal processing in the health detection application.
 *
 * Created: 2026-01-08
 */

#ifndef HEALTH_DETECT_DSS_H
#define HEALTH_DETECT_DSS_H

/**************************************************************************
 *************************** Include Files ********************************
 **************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <kernel/dpl/SemaphoreP.h>
#include <kernel/dpl/HeapP.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

/* Common project headers */
#include "../../common/shared_memory.h"
#include "../../common/data_path.h"
#include "../../common/health_detect_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************
 *************************** Macros ***************************************
 **************************************************************************/

/** @brief Maximum detected objects per frame */
#define HEALTH_DSS_MAX_DET_OBJ              (500U)

/** @brief Maximum point cloud list size */
#define HEALTH_DSS_POINT_CLOUD_MAX_SIZE     (500U)

/** @brief FreeRTOS task priorities */
#define DSS_DPC_TASK_PRI                    (5U)
#define DSS_PROCESS_TASK_PRI                (4U)

/** @brief FreeRTOS task stack sizes */
#define DSS_DPC_TASK_STACK_SIZE             (8192U)
#define DSS_PROCESS_TASK_STACK_SIZE         (4096U)

/** @brief Memory heap sizes */
#define DSS_L3_HEAP_SIZE                    (0x30000U)  /* 192KB L3 heap */
#define DSS_L2_HEAP_SIZE                    (0x26000U)  /* 152KB L2 heap */
#define DSS_L2_SCRATCH_SIZE                 (0x1000U)   /* 4KB L2 scratch */
#define DSS_L1_HEAP_SIZE                    (0x2000U)   /* 8KB L1 heap */
#define DSS_L1_SCRATCH_SIZE                 (0x2000U)   /* 8KB L1 scratch */

/**************************************************************************
 *************************** Error Codes **********************************
 **************************************************************************/

#define HEALTH_DSS_ENOMEM_L3                (-1)
#define HEALTH_DSS_ENOMEM_L2                (-2)
#define HEALTH_DSS_ENOMEM_L1                (-3)
#define HEALTH_DSS_EINVAL_CFG               (-4)
#define HEALTH_DSS_EINVAL_STATE             (-5)
#define HEALTH_DSS_EIPC_FAIL                (-6)

/**************************************************************************
 *************************** Type Definitions *****************************
 **************************************************************************/

/**
 * @brief DSS Processing States (FSM)
 */
typedef enum HealthDSS_State_e
{
    HEALTH_DSS_STATE_IDLE = 0,      /**< Idle, waiting for configuration */
    HEALTH_DSS_STATE_CONFIGURED,    /**< Configured, ready to start */
    HEALTH_DSS_STATE_RUNNING,       /**< Processing frames */
    HEALTH_DSS_STATE_STOPPED,       /**< Stopped, can be reconfigured */
    HEALTH_DSS_STATE_NUM            /**< Number of states */
} HealthDSS_State_e;

/**
 * @brief DSS Event Types (from MSS via IPC)
 */
typedef enum HealthDSS_Event_e
{
    HEALTH_DSS_EVT_CONFIGURE = 0,   /**< Configuration request from MSS */
    HEALTH_DSS_EVT_RADAR_CUBE_READY,/**< Radar cube data ready */
    HEALTH_DSS_EVT_STOP,            /**< Stop processing request */
    HEALTH_DSS_EVT_NUM              /**< Number of events */
} HealthDSS_Event_e;

/**
 * @brief Memory Pool Object for heap management
 */
typedef struct HealthDSS_MemPool_t
{
    void        *baseAddr;      /**< Base address of memory pool */
    uint32_t    totalSize;      /**< Total size of pool in bytes */
    uintptr_t   currAddr;       /**< Current allocation pointer */
    uintptr_t   maxAddr;        /**< Maximum address used (for stats) */
} HealthDSS_MemPool_t;

/**
 * @brief DPC Initialization Parameters
 */
typedef struct HealthDSS_DPC_InitParams_t
{
    /* L3 Memory */
    HealthDSS_MemPool_t l3Heap;
    
    /* L2 Memory */
    HealthDSS_MemPool_t l2Heap;
    HealthDSS_MemPool_t l2Scratch;
    
    /* L1 Memory */
    HealthDSS_MemPool_t l1Heap;
    HealthDSS_MemPool_t l1Scratch;
    
} HealthDSS_DPC_InitParams_t;

/**
 * @brief Processing Statistics
 */
typedef struct HealthDSS_Stats_t
{
    uint32_t    framesProcessed;        /**< Total frames processed */
    uint32_t    processingTimeUs;       /**< Last frame processing time in us */
    uint32_t    avgProcessingTimeUs;    /**< Average processing time */
    uint32_t    maxProcessingTimeUs;    /**< Maximum processing time */
    uint32_t    overflowCount;          /**< Frame overflow count */
    uint32_t    l3MemUsed;              /**< L3 memory used in bytes */
    uint32_t    l2MemUsed;              /**< L2 memory used in bytes */
} HealthDSS_Stats_t;

/**
 * @brief Processing Output Result
 */
typedef struct HealthDSS_Result_t
{
    uint32_t    numDetectedObj;         /**< Number of detected objects */
    void        *pointCloud;            /**< Pointer to point cloud data */
    uint32_t    pointCloudSize;         /**< Size of point cloud in bytes */
    void        *healthFeatures;        /**< Pointer to health feature data */
    uint32_t    healthFeaturesSize;     /**< Size of health features */
} HealthDSS_Result_t;

/**
 * @brief IPC Message Structure
 */
typedef struct HealthDSS_Msg_t
{
    uint32_t    event;                  /**< Event type */
    uint32_t    arg;                    /**< Event argument */
} HealthDSS_Msg_t;

/**
 * @brief DSS Main Control Block (MCB)
 *
 * Central data structure containing all DSS state and configuration
 */
typedef struct HealthDSS_MCB_t
{
    /*--- State Management ---*/
    HealthDSS_State_e   currentState;       /**< Current FSM state */
    bool                isInitialized;      /**< Initialization flag */
    
    /*--- Semaphores ---*/
    SemaphoreP_Object   initCompleteSem;    /**< Init completion semaphore */
    SemaphoreP_Object   frameReadySem;      /**< Frame ready semaphore */
    
    /*--- Event Queue ---*/
    QueueHandle_t       eventQueue;         /**< FreeRTOS event queue */
    
    /*--- Memory Pools ---*/
    HealthDSS_MemPool_t l3Pool;             /**< L3 RAM pool */
    HealthDSS_MemPool_t l2Pool;             /**< L2 RAM pool */
    HeapP_Object        rtosHeap;           /**< FreeRTOS heap */
    
    /*--- Radar Data ---*/
    void                *radarCubeAddr;     /**< Radar cube base address */
    uint32_t            radarCubeSize;      /**< Radar cube size */
    
    /*--- Configuration ---*/
    SubFrame_Cfg_t      subframeCfg;        /**< Subframe configuration */
    uint32_t            numChirpsPerFrame;  /**< Chirps per frame */
    uint32_t            numRangeBins;       /**< Number of range bins */
    uint32_t            numDopplerBins;     /**< Number of Doppler bins */
    uint32_t            numVirtualAntennas; /**< Virtual antenna count */
    
    /*--- Processing Results ---*/
    HealthDSS_Result_t  result;             /**< Processing output */
    HealthDSS_Stats_t   stats;              /**< Processing statistics */
    
    /*--- Synchronization ---*/
    uint32_t            frameProcToken;     /**< Frame processing token */
    uint32_t            configCount;        /**< Configuration counter */
    uint32_t            frameCount;         /**< Frame counter */
    
} HealthDSS_MCB_t;

/**************************************************************************
 *************************** Global Declarations **************************
 **************************************************************************/

/** @brief Global DSS MCB instance */
extern HealthDSS_MCB_t gHealthDssMCB;

/**************************************************************************
 *************************** Function Prototypes **************************
 **************************************************************************/

/**
 * @brief Initialize DSS subsystem
 *
 * @param[in] initParams    Initialization parameters
 * @return 0 on success, error code on failure
 */
int32_t HealthDSS_init(HealthDSS_DPC_InitParams_t *initParams);

/**
 * @brief Configure DSS for processing
 *
 * @param[in] cfg   Subframe configuration
 * @return 0 on success, error code on failure
 */
int32_t HealthDSS_configure(SubFrame_Cfg_t *cfg);

/**
 * @brief Process one frame of radar data
 *
 * @param[out] result   Processing result
 * @return 0 on success, error code on failure
 */
int32_t HealthDSS_process(HealthDSS_Result_t *result);

/**
 * @brief Stop DSS processing
 *
 * @return 0 on success, error code on failure
 */
int32_t HealthDSS_stop(void);

/**
 * @brief Get processing statistics
 *
 * @param[out] stats    Statistics output
 * @return 0 on success, error code on failure
 */
int32_t HealthDSS_getStats(HealthDSS_Stats_t *stats);

/**
 * @brief Handle IPC message from MSS
 *
 * @param[in] msg   Received message
 */
void HealthDSS_handleMessage(HealthDSS_Msg_t *msg);

/**
 * @brief DSS main entry point (called from main task)
 *
 * @param[in] args  Task arguments (unused)
 */
void health_detect_dss(void *args);

#ifdef __cplusplus
}
#endif

#endif /* HEALTH_DETECT_DSS_H */
