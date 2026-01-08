/*
 * Copyright (C) 2024 Texas Instruments Incorporated
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 *   Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the
 *   distribution.
 *
 *   Neither the name of Texas Instruments Incorporated nor the names of
 *   its contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file feature_extract.c
 * @brief Feature Extraction Module Implementation
 *
 * Reference: New functionality for Health Detection project
 * Adapted for: Health Detection three-layer architecture - DSS Layer
 *
 * This module extracts health-related features from radar point cloud data
 * running on the C66x DSP for optimal performance.
 *
 * Created: 2026-01-08
 */

/**************************************************************************
 *************************** Include Files ********************************
 **************************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* SDK DPL */
#include <kernel/dpl/CacheP.h>
#include <kernel/dpl/DebugP.h>

/* DSP Library (optional, for optimized math) */
#ifdef USE_DSPLIB
#include <ti/dsplib/dsplib.h>
#endif

/* Module Header */
#include "feature_extract.h"
#include "dsp_utils.h"

/**************************************************************************
 *************************** Local Definitions ****************************
 **************************************************************************/

/* Small value to avoid division by zero */
#define EPSILON     (1e-6f)

/**************************************************************************
 *************************** Local Functions ******************************
 **************************************************************************/

/**
 * @brief Compute statistics for an array of float values
 */
static void FeatureExtract_computeStats(const float *data, 
                                        uint16_t numPoints,
                                        StatisticsInfo_t *stats)
{
    uint16_t i;
    float sum = 0.0f;
    float sumSq = 0.0f;
    float minVal = 1e10f;
    float maxVal = -1e10f;
    
    if ((data == NULL) || (stats == NULL) || (numPoints == 0))
    {
        if (stats != NULL)
        {
            memset(stats, 0, sizeof(StatisticsInfo_t));
        }
        return;
    }
    
    /* First pass: compute sum, min, max */
    for (i = 0; i < numPoints; i++)
    {
        float val = data[i];
        sum += val;
        
        if (val < minVal) minVal = val;
        if (val > maxVal) maxVal = val;
    }
    
    stats->min = minVal;
    stats->max = maxVal;
    stats->mean = sum / (float)numPoints;
    
    /* Second pass: compute variance */
    for (i = 0; i < numPoints; i++)
    {
        float diff = data[i] - stats->mean;
        sumSq += diff * diff;
    }
    
    stats->variance = sumSq / (float)numPoints;
    stats->stdDev = sqrtf(stats->variance);
}

/**
 * @brief Compute motion energy from velocities
 */
static float FeatureExtract_computeMotionEnergy(const float *velocities,
                                                 uint16_t numPoints)
{
    uint16_t i;
    float energy = 0.0f;
    
    if ((velocities == NULL) || (numPoints == 0))
    {
        return 0.0f;
    }
    
    for (i = 0; i < numPoints; i++)
    {
        energy += velocities[i] * velocities[i];
    }
    
    return energy / (float)numPoints;
}

/**************************************************************************
 *************************** API Functions ********************************
 **************************************************************************/

void FeatureExtract_getDefaultConfig(FeatureExtract_Config_t *config)
{
    if (config == NULL)
    {
        return;
    }
    
    config->rangeMin_m = 0.3f;              /* 30cm minimum range */
    config->rangeMax_m = 5.0f;              /* 5m maximum range */
    config->velocityThreshold_mps = 0.05f;  /* 5cm/s velocity threshold */
    config->snrThreshold_dB = 10.0f;        /* 10dB SNR threshold */
    config->smoothingAlpha = FEATURE_EXTRACT_SMOOTHING_ALPHA;
    config->minPointsForStats = FEATURE_EXTRACT_MIN_POINTS;
}

int32_t FeatureExtract_init(FeatureExtract_Handle_t *handle,
                            const FeatureExtract_Config_t *config)
{
    if (handle == NULL)
    {
        return -1;
    }
    
    /* Clear handle */
    memset(handle, 0, sizeof(FeatureExtract_Handle_t));
    
    /* Set configuration */
    if (config != NULL)
    {
        memcpy(&handle->config, config, sizeof(FeatureExtract_Config_t));
    }
    else
    {
        FeatureExtract_getDefaultConfig(&handle->config);
    }
    
    /* Initialize state */
    handle->prevMotionEnergy = 0.0f;
    handle->frameCount = 0;
    handle->isInitialized = 1;
    
    DebugP_log("[FeatureExtract] Initialized with range [%.2f, %.2f]m\r\n",
               handle->config.rangeMin_m, handle->config.rangeMax_m);
    
    return 0;
}

int32_t FeatureExtract_process(FeatureExtract_Handle_t *handle,
                               const PointCloud_Point_t *pointCloud,
                               uint16_t numPoints,
                               FeatureExtract_Output_t *output)
{
    uint16_t i;
    uint16_t validCount = 0;
    uint16_t movingCount = 0;
    uint32_t startCycles, endCycles;
    
    /* Temporary arrays for statistics computation */
    float ranges[FEATURE_EXTRACT_MAX_POINTS];
    float velocities[FEATURE_EXTRACT_MAX_POINTS];
    float azimuths[FEATURE_EXTRACT_MAX_POINTS];
    float elevations[FEATURE_EXTRACT_MAX_POINTS];
    float snrs[FEATURE_EXTRACT_MAX_POINTS];
    
    /* Centroid accumulators */
    float sumX = 0.0f, sumY = 0.0f, sumZ = 0.0f;
    float minX = 1e10f, maxX = -1e10f;
    float minY = 1e10f, maxY = -1e10f;
    float minZ = 1e10f, maxZ = -1e10f;
    
    /* Peak tracking */
    float peakSnr = FEATURE_EXTRACT_INVALID_SNR;
    uint16_t peakIdx = 0;
    
    if ((handle == NULL) || (!handle->isInitialized))
    {
        return -1;
    }
    
    /* Start cycle count */
    startCycles = DSPUtils_getCycleCount();
    
    /* Clear output */
    memset(&handle->output, 0, sizeof(FeatureExtract_Output_t));
    
    /* Check for empty point cloud */
    if ((pointCloud == NULL) || (numPoints == 0))
    {
        handle->output.isValid = 0;
        handle->frameCount++;
        
        if (output != NULL)
        {
            memcpy(output, &handle->output, sizeof(FeatureExtract_Output_t));
        }
        return 0;
    }
    
    /* Filter and collect valid points */
    for (i = 0; (i < numPoints) && (validCount < FEATURE_EXTRACT_MAX_POINTS); i++)
    {
        const PointCloud_Point_t *pt = &pointCloud[i];
        float range = pt->range;
        float snr = pt->snr;
        
        /* Apply range filter */
        if ((range < handle->config.rangeMin_m) || 
            (range > handle->config.rangeMax_m))
        {
            continue;
        }
        
        /* Apply SNR filter */
        if (snr < handle->config.snrThreshold_dB)
        {
            continue;
        }
        
        /* Store valid point data */
        ranges[validCount] = range;
        velocities[validCount] = pt->velocity;
        azimuths[validCount] = pt->azimuth;
        elevations[validCount] = pt->elevation;
        snrs[validCount] = snr;
        
        /* Compute Cartesian coordinates for centroid */
        float cosEl = cosf(pt->elevation);
        float x = range * cosEl * sinf(pt->azimuth);
        float y = range * cosEl * cosf(pt->azimuth);
        float z = range * sinf(pt->elevation);
        
        sumX += x;
        sumY += y;
        sumZ += z;
        
        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
        if (z < minZ) minZ = z;
        if (z > maxZ) maxZ = z;
        
        /* Track peak SNR */
        if (snr > peakSnr)
        {
            peakSnr = snr;
            peakIdx = validCount;
        }
        
        /* Count moving points */
        if (fabsf(pt->velocity) > handle->config.velocityThreshold_mps)
        {
            movingCount++;
        }
        
        validCount++;
    }
    
    /* Store point counts */
    handle->output.numValidPoints = validCount;
    handle->output.numMovingPoints = movingCount;
    handle->output.numStaticPoints = validCount - movingCount;
    
    /* Compute statistics if enough points */
    if (validCount >= handle->config.minPointsForStats)
    {
        FeatureExtract_computeStats(ranges, validCount, &handle->output.rangeStats);
        FeatureExtract_computeStats(velocities, validCount, &handle->output.velocityStats);
        FeatureExtract_computeStats(azimuths, validCount, &handle->output.azimuthStats);
        FeatureExtract_computeStats(elevations, validCount, &handle->output.elevationStats);
        FeatureExtract_computeStats(snrs, validCount, &handle->output.snrStats);
        
        /* Compute centroid */
        handle->output.centroidX_m = sumX / (float)validCount;
        handle->output.centroidY_m = sumY / (float)validCount;
        handle->output.centroidZ_m = sumZ / (float)validCount;
        
        /* Compute spread */
        handle->output.spreadX_m = maxX - minX;
        handle->output.spreadY_m = maxY - minY;
        handle->output.spreadZ_m = maxZ - minZ;
        
        /* Peak features */
        handle->output.peakSnr_dB = peakSnr;
        handle->output.peakRange_m = ranges[peakIdx];
        handle->output.peakVelocity_mps = velocities[peakIdx];
        
        /* Motion energy */
        handle->output.motionEnergy = FeatureExtract_computeMotionEnergy(velocities, validCount);
        
        /* Smoothed motion energy (IIR filter) */
        handle->output.motionEnergySmoothed = 
            handle->config.smoothingAlpha * handle->output.motionEnergy +
            (1.0f - handle->config.smoothingAlpha) * handle->prevMotionEnergy;
        handle->prevMotionEnergy = handle->output.motionEnergySmoothed;
        
        /* Motion index (normalized 0-1) */
        float maxMotion = 1.0f;  /* Assume max velocity^2 = 1 m^2/s^2 */
        handle->output.motionIndex = handle->output.motionEnergySmoothed / maxMotion;
        if (handle->output.motionIndex > 1.0f)
        {
            handle->output.motionIndex = 1.0f;
        }
        
        handle->output.isValid = 1;
    }
    else
    {
        handle->output.isValid = 0;
    }
    
    /* End cycle count */
    endCycles = DSPUtils_getCycleCount();
    handle->output.processingCycles = endCycles - startCycles;
    
    /* Increment frame counter */
    handle->frameCount++;
    
    /* Copy output if requested */
    if (output != NULL)
    {
        memcpy(output, &handle->output, sizeof(FeatureExtract_Output_t));
    }
    
    return 0;
}

int32_t FeatureExtract_getOutput(const FeatureExtract_Handle_t *handle,
                                 FeatureExtract_Output_t *output)
{
    if ((handle == NULL) || (output == NULL))
    {
        return -1;
    }
    
    if (!handle->isInitialized)
    {
        return -2;
    }
    
    memcpy(output, &handle->output, sizeof(FeatureExtract_Output_t));
    return 0;
}

int32_t FeatureExtract_reset(FeatureExtract_Handle_t *handle)
{
    if (handle == NULL)
    {
        return -1;
    }
    
    handle->prevMotionEnergy = 0.0f;
    handle->frameCount = 0;
    memset(&handle->output, 0, sizeof(FeatureExtract_Output_t));
    
    DebugP_log("[FeatureExtract] Reset\r\n");
    
    return 0;
}

int32_t FeatureExtract_setConfig(FeatureExtract_Handle_t *handle,
                                 const FeatureExtract_Config_t *config)
{
    if ((handle == NULL) || (config == NULL))
    {
        return -1;
    }
    
    memcpy(&handle->config, config, sizeof(FeatureExtract_Config_t));
    
    DebugP_log("[FeatureExtract] Config updated: range [%.2f, %.2f]m\r\n",
               handle->config.rangeMin_m, handle->config.rangeMax_m);
    
    return 0;
}
