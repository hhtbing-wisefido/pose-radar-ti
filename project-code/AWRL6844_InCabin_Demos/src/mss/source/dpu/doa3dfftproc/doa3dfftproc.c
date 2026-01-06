/**
 *   @file  doa3dfftproc.c
 *
 *   @brief
 *      Implements 3D (range-/azimuth/elevation)  heatmap generation using FFT 
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


/* Data Path Include files */
#include <datapath/dpedma/v1/dpedmahwa.h>
#include <source/hwa_adapt/hwa_adapt.h>
#include <source/dpu/doa3dfftproc/doa3dfftprocinternal.h>

/* Flag to check input parameters */
#define DEBUG_CHECK_PARAMS   1

#define DPU_DOA3DPROC_BANK_0   HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(obj->hwaMemBankAddr[0])
#define DPU_DOA3DPROC_BANK_1   HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(obj->hwaMemBankAddr[1])
#define DPU_DOA3DPROC_BANK_2   HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(obj->hwaMemBankAddr[2])
#define DPU_DOA3DPROC_BANK_3   HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(obj->hwaMemBankAddr[3])

/* HWA ping/pong buffers offset */
#define DPU_DOA3DPROC_SRC_PING_OFFSET   HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(obj->hwaMemBankAddr[0])
#define DPU_DOA3DPROC_SRC_PONG_OFFSET   HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(obj->hwaMemBankAddr[1])
#define DPU_DOA3DPROC_DST_PING_OFFSET   HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(obj->hwaMemBankAddr[2])
#define DPU_DOA3DPROC_DST_PONG_OFFSET   HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(obj->hwaMemBankAddr[3])

#define DPU_DOA3DPROC_AZIMUTH_OUTPUT_BASE_OFFSET   DPU_DOA3DPROC_BANK_0

#define DPU_DOA3DPROC_ISK_BOARD 0
#define DPU_DOA3DPROC_ODS_BOARD 1
#define DPU_DOA3DPROC_AOP_BOARD 2

#define DPU_DOA3DPROC_MAPPING_ASYM 0
#define DPU_DOA3DPROC_MAPPING_SYM  1

#define DPU_DOA3DPROC_MAPPING_1D_1X6  0
#define DPU_DOA3DPROC_MAPPING_1D_1X4  1


/**
 * @brief   Radar cube data format ToDo should be defined in SDK
 */
#define DPU_DOA3DPROC_DPIF_RADARCUBE_FORMAT_6 6
/**
 * @brief   Detection matrix data format ToDo should be defined in SDK
 */
#define DPU_DOA3DPROC_DPIF_DETMATRIX_FORMAT_2 2

  /*! @brief   Number of input columns (doppler FFTs) in the input matrix to process */
  /*! @brief   Column offset in the input matrix */
  /*! @brief  column increment in number of columns in source matrix */
  /*! @brief   Scale in pre compensation */
  /*! @brief  column/row offset in the destination matrix dstAddrOffset = colOffset + rowOffset*numColumns */
  /*! @brief  column increment in number of columns in destination matrix */

int32_t doa3dProc_InternalLoop
(
    DPU_Doa3dProc_Obj *obj,
    DPU_Doa3dProc_OutParams *outParams
);
int32_t doa3dProc_ExternalLoop
(
    DPU_Doa3dProc_Obj *obj,
    DPU_Doa3dProc_OutParams *outParams
);

/**************************************************************************
 ************************ Global Variables       **********************
 **************************************************************************/
/* User defined heap memory and handle */
#define DOA3DFFT_HEAP_MEM_SIZE  (sizeof(DPU_Doa3dProc_Obj))

static uint8_t gDoa3dfftHeapMem[DOA3DFFT_HEAP_MEM_SIZE] __attribute__((aligned(HeapP_BYTE_ALIGNMENT)));

/*===========================================================
 *                    Internal Functions
 *===========================================================*/

/**
 *  @b Description
 *  @n
 *      HWA processing completion call back function.
 *  \ingroup    DPU_DOA_INTERNAL_FUNCTION
 */
static void doa3dProc_hwaDoneIsrCallback(void * arg)
{
    if (arg != NULL) {
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
 *  \ingroup    DPU_DOA_INTERNAL_FUNCTION
 */
static void doa3dProc_edmaDoneIsrCallback(Edma_IntrHandle intrHandle, void *arg)
{
    if (arg != NULL) {
        SemaphoreP_post((SemaphoreP_Object *)arg);
    }
}

/**
 *  @b Description
 *  @n
 *      Configures HWA for Doppler processing.
 *
 *  @param[in] obj    - DPU obj
 *  @param[in] cfg    - DPU configuration
 *
 *  \ingroup    DPU_DOA_INTERNAL_FUNCTION
 *
 *  @retval error code.
 */
int32_t doa3dProc_configHwa
(
    DPU_Doa3dProc_Obj      *obj,
    DPU_Doa3dProc_Config   *cfg
)
{
    HWA_ParamConfig         hwaParamCfg;
    HWA_InterruptConfig     paramISRConfig;
    uint32_t                paramsetIdx = 0;
    int32_t                 retVal = 0U;

    DPU_Doa3dProc_HWA_Option_Cfg * rngGateCfg;
    uint32_t                idx;

    uint16_t numOutputDopplerBins;
    uint16_t skipOutputDopplerBins;
    uint16_t numElevationBins;
    uint16_t numAzimuthBins;

    uint8_t destChan;
    uint32_t numAntRow;
    uint32_t numAntCol;

    rngGateCfg = &cfg->hwRes.hwaCfg.doaRngGateCfg;
    numAntRow = cfg->staticCfg.numAntRow;
    numAntCol = cfg->staticCfg.numAntCol;

    if (cfg->staticCfg.isStaticClutterRemovalEnabled)
    {
        numOutputDopplerBins = cfg->staticCfg.numDopplerBins - 1;
        skipOutputDopplerBins = 1;
    }
    else
    {
        numOutputDopplerBins = cfg->staticCfg.numDopplerBins;
        skipOutputDopplerBins = 0;
    }

    /* Save scaling division shift value for the Doppler non-coherent summation */
    obj->dopFftSumDiv = mathUtils_ceilLog2(cfg->staticCfg.numDopplerBins); //ToDo Tune this

    paramsetIdx = cfg->hwRes.hwaCfg.paramSetStartIdx;
    if (cfg->staticCfg.isRxChGainPhaseCompensationEnabled)
    {
        /********************************************************************************/
        /*                    Rx channel phase compensation                             */
        /********************************************************************************/
        memset((void*) &hwaParamCfg, 0, sizeof(HWA_ParamConfig));
        hwaParamCfg.triggerMode = HWA_TRIG_MODE_DMA;
        hwaParamCfg.dmaTriggerSrc = obj->hwaDmaTriggerSourceChan;

        hwaParamCfg.accelMode = HWA_ACCELMODE_FFT;
        hwaParamCfg.source.srcAddr =  DPU_DOA3DPROC_BANK_3;
        hwaParamCfg.source.srcAcnt = (cfg->staticCfg.numRxAntennas * cfg->staticCfg.numTxAntennas) -1;

        hwaParamCfg.source.srcAIdx = sizeof(cmplx16ImRe_t);
        hwaParamCfg.source.srcBcnt = cfg->staticCfg.numDopplerChirps - 1;
        hwaParamCfg.source.srcBIdx = (cfg->staticCfg.numRxAntennas * cfg->staticCfg.numTxAntennas) * sizeof(cmplx16ImRe_t);
        hwaParamCfg.source.srcShift = 0;
        hwaParamCfg.source.srcCircShiftWrap = 0;
        hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_COMPLEX;
        hwaParamCfg.source.srcWidth = HWA_SAMPLES_WIDTH_16BIT;
        hwaParamCfg.source.srcSign = HWA_SAMPLES_SIGNED;
        hwaParamCfg.source.srcConjugate = 0;
        hwaParamCfg.source.srcScale = 8;
        hwaParamCfg.source.bpmEnable = 0;
        hwaParamCfg.source.bpmPhase = 0;

        hwaParamCfg.dest.dstAddr = DPU_DOA3DPROC_BANK_0;
        hwaParamCfg.dest.dstAcnt = (cfg->staticCfg.numRxAntennas * cfg->staticCfg.numTxAntennas) - 1; //this is samples - 1
        hwaParamCfg.dest.dstAIdx = sizeof(cmplx16ImRe_t);
        hwaParamCfg.dest.dstBIdx = (cfg->staticCfg.numRxAntennas * cfg->staticCfg.numTxAntennas) * sizeof(cmplx16ImRe_t);
        hwaParamCfg.dest.dstRealComplex = HWA_SAMPLES_FORMAT_COMPLEX;
        hwaParamCfg.dest.dstWidth = HWA_SAMPLES_WIDTH_16BIT;
        hwaParamCfg.dest.dstSign = HWA_SAMPLES_SIGNED;
        hwaParamCfg.dest.dstConjugate = 0;
        hwaParamCfg.dest.dstScale = 0;

        hwaParamCfg.accelModeArgs.fftMode.fftEn = 0;
        hwaParamCfg.accelModeArgs.fftMode.magLogEn = HWA_FFT_MODE_MAGNITUDE_LOG2_DISABLED;
        hwaParamCfg.accelModeArgs.fftMode.fftOutMode = HWA_FFT_MODE_OUTPUT_DEFAULT;

        hwaParamCfg.complexMultiply.mode = HWA_COMPLEX_MULTIPLY_MODE_VECTOR_MULT;

        retVal = HWA_configParamSet(obj->hwaHandle,
                                    paramsetIdx,
                                    &hwaParamCfg, NULL);
        if (retVal != 0)
        {
            goto exit;
        }

        /* Make sure DMA interrupt/trigger is disabled for this paramset*/
        retVal = HWA_disableParamSetInterrupt(obj->hwaHandle,
                                              paramsetIdx,
                                              HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
        if (retVal != 0)
        {
            goto exit;
        }
        paramsetIdx++;
    }
    /********************************************************************************/
    /*                    Doppler FFT Configuration                                 */
    /********************************************************************************/
    for (idx = 0; idx < rngGateCfg->numDopFftParams; idx++)
    {
        memset((void*) &hwaParamCfg, 0, sizeof(HWA_ParamConfig));
        if ((idx == 0) && (!cfg->staticCfg.isRxChGainPhaseCompensationEnabled))
        {
            hwaParamCfg.triggerMode = HWA_TRIG_MODE_DMA;
            hwaParamCfg.dmaTriggerSrc = obj->hwaDmaTriggerSourceChan;
        }
        else
        {
            hwaParamCfg.triggerMode = HWA_TRIG_MODE_IMMEDIATE;
            hwaParamCfg.dmaTriggerSrc = 0;
        }
        hwaParamCfg.accelMode = HWA_ACCELMODE_FFT;
        hwaParamCfg.source.srcAddr =  DPU_DOA3DPROC_BANK_0 + rngGateCfg->dopFftCfg[idx].srcAddrOffset * sizeof(cmplx16ImRe_t);
        hwaParamCfg.source.srcAcnt = cfg->staticCfg.numDopplerChirps - 1; //size in samples - 1

        hwaParamCfg.source.srcAIdx = cfg->staticCfg.numRxAntennas * cfg->staticCfg.numTxAntennas * sizeof(cmplx16ImRe_t);
        hwaParamCfg.source.srcBcnt = rngGateCfg->dopFftCfg[idx].srcBcnt - 1;
        hwaParamCfg.source.srcBIdx = sizeof(cmplx16ImRe_t) * rngGateCfg->dopFftCfg[idx].srcBidx;
        hwaParamCfg.source.srcShift = 0;
        hwaParamCfg.source.srcCircShiftWrap = 0;
        hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_COMPLEX;
        hwaParamCfg.source.srcWidth = HWA_SAMPLES_WIDTH_16BIT;
        hwaParamCfg.source.srcSign = HWA_SAMPLES_SIGNED;
        hwaParamCfg.source.srcConjugate = 0; //no conjugate
        hwaParamCfg.source.srcScale = 8;
        hwaParamCfg.source.bpmEnable = 0;
        hwaParamCfg.source.bpmPhase = 0;

        hwaParamCfg.dest.dstAddr = DPU_DOA3DPROC_BANK_2 + rngGateCfg->dopFftCfg[idx].dstAddrOffset * sizeof(cmplx32ImRe_t);
        hwaParamCfg.dest.dstAcnt = (cfg->staticCfg.numDopplerBins) - 1; //this is samples - 1
        hwaParamCfg.dest.dstAIdx = numAntCol * sizeof(cmplx32ImRe_t);
        hwaParamCfg.dest.dstBIdx = sizeof(cmplx32ImRe_t) * rngGateCfg->dopFftCfg[idx].dstBidx;
        hwaParamCfg.dest.dstRealComplex = HWA_SAMPLES_FORMAT_COMPLEX;
        hwaParamCfg.dest.dstWidth = HWA_SAMPLES_WIDTH_32BIT;
        hwaParamCfg.dest.dstSign = HWA_SAMPLES_SIGNED;
        hwaParamCfg.dest.dstConjugate = 0; //no conjugate
        hwaParamCfg.dest.dstScale = 8;
        hwaParamCfg.dest.dstSkipInit = skipOutputDopplerBins; //Doppler zero skipped - CLUTTER RMOVAL

        hwaParamCfg.accelModeArgs.fftMode.fftEn = 1;
        hwaParamCfg.accelModeArgs.fftMode.fftSize = cfg->staticCfg.log2NumDopplerBins;

        /* scaling is disabled in all stages */
        hwaParamCfg.accelModeArgs.fftMode.butterflyScaling = (cfg->staticCfg.numDopplerBins - 1) >> 5;


        hwaParamCfg.accelModeArgs.fftMode.interfZeroOutEn = 0; //disabled
        hwaParamCfg.accelModeArgs.fftMode.windowEn = 0; //disabled
        hwaParamCfg.accelModeArgs.fftMode.windowStart = 0;
        hwaParamCfg.accelModeArgs.fftMode.winSymm = 0;
        hwaParamCfg.accelModeArgs.fftMode.winInterpolateMode = 0;
        hwaParamCfg.accelModeArgs.fftMode.magLogEn = HWA_FFT_MODE_MAGNITUDE_LOG2_DISABLED;
        hwaParamCfg.accelModeArgs.fftMode.fftOutMode = HWA_FFT_MODE_OUTPUT_DEFAULT;

        if(rngGateCfg->dopFftCfg[idx].scale == 0)
        {
            hwaParamCfg.complexMultiply.mode = HWA_COMPLEX_MULTIPLY_MODE_SCALAR_MULT;
        }
        else
        {
            hwaParamCfg.complexMultiply.mode = HWA_COMPLEX_MULTIPLY_MODE_DISABLE;
        }
        retVal = HWA_configParamSet(obj->hwaHandle,
                                    paramsetIdx,
                                    &hwaParamCfg, NULL);
        if (retVal != 0)
        {
            goto exit;
        }

        /* Make sure DMA interrupt/trigger is disabled for this paramset*/
        retVal = HWA_disableParamSetInterrupt(obj->hwaHandle,
                                              paramsetIdx,
                                              HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
        if (retVal != 0)
        {
            goto exit;
        }
        paramsetIdx++;
    }

    /***************************************************************/
    /******************* Configure Azimuth FFT *********************/
    /***************************************************************/
    memset( (void*) &hwaParamCfg, 0, sizeof(hwaParamCfg));
    hwaParamCfg.triggerMode = HWA_TRIG_MODE_IMMEDIATE;
    hwaParamCfg.dmaTriggerSrc = 0;

    hwaParamCfg.source.srcAddr = (uint16_t) (DPU_DOA3DPROC_BANK_2);
    hwaParamCfg.source.srcAcnt = numAntCol - 1;
    hwaParamCfg.source.srcAIdx = sizeof(cmplx32ImRe_t);
    hwaParamCfg.source.srcBIdx = numAntCol * sizeof(cmplx32ImRe_t);
    hwaParamCfg.source.srcBcnt = (numOutputDopplerBins * numAntRow) - 1;
    hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_COMPLEX;
    hwaParamCfg.source.srcWidth = HWA_SAMPLES_WIDTH_32BIT;
    hwaParamCfg.source.srcSign = HWA_SAMPLES_SIGNED;
    hwaParamCfg.source.srcConjugate = 0; //no conjugate
    hwaParamCfg.source.srcScale = 0;

    hwaParamCfg.complexMultiply.mode = HWA_COMPLEX_MULTIPLY_MODE_DISABLE;
    hwaParamCfg.accelModeArgs.fftMode.fftEn = 1;
    hwaParamCfg.accelModeArgs.fftMode.fftSize = mathUtils_ceilLog2(cfg->staticCfg.azimuthFftSize);//assumes power of 2;
    hwaParamCfg.accelModeArgs.fftMode.windowEn = 1; //Azimuth and elevation share FFT window. Window is 1,-1,1,-1... to achieve "fffshift" in spectral domain
    hwaParamCfg.accelModeArgs.fftMode.windowStart = cfg->hwRes.hwaCfg.winRamOffset;
    hwaParamCfg.accelModeArgs.fftMode.winSymm = cfg->hwRes.hwaCfg.winSym;
    hwaParamCfg.accelModeArgs.fftMode.butterflyScaling = 0; //ToDo Tweak this

    hwaParamCfg.dest.dstAddr =  (uint16_t) (DPU_DOA3DPROC_AZIMUTH_OUTPUT_BASE_OFFSET);
    hwaParamCfg.dest.dstAcnt = cfg->staticCfg.azimuthFftSize - 1;
    hwaParamCfg.accelModeArgs.fftMode.fftOutMode = HWA_FFT_MODE_OUTPUT_DEFAULT;
    if (cfg->staticCfg.angleDimension == 2)
    {
        /*With Elevation*/
        hwaParamCfg.accelModeArgs.fftMode.magLogEn = HWA_FFT_MODE_MAGNITUDE_LOG2_DISABLED;
        hwaParamCfg.dest.dstAIdx = sizeof(cmplx32ImRe_t);
        hwaParamCfg.dest.dstBIdx = cfg->staticCfg.azimuthFftSize * sizeof(cmplx32ImRe_t);
        hwaParamCfg.dest.dstSign = HWA_SAMPLES_SIGNED;
        hwaParamCfg.dest.dstRealComplex = HWA_SAMPLES_FORMAT_COMPLEX;
    }
    else
    {
        /*No Elevation*/
        hwaParamCfg.accelModeArgs.fftMode.magLogEn = HWA_FFT_MODE_MAGNITUDE_ONLY_ENABLED;
        hwaParamCfg.dest.dstAIdx = sizeof(uint32_t);
        hwaParamCfg.dest.dstBIdx = cfg->staticCfg.azimuthFftSize * sizeof(uint32_t);
        hwaParamCfg.dest.dstSign = HWA_SAMPLES_UNSIGNED;
        hwaParamCfg.dest.dstRealComplex = HWA_SAMPLES_FORMAT_REAL;
    }
    hwaParamCfg.dest.dstWidth = HWA_SAMPLES_WIDTH_32BIT;
    hwaParamCfg.dest.dstConjugate = 0; //no conjugate
    hwaParamCfg.dest.dstScale = 8;

    retVal = HWA_configParamSet(obj->hwaHandle, paramsetIdx, &hwaParamCfg, NULL);
    if (retVal != 0)
    {
      goto exit;
    }
    retVal = HWA_disableParamSetInterrupt(obj->hwaHandle,
                                          paramsetIdx,
                                          HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
    if (retVal != 0)
    {
      goto exit;
    }
    paramsetIdx++;



    numAzimuthBins = cfg->staticCfg.azimuthFftSize;

    if (cfg->staticCfg.angleDimension == 2)
    {
        /* [Elevation FFT]  [Doppler Max/Sum]  */
        /*****************************************************************/
        /******************* Configure Elevation FFT *********************/
        /*****************************************************************/
        memset( (void*) &hwaParamCfg, 0, sizeof(hwaParamCfg));

        hwaParamCfg.triggerMode = HWA_TRIG_MODE_IMMEDIATE;
        hwaParamCfg.dmaTriggerSrc = 0;
        hwaParamCfg.source.srcAddr = (uint16_t) DPU_DOA3DPROC_AZIMUTH_OUTPUT_BASE_OFFSET;
        hwaParamCfg.source.srcWidth = HWA_SAMPLES_WIDTH_32BIT;

        if (cfg->staticCfg.angleDimension == 2)
        {
            hwaParamCfg.source.srcAcnt = numAntRow - 1;
            hwaParamCfg.source.srcBcnt = (numAzimuthBins * numOutputDopplerBins) - 1;
            hwaParamCfg.source.srcAIdx = numAzimuthBins * numOutputDopplerBins * sizeof(cmplx32ImRe_t);
            hwaParamCfg.source.srcBIdx = sizeof(cmplx32ImRe_t);
            hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_COMPLEX;
            hwaParamCfg.source.srcSign = HWA_SAMPLES_SIGNED;
        }
        else
        {
            hwaParamCfg.source.srcAcnt = numAzimuthBins - 1;
            hwaParamCfg.source.srcBcnt = numOutputDopplerBins - 1;
            hwaParamCfg.source.srcAIdx = sizeof(uint32_t);
            hwaParamCfg.source.srcBIdx = numAzimuthBins * sizeof(uint32_t);
            hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_REAL;
            hwaParamCfg.source.srcSign = HWA_SAMPLES_UNSIGNED;
        }

        hwaParamCfg.source.srcConjugate = 0; //no conjugate
        hwaParamCfg.source.srcScale = 0;

        hwaParamCfg.complexMultiply.mode = HWA_COMPLEX_MULTIPLY_MODE_DISABLE;
        if (cfg->staticCfg.angleDimension == 2)
        {
            hwaParamCfg.accelModeArgs.fftMode.fftEn = 1;
        }
        else
        {
            hwaParamCfg.accelModeArgs.fftMode.fftEn = 0;
        }

        hwaParamCfg.accelModeArgs.fftMode.fftSize = mathUtils_ceilLog2(cfg->staticCfg.elevationFftSize);//assumes power of 2;
        hwaParamCfg.accelModeArgs.fftMode.windowEn = 1;   //Azimuth and elavation share FFT window. Window is 1,-1,1,-1... to achieve "fffshift" in spectral domain
        hwaParamCfg.accelModeArgs.fftMode.windowStart = cfg->hwRes.hwaCfg.winRamOffset;
        hwaParamCfg.accelModeArgs.fftMode.winSymm = cfg->hwRes.hwaCfg.winSym;
        hwaParamCfg.accelModeArgs.fftMode.butterflyScaling = 0; //ToDo Tweak this
        hwaParamCfg.accelModeArgs.fftMode.fftOutMode = HWA_FFT_MODE_OUTPUT_DEFAULT;
        if (cfg->staticCfg.angleDimension == 2)
        {
            hwaParamCfg.accelModeArgs.fftMode.magLogEn = HWA_FFT_MODE_MAGNITUDE_ONLY_ENABLED;
        }
        else
        {
            hwaParamCfg.accelModeArgs.fftMode.magLogEn = HWA_FFT_MODE_MAGNITUDE_ONLY_ENABLED;//HWA_FFT_MODE_MAGNITUDE_LOG2_DISABLED; //pass through
        }
        hwaParamCfg.dest.dstAddr =  (uint16_t) DPU_DOA3DPROC_BANK_2;
        if (cfg->staticCfg.angleDimension == 2)
        {
            hwaParamCfg.dest.dstAcnt = cfg->staticCfg.elevationFftSize - 1;
            hwaParamCfg.dest.dstBIdx = sizeof(uint32_t) * cfg->staticCfg.elevationFftSize;
        }
        else
        {
            hwaParamCfg.dest.dstAcnt = numAzimuthBins - 1;
            hwaParamCfg.dest.dstBIdx = numAzimuthBins * sizeof(uint32_t);
        }
        hwaParamCfg.dest.dstAIdx = sizeof(uint32_t);
        hwaParamCfg.dest.dstRealComplex = HWA_SAMPLES_FORMAT_REAL;
        hwaParamCfg.dest.dstWidth = HWA_SAMPLES_WIDTH_32BIT;
        hwaParamCfg.dest.dstConjugate = 0;  //no conjugate
        hwaParamCfg.dest.dstScale = 8;
        hwaParamCfg.dest.dstSign = HWA_SAMPLES_UNSIGNED;

        retVal = HWA_configParamSet(obj->hwaHandle, paramsetIdx, &hwaParamCfg, NULL);
        if (retVal != 0)
        {
          goto exit;
        }
        retVal = HWA_disableParamSetInterrupt(obj->hwaHandle,
                                              paramsetIdx,
                                              HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
        if (retVal != 0)
        {
          goto exit;
        }
        paramsetIdx++;
    }

    if (cfg->staticCfg.selectCoherentPeakInDopplerDim == 2)
    {
        /*****************************************************************/
        /******** Configure MAXIMUM along Doppler dimension      *********/
        /*****************************************************************/
        memset( (void*) &hwaParamCfg, 0, sizeof(hwaParamCfg));
        hwaParamCfg.triggerMode = HWA_TRIG_MODE_IMMEDIATE;
        hwaParamCfg.dmaTriggerSrc = 0;

        hwaParamCfg.source.srcAcnt = numOutputDopplerBins - 1;

        if (cfg->staticCfg.angleDimension == 2)
        {
            hwaParamCfg.source.srcAddr = (uint16_t) (DPU_DOA3DPROC_BANK_2);
            numElevationBins = cfg->staticCfg.elevationFftSize;
        }
        else
        {
            hwaParamCfg.source.srcAddr = (uint16_t) (DPU_DOA3DPROC_BANK_0);
            numElevationBins = 1;
        }
        hwaParamCfg.source.srcAIdx = numAzimuthBins * numElevationBins * sizeof(uint32_t);
        hwaParamCfg.source.srcBIdx = sizeof(uint32_t);
        hwaParamCfg.source.srcBcnt = (numAzimuthBins * numElevationBins)  - 1;
        hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_REAL;
        hwaParamCfg.source.srcWidth = HWA_SAMPLES_WIDTH_32BIT;
        hwaParamCfg.source.srcSign = HWA_SAMPLES_UNSIGNED;
        hwaParamCfg.source.srcConjugate = 0;
        hwaParamCfg.source.srcScale = 0;

        hwaParamCfg.complexMultiply.mode = HWA_COMPLEX_MULTIPLY_MODE_DISABLE;
        hwaParamCfg.accelModeArgs.fftMode.fftEn = 0;
        hwaParamCfg.accelModeArgs.fftMode.fftSize = 0;
        hwaParamCfg.accelModeArgs.fftMode.windowEn = 0;
        hwaParamCfg.accelModeArgs.fftMode.butterflyScaling = 0;
        hwaParamCfg.accelModeArgs.fftMode.fftOutMode = HWA_FFT_MODE_OUTPUT_MAX_STATS;     //coherent integration (MAX)

        if (cfg->staticCfg.angleDimension == 2)
        {
            hwaParamCfg.dest.dstAddr =  (uint16_t) DPU_DOA3DPROC_BANK_0;
        }
        else
        {
            hwaParamCfg.dest.dstAddr =  (uint16_t) DPU_DOA3DPROC_BANK_0; //ToDo Need work for angle dimension 1
        }
        hwaParamCfg.dest.dstAcnt = 4095;    //HWA user guide recommendation
        hwaParamCfg.dest.dstAIdx = 8;       //HWA user guide recommendation
        hwaParamCfg.dest.dstBIdx = 8;       //HWA user guide recommendation
        hwaParamCfg.dest.dstRealComplex = 0;//HWA user guide recommendation
        hwaParamCfg.dest.dstWidth = 1;      //HWA user guide recommendation
        hwaParamCfg.dest.dstConjugate = 0;  //no conjugate
        hwaParamCfg.dest.dstScale = 8;

        retVal = HWA_configParamSet(obj->hwaHandle, paramsetIdx, &hwaParamCfg, NULL);
        if (retVal != 0)
        {
          goto exit;
        }
        retVal = HWA_disableParamSetInterrupt(obj->hwaHandle,
                                              paramsetIdx,
                                              HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
        if (retVal != 0)
        {
          goto exit;
        }
        paramsetIdx++;
    }

    /*****************************************************************/
    /******** Configure SUM along Doppler dimension          *********/
    /*****************************************************************/
    memset( (void*) &hwaParamCfg, 0, sizeof(hwaParamCfg));
    hwaParamCfg.triggerMode = HWA_TRIG_MODE_IMMEDIATE;
    hwaParamCfg.dmaTriggerSrc = 0;

    hwaParamCfg.source.srcAcnt = numOutputDopplerBins - 1;

    if (cfg->staticCfg.angleDimension == 2)
    {
        hwaParamCfg.source.srcAddr = (uint16_t) (DPU_DOA3DPROC_BANK_2);
        numElevationBins = cfg->staticCfg.elevationFftSize;
    }
    else
    {
        hwaParamCfg.source.srcAddr = (uint16_t) (DPU_DOA3DPROC_BANK_0);
        numElevationBins = 1;
    }
    hwaParamCfg.source.srcAIdx = numAzimuthBins * numElevationBins * sizeof(uint32_t);
    hwaParamCfg.source.srcBIdx = sizeof(uint32_t);
    hwaParamCfg.source.srcBcnt = (numAzimuthBins * numElevationBins)  - 1;
    hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_REAL;
    hwaParamCfg.source.srcWidth = HWA_SAMPLES_WIDTH_32BIT;
    hwaParamCfg.source.srcSign = HWA_SAMPLES_UNSIGNED;
    hwaParamCfg.source.srcConjugate = 0;
    hwaParamCfg.source.srcScale = 0;

    hwaParamCfg.complexMultiply.mode = HWA_COMPLEX_MULTIPLY_MODE_DISABLE;
    hwaParamCfg.accelModeArgs.fftMode.fftEn = 0;
    hwaParamCfg.accelModeArgs.fftMode.fftSize = 0;
    hwaParamCfg.accelModeArgs.fftMode.windowEn = 0;
    hwaParamCfg.accelModeArgs.fftMode.butterflyScaling = 0;
    hwaParamCfg.accelModeArgs.fftMode.fftOutMode = HWA_FFT_MODE_OUTPUT_SUM_STATS;     //non-coherent integration (SUM)

    if (cfg->staticCfg.angleDimension == 2)
    {
        hwaParamCfg.dest.dstAddr =  (uint16_t) DPU_DOA3DPROC_BANK_1;
    }
    else
    {
        hwaParamCfg.dest.dstAddr =  (uint16_t) DPU_DOA3DPROC_BANK_0; //ToDo Need work...
    }
    hwaParamCfg.dest.dstAcnt = 4095;    //HWA user guide recommendation
    hwaParamCfg.dest.dstAIdx = 8;       //HWA user guide recommendation
    hwaParamCfg.dest.dstBIdx = 8;       //HWA user guide recommendation
    hwaParamCfg.dest.dstRealComplex = 0;//HWA user guide recommendation
    hwaParamCfg.dest.dstWidth = 1;      //HWA user guide recommendation
    hwaParamCfg.dest.dstConjugate = 0;  //no conjugate
    hwaParamCfg.dest.dstScale = 8;

    retVal = HWA_configParamSet(obj->hwaHandle, paramsetIdx, &hwaParamCfg, NULL);
    if (retVal != 0)
    {
      goto exit;
    }
    retVal = HWA_disableParamSetInterrupt(obj->hwaHandle,
                                          paramsetIdx,
                                          HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
    if (retVal != 0)
    {
      goto exit;
    }
    paramsetIdx++;

    if (cfg->staticCfg.selectCoherentPeakInDopplerDim == 2)
    {
        /*****************************************************************/
        /******** Configure reformating Doppler indices          *********/
        /*****************************************************************/
        memset( (void*) &hwaParamCfg, 0, sizeof(hwaParamCfg));
        hwaParamCfg.triggerMode = HWA_TRIG_MODE_IMMEDIATE;
        hwaParamCfg.dmaTriggerSrc = 0;

        hwaParamCfg.source.srcAcnt = (numAzimuthBins) - 1;

        if (cfg->staticCfg.angleDimension == 2)
        {
            hwaParamCfg.source.srcAddr = (uint16_t) (DPU_DOA3DPROC_BANK_0);
            numElevationBins = cfg->staticCfg.elevationFftSize;
        }
        else
        {
            hwaParamCfg.source.srcAddr = (uint16_t) (DPU_DOA3DPROC_BANK_0); //ToDo Need work...
            numElevationBins = 1;
        }
        hwaParamCfg.source.srcAIdx = sizeof(DPU_Doa3dProc_HwaMaxOutput) * numElevationBins;
        hwaParamCfg.source.srcBIdx = sizeof(DPU_Doa3dProc_HwaMaxOutput);
        hwaParamCfg.source.srcBcnt = numElevationBins  - 1;
        hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_REAL;
        hwaParamCfg.source.srcWidth = HWA_SAMPLES_WIDTH_16BIT;
        hwaParamCfg.source.srcSign = HWA_SAMPLES_UNSIGNED;
        hwaParamCfg.source.srcConjugate = 0;
        hwaParamCfg.source.srcScale = 8;

        hwaParamCfg.complexMultiply.mode = HWA_COMPLEX_MULTIPLY_MODE_DISABLE;
        hwaParamCfg.accelModeArgs.fftMode.fftEn = 0;
        hwaParamCfg.accelModeArgs.fftMode.fftSize = 0;
        hwaParamCfg.accelModeArgs.fftMode.windowEn = 0;
        hwaParamCfg.accelModeArgs.fftMode.butterflyScaling = 0;
        hwaParamCfg.accelModeArgs.fftMode.fftOutMode = HWA_FFT_MODE_OUTPUT_DEFAULT;

        if (cfg->staticCfg.angleDimension == 2)
        {
            hwaParamCfg.dest.dstAddr =  (uint16_t) DPU_DOA3DPROC_BANK_2;
        }
        else
        {
            hwaParamCfg.dest.dstAddr =  (uint16_t) DPU_DOA3DPROC_BANK_0; //ToDo Need work...
        }
        hwaParamCfg.dest.dstAcnt = numAzimuthBins - 1;
        hwaParamCfg.dest.dstAIdx = sizeof(uint8_t);
        hwaParamCfg.dest.dstBIdx = sizeof(uint8_t) * numAzimuthBins;
        hwaParamCfg.dest.dstRealComplex = HWA_SAMPLES_FORMAT_REAL;
        hwaParamCfg.dest.dstWidth = HWA_SAMPLES_WIDTH_16BIT;
        hwaParamCfg.dest.dstConjugate = 0;
        hwaParamCfg.dest.dstScale = 0;

        retVal = HWA_configParamSet(obj->hwaHandle, paramsetIdx, &hwaParamCfg, NULL);
        if (retVal != 0)
        {
          goto exit;
        }
        retVal = HWA_disableParamSetInterrupt(obj->hwaHandle,
                                              paramsetIdx,
                                              HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
        if (retVal != 0)
        {
          goto exit;
        }
        paramsetIdx++;
    }

    /*****************************************************************/
    /******** Configure reformating Sum                      *********/
    /*****************************************************************/
    memset( (void*) &hwaParamCfg, 0, sizeof(hwaParamCfg));
    hwaParamCfg.triggerMode = HWA_TRIG_MODE_IMMEDIATE;
    hwaParamCfg.dmaTriggerSrc = 0;

    hwaParamCfg.source.srcAcnt = (numAzimuthBins) - 1;

    if (cfg->staticCfg.angleDimension == 2)
    {
        hwaParamCfg.source.srcAddr = (uint16_t) (DPU_DOA3DPROC_BANK_1) + sizeof(uint32_t);
        numElevationBins = cfg->staticCfg.elevationFftSize;
    }
    else
    {
        hwaParamCfg.source.srcAddr = (uint16_t) (DPU_DOA3DPROC_BANK_0);  //ToDo Need work...
        numElevationBins = 1;
    }
    hwaParamCfg.source.srcAIdx = sizeof(DPU_Doa3dProc_HwaMaxOutput) * numElevationBins;
    hwaParamCfg.source.srcBIdx = sizeof(DPU_Doa3dProc_HwaMaxOutput);
    hwaParamCfg.source.srcBcnt = (numElevationBins)  - 1;
    hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_REAL;
    hwaParamCfg.source.srcWidth = HWA_SAMPLES_WIDTH_32BIT;
    hwaParamCfg.source.srcSign = HWA_SAMPLES_UNSIGNED;
    hwaParamCfg.source.srcConjugate = 0;
    hwaParamCfg.source.srcScale = 0;

    hwaParamCfg.complexMultiply.mode = HWA_COMPLEX_MULTIPLY_MODE_DISABLE;
    hwaParamCfg.accelModeArgs.fftMode.fftEn = 0;
    hwaParamCfg.accelModeArgs.fftMode.fftSize = 0;
    hwaParamCfg.accelModeArgs.fftMode.windowEn = 0;
    hwaParamCfg.accelModeArgs.fftMode.butterflyScaling = 0;
    hwaParamCfg.accelModeArgs.fftMode.fftOutMode = HWA_FFT_MODE_OUTPUT_DEFAULT;

    if (cfg->staticCfg.angleDimension == 2)
    {
        hwaParamCfg.dest.dstAddr =  (uint16_t) DPU_DOA3DPROC_BANK_3;
    }
    else
    {
        hwaParamCfg.dest.dstAddr =  (uint16_t) DPU_DOA3DPROC_BANK_0; //ToDo Need work...
    }
    hwaParamCfg.dest.dstAcnt = (numAzimuthBins)  - 1;
    hwaParamCfg.dest.dstAIdx = sizeof(uint32_t);
    hwaParamCfg.dest.dstBIdx = sizeof(uint32_t) * numAzimuthBins;
    hwaParamCfg.dest.dstRealComplex = HWA_SAMPLES_FORMAT_REAL;
    hwaParamCfg.dest.dstWidth = HWA_SAMPLES_WIDTH_32BIT;
    hwaParamCfg.dest.dstConjugate = 0;
    hwaParamCfg.dest.dstScale = 8;

    retVal = HWA_configParamSet(obj->hwaHandle, paramsetIdx, &hwaParamCfg, NULL);
    if (retVal != 0)
    {
      goto exit;
    }
    retVal = HWA_disableParamSetInterrupt(obj->hwaHandle,
                                          paramsetIdx,
                                          HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
    if (retVal != 0)
    {
      goto exit;
    }

    retVal = HWA_getDMAChanIndex(obj->hwaHandle,
                                  cfg->hwRes.edmaCfg.edmaDetMatOut.channel,
                                  &destChan);
    if (retVal != 0)
    {
     goto exit;
    }
    /* Now enable interrupt */
    paramISRConfig.interruptTypeFlag = HWA_PARAMDONE_INTERRUPT_TYPE_DMA;
    paramISRConfig.dma.dstChannel = destChan;
    paramISRConfig.cpu.callbackArg = NULL;
    retVal = HWA_enableParamSetInterrupt(obj->hwaHandle, paramsetIdx, &paramISRConfig);
    if (retVal != 0)
    {
     goto exit;
    }
    /* END: This was the last Param Set */
    obj->hwaParamStopIdx = paramsetIdx;

    /* Save HWA Params */
    if (obj->hwaParamsSaveLoc.sizeBytes < (obj->hwaParamStopIdx - obj->hwaParamStartIdx + 1)*HWA_NUM_REG_PER_PARAM_SET*sizeof(uint32_t))
    {
        retVal = DPU_DOA3DPROC_EHWA_PARAM_SAVE_LOC_SIZE;
        goto exit;
    }
    retVal = HWA_saveParams(obj->hwaHandle,
                            obj->hwaParamStartIdx,
                            obj->hwaParamStopIdx - obj->hwaParamStartIdx + 1,
                            obj->hwaParamsSaveLoc.data);
    if (retVal != 0)
    {
        goto exit;
    }

exit:
    return(retVal);
 }



volatile int gDbgStartRngBin = 0; //ToDo Remove this DBG_REMOVE
volatile int gDbgStepScaleRngBin = 1; //ToDo Remove this DBG_REMOVE
/**
 *  @b Description
 *  @n
 *  EDMA configuration.
 *
 *  @param[in] obj    - DPU obj
 *  @param[in] cfg    - DPU configuration
 *
 *  \ingroup    DPU_DOA_INTERNAL_FUNCTION
 *
 *  @retval EDMA error code, see EDMA API.
 */
static inline int32_t doa3dProc_configEdma
(
    DPU_Doa3dProc_Obj      *obj,
    DPU_Doa3dProc_Config   *cfg
)
{
    int32_t             retVal = SystemP_SUCCESS;
    cmplx16ImRe_t       *radarCubeBase = (cmplx16ImRe_t *)cfg->hwRes.radarCube.data;
    uint8_t            *detMatrixBase = (uint8_t *)cfg->hwRes.detMatrix.data;
    uint16_t            *dopIdxMatrixBase = (uint16_t *)cfg->hwRes.dopplerIndexMatrix.data;
    int16_t             sampleLenInBytes = sizeof(cmplx16ImRe_t);
    DPEDMA_ChainingCfg  chainingCfg;
    DPEDMA_syncABCfg    syncABCfg;

    bool isTransferCompletionEnabled;
    bool isIntermediateTransferInterruptEnabled;
    Edma_EventCallback transferCompletionCallbackFxn = NULL;
    void* transferCompletionCallbackFxnArg = NULL;
    uint8_t chainingLoopEdmaChannel;

    if(obj == NULL)
    {
        retVal = DPU_DOA3DPROC_EINVAL;
        goto exit;
    }

    /*****************************************************************************************/
    /**************                     PROGRAM DMA INPUT                    *****************/
    /*****************************************************************************************/
    /******************************************************************************************
    *  PROGRAM DMA channel  to transfer data from Radar cube to accelerator input buffer (PING)
    ******************************************************************************************/
    chainingCfg.chainingChan                  = cfg->hwRes.edmaCfg.edmaIn.chunk[1].channel;
    chainingCfg.isIntermediateChainingEnabled = true;
    chainingCfg.isFinalChainingEnabled        = true;

    syncABCfg.srcAddress  = (uint32_t)(&radarCubeBase[0]) + gDbgStartRngBin*sampleLenInBytes;  //ToDo DEBUG_REMOVE gDbgStartRngBin
    if (cfg->staticCfg.isRxChGainPhaseCompensationEnabled)
    {
        syncABCfg.destAddress = (uint32_t) (obj->hwaMemBankAddr[3]);
    }
    else
    {
        syncABCfg.destAddress = (uint32_t)(obj->hwaMemBankAddr[0]);
    }
    syncABCfg.aCount      = sampleLenInBytes;
    syncABCfg.bCount      = cfg->staticCfg.numRxAntennas * cfg->staticCfg.numTxAntennas * cfg->staticCfg.numDopplerChirps;
    syncABCfg.cCount      = cfg->staticCfg.numRangeBins;
    syncABCfg.srcBIdx     = cfg->staticCfg.numRangeBins * sampleLenInBytes;
    syncABCfg.srcCIdx     = sampleLenInBytes * gDbgStepScaleRngBin;
    syncABCfg.dstBIdx     = sampleLenInBytes;
    syncABCfg.dstCIdx     = 0;

    retVal = DPEDMA_configSyncAB(cfg->hwRes.edmaCfg.edmaHandle,
                                 &cfg->hwRes.edmaCfg.edmaIn.chunk[0],
                                 &chainingCfg,
                                 &syncABCfg,
                                 false,//isEventTriggered
                                 false, //isIntermediateTransferCompletionEnabled
                                 false,//isTransferCompletionEnabled
                                 NULL, //transferCompletionCallbackFxn
                                 NULL, //transferCompletionCallbackFxnArg
                                 NULL);

    if (retVal != SystemP_SUCCESS)
    {
        goto exit;
    }

    /******************************************************************************************
    *  PROGRAM DMA channel  to transfer data from Radar cube to accelerator input buffer (PONG)
    ******************************************************************************************/
    chainingCfg.chainingChan                  = cfg->hwRes.edmaCfg.edmaHotSig.channel;
    chainingCfg.isIntermediateChainingEnabled = true;
    chainingCfg.isFinalChainingEnabled        = true;

    syncABCfg.srcAddress  = (uint32_t)(&radarCubeBase[0]);
    if (cfg->staticCfg.isRxChGainPhaseCompensationEnabled)
    {
        syncABCfg.destAddress = (uint32_t) (obj->hwaMemBankAddr[3]);
    }
    else
    {
        syncABCfg.destAddress = (uint32_t)(obj->hwaMemBankAddr[0]);
    }
    syncABCfg.aCount      = sampleLenInBytes;
    syncABCfg.bCount      = 0; //This will be updated per frame with the minor motion
    syncABCfg.cCount      = cfg->staticCfg.numRangeBins;
    syncABCfg.srcBIdx     = cfg->staticCfg.numRangeBins * sampleLenInBytes;
    syncABCfg.srcCIdx     = sampleLenInBytes;
    syncABCfg.dstBIdx     = sampleLenInBytes;
    syncABCfg.dstCIdx     = 0;

    retVal = DPEDMA_configSyncAB(cfg->hwRes.edmaCfg.edmaHandle,
                                 &cfg->hwRes.edmaCfg.edmaIn.chunk[1],
                                 &chainingCfg,
                                 &syncABCfg,
                                 false,//isEventTriggered
                                 false, //isIntermediateTransferCompletionEnabled
                                 false,//isTransferCompletionEnabled
                                 NULL, //transferCompletionCallbackFxn
                                 NULL, //transferCompletionCallbackFxnArg
                                 NULL);

    if (retVal != SystemP_SUCCESS)
    {
        goto exit;
    }

    /******************************************************************************************
    *  PROGRAM DMA Hot Signature
    ******************************************************************************************/            
    retVal = DPEDMAHWA_configOneHotSignature(cfg->hwRes.edmaCfg.edmaHandle,
                                             &cfg->hwRes.edmaCfg.edmaHotSig,
                                             obj->hwaHandle,
                                             obj->hwaDmaTriggerSourceChan,
                                             false);

    if (retVal != SystemP_SUCCESS)
    {
        goto exit;
    }

    /******************************************************************************************/
    /**************                      PROGRAM DMA OUTPUT                   *****************/
    /******************************************************************************************/

    chainingLoopEdmaChannel = cfg->hwRes.edmaCfg.edmaIn.chunk[0].channel;

    /****************************************************************************************/
    /* 2D case: (SUM (2)): [Det Matrix] -> [DopplerInd Matrix]                              */
    /****************************************************************************************
     *  PROGRAM DMA channel to transfer Detection Matrix to L3
     ****************************************************************************************/



    /* Detection matrix out */
    if (cfg->staticCfg.selectCoherentPeakInDopplerDim == 2)
    {
        /* Chain to Doppler indices transfer */
        chainingCfg.chainingChan = cfg->hwRes.edmaCfg.dopIndMatOut.channel;
        chainingCfg.isFinalChainingEnabled        = true;
        chainingCfg.isIntermediateChainingEnabled = true;

        isIntermediateTransferInterruptEnabled = false;
        isTransferCompletionEnabled = false;
        transferCompletionCallbackFxn = NULL;
        transferCompletionCallbackFxnArg = NULL;
    }
    else
    {
        /* Chain back to next range gate */
        chainingCfg.chainingChan = chainingLoopEdmaChannel;
        chainingCfg.isFinalChainingEnabled        = false;
        chainingCfg.isIntermediateChainingEnabled = true;

        isIntermediateTransferInterruptEnabled = false;
        isTransferCompletionEnabled = true;
        transferCompletionCallbackFxn = doa3dProc_edmaDoneIsrCallback;
        transferCompletionCallbackFxnArg = (void *) &(obj->edmaDoneSemaHandle);
    }
    syncABCfg.srcAddress  = (uint32_t)obj->hwaMemBankAddr[3];
    syncABCfg.destAddress = (uint32_t)(detMatrixBase);
    syncABCfg.aCount      = sizeof(uint32_t) * cfg->staticCfg.azimuthFftSize;
    syncABCfg.bCount      = cfg->staticCfg.elevationFftSize;
    syncABCfg.cCount      = cfg->staticCfg.numRangeBins;
    syncABCfg.srcBIdx     = sizeof(uint32_t) * cfg->staticCfg.azimuthFftSize;
    syncABCfg.srcCIdx     = 0;

    syncABCfg.dstBIdx     = sizeof(uint32_t) * cfg->staticCfg.azimuthFftSize * cfg->staticCfg.numRangeBins;
    syncABCfg.dstCIdx     = sizeof(uint32_t) * cfg->staticCfg.azimuthFftSize;

    retVal = DPEDMA_configSyncAB(cfg->hwRes.edmaCfg.edmaHandle,
                                &cfg->hwRes.edmaCfg.edmaDetMatOut,
                                &chainingCfg,
                                &syncABCfg,
                                true, //isEventTriggered
                                isIntermediateTransferInterruptEnabled,
                                isTransferCompletionEnabled,
                                transferCompletionCallbackFxn,
                                transferCompletionCallbackFxnArg,
                                &obj->intrObj);
    if (retVal != SystemP_SUCCESS)
    {
        goto exit;
    }


    /* Valid values: 0 or 2 */
    if (cfg->staticCfg.selectCoherentPeakInDopplerDim == 2)
    {
        /*********************************************************************************************
        *  PROGRAM DMA channel  to transfer Doppler Indices to L3, and chain  back to next range gate
        *********************************************************************************************/
        chainingCfg.chainingChan = chainingLoopEdmaChannel;
        chainingCfg.isIntermediateChainingEnabled = true;
        chainingCfg.isFinalChainingEnabled        = false;

        isIntermediateTransferInterruptEnabled = false;
        isTransferCompletionEnabled = true;
        transferCompletionCallbackFxn = doa3dProc_edmaDoneIsrCallback;
        transferCompletionCallbackFxnArg = (void *)&(obj->edmaDoneSemaHandle);

        /* Transfer parameters are the same as ping, except for src/dst addresses */
        if (cfg->staticCfg.angleDimension == 2)
        {
            syncABCfg.srcAddress  = (uint32_t)obj->hwaMemBankAddr[2];
        }
        else
        {
            syncABCfg.srcAddress  = (uint32_t)obj->hwaMemBankAddr[2]; //ToDo: need work
        }
        syncABCfg.destAddress = (uint32_t)(dopIdxMatrixBase);
        syncABCfg.aCount      = sizeof(uint8_t)  * cfg->staticCfg.azimuthFftSize;;
        syncABCfg.bCount      = cfg->staticCfg.elevationFftSize;
        syncABCfg.cCount      = cfg->staticCfg.numRangeBins;
        syncABCfg.srcBIdx     = sizeof(uint8_t) * cfg->staticCfg.azimuthFftSize;
        syncABCfg.srcCIdx     = 0;

        syncABCfg.dstBIdx     = sizeof(uint8_t) * cfg->staticCfg.azimuthFftSize * cfg->staticCfg.numRangeBins;
        syncABCfg.dstCIdx     = sizeof(uint8_t) * cfg->staticCfg.azimuthFftSize;;

        retVal = DPEDMA_configSyncAB(cfg->hwRes.edmaCfg.edmaHandle,
                                    &cfg->hwRes.edmaCfg.dopIndMatOut,
                                    &chainingCfg,
                                    &syncABCfg,
                                    false, //isEventTriggered
                                    isIntermediateTransferInterruptEnabled,
                                    isTransferCompletionEnabled,
                                    transferCompletionCallbackFxn,
                                    transferCompletionCallbackFxnArg,
                                    &obj->intrObj);
        if (retVal != SystemP_SUCCESS)
        {
            goto exit;
        }
    }

exit:
    return(retVal);
} 

/*===========================================================
 *                    Doppler Proc External APIs
 *===========================================================*/

/**
 *  @b Description
 *  @n
 *      dopplerProc DPU init function. It allocates memory to store
 *  its internal data object and returns a handle if it executes successfully.
 *
 *  @param[in]   initCfg Pointer to initial configuration parameters
 *  @param[out]  errCode Pointer to errCode generates by the API
 *
 *  \ingroup    DPU_DOA3DPROC_EXTERNAL_FUNCTION
 *
 *  @retval
 *      Success     - valid handle
 *  @retval
 *      Error       - NULL
 */
DPU_Doa3dProc_Handle DPU_Doa3dProc_init
(
    DPU_Doa3dProc_InitParams *initCfg,
    int32_t                    *errCode
)
{
    DPU_Doa3dProc_Obj  *obj = NULL;
    HWA_MemInfo             hwaMemInfo;
    uint32_t                i;
    int32_t status;

    *errCode       = 0;
    
    if((initCfg == NULL) || (initCfg->hwaHandle == NULL))
    {
        *errCode = DPU_DOA3DPROC_EINVAL;
        goto exit;
    }    

    /* Allocate memory */
    obj = (DPU_Doa3dProc_Obj  *) &gDoa3dfftHeapMem;//MemoryP_ctrlAlloc(sizeof(DPU_Doa3dProc_Obj), 0U);
    if(obj == NULL)
    {
        *errCode = DPU_DOA3DPROC_ENOMEM;
        goto exit;
    }

    /* Initialize memory */
    memset((void *)obj, 0U, sizeof(DPU_Doa3dProc_Obj));
    
    printf("DOA3D DPU: (DPU_Doa3dProc_Obj *) 0x%08x\n", (uint32_t) obj);

    /* Save init config params */
    obj->hwaHandle   = initCfg->hwaHandle;

    /* Create DPU semaphores */
    status = SemaphoreP_constructBinary(&obj->edmaDoneSemaHandle, 0);
    if(status != SystemP_SUCCESS)
    {
        *errCode = DPU_DOA3DPROC_ESEMA;
        goto exit;
    }

    status = SemaphoreP_constructBinary(&obj->hwaDoneSemaHandle, 0);
    if(status != SystemP_SUCCESS)
    {
        *errCode = DPU_DOA3DPROC_ESEMA;
        goto exit;
    }
    
    /* Populate HWA base addresses and offsets. This is done only once, at init time.*/
    *errCode =  HWA_getHWAMemInfo(obj->hwaHandle, &hwaMemInfo);
    if (*errCode < 0)
    {       
        goto exit;
    }
    
    /* check if we have enough memory banks*/
    if(hwaMemInfo.numBanks < DPU_DOA3DPROC_NUM_HWA_MEMBANKS)
    {    
        *errCode = DPU_DOA3DPROC_EHWARES;
        goto exit;
    }
    
    for (i = 0; i < DPU_DOA3DPROC_NUM_HWA_MEMBANKS; i++)
    {
        obj->hwaMemBankAddr[i] = hwaMemInfo.baseAddress + i * hwaMemInfo.bankSize;
    }
    
exit:    

    if(*errCode < 0)
    {
        if(obj != NULL)
        {
            obj = NULL;
        }
    }
    return ((DPU_Doa3dProc_Handle)obj);
}

/**
  *  @b Description
  *  @n
  *   DOA3D DPU configuration
  *
  *  @param[in]   handle     DPU handle.
  *  @param[in]   cfg        Pointer to configuration parameters.
  *
  *  \ingroup    DPU_DOA3DPROC_EXTERNAL_FUNCTION
  *
  *  @retval
  *      Success      = 0
  *  @retval
  *      Error       != 0 @ref DPU_DOPPLERPROC_ERROR_CODE
  */
int32_t DPU_Doa3dProc_config
(
    DPU_Doa3dProc_Handle    handle,
    DPU_Doa3dProc_Config    *cfg
)
{
    DPU_Doa3dProc_Obj   *obj;
    int32_t                  retVal = 0;
    //uint16_t                 expectedWinSamples;


    obj = (DPU_Doa3dProc_Obj *)handle;
    if(obj == NULL)
    {
        retVal = DPU_DOA3DPROC_EINVAL;
        goto exit;
    }
    
#if DEBUG_CHECK_PARAMS
    /* Validate params */
    if(!cfg ||
       !cfg->hwRes.edmaCfg.edmaHandle ||
       !cfg->hwRes.hwaCfg.window
       //!cfg->hwRes.radarCube.data
      )
    {
        retVal = DPU_DOA3DPROC_EINVAL;
        goto exit;
    }

    /* Valid values are: 0-only data output (summed across Doppler) or 2-data output plus Doppler indices */
    if(cfg->staticCfg.selectCoherentPeakInDopplerDim == 1)
    {
        retVal = DPU_DOA3DPROC_EINVAL;
        goto exit;
    }

    /* Support only 2D */
    if(cfg->staticCfg.angleDimension == 1)
    {
        retVal = DPU_DOA3DPROC_EINVAL;
        goto exit;
    }

    if(!cfg->staticCfg.isRxChGainPhaseCompensationEnabled)
    {
        /* Rx channel compensation must be enabled */
        retVal = DPU_DOA3DPROC_EINVAL;
        goto exit;
    }

    /* Check if radar cube format is supported by DPU*/
    if(cfg->hwRes.radarCube.datafmt != DPU_DOA3DPROC_DPIF_RADARCUBE_FORMAT_6)
    {
        retVal = DPU_DOA3DPROC_ECUBEFORMAT;
        goto exit;
    }

    /* Check if radar cube column fits into one HWA memory bank */
    if((cfg->staticCfg.numTxAntennas * cfg->staticCfg.numRxAntennas * 
        cfg->staticCfg.numDopplerChirps * sizeof(cmplx16ImRe_t)) > (SOC_HWA_MEM_SIZE/SOC_HWA_NUM_MEM_BANKS))
    {
        retVal = DPU_DOA3DPROC_EEXCEEDHWAMEM;
        goto exit;
    }

    /* Check if Azimuth FFT output fits into Two HWA memory banks */
    {
        uint32_t numAntRows;
        uint32_t outputElementSizeBytes;
        numAntRows = cfg->staticCfg.numAntRow;
        if (cfg->staticCfg.angleDimension == 2)
        {
            outputElementSizeBytes = sizeof(cmplx32ImRe_t);
        }
        else
        {
            outputElementSizeBytes = sizeof(uint32_t);
        }
        if((numAntRows * cfg->staticCfg.numDopplerChirps * cfg->staticCfg.azimuthFftSize *
                outputElementSizeBytes) > 2*(SOC_HWA_MEM_SIZE/SOC_HWA_NUM_MEM_BANKS))
        {
            retVal = DPU_DOA3DPROC_EEXCEEDHWAMEM;
            goto exit;
        }
    }
    /* Check if the elevation magnitude output (uint32_t) fits into Two HWA memory banks. */
    {
        uint32_t numElevationBins;
        uint32_t numazimuthBins;
        numazimuthBins = cfg->staticCfg.azimuthFftSize;
        if (cfg->staticCfg.angleDimension == 2)
        {
            numElevationBins = cfg->staticCfg.elevationFftSize;
        }
        else
        {
            numElevationBins = 1;
        }
        if((numazimuthBins * numElevationBins *
            cfg->staticCfg.numDopplerBins * sizeof(uint32_t)) > 2*(SOC_HWA_MEM_SIZE/SOC_HWA_NUM_MEM_BANKS))
        {
            retVal = DPU_DOA3DPROC_EEXCEEDHWAMEM;
            goto exit;
        }
        /* Check if the Doppler Max/SUM param output (uint32_t) fits into one of the HWA memory bank. */
        if((numazimuthBins  * numElevationBins * sizeof(DPU_Doa3dProc_HwaMaxOutput)) > ((SOC_HWA_MEM_SIZE/SOC_HWA_NUM_MEM_BANKS)))
        {
            retVal = DPU_DOA3DPROC_EEXCEEDHWAMEM;
            goto exit;
        }
    }

    if (cfg->staticCfg.numDopplerBins > 128)
    {
        /*Currently it is limited to 128, since the doppler maximum index is stored in array of type uint8_t. Technically
         * we could allow Doppler size 256 but with clutter removal enabled, the Doppler FFT output is 255 */
        retVal = DPU_DOA3DPROC_E_EXCEEDED_MAX_NUM_DOPPLER_BINS;
        goto exit;
    }
#endif

    /* Save necessary parameters to DPU object that will be used during Process time */
    /* EDMA parameters needed to trigger first EDMA transfer*/
    obj->edmaHandle  = cfg->hwRes.edmaCfg.edmaHandle;
    memcpy((void*)(&obj->edmaIn), (void *)(&cfg->hwRes.edmaCfg.edmaIn), sizeof(DPU_Doa3dProc_Edma));
    memcpy((void*)(&obj->edmaDetMatOut), (void *)(&cfg->hwRes.edmaCfg.edmaDetMatOut), sizeof(DPEDMA_ChanCfg));
    memcpy((void*)(&obj->edmaInterLoopIn), (void *)(&cfg->hwRes.edmaCfg.edmaInterLoopIn), sizeof(DPEDMA_ChanCfg));
    
    /*HWA parameters needed for the HWA common configuration*/
    obj->hwaNumLoops      = cfg->staticCfg.numRangeBins;
    obj->hwaParamStartIdx = cfg->hwRes.hwaCfg.paramSetStartIdx;    
    obj->hwaParamsSaveLoc = cfg->hwRes.hwaCfg.hwaParamsSaveLoc; //Copy structure

    obj->loadHwaParamSetsBeforeExec =  cfg->staticCfg.loadHwaParamSetsBeforeExec;

    /* Disable the HWA */
    retVal = HWA_enable(obj->hwaHandle, 0); 
    if (retVal != 0)
    {
        goto exit;
    }
    
    /* HWA window configuration */
    retVal = HWA_configRam(obj->hwaHandle,
                           HWA_RAM_TYPE_WINDOW_RAM,
                           (uint8_t *)cfg->hwRes.hwaCfg.window,
                           cfg->hwRes.hwaCfg.windowSize, //size in bytes
                           cfg->hwRes.hwaCfg.winRamOffset * sizeof(int32_t)); 
    if (retVal != 0)
    {
        goto exit;
    }

    if(cfg->staticCfg.isRxChGainPhaseCompensationEnabled)
    {
        /* HWA window configuration */
        retVal = HWA_configRam(obj->hwaHandle,
                               HWA_RAM_TYPE_INTERNAL_RAM,
                               (uint8_t *)cfg->staticCfg.compRxChanCfg.rxChPhaseComp,
                               cfg->staticCfg.numRxAntennas * cfg->staticCfg.numTxAntennas *sizeof(cmplx32ImRe_t), //size in bytes
                               0);
        if (retVal != 0)
        {
            goto exit;
        }
    }
    
    /*******************************/
    /**  Configure HWA            **/
    /*******************************/
    /*Compute source DMA channels that will be programmed in both HWA and EDMA.   
      The DMA channels are set to be equal to the paramSetIdx used by HWA*/
    /* Ping DMA channel (Ping uses the first paramset)*/
    obj->hwaDmaTriggerSourceChan = cfg->hwRes.hwaCfg.dmaTrigSrcChan;
    retVal = doa3dProc_configHwa(obj, cfg);
    if (retVal != 0)
    {
        goto exit;
    }
                    
    /*******************************/
    /**  Configure EDMA           **/
    /*******************************/    
    retVal = doa3dProc_configEdma(obj, cfg);
    if (retVal != 0)
    {
        goto exit;
    }

exit:
    return retVal;
}

int32_t DPU_Doa3dProc_GetNumUsedHwaParamSets
(
    DPU_Doa3dProc_Handle    handle,
    uint8_t *numUsedHwaParamSets
)
{
    DPU_Doa3dProc_Obj *obj;
    int32_t retVal = 0;

    obj = (DPU_Doa3dProc_Obj *)handle;
    if (obj == NULL)
    {
        retVal = DPU_DOA3DPROC_EINVAL;
        goto exit;
    }
    *numUsedHwaParamSets = (uint8_t) (obj->hwaParamStopIdx - obj->hwaParamStartIdx + 1);
exit:
    return retVal;
}
/**
*  @b Description
*  @n DOA3D DPU process function. Computes the range/azimuth detection matrix
*
*  @param[in]   handle        DPU handle
*  @param[in]   radarCubeSrc  Structure descriptor of the input radar cube
*  @param[in]   detMatrix     Pointer to output detection matrix.
*  @param[out]  outParams     Output parameters.
*
*  \ingroup    DPU_DOA3DPROC_EXTERNAL_FUNCTION
*
*  @retval
*      Success     =0
*  @retval
*      Error      !=0
*/
int32_t DPU_Doa3dProc_process
(
    DPU_Doa3dProc_Handle    handle,
    DPU_Doa3dProc_OutParams *outParams
)
{
    //volatile uint32_t   startTime;

    DPU_Doa3dProc_Obj *obj;
    int32_t             retVal = 0;

    obj = (DPU_Doa3dProc_Obj *)handle;
    if (obj == NULL)
    {
        retVal = DPU_DOA3DPROC_EINVAL;
        goto exit;
    }
    /* Set inProgress state */
    obj->inProgress = true;

    //startTime = Cycleprofiler_getTimeStamp();

    if (obj->loadHwaParamSetsBeforeExec)
    {
        /* Load HWA Params */
        retVal = HWA_loadParams(obj->hwaHandle,
                                obj->hwaParamStartIdx,
                                obj->hwaParamStopIdx - obj->hwaParamStartIdx + 1,
                                obj->hwaParamsSaveLoc.data);
        if (retVal != 0)
        {
            goto exit;
        }
    }


    /*HWA controlled loop, HWA Internal per range processing loop */
    doa3dProc_InternalLoop(obj, outParams);
    
    outParams->stats.numProcess++;
    //outParams->stats.processingTime = Cycleprofiler_getTimeStamp() - startTime;

exit:
    if (obj != NULL)
    {
        obj->inProgress = false;
    }    
    
    return retVal;
}



/**
  *  @b Description
  *  @n
  *  Doppler DPU deinit 
  *
  *  @param[in]   handle   DPU handle.
  *
  *  \ingroup    DPU_DOA3DPROC_EXTERNAL_FUNCTION
  *
  *  @retval
  *      Success      =0
  *  @retval
  *      Error       !=0 @ref DPU_DOPPLERPROC_ERROR_CODE
  */
int32_t DPU_Doa3dProc_deinit(DPU_Doa3dProc_Handle handle)
{
    int32_t     retVal = 0;
    DPU_Doa3dProc_Obj *obj;

    /* Sanity Check */
    obj = (DPU_Doa3dProc_Obj *)handle;
    if (obj == NULL)
    {
        retVal = DPU_DOA3DPROC_EINVAL;
        goto exit;
    }

    /* Delete Semaphores */
    SemaphoreP_destruct(&obj->edmaDoneSemaHandle);
    SemaphoreP_destruct(&obj->hwaDoneSemaHandle);

exit:
    return retVal;
}

/**
*  @b Description
*  @n DOA3D DPU process function using internal HWA loop
*
*  @param[in]   obj        DPU object
*  @param[out]  outParams     Output parameters.
*
*  \ingroup    DPU_DOA3DPROC_INTERNAL_FUNCTION
*
*  @retval
*      Success     =0
*  @retval
*      Error      !=0
*/
int32_t doa3dProc_InternalLoop
(
        DPU_Doa3dProc_Obj * obj,
        DPU_Doa3dProc_OutParams *outParams
)
{

    int32_t             retVal = 0;
    bool                status;
    HWA_CommonConfig    hwaCommonConfig;

    uint32_t            baseAddr, regionId;

    baseAddr = EDMA_getBaseAddr(obj->edmaHandle);
    DebugP_assert(baseAddr != 0);

    regionId = EDMA_getRegionId(obj->edmaHandle);
    DebugP_assert(regionId < SOC_EDMA_NUM_REGIONS);

    /**********************************************/
    /* ENABLE NUMLOOPS DONE INTERRUPT FROM HWA */
    /**********************************************/
    retVal = HWA_enableDoneInterrupt(obj->hwaHandle,
                                       doa3dProc_hwaDoneIsrCallback,
                                       (void*)&obj->hwaDoneSemaHandle);
    if (retVal != 0)
    {
        goto exit;
    }

    /***********************/
    /* HWA COMMON CONFIG   */
    /***********************/
    memset((void*) &hwaCommonConfig, 0, sizeof(HWA_CommonConfig));

    /* Config Common Registers */
    hwaCommonConfig.configMask =
        HWA_COMMONCONFIG_MASK_NUMLOOPS |
        HWA_COMMONCONFIG_MASK_PARAMSTARTIDX |
        HWA_COMMONCONFIG_MASK_PARAMSTOPIDX |
        HWA_COMMONCONFIG_MASK_FFT1DENABLE |
        HWA_COMMONCONFIG_MASK_INTERFERENCETHRESHOLD |
        HWA_COMMONCONFIG_MASK_I_CMULT_SCALE |
        HWA_COMMONCONFIG_MASK_Q_CMULT_SCALE |
        HWA_COMMONCONFIG_MASK_FFTSUMDIV;

    hwaCommonConfig.numLoops      = obj->hwaNumLoops;
    hwaCommonConfig.paramStartIdx = obj->hwaParamStartIdx;
    hwaCommonConfig.paramStopIdx  = obj->hwaParamStopIdx;
    hwaCommonConfig.fftConfig.fft1DEnable = HWA_FEATURE_BIT_DISABLE;
    hwaCommonConfig.fftConfig.interferenceThreshold = 0xFFFFFF;

    hwaCommonConfig.fftConfig.fftSumDiv = obj->dopFftSumDiv;

    hwaCommonConfig.scalarMult.i_cmult_scale[0] = 0; //ToDo: set all scale values to zero
    hwaCommonConfig.scalarMult.q_cmult_scale[0] = 0;



    retVal = HWA_configCommon(obj->hwaHandle, &hwaCommonConfig);
    if (retVal != 0)
    {
        goto exit;
    }

    /* Enable the HWA */
    retVal = HWA_enable(obj->hwaHandle,1);
    if (retVal != 0)
    {
        goto exit;
    }

    EDMAEnableTransferRegion(baseAddr, regionId, obj->edmaIn.chunk[0].channel, EDMA_TRIG_MODE_MANUAL);

    /**********************************************/
    /* WAIT FOR HWA NUMLOOPS INTERRUPT            */
    /**********************************************/
    status = SemaphoreP_pend(&obj->hwaDoneSemaHandle, SystemP_WAIT_FOREVER);

    if (status != SystemP_SUCCESS)
    {
        retVal = DPU_DOA3DPROC_ESEMASTATUS;
        goto exit;
    }

    HWA_disableDoneInterrupt(obj->hwaHandle);

    /* Disable the HWA */
    retVal = HWA_enable(obj->hwaHandle, 0);
    if (retVal != 0)
    {
        goto exit;
    }

    /**********************************************/
    /* WAIT FOR EDMA DONE INTERRUPT            */
    /**********************************************/
    status = SemaphoreP_pend(&obj->edmaDoneSemaHandle, SystemP_WAIT_FOREVER);
    if (status != SystemP_SUCCESS)
    {
        retVal = DPU_DOA3DPROC_ESEMASTATUS;
        goto exit;
    }
exit:
    return retVal;
}
