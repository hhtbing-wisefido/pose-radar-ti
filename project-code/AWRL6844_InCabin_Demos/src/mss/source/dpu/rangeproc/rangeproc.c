/*
 *  NOTE:
 *      (C) Copyright 2018 Texas Instruments, Inc.
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
/**
 *   @file  rangeprochwa.c
 *
 *   @brief
 *      Implements Range FFT data processing Unit using HWA.
 */

/**************************************************************************
 *************************** Include Files ********************************
 **************************************************************************/

/* Standard Include Files. */
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* MCU+SDK Include files */
#include <drivers/hw_include/hw_types.h>
#include <kernel/dpl/SemaphoreP.h>
#include <kernel/dpl/CacheP.h>
#include <kernel/dpl/HeapP.h>
#include <drivers/edma.h>
#include <drivers/soc.h>
#ifdef SUBSYS_MSS
#include <kernel/dpl/CacheP.h>
#endif

#include <source/hwa_adapt/hwa_adapt.h>

/* Data Path Include files */
#include <source/dpu/rangeproc/rangeproc.h>

/* MATH utils library Include files */
#include <utils/mathutils/mathutils.h>

/* Internal include Files */
#include <source/dpu/rangeproc/rangeproc_internal.h>

/* HWA functions expected to be part of HWA driver */
#include <source/hwa_adapt/hwa_adapt.h>

/* Flag to check input parameters */
#define DEBUG_CHECK_PARAMS 1
#define DEBUG_CHECK_EDMA     0

/**************************************************************************
 ************************ Global Variables       **********************
 **************************************************************************/
/* User defined heap memory and handle */
#define RANGEPROCHWA_HEAP_MEM_SIZE  (sizeof(rangeProcObj))

static uint8_t gRangeProcHeapMem[RANGEPROCHWA_HEAP_MEM_SIZE] __attribute__((aligned(HeapP_BYTE_ALIGNMENT)));

/**************************************************************************
 ************************ Internal Functions Prototype       **********************
 **************************************************************************/
static void rangeProcDoneIsrCallback(void *arg);
void rangeProc_EDMA_transferCompletionCallbackFxn(Edma_IntrHandle intrHandle, void *args);
int32_t rangeProc_updateEdmaOutDestAddress(rangeProcObj *obj);


static int32_t rangeProc_ConfigHWA(
    rangeProcObj *obj,
    uint8_t          destChanPing,
    uint8_t          destChanPong,
    uint16_t         hwaMemSrcPingOffset,
    uint16_t         hwaMemSrcPongOffset,
    uint16_t         hwaMemDestPingOffset,
    uint16_t         hwaMemDestPongOffset);

static int32_t rangeProc_TriggerHWA(
    rangeProcObj *obj);
static int32_t rangeProc_ConfigEDMA_DataOut(
    rangeProcObj               *obj,
    rangeProc_dpParams            *DPParams,
    DPU_RangeProc_HW_Resources *pHwConfig,
    uint32_t                       hwaOutPingOffset,
    uint32_t                       hwaOutPongOffset);
static int32_t rangeProc_ConfigEDMA_DataIn(
    rangeProcObj               *obj,
    rangeProc_dpParams            *DPParams,
    DPU_RangeProc_HW_Resources *pHwConfig);
static int32_t rangeProc_ConifgNonInterleaveMode(
    rangeProcObj               *obj,
    rangeProc_dpParams            *DPParams,
    DPU_RangeProc_HW_Resources *pHwConfig);

/**************************************************************************
 ************************RangeProc Internal Functions    ******************
 **************************************************************************/

/**
 *  @b Description
 *  @n
 *      HWA processing completion call back function as per HWA API.
 *      Depending on the programmed transfer completion codes,
 *      posts HWA done semaphore.
 *
 *  @param[in]  arg                 Argument to the callback function
 *
 *  \ingroup    DPU_RANGEPROC_INTERNAL_FUNCTION
 *
 *  @retval     N/A
 */
static void rangeProcDoneIsrCallback(void *arg)
{
    if (arg != NULL)
    {
        SemaphoreP_post((SemaphoreP_Object*)arg);
    }
}


/**
 *  @b Description
 *  @n
 *      EDMA processing completion call back function as per EDMA API.
 *
 *  @param[in]  arg                     Argument to the callback function
 *  @param[in]  transferCompletionCode  EDMA transfer complete code
 *
 *  \ingroup    DPU_RANGEPROC_INTERNAL_FUNCTION
 *
 *  @retval     N/A
 */
volatile uint32_t gRangeDpuEdmaCallbackcnt = 0;
void rangeProc_EDMA_transferCompletionCallbackFxn(Edma_IntrHandle intrHandle,
   void *args)
{
    rangeProcObj     *obj;

    /* Get rangeProc object */
    obj = (rangeProcObj *) args;

    gRangeDpuEdmaCallbackcnt++;
    if (intrHandle->tccNum == obj->dataOutSignatureChan)
    {
        obj->numEdmaDataOutCnt++;
        SemaphoreP_post(&obj->edmaDoneSemaHandle);
    }
}



/**
 *  @b Description
 *  @n
 *      Internal function to config HWA to perform range FFT
 *
 *  @param[in]  rangeProcObj                  Pointer to rangeProc object
 *
 *  \ingroup    DPU_RANGEPROC_INTERNAL_FUNCTION
 *
 *  @retval
 *      Success     - 0
 *  @retval
 *      Error       - <0
 */
static int32_t rangeProc_ConfigHWACommon(
    rangeProcObj *obj)
{
    HWA_CommonConfig    hwaCommonConfig;
    rangeProc_dpParams *DPParams;
    int32_t             retVal;

    DPParams = &obj->params;

    /***********************/
    /* HWA COMMON CONFIG   */
    /***********************/
    /* Config Common Registers */
    hwaCommonConfig.configMask = HWA_COMMONCONFIG_MASK_NUMLOOPS |
        HWA_COMMONCONFIG_MASK_PARAMSTARTIDX |
        HWA_COMMONCONFIG_MASK_PARAMSTOPIDX |
        HWA_COMMONCONFIG_MASK_FFT1DENABLE |
        HWA_COMMONCONFIG_MASK_INTERFERENCETHRESHOLD |
        HWA_COMMONCONFIG_MASK_TWIDDITHERENABLE |
        HWA_COMMONCONFIG_MASK_LFSRSEED;

    hwaCommonConfig.fftConfig.twidDitherEnable = HWA_FEATURE_BIT_ENABLE;
    hwaCommonConfig.fftConfig.lfsrSeed         = 0x1234567; /*Some non-zero value*/
    hwaCommonConfig.numLoops = DPParams->numChirpsPerOneFrame / 2U; /* ping/pong approach */


    hwaCommonConfig.paramStartIdx = obj->hwaCfg.paramSetStartIdx;
    hwaCommonConfig.paramStopIdx  = obj->hwaCfg.paramSetStartIdx + obj->hwaCfg.numParamSet - 1U;

    if (obj->hwaCfg.dataInputMode == DPU_RangeProc_InputMode_ISOLATED)
    {
        /* HWA will input data from M0 memory*/
        hwaCommonConfig.fftConfig.fft1DEnable = HWA_FEATURE_BIT_DISABLE;
    }
    else
    {
        /* HWA will input data from ADC buffer memory*/
        hwaCommonConfig.fftConfig.fft1DEnable = HWA_FEATURE_BIT_ENABLE;
    }
    hwaCommonConfig.fftConfig.interferenceThreshold = 0xFFFFFF;
    retVal                                          = HWA_configCommon(obj->initParms.hwaHandle, &hwaCommonConfig);
    if (retVal != 0)
    {
        goto exit;
    }

    /**********************************************/
    /* ENABLE NUMLOOPS DONE INTERRUPT FROM HWA */
    /**********************************************/
    retVal = HWA_enableDoneInterrupt(obj->initParms.hwaHandle,
                                     rangeProcDoneIsrCallback,
                                     (void*)&obj->hwaDoneSemaHandle);
    if (retVal != 0)
    {
        goto exit;
    }

exit:
    return (retVal);
}
/**
 *  @b Description
 *  @n
 *      Internal function to config HWA to perform range FFT
 *
 *  @param[in]  obj                  Pointer to rangeProc object
 *  @param[in]  destChanPing                  Destination channel id for PING
 *  @param[in]  destChanPong                  Destination channel id for PONG
 *  @param[in]  hwaMemSrcPingOffset           Source Address offset for Ping input
 *  @param[in]  hwaMemSrcPongOffset           Source Address offset for Pong input
 *  @param[in]  hwaMemDestPingOffset          Destination address offset for Ping output
 *  @param[in]  hwaMemDestPongOffset          Destination address offset for Pong output
 *
 *  \ingroup    DPU_RANGEPROC_INTERNAL_FUNCTION
 *
 *  @retval
 *      Success     - 0
 *  @retval
 *      Error       - <0
 */
static int32_t rangeProc_ConfigHWA(
    rangeProcObj *obj,
    uint8_t          destChanPing,
    uint8_t          destChanPong,
    uint16_t         hwaMemSrcPingOffset,
    uint16_t         hwaMemSrcPongOffset,
    uint16_t         hwaMemDestPingOffset,
    uint16_t         hwaMemDestPongOffset)
{
    HWA_InterruptConfig paramISRConfig;
    int32_t             errCode     = 0;
    uint32_t            paramsetIdx = 0;
    uint32_t            hwParamsetIdx;
    uint32_t            pingParamSetIdx = 0;
    HWA_ParamConfig     hwaParamCfg[DPU_RANGEPROC_NUM_HWA_PARAM_SETS];
    HWA_Handle          hwaHandle;
    rangeProc_dpParams *pDPParams;
    uint8_t             index;
    uint32_t            elementSizeInBytes;
    uint8_t             hwaSamplesFormat;

    hwaHandle = obj->initParms.hwaHandle;
    pDPParams = &obj->params;

    memset(hwaParamCfg, 0, sizeof(hwaParamCfg));

    hwParamsetIdx = obj->hwaCfg.paramSetStartIdx;
    for (index = 0; index < DPU_RANGEPROC_NUM_HWA_PARAM_SETS; index++)
    {
        errCode = HWA_disableParamSetInterrupt(hwaHandle, index + obj->hwaCfg.paramSetStartIdx, HWA_PARAMDONE_INTERRUPT_TYPE_CPU | HWA_PARAMDONE_INTERRUPT_TYPE_DMA);
        if (errCode != 0)
        {
            goto exit;
        }
    }

    /***********************/
    /* PING DUMMY PARAMSET */
    /***********************/
    hwaParamCfg[paramsetIdx].triggerMode   = HWA_TRIG_MODE_DMA;
    hwaParamCfg[paramsetIdx].dmaTriggerSrc = obj->dataOutTrigger[0]; //hwParamsetIdx;
    hwaParamCfg[paramsetIdx].accelMode     = HWA_ACCELMODE_NONE;
    errCode                                = HWA_configParamSet(hwaHandle,
                                 hwParamsetIdx,
                                 &hwaParamCfg[paramsetIdx],
                                 NULL);
    if (errCode != 0)
    {
        goto exit;
    }

    if (pDPParams->dataFmt == DPIF_DATAFORMAT_REAL16)
    {
        elementSizeInBytes = sizeof(int16_t);
        hwaSamplesFormat = HWA_SAMPLES_FORMAT_REAL;
    }
    else
    {
        elementSizeInBytes = sizeof(cmplx16ImRe_t);
        hwaSamplesFormat = HWA_SAMPLES_FORMAT_COMPLEX;
    }
    /***********************/
    /* PING PROCESS PARAMSET */
    /***********************/
    paramsetIdx++;
    hwParamsetIdx++;
    pingParamSetIdx = paramsetIdx;

    hwaParamCfg[paramsetIdx].triggerMode   = HWA_TRIG_MODE_DMA;
    hwaParamCfg[paramsetIdx].dmaTriggerSrc = obj->dataInTrigger[0]; //hwParamsetIdx;

    hwaParamCfg[paramsetIdx].accelMode      = HWA_ACCELMODE_FFT;
    hwaParamCfg[paramsetIdx].source.srcAddr = hwaMemSrcPingOffset;

    hwaParamCfg[paramsetIdx].source.srcShift         = 0;
    hwaParamCfg[paramsetIdx].source.srcCircShiftWrap = 0;
    hwaParamCfg[paramsetIdx].source.srcRealComplex   = hwaSamplesFormat;
    hwaParamCfg[paramsetIdx].source.srcWidth         = HWA_SAMPLES_WIDTH_16BIT;
    hwaParamCfg[paramsetIdx].source.srcSign          = HWA_SAMPLES_SIGNED;
    hwaParamCfg[paramsetIdx].source.srcConjugate     = 0;
    hwaParamCfg[paramsetIdx].source.srcScale         = 8;
    hwaParamCfg[paramsetIdx].source.bpmEnable        = 0;
    hwaParamCfg[paramsetIdx].source.bpmPhase         = 0;
    hwaParamCfg[paramsetIdx].dest.dstAddr            = hwaMemDestPingOffset;

    hwaParamCfg[paramsetIdx].dest.dstRealComplex = HWA_SAMPLES_FORMAT_COMPLEX;
    hwaParamCfg[paramsetIdx].dest.dstWidth       = HWA_SAMPLES_WIDTH_16BIT;
    hwaParamCfg[paramsetIdx].dest.dstSign        = HWA_SAMPLES_SIGNED;
    hwaParamCfg[paramsetIdx].dest.dstConjugate   = 0;
    hwaParamCfg[paramsetIdx].dest.dstScale       = 2; // 0;
    hwaParamCfg[paramsetIdx].dest.dstSkipInit    = 0;

    hwaParamCfg[paramsetIdx].accelModeArgs.fftMode.fftEn              = 1;
    hwaParamCfg[paramsetIdx].accelModeArgs.fftMode.fftSize            = mathUtils_ceilLog2(pDPParams->rangeFftSize);
    hwaParamCfg[paramsetIdx].accelModeArgs.fftMode.butterflyScaling   = pDPParams->butterflyScalingBitMask;
    hwaParamCfg[paramsetIdx].accelModeArgs.fftMode.interfZeroOutEn    = 0;
    hwaParamCfg[paramsetIdx].accelModeArgs.fftMode.windowEn           = 1;
    hwaParamCfg[paramsetIdx].accelModeArgs.fftMode.windowStart        = obj->hwaCfg.hwaWinRamOffset;
    hwaParamCfg[paramsetIdx].accelModeArgs.fftMode.winSymm            = obj->hwaCfg.hwaWinSym;
    hwaParamCfg[paramsetIdx].accelModeArgs.fftMode.winInterpolateMode = 0;
    hwaParamCfg[paramsetIdx].accelModeArgs.fftMode.magLogEn           = HWA_FFT_MODE_MAGNITUDE_LOG2_DISABLED;
    hwaParamCfg[paramsetIdx].accelModeArgs.fftMode.fftOutMode         = HWA_FFT_MODE_OUTPUT_DEFAULT;
    hwaParamCfg[paramsetIdx].complexMultiply.mode                     = HWA_COMPLEX_MULTIPLY_MODE_DISABLE;

    /* HWA range FFT src/dst configuration*/

    hwaParamCfg[paramsetIdx].source.srcAcnt = pDPParams->numAdcSamples - 1;
    hwaParamCfg[paramsetIdx].source.srcAIdx = elementSizeInBytes;
    hwaParamCfg[paramsetIdx].source.srcBcnt = pDPParams->numRxAntennas - 1;
    hwaParamCfg[paramsetIdx].source.srcBIdx = obj->rxChanOffset;
    hwaParamCfg[paramsetIdx].dest.dstAcnt = pDPParams->numRangeBins - 1;
    if (obj->radarCubeLayout == rangeProc_dataLayout_RANGE_CHIRP_TxAnt_RxAnt)
    {
        hwaParamCfg[paramsetIdx].dest.dstAIdx = sizeof(cmplx16ImRe_t) * pDPParams->numRxAntennas;
        hwaParamCfg[paramsetIdx].dest.dstBIdx = sizeof(cmplx16ImRe_t);
    }
    else if (obj->radarCubeLayout == rangeProc_dataLayout_CHIRP_TxAnt_RxAnt_RANGE)
    {
        hwaParamCfg[paramsetIdx].dest.dstAIdx   = sizeof(cmplx16ImRe_t);
        hwaParamCfg[paramsetIdx].dest.dstBIdx   = pDPParams->numRangeBins * sizeof(cmplx16ImRe_t);
    }
    else
    {
        errCode = DPU_RANGEPROC_EINVAL;
        goto exit;
    }

    errCode = HWA_configParamSet(hwaHandle,
                                 hwParamsetIdx,
                                 &hwaParamCfg[paramsetIdx],
                                 NULL);
    if (errCode != 0)
    {
        goto exit;
    }

    /* enable the DMA hookup to this paramset so that data gets copied out */
    paramISRConfig.interruptTypeFlag = HWA_PARAMDONE_INTERRUPT_TYPE_DMA;
    paramISRConfig.dma.dstChannel    = destChanPing;

    errCode = HWA_enableParamSetInterrupt(hwaHandle, hwParamsetIdx, &paramISRConfig);
    if (errCode != 0)
    {
        goto exit;
    }

    /***********************/
    /* PONG DUMMY PARAMSET */
    /***********************/
    paramsetIdx++;
    hwParamsetIdx++;

    hwaParamCfg[paramsetIdx].triggerMode   = HWA_TRIG_MODE_DMA;
    hwaParamCfg[paramsetIdx].dmaTriggerSrc = obj->dataOutTrigger[1]; // hwParamsetIdx;
    hwaParamCfg[paramsetIdx].accelMode     = HWA_ACCELMODE_NONE;
    errCode                                = HWA_configParamSet(hwaHandle,
                                 hwParamsetIdx,
                                 &hwaParamCfg[paramsetIdx],
                                 NULL);
    if (errCode != 0)
    {
        goto exit;
    }

    /***********************/
    /* PONG PROCESS PARAMSET */
    /***********************/
    paramsetIdx++;
    hwParamsetIdx++;
    hwaParamCfg[paramsetIdx]                = hwaParamCfg[pingParamSetIdx];
    hwaParamCfg[paramsetIdx].source.srcAddr = hwaMemSrcPongOffset;
    hwaParamCfg[paramsetIdx].dest.dstAddr   = hwaMemDestPongOffset;

    hwaParamCfg[paramsetIdx].dmaTriggerSrc = obj->dataInTrigger[1]; // hwParamsetIdx;

    errCode = HWA_configParamSet(hwaHandle,
                                 hwParamsetIdx,
                                 &hwaParamCfg[paramsetIdx],
                                 NULL);
    if (errCode != 0)
    {
        goto exit;
    }

    /* Enable the DMA hookup to this paramset so that data gets copied out */
    paramISRConfig.interruptTypeFlag = HWA_PARAMDONE_INTERRUPT_TYPE_DMA;
    paramISRConfig.dma.dstChannel    = destChanPong;
    errCode                          = HWA_enableParamSetInterrupt(hwaHandle,
                                          hwParamsetIdx,
                                          &paramISRConfig);
    if (errCode != 0)
    {
        goto exit;
    }

    paramsetIdx++;
    obj->hwaCfg.numParamSet = paramsetIdx;

    if (obj->params.loadHwaParamSetsBeforeExec)
    {
        errCode = HWA_saveParams(hwaHandle,
                                 obj->hwaCfg.paramSetStartIdx,
                                 obj->hwaCfg.numParamSet,
                                 obj->hwaCfg.hwaParamsSaveLoc.data);
        if (errCode != 0)
        {
            goto exit;
        }
    }
exit:
    return (errCode);
}

/**
 *  @b Description
 *  @n
 *      Trigger HWA for range processing.
 *
 *  @param[in]  obj              Pointer to rangeProc object
 *
 *  \ingroup    DPU_RANGEPROC_INTERNAL_FUNCTION
 *
 *  @retval
 *      Success     - 0
 *  @retval
 *      Error       - <0
 */
static int32_t rangeProc_TriggerHWA(
    rangeProcObj *obj)
{
    int32_t    retVal = 0;
    HWA_Handle hwaHandle;

    /* Get HWA driver handle */
    hwaHandle = obj->initParms.hwaHandle;

    if (obj->params.loadHwaParamSetsBeforeExec)
    {
        /* Load HWA Params */
        retVal = HWA_loadParams(hwaHandle,
                                obj->hwaCfg.paramSetStartIdx,
                                obj->hwaCfg.numParamSet,
                                obj->hwaCfg.hwaParamsSaveLoc.data);
        if (retVal != 0)
        {
            goto exit;
        }
    }
    if (obj->params.numFramesPerSlidingWindow > 1)
    {
        /* Update EDMAout destination address - only if the radar cube is spread across several frames */
        retVal = rangeProc_updateEdmaOutDestAddress(obj);
        if (retVal < 0)
        {
            goto exit;
        }
    }

    /* Configure HWA common parameters */
    retVal = rangeProc_ConfigHWACommon(obj);
    if (retVal < 0)
    {
        goto exit;
    }

    /* Enable the HWA */
    retVal = HWA_enable(hwaHandle, 1);
    if (retVal != 0)
    {
        goto exit;
    }

    /* Trigger the HWA paramset for Ping */
    retVal = HWA_setDMA2ACCManualTrig(hwaHandle, obj->dataOutTrigger[0]);
    if (retVal != 0)
    {
        goto exit;
    }

    /* Trigger the HWA paramset for Pong */
    retVal = HWA_setDMA2ACCManualTrig(hwaHandle, obj->dataOutTrigger[1]);
    if (retVal != 0)
    {
        goto exit;
    }

exit:
    return (retVal);
}

/**
 *  @b Description
 *  @n
 *     Updates the EDMAout destination addresses of ping and pong path. It is caled per frame. 
 *     The address is pointing to the radar cube and is circularly incremented each frame, during the 
 *     range DPU setup for the next frame. 
 *
 *  @param[in]  rangeProcObj              Pointer to rangeProc object
 *
 *  \ingroup    DPU_RANGEPROC_INTERNAL_FUNCTION
 *
 *  @retval
 *      Success     - 0
 *  @retval
 *      Error       - <0
 */
int32_t rangeProc_updateEdmaOutDestAddress(rangeProcObj *obj)
{
    int32_t errorCode = SystemP_SUCCESS;
    EDMA_Handle        handle = obj->edmaHandle;
    rangeProc_dpParams *DPParams = &obj->params;
    uint32_t  destAddress;


    /* EDMA output destination address for Ping path */
    destAddress = ((uint32_t)obj->radarCubebuf) + obj->frmCntrInSlidingWindow * obj->radCubeFrameOffsetInBytes;
    errorCode = DPEDMA_setDestinationAddress(handle,
                                             obj->edmaDataOutChan[0],
                                             destAddress);
    if(errorCode != SystemP_SUCCESS)
    {
        goto exit;
    }


    /* EDMA output destination address for Pong path */
    if (obj->radarCubeLayout == rangeProc_dataLayout_RANGE_CHIRP_TxAnt_RxAnt)
    {
        destAddress = ((uint32_t)(obj->radarCubebuf)) + DPParams->numRxAntennas * sizeof(cmplx16ImRe_t);
    }
    else if (obj->radarCubeLayout == rangeProc_dataLayout_CHIRP_TxAnt_RxAnt_RANGE)
    {
        destAddress = ((uint32_t)(obj->radarCubebuf)) + DPParams->numRangeBins * DPParams->numRxAntennas * sizeof(cmplx16ImRe_t);
    }
    destAddress += obj->frmCntrInSlidingWindow * obj->radCubeFrameOffsetInBytes;
    errorCode = DPEDMA_setDestinationAddress(handle,
                                        obj->edmaDataOutChan[1],
                                        destAddress);
    if(errorCode != SystemP_SUCCESS)
    {
        goto exit;
    }



    /* Increment frame counter modulo numFrmPerSilidingWindow */
    obj->frmCntrInSlidingWindow++;
    if (obj->frmCntrInSlidingWindow >= obj->params.numFramesPerSlidingWindow)
    {
        obj->frmCntrInSlidingWindow = 0;
    }

exit:
    return(errorCode);
}
/**
 *  @b Description
 *  @n
 *      EDMA configuration for rangeProc data output in non-interleave mode
 *
 *  @param[in]  obj              Pointer to rangeProc object
 *  @param[in]  DPParams                  Pointer to datapath parameter
 *  @param[in]  pHwConfig                 Pointer to rangeProc hardware resources
 *  @param[in]  hwaOutPingOffset          Ping HWA memory address offset
 *  @param[in]  hwaOutPongOffset          Pong HWA memory address offset
 *
 *  \ingroup    DPU_RANGEPROC_INTERNAL_FUNCTION
 *
 *  @retval
 *      Success     - 0
 *  @retval
 *      Error       - <0
 */
static int32_t rangeProc_ConfigEDMA_DataOut(
    rangeProcObj               *obj,
    rangeProc_dpParams            *DPParams,
    DPU_RangeProc_HW_Resources *pHwConfig,
    uint32_t                       hwaOutPingOffset,
    uint32_t                       hwaOutPongOffset)
{
    int32_t            errorCode = SystemP_SUCCESS;
    EDMA_Handle        handle;
    DPEDMA_syncABCfg   syncABCfg;
    DPEDMA_ChainingCfg chainingCfg;

    /* Get rangeProc Configuration */
    handle = obj->edmaHandle;

    /* Chaining configuration for all cases -> chaining to the data out signature channel */
    chainingCfg.chainingChan                  = pHwConfig->edmaOutCfg.dataOutSignature.channel;
    chainingCfg.isIntermediateChainingEnabled = true;
    chainingCfg.isFinalChainingEnabled        = true;

    /**************************************************************************
     *  Configure EDMA to copy from HWA memory to radar cube
     *************************************************************************/
    if (obj->radarCubeLayout == rangeProc_dataLayout_RANGE_CHIRP_TxAnt_RxAnt)
    {
        /* Ping/Pong common configuration */
        syncABCfg.aCount  = DPParams->numRxAntennas * sizeof(cmplx16ImRe_t);
        syncABCfg.bCount  = DPParams->numRangeBins;
        syncABCfg.srcBIdx = DPParams->numRxAntennas * sizeof(cmplx16ImRe_t);
        syncABCfg.srcCIdx = 0U;
        syncABCfg.dstBIdx = DPParams->numRxAntennas * sizeof(cmplx16ImRe_t) * DPParams->numChirpsPerSlidingWindow;
        syncABCfg.dstCIdx = DPParams->numRxAntennas * sizeof(cmplx16ImRe_t) * 2U;
    }
    else if (obj->radarCubeLayout == rangeProc_dataLayout_CHIRP_TxAnt_RxAnt_RANGE)
    {
        /* Ping/Pong common configuration */
        syncABCfg.aCount  = DPParams->numRangeBins * sizeof(cmplx16ImRe_t);
        syncABCfg.bCount  = DPParams->numRxAntennas;
        syncABCfg.srcBIdx = DPParams->numRangeBins * sizeof(cmplx16ImRe_t);
        syncABCfg.srcCIdx = 0U;
        syncABCfg.dstBIdx = DPParams->numRangeBins * sizeof(cmplx16ImRe_t) ;
        syncABCfg.dstCIdx = DPParams->numRangeBins * DPParams->numRxAntennas * sizeof(cmplx16ImRe_t) * 2U;
    }
    else
    {
        errorCode = DPU_RANGEPROC_EINVAL;
        goto exit;
    }

    syncABCfg.cCount = DPParams->numChirpsPerOneFrame / 2U;

    /* Ping specific config */
    syncABCfg.srcAddress = hwaOutPingOffset;

    /* Note: EDMA Destination address will be updated during the common configuration before each frame */
    syncABCfg.destAddress = (uint32_t)obj->radarCubebuf;

    errorCode = DPEDMA_configSyncAB(handle,
                                    &pHwConfig->edmaOutCfg.u.fmt1.dataOutPing,
                                    &chainingCfg,
                                    &syncABCfg,
                                    true, /* isEventTriggered */
                                    false, /* isIntermediateTransferCompletionEnabled */
                                    false, /* isTransferCompletionEnabled */
                                    NULL,
                                    NULL,
                                    NULL);

    if (errorCode != SystemP_SUCCESS)
    {
        goto exit;
    }

    /* Pong specific configuration */
    syncABCfg.srcAddress = hwaOutPongOffset;


    /* Note: EDMA Destination address will be updated during the common configuration before each frame */
    if (obj->radarCubeLayout == rangeProc_dataLayout_RANGE_CHIRP_TxAnt_RxAnt)
    {
        syncABCfg.destAddress = ((uint32_t)(obj->radarCubebuf)) + DPParams->numRxAntennas * sizeof(cmplx16ImRe_t);
    }
    else if (obj->radarCubeLayout == rangeProc_dataLayout_CHIRP_TxAnt_RxAnt_RANGE)
    {
        syncABCfg.destAddress = ((uint32_t)(obj->radarCubebuf)) + DPParams->numRangeBins * DPParams->numRxAntennas * sizeof(cmplx16ImRe_t);
    }
    else
    {
        errorCode = DPU_RANGEPROC_EINVAL;
        goto exit;
    }

    errorCode = DPEDMA_configSyncAB(handle,
                                    &pHwConfig->edmaOutCfg.u.fmt1.dataOutPong,
                                    &chainingCfg,
                                    &syncABCfg,
                                    true, /* isEventTriggered */
                                    false, /* isIntermediateTransferCompletionEnabled */
                                    true, /* isTransferCompletionEnabled */
                                    rangeProc_EDMA_transferCompletionCallbackFxn,
                                    (void *)obj,
                                    &obj->intrObj);

    if (errorCode != SystemP_SUCCESS)
    {
        goto exit;
    }


    /**************************************************************************
     *  HWA hot signature EDMA, chained to the transpose EDMA channels
     *************************************************************************/
    errorCode = DPEDMAHWA_configTwoHotSignature(handle,
                                                &pHwConfig->edmaOutCfg.dataOutSignature,
                                                obj->initParms.hwaHandle,
                                                obj->dataOutTrigger[0],
                                                obj->dataOutTrigger[1],
                                                false);
    if (errorCode != SystemP_SUCCESS)
    {
        goto exit;
    }

exit:
    return (errorCode);
}

/**
 *  @b Description
 *  @n
 *      EDMA configuration for rangeProc data in when EDMA is used to copy data from
 *  ADCBuf to HWA memory
 *
 *  @param[in]  obj              Pointer to rangeProc object handle
 *  @param[in]  DPParams                  Pointer to datapath parameter
 *  @param[in]  pHwConfig                 Pointer to rangeProc hardware resources
 *
 *  \ingroup    DPU_RANGEPROC_INTERNAL_FUNCTION
 *
 *  @retval
 *      Success     - 0
 *  @retval
 *      Error       - <0
 */
static int32_t rangeProc_ConfigEDMA_DataIn(
    rangeProcObj               *obj,
    rangeProc_dpParams            *DPParams,
    DPU_RangeProc_HW_Resources *pHwConfig)
{
    int32_t            errorCode = SystemP_SUCCESS;
    EDMA_Handle        handle;
    uint16_t           bytePerRxChan;
    DPEDMA_ChainingCfg chainingCfg;
    uint8_t            isEventTriggered;

    /* Get rangeProc Configuration */
    handle = obj->edmaHandle;

    /* Configure ping (even chirp) and pong (odd chirp) path Params */
    if (DPParams->dataFmt == DPIF_DATAFORMAT_REAL16)
    {
        //real ADC samples
        bytePerRxChan = DPParams->numAdcSamples * sizeof(int16_t);
    }
    else
    {
        //complex ADC samples
        bytePerRxChan = DPParams->numAdcSamples * sizeof(cmplx16ImRe_t);
    }

    /**********************************************/
    /* ADCBuf -> Ping/Pong Buffer(M0 and M1)           */
    /**********************************************/
    chainingCfg.chainingChan                  = pHwConfig->edmaInCfg.dataInSignature.channel;
    chainingCfg.isFinalChainingEnabled        = true;
    chainingCfg.isIntermediateChainingEnabled = true;

    DPEDMA_syncABCfg syncABCfg;

    syncABCfg.srcAddress  = (uint32_t)obj->ADCdataBuf;
    syncABCfg.destAddress = obj->hwaMemBankAddr[0];

    syncABCfg.aCount = bytePerRxChan;
    syncABCfg.bCount = DPParams->numRxAntennas;
    syncABCfg.cCount = 2U; /* ping and pong */

    syncABCfg.srcBIdx = obj->rxChanOffset;
    syncABCfg.dstBIdx = obj->rxChanOffset;
    syncABCfg.srcCIdx = 0U;
    syncABCfg.dstCIdx = ((uint32_t)obj->hwaMemBankAddr[1] - (uint32_t)obj->hwaMemBankAddr[0]);

    if (DPParams->prolonedBurstingMode)
    {
        /* In Prolonged bursting mode, triggered through chaining from the EDMA event selector */
        isEventTriggered = false;
    }
    else
    {
        /* In Normal bursting mode, triggered by chirp available event */
        isEventTriggered = true;
    }
    errorCode = DPEDMA_configSyncAB(handle,
                                    &pHwConfig->edmaInCfg.dataIn,
                                    &chainingCfg,
                                    &syncABCfg,
                                    isEventTriggered,
                                    false, /* isIntermediateTransferInterruptEnabled */
                                    false, /*isFinalTransferInterruptEnabled */
                                    NULL,
                                    NULL,
                                    NULL);


    if (errorCode != SystemP_SUCCESS)
    {
        goto exit;
    }

    /*************************************************/
    /* Generate Hot Signature to trigger Ping/Pong paramset   */
    /*************************************************/

    errorCode = DPEDMAHWA_configTwoHotSignature(handle,
                                                &pHwConfig->edmaInCfg.dataInSignature,
                                                obj->initParms.hwaHandle,
                                                obj->dataInTrigger[0],
                                                obj->dataInTrigger[1],
                                                false);

    if (errorCode != SystemP_SUCCESS)
    {
        goto exit;
    }
exit:
    return (errorCode);
}



/**
 *  @b Description
 *  @n
 *      rangeProc configuraiton in non-interleaved mode
 *
 *  @param[in]  obj                 Pointer to rangeProc object
 *  @param[in]  DPParams                     Pointer to data path common params
 *  @param[in]  pHwConfig                    Pointer to rangeProc hardware resources
 *
 *  \ingroup    DPU_RANGEPROC_INTERNAL_FUNCTION
 *
 *  @retval
 *      Success     - 0
 *  @retval
 *      Error       - <0
 */
static int32_t rangeProc_ConifgNonInterleaveMode(
    rangeProcObj               *obj,
    rangeProc_dpParams            *DPParams,
    DPU_RangeProc_HW_Resources *pHwConfig)
{
    HWA_Handle hwaHandle;
    int32_t    retVal = 0;
    uint8_t    destChanPing;
    uint8_t    destChanPong;
    uint8_t    edmaChanPing;
    uint8_t    edmaChanPong;

    hwaHandle = obj->initParms.hwaHandle;


    edmaChanPing = pHwConfig->edmaOutCfg.u.fmt1.dataOutPing.channel;
    edmaChanPong = pHwConfig->edmaOutCfg.u.fmt1.dataOutPong.channel;

    /* Get HWA destination channel id */
    retVal = HWA_getDMAChanIndex(hwaHandle, edmaChanPing, &destChanPing);
    if (retVal != 0)
    {
        goto exit;
    }
    /* In interleave mode, only edmaOutCfgFmt is supported */
    retVal = HWA_getDMAChanIndex(hwaHandle, edmaChanPong, &destChanPong);
    if (retVal != 0)
    {
        goto exit;
    }


    /* Copy data from ADC buffer to HWA buffer */
    rangeProc_ConfigEDMA_DataIn(obj, DPParams, pHwConfig);

    /* Range FFT configuration in HWA */
    retVal = rangeProc_ConfigHWA(obj,
                                destChanPing,
                                destChanPong,
                                HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(obj->hwaMemBankAddr[0]),
                                HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(obj->hwaMemBankAddr[1]),
                                HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(obj->hwaMemBankAddr[2]),
                                HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(obj->hwaMemBankAddr[3]));
    if (retVal < 0)
    {
        goto exit;
    }

    /* Data output EDMA configuration */
    retVal = rangeProc_ConfigEDMA_DataOut(obj,
                                           DPParams,
                                           pHwConfig,
                                           (uint32_t)obj->hwaMemBankAddr[2],
                                           (uint32_t)obj->hwaMemBankAddr[3]);
    if (retVal < 0)
    {
        goto exit;
    }

exit:
    return (retVal);
}

/**
 *  @b Description
 *  @n
 *      Internal function to parse rangeProc configuration and save in internal rangeProc object
 *
 *  @param[in]  obj              Pointer to rangeProc object
 *  @param[in]  pConfigIn                 Pointer to rangeProc configuration structure
 *
 *  \ingroup    DPU_RANGEPROC_INTERNAL_FUNCTION
 *
 *  @retval
 *      Success     - 0
 *  @retval
 *      Error       - <0
 */
static int32_t rangeProc_ParseConfig(
    rangeProcObj         *obj,
    DPU_RangeProc_Config *pConfigIn)
{
    int32_t                        retVal = 0;
    rangeProc_dpParams            *params;
    DPU_RangeProc_StaticConfig *pStaticCfg;

    /* Get configuration pointers */
    pStaticCfg = &pConfigIn->staticCfg;
    params     = &obj->params;

    /* Save datapath parameters */
    params->numTxAntennas          = pStaticCfg->numTxAntennas;
    params->numRxAntennas          = pStaticCfg->ADCBufData.dataProperty.numRxAntennas;
    params->numVirtualAntennas     = pStaticCfg->numVirtualAntennas;
    params->numChirpsPerChirpEvent = pStaticCfg->ADCBufData.dataProperty.numChirpsPerChirpEvent;
    params->numAdcSamples          = pStaticCfg->ADCBufData.dataProperty.numAdcSamples;
    params->dataFmt                = pStaticCfg->ADCBufData.dataProperty.dataFmt; //MY_DBG
    params->numRangeBins           = pStaticCfg->numRangeBins;
    params->rangeFftSize           = pStaticCfg->rangeFftSize;
    params->numFramesPerSlidingWindow = pStaticCfg->numFramesPerSlidingWindow;
    params->frmCntrInSlidingWindowInitVal = pStaticCfg->frmCntrInSlidingWindowInitVal;
    params->loadHwaParamSetsBeforeExec = pStaticCfg->loadHwaParamSetsBeforeExec;
    params->prolonedBurstingMode = pStaticCfg->prolonedBurstingMode;

    params->numChirpsPerOneFrame  = pStaticCfg->numChirpsPerOneFrame;
    params->numChirpsPerSlidingWindow  = pStaticCfg->numChirpsPerSlidingWindow;
    params->butterflyScalingBitMask = pStaticCfg->butterflyScalingBitMask;

    if (params->numChirpsPerSlidingWindow == 1)
    {
        retVal = DPU_RANGEPROC_EINVAL;
        goto exit;
    }

    /* Save buffers */
    obj->ADCdataBuf   = (cmplx16ImRe_t *)pStaticCfg->ADCBufData.data;
    obj->radarCubebuf = (cmplx16ImRe_t *)pConfigIn->hwRes.radarCube.data;

    /* Save interleave mode from ADCBuf configuraiton */
    obj->interleave = pStaticCfg->ADCBufData.dataProperty.interleave;

    if ((obj->interleave == DPIF_RXCHAN_NON_INTERLEAVE_MODE) &&
        (obj->params.numRxAntennas > 1))
    {
        /* For rangeProcDPU needs rx channel has same offset from one channel to the next channel
           Use first two channel offset to calculate the BIdx for EDMA
         */
        int sampleSizeBytes;
        obj->rxChanOffset = pStaticCfg->ADCBufData.dataProperty.rxChanOffset[1] -
            pStaticCfg->ADCBufData.dataProperty.rxChanOffset[0];

        //Support real ADC
        if (pStaticCfg->ADCBufData.dataProperty.dataFmt == DPIF_DATAFORMAT_COMPLEX16_IMRE)
        {
            sampleSizeBytes = sizeof(cmplx16ImRe_t);
        }
        else
        {
            sampleSizeBytes = sizeof(uint16_t);
        }

        /* rxChanOffset should be 16 bytes aligned and should be big enough to hold numAdcSamples */
        if ((obj->rxChanOffset < (obj->params.numAdcSamples * sampleSizeBytes)) ||
            ((obj->rxChanOffset & 0xF) != 0))
        {
            retVal = DPU_RANGEPROC_EADCBUF_INTF;
            goto exit;
        }
    }

    /* Save RadarCube format */
    if (pConfigIn->hwRes.radarCube.datafmt == DPIF_RADARCUBE_FORMAT_2)
    {
        obj->radarCubeLayout = rangeProc_dataLayout_RANGE_CHIRP_TxAnt_RxAnt;
    }
    else if (pConfigIn->hwRes.radarCube.datafmt == DPIF_RADARCUBE_FORMAT_6)
    {
        obj->radarCubeLayout = rangeProc_dataLayout_CHIRP_TxAnt_RxAnt_RANGE;
    }
    else
    {
        retVal = DPU_RANGEPROC_EINTERNAL;
        goto exit;
    }


    /* Prepare internal hardware resouces = trigger source matchs its  paramset index */
    obj->dataInTrigger[0]  = pConfigIn->hwRes.hwaCfg.dmaTrigSrcChan[1]; // 1U + pConfigIn->hwRes.hwaCfg.paramSetStartIdx;
    obj->dataInTrigger[1]  = pConfigIn->hwRes.hwaCfg.dmaTrigSrcChan[3]; // 3U + pConfigIn->hwRes.hwaCfg.paramSetStartIdx;
    obj->dataOutTrigger[0] = pConfigIn->hwRes.hwaCfg.dmaTrigSrcChan[0]; // 0U + pConfigIn->hwRes.hwaCfg.paramSetStartIdx;
    obj->dataOutTrigger[1] = pConfigIn->hwRes.hwaCfg.dmaTrigSrcChan[2]; // 2U + pConfigIn->hwRes.hwaCfg.paramSetStartIdx;

    /* Save hardware resources that will be used at runtime */
    obj->edmaHandle           = pConfigIn->hwRes.edmaHandle;
    obj->dataOutSignatureChan = pConfigIn->hwRes.edmaOutCfg.dataOutSignature.channel;
    obj->edmaDataOutChan[0] = pConfigIn->hwRes.edmaOutCfg.u.fmt1.dataOutPing.channel;
    obj->edmaDataOutChan[1] = pConfigIn->hwRes.edmaOutCfg.u.fmt1.dataOutPong.channel;
    memcpy((void *)&obj->hwaCfg, (void *)&pConfigIn->hwRes.hwaCfg, sizeof(DPU_RangeProc_HwaConfig));

    /* EDMAout destination address offset (starting write position into the radar Cube) of the next frame data */
    if (obj->radarCubeLayout == rangeProc_dataLayout_RANGE_CHIRP_TxAnt_RxAnt)
    {
        obj->radCubeFrameOffsetInBytes = sizeof(cmplx16ImRe_t) * params->numRxAntennas * params->numChirpsPerOneFrame;
    }
    else if (obj->radarCubeLayout == rangeProc_dataLayout_CHIRP_TxAnt_RxAnt_RANGE)
    {
        obj->radCubeFrameOffsetInBytes = sizeof(cmplx16ImRe_t) * params->numRangeBins * params->numRxAntennas * params->numChirpsPerOneFrame;
    }
    else
    {
        retVal = DPU_RANGEPROC_EINTERNAL;
        goto exit;
    }
    obj->frmCntrInSlidingWindow = params->frmCntrInSlidingWindowInitVal;

exit:
    return (retVal);
}

/**
 *  @b Description
 *  @n
 *      Internal function to config HWA/EDMA to perform range FFT
 *
 *  @param[in]  obj              Pointer to rangeProc object
 *  @param[in]  pHwConfig                 Pointer to rangeProc hardware resources
 *
 *  \ingroup    DPU_RANGEPROC_INTERNAL_FUNCTION
 *
 *  @retval
 *      Success     - 0
 *  @retval
 *      Error       - <0
 */
static int32_t rangeProc_HardwareConfig(
    rangeProcObj               *obj,
    DPU_RangeProc_HW_Resources *pHwConfig)
{
    int32_t             retVal = 0;
    rangeProc_dpParams *DPParams;
    DPParams = &obj->params;

    retVal = rangeProc_ConifgNonInterleaveMode(obj, DPParams, pHwConfig);
    if (retVal != 0)
    {
        goto exit;
    }

exit:
    return (retVal);
}

/**************************************************************************
 ************************RangeProcHWA External APIs **************************
 **************************************************************************/

/**
 *  @b Description
 *  @n
 *      The function is rangeProc DPU init function. It allocates memory to store
 *  its internal data object and returns a handle if it executes successfully.
 *
 *  @param[in]  initParams              Pointer to DPU init parameters
 *  @param[in]  errCode                 Pointer to errCode generates from the API
 *
 *  \ingroup    DPU_RANGEPROC_EXTERNAL_FUNCTION
 *
 *  @retval
 *      Success     - valid rangeProc handle
 *  @retval
 *      Error       - NULL
 */
DPU_RangeProc_Handle DPU_RangeProc_init(
    DPU_RangeProc_InitParams *initParams,
    int32_t                     *errCode)
{
    rangeProcObj  *obj = NULL;
    HWA_MemInfo       hwaMemInfo;
    uint8_t           index;
    int32_t             status;

    *errCode = 0;

    if ((initParams == NULL) ||
        (initParams->hwaHandle == NULL))
    {
        *errCode = DPU_RANGEPROC_EINVAL;
        goto exit;
    }

    /* Allocate Memory for rangeProc */
    obj = (rangeProcObj*)&gRangeProcHeapMem;//MemoryP_ctrlAlloc(sizeof(rangeProcObj), 0);
    if (obj == NULL)
    {
        *errCode = DPU_RANGEPROC_ENOMEM;
        goto exit;
    }

    printf("RANGE DPU: (rangeProcObj *) 0x%08x\n", (uint32_t) obj);

    /* Initialize memory */
    memset((void *)obj, 0, sizeof(rangeProcObj));

    memcpy((void *)&obj->initParms, initParams, sizeof(DPU_RangeProc_InitParams));

    /* Set HWA bank memory address */
    *errCode = HWA_getHWAMemInfo(initParams->hwaHandle, &hwaMemInfo);
    if (*errCode < 0)
    {
        goto exit;
    }

    for (index = 0; index < hwaMemInfo.numBanks; index++)
    {
        obj->hwaMemBankAddr[index] = hwaMemInfo.baseAddress + index * hwaMemInfo.bankSize;
    }

    /* Create semaphore for EDMA done */
    status = SemaphoreP_constructBinary(&obj->edmaDoneSemaHandle, 0);
    if(status != SystemP_SUCCESS)
    {
        *errCode = DPU_RANGEPROC_ESEMA;
        goto exit;
    }




    /* Create semaphore for HWA done */
    status = SemaphoreP_constructBinary(&obj->hwaDoneSemaHandle, 0);
    if(status != SystemP_SUCCESS)
    {
        *errCode = DPU_RANGEPROC_ESEMA;
        goto exit;
    }


exit:
    if (*errCode < 0)
    {
        obj = (DPU_RangeProc_Handle)NULL;
    }
    else
    {
        /* Fall through */
    }
    return ((DPU_RangeProc_Handle)obj);
}


/**
  *  @b Description
  *  @n
  *   Returns number of allocated HWA Param sets
  *
  *  @param[in]   handle     DPU handle.
  *  @param[out]   cfg       Number of allocated HWA Param sets
  *
  *  @retval
  *      Success      = 0
  *  @retval
  *      Error       != 0
  */
int32_t DPU_RangeProc_GetNumUsedHwaParamSets
(
    DPU_RangeProc_Handle  handle,
    uint8_t *numUsedHwaParamSets
)
{
    rangeProcObj *obj;
    int32_t retVal = 0;

    obj = (rangeProcObj *)handle;
    if (obj == NULL)
    {
        retVal = DPU_RANGEPROC_EINVAL;
        goto exit;
    }
    *numUsedHwaParamSets = (uint8_t) (obj->hwaCfg.numParamSet);
exit:
    return retVal;
}

/**
 *  @b Description
 *  @n
 *      The function is rangeProc DPU config function. It saves buffer pointer and configurations
 *  including system resources and configures HWA and EDMA for runtime range processing.
 *
 *  @pre    DPU_RangeProc_init() has been called
 *
 *  @param[in]  handle                  rangeProc DPU handle
 *  @param[in]  pConfigIn               Pointer to rangeProc configuration data structure
 *
 *  \ingroup    DPU_RANGEPROC_EXTERNAL_FUNCTION
 *
 *  @retval
 *      Success     - 0
 *  @retval
 *      Error       - <0
 */
int32_t DPU_RangeProc_config(
    DPU_RangeProc_Handle  handle,
    DPU_RangeProc_Config *pConfigIn)
{
    rangeProcObj               *obj;
    DPU_RangeProc_StaticConfig *pStaticCfg;
    HWA_Handle                     hwaHandle;
    int32_t                        retVal = 0;

    obj = (rangeProcObj *)handle;
    if (obj == NULL)
    {
        retVal = DPU_RANGEPROC_EINVAL;
        goto exit;
    }

    /* Get configuration pointers */
    pStaticCfg = &pConfigIn->staticCfg;
    hwaHandle  = obj->initParms.hwaHandle;

    /* Needed to fix mmwave restart */
    obj->inProgress = false;

#if DEBUG_CHECK_PARAMS
    /* Validate params */
    if (!pConfigIn || !pConfigIn->hwRes.edmaHandle)
    {
        retVal = DPU_RANGEPROC_EINVAL;
        goto exit;
    }

    /* Parameter check: validate Adc data interface configuration
        Support:
            - 1 chirp per chirpEvent
            - Complex 16bit ADC data in IMRE format
     */
    if (((pStaticCfg->ADCBufData.dataProperty.dataFmt != DPIF_DATAFORMAT_COMPLEX16_IMRE) && (pStaticCfg->ADCBufData.dataProperty.dataFmt != DPIF_DATAFORMAT_REAL16)) ||      //Support real ADC
        (pStaticCfg->ADCBufData.dataProperty.numChirpsPerChirpEvent != 1U))
    {
        retVal = DPU_RANGEPROC_EADCBUF_INTF;
        goto exit;
    }

    /* Parameter check: windowing Size */
    {
        uint16_t expectedWinSize;

        if (pConfigIn->hwRes.hwaCfg.hwaWinSym == HWA_FFT_WINDOW_SYMMETRIC)
        {
            /* Only half of the windowing factor is needed for symmetric window */
            expectedWinSize = ((pStaticCfg->ADCBufData.dataProperty.numAdcSamples + 1U) / 2U) * sizeof(uint32_t);
        }
        else
        {
            expectedWinSize = pStaticCfg->ADCBufData.dataProperty.numAdcSamples * sizeof(uint32_t);
        }

        if (pStaticCfg->windowSize != expectedWinSize)
        {
            retVal = DPU_RANGEPROC_EWINDOW;
            goto exit;
        }
    }

    /* Refer to radar cube definition for FORMAT_x , the following are the only supported formats
        Following assumption is made upon radar cube FORMAT_x definition
           1. data type is complex in cmplx16ImRe_t format only
           2. It is always 1D range output.
     */
    if ((pConfigIn->hwRes.radarCube.datafmt != DPIF_RADARCUBE_FORMAT_6) &&
        (pConfigIn->hwRes.radarCube.datafmt != DPIF_RADARCUBE_FORMAT_2))
    {
        retVal = DPU_RANGEPROC_ERADARCUBE_INTF;
        goto exit;
    }

    /* Not supported input & output format combination */
#if 0
    if (pStaticCfg->ADCBufData.dataProperty.numRxAntennas == 3U)
    {
        retVal = DPU_RANGEPROC_ENOTIMPL;
        goto exit;
    }
#endif

    if (pConfigIn->hwRes.radarCube.dataSize != (pStaticCfg->numRangeBins * sizeof(cmplx16ImRe_t) * pStaticCfg->numChirpsPerSlidingWindow * pStaticCfg->ADCBufData.dataProperty.numRxAntennas))
    {
        retVal = DPU_RANGEPROC_ERADARCUBE_INTF;
        goto exit;
    }
#endif // DEBUG_CHECK_PARAMS

    retVal = rangeProc_ParseConfig(obj, pConfigIn);
    if (retVal < 0)
    {
        goto exit;
    }


    /* Disable the HWA */
    retVal = HWA_enable(hwaHandle, 0);
    if (retVal != 0)
    {
        goto exit;
    }

    /* Reset the internal state of the HWA */
    retVal = HWA_reset(hwaHandle);
    if (retVal != 0)
    {
        goto exit;
    }

    /* Windowing configuraiton in HWA */
    retVal = HWA_configRam(hwaHandle,
                           HWA_RAM_TYPE_WINDOW_RAM,
                           (uint8_t *)pStaticCfg->window,
                           pStaticCfg->windowSize, /* size in bytes */
                           pConfigIn->hwRes.hwaCfg.hwaWinRamOffset * sizeof(uint32_t));
    if (retVal != 0)
    {
        goto exit;
    }

    /* Clear stats */
    obj->numProcess = 0U;

    /* Initial configuration of rangeProc */
    retVal = rangeProc_HardwareConfig(obj, &pConfigIn->hwRes);

exit:
    return retVal;
}

/**
 *  @b Description
 *  @n
 *      The function is rangeProc DPU process function. It allocates memory to store
 *  its internal data object and returns a handle if it executes successfully.
 *
 *  @pre    DPU_RangeProc_init() has been called
 *
 *  @param[in]  handle                  rangeProc DPU handle
 *  @param[in]  outParams               DPU output parameters
 *
 *  \ingroup    DPU_RANGEPROC_EXTERNAL_FUNCTION
 *
 *  @retval
 *      Success     - 0
 *  @retval
 *      Error       - <0
 */
int32_t DPU_RangeProc_process(
    DPU_RangeProc_Handle     handle,
    DPU_RangeProc_OutParams *outParams)
{
    rangeProcObj *obj;
    int32_t          retVal = 0;

    obj = (rangeProcObj *)handle;
    if ((obj == NULL) ||
        (outParams == NULL))
    {
        retVal = DPU_RANGEPROC_EINVAL;
        goto exit;
    }

    /* Set inProgress state */
    obj->inProgress = true;
    outParams->endOfChirp    = false;

    /**********************************************/
    /* WAIT FOR HWA NUMLOOPS INTERRUPT            */
    /**********************************************/
    /* wait for the all paramSets done interrupt */
    SemaphoreP_pend(&obj->hwaDoneSemaHandle, SystemP_WAIT_FOREVER);

    /**********************************************/
    /* WAIT FOR EDMA INTERRUPT                    */
    /**********************************************/
    SemaphoreP_pend(&obj->edmaDoneSemaHandle, SystemP_WAIT_FOREVER);

    /* Range FFT is done, disable Done interrupt */
    HWA_disableDoneInterrupt(obj->initParms.hwaHandle);

    /* Disable the HWA */
    retVal = HWA_enable(obj->initParms.hwaHandle, 0);
    if (retVal != 0)
    {
        goto exit;
    }

    /* Update stats and output parameters */
    obj->numProcess++;

    /* Following stats is not available for rangeProc */
    //outParams->stats.processingTime = 0;
    //outParams->stats.waitTime       = 0;

    outParams->endOfChirp = true;

    /* Clear inProgress state */
    obj->inProgress = false;

exit:

    return retVal;
}


/**
 *  @b Description
 *  @n
 *      The function is rangeProc DPU control function.
 *
 *  @pre    DPU_RangeProc_init() has been called
 *
 *  @param[in]  handle           rangeProc DPU handle
 *  @param[in]  cmd              rangeProc DPU control command
 *  @param[in]  arg              rangeProc DPU control argument pointer
 *  @param[in]  argSize          rangeProc DPU control argument size
 *
 *  \ingroup    DPU_RANGEPROC_EXTERNAL_FUNCTION
 *
 *  @retval
 *      Success     - 0
 *  @retval
 *      Error       - <0
 */
int32_t DPU_RangeProc_control(
    DPU_RangeProc_Handle handle,
    DPU_RangeProc_Cmd    cmd,
    void                   *arg,
    uint32_t                argSize)
{
    int32_t          retVal = 0;
    rangeProcObj *obj;

    /* Get rangeProc data object */
    obj = (rangeProcObj *)handle;

    /* Sanity check */
    if (obj == NULL)
    {
        retVal = DPU_RANGEPROC_EINVAL;
        goto exit;
    }

    /* Check if control() is called during processing time */
    if (obj->inProgress == true)
    {
        retVal = DPU_RANGEPROC_EINPROGRESS;
        goto exit;
    }

    /* Control command handling */
    switch (cmd)
    {

        case DPU_RangeProc_Cmd_triggerProc:
            /* Trigger rangeProc in HWA */
            retVal = rangeProc_TriggerHWA(obj);
            if (retVal != 0)
            {
                goto exit;
            }
            break;

        default:
            retVal = DPU_RANGEPROC_ECMD;
            break;
    }
exit:
    return (retVal);
}


/**
 *  @b Description
 *  @n
 *      The function is rangeProc DPU deinit function. It frees the resources used for the DPU.
 *
 *  @pre    DPU_RangeProc_init() has been called
 *
 *  @param[in]  handle           rangeProc DPU handle
 *
 *  \ingroup    DPU_RANGEPROC_EXTERNAL_FUNCTION
 *
 *  @retval
 *      Success     - 0
 *  @retval
 *      Error       - <0
 */
int32_t DPU_RangeProc_deinit(
    DPU_RangeProc_Handle handle)
{
    rangeProcObj *obj;
    int32_t          retVal = 0;

    /* Sanity Check */
    obj = (rangeProcObj *)handle;
    if (obj == NULL)
    {
        retVal = DPU_RANGEPROC_EINVAL;
        goto exit;
    }

    /* Delete Semaphores */
    SemaphoreP_destruct(&obj->edmaDoneSemaHandle);
    SemaphoreP_destruct(&obj->hwaDoneSemaHandle);

exit:
    return (retVal);
}
