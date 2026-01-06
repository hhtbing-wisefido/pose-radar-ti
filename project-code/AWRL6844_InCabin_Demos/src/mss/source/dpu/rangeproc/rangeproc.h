/*
 *
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
 *   @file  rangeproc.h
 *
 *   @brief
 *      Implements range processing functionality using HWA.
 */

/**************************************************************************
 *************************** Include Files ********************************
 **************************************************************************/
#ifndef RANGEPROC_H
#define RANGEPROC_H

/* Standard Include Files. */
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* mmWave SDK Driver/Common Include Files */
#include <drivers/hwa.h>

/* mmWave SDK Data Path Include Files */
#include <datapath/dpif/dpif_adcdata.h>
#include <datapath/dpif/dpif_radarcube.h>
#include <datapath/dpif/dp_error.h>
#include <datapath/dpedma/v1/dpedmahwa.h>
#include <datapath/dpedma/v1/dpedma.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define DPIF_RADARCUBE_FORMAT_6 6

/*! Number of HWA parameter sets */
#define DPU_RANGEPROC_NUM_HWA_PARAM_SETS 4U

/** @addtogroup DPU_RANGEPROC_ERROR_CODE
 *  Base error code for the rangeProc DPU is defined in the
 *  \include ti/datapath/dpif/dp_error.h
 @{ */

/**
 * @brief   Error Code: Invalid argument
 */
#define DPU_RANGEPROC_EINVAL (DP_ERRNO_RANGE_PROC_BASE - 1)

/**
 * @brief   Error Code: Out of memory
 */
#define DPU_RANGEPROC_ENOMEM (DP_ERRNO_RANGE_PROC_BASE - 2)

/**
 * @brief   Error Code: Internal error
 */
#define DPU_RANGEPROC_EINTERNAL (DP_ERRNO_RANGE_PROC_BASE - 3)

/**
 * @brief   Error Code: Not implemented
 */
#define DPU_RANGEPROC_ENOTIMPL (DP_ERRNO_RANGE_PROC_BASE - 4)

/**
 * @brief   Error Code: Not implemented
 */
#define DPU_RANGEPROC_EINPROGRESS (DP_ERRNO_RANGE_PROC_BASE - 5)

/**
 * @brief   Error Code: Invalid control command
 */
#define DPU_RANGEPROC_ECMD (DP_ERRNO_RANGE_PROC_BASE - 6)

/**
 * @brief   Error Code: Semaphore error
 */
#define DPU_RANGEPROC_ESEMA (DP_ERRNO_RANGE_PROC_BASE - 7)

/**
 * @brief   Error Code: DC range signal removal configuration error
 */
#define DPU_RANGEPROC_EDCREMOVAL (DP_ERRNO_RANGE_PROC_BASE - 8)

/**
 * @brief   Error Code: ADCBuf data interface configuration error
 */
#define DPU_RANGEPROC_EADCBUF_INTF (DP_ERRNO_RANGE_PROC_BASE - 9)

/**
 * @brief   Error Code: ADCBuf data interface configuration error
 */
#define DPU_RANGEPROC_ERADARCUBE_INTF (DP_ERRNO_RANGE_PROC_BASE - 10)

/**
 * @brief   Error Code: HWA windowing configuration error
 */
#define DPU_RANGEPROC_EWINDOW (DP_ERRNO_RANGE_PROC_BASE - 11)


    /**
    @}
    */

    /**
     * @brief
     *  RangeProc data input mode
     *
     * @details
     *  This enum defines if the rangeProc input data is from RF front end or it is in M0 but
     *  standalone from RF.
     *
     *  \ingroup DPU_RANGEPROC_EXTERNAL_DATA_STRUCTURE
     */
    typedef enum DPU_RangeProc_InputMode_e
    {
        /*! @brief     Range input is integrated with DFE input
                        ADC buffer is mapped to HWA memory
                        DMA data from ADC buffer to HWA is NOT required */
        DPU_RangeProc_InputMode_MAPPED,

        /*! @brief     Range input is integrated with DFE input
                        ADC buffer is not mapped to HWA memory,
                        DMA data from ADCBuf to HWA memory is
                        needed in range processing */
        DPU_RangeProc_InputMode_ISOLATED
    } DPU_RangeProc_InputMode;

    /**
     * @brief
     *  rangeProc control command
     *
     * @details
     *  The enum defines the rangeProc supported run time command
     *
     *  \ingroup DPU_RANGEPROC_EXTERNAL_DATA_STRUCTURE
     */
    typedef enum DPU_RangeProc_Cmd_e
    {

        /*! @brief     Command to trigger rangeProc process */
        DPU_RangeProc_Cmd_triggerProc,
    } DPU_RangeProc_Cmd;

#if 0
    /**
     * @brief
     *  Data processing Unit statistics
     *
     * @details
     *  The structure is used to hold the statistics of the DPU
     *
     *  \ingroup DPU_RANGEPROC_EXTERNAL_DATA_STRUCTURE
     */
    typedef struct DPU_RangeProc_stats_t
    {
        /*! @brief total processing time during all chirps in a frame excluding EDMA waiting time*/
        uint32_t            processingTime;

        /*! @brief total wait time for EDMA data transfer during all chirps in a frame*/
        uint32_t            waitTime;
    }DPU_RangeProc_stats;
#endif
    /**
     * @brief
     *  Structure for the HWA Params save location
     *
     *
     *  \ingroup DPU_RANGEPROC_EXTERNAL_DATA_STRUCTURE
     */
    typedef struct DPU_RangeProc_HwaParamSaveLoc_t
    {
        /*! @brief  Pointer to save location for HWA PARAM sets */
        void        *data;

        /*! @brief  Size of the save location for HWA PARAM sets in Bytes*/
        uint32_t    sizeBytes;
    } DPU_RangeProc_HwaParamSaveLoc;

    /**
     * @brief
     *  RangeProc HWA configuration
     *
     * @details
     *  The structure is used to hold the HWA configuration needed for Range FFT
     *
     *  \ingroup DPU_RANGEPROC_EXTERNAL_DATA_STRUCTURE
     */
    typedef struct DPU_RangeProc_HwaConfig_t
    {
        /*! @brief     HWA paramset Start index */
        uint8_t paramSetStartIdx;

        /*! @brief     Number of HWA param sets must be @ref DPU_RANGEPROC_NUM_HWA_PARAM_SETS */
        uint8_t numParamSet;

        /*! @brief     Flag to indicate if HWA windowing is symmetric
                        see HWA_WINDOW_SYMM definitions in HWA driver's doxygen documentation
         */
        uint8_t hwaWinSym;

        /*! @brief     HWA windowing RAM offset in number of samples */
        uint16_t hwaWinRamOffset;

        /*! @brief     Data Input Mode, */
        DPU_RangeProc_InputMode dataInputMode;

        /*! @brief HWA Params save location
           */
           DPU_RangeProc_HwaParamSaveLoc hwaParamsSaveLoc;

       /*! @brief     RangeProc HWA data input paramset dma trigger source channel */
       uint8_t        dmaTrigSrcChan[4];

    } DPU_RangeProc_HwaConfig;

    /**
     * @brief
     *  RangeProc EDMA configuration
     *
     * @details
     *  The structure is used to hold the EDMA configuration needed for Range FFT
     *
     *  \ingroup DPU_RANGEPROC_EXTERNAL_DATA_STRUCTURE
     */
    typedef struct DPU_RangeProc_EDMAInputConfig_t
    {
        /*! @brief     EDMA configuration for rangeProc data Input
                        This is needed only in @ref DPU_RangeProc_InputMode_ISOLATED
         */
        DPEDMA_ChanCfg dataIn;

        /*! @brief     EDMA configuration for rangeProc data Input Signature */
        DPEDMA_ChanCfg dataInSignature;
    } DPU_RangeProc_EDMAInputConfig;

    /**
     * @brief
     *  RangeProc EDMA configuration
     *
     * @details
     *  The structure is used to hold the EDMA configuration needed for Range FFT
     *
     *  \ingroup DPU_RANGEPROC_EXTERNAL_DATA_STRUCTURE
     */
    typedef struct DPU_RangeProc_EDMAOutputConfigFmt1_t
    {
        /*! @brief     EDMA configuration for rangeProc data Out- ping
                        It must be a HWACC triggered EDMA channel.
         */
        DPEDMA_ChanCfg dataOutPing;

        /*! @brief     EDMA configuration for rangeProc data Out- pong
                        It must be a HWACC triggered EDMA channel
         */
        DPEDMA_ChanCfg dataOutPong;
    } DPU_RangeProc_EDMAOutputConfigFmt1;

    /**
     * @brief
     *  RangeProc EDMA configuration
     *
     * @details
     *  The structure is used to hold the EDMA configuration needed for Range FFT
     *
     *  \ingroup DPU_RANGEPROC_EXTERNAL_DATA_STRUCTURE
     */
    typedef struct DPU_RangeProc_EDMAOutputConfigFmt2_t
    {
        /*! @brief     EDMA configuration for rangeProc data Out- ping
                        It must be a HWACC triggered EDMA channel
         */
        DPEDMA_3LinkChanCfg dataOutPing;
        DPEDMA_ChanCfg      dataOutPingData[3];

        /*! @brief     EDMA configuration for rangeProc data Out- pong
                        It must be a HWACC triggered EDMA channel
         */
        DPEDMA_3LinkChanCfg dataOutPong;
        DPEDMA_ChanCfg      dataOutPongData[3];
    } DPU_RangeProc_EDMAOutputConfigFmt2;

    /**
     * @brief
     *  RangeProc output EDMA configuration
     *
     * @details
     *  The structure is used to hold the EDMA configuration needed for Range FFT
     *
     *  Fmt1: Generic EDMA ping/pong output mode
     *       - 1 ping/pong EDMA channel,
     *       - 1 ping/pong HWA signature channel
     *
     *  Fmt2: Specific EDMA ping/pong output mode used ONLY for 3 TX anntenna for radar cube
     *        layout format: @ref DPIF_RADARCUBE_FORMAT_1, ADCbuf interleave mode
     *        @ref DPIF_RXCHAN_NON_INTERLEAVE_MODE
     *       - 1 ping/pong dummy EDMA channel with 3 shadow channels
             - 3 ping/pong dataOut channel
     *       - 1 ping/pong HWA signature channel
     *
     *  \ingroup DPU_RANGEPROC_EXTERNAL_DATA_STRUCTURE
     */
    typedef struct DPU_RangeProc_EDMAOutputConfig_t
    {
        /*! @brief     EDMA data output Signature */
        DPEDMA_ChanCfg dataOutSignature;

        union
        {
            /*! @brief     EDMA data output fmt1 @ref DPU_RangeProc_EDMAOutputConfigFmt1 */
            DPU_RangeProc_EDMAOutputConfigFmt1 fmt1;

            /*! @brief     EDMA data output fmt2 @ref DPU_RangeProc_EDMAOutputConfigFmt2 */
            DPU_RangeProc_EDMAOutputConfigFmt2 fmt2;
        } u;
    } DPU_RangeProc_EDMAOutputConfig;

    /**
     * @brief
     *  RangeProcHWA hardware resources
     *
     * @details
     *  The structure is used to hold the hardware resources needed for Range FFT
     *
     *  \ingroup DPU_RANGEPROC_EXTERNAL_DATA_STRUCTURE
     */
    typedef struct DPU_RangeProc_HW_Resources_t
    {
        /*! @brief     EDMA Handle */
        EDMA_Handle edmaHandle;

        /*! @brief     HWA configuration */
        DPU_RangeProc_HwaConfig hwaCfg;

        /*! @brief     EDMA configuration for rangeProc data Input */
        DPU_RangeProc_EDMAInputConfig edmaInCfg;

        /*! @brief     EDMA configuration for rangeProc data Output */
        DPU_RangeProc_EDMAOutputConfig edmaOutCfg;


        /*! @brief      Radar cube data interface */
        DPIF_RadarCube radarCube;

    } DPU_RangeProc_HW_Resources;

    /**
     * @brief
     *  RangeProcHWA static configuration
     *
     * @details
     *  The structure is used to hold the static configuraiton used by rangeProc
     *
     *  \ingroup DPU_RANGEPROC_EXTERNAL_DATA_STRUCTURE
     */
    typedef struct DPU_RangeProc_StaticConfig_t
    {

        /*! @brief  Number of transmit antennas */
        uint8_t numTxAntennas;

        /*! @brief  Number of virtual antennas */
        uint8_t numVirtualAntennas;

        /*! @brief  Number of range bins */
        uint16_t numRangeBins;

        /*! @brief  Range FFT size, this is at a minimum the next power of 2 of */
        uint16_t    rangeFftSize;

        /*! @brief  Number of frames per sliding window */
        uint16_t  numFramesPerSlidingWindow;

        /*! @brief  frame counter within sliding window */
        uint16_t frmCntrInSlidingWindowInitVal;

        /*! @brief  Number of chirps per frame */
        uint16_t numChirpsPerOneFrame;

        /*! @brief  Number of chirps per sliding window (number of chirps per radar cube) */
        uint16_t numChirpsPerSlidingWindow;

        /*! @brief  Butterfly scaling bit mask */
        uint16_t butterflyScalingBitMask;

        /*! @brief  Range FFT window coefficients, Appliation provided windows coefficients
                    After @ref DPU_RangeProc_config(), windowing buffer is not used by rangeProc DPU,
                    Hence memory can be released
         */
        int32_t *window;

        /*! @brief     Range FFT window coefficients size in bytes
                        non-symmetric window, size = sizeof(uint32_t) * numADCSamples
                        symmetric window, size = sizeof(uint32_t)*(numADCSamples round up to even number )/2
         */
        uint32_t windowSize;

        /*! @brief      ADCBuf buffer interface */
        DPIF_ADCBufData ADCBufData;

        /*! @brief     Load HWA params sets before execution */
        bool loadHwaParamSetsBeforeExec;

        /*! @brief     Prolonged (continuous) bursting mode */
        uint8_t prolonedBurstingMode;

    } DPU_RangeProc_StaticConfig;

    /**
     * @brief
     *  RangeProcHWA dynamic configuration
     *
     * @details
     *  The structure is used to hold the dynamic configuraiton used by rangeProc
     *
     *  \ingroup DPU_RANGEPROC_EXTERNAL_DATA_STRUCTURE
     */
    typedef struct DPU_RangeProc_DynamicConfig_t
    {
        /*! @brief      Pointer to Calibrate DC Range signature configuration */
        //DPU_RangeProc_CalibDcRangeSigCfg calibDcRangeSigCfg;
        uint32_t reserved;
    } DPU_RangeProc_DynamicConfig;

    /**
     * @brief
     *  Range FFT configuration
     *
     * @details
     *  The structure is used to hold the configuration needed for Range FFT
     *
     *  \ingroup DPU_RANGEPROC_EXTERNAL_DATA_STRUCTURE
     */
    typedef struct DPU_RangeProc_Config_t
    {
        /*! @brief     rangeProc hardware resources */
        DPU_RangeProc_HW_Resources hwRes;

        /*! @brief     rangeProc static configuration */
        DPU_RangeProc_StaticConfig staticCfg;

        /*! @brief     rangeProc dynamic configuration */
        DPU_RangeProc_DynamicConfig dynCfg;
    } DPU_RangeProc_Config;

    /**
     * @brief
     *  rangeProc output parameters populated during rangeProc Processing time
     *
     * @details
     *  The structure is used to hold the output parameters for rangeProc
     *
     *  \ingroup DPU_RANGEPROC_EXTERNAL_DATA_STRUCTURE
     */
    typedef struct DPU_RangeProc_InitParams_t
    {
        /*! @brief     HWA Handle */
        HWA_Handle hwaHandle;
    } DPU_RangeProc_InitParams;

    /**
     * @brief
     *  rangeProc output parameters populated during rangeProc Processing time
     *
     * @details
     *  The structure is used to hold the output parameters for rangeProc
     *
     *  \ingroup DPU_RANGEPROC_EXTERNAL_DATA_STRUCTURE
     */
    typedef struct DPU_RangeProc_OutParams_t
    {
        /*! @brief      End of Chirp indication for rangeProc */
        bool endOfChirp;

        /*! @brief     rangeProc stats */
        //DPU_RangeProc_stats stats;
    } DPU_RangeProc_OutParams;

    /**
     * @brief
     *  rangeProc DPU Handle
     *
     *  \ingroup DPU_RANGEPROC_EXTERNAL_DATA_STRUCTURE
     */
    typedef void *DPU_RangeProc_Handle;

    /*================================================================
                   rangeProc DPU exposed APIs
     ================================================================*/
    DPU_RangeProc_Handle DPU_RangeProc_init(
        DPU_RangeProc_InitParams *initParams,
        int32_t                     *errCode);

    int32_t DPU_RangeProc_config(
        DPU_RangeProc_Handle  handle,
        DPU_RangeProc_Config *rangeHwaCfg);

    int32_t DPU_RangeProc_process(
        DPU_RangeProc_Handle     handle,
        DPU_RangeProc_OutParams *outParams);

    int32_t DPU_RangeProc_control(
        DPU_RangeProc_Handle handle,
        DPU_RangeProc_Cmd    cmd,
        void                   *arg,
        uint32_t                argSize);

    int32_t DPU_RangeProc_deinit(
        DPU_RangeProc_Handle handle);

    int32_t DPU_RangeProc_GetNumUsedHwaParamSets(
        DPU_RangeProc_Handle  handle,
        uint8_t *numUsedHwaParamSets);

#ifdef __cplusplus
}
#endif

#endif
