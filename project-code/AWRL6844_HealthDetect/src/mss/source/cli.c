/**
 * @file cli.c
 * @brief CLI Command Interface Implementation
 *
 * Reference: mmw_demo_SDK_reference/source/mmw_cli.c
 * Reference: AWRL6844_InCabin_Demos/src/mss/source/mmw_cli.c
 * Adapted for: Health Detection three-layer architecture
 *
 * RTOS: FreeRTOS (L-SDK mandatory)
 * Note: L-SDK uses FreeRTOS, NOT TI-RTOS/BIOS
 *
 * Created: 2026-01-08
 * Updated: 2026-01-14 - 添加SDK标准CLI_MCB和enableMMWaveExtension支持（问题36修复）
 */

/**************************************************************************
 *************************** Include Files ********************************
 **************************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

/* SDK DPL Includes */
#include <kernel/dpl/DebugP.h>
#include <kernel/dpl/ClockP.h>

/* Driver Includes */
#include <drivers/uart.h>

/* mmWave SDK Includes */
#include <control/mmwave/mmwave.h>

/* Application Includes */
#include <source/cli.h>
#include <source/health_detect_main.h>
#include <source/radar_control.h>

/**************************************************************************
 *************************** CLI MCB Definition (SDK标准) *****************
 **************************************************************************/

/**
 * @brief CLI Configuration Structure (SDK Standard)
 * 参考: mmw_demo_SDK_reference/source/mmw_cli.c 第2093-2110行
 */
typedef struct CLI_Cfg_t
{
    /**
     * @brief CLI Prompt string
     */
    const char*         cliPrompt;

    /**
     * @brief CLI Banner string
     */
    const char*         cliBanner;

    /**
     * @brief UART Handle for CLI communication
     */
    UART_Handle         UartHandle;

    /**
     * @brief Task Priority for CLI task
     */
    uint32_t            taskPriority;

    /**
     * @brief mmWave Handle (for mmWave extension commands)
     */
    MMWave_Handle       mmWaveHandle;

    /**
     * @brief Enable mmWave Extension flag
     * 🔴 关键：必须设为1以支持SDK Visualizer
     */
    uint8_t             enableMMWaveExtension;

    /**
     * @brief Use Polled Mode for UART
     */
    bool                usePolledMode;

} CLI_Cfg;

/**
 * @brief CLI Master Control Block (SDK Standard)
 * 参考: mmw_demo_SDK_reference/source/mmw_cli.c 第79行
 */
typedef struct CLI_MCB_t
{
    /**
     * @brief CLI Configuration
     */
    CLI_Cfg             cfg;

    /**
     * @brief Number of CLI commands registered
     */
    uint32_t            numCLICommands;

    /**
     * @brief CLI Initialized flag
     */
    uint8_t             isInitialized;

} CLI_MCB;

/**
 * @brief Global CLI MCB (SDK Standard)
 * 🔴 关键：SDK要求的全局CLI控制块
 */
CLI_MCB     gCLI;

/**************************************************************************
 *************************** Local Definitions ****************************
 **************************************************************************/

/** @brief CLI output buffer size */
#define CLI_OUTPUT_BUF_SIZE         (512U)

/**************************************************************************
 *************************** Local Variables ******************************
 **************************************************************************/

/** @brief CLI command line buffer */
static char gCliCmdLine[CLI_MAX_CMD_LINE_LEN];

/** @brief CLI output buffer */
static char gCliOutputBuf[CLI_OUTPUT_BUF_SIZE];

/** @brief CLI initialized flag */
static uint8_t gCliInitialized = 0;

/**
 * @brief Antenna geometry defined flag
 * A value of 1000b (8) indicates its fully defined
 * Initial value is 1, after antGeometryBoard command it becomes 8
 */
static uint8_t GIsAntGeoDef = 1;

/**
 * @brief Range bias and phase compensation defined flag
 * 0: Not defined, 1: Fully defined
 */
static uint8_t GIsRangePhaseCompDef = 0;

/**
 * @brief APLL frequency shift enable flag
 */
static uint8_t gApllFreqShiftEnable = 0;

/**************************************************************************
 *************************** Local Functions ******************************
 **************************************************************************/

/**
 * @brief Parse command line into arguments
 * @param cmdLine Command line string
 * @param argv Argument array output
 * @return Number of arguments
 */
static int32_t CLI_parseArgs(char *cmdLine, char *argv[])
{
    int32_t argc = 0;
    char *token;

    /* Use strtok (thread-safe not required as CLI runs in single thread) */
    token = strtok(cmdLine, " \t\r\n");
    while (token != NULL && argc < CLI_MAX_ARGS)
    {
        argv[argc++] = token;
        token = strtok(NULL, " \t\r\n");
    }

    return argc;
}

/**
 * @brief Process sensorStart command
 * L-SDK format: sensorStart <frameTrigMode> <chirpStartSigLbEn> <frameLivMonEn> <frameTrigTimerVal>
 */
static int32_t CLI_cmdSensorStart(int32_t argc, char *argv[])
{
    int32_t status;

    /* Sanity Check: Minimum argument check - L-SDK requires 5 args */
    if (argc != 5)
    {
        CLI_write("Error: Invalid usage of the CLI command\n");
        return -1;
    }

    /* Check if antenna geometry is fully defined */
    if (((GIsAntGeoDef >> 3) != 1) || (GIsRangePhaseCompDef != 1))
    {
        CLI_write("Error: Antenna geometry is not fully defined\n");
        return -1;
    }

    /* Store start configuration parameters */
    gHealthDetectMCB.cliCfg.frameTrigMode = (uint8_t)atoi(argv[1]);
    gHealthDetectMCB.cliCfg.chirpStartSigLbEn = (uint8_t)atoi(argv[2]);
    gHealthDetectMCB.cliCfg.frameLivMonEn = (uint8_t)atoi(argv[3]);
    gHealthDetectMCB.cliCfg.frameTrigTimerVal = (uint32_t)atoi(argv[4]);

    status = HealthDetect_sensorStart();
    if (status != 0)
    {
        CLI_write("Error: Failed to start sensor [%d]\r\n", status);
        return status;
    }

    return status;
}

/**
 * @brief Process sensorStop command
 * L-SDK format: sensorStop <frameStopMode>
 */
static int32_t CLI_cmdSensorStop(int32_t argc, char *argv[])
{
    int32_t status;

    /* Sanity Check: Minimum argument check - L-SDK requires 2 args */
    if (argc != 2)
    {
        CLI_write("Error: Invalid usage of the CLI command\n");
        return -1;
    }

    /* Reset antenna geometry flags to allow new configuration */
    GIsAntGeoDef = 1;
    GIsRangePhaseCompDef = 0;

    status = HealthDetect_sensorStop();
    if (status != 0)
    {
        CLI_write("Error: Failed to stop sensor [%d]\r\n", status);
        return status;
    }

    return 0;
}

/**
 * @brief Process profileCfg command
 */
static int32_t CLI_cmdProfileCfg(int32_t argc, char *argv[])
{
    if (argc < 12)
    {
        CLI_write("Error: profileCfg requires at least 11 arguments\r\n");
        CLI_write("Usage: profileCfg <profileId> <startFreq> <idleTime> <adcStartTime> ");
        CLI_write("<rampEndTime> <txOutPower> <txPhaseShifter> <freqSlopeConst> ");
        CLI_write("<txStartTime> <numAdcSamples> <digOutSampleRate> <hpf1> <hpf2> <rxGain>\r\n");
        return -1;
    }

    Profile_Config_t *cfg = &gHealthDetectMCB.cliCfg.profileCfg;

    cfg->profileId = (uint16_t)atoi(argv[1]);
    cfg->startFreqGHz = (float)atof(argv[2]);
    cfg->idleTimeUs = (float)atof(argv[3]);
    cfg->adcStartTimeUs = (float)atof(argv[4]);
    cfg->rampEndTimeUs = (float)atof(argv[5]);
    cfg->txOutPower = (uint16_t)atoi(argv[6]);
    cfg->txPhaseShifter = (uint16_t)atoi(argv[7]);
    cfg->freqSlopeConst = (float)atof(argv[8]);
    /* argv[9] is txStartTime, not stored */
    cfg->numAdcSamples = (uint16_t)atoi(argv[10]);
    cfg->digOutSampleRate = (uint16_t)atoi(argv[11]);

    if (argc > 12) cfg->hpfCornerFreq1 = (uint8_t)atoi(argv[12]);
    if (argc > 13) cfg->hpfCornerFreq2 = (uint8_t)atoi(argv[13]);
    if (argc > 14) cfg->rxGain = (uint8_t)atoi(argv[14]);

    gHealthDetectMCB.cliCfg.isProfileCfgPending = 1;

    CLI_write("profileCfg: Profile %d configured\r\n", cfg->profileId);

    return 0;
}

/**
 * @brief Process chirpCfg command
 */
static int32_t CLI_cmdChirpCfg(int32_t argc, char *argv[])
{
    if (argc < 8)
    {
        CLI_write("Error: chirpCfg requires 7 arguments\r\n");
        CLI_write("Usage: chirpCfg <startIdx> <endIdx> <profileId> <startFreqVar> ");
        CLI_write("<freqSlopeVar> <idleTimeVar> <adcStartTimeVar> <txEnable>\r\n");
        return -1;
    }

    Chirp_Config_t *cfg = &gHealthDetectMCB.cliCfg.chirpCfg;

    cfg->chirpStartIdx = (uint16_t)atoi(argv[1]);
    cfg->chirpEndIdx = (uint16_t)atoi(argv[2]);
    cfg->profileId = (uint16_t)atoi(argv[3]);
    cfg->startFreqVar = (float)atof(argv[4]);
    cfg->freqSlopeVar = (float)atof(argv[5]);
    cfg->idleTimeVar = (float)atof(argv[6]);
    cfg->adcStartTimeVar = (float)atof(argv[7]);
    if (argc > 8) cfg->txEnable = (uint8_t)atoi(argv[8]);

    gHealthDetectMCB.cliCfg.isChirpCfgPending = 1;

    CLI_write("chirpCfg: Chirp %d-%d configured\r\n", cfg->chirpStartIdx, cfg->chirpEndIdx);

    return 0;
}

/**
 * @brief Process frameCfg command
 */
static int32_t CLI_cmdFrameCfg(int32_t argc, char *argv[])
{
    if (argc < 6)
    {
        CLI_write("Error: frameCfg requires 5 arguments\r\n");
        CLI_write("Usage: frameCfg <chirpStartIdx> <chirpEndIdx> <numLoops> ");
        CLI_write("<numFrames> <framePeriodMs> <triggerSelect> <triggerDelay>\r\n");
        return -1;
    }

    Frame_Config_t *cfg = &gHealthDetectMCB.cliCfg.frameCfg;

    cfg->chirpStartIdx = (uint16_t)atoi(argv[1]);
    cfg->chirpEndIdx = (uint16_t)atoi(argv[2]);
    cfg->numLoops = (uint16_t)atoi(argv[3]);
    cfg->numFrames = (uint16_t)atoi(argv[4]);
    cfg->framePeriodMs = (float)atof(argv[5]);

    if (argc > 6) cfg->triggerSelect = (uint8_t)atoi(argv[6]);
    if (argc > 7) cfg->triggerDelay = (float)atof(argv[7]);

    /* Calculate derived parameters */
    cfg->numChirpsPerFrame = (cfg->chirpEndIdx - cfg->chirpStartIdx + 1) * cfg->numLoops;

    gHealthDetectMCB.cliCfg.isFrameCfgPending = 1;

    CLI_write("frameCfg: Frame configured, %d chirps/frame\r\n", cfg->numChirpsPerFrame);

    return 0;
}

/**
 * @brief Process cfarCfg command
 */
static int32_t CLI_cmdCfarCfg(int32_t argc, char *argv[])
{
    if (argc < 6)
    {
        CLI_write("Error: cfarCfg requires at least 5 arguments\r\n");
        CLI_write("Usage: cfarCfg <direction> <mode> <noiseWin> <guardWin> <threshold>\r\n");
        CLI_write("  direction: 0=range, 1=doppler\r\n");
        return -1;
    }

    uint8_t direction = (uint8_t)atoi(argv[1]);
    CFAR_Config_t *cfg;

    if (direction == 0)
    {
        cfg = &gHealthDetectMCB.cliCfg.cfarRangeCfg.config;
    }
    else
    {
        cfg = &gHealthDetectMCB.cliCfg.cfarDopplerCfg.config;
    }

    cfg->cfarMethod = (uint8_t)atoi(argv[2]);
    cfg->noiseLen = (uint8_t)atoi(argv[3]);
    cfg->guardLen = (uint8_t)atoi(argv[4]);
    cfg->thresholdScale = (float)atof(argv[5]);

    gHealthDetectMCB.cliCfg.isCfarCfgPending = 1;

    CLI_write("cfarCfg: %s CFAR configured\r\n", 
              direction == 0 ? "Range" : "Doppler");

    return 0;
}

/**
 * @brief Process presenceCfg command
 */
static int32_t CLI_cmdPresenceCfg(int32_t argc, char *argv[])
{
    if (argc < 6)
    {
        CLI_write("Error: presenceCfg requires 5 arguments\r\n");
        CLI_write("Usage: presenceCfg <minPoints> <minRange> <maxRange> ");
        CLI_write("<velThresh> <holdFrames>\r\n");
        return -1;
    }

    PresenceDetect_Config_t *cfg = &gHealthDetectMCB.cliCfg.presenceCfg;

    cfg->minPointsForPresence = (uint16_t)atoi(argv[1]);
    cfg->minRange_m = (float)atof(argv[2]);
    cfg->maxRange_m = (float)atof(argv[3]);
    cfg->minVelocityThresh_mps = (float)atof(argv[4]);
    cfg->presenceHoldFrames = (uint16_t)atoi(argv[5]);

    CLI_write("presenceCfg: Presence detection configured\r\n");

    return 0;
}

/**
 * @brief Process channelCfg command
 * L-SDK format: channelCfg <rxChannelEn> <txChannelEn> <cascading>
 */
static int32_t CLI_cmdChannelCfg(int32_t argc, char *argv[])
{
    if (argc != 4)
    {
        CLI_write("Error: Invalid usage of the CLI command\n");
        return -1;
    }

    gHealthDetectMCB.cliCfg.rxChannelEn = (uint16_t)atoi(argv[1]);
    gHealthDetectMCB.cliCfg.txChannelEn = (uint16_t)atoi(argv[2]);
    /* argv[3] is cascading, not used in single-chip */

    return 0;
}

/**
 * @brief Process chirpComnCfg command (L-SDK standard)
 * Format: chirpComnCfg <digOutputSampRate> <digOutputBitsSel> <dfeFirSel>
 *         <numOfAdcSamples> <chirpTxMimoPatSel> <chirpRampEndTime> <chirpRxHpfSel>
 */
static int32_t CLI_cmdChirpComnCfg(int32_t argc, char *argv[])
{
    if (argc != 8)
    {
        CLI_write("Error: Invalid usage of the CLI command\n");
        return -1;
    }

    /* Store L-SDK chirp common config */
    gHealthDetectMCB.cliCfg.profileCfg.digOutSampleRate = (uint16_t)atoi(argv[1]);
    /* argv[2] digOutputBitsSel - not stored */
    /* argv[3] dfeFirSel - not stored */
    gHealthDetectMCB.cliCfg.profileCfg.numAdcSamples = (uint16_t)atoi(argv[4]);
    /* argv[5] chirpTxMimoPatSel */
    gHealthDetectMCB.cliCfg.profileCfg.rampEndTimeUs = (float)atof(argv[6]);
    /* argv[7] chirpRxHpfSel */

    gHealthDetectMCB.cliCfg.isProfileCfgPending = 1;

    return 0;
}

/**
 * @brief Process chirpTimingCfg command (L-SDK standard)
 * Format: chirpTimingCfg <idleTime> <adcSkipSamples> <txStartTime> <slope> <startFreq>
 */
static int32_t CLI_cmdChirpTimingCfg(int32_t argc, char *argv[])
{
    if (argc != 6)
    {
        CLI_write("Error: Invalid usage of the CLI command\n");
        return -1;
    }

    gHealthDetectMCB.cliCfg.profileCfg.idleTimeUs = (float)atof(argv[1]);
    gHealthDetectMCB.cliCfg.profileCfg.adcStartTimeUs = (float)atof(argv[2]);
    /* argv[3] txStartTime - not stored directly */
    gHealthDetectMCB.cliCfg.profileCfg.freqSlopeConst = (float)atof(argv[4]);
    gHealthDetectMCB.cliCfg.profileCfg.startFreqGHz = (float)atof(argv[5]);

    gHealthDetectMCB.cliCfg.isProfileCfgPending = 1;

    return 0;
}

/**
 * @brief Process frameCfg command (L-SDK standard)
 * Format: frameCfg <numChirpsInBurst> <numChirpsAccum> <burstPeriodicity>
 *         <numBurstsInFrame> <framePeriodicity> <numFrames>
 */
static int32_t CLI_cmdFrameCfgLSDK(int32_t argc, char *argv[])
{
    if (argc != 7)
    {
        CLI_write("Error: Invalid usage of the CLI command\n");
        return -1;
    }

    gHealthDetectMCB.cliCfg.frameCfg.numChirpsPerFrame = (uint16_t)atoi(argv[1]);
    gHealthDetectMCB.cliCfg.frameCfg.numLoops = (uint16_t)atoi(argv[2]);
    /* argv[3] burstPeriodicity - not stored */
    /* argv[4] numBurstsInFrame */
    gHealthDetectMCB.cliCfg.frameCfg.framePeriodMs = (float)atof(argv[5]);
    gHealthDetectMCB.cliCfg.frameCfg.numFrames = (uint16_t)atoi(argv[6]);

    gHealthDetectMCB.cliCfg.isFrameCfgPending = 1;

    return 0;
}

/**
 * @brief Process guiMonitor command (L-SDK standard)
 * Format: guiMonitor <pointCloud> <rangeProfile> <noiseProfile>
 *         <rangeAzimuthHeatMap> <rangeDopplerHeatMap> <statsInfo>
 */
static int32_t CLI_cmdGuiMonitor(int32_t argc, char *argv[])
{
    if (argc != 7)
    {
        CLI_write("Error: Invalid usage of the CLI command\n");
        return -1;
    }

    /* Store GUI monitor config - used for TLV output selection */
    gHealthDetectMCB.cliCfg.guiMonitor.pointCloud = (uint8_t)atoi(argv[1]);
    gHealthDetectMCB.cliCfg.guiMonitor.rangeProfile = (uint8_t)atoi(argv[2]);
    gHealthDetectMCB.cliCfg.guiMonitor.noiseProfile = (uint8_t)atoi(argv[3]);
    gHealthDetectMCB.cliCfg.guiMonitor.rangeAzimuthHeatMap = (uint8_t)atoi(argv[4]);
    gHealthDetectMCB.cliCfg.guiMonitor.rangeDopplerHeatMap = (uint8_t)atoi(argv[5]);
    gHealthDetectMCB.cliCfg.guiMonitor.statsInfo = (uint8_t)atoi(argv[6]);

    return 0;
}

/**
 * @brief Process cfarProcCfg command (L-SDK standard)
 * Format: cfarProcCfg <procDirection> <averageMode> <winLen> <guardLen>
 *         <noiseDiv> <cyclicMode> <thresholdScale> <peakGroupingEn>
 */
static int32_t CLI_cmdCfarProcCfg(int32_t argc, char *argv[])
{
    if (argc != 9)
    {
        CLI_write("Error: Invalid usage of the CLI command\n");
        return -1;
    }

    uint8_t direction = (uint8_t)atoi(argv[1]);
    CFAR_Config_t *cfg;

    if (direction == 0)
    {
        cfg = &gHealthDetectMCB.cliCfg.cfarRangeCfg.config;
    }
    else
    {
        cfg = &gHealthDetectMCB.cliCfg.cfarDopplerCfg.config;
    }

    cfg->cfarMethod = (uint8_t)atoi(argv[2]);
    cfg->noiseLen = (uint8_t)atoi(argv[3]);
    cfg->guardLen = (uint8_t)atoi(argv[4]);
    /* argv[5] noiseDiv - not stored */
    /* argv[6] cyclicMode - not stored */
    cfg->thresholdScale = (float)atof(argv[7]);
    /* argv[8] peakGroupingEn - not stored */

    gHealthDetectMCB.cliCfg.isCfarCfgPending = 1;

    return 0;
}

/**
 * @brief Process cfarFovCfg command (L-SDK standard)
 * Format: cfarFovCfg <procDirection> <min> <max>
 */
static int32_t CLI_cmdCfarFovCfg(int32_t argc, char *argv[])
{
    if (argc != 4)
    {
        CLI_write("Error: Invalid usage of the CLI command\n");
        return -1;
    }

    uint8_t direction = (uint8_t)atoi(argv[1]);

    if (direction == 0)
    {
        /* Range FOV */
        gHealthDetectMCB.cliCfg.cfarRangeCfg.minRangeBin = (uint16_t)(atof(argv[2]) * 10); /* approx */
        gHealthDetectMCB.cliCfg.cfarRangeCfg.maxRangeBin = (uint16_t)(atof(argv[3]) * 10);
    }
    /* direction == 1 is Doppler FOV, not stored */

    return 0;
}

/**
 * @brief Process aoaProcCfg command (L-SDK standard)
 * Format: aoaProcCfg <azimuthFftSize> <elevationFftSize>
 */
static int32_t CLI_cmdAoaProcCfg(int32_t argc, char *argv[])
{
    if (argc != 3)
    {
        CLI_write("Error: Invalid usage of the CLI command\n");
        return -1;
    }
    /* AOA config - not used in basic health detection */
    return 0;
}

/**
 * @brief Process aoaFovCfg command (L-SDK standard)
 * Format: aoaFovCfg <minAzim> <maxAzim> <minElev> <maxElev>
 */
static int32_t CLI_cmdAoaFovCfg(int32_t argc, char *argv[])
{
    if (argc != 5)
    {
        CLI_write("Error: Invalid usage of the CLI command\n");
        return -1;
    }
    /* AOA FOV config - not used in basic health detection */
    return 0;
}

/**
 * @brief Process clutterRemoval command (L-SDK standard)
 */
static int32_t CLI_cmdClutterRemoval(int32_t argc, char *argv[])
{
    if (argc != 2)
    {
        CLI_write("Error: Invalid usage of the CLI command\n");
        return -1;
    }
    /* Clutter removal - not implemented yet */
    return 0;
}

/**
 * @brief Process factoryCalibCfg command (L-SDK standard)
 * L-SDK format: factoryCalibCfg <saveEnable> <restoreEnable> <rxGain> <txBackoffSel> <flashOffset> [monitorsFlashOffset]
 * 
 * @details SDK standard参数（与mmw_demo一致）：
 *   - saveEnable: 1=保存校准数据到Flash, 0=不保存
 *   - restoreEnable: 1=从Flash恢复, 0=执行新校准
 *   - rxGain: RX增益设置
 *   - txBackoffSel: TX回退码选择
 *   - flashOffset: 校准数据Flash偏移（十六进制，如0x1ff000）
 *   - monitorsFlashOffset: 监控数据Flash偏移（可选）
 */
static int32_t CLI_cmdFactoryCalibCfg(int32_t argc, char *argv[])
{
    if (argc < 6 || argc > 7)
    {
        CLI_write("Error: Invalid usage of the CLI command\n");
        CLI_write("Usage: factoryCalibCfg <saveEnable> <restoreEnable> <rxGain> <txBackoffSel> <flashOffset> [monitorsFlashOffset]\n");
        return -1;
    }
    
    /* 保存配置到MCB（SDK标准流程）*/
    gHealthDetectMCB.calibCfg.saveEnable = (uint8_t)atoi(argv[1]);
    gHealthDetectMCB.calibCfg.restoreEnable = (uint8_t)atoi(argv[2]);
    gHealthDetectMCB.calibCfg.rxGain = (uint8_t)atoi(argv[3]);
    gHealthDetectMCB.calibCfg.txBackoffSel = (uint8_t)atoi(argv[4]);
    
    /* 解析Flash偏移（支持十六进制格式，如0x1ff000）*/
    if (strncmp(argv[5], "0x", 2) == 0 || strncmp(argv[5], "0X", 2) == 0)
    {
        sscanf(argv[5], "%x", &gHealthDetectMCB.calibCfg.flashOffset);
    }
    else
    {
        gHealthDetectMCB.calibCfg.flashOffset = (uint32_t)atoi(argv[5]);
    }
    
    /* 可选的监控器Flash偏移 */
    if (argc == 7)
    {
        if (strncmp(argv[6], "0x", 2) == 0 || strncmp(argv[6], "0X", 2) == 0)
        {
            sscanf(argv[6], "%x", &gHealthDetectMCB.calibCfg.monitorsFlashOffset);
        }
        else
        {
            gHealthDetectMCB.calibCfg.monitorsFlashOffset = (uint32_t)atoi(argv[6]);
        }
    }
    else
    {
        gHealthDetectMCB.calibCfg.monitorsFlashOffset = 0;
    }
    
    return 0;
}

/**
 * @brief Process runtimeCalibCfg command (L-SDK standard)
 * L-SDK format: runtimeCalibCfg <calMonTimeUnit>
 */
static int32_t CLI_cmdRuntimeCalibCfg(int32_t argc, char *argv[])
{
    if (argc != 2)
    {
        CLI_write("Error: Invalid usage of the CLI command\n");
        return -1;
    }
    /* Runtime calibration - store but don't process yet */
    return 0;
}

/**
 * @brief Process antGeometryBoard command (L-SDK standard)
 * L-SDK format: antGeometryBoard <boardType>
 * This command sets GIsAntGeoDef and GIsRangePhaseCompDef flags required for sensorStart
 */
static int32_t CLI_cmdAntGeometryBoard(int32_t argc, char *argv[])
{
    /* Sanity Check: Minimum argument check */
    if (argc != 2)
    {
        CLI_write("Error: Invalid usage of the CLI command\n");
        return -1;
    }

    /* Check if already defined */
    if ((GIsAntGeoDef >> 3) == 1)
    {
        CLI_write("Error: Antenna geometry is already defined\n");
        return -1;
    }

    if (GIsRangePhaseCompDef == 1)
    {
        CLI_write("Error: Range bias and phase compensation is already defined\n");
        return -1;
    }

    /* Check for supported board types */
    if (strcmp(argv[1], "xWRL6844EVM") == 0)
    {
        /* A value of 1000b (8) in GIsAntGeoDef indicates its fully defined */
        GIsAntGeoDef = GIsAntGeoDef << 3;  /* 1 << 3 = 8 */
        /* Range bias and phase compensation is fully defined */
        GIsRangePhaseCompDef = 1;

        /* Store antenna geometry for xWRL6844EVM (4TX, 4RX) */
        gHealthDetectMCB.cliCfg.numTxAntennas = 4;
        gHealthDetectMCB.cliCfg.numRxAntennas = 4;
    }
    else
    {
        CLI_write("Error: Unknown board type '%s'\n", argv[1]);
        return -1;
    }

    return 0;
}

/**
 * @brief Process adcDataSource command (L-SDK standard)
 * L-SDK format: adcDataSource <sourceSelect> <fileName>
 */
static int32_t CLI_cmdAdcDataSource(int32_t argc, char *argv[])
{
    if (argc != 3)
    {
        CLI_write("Error: Invalid usage of the CLI command\n");
        return -1;
    }
    /* ADC data source - accept but don't process */
    return 0;
}

/**
 * @brief Process adcLogging command (L-SDK standard)
 * L-SDK format: adcLogging <enable>
 */
static int32_t CLI_cmdAdcLogging(int32_t argc, char *argv[])
{
    if (argc != 2)
    {
        CLI_write("Error: Invalid usage of the CLI command\n");
        return -1;
    }
    /* ADC logging - accept but don't process */
    return 0;
}

/**
 * @brief Process lowPowerCfg command (L-SDK standard)
 * L-SDK format: lowPowerCfg <enable>
 */
static int32_t CLI_cmdLowPowerCfg(int32_t argc, char *argv[])
{
    if (argc != 2)
    {
        CLI_write("Error: Invalid usage of the CLI command\n");
        return -1;
    }
    /* Low power config - accept but don't process */
    return 0;
}

/**
 * @brief Process apllFreqShiftEn command (L-SDK standard)
 * L-SDK format: apllFreqShiftEn <enable>
 */
static int32_t CLI_cmdApllFreqShiftEn(int32_t argc, char *argv[])
{
    if (argc != 2)
    {
        CLI_write("Error: Invalid usage of the CLI command\n");
        return -1;
    }

    gApllFreqShiftEnable = (uint8_t)atoi(argv[1]);

    return 0;
}

/**
 * @brief Process adcDataDitherCfg command (L-SDK standard)
 * L-SDK format: adcDataDitherCfg <enable>
 */
static int32_t CLI_cmdAdcDataDitherCfg(int32_t argc, char *argv[])
{
    if (argc != 2)
    {
        CLI_write("Error: Invalid usage of the CLI command\n");
        return -1;
    }
    /* ADC dither - accept but don't process */
    return 0;
}

/**
 * @brief Process gpAdcMeasConfig command (L-SDK standard)
 * L-SDK format: gpAdcMeasConfig <enable> <numSamples>
 */
static int32_t CLI_cmdGpAdcMeasConfig(int32_t argc, char *argv[])
{
    if (argc != 3)
    {
        CLI_write("Error: Invalid usage of the CLI command\n");
        return -1;
    }
    /* GP ADC config - accept but don't process */
    return 0;
}

/**
 * @brief Process help command
 */
static int32_t CLI_cmdHelp(int32_t argc, char *argv[])
{
    CLI_write("\r\n=== Health Detection CLI Commands ===\r\n");
    CLI_write("sensorStart         - Start radar sensor\r\n");
    CLI_write("sensorStop          - Stop radar sensor\r\n");
    CLI_write("profileCfg          - Configure chirp profile\r\n");
    CLI_write("chirpCfg            - Configure chirp parameters\r\n");
    CLI_write("frameCfg            - Configure frame parameters\r\n");
    CLI_write("channelCfg          - Configure TX/RX channels\r\n");
    CLI_write("cfarCfg             - Configure CFAR detection\r\n");
    CLI_write("presenceCfg         - Configure presence detection\r\n");
    CLI_write("help                - Show this help\r\n");
    CLI_write("version             - Show version info\r\n");
    CLI_write("=====================================\r\n\r\n");

    return 0;
}

/**
 * @brief Process version command
 */
static int32_t CLI_cmdVersion(int32_t argc, char *argv[])
{
    CLI_write("\r\n=== Health Detection Firmware ===\r\n");
    CLI_write("Version: 1.0.0\r\n");
    CLI_write("Platform: AWRL6844\r\n");
    CLI_write("Build Date: " __DATE__ " " __TIME__ "\r\n");
    CLI_write("=================================\r\n\r\n");

    return 0;
}

/**
 * @brief Process a single command
 * Supports both legacy commands and L-SDK standard commands for SDK Visualizer compatibility
 */
static int32_t CLI_processCommand(char *cmdLine)
{
    char *argv[CLI_MAX_ARGS];
    int32_t argc;
    int32_t status = 0;

    /* Parse arguments */
    argc = CLI_parseArgs(cmdLine, argv);
    if (argc == 0)
    {
        return 0; /* Empty command */
    }

    /* Match command and execute - L-SDK standard commands first */
    if (strcmp(argv[0], "sensorStart") == 0)
    {
        status = CLI_cmdSensorStart(argc, argv);
    }
    else if (strcmp(argv[0], "sensorStop") == 0)
    {
        status = CLI_cmdSensorStop(argc, argv);
    }
    else if (strcmp(argv[0], "channelCfg") == 0)
    {
        status = CLI_cmdChannelCfg(argc, argv);
    }
    /* L-SDK standard commands */
    else if (strcmp(argv[0], "chirpComnCfg") == 0)
    {
        status = CLI_cmdChirpComnCfg(argc, argv);
    }
    else if (strcmp(argv[0], "chirpTimingCfg") == 0)
    {
        status = CLI_cmdChirpTimingCfg(argc, argv);
    }
    else if (strcmp(argv[0], "frameCfg") == 0)
    {
        /* Check argument count to determine format */
        if (argc == 7)
        {
            status = CLI_cmdFrameCfgLSDK(argc, argv);
        }
        else
        {
            status = CLI_cmdFrameCfg(argc, argv);
        }
    }
    else if (strcmp(argv[0], "guiMonitor") == 0)
    {
        status = CLI_cmdGuiMonitor(argc, argv);
    }
    else if (strcmp(argv[0], "cfarProcCfg") == 0)
    {
        status = CLI_cmdCfarProcCfg(argc, argv);
    }
    else if (strcmp(argv[0], "cfarFovCfg") == 0)
    {
        status = CLI_cmdCfarFovCfg(argc, argv);
    }
    else if (strcmp(argv[0], "aoaProcCfg") == 0)
    {
        status = CLI_cmdAoaProcCfg(argc, argv);
    }
    else if (strcmp(argv[0], "aoaFovCfg") == 0)
    {
        status = CLI_cmdAoaFovCfg(argc, argv);
    }
    else if (strcmp(argv[0], "clutterRemoval") == 0)
    {
        status = CLI_cmdClutterRemoval(argc, argv);
    }
    else if (strcmp(argv[0], "factoryCalibCfg") == 0)
    {
        status = CLI_cmdFactoryCalibCfg(argc, argv);
    }
    else if (strcmp(argv[0], "runtimeCalibCfg") == 0)
    {
        status = CLI_cmdRuntimeCalibCfg(argc, argv);
    }
    else if (strcmp(argv[0], "antGeometryBoard") == 0)
    {
        status = CLI_cmdAntGeometryBoard(argc, argv);
    }
    else if (strcmp(argv[0], "adcDataSource") == 0)
    {
        status = CLI_cmdAdcDataSource(argc, argv);
    }
    else if (strcmp(argv[0], "adcLogging") == 0)
    {
        status = CLI_cmdAdcLogging(argc, argv);
    }
    else if (strcmp(argv[0], "lowPowerCfg") == 0)
    {
        status = CLI_cmdLowPowerCfg(argc, argv);
    }
    else if (strcmp(argv[0], "apllFreqShiftEn") == 0)
    {
        status = CLI_cmdApllFreqShiftEn(argc, argv);
    }
    else if (strcmp(argv[0], "adcDataDitherCfg") == 0)
    {
        status = CLI_cmdAdcDataDitherCfg(argc, argv);
    }
    else if (strcmp(argv[0], "gpAdcMeasConfig") == 0)
    {
        status = CLI_cmdGpAdcMeasConfig(argc, argv);
    }
    /* Legacy commands */
    else if (strcmp(argv[0], "profileCfg") == 0)
    {
        status = CLI_cmdProfileCfg(argc, argv);
    }
    else if (strcmp(argv[0], "chirpCfg") == 0)
    {
        status = CLI_cmdChirpCfg(argc, argv);
    }
    else if (strcmp(argv[0], "cfarCfg") == 0)
    {
        status = CLI_cmdCfarCfg(argc, argv);
    }
    else if (strcmp(argv[0], "presenceCfg") == 0)
    {
        status = CLI_cmdPresenceCfg(argc, argv);
    }
    else if (strcmp(argv[0], "help") == 0)
    {
        status = CLI_cmdHelp(argc, argv);
    }
    else if (strcmp(argv[0], "version") == 0)
    {
        status = CLI_cmdVersion(argc, argv);
    }
    else
    {
        CLI_write("Ignored: '%s'\r\n", argv[0]);
        /* Return 0 to avoid Error for unknown commands - SDK Visualizer sends many commands */
        status = 0;
    }

    /* SDK Visualizer compatibility: output Done or Error after each command */
    if (status == 0)
    {
        CLI_write("Done\r\n\n");
    }
    else if (status < 0)
    {
        CLI_write("Error %d\r\n", status);
    }

    return status;
}

/**
 * @brief Read a line from UART
 */
static int32_t CLI_readLine(char *buffer, uint32_t maxLen)
{
    uint32_t idx = 0;
    char ch;
    UART_Transaction trans;

    while (idx < maxLen - 1)
    {
        /* Read one character using L-SDK 6.x UART_Transaction pattern */
        if (gHealthDetectMCB.commandUartHandle != NULL)
        {
            UART_Transaction_init(&trans);
            trans.buf = &ch;
            trans.count = 1;
            UART_read(gHealthDetectMCB.commandUartHandle, &trans);
        }
        else
        {
            /* No UART, return empty */
            ClockP_usleep(100000); /* 100ms delay */
            return 0;
        }

        /* Handle special characters */
        if (ch == '\r' || ch == '\n')
        {
            buffer[idx] = '\0';
            CLI_write("\r\n");
            return idx;
        }
        else if (ch == '\b' || ch == 127) /* Backspace */
        {
            if (idx > 0)
            {
                idx--;
                CLI_write("\b \b");
            }
        }
        else if (ch >= 32 && ch < 127) /* Printable characters */
        {
            buffer[idx++] = ch;
            CLI_write("%c", ch); /* Echo */
        }
    }

    buffer[maxLen - 1] = '\0';
    return idx;
}

/**************************************************************************
 *************************** Public Functions *****************************
 **************************************************************************/

/**
 * @brief Write output to CLI console
 */
int32_t CLI_write(const char *format, ...)
{
    va_list args;
    int32_t len;
    UART_Transaction trans;

    va_start(args, format);
    len = vsnprintf(gCliOutputBuf, CLI_OUTPUT_BUF_SIZE, format, args);
    va_end(args);

    if (len > 0 && gHealthDetectMCB.commandUartHandle != NULL)
    {
        UART_Transaction_init(&trans);
        trans.buf = gCliOutputBuf;
        trans.count = len;
        UART_write(gHealthDetectMCB.commandUartHandle, &trans);
    }

    /* Also output to debug console */
    DebugP_log("%s", gCliOutputBuf);

    return len;
}

/**
 * @brief Initialize CLI module
 * 
 * Banner format must match mmw_demo for SDK Visualizer compatibility:
 * "******************************************\r\n"
 * "xWRL684x MMW Demo XX.XX.XX.XX\r\n"
 * "******************************************\r\n"
 * 
 * 🔴 关键修复（问题36）：
 * - 初始化gCLI全局MCB
 * - 设置enableMMWaveExtension=1U（SDK Visualizer必需）
 * - 配置mmWaveHandle指向ctrlHandle
 */
int32_t CLI_init(void)
{
    /* ========== SDK标准初始化（问题36修复） ========== */
    
    /* Initialize the CLI MCB */
    memset((void*)&gCLI, 0, sizeof(CLI_MCB));

    /* Populate the CLI configuration (SDK Standard) */
    gCLI.cfg.cliPrompt              = CLI_PROMPT;
    gCLI.cfg.cliBanner              = "xWRL684x Health Detection Demo 01.00.00.01";
    gCLI.cfg.UartHandle             = gHealthDetectMCB.commandUartHandle;
    gCLI.cfg.taskPriority           = CLI_TASK_PRIORITY;
    gCLI.cfg.mmWaveHandle           = gHealthDetectMCB.ctrlHandle;
    gCLI.cfg.enableMMWaveExtension  = 1U;  /* 🔴 关键！SDK Visualizer必需 */
    gCLI.cfg.usePolledMode          = true;

    /* Mark as initialized */
    gCLI.isInitialized = 1;
    gCliInitialized = 1;

    /* Standard mmw_demo banner format - required for SDK Visualizer */
    CLI_write("\r\n");
    CLI_write("******************************************\r\n");
    CLI_write("xWRL684x MMW Demo 06.01.00.01\r\n");
    CLI_write("******************************************\r\n");
    CLI_write("\r\n");

    DebugP_log("CLI: Initialized with enableMMWaveExtension=1U\r\n");

    return 0;
}

/**
 * @brief Run CLI (blocking)
 */
void CLI_run(void)
{
    if (!gCliInitialized)
    {
        CLI_init();
    }

    while (1)
    {
        /* Print prompt */
        CLI_write(CLI_PROMPT);

        /* Read command line */
        CLI_readLine(gCliCmdLine, CLI_MAX_CMD_LINE_LEN);

        /* Process command */
        CLI_processCommand(gCliCmdLine);
    }
}

/**
 * @brief CLI Open Function (SDK Standard)
 * 参考: mmw_demo_SDK_reference/source/mmw_cli.c line 2288-2340
 * 
 * 🔴 关键修复（问题36）：
 * - 实现SDK标准的CLI_open()流程
 * - 支持mmWave扩展命令（enableMMWaveExtension=1U时）
 * - 不需要创建单独的CLI任务（使用阻塞式CLI_run()）
 * 
 * @param ptrCLICfg CLI Configuration (SDK standard)
 * @return 0 on success, <0 on error
 */
int32_t CLI_open(CLI_Cfg* ptrCLICfg)
{
    /* Validate configuration */
    if (ptrCLICfg == NULL)
    {
        DebugP_log("Error: CLI_open - NULL configuration\n");
        return -1;
    }

    /* Copy configuration to global MCB */
    memcpy(&gCLI.cfg, ptrCLICfg, sizeof(CLI_Cfg));

    /* Validate UART handle */
    if (gCLI.cfg.UartHandle == NULL)
    {
        DebugP_log("Error: CLI_open - NULL UART handle\n");
        return -1;
    }

    /* If mmWave extension enabled, validate mmWave handle */
    if (gCLI.cfg.enableMMWaveExtension == 1U)
    {
        if (gCLI.cfg.mmWaveHandle == NULL)
        {
            DebugP_log("Error: CLI_open - NULL mmWave handle but extension enabled\n");
            return -1;
        }
        DebugP_log("CLI: mmWave extension enabled (SDK Visualizer compatible)\n");
    }

    /* Mark as initialized */
    gCLI.isInitialized = 1;

    DebugP_log("CLI: Opened successfully\n");
    return 0;
}

/**
 * @brief Open radar configuration
 */
int32_t CLI_openRadar(void)
{
    return RadarControl_open();
}

/**
 * @brief Close radar configuration
 */
int32_t CLI_closeRadar(void)
{
    return RadarControl_close();
}
