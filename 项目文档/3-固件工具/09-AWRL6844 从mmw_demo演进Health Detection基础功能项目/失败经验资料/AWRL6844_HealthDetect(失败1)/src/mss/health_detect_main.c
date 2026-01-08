/**
 * @file health_detect_main.c
 * @brief Health Detection Main Control - MSS/R5F Application Entry
 * 
 * This is the NEW main control file following Chapter 3 architecture.
 * Reference: mmw_demo/source/mmwave_demo.c
 * Purpose: Radar control, DPC coordination, feature processing
 * 
 * RTOS: FreeRTOS (L-SDK mandatory)
 * Note: L-SDK uses FreeRTOS from the beginning, not migrated from BIOS
 */

#include "health_detect_main.h"
#include "../common/shared_memory.h"
#include "../common/data_path.h"
#include "../common/health_detect_types.h"
#include "../common/mmwave_output.h"

/* FreeRTOS includes - L-SDK mandatory RTOS */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

/* Standard includes */
#include <stdio.h>
#include <string.h>

/*----------------------------------------------------------------------------*/
/* Global Application Context                                                */
/*----------------------------------------------------------------------------*/

typedef struct HealthDetect_AppContext_t {
    /* DPC State */
    DPC_State_e dpcState;
    DPC_Config_t* dpcConfig;        /* @ DPC_CONFIG_BASE */
    DPC_Result_t* dpcResult;        /* @ DPC_RESULT_BASE */
    
    /* Data Buffers */
    DPC_PointCloud_t* pointCloud;   /* @ POINT_CLOUD_BASE */
    HealthDetect_PointCloudFeatures_t* features; /* @ FEATURE_DATA_BASE */
    
    /* Frame Counter */
    uint32_t frameCount;
    
    /* FreeRTOS Semaphores (migrated from BIOS) */
    SemaphoreHandle_t frameStartSem;
    SemaphoreHandle_t frameReadySem;
    
    /* FreeRTOS Task Handles */
    TaskHandle_t initTaskHandle;
    TaskHandle_t frameProcessTaskHandle;
    
} HealthDetect_AppContext_t;

static HealthDetect_AppContext_t gAppContext;

/*----------------------------------------------------------------------------*/
/* Function Declarations                                                     */
/*----------------------------------------------------------------------------*/

/* Task Functions (FreeRTOS signature: void func(void* pvParameters)) */
static void HealthDetect_initTask(void* pvParameters);
static void HealthDetect_frameProcessTask(void* pvParameters);

/* Initialization */
static int32_t HealthDetect_initSharedMemory(void);
static int32_t HealthDetect_initRadar(void);
static int32_t HealthDetect_configureDPC(void);

/* Frame Processing */
static void HealthDetect_frameStartCallback(uint32_t frameNum);
static void HealthDetect_processFrame(void);
static void HealthDetect_outputResults(void);

/*----------------------------------------------------------------------------*/
/* Main Entry Point                                                          */
/*----------------------------------------------------------------------------*/

/**
 * @brief Main function - FreeRTOS entry point
 * 
 * Reference: mmw_demo main.c (adapted for FreeRTOS)
 * Creates tasks and starts FreeRTOS scheduler
 */
int main(void)
{
    BaseType_t xReturned;
    
    /* Initialize platform (SOC, pinmux, etc) - from SDK */
    printf("AWRL6844 Health Detection Starting...\n");
    
    /* Create Initialization Task (FreeRTOS style) */
    xReturned = xTaskCreate(
        HealthDetect_initTask,          /* Task function */
        "InitTask",                     /* Task name (debug) */
        configMINIMAL_STACK_SIZE + 1024,/* Stack size in words */
        NULL,                           /* Task parameter */
        tskIDLE_PRIORITY + 10,          /* Priority */
        &gAppContext.initTaskHandle     /* Task handle */
    );
    
    if (xReturned != pdPASS) {
        printf("ERROR: Failed to create init task\n");
        return -1;
    }
    
    /* Start FreeRTOS scheduler - never returns */
    vTaskStartScheduler();
    
    /* Should never reach here */
    return 0;
}

/*----------------------------------------------------------------------------*/
/* Initialization Task                                                       */
/*----------------------------------------------------------------------------*/

/**
 * @brief Initialization task (FreeRTOS)
 * 
 * Learned from: mmw_demo initialization sequence
 * Steps:
 *   1. Init shared memory
 *   2. Init radar (mmWave API)
 *   3. Configure DPC
 *   4. Create processing task
 */
static void HealthDetect_initTask(void* pvParameters)
{
    int32_t retVal;
    BaseType_t xReturned;
    
    printf("Init Task: Starting\n");
    
    /* Step 1: Initialize Shared Memory Pointers */
    retVal = HealthDetect_initSharedMemory();
    if (retVal < 0) {
        printf("ERROR: Shared memory init failed\n");
        vTaskDelete(NULL);
        return;
    }
    
    /* Step 2: Initialize Radar (mmWave API) */
    /* TODO: Call mmWave control module - learned from mmw_demo/mmwave_control */
    retVal = HealthDetect_initRadar();
    if (retVal < 0) {
        printf("ERROR: Radar init failed\n");
        vTaskDelete(NULL);
        return;
    }
    
    /* Step 3: Configure DPC */
    retVal = HealthDetect_configureDPC();
    if (retVal < 0) {
        printf("ERROR: DPC config failed\n");
        vTaskDelete(NULL);
        return;
    }
    
    /* Step 4: Create Semaphores for Frame Synchronization (FreeRTOS) */
    gAppContext.frameStartSem = xSemaphoreCreateBinary();
    gAppContext.frameReadySem = xSemaphoreCreateBinary();
    
    if (gAppContext.frameStartSem == NULL || gAppContext.frameReadySem == NULL) {
        printf("ERROR: Failed to create semaphores\n");
        vTaskDelete(NULL);
        return;
    }
    
    /* Step 5: Create Frame Processing Task (FreeRTOS) */
    xReturned = xTaskCreate(
        HealthDetect_frameProcessTask,      /* Task function */
        "FrameProcessTask",                 /* Task name */
        configMINIMAL_STACK_SIZE + 4096,    /* Stack size in words */
        NULL,                               /* Task parameter */
        tskIDLE_PRIORITY + 5,               /* Priority */
        &gAppContext.frameProcessTaskHandle /* Task handle */
    );
    
    if (xReturned != pdPASS) {
        printf("ERROR: Failed to create frame process task\n");
        vTaskDelete(NULL);
        return;
    }
    
    printf("Init Task: Complete\n");
    
    /* Initialization task exits - delete self (processing task takes over) */
    vTaskDelete(NULL);
}

/*----------------------------------------------------------------------------*/
/* Shared Memory Initialization                                              */
/*----------------------------------------------------------------------------*/

/**
 * @brief Map shared memory regions
 * 
 * Reference: InCabin demo shared memory setup
 */
static int32_t HealthDetect_initSharedMemory(void)
{
    /* Map L3 RAM regions to application pointers */
    gAppContext.dpcConfig = (DPC_Config_t*)DPC_CONFIG_BASE;
    gAppContext.dpcResult = (DPC_Result_t*)DPC_RESULT_BASE;
    gAppContext.pointCloud = (DPC_PointCloud_t*)POINT_CLOUD_BASE;
    gAppContext.features = (HealthDetect_PointCloudFeatures_t*)FEATURE_DATA_BASE;
    
    /* Validate addresses */
    if (!IS_VALID_L3_ADDRESS(gAppContext.dpcConfig)) {
        printf("ERROR: Invalid DPC config address\n");
        return -1;
    }
    
    /* Zero initialize */
    memset(gAppContext.dpcConfig, 0, sizeof(DPC_Config_t));
    memset(gAppContext.dpcResult, 0, sizeof(DPC_Result_t));
    
    gAppContext.frameCount = 0;
    gAppContext.dpcState = DPC_STATE_IDLE;
    
    printf("Shared Memory: DPC_CONFIG @ 0x%08x\n", (uint32_t)gAppContext.dpcConfig);
    printf("Shared Memory: POINT_CLOUD @ 0x%08x\n", (uint32_t)gAppContext.pointCloud);
    printf("Shared Memory: FEATURES @ 0x%08x\n", (uint32_t)gAppContext.features);
    
    return 0;
}

/*----------------------------------------------------------------------------*/
/* Radar Initialization (Placeholder)                                        */
/*----------------------------------------------------------------------------*/

/**
 * @brief Initialize radar using mmWave API
 * 
 * TODO: Implement using mmWave control module
 * Reference: mmw_demo/mmwave_control/mmwavecontrol.c
 */
static int32_t HealthDetect_initRadar(void)
{
    printf("Radar Init: TODO - mmWave API calls\n");
    
    /* TODO:
     * 1. MMWave_init()
     * 2. Configure chirp parameters
     * 3. Set frame config
     * 4. Register frame callback
     */
    
    return 0;
}

/*----------------------------------------------------------------------------*/
/* DPC Configuration                                                         */
/*----------------------------------------------------------------------------*/

/**
 * @brief Configure DPC parameters
 * 
 * Learned from: mmw_demo DPC configuration
 */
static int32_t HealthDetect_configureDPC(void)
{
    DPC_Config_t* cfg = gAppContext.dpcConfig;
    
    /* Frame Configuration */
    cfg->frameCount = 0;
    cfg->numChirpsPerFrame = 128;
    cfg->numRangeBins = 256;
    cfg->numDopplerBins = 128;
    
    /* Range CFAR */
    cfg->rangeCfar.averageMode = 0; /* CFAR-CA */
    cfg->rangeCfar.winLen = 8;
    cfg->rangeCfar.guardLen = 2;
    cfg->rangeCfar.noiseDivShift = 3;
    cfg->rangeCfar.thresholdScale = 15.0f;
    
    /* Doppler CFAR */
    cfg->dopplerCfar.averageMode = 0;
    cfg->dopplerCfar.winLen = 8;
    cfg->dopplerCfar.guardLen = 2;
    cfg->dopplerCfar.noiseDivShift = 3;
    cfg->dopplerCfar.thresholdScale = 15.0f;
    
    /* DOA Configuration */
    cfg->doaCfg.numVirtualAntennas = 8;
    cfg->doaCfg.numAntAzim = 4;
    cfg->doaCfg.numAntElev = 2;
    cfg->doaCfg.estResolution = 1.0f;
    cfg->doaCfg.gamma = 0.5f;
    cfg->doaCfg.multiPeakEn = false;
    
    /* Feature Extraction */
    cfg->featureExtractEn = true;
    cfg->minClusterSize = 3.0f; /* Minimum 3 points */
    
    gAppContext.dpcState = DPC_STATE_CONFIG;
    
    printf("DPC Config: Complete\n");
    return 0;
}

/*----------------------------------------------------------------------------*/
/* Frame Processing Task                                                     */
/*----------------------------------------------------------------------------*/

/**
 * @brief Frame processing task - runs continuously (FreeRTOS)
 * 
 * Reference: mmw_demo frame processing loop
 */
static void HealthDetect_frameProcessTask(void* pvParameters)
{
    printf("Frame Processing Task: Ready\n");
    
    gAppContext.dpcState = DPC_STATE_RUNNING;
    
    while (1) {
        /* Wait for frame start interrupt (from radar) - FreeRTOS style */
        if (xSemaphoreTake(gAppContext.frameStartSem, portMAX_DELAY) == pdTRUE) {
            /* Process frame */
            HealthDetect_processFrame();
            
            /* Output results to UART */
            HealthDetect_outputResults();
        }
    }
}

/**
 * @brief Frame start callback (ISR context)
 * Called when radar completes ADC capture
 * 
 * Note: FreeRTOS ISR-safe API must be used here
 */
static void HealthDetect_frameStartCallback(uint32_t frameNum)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    gAppContext.frameCount = frameNum;
    
    /* Use ISR-safe semaphore give (FreeRTOS) */
    xSemaphoreGiveFromISR(gAppContext.frameStartSem, &xHigherPriorityTaskWoken);
    
    /* Yield if a higher priority task was woken */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
 * @brief Process single frame
 * 
 * Steps:
 *   1. Trigger DSS DPC processing
 *   2. Wait for DSS completion
 *   3. Read point cloud and features from L3 RAM
 *   4. Run presence detection
 */
static void HealthDetect_processFrame(void)
{
    /* Step 1: Update DPC config frame number */
    gAppContext.dpcConfig->frameCount = gAppContext.frameCount;
    
    /* Step 2: Trigger DSS processing (IPC call) */
    /* TODO: Implement IPC to DSS - learned from InCabin demo */
    
    /* Step 3: Wait for DSS completion */
    /* TODO: Wait on semaphore posted by DSS mailbox handler */
    
    /* Step 4: Read results from L3 RAM */
    uint32_t numPoints = gAppContext.pointCloud->numPoints;
    
    printf("Frame %d: %d points detected\n", 
                  gAppContext.frameCount, numPoints);
    
    /* Step 5: Run presence detection on MSS */
    /* TODO: Call presence detection module */
}

/**
 * @brief Output results via UART TLV
 * 
 * Reference: mmw_demo TLV output
 */
static void HealthDetect_outputResults(void)
{
    /* TODO: Implement TLV output module
     * 1. Build TLV packet header
     * 2. Add TLV_TYPE_DETECTED_POINTS
     * 3. Add TLV_TYPE_HEALTH_FEATURES
     * 4. Send via UART
     */
}
