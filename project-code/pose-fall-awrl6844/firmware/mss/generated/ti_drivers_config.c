/*
 *  Copyright (C) 2021 Texas Instruments Incorporated
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
/*
 * Auto generated file 
 */

#include "ti_drivers_config.h"

/*
 * QSPI
 */


/* QSPI attributes */
static QSPI_Attrs gQspiAttrs[CONFIG_QSPI_NUM_INSTANCES] =
{
    {
        .baseAddr             = CSL_APP_CFG_QSPI_U_BASE,
        .memMapBaseAddr       = CSL_APP_QSPI_EXT_FLASH_U_BASE,
        .inputClkFreq         = 80000000U,
        .intrNum              = 13U,
        .intrEnable           = FALSE,
        .dmaEnable            = FALSE,
        .intrPriority         = 4U,
        .rxLines              = QSPI_RX_LINES_QUAD,
        .chipSelect           = QSPI_CS0,
        .csPol                = QSPI_CS_POL_ACTIVE_LOW,
        .dataDelay            = QSPI_DATA_DELAY_0,
        .frmFmt               = QSPI_FF_POL0_PHA0,
        .wrdLen               = 8,
        .baudRateDiv          = 0,
    },
};
/* QSPI objects - initialized by the driver */
static QSPI_Object gQspiObjects[CONFIG_QSPI_NUM_INSTANCES];
/* QSPI driver configuration */
QSPI_Config gQspiConfig[CONFIG_QSPI_NUM_INSTANCES] =
{
    {
        &gQspiAttrs[CONFIG_QSPI0],
        &gQspiObjects[CONFIG_QSPI0],
    },
};

uint32_t gQspiConfigNum = CONFIG_QSPI_NUM_INSTANCES;

/*
 * EDMA
 */
/* EDMA atrributes */
static EDMA_Attrs gEdmaAttrs[CONFIG_EDMA_NUM_INSTANCES] =
{
    {

        .baseAddr           = CSL_DSS_TPCC_A_U_BASE,
        .compIntrNumber     = CSL_APPSS_INTR_DSS_TPCC_A_INTAGG,
        .intrAggEnableAddr  = CSL_DSS_CTRL_U_BASE + CSL_DSS_CTRL_DSS_TPCC_A_INTAGG_MASK,
        .intrAggEnableMask  = 0x1FF & (~(2U << 0)),
        .intrAggStatusAddr  = CSL_DSS_CTRL_U_BASE + CSL_DSS_CTRL_DSS_TPCC_A_INTAGG_STATUS,
        .intrAggClearMask   = (2U << 0),
        .initPrms           =
        {
            .regionId     = 0,
            .queNum       = 0,
            .initParamSet = FALSE,
            .ownResource    =
            {
                .qdmaCh      = 0xFFU,
                .dmaCh[0]    = 0xFFFFFFFFU,
                .dmaCh[1]    = 0xFFFFFFFFU,
                .tcc[0]      = 0xFFFFFFFFU,
                .tcc[1]      = 0xFFFFFFFFU,
                .paramSet[0] = 0xFFFFFFFFU,
                .paramSet[1] = 0xFFFFFFFFU,
            },
            .reservedDmaCh[0]    = 0x01U,
            .reservedDmaCh[1]    = 0x00U,
        },
    },
    {

        .baseAddr           = CSL_APP_TPCC_A_U_BASE,
        .compIntrNumber     = CSL_APPSS_INTR_APPSS_TPCC_A_INTAGG,
        .intrAggEnableAddr  = CSL_APP_CTRL_U_BASE + CSL_APP_CTRL_APPSS_TPCC_A_INTAGG_MASK,
        .intrAggEnableMask  = 0x1FF & (~(2U << 0)),
        .intrAggStatusAddr  = CSL_APP_CTRL_U_BASE + CSL_APP_CTRL_APPSS_TPCC_A_INTAGG_STATUS,
        .intrAggClearMask   = (2U << 0),
        .initPrms           =
        {
            .regionId     = 0,
            .queNum       = 0,
            .initParamSet = FALSE,
            .ownResource    =
            {
                .qdmaCh      = 0xFFU,
                .dmaCh[0]    = 0xFFFFFFFFU,
                .dmaCh[1]    = 0xFFFFFFFFU,
                .tcc[0]      = 0xFFFFFFFFU,
                .tcc[1]      = 0xFFFFFFFFU,
                .paramSet[0] = 0xFFFFFFFFU,
                .paramSet[1] = 0xFFFFFFFFU,
            },
            .reservedDmaCh[0]    = 0x03U,
            .reservedDmaCh[1]    = 0x00U,
        },
    },
};

/* EDMA objects - initialized by the driver */
static EDMA_Object gEdmaObjects[CONFIG_EDMA_NUM_INSTANCES];
/* EDMA driver configuration */
EDMA_Config gEdmaConfig[CONFIG_EDMA_NUM_INSTANCES] =
{
    {
        &gEdmaAttrs[CONFIG_EDMA0],
        &gEdmaObjects[CONFIG_EDMA0],
    },
    {
        &gEdmaAttrs[CONFIG_EDMA1],
        &gEdmaObjects[CONFIG_EDMA1],
    },
};

uint32_t gEdmaConfigNum = CONFIG_EDMA_NUM_INSTANCES;

/*
 * ADCBUF
 */
/* ADCBUF atrributes */
static ADCBuf_Attrs gADCBufAttrs[CONFIG_ADCBUF_NUM_INSTANCES] =
{
    {
        .baseAddr           = CSL_DSS_CTRL_U_BASE,
        .interruptNum       = 34U,
        .adcbufBaseAddr     = CSL_DSS_ADCBUF_READ_U_BASE,
        .radarCfgBaseAddr   = CSL_FEC_RADAR_CFG_U_BASE,
    },
};
/* ADCBUF objects - initialized by the driver */
static ADCBuf_Object gADCBufObjects[CONFIG_ADCBUF_NUM_INSTANCES];
/* ADCBUF driver configuration */
ADCBuf_Config gADCBufConfig[CONFIG_ADCBUF_NUM_INSTANCES] =
{
    {
        &gADCBufAttrs[CONFIG_ADCBUF0],
        &gADCBufObjects[CONFIG_ADCBUF0],
    },
};

uint32_t gADCBufConfigNum = CONFIG_ADCBUF_NUM_INSTANCES;

/*
 * CBUFF
 */
/* CBUFF atrributes */
CBUFF_Attrs gCbuffAttrs[CONFIG_CBUFF_NUM_INSTANCES] =
{
    {
        .baseAddr                   = CSL_DSS_CBUFF_U_BASE,
        .fifoBaseAddr               = CSL_DSS_CBUFF_FIFO_U_BASE,
        .adcBufBaseAddr             = CSL_DSS_ADCBUF_READ_U_BASE,
        .maxLVDSLanesSupported      = 2,
        .errorIntrNum               = CSL_APPSS_INTR_DSS_CBUFF_INT_ERR,
        .intrNum                    = CSL_APPSS_INTR_DSS_CBUFF_INT,
        .chirpModeStartIndex        = 1,
        .chirpModeEndIndex          = 8,
        .cbuffChannelId[0]             = EDMA_DSS_TPCC_A_EVT_DSS_CBUFF_DMA_REQ0,
        .cbuffChannelId[1]             = EDMA_DSS_TPCC_A_EVT_DSS_CBUFF_DMA_REQ1,
        .cbuffChannelId[2]             = EDMA_DSS_TPCC_A_EVT_DSS_CBUFF_DMA_REQ2,
        .cbuffChannelId[3]             = EDMA_DSS_TPCC_A_EVT_DSS_CBUFF_DMA_REQ3,
        .cbuffChannelId[4]             = EDMA_DSS_TPCC_A_EVT_DSS_CBUFF_DMA_REQ4,
        .cbuffChannelId[5]             = EDMA_DSS_TPCC_A_EVT_DSS_CBUFF_DMA_REQ5,
        .cbuffChannelId[6]             = EDMA_DSS_TPCC_A_EVT_DSS_CBUFF_DMA_REQ6,
    },
};

/* CBUFF objects - initialized by the driver */
CBUFF_Object gCbuffObject[CONFIG_CBUFF_NUM_INSTANCES];
/* CBUFF objects - storage for CBUFF driver object handles */
CBUFF_Object *gCbuffObjectPtr[CONFIG_CBUFF_NUM_INSTANCES] = { NULL };
/* CBUFF objects count */
uint32_t gCbuffConfigNum = CONFIG_CBUFF_NUM_INSTANCES;

/*
 * HWA
 */
/* HWA atrributes */
HWA_Attrs gHwaAttrs[CONFIG_HWA_NUM_INSTANCES] =
{
    {
        .instanceNum                = 0U,
        .ctrlBaseAddr               = CSL_DSS_HWA_CFG_U_BASE,
        .paramBaseAddr              = CSL_DSS_HWA_PARAM_U_BASE,
        .ramBaseAddr                = CSL_DSS_HWA_WINDOW_RAM_U_BASE,
        .numHwaParamSets            = SOC_HWA_NUM_PARAM_SETS,
        .intNumParamSet             = CSL_APPSS_INTR_HWASS_PARAMDONE_INT,
        .intNumDone                 = CSL_APPSS_INTR_HWASS_LOOP_INT,
        .numDmaChannels             = SOC_HWA_NUM_DMA_CHANNEL,
        .accelMemBaseAddr           = CSL_DSS_HWA_DMA0_U_BASE,
        .accelMemSize               = SOC_HWA_MEM_SIZE,
        .isConcurrentAccessAllowed  = true,
        .isCompressionEnginePresent = true,
    },
};
/* HWA RAM atrributes */
HWA_RAMAttrs gHwaRamCfg[HWA_NUM_RAMS] =
{
    {CSL_DSS_HWA_WINDOW_RAM_U_BASE, CSL_DSS_HWA_WINDOW_RAM_U_SIZE},
    {CSL_DSS_HWA_MC_PING_RAM_U_BASE, CSL_DSS_HWA_MC_PING_RAM_U_SIZE},
    {CSL_DSS_HWA_MC_PONG_RAM_U_BASE, CSL_DSS_HWA_MC_PONG_RAM_U_SIZE}
};

/* HWA objects - initialized by the driver */
HWA_Object gHwaObject[CONFIG_HWA_NUM_INSTANCES];
/* HWA objects - storage for HWA driver object handles */
HWA_Object *gHwaObjectPtr[CONFIG_HWA_NUM_INSTANCES] = { NULL };
/* HWA objects count */
uint32_t gHwaConfigNum = CONFIG_HWA_NUM_INSTANCES;

/*
 * I2C
 */
/* I2C atrributes */
static I2C_HwAttrs gI2cHwAttrs[CONFIG_I2C_NUM_INSTANCES] =
{
    {
        .baseAddr       = CSL_APP_I2C_U_BASE,
        .intNum         = CSL_APPSS_INTR_APPSS_I2C_INT_AND_PBIST_INTR,
        .eventId        = 0,
        .funcClk        = 40000000U,
        .enableIntr     = 1,
        .ownTargetAddr   = 0x1C,
    },
};
/* I2C objects - initialized by the driver */
static I2C_Object gI2cObjects[CONFIG_I2C_NUM_INSTANCES];
/* I2C driver configuration */
I2C_Config gI2cConfig[CONFIG_I2C_NUM_INSTANCES] =
{
    {
        .object = &gI2cObjects[CONFIG_I2C0],
        .hwAttrs = &gI2cHwAttrs[CONFIG_I2C0]
    },
};

uint32_t gI2cConfigNum = CONFIG_I2C_NUM_INSTANCES;

/*
 * MCSPI
 */

/* MCSPI atrributes */
static MCSPI_Attrs gMcspiAttrs[CONFIG_MCSPI_NUM_INSTANCES] =
{
    {
        .baseAddr           = CSL_MCU_MCSPI0_CFG_BASE,
        .inputClkFreq       = 40000000U,
        .intrNum            = 14,
        .operMode           = MCSPI_OPER_MODE_DMA,
        .intrPriority       = 4U,
        .chMode             = MCSPI_CH_MODE_SINGLE,
        .pinMode            = MCSPI_PINMODE_4PIN,
        .initDelay          = MCSPI_INITDLY_0,
    },
};
/* MCSPI objects - initialized by the driver */
static MCSPI_Object gMcspiObjects[CONFIG_MCSPI_NUM_INSTANCES];
/* MCSPI driver configuration */
MCSPI_Config gMcspiConfig[CONFIG_MCSPI_NUM_INSTANCES] =
{
    {
        &gMcspiAttrs[CONFIG_MCSPI0],
        &gMcspiObjects[CONFIG_MCSPI0],
    },
};

uint32_t gMcspiConfigNum = CONFIG_MCSPI_NUM_INSTANCES;

#include <drivers/mcspi/v0/dma/mcspi_dma.h>
#include <drivers/mcspi/v0/dma/edma/mcspi_dma_edma.h>
#include <drivers/edma.h>

McspiDma_EdmaArgs gMcspiEdmaArgs =
{
    .drvHandle        = (void *)&gEdmaConfig[CONFIG_EDMA1],
};

MCSPI_DmaConfig gMcspiDmaConfig =
{
    .fxns        = &gMcspiDmaEdmaFxns,
    .mcspiDmaArgs = (void *)&gMcspiEdmaArgs,
};



uint32_t gMcspiDmaConfigNum = CONFIG_MCSPI_NUM_DMA_INSTANCES;

/*
 * Copyright (c) 2018-2020, Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

 

/*
 *  =============================== Power ===============================
 */
#include <drivers/power.h>
#include <drivers/power_xwrL68xx.h>
#include <drivers/prcm.h>
#include <drivers/hw_include/cslr.h>
#include <drivers/hw_include/xwrL684x/cslr_soc_baseaddress.h>


extern void Power_initPolicy(void);
extern void Power_sleepPolicy(unsigned long long sleepTimeus);
extern void power_LPDSentryhook(void);
extern void power_LPDSresumehook(void);
extern void power_idle3entryhook(void);
extern void power_idle3resumehook(void);
Power_ParkInfo parkInfo[];


/*
 *  This structure defines the configuration for the Power Manager.
 */
const Power_ConfigV1 Power_config = {
    .policyInitFxn             = Power_initPolicy,
    .policyFxn                 = Power_sleepPolicy,
    .enterLPDSHookFxn          = power_LPDSentryhook,
    .resumeLPDSHookFxn         = power_LPDSresumehook,
    .enteridle3HookFxn          = power_idle3entryhook,
    .resumeidle3HookFxn         = power_idle3resumehook,
    .enablePolicy                   = false,
    .enableSleepCounterWakeupLPDS   = true,
    .enableUARTWakeupLPDS           = false,
    .enablePRCMPMICDeepSleep             = false,
    .enableRTCWakeupLPDS            = false,
    .enableFRCWakeupLPDS            = false,
    .enableGPIOSyncIOWakeupLPDS           = false,
    .wakeupSyncIOEdgeLPDS           = PRCM_LPDS_FALL_EDGE,
    .LPDSThreshold                   = 6000,
    .totalLatencyForLPDS                 = 5000,
    .ramRetentionMaskLPDS           = PRCM_APP_PD_SRAM_CLUSTER_1|PRCM_APP_PD_SRAM_CLUSTER_2|PRCM_APP_PD_SRAM_CLUSTER_3|PRCM_APP_PD_SRAM_CLUSTER_4|PRCM_APP_PD_SRAM_CLUSTER_5|PRCM_APP_PD_SRAM_CLUSTER_6|PRCM_APP_PD_SRAM_CLUSTER_7|PRCM_FEC_PD_SRAM_CLUSTER_1|PRCM_FEC_PD_SRAM_CLUSTER_2|PRCM_FEC_PD_SRAM_CLUSTER_3,
    .idleThreshold                   = 2000,
    .totalLatencyForIdle                 = 1700,
    .pinParkDefs                    = parkInfo,
    .numPins                        = 32
};

Power_ParkInfo parkInfo[] = {
/*        PIN                    PARK STATE              Pin Alias
   -----------------  ------------------------------     ---------------*/

   {POWER_PIN_PAD_AA, POWER_PARK_IDLE},   /*PIN_PAD_AA*/

   {POWER_PIN_PAD_AB, POWER_PARK_IDLE},   /*PIN_PAD_AB*/

   {POWER_PIN_PAD_AC, POWER_PARK_IDLE},   /*PIN_PAD_AC*/

   {POWER_PIN_PAD_AD, POWER_PARK_IDLE},   /*PIN_PAD_AD*/

   {POWER_PIN_PAD_AE, POWER_PARK_IDLE},   /*PIN_PAD_AE*/

   {POWER_PIN_PAD_AF, POWER_PARK_IDLE},   /*PIN_PAD_AF*/

   {POWER_PIN_PAD_AG, POWER_PARK_IDLE},   /*PIN_PAD_AG*/

   {POWER_PIN_PAD_AH, POWER_PARK_IDLE},   /*PIN_PAD_AH*/

   {POWER_PIN_PAD_AI, POWER_PARK_IDLE},   /*PIN_PAD_AI*/

   {POWER_PIN_PAD_AJ, POWER_PARK_IDLE},   /*PIN_PAD_AJ*/

   {POWER_PIN_PAD_AK, POWER_PARK_IDLE},   /*PIN_PAD_AK*/

   {POWER_PIN_PAD_AL, POWER_PARK_IDLE},   /*PIN_PAD_AL*/

   {POWER_PIN_PAD_AM, POWER_PARK_IDLE_AND_LPDS},   /*PIN_PAD_AM*/

   {POWER_PIN_PAD_AN, POWER_PARK_IDLE_AND_LPDS},   /*PIN_PAD_AN*/

   {POWER_PIN_PAD_AO, POWER_PARK_IDLE_AND_LPDS},   /*PIN_PAD_AO*/

   {POWER_PIN_PAD_AP, POWER_PARK_IDLE_AND_LPDS},   /*PIN_PAD_AP*/

   {POWER_PIN_PAD_AQ, POWER_DONT_PARK},   /*PIN_PAD_AQ*/

   {POWER_PIN_PAD_AR, POWER_DONT_PARK},   /*PIN_PAD_AR*/

   {POWER_PIN_PAD_AS, POWER_DONT_PARK},   /*PIN_PAD_AS*/

   {POWER_PIN_PAD_AT, POWER_DONT_PARK},   /*PIN_PAD_AT*/

   {POWER_PIN_PAD_AU, POWER_PARK_IDLE_AND_LPDS},   /*PIN_PAD_AU*/

   {POWER_PIN_PAD_AV, POWER_PARK_IDLE_AND_LPDS},   /*PIN_PAD_AV*/

   {POWER_PIN_PAD_AW, POWER_DONT_PARK},   /*PIN_PAD_AW*/

   {POWER_PIN_PAD_AX, POWER_PARK_IDLE},   /*PIN_PAD_AX*/

   {POWER_PIN_PAD_AY, POWER_PARK_IDLE_AND_LPDS},   /*PIN_PAD_AY*/

   {POWER_PIN_PAD_AZ, POWER_PARK_IDLE_AND_LPDS},   /*PIN_PAD_AZ*/

   {POWER_PIN_PAD_BA, POWER_PARK_IDLE_AND_LPDS},   /*PIN_PAD_BA*/

   {POWER_PIN_PAD_BB, POWER_PARK_IDLE_AND_LPDS},   /*PIN_PAD_BB*/

   {POWER_PIN_PAD_BC, POWER_PARK_IDLE_AND_LPDS},   /*PIN_PAD_BC*/

   {POWER_PIN_PAD_BD, POWER_PARK_IDLE_AND_LPDS},   /*PIN_PAD_BD*/

   {POWER_PIN_PAD_BE, POWER_PARK_IDLE_AND_LPDS},   /*PIN_PAD_BE*/

   {POWER_PIN_PAD_BF, POWER_PARK_IDLE_AND_LPDS},   /*PIN_PAD_BF*/

};

/*
 * UART
 */
#include "drivers/soc.h"

/* UART atrributes */
static UART_Attrs gUartAttrs[CONFIG_UART_NUM_INSTANCES] =
{
    {
        .baseAddr           = CSL_APP_UARTB_U_BASE,
        .inputClkFreq       = 40000000,
    },
    {
        .baseAddr           = CSL_APP_UARTA_U_BASE,
        .inputClkFreq       = 40000000,
    },
};
/* UART objects - initialized by the driver */
static UART_Object gUartObjects[CONFIG_UART_NUM_INSTANCES];
/* UART driver configuration */
UART_Config gUartConfig[CONFIG_UART_NUM_INSTANCES] =
{
    {
        &gUartAttrs[CONFIG_UART0],
        &gUartObjects[CONFIG_UART0],
    },
    {
        &gUartAttrs[CONFIG_UART1],
        &gUartObjects[CONFIG_UART1],
    },
};

uint32_t gUartConfigNum = CONFIG_UART_NUM_INSTANCES;

void Drivers_uartInit(void)
{
    uint32_t i;
    for (i=0; i<CONFIG_UART_NUM_INSTANCES; i++)
    {
        SOC_RcmPeripheralId periphID;
        if(gUartAttrs[i].baseAddr == CSL_APP_UARTA_U_BASE) {
            periphID = SOC_RcmPeripheralId_APPSS_UARTA;
        } else if (gUartAttrs[i].baseAddr == CSL_APP_UARTB_U_BASE) {
            periphID = SOC_RcmPeripheralId_APPSS_UARTB;
        } else if (gUartAttrs[i].baseAddr == CSL_DSS_SCIA_U_BASE) {
            periphID = SOC_RcmPeripheralId_DSS_SCIA;
        } else {
            continue;
        }            
        gUartAttrs[i].inputClkFreq = SOC_rcmGetPeripheralClock(periphID);
    }
    UART_init();
}


#define TOP_PRCM_CLK_CTRL_REG1_LDO_CLKTOP_NO_LOAD 0X1U
void Pinmux_init();
void PowerClock_init(void);
void PowerClock_deinit(void);

/*
 * Common Functions
 */
void System_init(void)
{
    CSL_top_prcmRegs* ptrTOPRCMRegs = (CSL_top_prcmRegs*) CSL_TOP_PRCM_U_BASE;
    /* DPL init sets up address transalation unit, on some CPUs this is needed
     * to access SCICLIENT services, hence this needs to happen first
     */
    Dpl_init();
    PowerClock_init();
    
    /* initialize PMU */
    CycleCounterP_init(SOC_getSelfCpuClk());

    /* Turn off all current loading after oscillator is enabled to reduce power and extend reliability */
    CSL_FINS(ptrTOPRCMRegs->CLK_CTRL_REG1_LDO_CLKTOP, TOP_PRCM_CLK_CTRL_REG1_LDO_CLKTOP_CLK_CTRL_REG1_LDO_CLKTOP_TLOAD_CTRL, TOP_PRCM_CLK_CTRL_REG1_LDO_CLKTOP_NO_LOAD);

    /* Now we can do pinmux */
    Pinmux_init();
    /* finally we initialize all peripheral drivers */
    QSPI_init();
    EDMA_init();
    ADCBuf_init(SystemP_WAIT_FOREVER);
    HWA_init();
    I2C_init();
    MCSPI_init();
    Power_init();

    Drivers_uartInit();
}

void System_deinit(void)
{
    QSPI_deinit();
    EDMA_deinit();
    ADCBuf_deinit();
    HWA_deinit();
    I2C_deinit();
    MCSPI_deinit();
    UART_deinit();
    PowerClock_deinit();
    Dpl_deinit();
}
