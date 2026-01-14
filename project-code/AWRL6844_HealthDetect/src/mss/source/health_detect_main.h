/**
 * @file health_detect_main.h
 * @brief Health Detection Main Application Header
 *
 * Reference: mmw_demo_SDK_reference/source/mmwave_demo.h
 * Adapted for: Health Detection three-layer architecture
 *
 * RTOS: FreeRTOS (L-SDK mandatory)
 * Note: L-SDK uses FreeRTOS, NOT TI-RTOS/BIOS
 *
 * Created: 2026-01-08
 * Updated: 2026-01-14 - MCB结构完全对齐SDK标准（问题36修复）
 */

#ifndef HEALTH_DETECT_MAIN_H
#define HEALTH_DETECT_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* FreeRTOS Includes */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

/* SDK DPL Includes */
#include <kernel/dpl/SemaphoreP.h>
#include <kernel/dpl/ClockP.h>
#include <kernel/dpl/DebugP.h>

/* mmWave SDK Includes */
#include <control/mmwave/mmwave.h>
#include <control/mmwavelink/mmwavelink.h>  /* For T_RL_API_FECSS_FACT_CAL_DATA */
#include <drivers/uart.h>
#include <drivers/hwa.h>
#include <drivers/edma.h>
#include <drivers/adcbuf.h>

/* Common Includes */
#include "common/data_path.h"
#include "common/health_detect_types.h"
#include "common/mmwave_output.h"

/* CLI Includes - 需要CLI相关的结构定义 */
#include <common/syscommon.h>

/*===========================================================================*/
/*                         Task Configuration                                 */
/*===========================================================================*/

/** @brief CLI Task Priority */
#define CLI_TASK_PRIORITY           (1U)

/** @brief DPC Task Priority */
#define DPC_TASK_PRIORITY           (5U)

/** @brief TLV Output Task Priority */
#define TLV_TASK_PRIORITY           (3U)

/** @brief Main Task Priority */
#define MAIN_TASK_PRIORITY          (configMAX_PRIORITIES - 1)

/** @brief CLI Task Stack Size */
#define CLI_TASK_STACK_SIZE         (4096U)

/** @brief DPC Task Stack Size */
#define DPC_TASK_STACK_SIZE         (8192U)

/** @brief TLV Task Stack Size */
#define TLV_TASK_STACK_SIZE         (2048U)

/** @brief Main Task Stack Size */
#define MAIN_TASK_STACK_SIZE        (16384U / sizeof(StackType_t))

/*===========================================================================*/
/*                         APLL Configuration                                 */
/*===========================================================================*/

/* Note: APLL_FREQ_400MHZ and APLL_FREQ_396MHZ are already defined in SDK mmwave.h */
/* We only define our custom SAVE/RESTORE modes here */

/** @brief APLL校准数据 - 保存模式 */
#define SAVE_APLL_CALIB_DATA        (1U)

/** @brief APLL校准数据 - 恢复模式 */
#define RESTORE_APLL_CALIB_DATA     (0U)

/*===========================================================================*/
/*                         System State                                       */
/*===========================================================================*/

/**
 * @brief System State Enumeration
 */
typedef enum HealthDetect_State_e
{
    HEALTH_DETECT_STATE_INIT = 0,       /**< Initialization state */
    HEALTH_DETECT_STATE_IDLE,           /**< Idle, waiting for config */
    HEALTH_DETECT_STATE_CONFIGURED,     /**< Configured, ready to start */
    HEALTH_DETECT_STATE_RUNNING,        /**< Sensor running */
    HEALTH_DETECT_STATE_STOPPED,        /**< Sensor stopped */
    HEALTH_DETECT_STATE_ERROR,          /**< Error state */
    HEALTH_DETECT_STATE_MAX
} HealthDetect_State_e;

/*===========================================================================*/
/*                         CLI Configuration Types                            */
/*===========================================================================*/

/**
 * @brief GUI Monitor Selection Structure (L-SDK standard)
 */
typedef struct CLI_GuiMonSel_t
{
    uint8_t     pointCloud;             /**< Enable point cloud output */
    uint8_t     rangeProfile;           /**< Enable range profile output */
    uint8_t     noiseProfile;           /**< Enable noise profile output */
    uint8_t     rangeAzimuthHeatMap;    /**< Enable range-azimuth heatmap */
    uint8_t     rangeDopplerHeatMap;    /**< Enable range-Doppler heatmap */
    uint8_t     statsInfo;              /**< Enable statistics info */
} CLI_GuiMonSel;

/**
 * @brief CFAR configuration structure
 */
typedef struct CLI_CfarCfg_t
{
    uint8_t     averageMode;            /**< Average mode */
    uint8_t     winLen;                 /**< Window length */
    uint8_t     guardLen;               /**< Guard length */
    uint8_t     noiseDiv;               /**< Noise divisor shift */
    uint8_t     cyclicMode;             /**< Cyclic mode */
    float       thresholdScale;         /**< Threshold scale */
    uint8_t     peakGroupingEn;         /**< Peak grouping enable */
} CLI_CfarCfg;

/**
 * @brief CFAR FOV configuration
 */
typedef struct CLI_CfarFovCfg_t
{
    float       min;                    /**< Minimum value (meters or m/s) */
    float       max;                    /**< Maximum value (meters or m/s) */
} CLI_CfarFovCfg;

/**
 * @brief AOA configuration structure
 */
typedef struct CLI_AoaCfg_t
{
    uint16_t    numAngleBins;           /**< Number of angle bins */
    float       elevSearchStep;         /**< Elevation search step */
    float       azimSearchStep;         /**< Azimuth search step */
    float       elevMin;                /**< Minimum elevation */
    float       elevMax;                /**< Maximum elevation */
    float       azimMin;                /**< Minimum azimuth */
    float       azimMax;                /**< Maximum azimuth */
} CLI_AoaCfg;

/**
 * @brief APLL Calibration Result
 * 
 * Note: SDK API MMWave_GetApllCalResult/MMWave_SetApllCalResult expects uint32_t*
 * The APLL calibration result is a single uint32_t value in L-SDK 6.x
 */
typedef uint32_t APLL_CalResult;

/*===========================================================================*/
/*                         Master Control Block (对齐SDK标准)                 */
/*===========================================================================*/

/**
 * @brief Health Detection Master Control Block
 * 
 * 参考: mmw_demo_SDK_reference/source/mmwave_demo.h MmwDemo_MSS_MCB
 * 说明: 保持与SDK标准MCB高度一致，确保所有SDK API调用正确
 */
typedef struct HealthDetect_MCB_t
{
    /*! ========== UART Handles (SDK标准) ========== */
    
    /*! @brief UART Logging Handle */
    UART_Handle                 loggingUartHandle;

    /*! @brief UART Command Rx/Tx Handle (CLI使用) */
    UART_Handle                 commandUartHandle;

    /*! ========== mmWave Control (SDK标准) ========== */
    
    /*! @brief mmWave control handle (用于配置BSS) */
    MMWave_Handle               ctrlHandle;

    /*! @brief ADC buffer handle */
    ADCBuf_Handle               adcBuffHandle;

    /*! @brief EDMA driver handle (用于CBUFF) */
    EDMA_Handle                 edmaHandle;

    /*! @brief Number of EDMA event Queues (tc) */
    uint8_t                     numEdmaEventQueues;

    /*! ========== Semaphore Objects (FreeRTOS) ========== */
    
    /*! @brief Semaphore Handle for DPC task (FreeRTOS) */
    SemaphoreHandle_t           dpcSemHandle;

    /*! @brief Semaphore Handle for TLV task (FreeRTOS) */
    SemaphoreHandle_t           tlvSemHandle;

    /*! 
     * @brief 注意：SDK使用DPL SemaphoreP_Object，但L-SDK FreeRTOS项目
     *        使用FreeRTOS原生SemaphoreHandle_t类型
     *        以下字段预留给未来CLI初始化等SDK标准流程使用
     */
    #if 0
    SemaphoreP_Object           demoInitTaskCompleteSemHandle;
    SemaphoreP_Object           cliInitTaskCompleteSemHandle;
    SemaphoreP_Object           dpcTaskConfigDoneSemHandle;
    SemaphoreP_Object           uartTaskConfigDoneSemHandle;
    #endif

    /*! ========== Sensor Control (SDK标准) ========== */
    
    /*! @brief Tracks the number of sensor start */
    uint32_t                    sensorStartCount;

    /*! @brief Tracks the number of sensor stop */
    uint32_t                    sensorStopCount;

    /*! @brief Sensor started flag */
    uint8_t                     isSensorStarted;

    /*! @brief Sensor stopped flag */
    uint8_t                     isSensorStopped;

    /*! ========== MMWave Configuration (SDK标准) ========== */
    
    /**
     * @brief MMWave configuration which includes frameCfg, profileTimeCfg, profileComCfg.
     */
    MMWave_Cfg                  mmWaveCfg;

    /**
     * @brief Configuration to open DFP
     */
    MMWave_OpenCfg              mmwOpenCfg;

    /*! ========== CLI Configuration (SDK标准 + HealthDetect扩展) ========== */
    
    /**
     * @brief 完整CLI配置结构（包含所有CLI命令配置）
     * 注意：为保持代码兼容性，保留cliCfg嵌套结构
     */
    struct {
        /* 雷达配置 */
        Profile_Config_t        profileCfg;         /**< Profile configuration */
        Chirp_Config_t          chirpCfg;           /**< Chirp configuration */
        Frame_Config_t          frameCfg;           /**< Frame configuration */

        /* 检测配置 */
        CFAR_Range_Config_t     cfarRangeCfg;       /**< CFAR range config */
        CFAR_Doppler_Config_t   cfarDopplerCfg;     /**< CFAR Doppler config */
        AOA_Config_t            aoaCfg;             /**< AOA config */

        /* 健康检测配置 */
        PresenceDetect_Config_t presenceCfg;        /**< Presence detection config */
        DetectionZone_t         zones[HEALTH_DETECT_MAX_ZONES]; /**< Detection zones */

        /* 通道配置 */
        uint16_t                rxChannelEn;        /**< RX channel enable mask */
        uint16_t                txChannelEn;        /**< TX channel enable mask */
        uint8_t                 numTxAntennas;      /**< Number of TX antennas */
        uint8_t                 numRxAntennas;      /**< Number of RX antennas */

        /* Sensor Start配置 (L-SDK standard) */
        uint8_t                 frameTrigMode;      /**< Frame trigger mode */
        uint8_t                 chirpStartSigLbEn;  /**< Chirp start signal loopback enable */
        uint8_t                 frameLivMonEn;      /**< Frame live monitor enable */
        uint32_t                frameTrigTimerVal;  /**< Frame trigger timer value */

        /* GUI Monitor配置 (L-SDK standard) */
        CLI_GuiMonSel           guiMonitor;         /**< GUI monitor selection */

        /* 配置标志 */
        uint8_t                 isProfileCfgPending;
        uint8_t                 isChirpCfgPending;
        uint8_t                 isFrameCfgPending;
        uint8_t                 isCfarCfgPending;
    } cliCfg;

    /*! ========== Radar Parameters (从配置计算得出) ========== */
    
    /*! @brief Chirping center frequency */
    float                       centerFreq;

    /*! @brief ADC sampling rate in MHz */
    float                       adcSamplingRate;

    /*! @brief Number of range bins */
    uint32_t                    numRangeBins;

    /*! @brief Number of Doppler bins */
    uint32_t                    numDopplerBins;

    /*! ========== APLL Configuration (SDK标准，问题36关键) ========== */
    
    /*! @brief APLL frequency shift enable flag */
    uint8_t                     apllFreqShiftEnable;

    /*! @brief Flag to control one-time configuration in low power mode */
    uint8_t                     oneTimeConfigDone;

    /*! @brief Default APLL calibration result (400MHz) */
    APLL_CalResult              defaultApllCalRes;

    /*! @brief Down-shifted APLL calibration result (396MHz) */
    APLL_CalResult              downShiftedApllCalRes;

    /*! ========== Factory Calibration (SDK标准，问题37关键) ========== */
    
    /*! @brief Factory calibration configuration (from CLI factoryCalibCfg command) */
    struct {
        uint8_t                 saveEnable;          /**< 1: Save calibration data to Flash */
        uint8_t                 restoreEnable;       /**< 1: Restore from Flash, 0: Perform new calibration */
        uint8_t                 rxGain;              /**< RX gain setting for calibration */
        uint8_t                 txBackoffSel;        /**< TX backoff code selection */
        uint32_t                flashOffset;         /**< Flash offset for calibration data */
        uint32_t                monitorsFlashOffset; /**< Flash offset for monitor data (optional) */
    } calibCfg;

    /*! @brief Factory calibration data buffer (allocated at init) */
    T_RL_API_FECSS_FACT_CAL_DATA factoryCalibData;

    /*! ========== Statistics and Timing ========== */
    
    uint32_t                    frameNum;           /**< Current frame number */
    uint32_t                    framePeriodUs;      /**< Measured frame period (us) */
    uint32_t                    interFrameTimeUs;   /**< Inter-frame processing time */
    uint32_t                    transmitTimeUs;     /**< Data transmit time */

    /*! ========== FreeRTOS Objects ========== */
    
    TaskHandle_t                dpcTaskHandle;      /**< DPC task handle */
    TaskHandle_t                tlvTaskHandle;      /**< TLV task handle */
    TaskHandle_t                cliTaskHandle;      /**< CLI task handle */

    /*! ========== Health Detection Results (运行时数据) ========== */
    
    /*! @brief Health detection features */
    HealthDetect_Features_t     healthFeatures;

    /*! @brief Presence detection result */
    PresenceDetect_Result_t     presenceResult;

    /*! @brief DPC processing result */
    DPC_Result_t                dpcResult;

    /*! ========== System State ========== */
    
    HealthDetect_State_e        state;              /**< Current system state */
    uint32_t                    errorCode;          /**< Last error code */

    /*! ========== Memory Buffers ========== */
    
    uint8_t                     *l3RamPtr;          /**< L3 RAM pointer */
    uint32_t                    l3RamSize;          /**< L3 RAM size */
    uint8_t                     *localRamPtr;       /**< Local RAM pointer */
    uint32_t                    localRamSize;       /**< Local RAM size */

} HealthDetect_MCB_t;

/*===========================================================================*/
/*                         Global Variables                                   */
/*===========================================================================*/

/** @brief Global Master Control Block */
extern HealthDetect_MCB_t gHealthDetectMCB;

/*===========================================================================*/
/*                         Function Prototypes                                */
/*===========================================================================*/

/**
 * @brief Main application entry point (called from FreeRTOS main task)
 * @param args Task arguments (unused)
 */
void health_detect_main(void *args);

/**
 * @brief Initialize the Health Detection application
 * @return 0 on success, error code on failure
 */
int32_t HealthDetect_init(void);

/**
 * @brief Start the sensor (参考SDK MmwStart流程)
 * @return 0 on success, error code on failure
 */
int32_t HealthDetect_sensorStart(void);

/**
 * @brief Stop the sensor
 * @return 0 on success, error code on failure
 */
int32_t HealthDetect_sensorStop(void);

/**
 * @brief Configure and Enable APLL (参考SDK MmwDemo_configAndEnableApll)
 * @param apllFreqMHz APLL frequency in MHz (400.0 or 396.0)
 * @param saveRestoreCalData SAVE_APLL_CALIB_DATA or RESTORE_APLL_CALIB_DATA
 * @return 0 on success, error code on failure
 */
int32_t HealthDetect_configAndEnableApll(float apllFreqMHz, uint8_t saveRestoreCalData);

/**
 * @brief DPC processing task
 * @param args Task arguments (unused)
 */
void HealthDetect_dpcTask(void *args);

/**
 * @brief TLV output task
 * @param args Task arguments (unused)
 */
void HealthDetect_tlvTask(void *args);

/**
 * @brief Frame start callback
 * @param arg Callback argument
 */
void HealthDetect_frameStartCallback(void *arg);

/**
 * @brief Frame stop callback
 * @param arg Callback argument
 */
void HealthDetect_frameStopCallback(void *arg);

/**
 * @brief Assert handler for debug
 * @param expression Expression that failed
 * @param file File name
 * @param line Line number
 */
void HealthDetect_debugAssert(int32_t expression, const char *file, int32_t line);

/*===========================================================================*/
/*                         Debug Macros                                       */
/*===========================================================================*/

/** @brief Debug assert macro */
#define HealthDetect_assert(expr) \
    HealthDetect_debugAssert((expr), __FILE__, __LINE__)

#ifdef __cplusplus
}
#endif

#endif /* HEALTH_DETECT_MAIN_H */
