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
 * @file dsp_utils.c
 * @brief DSP Utility Functions Implementation
 *
 * Reference: AWRL6844_InCabin_Demos DSS utilities
 * Adapted for: Health Detection three-layer architecture - DSS Layer
 *
 * Created: 2026-01-08
 */

/**************************************************************************
 *************************** Include Files ********************************
 **************************************************************************/

#include <stdint.h>
#include <string.h>
#include <math.h>

/* SDK DPL */
#include <kernel/dpl/CacheP.h>

/* C66x Intrinsics */
#include <c6x.h>

/* Module Header */
#include "dsp_utils.h"

/**************************************************************************
 *************************** Cycle Counting *******************************
 **************************************************************************/

uint32_t DSPUtils_getCycleCount(void)
{
    /* Use C66x TSCL (Time Stamp Counter Low) register */
    return TSCL;
}

void DSPUtils_profileInit(DSPUtils_Profile_t *profile)
{
    if (profile == NULL)
    {
        return;
    }
    
    profile->startCycle = 0;
    profile->endCycle = 0;
    profile->totalCycles = 0;
    profile->minCycles = 0xFFFFFFFFU;
    profile->maxCycles = 0;
    profile->avgCycles = 0;
    profile->count = 0;
}

void DSPUtils_profileStart(DSPUtils_Profile_t *profile)
{
    if (profile != NULL)
    {
        profile->startCycle = DSPUtils_getCycleCount();
    }
}

void DSPUtils_profileEnd(DSPUtils_Profile_t *profile)
{
    uint32_t elapsed;
    
    if (profile == NULL)
    {
        return;
    }
    
    profile->endCycle = DSPUtils_getCycleCount();
    
    /* Handle wrap-around */
    if (profile->endCycle >= profile->startCycle)
    {
        elapsed = profile->endCycle - profile->startCycle;
    }
    else
    {
        elapsed = (0xFFFFFFFFU - profile->startCycle) + profile->endCycle + 1;
    }
    
    /* Update statistics */
    profile->totalCycles += elapsed;
    profile->count++;
    
    if (elapsed < profile->minCycles)
    {
        profile->minCycles = elapsed;
    }
    if (elapsed > profile->maxCycles)
    {
        profile->maxCycles = elapsed;
    }
    
    profile->avgCycles = profile->totalCycles / profile->count;
}

uint32_t DSPUtils_profileGetElapsed(const DSPUtils_Profile_t *profile)
{
    if (profile == NULL)
    {
        return 0;
    }
    
    if (profile->endCycle >= profile->startCycle)
    {
        return profile->endCycle - profile->startCycle;
    }
    else
    {
        return (0xFFFFFFFFU - profile->startCycle) + profile->endCycle + 1;
    }
}

/**************************************************************************
 *************************** Cache Operations *****************************
 **************************************************************************/

void DSPUtils_cacheInvalidate(void *addr, uint32_t size)
{
    if ((addr != NULL) && (size > 0))
    {
        CacheP_inv(addr, size, CacheP_TYPE_ALL);
    }
}

void DSPUtils_cacheWriteBack(void *addr, uint32_t size)
{
    if ((addr != NULL) && (size > 0))
    {
        CacheP_wb(addr, size, CacheP_TYPE_ALL);
    }
}

void DSPUtils_cacheWriteBackInvalidate(void *addr, uint32_t size)
{
    if ((addr != NULL) && (size > 0))
    {
        CacheP_wbInv(addr, size, CacheP_TYPE_ALL);
    }
}

/**************************************************************************
 *************************** Memory Operations ****************************
 **************************************************************************/

void DSPUtils_memCopy(void *dst, const void *src, uint32_t size)
{
    if ((dst != NULL) && (src != NULL) && (size > 0))
    {
        memcpy(dst, src, size);
    }
}

void DSPUtils_memSet(void *dst, uint8_t value, uint32_t size)
{
    if ((dst != NULL) && (size > 0))
    {
        memset(dst, value, size);
    }
}

void DSPUtils_memZero(void *dst, uint32_t size)
{
    if ((dst != NULL) && (size > 0))
    {
        memset(dst, 0, size);
    }
}

/**************************************************************************
 *************************** Math Utilities *******************************
 **************************************************************************/

float DSPUtils_sqrtf(float x)
{
    if (x <= 0.0f)
    {
        return 0.0f;
    }
    
    /* Use C66x reciprocal square root intrinsic for speed */
    float rsqrt = _rsqrsp(x);
    
    /* Newton-Raphson refinement for better accuracy */
    rsqrt = rsqrt * (1.5f - 0.5f * x * rsqrt * rsqrt);
    
    return x * rsqrt;
}

float DSPUtils_rsqrtf(float x)
{
    if (x <= 0.0f)
    {
        return 0.0f;
    }
    
    /* Use C66x reciprocal square root intrinsic */
    float rsqrt = _rsqrsp(x);
    
    /* Newton-Raphson refinement */
    rsqrt = rsqrt * (1.5f - 0.5f * x * rsqrt * rsqrt);
    
    return rsqrt;
}

float DSPUtils_fabsf(float x)
{
    return _fabsf(x);
}

float DSPUtils_clampf(float val, float min, float max)
{
    if (val < min)
    {
        return min;
    }
    else if (val > max)
    {
        return max;
    }
    return val;
}

float DSPUtils_lerpf(float a, float b, float t)
{
    return a + t * (b - a);
}

/**************************************************************************
 *************************** Timing Utilities *****************************
 **************************************************************************/

void DSPUtils_delayUs(uint32_t us)
{
    uint32_t cycles = us * DSP_CLOCK_MHZ;
    DSPUtils_delayCycles(cycles);
}

void DSPUtils_delayCycles(uint32_t cycles)
{
    uint32_t start = DSPUtils_getCycleCount();
    uint32_t elapsed;
    
    do
    {
        uint32_t now = DSPUtils_getCycleCount();
        if (now >= start)
        {
            elapsed = now - start;
        }
        else
        {
            elapsed = (0xFFFFFFFFU - start) + now + 1;
        }
    } while (elapsed < cycles);
}
