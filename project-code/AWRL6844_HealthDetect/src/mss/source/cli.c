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

/* Application Includes */
#include <source/cli.h>
#include <source/health_detect_main.h>
#include <source/radar_control.h>

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
    char *saveptr;

    token = strtok_r(cmdLine, " \t\r\n", &saveptr);
    while (token != NULL && argc < CLI_MAX_ARGS)
    {
        argv[argc++] = token;
        token = strtok_r(NULL, " \t\r\n", &saveptr);
    }

    return argc;
}

/**
 * @brief Process sensorStart command
 */
static int32_t CLI_cmdSensorStart(int32_t argc, char *argv[])
{
    int32_t status;

    CLI_write("Starting sensor...\r\n");

    status = HealthDetect_sensorStart();
    if (status == 0)
    {
        CLI_write("Sensor started successfully\r\n");
    }
    else
    {
        CLI_write("Error: Failed to start sensor [%d]\r\n", status);
    }

    return status;
}

/**
 * @brief Process sensorStop command
 */
static int32_t CLI_cmdSensorStop(int32_t argc, char *argv[])
{
    int32_t status;

    CLI_write("Stopping sensor...\r\n");

    status = HealthDetect_sensorStop();
    if (status == 0)
    {
        CLI_write("Sensor stopped successfully\r\n");
    }
    else
    {
        CLI_write("Error: Failed to stop sensor [%d]\r\n", status);
    }

    return status;
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
 */
static int32_t CLI_cmdChannelCfg(int32_t argc, char *argv[])
{
    if (argc < 3)
    {
        CLI_write("Error: channelCfg requires 2 arguments\r\n");
        CLI_write("Usage: channelCfg <rxChannelEn> <txChannelEn>\r\n");
        return -1;
    }

    gHealthDetectMCB.cliCfg.rxChannelEn = (uint8_t)atoi(argv[1]);
    gHealthDetectMCB.cliCfg.txChannelEn = (uint8_t)atoi(argv[2]);

    CLI_write("channelCfg: RX=0x%02X, TX=0x%02X\r\n",
              gHealthDetectMCB.cliCfg.rxChannelEn,
              gHealthDetectMCB.cliCfg.txChannelEn);

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

    /* Match command and execute */
    if (strcmp(argv[0], "sensorStart") == 0)
    {
        status = CLI_cmdSensorStart(argc, argv);
    }
    else if (strcmp(argv[0], "sensorStop") == 0)
    {
        status = CLI_cmdSensorStop(argc, argv);
    }
    else if (strcmp(argv[0], "profileCfg") == 0)
    {
        status = CLI_cmdProfileCfg(argc, argv);
    }
    else if (strcmp(argv[0], "chirpCfg") == 0)
    {
        status = CLI_cmdChirpCfg(argc, argv);
    }
    else if (strcmp(argv[0], "frameCfg") == 0)
    {
        status = CLI_cmdFrameCfg(argc, argv);
    }
    else if (strcmp(argv[0], "channelCfg") == 0)
    {
        status = CLI_cmdChannelCfg(argc, argv);
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
        CLI_write("Error: Unknown command '%s'. Type 'help' for available commands.\r\n", argv[0]);
        status = -1;
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

    while (idx < maxLen - 1)
    {
        /* Read one character */
        if (gHealthDetectMCB.uartHandle != NULL)
        {
            UART_read(gHealthDetectMCB.uartHandle, &ch, 1, NULL);
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

    va_start(args, format);
    len = vsnprintf(gCliOutputBuf, CLI_OUTPUT_BUF_SIZE, format, args);
    va_end(args);

    if (len > 0 && gHealthDetectMCB.uartHandle != NULL)
    {
        UART_write(gHealthDetectMCB.uartHandle, gCliOutputBuf, len, NULL);
    }

    /* Also output to debug console */
    DebugP_log("%s", gCliOutputBuf);

    return len;
}

/**
 * @brief Initialize CLI module
 */
int32_t CLI_init(void)
{
    gCliInitialized = 1;

    CLI_write("\r\n");
    CLI_write("********************************************\r\n");
    CLI_write("*   AWRL6844 Health Detection Firmware     *\r\n");
    CLI_write("*   Version 1.0.0                          *\r\n");
    CLI_write("********************************************\r\n");
    CLI_write("\r\n");
    CLI_write("Type 'help' for available commands\r\n");
    CLI_write("\r\n");

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
