/*
 *  
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
 
 /**
 *   @file  trackerproc.h
 *
 *   @brief
 *      Implements Tracker processing functionality.
 */

/**************************************************************************
 *************************** Include Files ********************************
 **************************************************************************/
#ifndef CLASSIFIERPROC_H
#define CLASSIFIERPROC_H

/* Standard Include Files. */
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* mmWave SDK Driver/Common Include Files */

/* mmWave SDK Data Path Include Files */
#include <common/mmwave_error.h>
#include <source/alg/occupancyFeatExtract/featExtract.h>
#include <source/alg/occupancyClassifier/classifier.h>
#include <source/alg/cnn_classifier/cnn_classifier.h>

/*! @brief Alignment for memory allocation purpose. There is CPU access of this buffer
 *         in the implementation.
 */
#define DPU_CLASSIFIERPROC_POINT_CLOUD_CARTESIAN_BYTE_ALIGNMENT  DPIF_POINT_CLOUD_CARTESIAN_CPU_BYTE_ALIGNMENT

/*! @brief Alignment for memory allocation purpose. There is CPU access of this buffer
 *         in the implementation.
 */
#define DPU_CLASSIFIERPROC_POINT_CLOUD_SIDE_INFO_BYTE_ALIGNMENT  DPIF_POINT_CLOUD_SIDE_INFO_CPU_BYTE_ALIGNMENT


#ifdef __cplusplus
extern "C" {
#endif

#define DP_ERRNO_CLASSIFIER_PROC_BASE              (MMWAVE_ERRNO_DPU_BASE -950)

/** @addtogroup DPU_CLASSIFIERPROC_ERROR_CODE
 *  Base error code for the trackerProc DPU is defined in the
 *  \include ti/datapath/dpif/dp_error.h
 @{ */

/**
 * @brief   Error Code: Invalid argument
 */
#define DPU_CLASSIFIERPROC_EINVAL                  (DP_ERRNO_CLASSIFIER_PROC_BASE-1)

/**
 * @brief   Error Code: Out of memory
 */
#define DPU_CLASSIFIERPROC_ENOMEM                  (DP_ERRNO_CLASSIFIER_PROC_BASE-2)

/**
 * @brief   Error Code: Internal error
 */
#define DPU_CLASSIFIERPROC_EINTERNAL               (DP_ERRNO_CLASSIFIER_PROC_BASE-3)

/**
 * @brief   Error Code: Not implemented
 */
#define DPU_CLASSIFIERPROC_ENOTIMPL                (DP_ERRNO_CLASSIFIER_PROC_BASE-4)

/**
 * @brief   Error Code: In Progress
 */
#define DPU_CLASSIFIERPROC_EINPROGRESS             (DP_ERRNO_CLASSIFIER_PROC_BASE-5)

/**
 * @brief   Error Code: Invalid control command
 */
#define DPU_CLASSIFIERPROC_ECMD                    (DP_ERRNO_CLASSIFIER_PROC_BASE-6)


/* @TODO: Remove this hardcoding and define these correctly in the DPC or elsewhere.
 */

/* Running modes */
#define RUNNING_MODE_INDET 0
#define RUNNING_MODE_SBR 1
#define RUNNING_MODE_CPD 2

#define DPU_CLASSIFIERPROC_CPD_MODE_LPD_USING_ANN 0
#define DPU_CLASSIFIERPROC_CPD_MODE_LPD_USING_CNN 1


#define DPU_CLASSIFIERPROC_CLASSIFIER_MAX_NUM_FEATURES 32
#define DPU_CLASSIFIERPROC_CLASSIFIER_SBR_NUM_FEATURES 7
#define DPU_CLASSIFIERPROC_CLASSIFIER_CPD_NUM_FEATURES 22
#define DPU_CLASSIFIERPROC_CLASSIFIER_HEIGHT_NUM_FEATURES 17
#define DPU_CLASSIFIERPROC_CLASSIFIER_TWO_CLASSES  2
#define DPU_CLASSIFIERPROC_CLASSIFIER_THREE_CLASSES  3

/**
 * @brief
 *  ClassifierProc static configuration
 *
 * @details
 *  The structure is used to hold the static configuraiton used by trackerProc
 *
 *  \ingroup DPU_CLASSIFIERPROC_EXTERNAL_DATA_STRUCTURE
 */
typedef struct DPU_ClassifierProc_StaticConfig_t
{
    uint16_t numFeatures;
    uint16_t numClasses;

    /*! @brief  multi-doppler (macro-doppler) FFT size */
    uint16_t multiFrmDopplerFftSize;

    FEXTRACT_moduleConfig featureExtrModuleCfg;

    /*! @brief     Macro Doppler map scale at the input to CNN for each SBR/CPD zone */
    float cnnInputScale[FEXTRACT_MAX_OCCUPANCY_BOXES];

    uint8_t runningMode;

    /*! @brief  CPD option 1:LPD using ANN, 2:LPD using 1D-CNN */
    uint8_t cpdOption;

    /*! @brief  Macro Doppler DPU enabled */
    uint8_t  macroDopplerFeatureEnabled;

}DPU_ClassifierProc_StaticConfig; 


/**
 * @brief
 *  ClassifierProc DPU Hardware resources
 *
 * @details
 *  ClassifierProc DPU Hardware resources
 *
 *
 *  \ingroup DPU_CLASSIFIERPROC_EXTERNAL_DATA_STRUCTURE
 *
 */
typedef struct DPU_ClassifierProc_Resources_t
{

    /*! @brief      Number of AoA DPU detected points*/
    uint32_t                    numDetObjIn;

    /*! @brief      Detected objects input list sized to @ref detObjOutMaxSize elements,
     *              must be aligned to @ref DPU_CLASSIFIERPROC_POINT_CLOUD_CARTESIAN_BYTE_ALIGNMENT  */
    //DPIF_PointCloudSpherical    *detObjIn;

    /*! @brief      Detected objects side information (snr + noise) output list,
     *              sized to @ref detObjOutMaxSize elements,
     *              must be aligned to @ref DPU_CLASSIFIERPROC_POINT_CLOUD_SIDE_INFO_BYTE_ALIGNMENT */

    float   *featureMultiFrmDoppler;
} DPU_ClassifierProc_HW_Resources;

/**
 * @brief
 *  Tracking configuration
 *
 * @details
 *  The structure is used to hold all the relevant configuration
 *  which is used to configure Tracking module
 */
typedef struct DPU_ClassifierProc_Config_t
{
    /*! @brief      trackerProc static configuration */
    DPU_ClassifierProc_StaticConfig    staticCfg;

    /*! @brief      Hardware resources */
    DPU_ClassifierProc_HW_Resources    res;

    /*! @brief      trackerProc dynamic configuration */
    /*@TODO: If needed*/
}DPU_ClassifierProc_Config;

#if 0
/**
 * @brief
 *  ClassifierProc DPU init param structure
 *
 * @details
 *  TBD
 *
 *  \ingroup DPU_CLASSIFIERPROC_EXTERNAL_DATA_STRUCTURE
 */
typedef struct DPU_ClassifierProc_InitParams_t
{
    /*! @brief   Tracker Handle */
    void        *gtrackHandle;
    
}DPU_ClassifierProc_InitParams;
#endif

/**
 * @brief
 *  ClassifierProc output parameter structure
 *
 * @details
 *  The structure is used to hold the output parameters for ClassifierProc
 *
 *  \ingroup DPU_CLASSIFIERPROC_EXTERNAL_DATA_STRUCTURE
 */
typedef struct DPU_ClassifierProc_OutParams_t
{
    /*! @brief      Features - Feature extraction outputs */
    FEXTRACT_output featOut;

    /*! @brief      Classifier predictions */
    float   zonesPredictions[FEXTRACT_MAX_OCCUPANCY_BOXES * DPU_CLASSIFIERPROC_CLASSIFIER_THREE_CLASSES];

    /*! @brief      Estimated heights of the person occypying the seat */
    float   heightEstimations[FEXTRACT_MAX_OCCUPANCY_BOXES];
}DPU_ClassifierProc_OutParams;

/**
 * @brief
 *  ClassifierProc control command
 *
 * @details
 *  The enum defines the ClassifierProc supported run time command
 *
 *  \ingroup DPU_CLASSIFIERPROC_EXTERNAL_DATA_STRUCTURE
 */
typedef enum DPU_ClassifierProc_Cmd_e
{
    /*! @brief     Command to update macro-doppler map scale for each SBR/CPD zone */
    DPU_ClassifierProc_Cmd_MacroDopplerMapScaleCfg,

    /*! @brief     Command to update zOffset for each SBR/CPD zone */
    DPU_ClassifierProc_Cmd_zOffsetsCfg

}DPU_ClassifierProc_Cmd;

#if 0
typedef struct classifierProc_DescrHandle_t
{
    bool    currentDescr;
    //trackerProc_Target  *tList[2];
    //trackerProc_TargetIndex *tIndex[2];
    /*! @brief   UART processing time in usec */
    uint32_t     uartProcessingTime;
    /*! @brief   track processing time in usec */
    uint32_t     trackProcessingTime;
} classifierProc_DescrHandle;
#endif


/**
 * @brief
 *  ClassifierProc DPU Handle
 *
 *  \ingroup DPU_CLASSIFIERPROC_EXTERNAL_DATA_STRUCTURE
 */
typedef void* DPU_ClassifierProc_Handle;


/**************************************************************************
 ************************ClassifierProc External APIs **************************
 **************************************************************************/

/**
 *  @b Description
 *  @n
 *      The function is ClassifierProc DPU init function. It allocates memory to store
 *  its internal data object and returns a handle if it executes successfully.
 *
 *  @param[in]  errCode                 Pointer to errCode generates from the API
 *
 *  \ingroup    DPU_CLASSIFIERPROC_EXTERNAL_FUNCTION
 *
 *  @retval
 *      Success     - valid ClassifierProc handle
 *  @retval
 *      Error       - NULL
 */
DPU_ClassifierProc_Handle DPU_ClassifierProc_init
(
    int32_t*    errCode
);

/**
 *  @b Description
 *  @n
 *      The function is trackerProc DPU config function.
 *  
 *  @pre    DPU_ClassifierProc_init() has been called
 *
 *  @param[in]  handle                  trackerProc DPU handle
 *  @param[in]  pConfigIn               Pointer to trackerProc configuration data structure
 *
 *  \ingroup    DPU_CLASSIFIERPROC_EXTERNAL_FUNCTION
 *
 *  @retval
 *      Success     - 0
 *  @retval
 *      Error       - <0
 */
int32_t DPU_ClassifierProc_config
(
    DPU_ClassifierProc_Handle  handle,
    DPU_ClassifierProc_Config  *pConfigIn
);

/**
 *  @b Description
 *  @n
 *      The function is trackerProc DPU process function.
 *
 *  @pre    DPU_trackerProc_init() has been called
 *
 *  @param[in]  handle                  trackerProc DPU handle
 *  @param[in]  numObjsIn               number of input points
 *  @param[in]  detObjIn                input point cloud in Spherical format
 *  @param[in]  detObjInSideInfo        point cloud side info
 *  @param[in]  outParams               DPU output parameters
 *
 *  \ingroup    DPU_CLASSIFIERPROC_EXTERNAL_FUNCTION
 *
 *  @retval
 *      Success     - 0
 *  @retval
 *      Error       - <0
 */
int32_t DPU_ClassifierProc_process
(
    DPU_ClassifierProc_Handle      handle,
    uint32_t                    numObjsIn,
    FEXTRACT_measurementPoint    *detObjIn,
    uint32_t                      *multiFrameDopplerFft,
    DPU_ClassifierProc_OutParams   *outParams
);

/**
 *  @b Description
 *  @n
 *      The function is the ClassifierProc DPU control function. 
 *
 *  @pre    DPU_ClassifierProc_init() has been called
 *
 *  @param[in]  handle           ClassifierProc DPU handle
 *  @param[in]  cmd              ClassifierProc DPU control command
 *  @param[in]  arg              ClassifierProc DPU control argument pointer
 *  @param[in]  argSize          ClassifierProc DPU control argument size
 *
 *  \ingroup    DPU_CLASSIFIERPROC_EXTERNAL_FUNCTION
 *
 *  @retval
 *      Success     - 0
 *  @retval
 *      Error       - <0
 */
int32_t DPU_ClassifierProc_control
(
    DPU_ClassifierProc_Handle     handle,
    DPU_ClassifierProc_Cmd        cmd,
    void*                      arg,
    uint32_t                   argSize
);

/**
 *  @b Description
 *  @n
 *      The function is the ClassifierProc DPU deinit function. It frees up the 
 *   resources allocated during init.
 *
 *  @pre    DPU_ClassifierProc_init() has been called
 *
 *  @param[in]  handle           ClassifierProc DPU handle
 *
 *  \ingroup    DPU_CLASSIFIERPROC_EXTERNAL_FUNCTION
 *
 *  @retval
 *      Success     - 0
 *  @retval
 *      Error       - <0
 */
int32_t DPU_ClassifierProc_deinit
(
    DPU_ClassifierProc_Handle handle
);

#if 0
/**
 *  @b Description
 *  @n
 *      Utility function to convert Cartesian point cloud to Spherical.
 *
 *  @pre    None
 *
 *  @param[in]  pointCloudCartesianIn   Input point cloud in cartesian format 
 *  @param[out] pointCloudCartesianIn   Output point cloud in spherical format  
 *  @param[in]  numPoints               Number of input points
 *
 *  \ingroup    DPU_CLASSIFIERPROC_EXTERNAL_FUNCTION
 *
 *  @retval
 *      Success     - 0
 *  @retval
 *      Error       - <0
 */
int32_t DPU_ClassifierProc_CartesianToSpherical
(
    DPIF_PointCloudCartesian    *pointCloudCartesianIn,
    DPIF_PointCloudSpherical    *pointCloudSphericalOut,
    uint16_t                    numPoints
);
#endif

#ifdef __cplusplus
}
#endif

#endif
