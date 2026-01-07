/**
 * @file dsp_utils.c
 * @brief DSP Utility Functions for C66x Core
 * 
 * Helper functions for DSP-specific operations.
 */

#include "dsp_utils.h"
#include <c6x.h>
#include <string.h>

/*----------------------------------------------------------------------------*/
/* Cache Operations                                                          */
/*----------------------------------------------------------------------------*/

/**
 * @brief Invalidate L1D cache for buffer
 * 
 * Must be called before reading DMA/shared memory data
 */
void DSP_cacheInvalidate(void* addr, uint32_t size)
{
    /* TODO: Call CACHE_invL1d() from TI BIOS */
    /* CACHE_invL1d(addr, size, CACHE_WAIT); */
}

/**
 * @brief Write-back L1D cache for buffer
 * 
 * Must be called after writing data to shared memory
 */
void DSP_cacheWriteback(void* addr, uint32_t size)
{
    /* TODO: Call CACHE_wbL1d() from TI BIOS */
    /* CACHE_wbL1d(addr, size, CACHE_WAIT); */
}

/*----------------------------------------------------------------------------*/
/* Memory Utilities                                                          */
/*----------------------------------------------------------------------------*/

/**
 * @brief Zero-initialize buffer (optimized for DSP)
 */
void DSP_memzero(void* ptr, uint32_t size)
{
    memset(ptr, 0, size);
}

/**
 * @brief Memory copy (optimized for DSP)
 */
void DSP_memcpy(void* dst, const void* src, uint32_t size)
{
    memcpy(dst, src, size);
}

/*----------------------------------------------------------------------------*/
/* Cycle Counter                                                             */
/*----------------------------------------------------------------------------*/

/**
 * @brief Get DSP cycle counter (TSCL)
 */
uint32_t DSP_getCycleCount(void)
{
    return TSCL;
}

/**
 * @brief Calculate elapsed cycles
 */
uint32_t DSP_getElapsedCycles(uint32_t startCycles)
{
    return TSCL - startCycles;
}
