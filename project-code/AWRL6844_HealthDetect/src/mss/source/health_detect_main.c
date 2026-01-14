/**
 * @file health_detect_main.c
 * @brief Health Detection Main Application Implementation
 *
 * Reference: mmw_demo_SDK_reference/source/mmwave_demo.c
 * Reference: AWRL6844_InCabin_Demos/src/mss/source/mmwave_demo_mss.c
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

/* Standard Includes */
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* FreeRTOS Includes */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

/* SDK DPL Includes */
#include <kernel/dpl/SemaphoreP.h>
#include <kernel/dpl/CacheP.h>
#include <kernel/dpl/ClockP.h>
#include <kernel/dpl/DebugP.h>
#include <kernel/dpl/HwiP.h>
#include <kernel/dpl/AddrTranslateP.h>

/* Driver Includes */
#include <drivers/uart.h>
#include <drivers/gpio.h>
#include <drivers/edma.h>
#include <drivers/hwa.h>

/* mmWave Control Includes */
#include <control/mmwave/mmwave.h>
#include <mmwavelink/include/rl_device.h>
#include <mmwavelink/include/rl_sensor.h>

/* SysConfig Generated Includes */
#include "ti_drivers_config.h"
#include "ti_drivers_open_close.h"
#include "ti_board_open_close.h"
#include "ti_board_config.h"

/* Application Includes */
#include <source/health_detect_main.h>
#include <source/cli.h>
#include <source/dpc_control.h>
#include <source/presence_detect.h>
#include <source/tlv_output.h>
#include <source/radar_control.h>
#include <common/shared_memory.h>

/**************************************************************************
 *************************** Global Definitions ***************************
 **************************************************************************/

/** @brief Global Master Control Block */
HealthDetect_MCB_t gHealthDetectMCB = {0};

/** @brief L3 RAM buffer for DPC */
uint8_t gMmwL3[L3_MSS_SIZE] __attribute__((section(".bss.l3")));

/** @brief Local RAM buffer for DPC */
#define MSS_CORE_LOCAL_MEM_SIZE     (28U * 1024U)
uint8_t gMmwCoreLocMem[MSS_CORE_LOCAL_MEM_SIZE];

/** @brief DPC Task Stack */
StackType_t gDpcTaskStack[DPC_TASK_STACK_SIZE / sizeof(StackType_t)] __attribute__((aligned(32)));
StaticTask_t gDpcTaskObj;

/** @brief TLV Task Stack */
StackType_t gTlvTaskStack[TLV_TASK_STACK_SIZE / sizeof(StackType_t)] __attribute__((aligned(32)));
StaticTask_t gTlvTaskObj;

/** @brief DPC Semaphore */
StaticSemaphore_t gDpcSemObj;

/** @brief TLV Semaphore */
StaticSemaphore_t gTlvSemObj;

/**************************************************************************
 *************************** Local Functions ******************************
 **************************************************************************/

/**
 * @brief Initialize default CLI configuration
 */
static void HealthDetect_initDefaultConfig(void)
{
    /* Initialize profile configuration */
    gHealthDetectMCB.cliCfg.profileCfg.profileId = 0;
    gHealthDetectMCB.cliCfg.profileCfg.startFreqGHz = 60.25f;
    gHealthDetectMCB.cliCfg.profileCfg.idleTimeUs = 7.0f;
    gHealthDetectMCB.cliCfg.profileCfg.adcStartTimeUs = 7.0f;
    gHealthDetectMCB.cliCfg.profileCfg.rampEndTimeUs = 60.0f;
    gHealthDetectMCB.cliCfg.profileCfg.freqSlopeConst = 60.0f;
    gHealthDetectMCB.cliCfg.profileCfg.numAdcSamples = 256;
    gHealthDetectMCB.cliCfg.profileCfg.digOutSampleRate = 5000;
    gHealthDetectMCB.cliCfg.profileCfg.rxGain = 30;

    /* Initialize frame configuration */
    gHealthDetectMCB.cliCfg.frameCfg.numChirpsPerFrame = 64;
    gHealthDetectMCB.cliCfg.frameCfg.numLoops = 1;
    gHealthDetectMCB.cliCfg.frameCfg.numFrames = 0; /* Infinite */
    gHealthDetectMCB.cliCfg.frameCfg.framePeriodMs = 100.0f;

    /* Initialize channel configuration */
    gHealthDetectMCB.cliCfg.rxChannelEn = 0x0F; /* All 4 RX enabled */
    gHealthDetectMCB.cliCfg.txChannelEn = 0x07; /* TX0, TX1, TX2 enabled */

    /* Initialize CFAR configuration */
    gHealthDetectMCB.cliCfg.cfarRangeCfg.config.cfarMethod = 2; /* CASO */
    gHealthDetectMCB.cliCfg.cfarRangeCfg.config.guardLen = 4;
    gHealthDetectMCB.cliCfg.cfarRangeCfg.config.noiseLen = 8;
    gHealthDetectMCB.cliCfg.cfarRangeCfg.config.thresholdScale = 15.0f;
    gHealthDetectMCB.cliCfg.cfarRangeCfg.minRangeBin = 2;
    gHealthDetectMCB.cliCfg.cfarRangeCfg.maxRangeBin = 250;

    gHealthDetectMCB.cliCfg.cfarDopplerCfg.config.cfarMethod = 2; /* CASO */
    gHealthDetectMCB.cliCfg.cfarDopplerCfg.config.guardLen = 2;
    gHealthDetectMCB.cliCfg.cfarDopplerCfg.config.noiseLen = 4;
    gHealthDetectMCB.cliCfg.cfarDopplerCfg.config.thresholdScale = 12.0f;

    /* Initialize presence detection configuration */
    gHealthDetectMCB.cliCfg.presenceCfg.minPointsForPresence = 5;
    gHealthDetectMCB.cliCfg.presenceCfg.minRange_m = 0.5f;
    gHealthDetectMCB.cliCfg.presenceCfg.maxRange_m = 3.0f;
    gHealthDetectMCB.cliCfg.presenceCfg.minVelocityThresh_mps = 0.1f;
    gHealthDetectMCB.cliCfg.presenceCfg.presenceHoldFrames = 10;
    gHealthDetectMCB.cliCfg.presenceCfg.snrThreshold_dB = 10.0f;
}

/**
 * @brief Create FreeRTOS tasks
 * @return 0 on success, error code on failure
 */
static int32_t HealthDetect_createTasks(void)
{
    /* Create DPC semaphore */
    gHealthDetectMCB.dpcSemHandle = xSemaphoreCreateBinaryStatic(&gDpcSemObj);
    if (gHealthDetectMCB.dpcSemHandle == NULL)
    {
        DebugP_log("Error: Failed to create DPC semaphore\r\n");
        return -1;
    }

    /* Create TLV semaphore */
    gHealthDetectMCB.tlvSemHandle = xSemaphoreCreateBinaryStatic(&gTlvSemObj);
    if (gHealthDetectMCB.tlvSemHandle == NULL)
    {
        DebugP_log("Error: Failed to create TLV semaphore\r\n");
        return -1;
    }

    /* Create DPC task */
    gHealthDetectMCB.dpcTaskHandle = xTaskCreateStatic(
        HealthDetect_dpcTask,
        "DpcTask",
        DPC_TASK_STACK_SIZE / sizeof(StackType_t),
        NULL,
        DPC_TASK_PRIORITY,
        gDpcTaskStack,
        &gDpcTaskObj
    );
    if (gHealthDetectMCB.dpcTaskHandle == NULL)
    {
        DebugP_log("Error: Failed to create DPC task\r\n");
        return -1;
    }

    /* Create TLV task */
    gHealthDetectMCB.tlvTaskHandle = xTaskCreateStatic(
        HealthDetect_tlvTask,
        "TlvTask",
        TLV_TASK_STACK_SIZE / sizeof(StackType_t),
        NULL,
        TLV_TASK_PRIORITY,
        gTlvTaskStack,
        &gTlvTaskObj
    );
    if (gHealthDetectMCB.tlvTaskHandle == NULL)
    {
        DebugP_log("Error: Failed to create TLV task\r\n");
        return -1;
    }

    return 0;
}

/**************************************************************************
 *************************** Public Functions *****************************
 **************************************************************************/

/**
 * @brief Debug assert handler
 */
void HealthDetect_debugAssert(int32_t expression, const char *file, int32_t line)
{
    if (!expression)
    {
        DebugP_log("ASSERT FAILED: %s, line %d\r\n", file, line);
        /* Halt in debug mode */
        while (1)
        {
            /* Infinite loop */
        }
    }
}

/**
 * @brief Frame start callback
 */
void HealthDetect_frameStartCallback(void *arg)
{
    /* Increment frame counter */
    gHealthDetectMCB.frameNum++;

    /* Signal DPC task to start processing */
    if (gHealthDetectMCB.dpcSemHandle != NULL)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(gHealthDetectMCB.dpcSemHandle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/**
 * @brief Frame stop callback
 */
void HealthDetect_frameStopCallback(void *arg)
{
    /* Frame processing complete - can be used for stats */
}

/**
 * @brief Initialize the Health Detection application
 */
int32_t HealthDetect_init(void)
{
    int32_t status = 0;

    /* ======== 问题29修复: 添加驱动初始化 ======== */
    /* Open peripheral drivers (UART, GPIO, etc.) */
    Drivers_open();
    Board_driversOpen();

    DebugP_log("Health Detection: Initializing...\r\n");

    /* Clear MCB */
    memset(&gHealthDetectMCB, 0, sizeof(HealthDetect_MCB_t));

    /* ======== 问题29修复: 设置UART句柄 ======== */
    /* Set UART handles for CLI and data output */
    gHealthDetectMCB.commandUartHandle = gUartHandle[0];      /* CLI/Command UART */
    gHealthDetectMCB.loggingUartHandle = gUartHandle[1];   /* Logging UART (if configured) */

    /* Set initial state */
    gHealthDetectMCB.state = HEALTH_DETECT_STATE_INIT;

    /* Initialize default configuration */
    HealthDetect_initDefaultConfig();

    /* Setup memory buffers */
    gHealthDetectMCB.l3RamPtr = gMmwL3;
    gHealthDetectMCB.l3RamSize = sizeof(gMmwL3);
    gHealthDetectMCB.localRamPtr = gMmwCoreLocMem;
    gHealthDetectMCB.localRamSize = sizeof(gMmwCoreLocMem);

    /* Initialize radar control (mmWave API) */
    status = RadarControl_init();
    if (status != 0)
    {
        DebugP_log("Error: RadarControl_init failed [%d]\r\n", status);
        gHealthDetectMCB.state = HEALTH_DETECT_STATE_ERROR;
        return status;
    }

    /* Initialize DPC control */
    status = DPC_init();
    if (status != 0)
    {
        DebugP_log("Error: DPC_init failed [%d]\r\n", status);
        gHealthDetectMCB.state = HEALTH_DETECT_STATE_ERROR;
        return status;
    }

    /* Initialize presence detection */
    status = PresenceDetect_init(&gHealthDetectMCB.cliCfg.presenceCfg);
    if (status != 0)
    {
        DebugP_log("Error: PresenceDetect_init failed [%d]\r\n", status);
        gHealthDetectMCB.state = HEALTH_DETECT_STATE_ERROR;
        return status;
    }

    /* Initialize TLV output */
    status = TLV_init(gHealthDetectMCB.commandUartHandle);
    if (status != 0)
    {
        DebugP_log("Error: TLV_init failed [%d]\r\n", status);
        gHealthDetectMCB.state = HEALTH_DETECT_STATE_ERROR;
        return status;
    }

    /* Create FreeRTOS tasks */
    status = HealthDetect_createTasks();
    if (status != 0)
    {
        DebugP_log("Error: Task creation failed [%d]\r\n", status);
        gHealthDetectMCB.state = HEALTH_DETECT_STATE_ERROR;
        return status;
    }

    /* Initialize CLI */
    status = CLI_init();
    if (status != 0)
    {
        DebugP_log("Error: CLI_init failed [%d]\r\n", status);
        gHealthDetectMCB.state = HEALTH_DETECT_STATE_ERROR;
        return status;
    }

    /* Update state */
    gHealthDetectMCB.state = HEALTH_DETECT_STATE_IDLE;

    DebugP_log("Health Detection: Initialization complete\r\n");
    DebugP_log("Entering CLI mode...\r\n");

    return 0;
}

/**
 * @brief Start the sensor
 */
int32_t HealthDetect_sensorStart(void)
{
    int32_t status = 0;

    if (gHealthDetectMCB.isSensorStarted)
    {
        DebugP_log("Warning: Sensor already started\r\n");
        return 0;
    }

    DebugP_log("Health Detection: Starting sensor...\r\n");

    /* Configure radar with CLI settings */
    status = RadarControl_config(&gHealthDetectMCB.cliCfg);
    if (status != 0)
    {
        DebugP_log("Error: RadarControl_config failed [%d]\r\n", status);
        return status;
    }

    /* Configure DPC */
    DPC_Config_t dpcCfg;
    memset(&dpcCfg, 0, sizeof(DPC_Config_t));
    dpcCfg.staticCfg.profile = gHealthDetectMCB.cliCfg.profileCfg;
    dpcCfg.staticCfg.chirp = gHealthDetectMCB.cliCfg.chirpCfg;
    dpcCfg.staticCfg.frame = gHealthDetectMCB.cliCfg.frameCfg;
    dpcCfg.dynamicCfg.cfarRangeCfg = gHealthDetectMCB.cliCfg.cfarRangeCfg;
    dpcCfg.dynamicCfg.cfarDopplerCfg = gHealthDetectMCB.cliCfg.cfarDopplerCfg;
    dpcCfg.dynamicCfg.aoaCfg = gHealthDetectMCB.cliCfg.aoaCfg;
    dpcCfg.isValid = 1;

    status = DPC_config(&dpcCfg);
    if (status != 0)
    {
        DebugP_log("Error: DPC_config failed [%d]\r\n", status);
        return status;
    }

    /* Start radar */
    status = RadarControl_start();
    if (status != 0)
    {
        DebugP_log("Error: RadarControl_start failed [%d]\r\n", status);
        return status;
    }

    /* Update state */
    gHealthDetectMCB.isSensorStarted = 1;
    gHealthDetectMCB.isSensorStopped = 0;
    gHealthDetectMCB.frameNum = 0;
    gHealthDetectMCB.state = HEALTH_DETECT_STATE_RUNNING;

    DebugP_log("Health Detection: Sensor started\r\n");

    return 0;
}

/**
 * @brief Stop the sensor
 */
int32_t HealthDetect_sensorStop(void)
{
    int32_t status = 0;

    if (!gHealthDetectMCB.isSensorStarted)
    {
        DebugP_log("Warning: Sensor not running\r\n");
        return 0;
    }

    DebugP_log("Health Detection: Stopping sensor...\r\n");

    /* Stop radar */
    status = RadarControl_stop();
    if (status != 0)
    {
        DebugP_log("Error: RadarControl_stop failed [%d]\r\n", status);
        return status;
    }

    /* Update state */
    gHealthDetectMCB.isSensorStarted = 0;
    gHealthDetectMCB.isSensorStopped = 1;
    gHealthDetectMCB.state = HEALTH_DETECT_STATE_STOPPED;

    DebugP_log("Health Detection: Sensor stopped\r\n");

    return 0;
}

/**
 * @brief DPC processing task
 */
void HealthDetect_dpcTask(void *args)
{
    uint32_t startTimeUs;

    DebugP_log("DPC Task: Started\r\n");

    while (1)
    {
        /* Wait for frame start signal */
        xSemaphoreTake(gHealthDetectMCB.dpcSemHandle, portMAX_DELAY);

        if (gHealthDetectMCB.isSensorStopped)
        {
            continue;
        }

        /* Record start time */
        startTimeUs = ClockP_getTimeUsec();

        /* Execute DPC processing */
        DPC_execute(&gHealthDetectMCB.dpcResult);

        /* Run presence detection on point cloud */
        if (gHealthDetectMCB.dpcResult.pointCloud.numDetectedPoints > 0)
        {
            PresenceDetect_process(
                gHealthDetectMCB.dpcResult.pointCloud.points,
                gHealthDetectMCB.dpcResult.pointCloud.numDetectedPoints,
                &gHealthDetectMCB.presenceResult
            );
        }

        /* Calculate processing time */
        gHealthDetectMCB.interFrameTimeUs = ClockP_getTimeUsec() - startTimeUs;

        /* Signal TLV task to transmit results */
        xSemaphoreGive(gHealthDetectMCB.tlvSemHandle);
    }
}

/**
 * @brief TLV output task
 */
void HealthDetect_tlvTask(void *args)
{
    uint32_t startTimeUs;

    DebugP_log("TLV Task: Started\r\n");

    while (1)
    {
        /* Wait for DPC completion signal */
        xSemaphoreTake(gHealthDetectMCB.tlvSemHandle, portMAX_DELAY);

        if (gHealthDetectMCB.isSensorStopped)
        {
            continue;
        }

        /* Record start time */
        startTimeUs = ClockP_getTimeUsec();

        /* Transmit output data via UART */
        TLV_sendOutput(
            gHealthDetectMCB.frameNum,
            &gHealthDetectMCB.dpcResult,
            &gHealthDetectMCB.presenceResult,
            &gHealthDetectMCB.healthFeatures
        );

        /* Calculate transmit time */
        gHealthDetectMCB.transmitTimeUs = ClockP_getTimeUsec() - startTimeUs;
    }
}

/**
 * @brief Main application entry point
 */
void health_detect_main(void *args)
{
    int32_t status;

    /* Enable FPU for floating point operations */
    vPortTaskUsesFPU();

    /* Initialize the application */
    status = HealthDetect_init();
    if (status != 0)
    {
        DebugP_log("Error: Application initialization failed\r\n");
        return;
    }

    /* Run CLI task (blocking) */
    CLI_run();

    /* Should not reach here */
    DebugP_log("Error: CLI exited unexpectedly\r\n");
}
