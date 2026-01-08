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
 * @file health_detect_dss.c
 * @brief Health Detection DSS Main Implementation
 *
 * Reference: AWRL6844_InCabin_Demos/src/dss/source/mmwave_demo_dss.c
 * Adapted for: Health Detection three-layer architecture
 *
 * This file implements the DSS (C66 DSP) subsystem for radar signal
 * processing in the health detection application.
 *
 * Created: 2026-01-08
 */

/**************************************************************************
 *************************** Include Files ********************************
 **************************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* MCU Plus SDK Files */
#include <kernel/dpl/SemaphoreP.h>
#include <kernel/dpl/CacheP.h>
#include <kernel/dpl/ClockP.h>
#include <kernel/dpl/DebugP.h>
#include <kernel/dpl/HwiP.h>

/* SysConfig Generated Files */
#include "ti_drivers_config.h"
#include "ti_drivers_open_close.h"
#include "ti_board_open_close.h"
#include "ti_board_config.h"

/* FreeRTOS */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

/* Project Headers */
#include "health_detect_dss.h"

/**************************************************************************
 *************************** Macros ***************************************
 **************************************************************************/

/* Frame reference timer clock for timing stats */
#define FRAME_REF_TIMER_CLOCK_MHZ   (40U)

/**************************************************************************
 *************************** Memory Sections ******************************
 **************************************************************************/

/* L3 Heap Memory (DDR/Shared) */
#pragma DATA_SECTION(gDssL3HeapMem, ".ddrHeap")
uint8_t gDssL3HeapMem[DSS_L3_HEAP_SIZE];

/* L2 Heap Memory */
#pragma DATA_SECTION(gDssL2HeapMem, ".L2heap")
uint8_t gDssL2HeapMem[DSS_L2_HEAP_SIZE];

/* L2 Scratch Memory */
#pragma DATA_SECTION(gDssL2ScratchMem, ".L2ScratchSect")
uint8_t gDssL2ScratchMem[DSS_L2_SCRATCH_SIZE];

/* L1 Heap Memory */
#pragma DATA_SECTION(gDssL1HeapMem, ".L1heap")
uint8_t gDssL1HeapMem[DSS_L1_HEAP_SIZE];

/* L1 Scratch Memory */
#pragma DATA_SECTION(gDssL1ScratchMem, ".L1ScratchSect")
uint8_t gDssL1ScratchMem[DSS_L1_SCRATCH_SIZE];

/**************************************************************************
 *************************** Global Variables *****************************
 **************************************************************************/

/** @brief Global DSS MCB instance */
HealthDSS_MCB_t gHealthDssMCB;

/** @brief DPC Task Handle */
TaskHandle_t gDssDpcTask;
StaticTask_t gDssDpcTaskObj;
StackType_t  gDssDpcTaskStack[DSS_DPC_TASK_STACK_SIZE / sizeof(StackType_t)] __attribute__((aligned(32)));

/**************************************************************************
 *************************** Local Function Prototypes ********************
 **************************************************************************/

static void HealthDSS_dpcTask(void *args);
static void HealthDSS_ipcCallback(uint32_t remoteCoreId, uint16_t localClientId, 
                                   uint64_t msgValue, int32_t crcStatus, void *arg);
static int32_t HealthDSS_memPoolAlloc(HealthDSS_MemPool_t *pool, uint32_t size, void **ptr);
static void HealthDSS_memPoolReset(HealthDSS_MemPool_t *pool);

/**************************************************************************
 *************************** Action Functions *****************************
 **************************************************************************/

/**
 * @brief Action: Handle configuration event
 */
static void action_configure(void *arg)
{
    SubFrame_Cfg_t *cfg = (SubFrame_Cfg_t *)arg;
    
    DebugP_log("DSS: Received configuration\r\n");
    
    if (cfg != NULL)
    {
        /* Store configuration */
        memcpy(&gHealthDssMCB.subframeCfg, cfg, sizeof(SubFrame_Cfg_t));
        
        /* Update derived parameters */
        gHealthDssMCB.numChirpsPerFrame = cfg->numChirpsPerFrame;
        gHealthDssMCB.numRangeBins = cfg->numRangeBins;
        gHealthDssMCB.numDopplerBins = cfg->numDopplerBins;
        gHealthDssMCB.numVirtualAntennas = cfg->numRxAntennas * cfg->numTxAntennas;
    }
    
    /* Update state */
    gHealthDssMCB.currentState = HEALTH_DSS_STATE_CONFIGURED;
    gHealthDssMCB.configCount++;
    
    /* TODO: Send configuration complete to MSS via IPC */
    /* IpcNotify_sendMsg(...); */
    
    gHealthDssMCB.frameProcToken--;
}

/**
 * @brief Action: Process radar cube
 */
static void action_process(void *arg)
{
    uint32_t startTime, endTime;
    
    /* Get start time */
    startTime = ClockP_getTimeUsec();
    
    DebugP_log("DSS: Processing frame %d\r\n", gHealthDssMCB.frameCount);
    
    /* ========== Signal Processing Pipeline ========== */
    
    /* Step 1: Range Processing (FFT on ADC samples)
     * Input: Raw ADC samples in radar cube
     * Output: Range bins
     * TODO: Implement using HWA or C66 DSP
     */
    
    /* Step 2: Doppler Processing (FFT across chirps)
     * Input: Range bins
     * Output: Range-Doppler matrix
     * TODO: Implement Doppler FFT
     */
    
    /* Step 3: CFAR Detection
     * Input: Range-Doppler matrix
     * Output: Detected peaks
     * TODO: Implement CA-CFAR or OS-CFAR
     */
    
    /* Step 4: Angle Estimation (if multiple antennas)
     * Input: Detected peaks
     * Output: Angle of arrival for each detection
     * TODO: Implement angle estimation
     */
    
    /* Step 5: Point Cloud Generation
     * Input: Range, Doppler, Angle for each detection
     * Output: 3D point cloud (x, y, z, velocity, SNR)
     * TODO: Convert to Cartesian coordinates
     */
    
    /* Step 6: Health Feature Extraction (future)
     * Input: Point cloud, Range-Doppler matrix
     * Output: Breathing rate, heart rate estimates
     * TODO: Implement vital signs extraction
     */
    
    /* ========== End of Pipeline ========== */
    
    /* Get end time and calculate processing time */
    endTime = ClockP_getTimeUsec();
    gHealthDssMCB.stats.processingTimeUs = endTime - startTime;
    
    /* Update statistics */
    gHealthDssMCB.stats.framesProcessed++;
    gHealthDssMCB.frameCount++;
    
    if (gHealthDssMCB.stats.processingTimeUs > gHealthDssMCB.stats.maxProcessingTimeUs)
    {
        gHealthDssMCB.stats.maxProcessingTimeUs = gHealthDssMCB.stats.processingTimeUs;
    }
    
    /* TODO: Send processing complete to MSS via IPC */
    /* IpcNotify_sendMsg(...); */
    
    gHealthDssMCB.frameProcToken--;
}

/**
 * @brief Action: Stop processing
 */
static void action_stop(void *arg)
{
    DebugP_log("DSS: Stop processing\r\n");
    
    gHealthDssMCB.currentState = HEALTH_DSS_STATE_STOPPED;
    gHealthDssMCB.frameProcToken--;
}

/**
 * @brief Action: No operation
 */
static void action_noop(void *arg)
{
    /* Do nothing */
    gHealthDssMCB.frameProcToken--;
}

/**************************************************************************
 *************************** FSM Transition Table *************************
 **************************************************************************/

typedef void (*ActionFn_t)(void *arg);

typedef struct FSM_Transition_t
{
    ActionFn_t          action;
    HealthDSS_State_e   nextState;
} FSM_Transition_t;

static FSM_Transition_t gFsmTable[HEALTH_DSS_STATE_NUM][HEALTH_DSS_EVT_NUM] = 
{
    /* STATE_IDLE */
    [HEALTH_DSS_STATE_IDLE] = {
        [HEALTH_DSS_EVT_CONFIGURE]        = { action_configure, HEALTH_DSS_STATE_CONFIGURED },
        [HEALTH_DSS_EVT_RADAR_CUBE_READY] = { action_noop,      HEALTH_DSS_STATE_IDLE },
        [HEALTH_DSS_EVT_STOP]             = { action_noop,      HEALTH_DSS_STATE_IDLE },
    },
    
    /* STATE_CONFIGURED */
    [HEALTH_DSS_STATE_CONFIGURED] = {
        [HEALTH_DSS_EVT_CONFIGURE]        = { action_configure, HEALTH_DSS_STATE_CONFIGURED },
        [HEALTH_DSS_EVT_RADAR_CUBE_READY] = { action_process,   HEALTH_DSS_STATE_RUNNING },
        [HEALTH_DSS_EVT_STOP]             = { action_stop,      HEALTH_DSS_STATE_STOPPED },
    },
    
    /* STATE_RUNNING */
    [HEALTH_DSS_STATE_RUNNING] = {
        [HEALTH_DSS_EVT_CONFIGURE]        = { action_configure, HEALTH_DSS_STATE_CONFIGURED },
        [HEALTH_DSS_EVT_RADAR_CUBE_READY] = { action_process,   HEALTH_DSS_STATE_RUNNING },
        [HEALTH_DSS_EVT_STOP]             = { action_stop,      HEALTH_DSS_STATE_STOPPED },
    },
    
    /* STATE_STOPPED */
    [HEALTH_DSS_STATE_STOPPED] = {
        [HEALTH_DSS_EVT_CONFIGURE]        = { action_configure, HEALTH_DSS_STATE_CONFIGURED },
        [HEALTH_DSS_EVT_RADAR_CUBE_READY] = { action_noop,      HEALTH_DSS_STATE_STOPPED },
        [HEALTH_DSS_EVT_STOP]             = { action_noop,      HEALTH_DSS_STATE_STOPPED },
    },
};

/**************************************************************************
 *************************** Public Functions *****************************
 **************************************************************************/

/**
 * @brief Initialize DSS subsystem
 */
int32_t HealthDSS_init(HealthDSS_DPC_InitParams_t *initParams)
{
    int32_t status = 0;
    
    /* Clear MCB */
    memset(&gHealthDssMCB, 0, sizeof(HealthDSS_MCB_t));
    
    /* Initialize memory pools */
    gHealthDssMCB.l3Pool.baseAddr = gDssL3HeapMem;
    gHealthDssMCB.l3Pool.totalSize = DSS_L3_HEAP_SIZE;
    gHealthDssMCB.l3Pool.currAddr = (uintptr_t)gDssL3HeapMem;
    gHealthDssMCB.l3Pool.maxAddr = (uintptr_t)gDssL3HeapMem;
    
    gHealthDssMCB.l2Pool.baseAddr = gDssL2HeapMem;
    gHealthDssMCB.l2Pool.totalSize = DSS_L2_HEAP_SIZE;
    gHealthDssMCB.l2Pool.currAddr = (uintptr_t)gDssL2HeapMem;
    gHealthDssMCB.l2Pool.maxAddr = (uintptr_t)gDssL2HeapMem;
    
    /* Initialize semaphores */
    status = SemaphoreP_constructBinary(&gHealthDssMCB.initCompleteSem, 0);
    if (status != SystemP_SUCCESS)
    {
        return HEALTH_DSS_EINVAL_CFG;
    }
    
    status = SemaphoreP_constructBinary(&gHealthDssMCB.frameReadySem, 0);
    if (status != SystemP_SUCCESS)
    {
        return HEALTH_DSS_EINVAL_CFG;
    }
    
    /* Create event queue */
    gHealthDssMCB.eventQueue = xQueueCreate(8, sizeof(HealthDSS_Msg_t));
    if (gHealthDssMCB.eventQueue == NULL)
    {
        return HEALTH_DSS_ENOMEM_L2;
    }
    
    /* Set initial state */
    gHealthDssMCB.currentState = HEALTH_DSS_STATE_IDLE;
    gHealthDssMCB.isInitialized = true;
    
    return 0;
}

/**
 * @brief Configure DSS for processing
 */
int32_t HealthDSS_configure(SubFrame_Cfg_t *cfg)
{
    if (!gHealthDssMCB.isInitialized)
    {
        return HEALTH_DSS_EINVAL_STATE;
    }
    
    if (cfg == NULL)
    {
        return HEALTH_DSS_EINVAL_CFG;
    }
    
    /* Store configuration */
    memcpy(&gHealthDssMCB.subframeCfg, cfg, sizeof(SubFrame_Cfg_t));
    
    /* Reset memory pools for new configuration */
    HealthDSS_memPoolReset(&gHealthDssMCB.l3Pool);
    HealthDSS_memPoolReset(&gHealthDssMCB.l2Pool);
    
    /* Update state */
    gHealthDssMCB.currentState = HEALTH_DSS_STATE_CONFIGURED;
    
    return 0;
}

/**
 * @brief Process one frame of radar data
 */
int32_t HealthDSS_process(HealthDSS_Result_t *result)
{
    if (!gHealthDssMCB.isInitialized)
    {
        return HEALTH_DSS_EINVAL_STATE;
    }
    
    if (gHealthDssMCB.currentState != HEALTH_DSS_STATE_CONFIGURED &&
        gHealthDssMCB.currentState != HEALTH_DSS_STATE_RUNNING)
    {
        return HEALTH_DSS_EINVAL_STATE;
    }
    
    /* Processing is done in the action_process function triggered by IPC */
    
    /* Copy result if provided */
    if (result != NULL)
    {
        memcpy(result, &gHealthDssMCB.result, sizeof(HealthDSS_Result_t));
    }
    
    return 0;
}

/**
 * @brief Stop DSS processing
 */
int32_t HealthDSS_stop(void)
{
    gHealthDssMCB.currentState = HEALTH_DSS_STATE_STOPPED;
    return 0;
}

/**
 * @brief Get processing statistics
 */
int32_t HealthDSS_getStats(HealthDSS_Stats_t *stats)
{
    if (stats == NULL)
    {
        return HEALTH_DSS_EINVAL_CFG;
    }
    
    memcpy(stats, &gHealthDssMCB.stats, sizeof(HealthDSS_Stats_t));
    
    /* Calculate memory usage */
    stats->l3MemUsed = gHealthDssMCB.l3Pool.maxAddr - (uintptr_t)gHealthDssMCB.l3Pool.baseAddr;
    stats->l2MemUsed = gHealthDssMCB.l2Pool.maxAddr - (uintptr_t)gHealthDssMCB.l2Pool.baseAddr;
    
    return 0;
}

/**
 * @brief Handle IPC message from MSS
 */
void HealthDSS_handleMessage(HealthDSS_Msg_t *msg)
{
    FSM_Transition_t transition;
    
    if (msg->event >= HEALTH_DSS_EVT_NUM)
    {
        DebugP_assert(0);
        return;
    }
    
    /* Get transition from FSM table */
    transition = gFsmTable[gHealthDssMCB.currentState][msg->event];
    
    /* Execute action */
    if (transition.action != NULL)
    {
        transition.action((void *)(uintptr_t)msg->arg);
    }
    
    /* Update state */
    gHealthDssMCB.currentState = transition.nextState;
}

/**************************************************************************
 *************************** Local Functions ******************************
 **************************************************************************/

/**
 * @brief IPC callback for messages from MSS
 */
static void HealthDSS_ipcCallback(uint32_t remoteCoreId, uint16_t localClientId,
                                   uint64_t msgValue, int32_t crcStatus, void *arg)
{
    HealthDSS_Msg_t msg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    /* Parse message */
    msg.event = (uint32_t)((msgValue >> 32) & 0xFFFF);
    msg.arg = (uint32_t)(msgValue & 0xFFFFFFFF);
    
    /* Check for processing token overflow */
    if (gHealthDssMCB.frameProcToken != 0)
    {
        gHealthDssMCB.stats.overflowCount++;
    }
    gHealthDssMCB.frameProcToken++;
    
    /* Send to event queue */
    if (gHealthDssMCB.eventQueue != NULL)
    {
        if (xQueueSendFromISR(gHealthDssMCB.eventQueue, &msg, &xHigherPriorityTaskWoken) != pdPASS)
        {
            DebugP_assert(0);
        }
    }
    
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
 * @brief Allocate memory from pool
 */
static int32_t HealthDSS_memPoolAlloc(HealthDSS_MemPool_t *pool, uint32_t size, void **ptr)
{
    uintptr_t alignedAddr;
    uintptr_t endAddr;
    
    /* Align to 8 bytes */
    alignedAddr = (pool->currAddr + 7U) & ~7U;
    endAddr = alignedAddr + size;
    
    /* Check bounds */
    if (endAddr > ((uintptr_t)pool->baseAddr + pool->totalSize))
    {
        *ptr = NULL;
        return -1;
    }
    
    *ptr = (void *)alignedAddr;
    pool->currAddr = endAddr;
    
    /* Track maximum usage */
    if (endAddr > pool->maxAddr)
    {
        pool->maxAddr = endAddr;
    }
    
    return 0;
}

/**
 * @brief Reset memory pool
 */
static void HealthDSS_memPoolReset(HealthDSS_MemPool_t *pool)
{
    pool->currAddr = (uintptr_t)pool->baseAddr;
    /* Don't reset maxAddr - keep for statistics */
}

/**
 * @brief DPC processing task
 */
static void HealthDSS_dpcTask(void *args)
{
    HealthDSS_Msg_t msg;
    
    while (1)
    {
        /* Wait for event from queue */
        if (xQueueReceive(gHealthDssMCB.eventQueue, &msg, portMAX_DELAY) == pdPASS)
        {
            HealthDSS_handleMessage(&msg);
        }
    }
}

/**************************************************************************
 *************************** Main Entry Point *****************************
 **************************************************************************/

/**
 * @brief DSS main entry point
 *
 * Called from the FreeRTOS main task after scheduler starts.
 */
void health_detect_dss(void *args)
{
    int32_t status;
    HealthDSS_DPC_InitParams_t initParams;
    
    /* Initialize drivers */
    Drivers_open();
    Board_driversOpen();
    
    DebugP_log("Health Detection DSS: Starting...\r\n");
    
    /* Setup initialization parameters */
    memset(&initParams, 0, sizeof(HealthDSS_DPC_InitParams_t));
    
    initParams.l3Heap.baseAddr = gDssL3HeapMem;
    initParams.l3Heap.totalSize = DSS_L3_HEAP_SIZE;
    
    initParams.l2Heap.baseAddr = gDssL2HeapMem;
    initParams.l2Heap.totalSize = DSS_L2_HEAP_SIZE;
    
    initParams.l2Scratch.baseAddr = gDssL2ScratchMem;
    initParams.l2Scratch.totalSize = DSS_L2_SCRATCH_SIZE;
    
    initParams.l1Heap.baseAddr = gDssL1HeapMem;
    initParams.l1Heap.totalSize = DSS_L1_HEAP_SIZE;
    
    initParams.l1Scratch.baseAddr = gDssL1ScratchMem;
    initParams.l1Scratch.totalSize = DSS_L1_SCRATCH_SIZE;
    
    /* Initialize DSS */
    status = HealthDSS_init(&initParams);
    if (status != 0)
    {
        DebugP_log("Health Detection DSS: Init failed with error %d\r\n", status);
        DebugP_assert(0);
    }
    
    /* TODO: Configure IPC with MSS */
    /* IpcNotify_registerClient(...); */
    
    /* TODO: Sync with MSS */
    /* IpcNotify_syncAll(...); */
    
    DebugP_log("Health Detection DSS: Initialized, entering event loop\r\n");
    
    /* Main event loop - process events from MSS */
    HealthDSS_Msg_t msg;
    while (1)
    {
        if (xQueueReceive(gHealthDssMCB.eventQueue, &msg, portMAX_DELAY) == pdPASS)
        {
            HealthDSS_handleMessage(&msg);
        }
    }
    
    /* Note: Code below is intentionally unreachable - kept for shutdown sequence reference */
#if 0
    SemaphoreP_pend(&gHealthDssMCB.initCompleteSem, SystemP_WAIT_FOREVER);
    
    Board_driversClose();
    Drivers_close();
#endif
}
