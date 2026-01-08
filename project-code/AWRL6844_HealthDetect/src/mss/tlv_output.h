/**
 * @file tlv_output.h
 * @brief TLV Output Module Header
 */

#ifndef TLV_OUTPUT_H
#define TLV_OUTPUT_H

#include <stdint.h>
#include <data_path.h>
#include <health_detect_types.h>

/* TLV Output API */
int32_t TLV_Output_init(uint32_t uartInstance);

int32_t TLV_Output_sendFrame(
    uint32_t frameNum,
    const DPC_PointCloud_t* pointCloud,
    const HealthDetect_PointCloudFeatures_t* features,
    const DPC_Result_t* dpcResult);

void TLV_Output_deinit(void);

#endif /* TLV_OUTPUT_H */
