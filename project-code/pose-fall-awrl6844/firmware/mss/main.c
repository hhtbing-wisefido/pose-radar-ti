/**
 * @file main.c
 * @brief MSS (Master Subsystem) 主入口 - FreeRTOS 版本
 * 
 * 适配 mmWave SDK 6.x (FreeRTOS)
 * 
 * @version 2.0.0
 * @date 2025-12-09
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* FreeRTOS */
#include "FreeRTOS.h"
#include "task.h"

/* SDK DPL (Driver Porting Layer) */
#include <kernel/dpl/DebugP.h>

/* Project Header files */
#include "pose_mss.h"

/*******************************************************************************
 * 任务配置
 ******************************************************************************/
#define MSS_TASK_PRIORITY       (configMAX_PRIORITIES - 2)
#define MSS_TASK_STACK_SIZE     (8192U / sizeof(configSTACK_DEPTH_TYPE))

static StackType_t gMssTaskStack[MSS_TASK_STACK_SIZE] __attribute__((aligned(32)));
static StaticTask_t gMssTaskObj;
static TaskHandle_t gMssTask;

/*******************************************************************************
 * MSS 主任务
 ******************************************************************************/
static void MSS_Task(void *args)
{
    PoseError_e retVal;
    
    /* 1. 初始化 MSS */
    retVal = MSS_init();
    if (retVal != POSE_OK) {
        DebugP_log("MSS Init Failed: %d\r\n", retVal);
        vTaskDelete(NULL);
        return;
    }
    
    DebugP_log("MSS Init OK\r\n");
    
    /* 2. 启动传感器 */
    retVal = MSS_sensorStart();
    if (retVal != POSE_OK) {
        DebugP_log("Sensor Start Failed: %d\r\n", retVal);
    }
    
    /* 3. 主循环 */
    while (1) {
        /* 等待帧就绪信号 (实际应使用信号量) */
        vTaskDelay(pdMS_TO_TICKS(100));  /* 100ms (10 FPS) */
        
        /* TODO: 处理帧数据 */
    }
}

/*******************************************************************************
 * 主入口
 ******************************************************************************/
int main(void)
{
    /* 1. 系统初始化 (由 SysConfig 生成的函数) */
    /* System_init(); */
    /* Board_init(); */
    
    /* 2. 创建 MSS 任务 */
    gMssTask = xTaskCreateStatic(
        MSS_Task,           /* 任务函数 */
        "MSS_Task",         /* 任务名称 */
        MSS_TASK_STACK_SIZE,/* 栈大小 */
        NULL,               /* 参数 */
        MSS_TASK_PRIORITY,  /* 优先级 */
        gMssTaskStack,      /* 栈内存 */
        &gMssTaskObj        /* 任务对象 */
    );
    
    configASSERT(gMssTask != NULL);
    
    /* 3. 启动调度器 */
    vTaskStartScheduler();
    
    /* 不应该到达这里 */
    DebugP_assertNoLog(0);
    
    return 0;
}
