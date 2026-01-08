/**
 * @file dsp_utils.h
 * @brief DSP Utility Functions Header
 *
 * Reference: AWRL6844_InCabin_Demos DSS utilities
 * Adapted for: Health Detection three-layer architecture - DSS Layer
 *
 * This module provides DSP utility functions including:
 * - Cache operations
 * - Cycle counting for profiling
 * - Memory operations
 * - Math utilities
 *
 * Created: 2026-01-08
 */

#ifndef DSP_UTILS_H
#define DSP_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************
 *************************** Include Files ********************************
 **************************************************************************/

#include <stdint.h>
#include <stddef.h>

/**************************************************************************
 *************************** Macros ***************************************
 **************************************************************************/

/** C66x DSP clock frequency in MHz */
#define DSP_CLOCK_MHZ           (450U)

/** Convert cycles to microseconds */
#define DSP_CYCLES_TO_US(c)     ((c) / DSP_CLOCK_MHZ)

/** Convert cycles to milliseconds */
#define DSP_CYCLES_TO_MS(c)     ((c) / (DSP_CLOCK_MHZ * 1000U))

/** Cache line size in bytes */
#define CACHE_LINE_SIZE         (64U)

/** Align address to cache line */
#define CACHE_ALIGN(addr)       (((addr) + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1))

/** Check if address is cache aligned */
#define IS_CACHE_ALIGNED(addr)  (((uint32_t)(addr) & (CACHE_LINE_SIZE - 1)) == 0)

/**************************************************************************
 *************************** Type Definitions *****************************
 **************************************************************************/

/**
 * @brief Profiling statistics structure
 */
typedef struct DSPUtils_Profile_t
{
    uint32_t    startCycle;     /**< Start cycle count */
    uint32_t    endCycle;       /**< End cycle count */
    uint32_t    totalCycles;    /**< Total cycles */
    uint32_t    minCycles;      /**< Minimum cycles */
    uint32_t    maxCycles;      /**< Maximum cycles */
    uint32_t    avgCycles;      /**< Average cycles */
    uint32_t    count;          /**< Number of samples */
} DSPUtils_Profile_t;

/**************************************************************************
 *************************** Function Prototypes **************************
 **************************************************************************/

/*========================================================================*/
/*                          Cycle Counting                                 */
/*========================================================================*/

/**
 * @brief Get current DSP cycle count
 *
 * @return Current cycle count (32-bit, wraps around)
 */
uint32_t DSPUtils_getCycleCount(void);

/**
 * @brief Initialize profiling structure
 *
 * @param[out] profile   Pointer to profile structure
 */
void DSPUtils_profileInit(DSPUtils_Profile_t *profile);

/**
 * @brief Start profiling measurement
 *
 * @param[in,out] profile   Pointer to profile structure
 */
void DSPUtils_profileStart(DSPUtils_Profile_t *profile);

/**
 * @brief End profiling measurement and update statistics
 *
 * @param[in,out] profile   Pointer to profile structure
 */
void DSPUtils_profileEnd(DSPUtils_Profile_t *profile);

/**
 * @brief Get elapsed cycles between start and end
 *
 * @param[in] profile   Pointer to profile structure
 * @return Elapsed cycles
 */
uint32_t DSPUtils_profileGetElapsed(const DSPUtils_Profile_t *profile);

/*========================================================================*/
/*                          Cache Operations                               */
/*========================================================================*/

/**
 * @brief Invalidate data cache for a memory region
 *
 * Use before reading data that may have been modified by another core.
 *
 * @param[in] addr   Start address
 * @param[in] size   Size in bytes
 */
void DSPUtils_cacheInvalidate(void *addr, uint32_t size);

/**
 * @brief Write back data cache for a memory region
 *
 * Use after writing data that needs to be visible to another core.
 *
 * @param[in] addr   Start address
 * @param[in] size   Size in bytes
 */
void DSPUtils_cacheWriteBack(void *addr, uint32_t size);

/**
 * @brief Write back and invalidate data cache
 *
 * Combined operation for data that is both read and written.
 *
 * @param[in] addr   Start address
 * @param[in] size   Size in bytes
 */
void DSPUtils_cacheWriteBackInvalidate(void *addr, uint32_t size);

/*========================================================================*/
/*                          Memory Operations                              */
/*========================================================================*/

/**
 * @brief Fast memory copy using DMA (if available) or optimized memcpy
 *
 * @param[out] dst    Destination address
 * @param[in]  src    Source address
 * @param[in]  size   Size in bytes
 */
void DSPUtils_memCopy(void *dst, const void *src, uint32_t size);

/**
 * @brief Fast memory set
 *
 * @param[out] dst    Destination address
 * @param[in]  value  Value to set (byte)
 * @param[in]  size   Size in bytes
 */
void DSPUtils_memSet(void *dst, uint8_t value, uint32_t size);

/**
 * @brief Zero memory
 *
 * @param[out] dst    Destination address
 * @param[in]  size   Size in bytes
 */
void DSPUtils_memZero(void *dst, uint32_t size);

/*========================================================================*/
/*                          Math Utilities                                 */
/*========================================================================*/

/**
 * @brief Fast square root (single precision)
 *
 * @param[in] x   Input value
 * @return Square root of x
 */
float DSPUtils_sqrtf(float x);

/**
 * @brief Fast reciprocal square root
 *
 * @param[in] x   Input value
 * @return 1/sqrt(x)
 */
float DSPUtils_rsqrtf(float x);

/**
 * @brief Fast absolute value (float)
 *
 * @param[in] x   Input value
 * @return |x|
 */
float DSPUtils_fabsf(float x);

/**
 * @brief Clamp value to range
 *
 * @param[in] val   Input value
 * @param[in] min   Minimum value
 * @param[in] max   Maximum value
 * @return Clamped value
 */
float DSPUtils_clampf(float val, float min, float max);

/**
 * @brief Linear interpolation
 *
 * @param[in] a   Start value
 * @param[in] b   End value
 * @param[in] t   Interpolation factor (0-1)
 * @return Interpolated value
 */
float DSPUtils_lerpf(float a, float b, float t);

/*========================================================================*/
/*                          Timing Utilities                               */
/*========================================================================*/

/**
 * @brief Busy-wait delay in microseconds
 *
 * @param[in] us   Microseconds to wait
 */
void DSPUtils_delayUs(uint32_t us);

/**
 * @brief Busy-wait delay in cycles
 *
 * @param[in] cycles   Cycles to wait
 */
void DSPUtils_delayCycles(uint32_t cycles);

#ifdef __cplusplus
}
#endif

#endif /* DSP_UTILS_H */
