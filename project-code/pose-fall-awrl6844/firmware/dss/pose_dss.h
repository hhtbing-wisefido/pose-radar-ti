/**
 * @file pose_dss.h
 * @brief DSS (DSP Subsystem) 姿态检测模块头文件
 * 
 * 运行在 C674x DSP 上
 * 负责: 数据处理链加速 (Range FFT, CFAR)
 * 
 * @version 1.0.0
 * @date 2025-12-09
 */

#ifndef POSE_DSS_H_
#define POSE_DSS_H_

#include "../common/pose_types.h"
#include "../common/pose_config.h"
#include "../common/pose_ipc.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * DSS 状态定义
 ******************************************************************************/

typedef enum {
    DSS_STATE_INIT = 0,         /**< 初始化状态 */
    DSS_STATE_IDLE,             /**< 空闲状态 */
    DSS_STATE_RUNNING,          /**< 运行状态 */
    DSS_STATE_STOPPED,          /**< 停止状态 */
    DSS_STATE_ERROR             /**< 错误状态 */
} DssState_e;

/*******************************************************************************
 * DSS 控制块结构
 ******************************************************************************/

/**
 * @brief DSS 主控制块
 */
typedef struct MmwDemo_DSS_MCB_t
{
    /* 系统状态 */
    DssState_e              state;              /**< 当前状态 */
    uint32_t                frameCount;         /**< 帧计数 */
    
    /* IPC 句柄 */
    // MmwDemo_IPC_Handle      ipcHandle;
    
    /* DPU 句柄 */
    // DPU_RangeProcDSP_Handle rangeProcHandle;
    // DPU_CFARCAProcDSP_Handle cfarProcHandle;
    
    /* L3 共享内存指针 */
    uint16_t*               radarCube;          /**< Range FFT 输出 */
    uint16_t*               detMatrix;          /**< 检测矩阵 */
    PointCloud_t*           pointCloud;         /**< 点云数据 */
    
} MmwDemo_DSS_MCB;

/*******************************************************************************
 * 全局变量声明
 ******************************************************************************/

extern MmwDemo_DSS_MCB gMmwDssMCB;

/*******************************************************************************
 * DSS 函数声明
 ******************************************************************************/

/**
 * @brief 初始化 DSS
 * @return POSE_OK 成功
 */
PoseError_e DSS_init(void);

/**
 * @brief DSS 主循环
 */
void DSS_mainLoop(void);

/**
 * @brief 处理 IPC 消息
 * @param[in] msg 接收到的消息
 */
void DSS_processIPCMessage(const PoseIpcMsg_t* msg);

/**
 * @brief 执行数据处理 (Range FFT + CFAR)
 * @param[in] frameNum 帧编号
 */
PoseError_e DSS_processFrame(uint32_t frameNum);

#ifdef __cplusplus
}
#endif

#endif /* POSE_DSS_H_ */
