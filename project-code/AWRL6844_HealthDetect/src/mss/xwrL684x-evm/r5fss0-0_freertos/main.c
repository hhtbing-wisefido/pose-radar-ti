/*
 *  Copyright (C) 2018-2021 Texas Instruments Incorporated
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file main.c
 * @brief MSS FreeRTOS Entry Point
 *
 * Reference: AWRL6844_InCabin_Demos/src/mss/xwrL684x-evm/r5fss0-0_freertos/main.c
 * Adapted for: Health Detection three-layer architecture
 *
 * RTOS: FreeRTOS (L-SDK mandatory)
 * Note: L-SDK uses FreeRTOS, NOT TI-RTOS/BIOS
 *
 * Created: 2026-01-08
 */

/**************************************************************************
 *************************** Include Files ********************************
 **************************************************************************/

#include <stdlib.h>
#include <kernel/dpl/DebugP.h>
#include "ti_drivers_config.h"
#include "ti_board_config.h"
#include <FreeRTOS.h>
#include <task.h>

/**************************************************************************
 *************************** Macros ***************************************
 **************************************************************************/

#define MAIN_TASK_PRI  (configMAX_PRIORITIES - 1)
#define MAIN_TASK_SIZE (16384U / sizeof(configSTACK_DEPTH_TYPE))

/**************************************************************************
 *************************** Global Variables *****************************
 **************************************************************************/

StackType_t gMainTaskStack[MAIN_TASK_SIZE] __attribute__((aligned(32)));
StaticTask_t gMainTaskObj;
TaskHandle_t gMainTask;

/**************************************************************************
 *************************** External Functions ***************************
 **************************************************************************/

/**
 * @brief Health Detection main application entry
 * Implemented in health_detect_main.c
 */
extern void health_detect_main(void *args);

/**************************************************************************
 *************************** Local Functions ******************************
 **************************************************************************/

/**
 * @brief FreeRTOS main task
 * 
 * This is the first task to run after FreeRTOS scheduler starts.
 * It initializes the Health Detection application and then deletes itself.
 */
void freertos_main(void *args)
{
    /* Enable FPU for floating point operations */
    vPortTaskUsesFPU();

    /* Call Health Detection main application */
    health_detect_main(NULL);

    /* Delete this task when application exits */
    vTaskDelete(NULL);
}

/**************************************************************************
 *************************** Main Entry Point *****************************
 **************************************************************************/

/**
 * @brief System entry point
 * 
 * Called by the bootloader after system reset.
 * Initializes system, creates main task, and starts FreeRTOS scheduler.
 */
int main(void)
{
    /* Initialize SOC specific modules */
    System_init();
    Board_init();

    /* Create main task at highest priority
     * This task will create other tasks and then delete itself
     */
    gMainTask = xTaskCreateStatic(
        freertos_main,              /* Pointer to the function that implements the task */
        "freertos_main",            /* Text name for the task (debugging only) */
        MAIN_TASK_SIZE,             /* Stack depth in units of StackType_t */
        NULL,                       /* Task parameter (not used) */
        MAIN_TASK_PRI,              /* Task priority (highest) */
        gMainTaskStack,             /* Pointer to stack base */
        &gMainTaskObj               /* Pointer to statically allocated task object */
    );
    configASSERT(gMainTask != NULL);

    /* Start the scheduler to begin task execution */
    vTaskStartScheduler();

    /* The following line should never be reached because vTaskStartScheduler()
     * will only return if there was not enough FreeRTOS heap memory available
     * to create the Idle and (if configured) Timer tasks.
     */
    DebugP_assertNoLog(0);

    return 0;
}
