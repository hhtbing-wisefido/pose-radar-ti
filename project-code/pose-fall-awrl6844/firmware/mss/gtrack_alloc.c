/**
 * @file gtrack_alloc.c
 * @brief GTRACK 内存分配函数实现
 * 
 * GTRACK 库要求应用层提供 gtrack_alloc/gtrack_free 函数
 * 基于 FreeRTOS pvPortMalloc/vPortFree
 */

#include <stddef.h>
#include <FreeRTOS.h>
#include <portable.h>

/**
 * @brief GTRACK 内存分配
 * @param numElements 元素数量
 * @param sizeInBytes 每个元素大小（字节）
 * @return 分配的内存指针，失败返回 NULL
 */
void *gtrack_alloc(unsigned int numElements, unsigned int sizeInBytes)
{
    size_t totalSize = (size_t)numElements * (size_t)sizeInBytes;
    
    if (totalSize == 0) {
        return NULL;
    }
    
    /* 使用 FreeRTOS 堆分配 */
    return pvPortMalloc(totalSize);
}

/**
 * @brief GTRACK 内存释放
 * @param ptr 要释放的内存指针
 * @param sizeInBytes 大小（未使用，保持接口一致）
 */
void gtrack_free(void *ptr, unsigned int sizeInBytes)
{
    (void)sizeInBytes; /* 未使用 */
    
    if (ptr != NULL) {
        vPortFree(ptr);
    }
}
