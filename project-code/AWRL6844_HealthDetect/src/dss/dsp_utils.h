/**
 * @file dsp_utils.h
 * @brief DSP Utility Functions Header
 */

#ifndef DSP_UTILS_H
#define DSP_UTILS_H

#include <c6x.h>
#include <stdint.h>

/* Cache Operations */
void DSP_cacheInvalidate(void* addr, uint32_t size);
void DSP_cacheWriteback(void* addr, uint32_t size);

/* Memory Utilities */
void DSP_memzero(void* ptr, uint32_t size);
void DSP_memcpy(void* dst, const void* src, uint32_t size);

/* Cycle Counter */
uint32_t DSP_getCycleCount(void);
uint32_t DSP_getElapsedCycles(uint32_t startCycles);

#endif /* DSP_UTILS_H */
