/*! 
 *  \file   classifier.h
 *
 *  \brief  Header file for classifier module
 *
 * Copyright (C) 2023 Texas Instruments Incorporated - http://www.ti.com/ 
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

#ifndef CNN_CLASSIFIER_H
#define CNN_CLASSIFIER_H


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


/* OSAL definition for MEX code generation. Comment out if needed (i.e., when the PC MEX is built) */
#define OSAL_MEX

/* Algorithm Error codes */
#define CNN_CLASSIFIER_ERRNO_BASE	(-9000)                         /* Base error code for CLASSIFIER algorithm */
#define CNN_CLASSIFIER_EOK			(0)                             /* Error code: No errors */
#define CNN_CLASSIFIER_EINVAL		(CNN_CLASSIFIER_ERRNO_BASE-1)       /* Error Code: Invalid argument */
#define CNN_CLASSIFIER_ENOMEM		(CNN_CLASSIFIER_ERRNO_BASE-2)       /* Error Code: Out of memory */


/* Module config */
typedef struct {
	int32_t num_frames;         /* Number of frames */
    int32_t num_features;       /* Number of features */
    int32_t num_classes;        /* Number of classes */

    float *scratchBuffer;   /* Scratch memory needed for computations. Keep NULL to request internal allocation */
    uint32_t scratchBufferSizeInBytes; /* Module needs (CNN_CLASSIFIER_BUFFER_SIZE_NEEDED * sizeof(float)) bytes */
} cnn_Classifier_moduleConfig;


/* Declarations of the classifier API functions */
extern void *cnn_classifier_create(cnn_Classifier_moduleConfig *config, int32_t *errCode);
extern void cnn_classifier_predict(void *handle, const float *features, float *predictions);
extern void cnn_classifier_delete(void *handle);
extern uint32_t cnn_classifier_bytes_needed(void);


/* Declarations of memory allocation functions. Call these functions to allocate or free memory */
void *cnn_classifier_malloc(uint32_t sizeInBytes);
void cnn_classifier_free(void *pFree, uint32_t sizeInBytes);


#ifdef __cplusplus
}
#endif

#endif /* CNN_CLASSIFIER_H */
