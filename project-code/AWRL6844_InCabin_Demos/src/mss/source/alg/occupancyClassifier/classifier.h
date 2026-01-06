/*! 
 *  \file   classifier.h
 *
 *  \brief  Header file for classifier module
 *
 * Copyright (C) 2024 Texas Instruments Incorporated - http://www.ti.com/ 
 * 
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
 *
*/

#ifndef CLASSIFIER_H
#define CLASSIFIER_H


#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <float.h>


/* Algorithm Error codes */
#define CLASSIFIER_ERRNO_BASE	(-9000)                         /* Base error code for algorithm */
#define CLASSIFIER_EOK			(0)                             /* Error code: No errors */
#define CLASSIFIER_EINVAL		(CLASSIFIER_ERRNO_BASE-1)       /* Error Code: Invalid argument */
#define CLASSIFIER_ENOTIMPL		(CLASSIFIER_ERRNO_BASE-2)       /* Error Code: Operation is not implemented */
#define CLASSIFIER_ENOMEM		(CLASSIFIER_ERRNO_BASE-3)       /* Error Code: Out of memory */


/* Classifier models supported in the module */
typedef enum
{
    SBR_EMPTY_OCCUPIED_MDL = 0,
    LPD_EMPTY_OCCUPIED_MDL,
    CPD_CHILD_ADULT_MDL,
    CPD_HEIGHT_ESTIMATION_MDL
} Classifier_classifierModels;


/* Module config */
typedef struct {
    int32_t classifier_model;    /* The classifier model to be created */
    int32_t num_features;        /* Number of features */
    int32_t num_classes;         /* Number of classes */

    float *scratchBuffer;               /* Scratch memory needed for computations. Keep NULL to request internal allocation */
    uint32_t scratchBufferSizeInBytes;  /* The bytes available in the scratch buffer. Keep 0 if internal allocation is requested */
} Classifier_moduleConfig;


/* Declarations of the classifier API functions */
extern void *classifier_create(Classifier_moduleConfig *config, int32_t *errCode);
extern void classifier_predict(void *handle, float *features, float *predictions);
extern void classifier_delete(void *handle);


/* Declarations of memory allocation functions. Call these functions to allocate or free memory */
extern void *classifier_malloc(uint32_t sizeInBytes);
extern void classifier_free(void *pFree, uint32_t sizeInBytes);


/* Cycle counter function */
#ifdef SUBSYS_MSS
#if defined (__GNUC__) && !defined(__ti__)
static inline uint32_t classifier_getCycleCount (void)
{
    uint32_t value;
    // Read CCNT Register
    asm volatile ("MRC p15, 0, %0, c9, c13, 0\t\n": "=r"(value));
    return value;
}
#else
#define classifier_getCycleCount() __MRC(15, 0, 9, 13, 0)
#endif
#endif


#ifdef __cplusplus
}
#endif

#endif /* CLASSIFIER_H */
