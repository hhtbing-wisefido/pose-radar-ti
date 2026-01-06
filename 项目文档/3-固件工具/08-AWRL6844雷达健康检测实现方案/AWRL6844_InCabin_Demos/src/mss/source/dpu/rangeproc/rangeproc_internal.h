/*
 *  NOTE:
 *      (C) Copyright 2016 Texas Instruments, Inc.
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
 *   @file  rangeprochwa_internal.h
 *
 *   @brief
 *      rangeProc internal definitions.
 */

/**************************************************************************
 *************************** Include Files ********************************
 **************************************************************************/
#ifndef DPU_RANGEPROCHWA_INTERNAL_H
#define DPU_RANGEPROCHWA_INTERNAL_H

/* Standard Include Files. */
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

//#include <ti/utils/cycleprofiler/cycle_profiler.h>
#include <source/dpu/rangeproc/rangeproc.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief
     *  Rangeproc supported Radar cube layout format
     *
     * @details
     *  The enumeration describes the radar cube layout format
     *
     *  \ingroup DPU_RANGEPROC_INTERNAL_DATA_STRUCTURE
     */
    typedef enum rangeProcRadarCubeLayoutFmt_e
    {
        /*! @brief  Data layout: range-chirp-TxAnt-RxAnt */
        rangeProc_dataLayout_RANGE_CHIRP_TxAnt_RxAnt,

        /*! @brief  Data layout: chirp-TxAnt-RxAnt-range */
        rangeProc_dataLayout_CHIRP_TxAnt_RxAnt_RANGE
    } rangeProcRadarCubeLayoutFmt;

    /**
     * @brief
     *  Data path common parameters needed by RangeProc
     *
     * @details
     *  The structure is used to hold the data path parameters used by both rangeProc and rangeProdDSP DPUs.
     *
     *  \ingroup DPU_RANGEPROC_INTERNAL_DATA_STRUCTURE
     *
     */
    typedef struct rangeProc_dpParams_t
    {
        /*! @brief  Number of transmit antennas */
        uint8_t numTxAntennas;

        /*! @brief  Number of receive antennas */
        uint8_t numRxAntennas;

        /*! @brief  Number of virtual antennas */
        uint8_t numVirtualAntennas;

        /*! @brief  ADCBUF will generate chirp interrupt event every this many chirps */
        uint8_t numChirpsPerChirpEvent;

        /*! @brief  Number of ADC samples */
        uint16_t numAdcSamples;

        /*! @brief  Number of range bins */
        uint16_t numRangeBins;

        /*! @brief  Number of range bins */
        uint16_t rangeFftSize;

        /*! @brief  Number of chirps per frame */
        uint16_t numChirpsPerFrame;

        /*! @brief  Number of chirps for Doppler computation purposes. */
        uint16_t numDopplerChirps;

        /*! @brief  ADC samples format */
        DPIF_DATAFORMAT     dataFmt;  //MY_DBG

        /*! @brief  Number of frame in a sliding window (number of frames in the radra cube) */
        uint16_t numFramesPerSlidingWindow;

        /*! @brief  Number of chirps per single frame */
        uint16_t numChirpsPerOneFrame;

        /*! @brief  Number of chirps per sliding window */
        uint16_t numChirpsPerSlidingWindow;

        /*! @brief  Butterfly scaling bit mask */
        uint16_t butterflyScalingBitMask;

        /*! @brief  Initial value for the frame counter (relevant for low power mode) */
        uint16_t frmCntrInSlidingWindowInitVal;

        /*! @brief     Load HWA params sets before execution */
        bool loadHwaParamSetsBeforeExec;

        /*! @brief     Prolonged (continuous) bursting mode */
        uint8_t prolonedBurstingMode;
    } rangeProc_dpParams;

    /**
     * @brief
     *  RangeProcHWA DPU Object
     *
     * @details
     *  The structure is used to hold RangeProcHWA internal data object
     *
     *  \ingroup DPU_RANGEPROC_INTERNAL_DATA_STRUCTURE
     */
    typedef struct rangeProcObj_t
    {
        DPU_RangeProc_InitParams initParms;

        /*! @brief     Data path common parameters used in rangeProc */
        rangeProc_dpParams params;

        /*! @brief      EDMA Handle */
        EDMA_Handle edmaHandle;

        /*! @brief     RangeProc HWA configuration */
        DPU_RangeProc_HwaConfig hwaCfg;

        /*! @brief     RangeProc HWA data input paramset trigger */
        uint8_t dataInTrigger[2];

        /*! @brief     RangeProc HWA data output paramset trigger */
        uint8_t dataOutTrigger[2];

        /*! @brief     EDMA done semaphore */
        SemaphoreP_Object        edmaDoneSemaHandle;

        /*! @brief     HWA Processing Done semaphore Handle */
        SemaphoreP_Object        hwaDoneSemaHandle;

        /*! @brief      Data in interleave or non-interleave mode */
        DPIF_RXCHAN_INTERLEAVE interleave;

        /*! @brief     Rada Cube layout */
        rangeProcRadarCubeLayoutFmt radarCubeLayout;

        /*! @brief     ADC data buffer RX channel offset - fixed for all channels */
        uint16_t rxChanOffset;

        /*! @brief      Pointer to ADC buffer */
        cmplx16ImRe_t *ADCdataBuf;

        /*! @brief      Pointer to Radar Cube buffer */
        cmplx16ImRe_t *radarCubebuf;


        /*! @brief      HWA Memory address */
        uint32_t hwaMemBankAddr[4];

        /*! @brief     DMA channel trigger after HWA processing is done */
        uint8_t calibDcNumLog2AvgChirps;

        /*! @brief     DMA data out Signature channel */
        uint8_t dataOutSignatureChan;

        /*! @brief     DMA data out channels */
        uint8_t edmaDataOutChan[2];

        /*! @brief     rangeProc DPU is in processing state */
        bool inProgress;

        /*! @brief     Total number of rangeProc DPU processing */
        uint32_t numProcess;

        /*! @brief     Total number of data output EDMA done interrupt */
        uint32_t numEdmaDataOutCnt;

        /*! @brief     Frame counter modulo number of frames per sliding window */
        uint16_t frmCntrInSlidingWindow;

        /*! @brief     Address offest in the radra cube of the starting position of the next frame in Bytes */
        uint32_t radCubeFrameOffsetInBytes;

        /*! @brief     EDMA output interrupt object */
        Edma_IntrObject intrObj;

    } rangeProcObj;

#ifdef __cplusplus
}
#endif

#endif
