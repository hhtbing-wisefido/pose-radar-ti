/**
 * @file pose_ipc.h
 * @brief IPC (Inter-Processor Communication) 协议定义
 * 
 * 定义 MSS (R4F) 和 DSS (DSP) 之间的通信消息和共享内存结构
 * 
 * @version 1.0.0
 * @date 2025-12-09
 */

#ifndef POSE_IPC_H_
#define POSE_IPC_H_

#include <stdint.h>
#include "pose_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * IPC 通道配置
 ******************************************************************************/

/* 消息类型 */
typedef enum {
    IPC_MSG_INVALID = 0,
    
    /* 系统控制 */
    IPC_MSG_SENSOR_START,       /**< 启动传感器 (MSS -> DSS) */
    IPC_MSG_SENSOR_STOP,        /**< 停止传感器 (MSS -> DSS) */
    
    /* 帧同步 */
    IPC_MSG_FRAME_START,        /**< 帧开始 (MSS -> DSS) */
    IPC_MSG_DETECTION_DONE,     /**< 检测完成 (DSS -> MSS) */
    
    /* 错误报告 */
    IPC_MSG_ERROR               /**< 错误报告 (Bidirectional) */
} PoseIpcMsgType_e;

/* 消息结构体 */
typedef struct {
    uint32_t type;              /**< 消息类型 (PoseIpcMsgType_e) */
    uint32_t frameNum;          /**< 帧编号 */
    uint32_t dataLen;           /**< 数据长度 */
    uint32_t payload[4];        /**< 简短数据载荷 */
} PoseIpcMsg_t;

/*******************************************************************************
 * 共享内存布局 (L3 RAM)
 * 
 * AWRL6844 L3 RAM: 256KB (0x51000000)
 ******************************************************************************/

/* 
 * 内存映射:
 * ---------------------------------------------------------
 * | 区域          | 大小   | 偏移       | 说明            |
 * |---------------|--------|------------|-----------------|
 * | Radar Cube    | 128KB  | 0x00000000 | Range FFT 输出  |
 * | Detection Mat | 32KB   | 0x00020000 | 检测矩阵        |
 * | Point Cloud   | 16KB   | 0x00028000 | CFAR 检测点     |
 * | Features      | 4KB    | 0x0002C000 | 提取的特征      |
 * | Reserved      | 76KB   | 0x0002D000 | 保留            |
 * ---------------------------------------------------------
 */

#define L3_RAM_BASE_ADDR            0x51000000U

/* Radar Cube (128KB) */
#define IPC_RADAR_CUBE_OFFSET       0x00000000U
#define IPC_RADAR_CUBE_SIZE         (128 * 1024)
#define IPC_RADAR_CUBE_ADDR         (L3_RAM_BASE_ADDR + IPC_RADAR_CUBE_OFFSET)

/* Detection Matrix (32KB) */
#define IPC_DET_MATRIX_OFFSET       0x00020000U
#define IPC_DET_MATRIX_SIZE         (32 * 1024)
#define IPC_DET_MATRIX_ADDR         (L3_RAM_BASE_ADDR + IPC_DET_MATRIX_OFFSET)

/* Point Cloud (16KB) */
#define IPC_POINT_CLOUD_OFFSET      0x00028000U
#define IPC_POINT_CLOUD_SIZE        (16 * 1024)
#define IPC_POINT_CLOUD_ADDR        (L3_RAM_BASE_ADDR + IPC_POINT_CLOUD_OFFSET)

/* Features (4KB) */
#define IPC_FEATURES_OFFSET         0x0002C000U
#define IPC_FEATURES_SIZE           (4 * 1024)
#define IPC_FEATURES_ADDR           (L3_RAM_BASE_ADDR + IPC_FEATURES_OFFSET)

#ifdef __cplusplus
}
#endif

#endif /* POSE_IPC_H_ */
