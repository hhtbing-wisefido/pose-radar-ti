/**
 * @file pose_dss.c
 * @brief DSS (DSP Subsystem) 姿态检测模块实现
 * 
 * 运行在 C674x DSP 上
 * 
 * @version 1.0.0
 * @date 2025-12-09
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "pose_dss.h"

/* 
 * TODO: 包含 DSP SDK 头文件
 * #include <ti/control/mmwave/mmwave.h>
 * #include <ti/drivers/mailbox/mailbox.h>
 */

/*******************************************************************************
 * 全局变量
 ******************************************************************************/

MmwDemo_DSS_MCB gMmwDssMCB = {0};

/*******************************************************************************
 * DSS 初始化
 ******************************************************************************/

PoseError_e DSS_init(void)
{
    memset(&gMmwDssMCB, 0, sizeof(MmwDemo_DSS_MCB));
    gMmwDssMCB.state = DSS_STATE_INIT;
    
    /* 映射共享内存 */
    gMmwDssMCB.radarCube = (uint16_t*)IPC_RADAR_CUBE_ADDR;
    gMmwDssMCB.detMatrix = (uint16_t*)IPC_DET_MATRIX_ADDR;
    gMmwDssMCB.pointCloud = (PointCloud_t*)IPC_POINT_CLOUD_ADDR;
    
    /* TODO: 初始化 IPC Mailbox */
    
    gMmwDssMCB.state = DSS_STATE_IDLE;
    return POSE_OK;
}

/*******************************************************************************
 * DSS 主循环
 ******************************************************************************/

void DSS_mainLoop(void)
{
    PoseIpcMsg_t msg;
    
    while (1) {
        /* TODO: 等待 IPC 消息 (阻塞) */
        // Mailbox_read(gMmwDssMCB.mailboxHandle, &msg, sizeof(msg));
        
        DSS_processIPCMessage(&msg);
    }
}

/*******************************************************************************
 * 消息处理
 ******************************************************************************/

void DSS_processIPCMessage(const PoseIpcMsg_t* msg)
{
    if (msg == NULL) return;
    
    switch (msg->type) {
        case IPC_MSG_SENSOR_START:
            gMmwDssMCB.state = DSS_STATE_RUNNING;
            break;
            
        case IPC_MSG_SENSOR_STOP:
            gMmwDssMCB.state = DSS_STATE_STOPPED;
            break;
            
        case IPC_MSG_FRAME_START:
            /* 收到帧开始消息，执行数据处理 */
            DSS_processFrame(msg->frameNum);
            break;
            
        default:
            break;
    }
}

/*******************************************************************************
 * 帧处理 (DSP 加速)
 ******************************************************************************/

PoseError_e DSS_processFrame(uint32_t frameNum)
{
    if (gMmwDssMCB.state != DSS_STATE_RUNNING) {
        return POSE_ERROR_NOT_READY;
    }
    
    gMmwDssMCB.frameCount = frameNum;
    
    /* 
     * 在双核架构下，DSP 可以分担以下任务：
     * 1. Range FFT (如果 HWA 资源紧张)
     * 2. Doppler FFT (目前 HWA 做不到 2D FFT)
     * 3. CFAR 检测 (如果需要更复杂的算法)
     */
    
    /* TODO: 调用 DSP 优化的 DPU 进行处理 */
    
    /* 发送完成消息给 MSS */
    PoseIpcMsg_t doneMsg;
    doneMsg.type = IPC_MSG_DETECTION_DONE;
    doneMsg.frameNum = frameNum;
    // Mailbox_write(gMmwDssMCB.mailboxHandle, &doneMsg, sizeof(doneMsg));
    
    return POSE_OK;
}
