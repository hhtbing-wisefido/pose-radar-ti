/**
 *   @file  dpc_common.h
 *
 *   @brief
 *      Object Detection DPC (DSP chain) Header File
 *
 *  \par
 *  NOTE:
 *      (C) Copyright 2019 Texas Instruments, Inc.
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

/** @mainpage Object Detection Data-path Processing Chain (DPC) with DSP based DPUs
 * [TOC]
 *  @section objdetdsp_intro Overview
 *
 *  The Object detection DPC provides the functionality of processing ADC samples
 *  to detect objects during the frame acquisition and inter-frame processing
 *  periods. It can be used by an application by registering with the DPM
 *  framework and invoked using DPM APIs. The external interface of Object detection
 *  DPC can be seen at @ref DPC_OBJDET_EXTERNAL
 *
 *   @image html object_detection_datapath.png "Object Detection Data Path Processing Chain"
 *   \n
 *   \n
 *   This data path chain processing consists of:
 *   - Processing during the chirps as seen in the timing diagram(optional):
 *     - This consists of 1D (range) FFT processing that takes input from multiple
 *     receive antennae from the ADC buffer for every chirp (corresponding to the
 *     chirping pattern on the transmit antennae)
 *     and performs FFT on it and generates output into the L3 RAM in the format
 *     defined by @ref DPIF_RADARCUBE_FORMAT_1. For more details, see the doxygen
 *     documentation of range processing DPU (Data Path Unit) located at:
 *       @verbatim
          ti/datapath/dpu/rangeproc/docs/doxygen/html/index.html
         @endverbatim
 *   - Processing during the time between the end of chirps until the beginning of the
 *     next chirping period, shown as "Inter frame Period" in the timing diagram.
 *     This processing consists of:
 *     - Doppler processing that takes input from 1D output in L3 RAM and performs
 *       2D FFT to give range-velocity detection matrix in the L3 RAM. For more details, see:
 *       @verbatim
          ti/datapath/dpc/dpu/dopplerproc/docs/doxygen/html/index.html
         @endverbatim
 *     - CFAR processing and peak grouping on detection matrix output of doppler processing.
 *       For more details, see:
 *       @verbatim
          ti/datapath/dpc/dpu/cfarcaproc/docs/doxygen/html/index.html
         @endverbatim
 *     - Angle (Azimuth, Elevation) of Arrival processing to produce a final list of
 *       detected objects with position coordinates (x,y,z) and velocity.
 *       For more details, see:
 *       @verbatim
          ti/datapath/dpc/dpu/aoaproc/docs/doxygen/html/index.html
         @endverbatim
 *
 *  @section objdetdsp_appdpcFlow Application-DPC Execution Flow
 *   Following diagram shows the application-DPC execution Flow.
 *
 *   The flow above shows the sequencing of initialization, configuration, execution and
 *   dynamic control operations of the DPC and some level of detail of
 *   what happens under these operations.
 *   Most of the hardware resource (e.g EDMA related) configuration
 *   for the DPUs that is issued by the DPC as part of processing
 *   @ref DPC_OBJDET_IOCTL__STATIC_PRE_START_CFG commands is provided by the
 *   application at build time using a resource file (DPC sources are built
 *   as part of building the application, there is no separate DPC library object).
 *   This file is passed as a compiler command line define
 *   @verbatim --define=APP_RESOURCE_FILE="fileName" @endverbatim The "fileName"
 *   above includes the path as if to include the file when building the the DPC sources
 *   as part of building the application, and any DPC source that needs to refer
 *   to this file (currently objectdetection.c) has the following code
 *    @verbatim #include APP_RESOURCE_FILE @endverbatim
 *   One of the demos that uses this DPC is located at ti/demo/xwr16xx/mmw. The
 *   resource file in this demo is mmw_res.h, this file shows all the definitions
 *   that are needed by the DPC from the application. This file is provided
 *   on compiler command line when building as follows:
 *   @verbatim --define=APP_RESOURCE_FILE="<ti/demo/xwr16xx/mmw/mmw_res.h>" @endverbatim
 *
 *   As seen in application execution flow, 2 options are given in objdetdsp DPC chain.
 *   - **Full DPC chain** with Range DPU, Doppler DPU , CFARCA DPU and AoA DPU.
 *        This is the default configuration. DPC accepts frameStart and chirpEvent(trigger DPC execution) from DPM. All DPUs
 *        will be excuted during DPC execution(@ref DPC_ObjectDetection_execute).
 *
 *   - **DPC without Range DPU**
 *        This option is enabled by compiler option defined as follows:
 *        @verbatim --define=OBJDET_NO_RANGE @endverbatim
 *        DPC accepts frameStart and data injection(trigger DPC execution) from DPM. All doppler DPU, cfarca DPU and AoA DPU will be
 *        executed during DPC execution(@ref DPC_ObjectDetection_execute).
 *
 *  @section objdetdsp_memory Data Memory
 *
 *  @subsection objdetdsp_memCfg Memory configuration
 *   The configuration of L3 and Core Local L2 and L1 RAM (hitherto referred in short as LRAM)
 *   memories are provided by the application
 *      - @ref DPC_ObjectDetection_InitParams_t::L3ramCfg,
 *      - @ref DPC_ObjectDetection_InitParams_t::CoreL1RamCfg and
 *      - @ref DPC_ObjectDetection_InitParams_t::CoreL2RamCfg
 *
 *   during @ref DPC_ObjectDetection_init (invoked by application through @ref DPM_init).
 *   This configuration is the default memory configuration for the DPC.
 *
 *   This DPC can also accept share memory(L3 RAM) configuraiton during processing of
 *   @ref DPC_OBJDET_IOCTL__STATIC_PRE_START_CFG command. This is used for option 2 of execution flow when range DPU is
 *   disabled and radarCube memory is allocated outside of this DPC.
 *
 *   @subsection objdetdsp_memPar Memory partition
 *   The L3 and LRAM partition happens during the processing of @ref DPC_OBJDET_IOCTL__STATIC_PRE_START_CFG command
 *   and is shown in the following figure. The allocation from application system heap
 *   (typically in LRAM) using the MemoryP_osal API is done during @ref DPC_ObjectDetection_init
 *   (object instances of DPC and DPUs for all sub-frames) and during the
 *   processing of @ref DPC_OBJDET_IOCTL__STATIC_PRE_START_COMMON_CFG command (range DPUs
 *   dc antenna coupling signature buffer that is unique for each sub-frame)
 *   is also shown in the figure.
 *
 *
 *   @image html memory_allocation.png "Data memory allocation"
 *
 *   In the above picture, the L2 RAM shows allocation
 *   of the "cfarRngDopSnrList" (@ref DPIF_CFARDetList_t) outside of scratch usage
 *   as this is shared buffer between CFAR and AoA in the processing flow and
 *   therefore needs to persist within the sub-frame until AoA is executed at the
 *   end of the processing chain.
 *
 *   The buffers labeled "windowBuffer" and "twiddle Buffer" in the picture
 *   for range, doppler and AoA DPUs are allocated/generated during DPU configue time.
 *   AoA DPU needs the same "windowBuffer" and "twiddle Buffer" as used in doppler DPU to recompute the 2D
 *   doppler FFT, hence the same 2D windowing and twiddle buffers are provided to AoA DPU as well.
 *
 *   In DPC, these "windowBuffer" and "twiddle buffer" are allocated in persistent memory(not overlapping with scratch
 *   buffers). This arrangement in memory makes the 2D window/twiddle sharable between Doppler and AoA DPUs.
 *   It also prevents window/twiddle buffer re-generation across frame boundary in non-advanced
 *   frame scenarios.
 *
 *   The AoA DPU API has been designed to require 2D-FFT window and twiddle buffer configuration
 *   (i.e configuration is not optional) because it may be used in contexts (unit test, other DPC flavors) where doppler
 *   processing may not exist.
 *   The AoA interface buffers are consumed by the application at the end of the
 *   DPC execute API.
 *
 *
 *   @subsection objdetdsp_reconfig DPU reconfiguration
 *   DPU reconfiguration is related to data path processing across sub-frames for advanced frame configuraiton.
 *   In such cases, reconfiguration is required when switching sub-frames because scratch buffers and EDMA resources
 *   are overlapped across sub-frames. Note that the DPU's xxx_config API
 *   is a full configuration API beyond the EDMA resources configuration (e.g static
 *   and dynamic configuration) so restricting to the full configuration
 *   would imply that no sub-frame specific DPU instantiation is necessary. However, the code illustrates separate
 *   instances of DPUs for each sub-frame to demonstrate generality of the sub-frame solution,
 *   in the case where there may be specialized (partial) configuration APIs in an
 *   optimized implementation (that only configured the overlapped EDMA resources).
 *   The limiting to full configuration also means that all the code required to build the
 *   configuration structures of the DPUs during the pre-start config time either
 *   has to be repeated or alternatively, the configurations that were created
 *   during the pre-start config processing be saved and reused later.
 *   The latter path has been taken, all DPU configuration that is built
 *   during the pre-start processing is stored in separate storage, this can be
 *   located at @ref SubFrameObj_t::dpuCfg. However, parts of this reconfiguration that
 *   cannot be captured in this storage need to be repeated, namely, window generation
 *   (@ref DPC_ObjDetDSP_GenRangeWindow) and rx phase compensation
 *   (@ref DPC_ObjDetDSP_GetRxChPhaseComp).
 *
 *   The DPU top-level dynamic configuration structure contains
 *   pointers to the individual configurations (e.g see @ref DPU_AoAProc_DynamicConfig_t)
 *   so DPC stores the dynamic configuration in pre-start config (@ref DPC_ObjectDetection_DynCfg_t)
 *   in permanent storage (@ref SubFrameObj_t::dynCfg) and DPUs are passed from this storage area
 *   so that their pointers point to this permanent storage during reconfiguration.
 */
#ifndef DPC_COMMON_H
#define DPC_COMMON_H

/* MMWAVE Driver Include Files */
#include <common/mmwave_error.h>

#include <datapath/dpif/dpif_pointcloud.h>
#include <datapath/dpif/dpif_radarcube.h>
#include <datapath/dpif/dpif_detmatrix.h>


#define DEMO_RL_MAX_SUBFRAMES 1

#ifdef __cplusplus
extern "C"
{
#endif

    /*
     * @brief Capon BF based AoA configuration
     *
     */
    typedef struct DPC_ObjectDetection_aoaCfg_t
    {
        /*! @brief   Subframe number for which this message is applicable. When
         *           advanced frame is not used, this should be set to
         *           0 (the 1st and only sub-frame) */
        uint8_t subFrameNum;

        /*! @brief    Configuration for Capon BF based AoA */
        DPU_radarProcessConfig_t cfg;
    } DPC_ObjectDetection_aoaCfg;

    /*!
     *  @brief      Call back function type for calling back during process
     *  @param[out] subFrameIndx Sub-frame indx [0..(numSubFrames-1)]
     */
    typedef void (*DPC_ObjectDetection_processCallBackFxn_t)(uint8_t subFrameIndx);

    /*! @brief  Process call backs configuration */
    typedef struct DPC_ObjectDetection_ProcessCallBackCfg_t
    {
        /*! @brief  Call back function that will be called at the beginning of frame
         *          processing (beginning of 1D) */
        DPC_ObjectDetection_processCallBackFxn_t processFrameBeginCallBackFxn;

        /*! @brief  Call back function that will be called at the beginning of inter-frame
         *          processing (beginning of 2D) */
        DPC_ObjectDetection_processCallBackFxn_t processInterFrameBeginCallBackFxn;
    } DPC_ObjectDetection_ProcessCallBackCfg;

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
     * @brief Configuration related to share memory allocation at run-time.
     *      These configuration should overwrite init time configuration once enabled.
     *      It is used to share memory across DPCs
     */
    typedef struct DPC_ObjectDetection_ShareMemCfg_t
    {
        /*! @brief   Enable run-time share memory configuration */
        bool shareMemEnable;

        /*! @brief   L3RAM run-time configuration */
        DPC_ObjectDetection_MemCfg L3Ram;

        /*! @brief   L3RAM run-time configuration */
        DPC_ObjectDetection_MemCfg radarCubeMem;
    } DPC_ObjectDetection_ShareMemCfg;

    /*
     * @brief Stats structure to convey to Application timing and related information.
     */
    typedef struct DPC_ObjectDetection_Stats_t
    {
        /*! @brief   Counter which tracks the number of frame start interrupt */
        uint32_t frameStartIntCounter;

        /*! @brief   Frame start CPU time stamp */
        uint32_t frameStartTimeStamp;

        /*! @brief   Inter-frame end CPU time stamp */
        uint32_t interFrameEndTimeStamp;

        /*! @brief   Inter-frame execution time in usec */
        uint32_t interFrameExecTimeInUsec;

        /*! @brief   active frame processing time in usec */
        uint32_t activeFrameProcTimeInUsec;

        /*! @brief DPU benchmark results. */
        radarProcessBenchmarkElem subFrbenchmarkDetails;

    } DPC_ObjectDetection_Stats;

    /*
     * @brief This is the result structure reported from DPC's registered processing function
     *        to the application through the DPM_Buffer structure. The DPM_Buffer's
     *        first fields will be populated as follows:
     *        pointer[0] = pointer to this structure.
     *        size[0] = size of this structure i.e sizeof(DPC_ObjectDetection_Result)
     *
     *        pointer[1..3] = NULL and size[1..3] = 0.
     */
    typedef struct DPC_ObjectDetection_ExecuteResult_t
    {
        /*! @brief      Sub-frame index, this is in the range [0..numSubFrames - 1] */
        uint8_t subFrameIdx;

        DPIF_RadarCube radarCube;

        /*! @brief      Detected objects output list of @ref numObjOut elements */
        DPIF_MSS_DSS_radarProcessOutput objOut;

    } DPC_ObjectDetection_ExecuteResult;


/**
@}
*/
#ifdef __cplusplus
}
#endif

#endif /* DPC_OBJECTDETECTION_H */
