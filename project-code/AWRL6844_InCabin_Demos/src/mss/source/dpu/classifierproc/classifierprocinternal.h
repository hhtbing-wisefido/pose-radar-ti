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
#ifndef CLASSIFIERPROCINTERNAL_H
#define CLASSIFIERPROCINTERNAL_H

/* Standard Include Files. */
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* mmWave SDK Data Path Include Files */
#include <common/mmwave_error.h>


#include <source/dpu/classifierproc/classifierproc.h>

#ifdef __cplusplus
extern "C" {
#endif



/**
 * @brief
 *  ClassifierProc DPU Object
 *
 * @details
 *  The structure is used to hold ClassifierProc internal data object
 *
 */
typedef struct classifierProcObj_t
{
    
    /*! @brief  Feature extraction handle */
    void * featExtractHandle;

    /*! @brief  SBR classifier handle */
    void * sbrEmptyOccClassifierHandle;

    /*! @brief  LPD 1D-CNN (Empty/Occupied) classifier handle */
    void * lpd_cnn_EmptyOccClassifierHandle;
    /*! @brief  LPD (Empty/Occupied) classifier handle */
    void * lpdEmptyOccClassifierHandle;
    /*! @brief  CPD (Child/Adult) classifier handle */
    void * cpdChildAdultClassifierHandle;
    /*! @brief  CPD (Height estimation) classifier handle */
    void * cpdHeightEstClassifierHandle;

    /*! @brief  Classifier DPU configuration */
    DPU_ClassifierProc_Config config;

    /*! @brief  Number of occupance zones */
    uint8_t numOccupancyZones;


    /*! @brief   DPU is in processing in progress */
    bool inProgress;


} classifierProcObj;





#ifdef __cplusplus
}
#endif

#endif
