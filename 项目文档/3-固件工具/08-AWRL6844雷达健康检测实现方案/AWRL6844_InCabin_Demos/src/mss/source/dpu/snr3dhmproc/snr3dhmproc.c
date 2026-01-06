/**
 *   @file  snr3dhmproc.c
 *
 *   @brief
 *      Implements snr3dhm DPU.
 *
 *  \par
 *  NOTE:
 *      (C) Copyright 2018-2024 Texas Instruments, Inc.
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

/* Standard Include Files. */
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* mmWave SDK driver/common Include Files */
#include <drivers/hw_include/hw_types.h>
#include <kernel/dpl/SemaphoreP.h>
#include <kernel/dpl/CacheP.h>
#include <kernel/dpl/HeapP.h>
#include <drivers/edma.h>
#include <drivers/soc.h>
#ifdef SUBSYS_MSS
#include <kernel/dpl/CacheP.h>
#endif

#include <common/syscommon.h>

/* MATH utils library Include files */
#include <utils/mathutils/mathutils.h>

/* Data Path Include Files */
#include <datapath/dpedma/v1/dpedmahwa.h>
#include <source/hwa_adapt/hwa_adapt.h>
#include <source/dpu/snr3dhmproc/snr3dhmprocinternal.h>

#define DPU_SNR3DHM_HWA_BANK_0   0x0000
#define DPU_SNR3DHM_HWA_BANK_1   0x4000
#define DPU_SNR3DHM_HWA_BANK_2   0x8000
#define DPU_SNR3DHM_HWA_BANK_3   0xC000
#define DPU_SNR3DHM_HWA_BANK_SIZE   0x4000

#define DBG_SNR3DHM_OBJ_DPU

/* @brief definition for detection matrix data QFormat */
#define CFAR_HWA_DETMATRIX_DATA_QFORMAT      11// 8 //ToDo check for 11

#ifdef DBG_SNR3DHM_OBJ_DPU
volatile snr3dhmObj *gSnr3dshObj = NULL;
#endif

/**
 *  @b Description
 *  @n
 *      HWA processing completion call back function as per HWA API.
 *      Depending on the programmed transfer completion codes,
 *      posts HWA done semaphore.
 *
 *  @param[in]  arg                 Argument to the callback function
 *
 *  \ingroup    DPU_SNR3DPROC_INTERNAL_FUNCTION
 *
 *  @retval     N/A
 */
static void SNR3DHM_hwaDoneIsrCallback(void * arg)
{
    if (arg != NULL) 
    {
        SemaphoreP_post((SemaphoreP_Object *)arg);
    }
}

/**
 *  @b Description
 *  @n
 *      EDMA completion call back function.
 *
 *  @param[in] intrHandle    EDMA Interrupt handle
 *  @param[in] arg           Input argument is pointer to semaphore object
 *
 *  \ingroup    DPU_SNR3DPROC_INTERNAL_FUNCTION
 */
static void SNR3DHM_edmaDoneIsrCallback(Edma_IntrHandle intrHandle, void *arg)
{
    if (arg != NULL) {
        SemaphoreP_post((SemaphoreP_Object *)arg);
    }
}


/** @brief Configures HWA ParameterSet.
 *
 *  \ingroup    DPU_SNR3DPROC_INTERNAL_FUNCTION
 *
 *   @param[in] pRes                Pointer to hardware resources
 *
 *   @param[in] handle              HWA driver handle
 *
 */
int32_t  SNR3DHM_config_HWA(snr3dhmObj *snr3dshObj,
                                 DPU_SNR3DHM_Config *cfarHwaCfg)
{
    HWA_ParamConfig hwaParamCfg;
    DPU_SNR3DHM_HW_Resources *pRes = &cfarHwaCfg->res;
    int32_t retVal = 0;
    HWA_InterruptConfig     paramISRConfig;
    uint8_t paramSetCurrentIdx;
    uint8_t destChan;
    HWA_Handle handle = snr3dshObj->hwaHandle;
    uint8_t  paramSetStartIdx = pRes->hwaCfg.paramSetStartIdx;
    DPU_SNR3DProc_CfarCfg *cfarCfg = cfarHwaCfg->dynCfg.cfarCfg;
    DPU_SNR3DProc_CfarScndPassCfg *cfarScndPassCfg = cfarHwaCfg->dynCfg.cfarScndPassCfg;
    uint32_t numRangeBins = cfarHwaCfg->staticCfg.numRangeBins;
    uint32_t numAzimuthBins = cfarHwaCfg->staticCfg.azimuthFftSize;
    uint32_t pingPongInd;
    uint8_t  divShiftBeforeLog = cfarHwaCfg->staticCfg.divShiftBeforeLog;

    if ((2 * numRangeBins * numAzimuthBins * sizeof(DPU_SNR3DHM_CfarRawOutput)) > (2*SOC_HWA_MEM_SIZE/SOC_HWA_NUM_MEM_BANKS))
    {
        retVal = DPU_SNR3DHM_ENOMEM__DET_MATRIX_EXCEEDS_HWA_INP_MEM;
        goto exit;
    }

    retVal = HWA_enable(handle, 0); // Disable HWA
    if (retVal != 0)
    {
        goto exit;
    }

    paramSetCurrentIdx = paramSetStartIdx;

    if (!cfarHwaCfg->dynCfg.cfarScndPassCfg->enabled)
    {
        for (pingPongInd = 0; pingPongInd < 2; pingPongInd++)
        {
            /**********************************/
            /* Configure Range-CFAR Param set */
            /**********************************/
            memset( (void*) &hwaParamCfg, 0, sizeof(hwaParamCfg));
            hwaParamCfg.triggerMode = HWA_TRIG_MODE_DMA;
            hwaParamCfg.dmaTriggerSrc = pRes->hwaCfg.dmaTriggerSource[pingPongInd];

            hwaParamCfg.accelMode = HWA_ACCELMODE_CFAR;
            if (cfarCfg->cyclicMode)
            {
                hwaParamCfg.source.srcAcnt = (numRangeBins - 1) + (cfarCfg->guardLen + cfarCfg->winLen) + (cfarCfg->guardLen + cfarCfg->winLen);
                hwaParamCfg.source.srcShift = numRangeBins - (cfarCfg->guardLen + cfarCfg->winLen);
                hwaParamCfg.source.srcCircShiftWrap = mathUtils_floorLog2(numRangeBins);
            }
            else
            {
                hwaParamCfg.source.srcAcnt = numRangeBins - 1;
                hwaParamCfg.source.srcShift = 0;
                hwaParamCfg.source.srcCircShiftWrap = 0;
            }

            hwaParamCfg.source.srcAIdx = numAzimuthBins*sizeof(uint32_t);
            hwaParamCfg.source.srcBIdx = sizeof(uint32_t);

            hwaParamCfg.accelModeArgs.cfarMode.operMode = HWA_CFAR_OPER_MODE_MAG_INPUT_REAL;
            hwaParamCfg.source.srcWidth = HWA_SAMPLES_WIDTH_32BIT;
            hwaParamCfg.source.srcSign = HWA_SAMPLES_UNSIGNED;
            hwaParamCfg.source.srcScale = 0;

            hwaParamCfg.source.srcBcnt = numAzimuthBins-1;
            hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_REAL;
            hwaParamCfg.source.srcAddr = DPU_SNR3DHM_HWA_BANK_0 + pingPongInd * DPU_SNR3DHM_HWA_BANK_SIZE;

            hwaParamCfg.dest.dstAddr = DPU_SNR3DHM_HWA_BANK_2 + pingPongInd * DPU_SNR3DHM_HWA_BANK_SIZE;
            hwaParamCfg.dest.dstAcnt = numRangeBins-1;
            hwaParamCfg.dest.dstRealComplex = HWA_SAMPLES_FORMAT_COMPLEX;
            hwaParamCfg.dest.dstWidth = HWA_SAMPLES_WIDTH_32BIT;
            hwaParamCfg.dest.dstSign = HWA_SAMPLES_UNSIGNED;
            hwaParamCfg.dest.dstAIdx = sizeof(DPU_SNR3DHM_CfarRawOutput);
            hwaParamCfg.dest.dstBIdx = numRangeBins * sizeof(DPU_SNR3DHM_CfarRawOutput);
            hwaParamCfg.dest.dstScale = 8;

            hwaParamCfg.accelModeArgs.cfarMode.numGuardCells = cfarCfg->guardLen;
            hwaParamCfg.accelModeArgs.cfarMode.nAvgDivFactor = cfarCfg->noiseDivShift;
            hwaParamCfg.accelModeArgs.cfarMode.cyclicModeEn = cfarCfg->cyclicMode;
            hwaParamCfg.accelModeArgs.cfarMode.nAvgMode = cfarCfg->averageMode;
            hwaParamCfg.accelModeArgs.cfarMode.numNoiseSamplesRight = cfarCfg->winLen >> 1;
            hwaParamCfg.accelModeArgs.cfarMode.numNoiseSamplesLeft =  cfarCfg->winLen >> 1;
            hwaParamCfg.accelModeArgs.cfarMode.outputMode = HWA_CFAR_OUTPUT_MODE_I_nAVG_ALL_Q_CUT;

            retVal = HWA_configParamSet(handle, paramSetCurrentIdx, &hwaParamCfg, NULL);
            if (retVal != 0)
            {
                goto exit;
            }

            /* Disable trigger DMA/interrupt for this param set */
            retVal = HWA_disableParamSetInterrupt(handle,
                                                  paramSetCurrentIdx,
                                                  HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
            if (retVal != 0)
            {
                goto exit;
            }
            paramSetCurrentIdx++;

            /**************************************************************/
            /* {s0,n0},{s1,n1}... ->Log2->  {s0,s1,s2...},{n0,n1,n2,...}  */
            /**************************************************************/
            memset( (void*) &hwaParamCfg, 0, sizeof(hwaParamCfg));
            hwaParamCfg.triggerMode = HWA_TRIG_MODE_IMMEDIATE;
            hwaParamCfg.dmaTriggerSrc = 0;

            hwaParamCfg.accelMode = HWA_ACCELMODE_FFT;

            hwaParamCfg.source.srcAcnt = (numRangeBins * numAzimuthBins) - 1;
            hwaParamCfg.source.srcAIdx = sizeof(DPU_SNR3DHM_CfarRawOutput);
            hwaParamCfg.source.srcBIdx = sizeof(uint32_t);

            hwaParamCfg.source.srcWidth = HWA_SAMPLES_WIDTH_32BIT;
            hwaParamCfg.source.srcScale = divShiftBeforeLog;
            hwaParamCfg.source.srcBcnt = 2 - 1;
            hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_REAL;
            hwaParamCfg.source.srcSign = HWA_SAMPLES_UNSIGNED;
            hwaParamCfg.source.srcAddr = DPU_SNR3DHM_HWA_BANK_2 + pingPongInd * DPU_SNR3DHM_HWA_BANK_SIZE;

            hwaParamCfg.dest.dstAddr = DPU_SNR3DHM_HWA_BANK_0 + pingPongInd * DPU_SNR3DHM_HWA_BANK_SIZE;
            hwaParamCfg.dest.dstAcnt = (numRangeBins * numAzimuthBins) - 1;
            hwaParamCfg.dest.dstRealComplex = HWA_SAMPLES_FORMAT_REAL;
            hwaParamCfg.dest.dstWidth = HWA_SAMPLES_WIDTH_16BIT;
            hwaParamCfg.dest.dstAIdx = sizeof(uint16_t);
            hwaParamCfg.dest.dstBIdx = (numRangeBins * numAzimuthBins) * sizeof(uint16_t);
            hwaParamCfg.dest.dstScale = 0;

            hwaParamCfg.accelModeArgs.fftMode.fftEn = 0;
            hwaParamCfg.accelModeArgs.fftMode.magLogEn = HWA_FFT_MODE_MAGNITUDE_LOG2_ENABLED;
            hwaParamCfg.accelModeArgs.fftMode.fftOutMode = HWA_FFT_MODE_OUTPUT_DEFAULT;

            retVal = HWA_configParamSet(handle, paramSetCurrentIdx, &hwaParamCfg, NULL);
            if (retVal != 0)
            {
                goto exit;
            }

            /* Disable trigger DMA/interrupt for this param set */
            retVal = HWA_disableParamSetInterrupt(handle,
                                                  paramSetCurrentIdx,
                                                  HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
            if (retVal != 0)
            {
                goto exit;
            }
            paramSetCurrentIdx++;

            /****************************************************************************/
            /* SNR: {s0,s1,s2...},{n0,n1,n2,...} -> 2-pointFFT -> {s0-n0},{s1-n1},...   */
            /****************************************************************************/
            memset( (void*) &hwaParamCfg, 0, sizeof(hwaParamCfg));
            hwaParamCfg.triggerMode = HWA_TRIG_MODE_IMMEDIATE;
            hwaParamCfg.dmaTriggerSrc = 0;

            hwaParamCfg.accelMode = HWA_ACCELMODE_FFT;

            hwaParamCfg.source.srcAcnt = 2 - 1;
            hwaParamCfg.source.srcAIdx = (numRangeBins * numAzimuthBins) * sizeof(uint16_t);
            hwaParamCfg.source.srcBIdx = sizeof(uint16_t);

            hwaParamCfg.source.srcWidth = HWA_SAMPLES_WIDTH_16BIT;
            hwaParamCfg.source.srcScale = 8;
            hwaParamCfg.source.srcBcnt = (numRangeBins * numAzimuthBins) - 1;
            hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_REAL;
            hwaParamCfg.source.srcSign = HWA_SAMPLES_UNSIGNED;
            hwaParamCfg.source.srcAddr = DPU_SNR3DHM_HWA_BANK_0 + pingPongInd * DPU_SNR3DHM_HWA_BANK_SIZE;

            hwaParamCfg.dest.dstAddr = DPU_SNR3DHM_HWA_BANK_2 + pingPongInd * DPU_SNR3DHM_HWA_BANK_SIZE;
            hwaParamCfg.dest.dstAcnt = 2 - 1; //subtraction
            hwaParamCfg.dest.dstSkipInit = 1; //subtraction
            hwaParamCfg.dest.dstRealComplex = HWA_SAMPLES_FORMAT_REAL;
            hwaParamCfg.dest.dstWidth = HWA_SAMPLES_WIDTH_16BIT;
            hwaParamCfg.dest.dstSign = HWA_SAMPLES_SIGNED;
            hwaParamCfg.dest.dstAIdx = sizeof(uint16_t);
            hwaParamCfg.dest.dstBIdx = sizeof(uint16_t);
            hwaParamCfg.dest.dstScale = 0;

            hwaParamCfg.accelModeArgs.fftMode.fftEn = 1;
            hwaParamCfg.accelModeArgs.fftMode.fftSize = 1;
            hwaParamCfg.accelModeArgs.fftMode.butterflyScaling = 0;
            hwaParamCfg.accelModeArgs.fftMode.magLogEn = HWA_FFT_MODE_MAGNITUDE_LOG2_DISABLED;
            hwaParamCfg.accelModeArgs.fftMode.fftOutMode = HWA_FFT_MODE_OUTPUT_DEFAULT;

            retVal = HWA_configParamSet(handle, paramSetCurrentIdx, &hwaParamCfg, NULL);
            if (retVal != 0)
            {
                goto exit;
            }

            /* Disable trigger DMA/interrupt for this param set */
            retVal = HWA_disableParamSetInterrupt(handle,
                                                  paramSetCurrentIdx,
                                                  HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
            if (retVal != 0)
            {
                goto exit;
            }

            retVal = HWA_getDMAChanIndex(handle,
                                         pRes->edmaHwaOut[pingPongInd].channel,
                                         &destChan);

            paramISRConfig.interruptTypeFlag = HWA_PARAMDONE_INTERRUPT_TYPE_DMA;
            paramISRConfig.dma.dstChannel = destChan;
            paramISRConfig.cpu.callbackArg = NULL;
            paramISRConfig.cpu.callbackFn = NULL;

            retVal = HWA_enableParamSetInterrupt(handle,
                                                 paramSetCurrentIdx,
                                                 &paramISRConfig);
            if (retVal != 0)
             {
                  goto exit;
             }
            paramSetCurrentIdx++;
        }
    }
    else
    {
        /****************************/
        /* Second Pass CFAR enabled */
        /****************************/
        for (pingPongInd = 0; pingPongInd < 2; pingPongInd++)
        {
            /**********************************/
            /* Configure Range-CFAR Param set */
            /**********************************/
            memset( (void*) &hwaParamCfg, 0, sizeof(hwaParamCfg));
            hwaParamCfg.triggerMode = HWA_TRIG_MODE_DMA;
            hwaParamCfg.dmaTriggerSrc = pRes->hwaCfg.dmaTriggerSource[pingPongInd];

            hwaParamCfg.accelMode = HWA_ACCELMODE_CFAR;
            if (cfarCfg->cyclicMode)
            {
                hwaParamCfg.source.srcAcnt = (numRangeBins - 1) + (cfarCfg->guardLen + cfarCfg->winLen) + (cfarCfg->guardLen + cfarCfg->winLen);
                hwaParamCfg.source.srcShift = numRangeBins - (cfarCfg->guardLen + cfarCfg->winLen);
                hwaParamCfg.source.srcCircShiftWrap = mathUtils_floorLog2(numRangeBins);
            }
            else
            {
                hwaParamCfg.source.srcAcnt = numRangeBins - 1;
                hwaParamCfg.source.srcShift = 0;
                hwaParamCfg.source.srcCircShiftWrap = 0;
            }

            hwaParamCfg.source.srcAIdx = numAzimuthBins*sizeof(uint32_t);
            hwaParamCfg.source.srcBIdx = sizeof(uint32_t);

            hwaParamCfg.source.srcWidth = HWA_SAMPLES_WIDTH_32BIT;
            hwaParamCfg.source.srcSign = HWA_SAMPLES_UNSIGNED;
            hwaParamCfg.source.srcScale = 0;

            hwaParamCfg.source.srcBcnt = numAzimuthBins-1;
            hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_REAL;
            hwaParamCfg.source.srcAddr = DPU_SNR3DHM_HWA_BANK_0 + pingPongInd * DPU_SNR3DHM_HWA_BANK_SIZE;

            hwaParamCfg.dest.dstAddr = DPU_SNR3DHM_HWA_BANK_2 + pingPongInd * DPU_SNR3DHM_HWA_BANK_SIZE;
            hwaParamCfg.dest.dstAcnt = numRangeBins-1;
            hwaParamCfg.dest.dstRealComplex = HWA_SAMPLES_FORMAT_COMPLEX;
            hwaParamCfg.dest.dstWidth = HWA_SAMPLES_WIDTH_32BIT;
            hwaParamCfg.dest.dstSign = HWA_SAMPLES_UNSIGNED;
            hwaParamCfg.dest.dstAIdx = sizeof(DPU_SNR3DHM_CfarRawOutput);
            hwaParamCfg.dest.dstBIdx = numRangeBins * sizeof(DPU_SNR3DHM_CfarRawOutput);
            hwaParamCfg.dest.dstScale = 8;

            hwaParamCfg.accelModeArgs.cfarMode.operMode = HWA_CFAR_OPER_MODE_MAG_INPUT_REAL;
            hwaParamCfg.accelModeArgs.cfarMode.numGuardCells = cfarCfg->guardLen;
            hwaParamCfg.accelModeArgs.cfarMode.nAvgDivFactor = cfarCfg->noiseDivShift;
            hwaParamCfg.accelModeArgs.cfarMode.cyclicModeEn = cfarCfg->cyclicMode;
            hwaParamCfg.accelModeArgs.cfarMode.nAvgMode = cfarCfg->averageMode;
            hwaParamCfg.accelModeArgs.cfarMode.numNoiseSamplesRight = cfarCfg->winLen >> 1;
            hwaParamCfg.accelModeArgs.cfarMode.numNoiseSamplesLeft =  cfarCfg->winLen >> 1;
            hwaParamCfg.accelModeArgs.cfarMode.outputMode = HWA_CFAR_OUTPUT_MODE_I_nAVG_ALL_Q_CUT;

            retVal = HWA_configParamSet(handle, paramSetCurrentIdx, &hwaParamCfg, NULL);
            if (retVal != 0)
            {
                goto exit;
            }

            /* Disable trigger DMA/interrupt for this param set */
            retVal = HWA_disableParamSetInterrupt(handle,
                                                  paramSetCurrentIdx,
                                                  HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
            if (retVal != 0)
            {
                goto exit;
            }
            paramSetCurrentIdx++;

            /************************************/
            /* Configure Azimuth-CFAR Param set */
            /************************************/
            memset( (void*) &hwaParamCfg, 0, sizeof(hwaParamCfg));
            hwaParamCfg.triggerMode = HWA_TRIG_MODE_IMMEDIATE;
            hwaParamCfg.dmaTriggerSrc = 0;

            hwaParamCfg.accelMode = HWA_ACCELMODE_CFAR;
            if (cfarScndPassCfg->cyclicMode)
            {
                hwaParamCfg.source.srcAcnt = (numAzimuthBins - 1) + (cfarScndPassCfg->guardLen + cfarScndPassCfg->winLen) + (cfarScndPassCfg->guardLen + cfarScndPassCfg->winLen);
                hwaParamCfg.source.srcShift = numAzimuthBins - (cfarScndPassCfg->guardLen + cfarScndPassCfg->winLen);
                hwaParamCfg.source.srcCircShiftWrap = mathUtils_floorLog2(numAzimuthBins);
            }
            else
            {
                hwaParamCfg.source.srcAcnt = numAzimuthBins - 1;
                hwaParamCfg.source.srcShift = 0;
                hwaParamCfg.source.srcCircShiftWrap = 0;
            }
            hwaParamCfg.source.srcAIdx = sizeof(uint32_t);
            hwaParamCfg.source.srcBIdx = numAzimuthBins*sizeof(uint32_t);

            hwaParamCfg.source.srcWidth = HWA_SAMPLES_WIDTH_32BIT;
            hwaParamCfg.source.srcSign = HWA_SAMPLES_UNSIGNED;
            hwaParamCfg.source.srcScale = 0;

            hwaParamCfg.source.srcBcnt = numRangeBins-1;
            hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_REAL;
            hwaParamCfg.source.srcAddr = DPU_SNR3DHM_HWA_BANK_0 + pingPongInd * DPU_SNR3DHM_HWA_BANK_SIZE;

            hwaParamCfg.dest.dstAddr = DPU_SNR3DHM_HWA_BANK_2 + numRangeBins*numAzimuthBins*sizeof(DPU_SNR3DHM_CfarRawOutput) + pingPongInd * DPU_SNR3DHM_HWA_BANK_SIZE;
            hwaParamCfg.dest.dstAcnt = numAzimuthBins-1;
            hwaParamCfg.dest.dstSkipInit = 0;
            hwaParamCfg.dest.dstRealComplex = HWA_SAMPLES_FORMAT_COMPLEX;
            hwaParamCfg.dest.dstWidth = HWA_SAMPLES_WIDTH_32BIT;
            hwaParamCfg.dest.dstSign = HWA_SAMPLES_UNSIGNED;
            hwaParamCfg.dest.dstAIdx = sizeof(DPU_SNR3DHM_CfarRawOutput);
            hwaParamCfg.dest.dstBIdx = numAzimuthBins * sizeof(DPU_SNR3DHM_CfarRawOutput);
            hwaParamCfg.dest.dstScale = 8;

            hwaParamCfg.accelModeArgs.cfarMode.operMode = HWA_CFAR_OPER_MODE_MAG_INPUT_REAL;
            hwaParamCfg.accelModeArgs.cfarMode.numGuardCells = cfarScndPassCfg->guardLen;
            hwaParamCfg.accelModeArgs.cfarMode.nAvgDivFactor = cfarScndPassCfg->noiseDivShift;
            hwaParamCfg.accelModeArgs.cfarMode.cyclicModeEn = cfarScndPassCfg->cyclicMode;
            hwaParamCfg.accelModeArgs.cfarMode.nAvgMode = cfarScndPassCfg->averageMode;
            hwaParamCfg.accelModeArgs.cfarMode.numNoiseSamplesRight = cfarScndPassCfg->winLen >> 1;
            hwaParamCfg.accelModeArgs.cfarMode.numNoiseSamplesLeft =  cfarScndPassCfg->winLen >> 1;
            hwaParamCfg.accelModeArgs.cfarMode.outputMode = HWA_CFAR_OUTPUT_MODE_I_nAVG_ALL_Q_CUT;

            retVal = HWA_configParamSet(handle, paramSetCurrentIdx, &hwaParamCfg, NULL);
            if (retVal != 0)
            {
                goto exit;
            }

            /* Disable trigger DMA/interrupt for this param set */
            retVal = HWA_disableParamSetInterrupt(handle,
                                                  paramSetCurrentIdx,
                                                  HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
            if (retVal != 0)
            {
                goto exit;
            }
            paramSetCurrentIdx++;

            /**************************************************************/
            /* Copy range output from M2 to M0                            */
            /**************************************************************/
            memset( (void*) &hwaParamCfg, 0, sizeof(hwaParamCfg));
            hwaParamCfg.triggerMode = HWA_TRIG_MODE_IMMEDIATE;
            hwaParamCfg.dmaTriggerSrc = 0;

            hwaParamCfg.accelMode = HWA_ACCELMODE_FFT;

            hwaParamCfg.source.srcAcnt = (numRangeBins * numAzimuthBins) - 1;
            hwaParamCfg.source.srcAIdx = sizeof(DPU_SNR3DHM_CfarRawOutput);
            hwaParamCfg.source.srcBIdx = 0;

            hwaParamCfg.source.srcWidth = HWA_SAMPLES_WIDTH_32BIT;
            hwaParamCfg.source.srcScale = 0;
            hwaParamCfg.source.srcBcnt = 1 - 1;
            hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_COMPLEX;
            hwaParamCfg.source.srcSign = HWA_SAMPLES_UNSIGNED;
            hwaParamCfg.source.srcAddr = DPU_SNR3DHM_HWA_BANK_2 + pingPongInd * DPU_SNR3DHM_HWA_BANK_SIZE;

            hwaParamCfg.dest.dstAddr = DPU_SNR3DHM_HWA_BANK_0 + pingPongInd * DPU_SNR3DHM_HWA_BANK_SIZE;
            hwaParamCfg.dest.dstAcnt = (numRangeBins * numAzimuthBins) - 1;
            hwaParamCfg.dest.dstRealComplex = HWA_SAMPLES_FORMAT_COMPLEX;
            hwaParamCfg.dest.dstWidth = HWA_SAMPLES_WIDTH_32BIT;
            hwaParamCfg.dest.dstAIdx = sizeof(DPU_SNR3DHM_CfarRawOutput);
            hwaParamCfg.dest.dstBIdx = 0;
            hwaParamCfg.dest.dstScale = 8;

            hwaParamCfg.accelModeArgs.fftMode.fftEn = 0;
            hwaParamCfg.accelModeArgs.fftMode.magLogEn = HWA_FFT_MODE_MAGNITUDE_LOG2_DISABLED;
            hwaParamCfg.accelModeArgs.fftMode.fftOutMode = HWA_FFT_MODE_OUTPUT_DEFAULT;

            retVal = HWA_configParamSet(handle, paramSetCurrentIdx, &hwaParamCfg, NULL);
            if (retVal != 0)
            {
                goto exit;
            }

            /* Disable trigger DMA/interrupt for this param set */
            retVal = HWA_disableParamSetInterrupt(handle,
                                                  paramSetCurrentIdx,
                                                  HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
            if (retVal != 0)
            {
                goto exit;
            }
            paramSetCurrentIdx++;

            /**************************************************************/
            /* Transpose azimuth output from M2 to M0 + offset            */
            /**************************************************************/
            memset( (void*) &hwaParamCfg, 0, sizeof(hwaParamCfg));
            hwaParamCfg.triggerMode = HWA_TRIG_MODE_IMMEDIATE;
            hwaParamCfg.dmaTriggerSrc = 0;

            hwaParamCfg.accelMode = HWA_ACCELMODE_FFT;

            hwaParamCfg.source.srcAcnt = numAzimuthBins - 1;
            hwaParamCfg.source.srcAIdx = sizeof(DPU_SNR3DHM_CfarRawOutput);
            hwaParamCfg.source.srcBIdx = numAzimuthBins * sizeof(DPU_SNR3DHM_CfarRawOutput);

            hwaParamCfg.source.srcWidth = HWA_SAMPLES_WIDTH_32BIT;
            hwaParamCfg.source.srcScale = 0;
            hwaParamCfg.source.srcBcnt = numRangeBins - 1;
            hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_COMPLEX;
            hwaParamCfg.source.srcSign = HWA_SAMPLES_UNSIGNED;
            hwaParamCfg.source.srcAddr = DPU_SNR3DHM_HWA_BANK_2 + numRangeBins*numAzimuthBins*sizeof(DPU_SNR3DHM_CfarRawOutput) + pingPongInd * DPU_SNR3DHM_HWA_BANK_SIZE;

            hwaParamCfg.dest.dstAddr = DPU_SNR3DHM_HWA_BANK_0 + numRangeBins*numAzimuthBins*sizeof(DPU_SNR3DHM_CfarRawOutput) + pingPongInd * DPU_SNR3DHM_HWA_BANK_SIZE;
            hwaParamCfg.dest.dstAcnt = numAzimuthBins - 1;
            hwaParamCfg.dest.dstRealComplex = HWA_SAMPLES_FORMAT_COMPLEX;
            hwaParamCfg.dest.dstWidth = HWA_SAMPLES_WIDTH_32BIT;
            hwaParamCfg.dest.dstAIdx = numRangeBins * sizeof(DPU_SNR3DHM_CfarRawOutput);
            hwaParamCfg.dest.dstBIdx = sizeof(DPU_SNR3DHM_CfarRawOutput);
            hwaParamCfg.dest.dstScale = 8;

            hwaParamCfg.accelModeArgs.fftMode.fftEn = 0;
            hwaParamCfg.accelModeArgs.fftMode.magLogEn = HWA_FFT_MODE_MAGNITUDE_LOG2_DISABLED;
            hwaParamCfg.accelModeArgs.fftMode.fftOutMode = HWA_FFT_MODE_OUTPUT_DEFAULT;

            retVal = HWA_configParamSet(handle, paramSetCurrentIdx, &hwaParamCfg, NULL);
            if (retVal != 0)
            {
                goto exit;
            }

            /* Disable trigger DMA/interrupt for this param set */
            retVal = HWA_disableParamSetInterrupt(handle,
                                                  paramSetCurrentIdx,
                                                  HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
            if (retVal != 0)
            {
                goto exit;
            }
            paramSetCurrentIdx++;

            /**************************************************************/
            /* Copy range output (signal) + log2 from M0 to M2            */
            /**************************************************************/
            memset( (void*) &hwaParamCfg, 0, sizeof(hwaParamCfg));
            hwaParamCfg.triggerMode = HWA_TRIG_MODE_IMMEDIATE;
            hwaParamCfg.dmaTriggerSrc = 0;

            hwaParamCfg.accelMode = HWA_ACCELMODE_FFT;

            hwaParamCfg.source.srcAcnt = (numRangeBins * numAzimuthBins) - 1;
            hwaParamCfg.source.srcAIdx = sizeof(DPU_SNR3DHM_CfarRawOutput);
            hwaParamCfg.source.srcBIdx = 0;

            hwaParamCfg.source.srcWidth = HWA_SAMPLES_WIDTH_32BIT;
            hwaParamCfg.source.srcScale = divShiftBeforeLog;
            hwaParamCfg.source.srcBcnt = 1 - 1;
            hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_REAL;
            hwaParamCfg.source.srcSign = HWA_SAMPLES_UNSIGNED;
            hwaParamCfg.source.srcAddr = DPU_SNR3DHM_HWA_BANK_0 + pingPongInd * DPU_SNR3DHM_HWA_BANK_SIZE;

            hwaParamCfg.dest.dstAddr = DPU_SNR3DHM_HWA_BANK_2 + pingPongInd * DPU_SNR3DHM_HWA_BANK_SIZE;
            hwaParamCfg.dest.dstAcnt = (numRangeBins * numAzimuthBins) - 1;
            hwaParamCfg.dest.dstRealComplex = HWA_SAMPLES_FORMAT_REAL;
            hwaParamCfg.dest.dstWidth = HWA_SAMPLES_WIDTH_16BIT;
            hwaParamCfg.dest.dstAIdx = sizeof(uint16_t);
            hwaParamCfg.dest.dstBIdx = 0;
            hwaParamCfg.dest.dstScale = 0;

            hwaParamCfg.accelModeArgs.fftMode.fftEn = 0;
            hwaParamCfg.accelModeArgs.fftMode.magLogEn = HWA_FFT_MODE_MAGNITUDE_LOG2_ENABLED;
            hwaParamCfg.accelModeArgs.fftMode.fftOutMode = HWA_FFT_MODE_OUTPUT_DEFAULT;

            retVal = HWA_configParamSet(handle, paramSetCurrentIdx, &hwaParamCfg, NULL);
            if (retVal != 0)
            {
                goto exit;
            }

            /* Disable trigger DMA/interrupt for this param set */
            retVal = HWA_disableParamSetInterrupt(handle,
                                                  paramSetCurrentIdx,
                                                  HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
            if (retVal != 0)
            {
                goto exit;
            }
            paramSetCurrentIdx++;

            /*****************************************************************************************************************/
            /* Average noise log2{(N+n)/2}: {s0,N0},{s1,N1},... + {s0,n0},{21,n1},... -> 2-pointFFT -> {nAvg0},{nAvg1},...   */
            /*****************************************************************************************************************/
            memset( (void*) &hwaParamCfg, 0, sizeof(hwaParamCfg));
            hwaParamCfg.triggerMode = HWA_TRIG_MODE_IMMEDIATE;
            hwaParamCfg.dmaTriggerSrc = 0;

            hwaParamCfg.accelMode = HWA_ACCELMODE_FFT;

            hwaParamCfg.source.srcAcnt = 2 - 1;
            hwaParamCfg.source.srcAIdx = (numRangeBins * numAzimuthBins) * sizeof(DPU_SNR3DHM_CfarRawOutput);
            hwaParamCfg.source.srcBIdx = sizeof(DPU_SNR3DHM_CfarRawOutput);

            hwaParamCfg.source.srcWidth = HWA_SAMPLES_WIDTH_32BIT;
            hwaParamCfg.source.srcScale = divShiftBeforeLog;
            hwaParamCfg.source.srcBcnt = (numRangeBins * numAzimuthBins) - 1;
            hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_REAL;
            hwaParamCfg.source.srcSign = HWA_SAMPLES_UNSIGNED;
            hwaParamCfg.source.srcAddr = DPU_SNR3DHM_HWA_BANK_0 + sizeof(uint32_t) + pingPongInd * DPU_SNR3DHM_HWA_BANK_SIZE;

            hwaParamCfg.dest.dstAddr = DPU_SNR3DHM_HWA_BANK_2 + (numRangeBins * numAzimuthBins) * sizeof(uint16_t) + pingPongInd * DPU_SNR3DHM_HWA_BANK_SIZE;
            hwaParamCfg.dest.dstAcnt = 1 - 1; //addition
            hwaParamCfg.dest.dstSkipInit = 0; //addition
            hwaParamCfg.dest.dstRealComplex = HWA_SAMPLES_FORMAT_REAL;
            hwaParamCfg.dest.dstWidth = HWA_SAMPLES_WIDTH_16BIT;
            hwaParamCfg.dest.dstSign = HWA_SAMPLES_UNSIGNED;
            hwaParamCfg.dest.dstAIdx = sizeof(uint16_t);
            hwaParamCfg.dest.dstBIdx = sizeof(uint16_t);
            hwaParamCfg.dest.dstScale = 0;

            hwaParamCfg.accelModeArgs.fftMode.fftEn = 1;
            hwaParamCfg.accelModeArgs.fftMode.fftSize = 1;
            hwaParamCfg.accelModeArgs.fftMode.butterflyScaling = 0x1;//Average: (N+n)/2
            hwaParamCfg.accelModeArgs.fftMode.magLogEn = HWA_FFT_MODE_MAGNITUDE_LOG2_ENABLED;
            hwaParamCfg.accelModeArgs.fftMode.fftOutMode = HWA_FFT_MODE_OUTPUT_DEFAULT;

            retVal = HWA_configParamSet(handle, paramSetCurrentIdx, &hwaParamCfg, NULL);
            if (retVal != 0)
            {
                goto exit;
            }

            /* Disable trigger DMA/interrupt for this param set */
            retVal = HWA_disableParamSetInterrupt(handle,
                                                  paramSetCurrentIdx,
                                                  HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
            if (retVal != 0)
            {
                goto exit;
            }
            paramSetCurrentIdx++;

            /*******************************************************************************************/
            /* SNR= {s-nAvg}: {s0,s1,...},{nAvg0,nAvg1,...} -> 2-pointFFT -> {s0-nAvg0},{s1-nAvg1},... */
            /*******************************************************************************************/
            memset( (void*) &hwaParamCfg, 0, sizeof(hwaParamCfg));
            hwaParamCfg.triggerMode = HWA_TRIG_MODE_IMMEDIATE;
            hwaParamCfg.dmaTriggerSrc = 0;

            hwaParamCfg.accelMode = HWA_ACCELMODE_FFT;

            hwaParamCfg.source.srcAcnt = 2 - 1;
            hwaParamCfg.source.srcAIdx = (numRangeBins * numAzimuthBins) * sizeof(uint16_t);
            hwaParamCfg.source.srcBIdx = sizeof(uint16_t);

            hwaParamCfg.source.srcWidth = HWA_SAMPLES_WIDTH_16BIT;
            hwaParamCfg.source.srcScale = 8;
            hwaParamCfg.source.srcBcnt = (numRangeBins * numAzimuthBins) - 1;
            hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_REAL;
            hwaParamCfg.source.srcSign = HWA_SAMPLES_UNSIGNED;
            hwaParamCfg.source.srcAddr = DPU_SNR3DHM_HWA_BANK_2 + pingPongInd * DPU_SNR3DHM_HWA_BANK_SIZE;

            hwaParamCfg.dest.dstAddr = DPU_SNR3DHM_HWA_BANK_0 + pingPongInd * DPU_SNR3DHM_HWA_BANK_SIZE;
            hwaParamCfg.dest.dstAcnt = 2 - 1; //subtraction
            hwaParamCfg.dest.dstSkipInit = 1; //subtraction
            hwaParamCfg.dest.dstRealComplex = HWA_SAMPLES_FORMAT_REAL;
            hwaParamCfg.dest.dstWidth = HWA_SAMPLES_WIDTH_16BIT;
            hwaParamCfg.dest.dstSign = HWA_SAMPLES_SIGNED;
            hwaParamCfg.dest.dstAIdx = sizeof(uint16_t);
            hwaParamCfg.dest.dstBIdx = sizeof(uint16_t);
            hwaParamCfg.dest.dstScale = 0;

            hwaParamCfg.accelModeArgs.fftMode.fftEn = 1;
            hwaParamCfg.accelModeArgs.fftMode.fftSize = 1;
            hwaParamCfg.accelModeArgs.fftMode.butterflyScaling = 0;
            hwaParamCfg.accelModeArgs.fftMode.magLogEn = HWA_FFT_MODE_MAGNITUDE_LOG2_DISABLED;
            hwaParamCfg.accelModeArgs.fftMode.fftOutMode = HWA_FFT_MODE_OUTPUT_DEFAULT;

            retVal = HWA_configParamSet(handle, paramSetCurrentIdx, &hwaParamCfg, NULL);
            if (retVal != 0)
            {
                goto exit;
            }
            /* Disable trigger DMA/interrupt for this param set */
            retVal = HWA_disableParamSetInterrupt(handle,
                                                  paramSetCurrentIdx,
                                                  HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
            if (retVal != 0)
            {
                goto exit;
            }

            /* Enable EDMA trigger */
            retVal = HWA_getDMAChanIndex(handle,
                                         pRes->edmaHwaOut[pingPongInd].channel,
                                         &destChan);

            paramISRConfig.interruptTypeFlag = HWA_PARAMDONE_INTERRUPT_TYPE_DMA;
            paramISRConfig.dma.dstChannel = destChan;
            paramISRConfig.cpu.callbackArg = NULL;
            paramISRConfig.cpu.callbackFn = NULL;

            retVal = HWA_enableParamSetInterrupt(handle,
                                                 paramSetCurrentIdx,
                                                 &paramISRConfig);
            if (retVal != 0)
             {
                  goto exit;
             }
            paramSetCurrentIdx++;
        }//end of pingPongInd
    }//end of second pass option

    /* Save number of PARAM sets used */
    pRes->hwaCfg.numParamSet = paramSetCurrentIdx - pRes->hwaCfg.paramSetStartIdx;

    /* Save HWA Params ToDo make this as in DOA and UDOP DPUs */
    if (pRes->hwaCfg.hwaParamsSaveLoc.sizeBytes < (pRes->hwaCfg.numParamSet * HWA_NUM_REG_PER_PARAM_SET * sizeof(uint32_t)))
    {
        retVal = DPU_SNR3DHM_EHWA_PARAM_SAVE_LOC_SIZE;
        goto exit;
    }
    retVal = HWA_saveParams(handle,
                        pRes->hwaCfg.paramSetStartIdx,
                        pRes->hwaCfg.numParamSet,
                        pRes->hwaCfg.hwaParamsSaveLoc.data);

exit:
    return (retVal);
}

/**
 *  @b Description
 *  @n
 *      Configures all CFAR processing related EDMA configuration.
 *
 *
 *  \ingroup    DPU_SNR3DPROC_INTERNAL_FUNCTION
 *
 *  @param[in] hwaHandle HWA handle
 *
 *  @param[in] pRes Pointer to hardware resources
 *
 *  @param[in] staticCfg Pointer to static configuration
 *
 *  @retval EDMA error code, see EDMA API.
 */
int32_t SNR3DHM_config_EDMA
(
    snr3dhmObj *obj,
    DPU_SNR3DHM_Config       *cfarHwaCfg
)
{
    int32_t errorCode = SystemP_SUCCESS;
    DPEDMA_ChainingCfg  chainingCfg;
    DPEDMA_syncABCfg     syncACfg;

    HWA_Handle hwaHandle = obj->hwaHandle;
    DPU_SNR3DHM_HW_Resources *pRes = &cfarHwaCfg->res;
    DPU_SNR3DHM_StaticConfig  *staticCfg = &cfarHwaCfg->staticCfg;
    EDMA_Handle edmaHandle = pRes->edmaHandle;

    bool isTransferCompletionEnabled;
    Edma_EventCallback transferCompletionCallbackFxn = NULL;
    void *transferCompletionCallbackFxnArg = NULL;
    DPEDMA_syncABCfg    syncABCfg;
    int32_t pingPongInd;


    /******************************************************************************************/
    /**************                     PROGRAM EDMA INPUT                    *****************/
    /******************************************************************************************/
    for (pingPongInd = 0; pingPongInd < 2; pingPongInd++)
    {
        chainingCfg.chainingChan                  = pRes->edmaHwaInSignature[pingPongInd].channel;
        chainingCfg.isIntermediateChainingEnabled = true;
        chainingCfg.isFinalChainingEnabled        = true;

        syncACfg.srcAddress  = (uint32_t)pRes->detMatrix.data + pingPongInd * staticCfg->numRangeBins * staticCfg->azimuthFftSize * sizeof(cmplx16ImRe_t);
        syncACfg.destAddress = (uint32_t)obj->hwaMemBankAddr[pingPongInd];
        syncACfg.aCount      = staticCfg->numRangeBins * staticCfg->azimuthFftSize *  sizeof(cmplx16ImRe_t);
        syncACfg.bCount      = 1;
        syncACfg.cCount      = staticCfg->elevationFftSize / 2;
        syncACfg.srcBIdx     = 0;
        syncACfg.srcCIdx     = 2 * staticCfg->numRangeBins * staticCfg->azimuthFftSize * sizeof(cmplx16ImRe_t);
        syncACfg.dstBIdx     = 0;
        syncACfg.dstCIdx     = 0;

        errorCode = DPEDMA_configSyncAB(edmaHandle,
                                       &pRes->edmaHwaIn[pingPongInd],
                                       &chainingCfg,
                                       &syncACfg,
                                       false, //isEventTriggered
                                       false,//isIntermediateTransferInterruptEnabled
                                       false,//isTransferCompletionEnabled
                                       NULL, //transferCompletionCallbackFxn
                                       NULL, //transferCompletionCallbackFxnArg
                                       NULL);
        if (errorCode != SystemP_SUCCESS)
        {
            goto exit;
        }

        errorCode = DPEDMAHWA_configOneHotSignature(edmaHandle,
                                                    &pRes->edmaHwaInSignature[pingPongInd],
                                                    hwaHandle,
                                                    pRes->hwaCfg.dmaTriggerSource[pingPongInd],
                                                    false);
        if (errorCode != SystemP_SUCCESS)
        {
            goto exit;
        }
    }
    /******************************************************************************************/
    /**************                     PROGRAM EDMA OUTPUT                   *****************/
    /******************************************************************************************/
    //ToDo EDMA out range profile only when the GUI plot is enabled


    for (pingPongInd = 0; pingPongInd < 2; pingPongInd++)
    {
        chainingCfg.chainingChan = pRes->edmaHwaIn[pingPongInd].channel;
        chainingCfg.isIntermediateChainingEnabled = true;
        chainingCfg.isFinalChainingEnabled        = false;

        if (pingPongInd == 0)
        {
            isTransferCompletionEnabled = false;
            transferCompletionCallbackFxn = NULL;
            transferCompletionCallbackFxnArg = NULL;
        }
        else
        {
            isTransferCompletionEnabled = true;
            transferCompletionCallbackFxn = SNR3DHM_edmaDoneIsrCallback;
            transferCompletionCallbackFxnArg = (void *)&(obj->edmaDoneSemaHandle);
        }
        if (cfarHwaCfg->dynCfg.cfarScndPassCfg->enabled)
        {
            syncABCfg.srcAddress  = ((uint32_t)obj->hwaMemBankAddr[pingPongInd]);
        }
        else
        {
            syncABCfg.srcAddress  = ((uint32_t)obj->hwaMemBankAddr[2 + pingPongInd]);
        }
        syncABCfg.destAddress = ((uint32_t) pRes->snrOutMatrix.data) + pingPongInd * staticCfg->numRangeBins * staticCfg->azimuthFftSize * sizeof(int16_t);
        syncABCfg.aCount      = staticCfg->numRangeBins * staticCfg->azimuthFftSize * sizeof(int16_t);
        syncABCfg.bCount      = 1;
        syncABCfg.cCount      = staticCfg->elevationFftSize / 2;
        syncABCfg.srcBIdx     = 0;
        syncABCfg.srcCIdx     = 0;
        syncABCfg.dstBIdx     = 0;
        syncABCfg.dstCIdx     = 2 * staticCfg->numRangeBins * staticCfg->azimuthFftSize * sizeof(int16_t);

        errorCode = DPEDMA_configSyncAB(pRes->edmaHandle,
                                    &pRes->edmaHwaOut[pingPongInd],
                                    &chainingCfg,
                                    &syncABCfg,
                                    true, //isEventTriggered
                                    false,//isIntermediateTransferInterruptEnabled
                                    isTransferCompletionEnabled,
                                    transferCompletionCallbackFxn,
                                    transferCompletionCallbackFxnArg,
                                    &obj->intrObj);
        if (errorCode != SystemP_SUCCESS)
        {
            goto exit;
        }
    }

exit:
    return(errorCode);
}

/**
 *  @b Description
 *  @n
 *      Prepares HWA during run time. Configures common registers. If the HWA
 *      param sets are not reused, they can be programmed one time, and only
 *      this function has to be called during run time.
 *
 *  \ingroup    DPU_SNR3DPROC_INTERNAL_FUNCTION
 *
 *  @retval EDMA error code, see EDMA API.
 */
int32_t SNR3DHM_prepareHwaRunTime
(
    snr3dhmObj *obj
)
{
    int32_t             retVal = 0;
    HWA_CommonConfig    hwaCommonConfig;

    DPU_SNR3DHM_HW_Resources *pRes = &obj->res;



    /* Disable the HWA */
    retVal = HWA_enable(obj->hwaHandle, 0); // set 1 to enable
    if (retVal != 0)
    {
        goto exit;
    }

    /***********************/
    /* HWA COMMON CONFIG   */
    /***********************/
    /* Config Common Registers */
    hwaCommonConfig.configMask = HWA_COMMONCONFIG_MASK_NUMLOOPS |
                               HWA_COMMONCONFIG_MASK_PARAMSTARTIDX |
                               HWA_COMMONCONFIG_MASK_PARAMSTOPIDX |
                               HWA_COMMONCONFIG_MASK_FFT1DENABLE |
                               HWA_COMMONCONFIG_MASK_INTERFERENCETHRESHOLD;

    hwaCommonConfig.numLoops = obj->staticCfg.elevationFftSize / 2;
    hwaCommonConfig.paramStartIdx = pRes->hwaCfg.paramSetStartIdx;
    hwaCommonConfig.paramStopIdx =  pRes->hwaCfg.paramSetStartIdx + pRes->hwaCfg.numParamSet - 1;
    hwaCommonConfig.fftConfig.fft1DEnable = HWA_FEATURE_BIT_DISABLE;
    hwaCommonConfig.fftConfig.interferenceThreshold = 0xFFFFFF;

    retVal = HWA_configCommon(obj->hwaHandle,&hwaCommonConfig);

exit:
    return retVal;
}


#define PI_OVER_180 0.01745329252

/**
 *  @b Description
 *  @n  Saves configuration parameters to SNR3D instance
 *
 *  \ingroup    DPU_SNR3DPROC_INTERNAL_FUNCTION
 *
 *
 * @param[in]  snr3dshObj Pointer to SNR3D instance
 *
 * @param[in]  cfarHwaCfg Pointer to configuration
 *
 *
 */
void SNR3DHM_saveConfiguration(snr3dhmObj * snr3dshObj,
                               DPU_SNR3DHM_Config *cfarHwaCfg)
{
    snr3dshObj->staticCfg = cfarHwaCfg->staticCfg;
    snr3dshObj->res = cfarHwaCfg->res;
    snr3dshObj->snr3dCfg = *cfarHwaCfg->dynCfg.cfarCfg;
    snr3dshObj->cfarScndPassCfg = *cfarHwaCfg->dynCfg.cfarScndPassCfg;

    snr3dshObj->hwaParamsSaveLoc = cfarHwaCfg->res.hwaCfg.hwaParamsSaveLoc; //ToDo Cleanup this, redundant

}

/**
 *  @b Description
 *  @n  Triggers CFAR execution.
 *
 *  \ingroup    DPU_SNR3DPROC_INTERNAL_FUNCTION
 *
 *
 * @param[in]  snr3dshObj Pointer to SNR3DHM instance
 *
 */
int32_t SNR3DHM_triggerCFAR(snr3dhmObj *obj)
{
    int32_t retVal = 0;
    DPU_SNR3DHM_HW_Resources *pRes = &obj->res;
    uint32_t            baseAddr, regionId;

    baseAddr = EDMA_getBaseAddr(obj->res.edmaHandle);
    DebugP_assert(baseAddr != 0);

    regionId = EDMA_getRegionId(obj->res.edmaHandle);
    DebugP_assert(regionId < SOC_EDMA_NUM_REGIONS);

    /* Enable the HWA */
    retVal = HWA_enable(obj->hwaHandle, 1);
    if (retVal != 0)
    {
        goto exit;
    }

    /* Trigger EDMA  + CFAR */
    retVal  = EDMAEnableTransferRegion(baseAddr, regionId, pRes->edmaHwaIn[0].channel, EDMA_TRIG_MODE_MANUAL);
    if (retVal != 1)
    {
        retVal = -1;
        goto exit;
    }
    else
    {
        retVal = 0;
    }

    retVal  = EDMAEnableTransferRegion(baseAddr, regionId, pRes->edmaHwaIn[1].channel, EDMA_TRIG_MODE_MANUAL);
    if (retVal != 1)
    {
        retVal = -1;
        goto exit;
    }
    else
    {
        retVal = 0;
    }
exit:
        return retVal;
}

/**************************************************************************
 ************************ Global Variables       **********************
 **************************************************************************/
/* User defined heap memory and handle */
#define SNR3DHM_HEAP_MEM_SIZE  (sizeof(snr3dhmObj))

static uint8_t gSnr3dfftHeapMem[SNR3DHM_HEAP_MEM_SIZE] __attribute__((aligned(HeapP_BYTE_ALIGNMENT)));



DPU_SNR3DHM_Handle DPU_SNR3DHM_init
(
    DPU_SNR3DHM_InitParams *initCfg,
    int32_t* errCode
)
{
    snr3dhmObj         *obj = NULL;
    *errCode = 0;
    HWA_MemInfo        hwaMemInfo;
    int32_t i;
    int32_t status;

    if ((initCfg == NULL) || (initCfg->hwaHandle == NULL))
    {
        *errCode = DPU_SNR3DHM_EINVAL;
        goto exit;
    }

    obj = (snr3dhmObj *) &gSnr3dfftHeapMem; //MemoryP_ctrlAlloc(sizeof(snr3dhmObj), 0);
    if (obj == NULL)
    {
        *errCode = DPU_SNR3DHM_ENOMEM;
        goto exit;
    }

    /* Save for debugging */
#ifdef DBG_SNR3DHM_OBJ_DPU
        gSnr3dshObj = obj;
#endif
    printf("SNR3D DPU: (snr3dhmObj *) 0x%08x\n", (uint32_t) obj);

    /* Initialize memory */
    memset((void *)obj, 0, sizeof(snr3dhmObj));

    /* Save init config params */
    obj->hwaHandle   = initCfg->hwaHandle;

    /* Populate HWA base addresses and offsets. This is done only once, at init time.*/
    *errCode =  HWA_getHWAMemInfo(obj->hwaHandle, &hwaMemInfo);
    if (*errCode < 0)
    {
        goto exit;
    }
    for (i = 0; i < DPU_SNR3DHM_NUM_HWA_MEMBANKS; i++)
    {
        obj->hwaMemBankAddr[i] = hwaMemInfo.baseAddress + i * hwaMemInfo.bankSize;
    }


    /* Create semaphore for HWA done */
    status = SemaphoreP_constructBinary(&obj->hwaDone_semaHandle, 0);
    if(status != SystemP_SUCCESS)
    {
        *errCode = DPU_SNR3DHM_ESEMA;
        goto exit;
    }

    status = SemaphoreP_constructBinary(&obj->edmaDoneSemaHandle, 0);
    if(status != SystemP_SUCCESS)
    {
        *errCode = DPU_SNR3DHM_ESEMA;
        goto exit;
    }

exit:

    if(*errCode < 0)
    {
        if(obj != NULL)
        {
            obj = NULL;
        }
    }

    return ((DPU_SNR3DHM_Handle)obj);
}

int32_t DPU_SNR3DHM_config
(
   DPU_SNR3DHM_Handle       handle,
   DPU_SNR3DHM_Config       *snr3dCfg
)
{
    int32_t  retVal = 0;

    snr3dhmObj *snr3dshObj = (snr3dhmObj *)handle;
    DPU_SNR3DHM_StaticConfig *staticCfg = &snr3dCfg->staticCfg;

    if(snr3dshObj == NULL)
    {
       retVal = DPU_SNR3DHM_EINVAL;
       goto exit;
    }
#if 0
    if MEM_IS_NOT_ALIGN(pRes->hwaMemOutDetList,
                       DPU_SNR3DHM_HWA_MEM_OUT_RANGE_BYTE_ALIGNMENT)
    {
       retVal = DPU_SNR3DHM_ENOMEMALIGN_HWA_MEM_OUT_RANGE;
       goto exit;
    }
#endif
    /* Validate CFAR guard/noise len */
    if ((snr3dCfg->dynCfg.cfarCfg->guardLen + snr3dCfg->dynCfg.cfarCfg->winLen) * 2U >= staticCfg->numRangeBins)
    {
        retVal = DPU_SNR3DHM_EINVAL;
        goto exit;
    }

    snr3dshObj->loadHwaParamSetsBeforeExec = snr3dCfg->staticCfg.loadHwaParamSetsBeforeExec;

    /**************************************/
    /* Configure HWA                      */
    /**************************************/
    retVal = SNR3DHM_config_HWA(snr3dshObj,
                                     snr3dCfg);
    if (retVal != 0)
    {
     goto exit;
    }

    /**************************************/
    /* Configure EDMA                     */
    /**************************************/
    retVal = SNR3DHM_config_EDMA(snr3dshObj,
                                      snr3dCfg);
    if (retVal != 0)
    {
       goto exit;
    }

    SNR3DHM_saveConfiguration(snr3dshObj, snr3dCfg);

exit:
   return retVal;
}

int32_t DPU_SNR3DHM_GetNumUsedHwaParamSets
(
    DPU_SNR3DHM_Handle   handle,
    uint8_t *numUsedHwaParamSets
)
{
    int32_t retVal = 0;
    snr3dhmObj *snr3dshObj;

    if (handle == NULL)
    {
        retVal = DPU_SNR3DHM_EINVAL;
        goto exit;
    }

    snr3dshObj = (snr3dhmObj *)handle;

    *numUsedHwaParamSets = (uint8_t) snr3dshObj->res.hwaCfg.numParamSet;
exit:
    return retVal;
}


int32_t DPU_SNR3DHM_process
(
    DPU_SNR3DHM_Handle   handle,
    DPU_SNR3DHM_OutParams  *outParams
)
{
    //volatile uint32_t   startTime;
    //volatile uint32_t   startTime1;
    int32_t             retVal = 0;

    snr3dhmObj *obj;

    if (handle == NULL)
    {
        retVal = DPU_SNR3DHM_EINVAL;
        goto exit;
    }

    obj = (snr3dhmObj *)handle;

    //startTime = Cycleprofiler_getTimeStamp();

    if (obj->loadHwaParamSetsBeforeExec)
    {
        /* Load HWA param sets */
        retVal = HWA_loadParams(obj->hwaHandle,
                                obj->res.hwaCfg.paramSetStartIdx,
                                obj->res.hwaCfg.numParamSet,
                                obj->hwaParamsSaveLoc.data);
        if (retVal != 0)
        {
            goto exit;
        }
    }
    /**********************************************/
    /* ENABLE NUMLOOPS DONE INTERRUPT FROM HWA */
    /**********************************************/
    retVal = HWA_enableDoneInterrupt(obj->hwaHandle,
                                     SNR3DHM_hwaDoneIsrCallback,
                                     (void*)&obj->hwaDone_semaHandle);
    if (retVal != 0)
    {
        goto exit;
    }


    /* Prepare HWA Common Reg and CFAR threshold */
    retVal = SNR3DHM_prepareHwaRunTime (obj);
    if (retVal != 0)
    {
        goto exit;
    }

    retVal = SNR3DHM_triggerCFAR(obj);
    if (retVal != 0)
    {
        goto exit;
    }


    //startTime1 = Cycleprofiler_getTimeStamp();
    /* wait for the paramSets done interrupt */
    SemaphoreP_pend(&obj->hwaDone_semaHandle, SystemP_WAIT_FOREVER);
    /* wait for EDMA output */
    SemaphoreP_pend(&obj->edmaDoneSemaHandle, SystemP_WAIT_FOREVER);
    //waitTimeLocal += Cycleprofiler_getTimeStamp() - startTime1;

    obj->numProcess++;

    /* Disable the HWA */
    retVal = HWA_enable(obj->hwaHandle, 0);
    if (retVal != 0)
    {
        goto exit;
    }


    HWA_disableDoneInterrupt(obj->hwaHandle);
    //outParams->stats.waitTime = waitTimeLocal;
    //outParams->stats.processingTime = Cycleprofiler_getTimeStamp() - startTime - waitTimeLocal;
    outParams->stats.numProcess = obj->numProcess;
exit:
    return (retVal);
}

int32_t DPU_SNR3DHM_control
(
    DPU_SNR3DHM_Handle handle,
    DPU_SNR3DHM_Cmd cmd,
    void *arg,
    uint32_t argSize
)
{
    int32_t    retVal = 0;
    return (retVal);
}

int32_t DPU_SNR3DHM_deinit(DPU_SNR3DHM_Handle handle)
{
    int32_t retVal = 0;
    snr3dhmObj *obj;
    if (handle == NULL)
    {
        retVal = DPU_SNR3DHM_EINVAL;
        goto exit;
    }

    obj = (snr3dhmObj *)handle;

    /* Delete Semaphores */
    SemaphoreP_destruct(&obj->edmaDoneSemaHandle);
    SemaphoreP_destruct(&obj->hwaDone_semaHandle);

exit:
    return (retVal);
}
