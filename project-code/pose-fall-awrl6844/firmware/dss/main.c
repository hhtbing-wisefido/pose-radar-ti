/**
 * @file main.c
 * @brief DSS (DSP Subsystem) 主入口
 * 
 * 运行在 C674x DSP 上
 * 
 * @version 1.0.0
 * @date 2025-12-09
 */

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

/* BIOS / OS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

/* Project Header files */
#include "pose_dss.h"

/*******************************************************************************
 * 任务配置
 ******************************************************************************/
#define DSS_TASK_PRIORITY       1
#define DSS_TASK_STACK_SIZE     2048

Task_Struct dssTaskStruct;
Char dssTaskStack[DSS_TASK_STACK_SIZE];

/*******************************************************************************
 * DSS 主任务
 ******************************************************************************/
void DSS_Task(UArg arg0, UArg arg1)
{
    PoseError_e retVal;
    
    /* 1. 初始化 DSS */
    retVal = DSS_init();
    if (retVal != POSE_OK) {
        // System_printf("DSS Init Failed\n");
        BIOS_exit(0);
    }
    
    /* 2. 进入主循环 (处理 IPC 消息) */
    DSS_mainLoop();
}

/*******************************************************************************
 * 主入口
 ******************************************************************************/
int main(void)
{
    Task_Params taskParams;
    
    /* 创建 DSS 任务 */
    Task_Params_init(&taskParams);
    taskParams.priority = DSS_TASK_PRIORITY;
    taskParams.stackSize = DSS_TASK_STACK_SIZE;
    taskParams.stack = dssTaskStack;
    
    Task_construct(&dssTaskStruct, DSS_Task, &taskParams, NULL);
    
    /* 启动 BIOS */
    BIOS_start();
    
    return 0;
}
