/*
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
 *   @file  trackerproc.c
 *
 *   @brief
 *      Implements DPU wrapper for the gtrack lib.
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

/* mmWave SDK drivers/common Include Files */
#include <common/syscommon.h>
#include <kernel/dpl/SemaphoreP.h>
#include <kernel/dpl/CacheP.h>
#include <kernel/dpl/HeapP.h>
#ifdef SUBSYS_MSS
#include <kernel/dpl/CacheP.h>
#endif

/* MATH utils library Include files */
#include <utils/mathutils/mathutils.h>

/* Data Path Include files */
#include <source/dpu/classifierproc/classifierprocinternal.h>
#include <source/alg/occupancyFeatExtract/featExtract.h>
#include <source/alg/occupancyClassifier/classifier.h>
#include <source/alg/cnn_classifier/cnn_classifier.h>


/**************************************************************************
 ************************ Global Variables       **********************
 **************************************************************************/
/* User defined heap memory and handle */
#define CLASSIFIERPROC_HEAP_MEM_SIZE  (sizeof(classifierProcObj))

static uint8_t gClassifierProcHeapMem[CLASSIFIERPROC_HEAP_MEM_SIZE] __attribute__((aligned(HeapP_BYTE_ALIGNMENT)));

/**************************************************************************
 *************************** Local Definitions ****************************
 **************************************************************************/

/**************************************************************************
 ************************ Internal Functions Prototyp**********************
 **************************************************************************/

/**************************************************************************
 ************************ClassifierProc Internal Functions **********************
 **************************************************************************/
    
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
)
{
    classifierProcObj *obj = NULL;

    /* Allocate memory for classifier DPU*/
    obj = (classifierProcObj  *) &gClassifierProcHeapMem;
    if(obj == NULL)
    {
        *errCode = DPU_CLASSIFIERPROC_ENOMEM;
        goto exit;
    }

    /* Initialize memory */
    memset((void *)obj, 0U, sizeof(classifierProcObj));

    printf("Classifier DPU: (classifierProcObj *) 0x%08x\n", (uint32_t) obj);


exit:
    return ((DPU_ClassifierProc_Handle) obj);

}

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
)
{
    classifierProcObj *obj = (classifierProcObj *) handle;

    int32_t                         retVal = 0;
    int32_t errCode;
    Classifier_moduleConfig classConfig;
    cnn_Classifier_moduleConfig cnnClassConfig;


    /* Configure feature extraction */
    obj->featExtractHandle = featExtract_create(&pConfigIn->staticCfg.featureExtrModuleCfg, &errCode);
    if (errCode < 0)
    {
        retVal = errCode;
        goto exit;
    }
    obj->numOccupancyZones = pConfigIn->staticCfg.featureExtrModuleCfg.sceneryParams.numOccupancyBoxes;


    classConfig.num_classes = DPU_CLASSIFIERPROC_CLASSIFIER_TWO_CLASSES;
    
    classConfig.scratchBuffer = NULL;
    classConfig.scratchBufferSizeInBytes = 0;

    if (pConfigIn->staticCfg.runningMode == RUNNING_MODE_SBR)
    {
        /* Configure SBR classifier */
        classConfig.num_features = DPU_CLASSIFIERPROC_CLASSIFIER_SBR_NUM_FEATURES;
        classConfig.classifier_model = SBR_EMPTY_OCCUPIED_MDL;

        obj->sbrEmptyOccClassifierHandle = classifier_create(&classConfig, &errCode);
        if (errCode < 0)
        {
            retVal = errCode;
            goto exit;
        }
    }
    else if (pConfigIn->staticCfg.runningMode == RUNNING_MODE_CPD)
    {
        if (pConfigIn->staticCfg.cpdOption == DPU_CLASSIFIERPROC_CPD_MODE_LPD_USING_ANN)
        {
            /* Configure LPD (empty/occupied) classifier */
            classConfig.num_features = DPU_CLASSIFIERPROC_CLASSIFIER_HEIGHT_NUM_FEATURES; //DPU_CLASSIFIERPROC_CLASSIFIER_CPD_NUM_FEATURES;
            classConfig.classifier_model = LPD_EMPTY_OCCUPIED_MDL;

            obj->lpdEmptyOccClassifierHandle = classifier_create(&classConfig, &errCode);
            if (errCode < 0)
            {
                retVal = errCode;
                goto exit;
            }
        }
        else if (pConfigIn->staticCfg.cpdOption == DPU_CLASSIFIERPROC_CPD_MODE_LPD_USING_CNN)
        {
            /* Configure 1D-CNN LPD (empty/occupied) classifier */
            memset(&cnnClassConfig, 0, sizeof(cnn_Classifier_moduleConfig));
            cnnClassConfig.scratchBuffer = NULL;
            cnnClassConfig.scratchBufferSizeInBytes = 0;
            cnnClassConfig.num_frames = pConfigIn->staticCfg.multiFrmDopplerFftSize;
            cnnClassConfig.num_features = 1;
            cnnClassConfig.num_classes = DPU_CLASSIFIERPROC_CLASSIFIER_TWO_CLASSES;

            obj->lpd_cnn_EmptyOccClassifierHandle = cnn_classifier_create(&cnnClassConfig, &errCode);
            if (errCode < 0)
            {
                retVal = errCode;
                goto exit;
            }
        }
        else
        {
            retVal = DPU_CLASSIFIERPROC_EINVAL;
            goto exit;
        }

        /* Configure CPD (Adult/Child) classifier */
        classConfig.num_features = DPU_CLASSIFIERPROC_CLASSIFIER_CPD_NUM_FEATURES;
        classConfig.classifier_model = CPD_CHILD_ADULT_MDL;

        obj->cpdChildAdultClassifierHandle = classifier_create(&classConfig, &errCode);
        if (errCode < 0)
        {
            retVal = errCode;
            goto exit;
        }

        /* Configure CPD (height) classifier */
        classConfig.num_features = DPU_CLASSIFIERPROC_CLASSIFIER_HEIGHT_NUM_FEATURES;
        classConfig.classifier_model = CPD_HEIGHT_ESTIMATION_MDL;
        classConfig.num_classes = 1;

        obj->cpdHeightEstClassifierHandle = classifier_create(&classConfig, &errCode);
        if (errCode < 0)
        {
            retVal = errCode;
            goto exit;
        }


    }

    /* Copy in the whole DPU configuration */
    obj->config = *pConfigIn;

exit:
    return retVal;
}

#define PROFILE_CLASSIFIER_DPU
#ifdef PROFILE_CLASSIFIER_DPU
extern uint32_t Cycleprofiler_getTimeStamp(void);
volatile uint32_t gProfileFeatureExtr[4];
volatile uint32_t gProfilePredict[4];
volatile uint32_t gProfileClassifierInd = 0;
#endif

/**
 *  @b Description
 *  @n
 *      The function is trackerProc DPU process function.
 *
 *  @pre    DPU_classifierProc_init() has been called
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
    uint32_t                       numObjsIn,
    FEXTRACT_measurementPoint     *detObjIn,
    uint32_t                      *multiFrameDopplerFft,
    DPU_ClassifierProc_OutParams  *outParams
)
{
    int32_t                 retVal = 0;
    int32_t zoneInd;
    float predictions[DPU_CLASSIFIERPROC_CLASSIFIER_TWO_CLASSES];
    float features[DPU_CLASSIFIERPROC_CLASSIFIER_MAX_NUM_FEATURES] = {0};
    FEXTRACT_output *featOut = &outParams->featOut;
    classifierProcObj *obj = (classifierProcObj *) handle;

#ifdef PROFILE_CLASSIFIER_DPU
    uint32_t startTime = Cycleprofiler_getTimeStamp();
#endif
    /* Feature extraction */
    featExtract_compute(obj->featExtractHandle, detObjIn, numObjsIn, featOut);

#ifdef PROFILE_CLASSIFIER_DPU
    uint32_t endTime = Cycleprofiler_getTimeStamp();
    //printf("featExt = %d\n", endTime -  startTime);
    if ((endTime -  startTime) > gProfileFeatureExtr[gProfileClassifierInd])
    {
        gProfileFeatureExtr[gProfileClassifierInd] = endTime -  startTime;
    }
    startTime = endTime;
#endif
    
    if (obj->config.staticCfg.runningMode == RUNNING_MODE_SBR)
    {
        /*** SBR Mode ***/
        if (featOut->featuresComputed)
        {
            for (zoneInd = 0; zoneInd < obj->numOccupancyZones; zoneInd++)
            {
                 /* Copy the features used by SBR */
                features[0] = featOut->featsPerZone[zoneInd].numPtsMean;
                features[1] = featOut->featsPerZone[zoneInd].xCordStd;
                features[2] = featOut->featsPerZone[zoneInd].yCordStd;
                features[3] = featOut->featsPerZone[zoneInd].zCordStd;
                features[4] = featOut->featsPerZone[zoneInd].xCordRms;
                features[5] = featOut->featsPerZone[zoneInd].yCordRms;
                features[6] = featOut->featsPerZone[zoneInd].zCordRms;

                classifier_predict(obj->sbrEmptyOccClassifierHandle, features, predictions);
                memcpy(&outParams->zonesPredictions[zoneInd * DPU_CLASSIFIERPROC_CLASSIFIER_TWO_CLASSES], predictions, DPU_CLASSIFIERPROC_CLASSIFIER_TWO_CLASSES * sizeof(float));
            }
        }
        else
        {
            /* Set all the predictions to zero */
            memset(outParams->zonesPredictions, 0.0f,  obj->numOccupancyZones * DPU_CLASSIFIERPROC_CLASSIFIER_TWO_CLASSES * sizeof(float));

            /* Set the first (empty) state of each zone to one, by default */
            for (zoneInd = 0; zoneInd < obj->numOccupancyZones; zoneInd++)
            {
                outParams->zonesPredictions[zoneInd * DPU_CLASSIFIERPROC_CLASSIFIER_TWO_CLASSES] = 1.0f;
            }
        }
    }
    else if (obj->config.staticCfg.runningMode == RUNNING_MODE_CPD)
    {
        /*** CPD Mode ***/
        if (featOut->featuresComputed)
        {
            for (zoneInd = 0; zoneInd < obj->numOccupancyZones; zoneInd++)
            {
                if (obj->config.staticCfg.cpdOption == DPU_CLASSIFIERPROC_CPD_MODE_LPD_USING_ANN)
                {
                    /* Copy the features used by LPD using ANN */
                    features[0] = featOut->featsPerZone[zoneInd].numPtsMean;
                    features[1] = featOut->featsPerZone[zoneInd].zCordMean;
                    features[2] = featOut->featsPerZone[zoneInd].yCordMean;
                    features[3] = featOut->featsPerZone[zoneInd].xCordMean;
                    features[4] = featOut->featsPerZone[zoneInd].zCordStd;
                    features[5] = featOut->featsPerZone[zoneInd].yCordStd;
                    features[6] = featOut->featsPerZone[zoneInd].xCordStd;
                    features[7] = featOut->featsPerZone[zoneInd].zCordMax;
                    features[8] = featOut->featsPerZone[zoneInd].yCordMax;
                    features[9] = featOut->featsPerZone[zoneInd].xCordMax;
                    features[10] = featOut->featsPerZone[zoneInd].zCordMin;
                    features[11] = featOut->featsPerZone[zoneInd].yCordMin;
                    features[12] = featOut->featsPerZone[zoneInd].xCordMin;
                    features[13] = featOut->featsPerZone[zoneInd].zCordSize;
                    features[14] = featOut->featsPerZone[zoneInd].yCordSize;
                    features[15] = featOut->featsPerZone[zoneInd].xCordSize;
                    features[16] = featOut->featsPerZone[zoneInd].volume;

                    /* Classify: Empty/Occupied */
                    classifier_predict(obj->lpdEmptyOccClassifierHandle, features, predictions);
                    outParams->zonesPredictions[zoneInd * DPU_CLASSIFIERPROC_CLASSIFIER_THREE_CLASSES + 0] = predictions[0];
                }
                else if (obj->config.staticCfg.cpdOption == DPU_CLASSIFIERPROC_CPD_MODE_LPD_USING_CNN)
                {
                    /* Copy the features used by LPD 1D-CNN: Macro-Doppler spectrum */
                    uint32_t *spectrumArray = &multiFrameDopplerFft[zoneInd * obj->config.staticCfg.multiFrmDopplerFftSize];
                    float    *featureMultiFrmDoppler = obj->config.res.featureMultiFrmDoppler;
                    for (int32_t i = 0; i < obj->config.staticCfg.multiFrmDopplerFftSize; i++)
                    {
                        featureMultiFrmDoppler[i] = obj->config.staticCfg.cnnInputScale[zoneInd] * (float) spectrumArray[i];
                    }

                    /* Classify: Empty/Occupied using 1D-CNN */
                    cnn_classifier_predict(obj->lpd_cnn_EmptyOccClassifierHandle, featureMultiFrmDoppler, predictions);
                    outParams->zonesPredictions[zoneInd * DPU_CLASSIFIERPROC_CLASSIFIER_THREE_CLASSES + 0] = predictions[0];
                }
                else
                {
                    retVal = -1;
                }
                //if (predictions[0] < 0.5)
                {
                    float probOfOccupied = 1;   //predictions[1];

                    /* Copy the features used by CPD */
                    features[0] = featOut->featsPerZone[zoneInd].numPtsMean;
                    features[1] = featOut->featsPerZone[zoneInd].snrMean;
                    features[2] = featOut->featsPerZone[zoneInd].xCordMean;
                    features[3] = featOut->featsPerZone[zoneInd].yCordMean;
                    features[4] = featOut->featsPerZone[zoneInd].zCordMean;
                    features[5] = featOut->featsPerZone[zoneInd].snrStd;
                    features[6] = featOut->featsPerZone[zoneInd].xCordStd;
                    features[7] = featOut->featsPerZone[zoneInd].yCordStd;
                    features[8] = featOut->featsPerZone[zoneInd].zCordStd;
                    features[9] = featOut->featsPerZone[zoneInd].snrMax;
                    features[10] = featOut->featsPerZone[zoneInd].xCordMax;
                    features[11] = featOut->featsPerZone[zoneInd].yCordMax;
                    features[12] = featOut->featsPerZone[zoneInd].zCordMax;
                    features[13] = featOut->featsPerZone[zoneInd].snrMin;
                    features[14] = featOut->featsPerZone[zoneInd].xCordMin;
                    features[15] = featOut->featsPerZone[zoneInd].yCordMin;
                    features[16] = featOut->featsPerZone[zoneInd].zCordMin;
                    features[17] = featOut->featsPerZone[zoneInd].snrSize;
                    features[18] = featOut->featsPerZone[zoneInd].xCordSize;
                    features[19] = featOut->featsPerZone[zoneInd].yCordSize;
                    features[20] = featOut->featsPerZone[zoneInd].zCordSize;
                    features[21] = featOut->featsPerZone[zoneInd].volume;

                    /* Classify: Child/Adult */
                    classifier_predict(obj->cpdChildAdultClassifierHandle, features, predictions);

                    outParams->zonesPredictions[zoneInd * DPU_CLASSIFIERPROC_CLASSIFIER_THREE_CLASSES + 1] = probOfOccupied * predictions[0]; // Child presence probability
                    outParams->zonesPredictions[zoneInd * DPU_CLASSIFIERPROC_CLASSIFIER_THREE_CLASSES + 2] = probOfOccupied * predictions[1]; // Adult presence probability

                    /* Copy the features used by height estimation */
                    features[0] = featOut->featsPerZone[zoneInd].numPtsMean;
                    features[1] = featOut->featsPerZone[zoneInd].zCordMean;
                    features[2] = featOut->featsPerZone[zoneInd].yCordMean;
                    features[3] = featOut->featsPerZone[zoneInd].xCordMean;
                    features[4] = featOut->featsPerZone[zoneInd].zCordStd;
                    features[5] = featOut->featsPerZone[zoneInd].yCordStd;
                    features[6] = featOut->featsPerZone[zoneInd].xCordStd;
                    features[7] = featOut->featsPerZone[zoneInd].zCordMax;
                    features[8] = featOut->featsPerZone[zoneInd].yCordMax;
                    features[9] = featOut->featsPerZone[zoneInd].xCordMax;
                    features[10] = featOut->featsPerZone[zoneInd].zCordMin;
                    features[11] = featOut->featsPerZone[zoneInd].yCordMin;
                    features[12] = featOut->featsPerZone[zoneInd].xCordMin;
                    features[13] = featOut->featsPerZone[zoneInd].zCordSize;
                    features[14] = featOut->featsPerZone[zoneInd].yCordSize;
                    features[15] = featOut->featsPerZone[zoneInd].xCordSize;
                    features[16] = featOut->featsPerZone[zoneInd].volume;
                    
                    /* Classify: Height estimation */
                    classifier_predict(obj->cpdHeightEstClassifierHandle, features, predictions);
                    outParams->heightEstimations[zoneInd] = predictions[0];                  // Person's height estimation

                }
                //else
                //{
                //    outParams->zonesPredictions[zoneInd * DPU_CLASSIFIERPROC_CLASSIFIER_THREE_CLASSES + 1] = predictions[1] * 0.5; // Child presence probability
                //    outParams->zonesPredictions[zoneInd * DPU_CLASSIFIERPROC_CLASSIFIER_THREE_CLASSES + 2] = predictions[1] * 0.5; // Adult presence probability
                //    outParams->heightEstimations[zoneInd] = 0;   // Person's height estimation
                //}
            }
        }
        else
        {
            /* Set all the predictions to zero */
            memset(outParams->zonesPredictions, 0.0f,  obj->numOccupancyZones * DPU_CLASSIFIERPROC_CLASSIFIER_THREE_CLASSES * sizeof(float));

            /* Set all heights to zero */
            memset(outParams->heightEstimations, 0,  obj->numOccupancyZones * sizeof(float));

            /* Set the first (empty) state of each zone to one, by default */
            for (zoneInd = 0; zoneInd < obj->numOccupancyZones; zoneInd++)
            {
                outParams->zonesPredictions[zoneInd * DPU_CLASSIFIERPROC_CLASSIFIER_THREE_CLASSES] = 1.0f;
            }
        }
    }
    else
    {
        retVal = -1;
    }

#ifdef PROFILE_CLASSIFIER_DPU
    endTime = Cycleprofiler_getTimeStamp();
    if ((endTime - startTime) > gProfilePredict[gProfileClassifierInd])
    {
        gProfilePredict[gProfileClassifierInd] = endTime -  startTime;
    }
    gProfileClassifierInd = (gProfileClassifierInd + 1) & 0x3;
#endif
    return retVal;
}


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
)
{
    int32_t             retVal = 0;
    classifierProcObj  *obj;
    

    /* Get trackerProc data object */
    obj = (classifierProcObj *)handle;
    
    /* Sanity check */
    if(obj == NULL)
    {
        retVal = DPU_CLASSIFIERPROC_EINVAL;
        goto exit;
    }

    /* Check if control() is called during processing time */
    if(obj->inProgress == true)
    {
        retVal = DPU_CLASSIFIERPROC_EINPROGRESS;
        goto exit;
    }

    /* Control command handling */
    switch(cmd)
    {
        case DPU_ClassifierProc_Cmd_MacroDopplerMapScaleCfg:
        {
            float *scale = (float *)arg;
            if (argSize == obj->config.staticCfg.featureExtrModuleCfg.sceneryParams.numOccupancyBoxes * sizeof(float))
            {
                for (uint32_t i = 0; i < obj->config.staticCfg.featureExtrModuleCfg.sceneryParams.numOccupancyBoxes; i++)
                {
                    obj->config.staticCfg.cnnInputScale[i] = scale[i];
                }
                retVal = 0;
            }
            else
            {
                retVal = DPU_CLASSIFIERPROC_EINVAL;
            }
        }
        break;

        case DPU_ClassifierProc_Cmd_zOffsetsCfg:
        {
            float *zOffsets = (float *)arg;
            if (argSize == obj->config.staticCfg.featureExtrModuleCfg.sceneryParams.numOccupancyBoxes * sizeof(float))
            {
                retVal = featExtract_updateZoffsets(obj->featExtractHandle, zOffsets, obj->config.staticCfg.featureExtrModuleCfg.sceneryParams.numOccupancyBoxes);
                if (retVal != 0)
                {
                    retVal = DPU_CLASSIFIERPROC_EINVAL;
                }
            }
            else
            {
                retVal = DPU_CLASSIFIERPROC_EINVAL;
            }
        }
        break;

        default:
            retVal = DPU_CLASSIFIERPROC_ECMD;
            break;
    }
exit:
    return (retVal);
}

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
)
{
    classifierProcObj  *obj;
    int32_t             retVal = 0;

    /* Get trackerProc data object */
    obj = (classifierProcObj *)handle;

    /* Sanity Check */
    if(obj == NULL)
    {
        retVal = DPU_CLASSIFIERPROC_EINVAL;
        goto exit;
    }

exit:
    return (retVal);
}

