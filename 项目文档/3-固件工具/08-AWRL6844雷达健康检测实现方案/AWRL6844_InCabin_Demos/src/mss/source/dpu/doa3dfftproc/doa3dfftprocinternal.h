/**
 *   @file  doa3dfftproc.h
 *
 *   @brief
 *      Implements doa3dfft processing functionality.
 *
 *  \par
 *  NOTE:
 *      (C) Copyright 2024 Texas Instruments, Inc.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**************************************************************************
 *************************** Include Files ********************************
 **************************************************************************/
#ifndef DOA3D_PROC_INTERNAL_H
#define DOA3D_PROC_INTERNAL_H

/* Standard Include Files. */
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* mmWave SDK Driver/Common Include Files */
#include <drivers/hwa.h>
//#include <ti/utils/cycleprofiler/cycle_profiler.h>

/* DPIF Components Include Files */
#include <datapath/dpif/dpif_detmatrix.h>
#include <datapath/dpif/dpif_radarcube.h>

/* mmWave SDK Data Path Include Files */
#include <datapath/dpif/dp_error.h>
#include <source/dpu/doa3dfftproc/doa3dfftproc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief
 *  dopplerProc DPU internal data Object
 *
 * @details
 *  The structure is used to hold dopplerProc internal data object
 *
 *  \ingroup DPU_DOPPLERPROC_INTERNAL_DATA_STRUCTURE
 */
typedef struct DPU_Doa3dProc_hwaLoopCfg_t
{
    /*! @brief  HWA number of loops */
    uint16_t hwaNumLoops;

    /*! @brief  HWA start paramset index */
    uint8_t  hwaParamStartIdx;

    /*! @brief  HWA stop paramset index */
    uint8_t  hwaParamStopIdx;
} DPU_Doa3dProc_hwaLoopCfg;

/**
 * @brief
 *  dopplerProc DPU internal data Object
 *
 * @details
 *  The structure is used to hold dopplerProc internal data object
 *
 *  \ingroup DPU_DOPPLERPROC_INTERNAL_DATA_STRUCTURE
 */
typedef struct DPU_Doa3dProc_Obj_t
{
    /*! @brief HWA Handle */
    HWA_Handle  hwaHandle;
    
    /*! @brief  EDMA driver handle. */
    EDMA_Handle edmaHandle;
    uint32_t    edmaInstanceId;

    /*! @brief  EDMA configuration for Input data (Radar cube -> HWA memory). */
    DPU_Doa3dProc_Edma edmaIn;

    /*! @brief  EDMA configuration for data output from HWA - Detection matrix */
    DPEDMA_ChanCfg edmaDetMatOut;

    /*! @brief  EDMA configuration for data in */
    DPEDMA_ChanCfg edmaInterLoopIn;

    /*! @brief   Dummy location */
    uint32_t dummySrc;
    /*! @brief   Dummy location  */
    uint32_t dummyDst;

    /*! @brief HWA Processing Done semaphore Handle */
    SemaphoreP_Object  hwaDoneSemaHandle;

    /*! @brief EDMA Done semaphore Handle */
    SemaphoreP_Object  edmaDoneSemaHandle;
    
    /*! @brief Flag to indicate if DPU is in processing state */
    bool inProgress;

    /*! @brief  DMA trigger source channel for Ping param set */
    uint8_t hwaDmaTriggerSourceChan;

    /*! @brief  DMA trigger source channel for Ping param set */
    uint8_t hwaDmaTriggerSourcePing;
    
    /*! @brief  DMA trigger source channel for Pong param set */
    uint8_t hwaDmaTriggerSourcePong;

            
    /*! @brief  HWA number of loops */
    uint16_t hwaNumLoops;
    
    /*! @brief  HWA start paramset index */
    uint8_t  hwaParamStartIdx;
    
    /*! @brief  HWA stop paramset index */
    uint8_t  hwaParamStopIdx;
    
    /*! @brief  External range loop - HWA common config for the first processing part (doppler FFT and Azimuth FFT) */
    DPU_Doa3dProc_hwaLoopCfg hwaDopplerLoop;

    /*! @brief  External range loop - HWA common config for the second processing part (elevation FFT) */
    DPU_Doa3dProc_hwaLoopCfg hwaElevationLoop;

    /*! @brief  HWA memory bank addresses */
    uint32_t hwaMemBankAddr[DPU_DOA3DPROC_NUM_HWA_MEMBANKS];

    /*! @brief  Summation division shift for Doppler FFT non-coherent integration */
    uint8_t dopFftSumDiv;

    /*! @brief HWA Params save location
    */
    DPU_Doa3dProc_HwaParamSaveLoc hwaParamsSaveLoc;

    /*! @brief     EDMA output interrupt object */
    Edma_IntrObject intrObj;

    /*! @brief     Load HWA params sets before execution */
    bool loadHwaParamSetsBeforeExec;

}DPU_Doa3dProc_Obj;


#ifdef __cplusplus
}
#endif

#endif
