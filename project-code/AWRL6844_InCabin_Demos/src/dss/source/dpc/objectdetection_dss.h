/**
 *   @file  objectdetection.h
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

#ifndef DPC_DSS_OBJECTDETECTION_H
#define DPC_DSS_OBJECTDETECTION_H

/* MMWAVE Driver Include Files */
#include <common/mmwave_error.h>

#include <datapath/dpif/dpif_pointcloud.h>
#include <datapath/dpif/dpif_radarcube.h>
#include <datapath/dpif/dpif_detmatrix.h>

#include <source/dpu/capon3d_overhead/radarProcess.h>

#define DEMO_RL_MAX_SUBFRAMES 1

#ifdef __cplusplus
extern "C"
{
#endif

    /** @defgroup DPC_OBJDET_EXTERNAL        Object Detection DPC (Data-path Processing Chain) External
     *
     * This DPC performs processes ADC samples and generates detected object list.
     */
    /**
    @defgroup DPC_OBJDET__GLOBAL                             Object Detection DPC Globals
    @ingroup DPC_OBJDET_EXTERNAL
    @brief
    *   This section has a list of all the globals exposed by the Object detection DPC.
    */
    /**
    @defgroup DPC_OBJDET_IOCTL__DATA_STRUCTURES               Object Detection DPC Data Structures
    @ingroup DPC_OBJDET_EXTERNAL
    @brief
    *   This section has a list of all the data structures which are a part of the DPC module
    *   and which are exposed to the application
    */
    /**
    @defgroup DPC_OBJDET_IOCTL__COMMAND                       Object Detection DPC Configuration Commands
    @ingroup DPC_OBJDET_EXTERNAL
    @brief
    *   This section has a list of all the commands which are supported by the DPC.
    *   All commands of the type IOCTL__STATIC_<...> can only be issued either before the
    *   first call to @ref DPM_start (DPC_ObjectDetection_start) or after the @ref DPM_stop (DPC_ObjectDetection_stop)
    *   All commands of the type IOCTL__DYNAMIC_<...> can be issued between at
    *   the inter-frame boundary i.e when the result is available from @ref DPM_execute (DPC_ObjectDetection_execute).
    *   All commands of the type IOCTL__STATIC_<..> must be issued
    *   before @ref DPM_start (DPC_ObjectDetection_start) because there are no defaults.
    */
    /**
    @defgroup DPC_OBJECTDETECTION_ERROR_CODE                   Object Detection DPC Error Codes
    @ingroup DPC_OBJDET_EXTERNAL
    @brief
    *   This section has a list of all the error codes returned when calling Object Detection DPC functions
    *   during error conditions.
    */
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

    /*
     * @brief Memory Configuration used during init API
     */
    typedef struct DPC_DSS_ObjectDetection_MemCfg_t
    {
        /*! @brief   Start address of memory provided by the application
         *           from which DPC will allocate.
         */
        void *addr;

        /*! @brief   Size limit of memory allowed to be consumed by the DPC */
        uint32_t size;
    } DPC_DSS_ObjectDetection_MemCfg;

    /*!
     *  @brief      Call back function type for calling back during process
     *  @param[out] subFrameIndx Sub-frame indx [0..(numSubFrames-1)]
     */
    typedef void (*DPC_ObjectDetection_processCallBackFxn_t)(uint8_t subFrameIndx);

    /*
     * @brief Configuration related to share memory allocation at run-time.
     *      These configuration should overwrite init time configuration once enabled.
     *      It is used to share memory across DPCs
     */
    typedef struct DPC_DSS_ObjectDetection_ShareMemCfg_t
    {
        /*! @brief   Enable run-time share memory configuration */
        bool shareMemEnable;

        /*! @brief   L3RAM run-time configuration */
        DPC_DSS_ObjectDetection_MemCfg L3Ram;

        /*! @brief   L3RAM run-time configuration */
        DPC_DSS_ObjectDetection_MemCfg radarCubeMem;
    } DPC_DSS_ObjectDetection_ShareMemCfg;

    /*
     * @brief Stats structure to convey to Application timing and related information.
     */
    typedef struct DPC_DSS_ObjectDetection_Stats_t
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
        DPIF_MSS_DSS_radarProcessBenchmarkElem subFrbenchmarkDetails;

    } DPC_DSS_ObjectDetection_Stats;

    /*
     * @brief This is the result structure reported from DPC's registered processing function
     *        to the application through the DPM_Buffer structure. The DPM_Buffer's
     *        first fields will be populated as follows:
     *        pointer[0] = pointer to this structure.
     *        size[0] = size of this structure i.e sizeof(DPC_ObjectDetection_Result)
     *
     *        pointer[1..3] = NULL and size[1..3] = 0.
     */
    typedef struct DPC_DSS_ObjectDetection_ExecuteResult_t
    {
        /*! @brief      Sub-frame index, this is in the range [0..numSubFrames - 1] */
        uint8_t subFrameIdx;

        DPIF_RadarCube radarCube;

        /*! @brief      Detected objects output list of @ref numObjOut elements */
        DPIF_MSS_DSS_radarProcessOutput objOut;

    } DPC_DSS_ObjectDetection_ExecuteResult;


    /*!
     *  @brief      Call back function type for calling back during process
     *  @param[out] subFrameIndx Sub-frame indx [0..(numSubFrames-1)]
     */
    typedef void (*DPC_ObjectDetection_processCallBackFxn_t)(uint8_t subFrameIndx);

    /*! @brief  Process call backs configuration */
    typedef struct DPC_DSS_ObjectDetection_ProcessCallBackCfg_t
    {
        /*! @brief  Call back function that will be called at the beginning of frame
         *          processing (beginning of 1D) */
        DPC_ObjectDetection_processCallBackFxn_t processFrameBeginCallBackFxn;

        /*! @brief  Call back function that will be called at the beginning of inter-frame
         *          processing (beginning of 2D) */
        DPC_ObjectDetection_processCallBackFxn_t processInterFrameBeginCallBackFxn;
    } DPC_DSS_ObjectDetection_ProcessCallBackCfg;

    /*
     * @brief Configuration for DPM's init API.
     *        DPM_init's arg = pointer to this structure.
     *        DPM_init's argLen = size of this structure.
     *
     */
    typedef struct DPC_DSS_ObjectDetection_InitParams_t
    {
        /*! @brief L3 heap configuration. DPC will allocate memory from this
         *         as needed and report the amount of memory consumed through
         *         @ref DPC_ObjectDetection_PreStartCfg to application */
        DPC_DSS_ObjectDetection_MemCfg L3HeapCfg;

        /*! @brief L3 scratch configuration. DPC will allocate memory from this
         *         as needed and report the amount of memory consumed through
         *         @ref DPC_ObjectDetection_PreStartCfg to application */
        DPC_DSS_ObjectDetection_MemCfg L3ScratchCfg;

        /*! @brief Core L2 heap configuration (DSP L2 memory).
         *         DPC will allocate memory from this as needed and report the amount
         *         of memory consumed through @ref DPC_ObjectDetection_PreStartCfg
         *         to the application */
        DPC_DSS_ObjectDetection_MemCfg CoreL2HeapCfg;

        /*! @brief Core L2 scratch configuration (DSP L2 memory).
         *         DPC will allocate memory from this as needed and report the amount
         *         of memory consumed through @ref DPC_ObjectDetection_PreStartCfg
         *         to the application */
        DPC_DSS_ObjectDetection_MemCfg CoreL2ScratchCfg;

        /*! @brief Core L1 heap configuration (DSP L1 memory).
         *         DPC will allocate memory from this as needed and report the amount
         *         of memory consumed through @ref DPC_ObjectDetection_PreStartCfg
         *         to the application */
        DPC_DSS_ObjectDetection_MemCfg CoreL1HeapCfg;

        /*! @brief Core L1 scratch configuration (DSP L1 memory).
         *         DPC will allocate memory from this as needed and report the amount
         *         of memory consumed through @ref DPC_ObjectDetection_PreStartCfg
         *         to the application */
        DPC_DSS_ObjectDetection_MemCfg CoreL1ScratchCfg;

        /*! @brief   Process call back function configuration */
        DPC_DSS_ObjectDetection_ProcessCallBackCfg processCallBackCfg;
    } DPC_DSS_ObjectDetection_InitParams;


    /*
     * @brief Dynamic Configuration that is part of the pre-start configuration.
     */
    typedef struct DPC_DSS_ObjectDetection_DynCfg_t
    {
        DPU_radarProcessConfig_t caponChainCfg;

        /*! @brief      radar cube format */
        uint8_t radarCubeFormat;

        /*! @brief      CFAR range threshold for dynamic scene */
        float dynSceneCfarRngThr;

        /*! @brief      CFAR angle threshold for dynamic scene */
        float dynSceneCfarAngThr;

        /*! @brief      CFAR angle sidelobe threshold for dynamic scene */
        float dynSceneCfarSideLobeThr;

        /*! @brief      CFAR range threshold for static scene */
        float staticSceneCfarRngThr;

        /*! @brief      CFAR angle threshold for static scene */
        float staticSceneCfarAngThr;

        /*! @brief      CFAR angle sidelobe for static scene */
        float staticSceneCfarSideLobeThr;

    } DPC_DSS_ObjectDetection_DynCfg;

    /*
     * @brief Configuration related to IOCTL API for command
     *        @ref DPC_OBJDET_IOCTL__STATIC_PRE_START_CFG.
     *
     */
    typedef struct DPC_DSS_ObjectDetection_PreStartCfg_t
    {
        /*! @brief   Run-time share memory configuraiton. This configuation
         *           should be used to overwrite the init time configuration if available.
         */
        DPC_DSS_ObjectDetection_ShareMemCfg shareMemCfg;

        /*! Dynamic configuration */
        DPC_DSS_ObjectDetection_DynCfg dynCfg;

    } DPC_DSS_ObjectDetection_PreStartCfg;

  
    /*
     * @brief This is the informational structure related to the IOCTL command
     *        @ref DPC_OBJDET_IOCTL__DYNAMIC_EXECUTE_RESULT_EXPORTED.
     */
    typedef struct DPC_DSS_ObjectDetection_ExecuteResultExportedInfo_t
    {
        /*! @brief      Sub-frame index, this is in the range [0..numSubFrames - 1].
         *              This is the sub-frame whose results have been exported.
         *              Although this DPC implementation knows what sub-frame to expect as the exports
         *              are expected to be sequential in sub-frames, this field helps
         *              in error checking when for example the application could miss
         *              exporting/consuming a sub-frame in a timely manner or have out of order
         *              export/consumption. */
        uint8_t subFrameIdx;
    } DPC_DSS_ObjectDetection_ExecuteResultExportedInfo;

/**
@}
*/

/** @addtogroup DPC_OBJDET_IOCTL__COMMAND
 @{ */

/**
 * @brief Command associated with @ref DPC_ObjectDetection_PreStartCfg_t. In this IOCTL, the sub-frame's
 *        configurations will be processed by configuring individual DPUs.
 *        The @ref DPC_OBJDET_IOCTL__STATIC_PRE_START_COMMON_CFG must be issued
 *        before issuing this IOCTL.
 */
#define DPC_OBJDET_IOCTL__STATIC_PRE_START_CFG (DPM_CMD_DPC_START_INDEX + 0U)

/**
 * @brief Command to get number of subframes. Must be issued before
 *        issuing @ref DPC_OBJDET_IOCTL__STATIC_PRE_START_CFG
 */
#define DPC_OBJDET_IOCTL__STATIC_PRE_START_COMMON_CFG (DPM_CMD_DPC_START_INDEX + 1U)

/**
 * @brief This commands indicates to the DPC that the results DPC provided to the application
 *        through its execute API (which application will access through DPM_execute API)
 *        have been exported/consumed. The purpose of this command is for DPC to
 *        reclaim the memory resources associated with the results. The DPC may
 *        also perform sub-frame switching, and do error-checking to see
 *        if export was later than expected e.g the DPC design may be such that
 *        the previous frame/sub-frame's export notification may need to come
 *        after a new frame/sub-frame (this is the case currently with this
 *        object detection DPC). The DPC will also consider this command as a
 *        signal from the application that all its processing for the current frame/sub-frame
 *        has been done and so if a new frame/sub-frame interrupt (DPC has registered
 *        a frame interrupt with the DPM) comes before the last step in the
 *        processing of this command (which could be sub-frame switching and
 *        preparing for next sub-frame/frame), then the DPC will signal an assert
 *        to the application from its frame interrupt. The expected sequence is
 *        the following:
 *
 *        1. App consumes the process result of the DPC (e.g sending output on UART).
 *        2. App performs any dynamic configuration command processing by issuing DPC's
 *           IOCTL APIs for the next frame/sub-frame.
 *        3. App issues this result-exported IOCTL.
 *        4. DPC does its processing related to this IOCTL in the following sequence:
 *            a. May do error checking and preparing for next sub-frame/frame.
 *            b. Do book-keeping related to marking this as end of sub-frame/frame processing
 *               by the app. The DPC's registered frame start interrupt performs
 *               check on this information to see if next frame/sub-frame came before
 *               this end of processing in which case it will issue an assert to app.
 *
 *        An informational structure @ref DPC_DSS_ObjectDetection_ExecuteResultExportedInfo_t
 *        is associated with this command.
 */
#define DPC_OBJDET_IOCTL__DYNAMIC_EXECUTE_RESULT_EXPORTED (DPM_CMD_DPC_START_INDEX + 2U)

/**
 * @brief This command is for non real-time (without RF) testing. When issued, it will simulate
 *        the trigger of frame start. No configuration structure is associated with this command.
 *        Must be issued between start and stop of DPC.
 */
#define DPC_OBJDET_IOCTL__TRIGGER_FRAME (DPM_CMD_DPC_START_INDEX + 3U)


/**
 * @brief This is not a command, just to capture the last command supported in DPC.
        This definition is used to track if the commond is targed for this DPC, otherwise return an error.
 */
#define DPC_OBJDET_IOCTL__MAX (DPC_OBJDET_IOCTL__TRIGGER_FRAME)

/**
@}
*/

/** @addtogroup DPC_OBJECTDETECTION_ERROR_CODE
 *  Base error code for the objdetdsp DPC is defined in the
 *  \include ti/datapath/dpif/dp_error.h
 @{ */

/**
 * @brief   Error Code: Invalid argument general (such as NULL argument pointer)
 */
#define DPC_OBJECTDETECTION_EINVAL (DP_ERRNO_OBJDETDSP_BASE - 1)

/**
 * @brief   Error Code: Invalid argSize in DPM_InitCfg provided to @ref DPC_ObjectDetection_init,
 *          does not match the expected size of @ref DPC_ObjectDetection_InitParams_t
 */
#define DPC_OBJECTDETECTION_EINVAL__INIT_CFG_ARGSIZE (DP_ERRNO_OBJDETDSP_BASE - 2)

/**
 * @brief   Error Code: Invalid argument bad command argument in DPM_ProcChainIoctlFxn for
 *                      Object detection DPC.
 */
#define DPC_OBJECTDETECTION_EINVAL__COMMAND (DP_ERRNO_OBJDETDSP_BASE - 3)

/**
 * @brief   Error Code: memory osal init error
 */
#define DPC_OBJECTDETECTION_MEMINITERR (DP_ERRNO_OBJDETDSP_BASE - 10)

/**
 * @brief   Error Code: no memory available
 */
#define DPC_OBJECTDETECTION_ENOMEM (DP_ERRNO_OBJDETDSP_BASE - 11)

/**
 * @brief   Error Code: wrong radar cube config
 */
#define DPC_OBJECTDETECTION_EINVAL_CUBE (DP_ERRNO_OBJDETDSP_BASE - 12)

/**
 * @brief   Error Code: Pre-start config was received before pre-start common config.
 */
#define DPC_OBJECTDETECTION_PRE_START_CONFIG_BEFORE_PRE_START_COMMON_CONFIG (DP_ERRNO_OBJDETDSP_BASE - 13)

/**
 * @brief   Error Code: Internal error
 */
#define DPC_OBJECTDETECTION_EINTERNAL (DP_ERRNO_OBJDETDSP_BASE - 40)

/**
 * @brief   Error Code: Not implemented
 */
#define DPC_OBJECTDETECTION_ENOTIMPL (DP_ERRNO_OBJDETDSP_BASE - 50)


extern void DPC_ObjectDetection_init(DPC_DSS_ObjectDetection_InitParams *dpcInitParams, int32_t *errCode);
extern int32_t DPC_ObjDetDSP_preStartConfig(DPC_DSS_ObjectDetection_PreStartCfg *preStartCfg);
extern int32_t DPC_ObjectDetection_execute(void);


/**
@}
*/
#ifdef __cplusplus
}
#endif

#endif /* DPC_OBJECTDETECTION_H */
