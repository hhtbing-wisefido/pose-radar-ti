/**
 * @file cli.c
 * @brief Command Line Interface Module - MSS Side
 * 
 * Handles UART CLI commands for configuration.
 * Reference: mmw_demo/source/mmw_cli.c
 * Rewritten for: Three-layer architecture
 */

#include "cli.h"
#include <drivers/uart.h>
#include <string.h>
#include <stdlib.h>

/*----------------------------------------------------------------------------*/
/* CLI Command Table                                                         */
/*----------------------------------------------------------------------------*/

typedef int32_t (*CLI_CmdHandler_t)(int32_t argc, char* argv[]);

typedef struct {
    const char* cmdName;
    CLI_CmdHandler_t handler;
    const char* helpText;
} CLI_Command_t;

/* Command Handlers */
static int32_t CLI_sensorStart(int32_t argc, char* argv[]);
static int32_t CLI_sensorStop(int32_t argc, char* argv[]);
static int32_t CLI_frameConfig(int32_t argc, char* argv[]);
static int32_t CLI_cfarConfig(int32_t argc, char* argv[]);
static int32_t CLI_featureConfig(int32_t argc, char* argv[]);

/* Command Table - Learned from mmw_demo CLI */
static const CLI_Command_t gCLI_CommandTable[] = {
    {"sensorStart", CLI_sensorStart, "Start radar sensor"},
    {"sensorStop", CLI_sensorStop, "Stop radar sensor"},
    {"frameConfig", CLI_frameConfig, "Configure frame parameters"},
    {"cfarCfg", CLI_cfarConfig, "Configure CFAR detection"},
    {"featureCfg", CLI_featureConfig, "Configure feature extraction"},
    {NULL, NULL, NULL} /* End marker */
};

/*----------------------------------------------------------------------------*/
/* CLI Module State                                                          */
/*----------------------------------------------------------------------------*/

static UART_Handle gCLI_uartHandle = NULL;
static bool gCLI_initialized = false;

/*----------------------------------------------------------------------------*/
/* CLI Initialization                                                        */
/*----------------------------------------------------------------------------*/

/**
 * @brief Initialize CLI module
 * 
 * Reference: mmw_demo UART initialization
 */
int32_t CLI_init(void)
{
    UART_Params uartParams;
    
    /* Configure UART */
    UART_Params_init(&uartParams);
    uartParams.baudRate = 115200;
    uartParams.readMode = UART_MODE_BLOCKING;
    uartParams.writeMode = UART_MODE_BLOCKING;
    
    /* Open UART instance */
    /* TODO: Get UART instance number from config */
    /* gCLI_uartHandle = UART_open(0, &uartParams); */
    
    gCLI_initialized = true;
    
    CLI_printf("CLI: Initialized\n");
    CLI_printf("Type 'help' for commands\n");
    
    return 0;
}

/**
 * @brief Process CLI input line
 */
int32_t CLI_processLine(char* line)
{
    char* argv[16];
    int32_t argc = 0;
    char* token;
    int32_t i;
    
    /* Skip empty lines */
    if (line[0] == '\0' || line[0] == '\n') {
        return 0;
    }
    
    /* Parse line into tokens */
    token = strtok(line, " \t\n");
    while (token != NULL && argc < 16) {
        argv[argc++] = token;
        token = strtok(NULL, " \t\n");
    }
    
    if (argc == 0) {
        return 0;
    }
    
    /* Find command in table */
    for (i = 0; gCLI_CommandTable[i].cmdName != NULL; i++) {
        if (strcmp(argv[0], gCLI_CommandTable[i].cmdName) == 0) {
            return gCLI_CommandTable[i].handler(argc, argv);
        }
    }
    
    CLI_printf("Unknown command: %s\n", argv[0]);
    return -1;
}

/**
 * @brief Printf to CLI UART
 */
void CLI_printf(const char* format, ...)
{
    char buffer[256];
    va_list args;
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    if (gCLI_uartHandle != NULL) {
        UART_write(gCLI_uartHandle, buffer, strlen(buffer));
    }
}

/*----------------------------------------------------------------------------*/
/* Command Handlers                                                          */
/*----------------------------------------------------------------------------*/

/**
 * @brief sensorStart command
 */
static int32_t CLI_sensorStart(int32_t argc, char* argv[])
{
    CLI_printf("Starting radar sensor...\n");
    
    /* TODO: Call radar control to start sensor
     * Reference: mmw_demo sensor start sequence
     */
    
    CLI_printf("Sensor started\n");
    return 0;
}

/**
 * @brief sensorStop command
 */
static int32_t CLI_sensorStop(int32_t argc, char* argv[])
{
    CLI_printf("Stopping radar sensor...\n");
    
    /* TODO: Call radar control to stop sensor */
    
    CLI_printf("Sensor stopped\n");
    return 0;
}

/**
 * @brief frameConfig command
 * 
 * Usage: frameConfig <chirps> <rangeBins> <dopplerBins>
 */
static int32_t CLI_frameConfig(int32_t argc, char* argv[])
{
    if (argc < 4) {
        CLI_printf("Usage: frameConfig <chirps> <rangeBins> <dopplerBins>\n");
        return -1;
    }
    
    uint32_t numChirps = atoi(argv[1]);
    uint32_t numRangeBins = atoi(argv[2]);
    uint32_t numDopplerBins = atoi(argv[3]);
    
    CLI_printf("Frame Config: chirps=%d, range=%d, doppler=%d\n",
               numChirps, numRangeBins, numDopplerBins);
    
    /* TODO: Update DPC config in shared RAM */
    
    return 0;
}

/**
 * @brief cfarCfg command
 * 
 * Usage: cfarCfg <type> <winLen> <guardLen> <threshold>
 * type: 0=range, 1=doppler
 */
static int32_t CLI_cfarConfig(int32_t argc, char* argv[])
{
    if (argc < 5) {
        CLI_printf("Usage: cfarCfg <type> <winLen> <guardLen> <threshold>\n");
        return -1;
    }
    
    /* TODO: Parse parameters and update DPC config */
    
    CLI_printf("CFAR configured\n");
    return 0;
}

/**
 * @brief featureCfg command
 * 
 * Usage: featureCfg <enable> <minClusterSize>
 */
static int32_t CLI_featureConfig(int32_t argc, char* argv[])
{
    if (argc < 3) {
        CLI_printf("Usage: featureCfg <enable> <minClusterSize>\n");
        return -1;
    }
    
    bool enable = (atoi(argv[1]) != 0);
    float minClusterSize = atof(argv[2]);
    
    CLI_printf("Feature Config: enable=%d, minSize=%.1f\n",
               enable, minClusterSize);
    
    /* TODO: Update DPC config */
    
    return 0;
}
