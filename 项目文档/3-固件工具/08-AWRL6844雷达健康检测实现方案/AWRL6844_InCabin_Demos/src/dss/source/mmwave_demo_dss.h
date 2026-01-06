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
#ifndef MMWAVE_DEMO_DSS_H
#define MMWAVE_DEMO_DSS_H

#include <kernel/dpl/SemaphoreP.h>
#include <kernel/dpl/HeapP.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include <datapath/dpif/dpif_pointcloud.h>
#include <common_mss_dss/dpif_mss_dss.h>

#include <common/syscommon.h>
#include <common_mss_dss/msg_ipc/msg_ipc.h>

#include <source/dpc/objectdetection_dss.h>

#include <source/dpc/objectdetection_dss_internal.h>
#include <source/utilities/radarOsal_malloc.h>


#ifdef __cplusplus
extern "C" {
#endif

/** @brief Output Point cloud list size in number of list elements */
#define MMWDEMO_OUTPUT_POINT_CLOUD_LIST_MAX_SIZE 500

/** @brief Output packet length is a multiple of this value, must be power of 2*/
#define MMWDEMO_OUTPUT_MSG_SEGMENT_LEN 32


/*! @brief CFAR threshold encoding factor
 */
#define MMWDEMO_CFAR_THRESHOLD_ENCODING_FACTOR (100.0)

/*! @brief Demo freeRTOS tasks priorities
 */
#define DPC_TASK_PRI (5U)
#define ADC_FILEREAD_TASK_PRI (4U)
#define TLV_TASK_PRI (3U)
#define CLI_TASK_PRIORITY (1U)

/*! @brief Demo freeRTOS tasks stack sizes
 */
#define DPC_TASK_STACK_SIZE 8192
#define ADC_FILEREAD_TASK_STACK_SIZE 1024
#define TLV_TASK_STACK_SIZE 2048

#define DPC_ADC_FILENAME_MAX_LEN 256

/** @brief Output packet length is a multiple of this value, must be power of 2*/
#define MMWDEMO_OUTPUT_MSG_SEGMENT_LEN 32

/**
 * @brief   Error Code: Out of L3 RAM during radar cube allocation.
 */
#define DPC_OBJECTDETECTION_ENOMEM__L3_RAM_RADAR_CUBE            (DP_ERRNO_OBJECTDETECTION_BASE-1)

/**
 * @brief   Error Code: Out of L3 RAM during detection matrix allocation.
 */
#define DPC_OBJECTDETECTION_ENOMEM__L3_RAM_DET_MATRIX            (DP_ERRNO_OBJECTDETECTION_BASE-2)

/**
 * @brief   Error Code: Out of Core Local RAM for generating window coefficients
 *          for HWA when doing range DPU Config.
 */
#define DPC_OBJECTDETECTION_ENOMEM__CORE_LOCAL_RAM_RANGE_HWA_WINDOW    (DP_ERRNO_OBJECTDETECTION_BASE-3)

/**
 * @brief   Error Code: Out of Core Local RAM for generating window coefficients
 *          for HWA when doing doppler DPU Config.
 */
#define DPC_OBJECTDETECTION_ENOMEM__CORE_LOCAL_RAM_DOA_HWA_WINDOW    (DP_ERRNO_OBJECTDETECTION_BASE-4)

/**
 * @brief   Error Code: Out of Core Local RAM for generating window coefficients
 *          for HWA when doing doppler DPU Config.
 */
#define DPC_OBJECTDETECTION_ENOMEM__CORE_LOCAL_RAM_HWA_WINDOW    (DP_ERRNO_OBJECTDETECTION_BASE-5)

/**
 * @brief   Error Code: Out of Core Local RAM for range profile
 */
#define DPC_OBJECTDETECTION_ENOMEM__CORE_LOCAL_RAM_RANGE_PROFILE    (DP_ERRNO_OBJECTDETECTION_BASE-6)

/**
 * @brief   Error Code: Out of L3 RAM during ADC test buffer allocation allocation.
 */
#define DPC_OBJECTDETECTION_ENOMEM__L3_RAM_ADC_TEST_BUFF            (DP_ERRNO_OBJECTDETECTION_BASE-7)

/**
 * @brief   Error Code: Invalid configuration
 */
#define DPC_OBJECTDETECTION_EINVAL_CFG                              (DP_ERRNO_OBJECTDETECTION_BASE-8)

/**
 * @brief   Error Code: Antenna geometry configuration failed
 */
#define DPC_OBJECTDETECTION_EANTENNA_GEOMETRY_CFG_FAILED            (DP_ERRNO_OBJECTDETECTION_BASE-9)

/**
 * @brief   Error Code: Out of Core Local RAM for generating window coefficients
 *          for HWA when doing doppler DPU Config.
 */
#define DPC_OBJECTDETECTION_ENOMEM__CORE_LOCAL_RAM_AOA2D_HWA_WINDOW    (DP_ERRNO_OBJECTDETECTION_BASE-10)

/**
 * @brief   Error Code: Out of Core Local RAM.
 */
#define DPC_OBJECTDETECTION_ENOMEM__CORE_LOCAL_RAM_AOA_DET_OBJ_OUT                  (DP_ERRNO_OBJECTDETECTION_BASE-11)
/**
 * @brief   Error Code: Out of Core Local RAM.
 */
#define DPC_OBJECTDETECTION_ENOMEM__CORE_LOCAL_RAM_AOA_DET_OBJ_OUT_SIDE_INFO        (DP_ERRNO_OBJECTDETECTION_BASE-12)
/**
 * @brief   Error Code: Out of Core Local RAM for CFAR Doppler detection output
 */
#define DPC_OBJECTDETECTION_ENOMEM__CORE_LOCAL_RAM_CFAR_DOPPLER_DET_OUT_BIT_MASK    (DP_ERRNO_OBJECTDETECTION_BASE-13)
/**
 * @brief   Error Code: Out of Core Local RAM for CFAR Doppler detection output
 */
#define DPC_OBJECTDETECTION_ENOMEM__CORE_LOCAL_RAM_CFAR_OUT_DET_LIST                (DP_ERRNO_OBJECTDETECTION_BASE-14)

/**
 * @brief   Error Code: Out of Core Local RAM for detection Azimuth index output
 */
#define DPC_OBJECTDETECTION_ENOMEM__CORE_LOCAL_RAM_AOA_DET_OBJ_2_AZIM_IDX           (DP_ERRNO_OBJECTDETECTION_BASE-15)

/**
 * @brief   Error Code: Out of Core Local RAM for detection Elevation angle output
 */
#define DPC_OBJECTDETECTION_ENOMEM__CORE_LOCAL_RAM_AOA_DET_OBJ_ELEVATION_ANGLE      (DP_ERRNO_OBJECTDETECTION_BASE-16)

/**
 * @brief   Error Code: Out of Core Local RAM for AOA Scratch buffer
 */
#define DPC_OBJECTDETECTION_ENOMEM__CORE_LOCAL_RAM_AOA_SCRATCH_BUFFER               (DP_ERRNO_OBJECTDETECTION_BASE-17)

/**
 * @brief   Error Code: Out of memory for HWA save location for range DPU
 */
#define DPC_OBJECTDETECTION_ENOMEM__RANGE_HWA_PARAM_SAVE_LOC          (DP_ERRNO_OBJECTDETECTION_BASE-18)

/**
 * @brief   Error Code: Out of HWA Window RAM
 */
#define DPC_OBJECTDETECTION_ENOMEM__HWA_WINDOW_RAM_INTERNAL           (DP_ERRNO_OBJDETRANGEHWA_BASE - 19)

/**
 * @brief   Error Code: Out of memory for HWA save location for doa3d DPU
 */
#define DPC_OBJECTDETECTION_ENOMEM__DOA3D_HWA_PARAM_SAVE_LOC          (DP_ERRNO_OBJECTDETECTION_BASE - 20)

/**
 * @brief   Error Code: Out of memory for for snr3d DPU
 */
#define DPC_OBJECTDETECTION_ENOMEM__L3_RAM_SNR3D_MATRIX               (DP_ERRNO_OBJECTDETECTION_BASE - 21)

/**
 * @brief   Error Code: Out of memory for HWA save location for snr3d DPU
 */
#define DPC_OBJECTDETECTION_ENOMEM__SNR3D_HWA_PARAM_SAVE_LOC          (DP_ERRNO_OBJECTDETECTION_BASE - 22)

/**
 * @brief   Error Code: Radar cube format not supported
 */
#define DPC_OBJECTDETECTION_ERADAR_CUBE_FORMAT_NOT_SUPPORTED          (DP_ERRNO_OBJECTDETECTION_BASE - 23)

/**
 * @brief Range Bias and rx channel gain/phase compensation configuration.
 *
 *
 */
typedef struct DPC_ObjDet_compRxChannelBiasFloatCfg_t
{

    /*! @brief  Compensation for range estimation bias in meters */
    float rangeBias;

    /*! @brief  Compensation for Rx channel phase bias in Q20 format.
     *          The order here is like x[tx][rx] where rx order is 0,1,....SYS_COMMON_NUM_RX_CHANNEL-1
     *          and tx order is 0, 1,...,SYS_COMMON_NUM_TX_ANTENNAS-1 */
    float rxChPhaseComp[2 * SYS_COMMON_NUM_TX_ANTENNAS * SYS_COMMON_NUM_RX_CHANNEL];

} DPC_ObjDet_compRxChannelBiasFloatCfg;

typedef struct DPC_memUsage_t
{
    /*! @brief   Indicates number of bytes of L3 memory allocated to be used by DPC */
    uint32_t L3RamTotal;

    /*! @brief   Indicates number of bytes of L3 memory used by DPC from the allocated
     *           amount indicated through @ref DPC_ObjectDetection_InitParams */
    uint32_t L3RamUsage;

    /*! @brief   Indicates number of bytes of Core Local memory allocated to be used by DPC */
    uint32_t CoreLocalRamTotal;

    /*! @brief   Indicates number of bytes of Core Local memory used by DPC from the allocated
     *           amount indicated through @ref DPC_ObjectDetection_InitParams */
    uint32_t CoreLocalRamUsage;

    /*! @brief   Indicates number of bytes of system heap allocated */
    uint32_t SystemHeapTotal;

    /*! @brief   Indicates number of bytes of system heap used at the end of PreStartCfg */
    uint32_t SystemHeapUsed;

    /*! @brief   Indicates number of bytes of system heap used by DCP at the end of PreStartCfg */
    uint32_t SystemHeapDPCUsed;

} DPC_memUsage;

/*
 * @brief Memory Configuration used during init API
 */
typedef struct DPC_ObjectDetection_MemCfg_t
{
    /*! @brief   Start address of memory provided by the application
     *           from which DPC will allocate.
     */
    void *addr;

    /*! @brief   Size limit of memory allowed to be consumed by the DPC */
    uint32_t size;
} DPC_ObjectDetection_MemCfg;

/*
 * @brief Memory pool object to manage memory based on @ref DPC_ObjectDetection_MemCfg_t.
 */
typedef struct MemPoolObj_t
{
    /*! @brief Memory configuration */
    DPC_ObjectDetection_MemCfg cfg;

    /*! @brief   Pool running adress.*/
    uintptr_t currAddr;

    /*! @brief   Pool max address. This pool allows setting address to desired
     *           (e.g for rewinding purposes), so having a running maximum
     *           helps in finding max pool usage
     */
    uintptr_t maxCurrAddr;
} MemPoolObj;


/* DSS States */
typedef enum {
    STATE_IDLE,
    STATE_READY,
    NUM_STATES
} MmwDemo_dss_State;


/**
 * @brief
 *  Millimeter Wave Demo MCB
 *
 * @details
 *  The structure is used to hold all the relevant information for the
 *  Millimeter Wave demo.
 */
typedef struct MmwDemo_DSS_MCB_t
{

    /*! @brief   Semaphore Object  */
    SemaphoreP_Object demoInitTaskCompleteSemHandle;

    
    /* @brief static clutter removal flag */
    bool staticClutterRemovalEnable;

    /*! @brief Flag to control in low power mode some configuration parts to be executed only once, and not to be repeated from frame to frame */
    uint8_t         oneTimeConfigDone;

    /*! @brief L3 ram memory pool object */
    MemPoolObj      L3RamObj;

    /*! @brief Core Local ram memory pool object */
    MemPoolObj      CoreLocalRamObj;

    /*! @brief Core Local ram memory pool object */
    MemPoolObj      CoreLocalRam3Obj;

    /*! @brief Memory pool object using RTOS API */
    HeapP_Object CoreLocalRtosHeapObj;


    /*! @brief      Radar cube data */
    DPIF_RadarCube  radarCube;


    /*! @brief      Pointers to DPC output data */
    DPC_DSS_ObjectDetection_ExecuteResult dpcResult;



    /*! @brief  DPC stats structure */
    //DPC_DSS_ObjectDetection_Stats stats;

    /*! @brief      DPC reported output stats structure */
    //MmwDemo_output_message_stats outStats;

    /*! @brief Token is checked in the frame start ISR, asserted to have zero value, and incremented. At the end of DSP task, it is decremented */
    uint32_t interSubFrameProcToken;
    /*! @brief Counts frames not completed in time */
    uint32_t interSubFrameProcOverflowCntr;
    /*! @brief Configuration message counter */
    uint32_t dssConfigurationCntr;
    /*! @brief Counts received radar cube ready message */
    uint32_t radarCubeReadyEventCntr;
    /*! @brief frame counter modulo N where N is number of frames  per sliding window */
    uint32_t frmCntrModNumFrmPerSlidWin;
    /*! @brief number of frames  per sliding window */
    uint32_t numFrmPerSlidingWindow;


    MsgIpc_CtrlObj msgIpcCtrlObj;

    /* Queue handle receives messages from R5F via IPC */
    QueueHandle_t eventQueue;

    /* Current state of the FSM */
    MmwDemo_dss_State currentState;


    /*! @brief   Output results */
    DPIF_MSS_DSS_radarProcessOutput *outputFromDSP;

    ObjDetObj objDetObj;

    /*! For debugging only, normally set to zero */
    uint8_t disablePointCloudGeneration;

} MmwDemo_DSS_MCB;


/* Debug Functions */
extern void _MmwDemo_debugAssert(int32_t expression, const char *file, int32_t line);
#define MmwDemo_debugAssert(expression) {                                      \
                                         _MmwDemo_debugAssert(expression,      \
                                                  __FILE__, __LINE__);         \
                                         DebugP_assert(expression);             \
                                        }


#ifdef __cplusplus
}
#endif

#endif /* MMWAVE_DEMO_H */
