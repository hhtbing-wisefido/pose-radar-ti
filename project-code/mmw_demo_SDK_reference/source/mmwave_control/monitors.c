/*
 * Copyright (C) 2025 Texas Instruments Incorporated
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 *   Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the
 *   distribution.
 *
 *   Neither the name of Texas Instruments Incorporated nor the names of
 *   its contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**************************************************************************
 *************************** Include Files ********************************
 **************************************************************************/

/* Standard Include Files. */
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
/* MCU Plus Include Files. */
#include <kernel/dpl/SemaphoreP.h>
#include <kernel/dpl/CacheP.h>
#include <kernel/dpl/ClockP.h>
#include <kernel/dpl/DebugP.h>
#include <kernel/dpl/HwiP.h>
#include <kernel/dpl/AddrTranslateP.h>
#include "FreeRTOS.h"
#include "task.h"
/* mmwave SDK files */
#include <control/mmwave/mmwave.h>
#include "source/mmw_cli.h"
#include "ti_drivers_config.h"
#include "ti_drivers_open_close.h"
#include "ti_board_open_close.h"
#include "ti_board_config.h"
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include "source/mmwave_demo.h"
#include "source/mmwave_control/interrupts.h"

#define LOW_PWR_MODE_DISABLE (0)
#define LOW_PWR_MODE_ENABLE (1)
#define LOW_PWR_TEST_MODE (2)

extern MmwDemo_MSS_MCB gMmwMssMCB;

#if (ENABLE_MONITORS==1)
/*! @brief  RF Monitor LB result during factory calibration */
extern volatile MmwDemo_MonResultSave rfMonResFactCal;
/**
 *  @b Description
 *  @n
 *      This is the ISR Handler for Monitors
 */
void mmwDemoMonitorISR(void)
{
    /*Clear the interrupt*/
    HwiP_clearInt(CSL_APPSS_INTR_FEC_INTR2);
    mmwDemo_GetMonRes();
    /*Posting the semaphore*/
    SemaphoreP_post(&gMmwMssMCB.rfmonSemHandle);
}

/**
 * @brief Configures the live monitors for the mmwave demo.
 *
 * @return None.
 */
void mmwDemo_LiveMonConfig()
{
    int32_t retVal;
    /*Configuring Synth Frequency Monitor*/
    if((gMmwMssMCB.mmWaveCfg.strtCfg.frameLivMonEn & 0x1) == 0x1)
    {
        retVal = MMWaveMon_TxnSynthFreqCfg();
        if(retVal < 0)
        {
            CLI_write("Incorrect Synth Frequency Monitor Cfg\n");
            DebugP_assert(0);
        }
    }
     /*Configuring Rx Saturation Live Monitor*/
    if((gMmwMssMCB.mmWaveCfg.strtCfg.frameLivMonEn & 0x2) == 0x2)
    {
        retVal = MMWaveMon_RxSatLiveCfg(MON_RX_SAT_LUT_OFFSET); 
        if(retVal < 0)
        {
            CLI_write("Incorrect Rx Saturation Live Monitor Cfg\n");
            DebugP_assert(0);
        }
    }
}

/**
*  @b Description
*  @n
 *      The function is used to configure the RF monitors.
 *
 */
void mmwDemo_MonitorConfig (void)
{
    int32_t retVal;
    /*Configuring PLL Monitors if its enabled*/
    if(gMmwMssMCB.monPllVolEnaMask != 0U)
    {
        retVal = MMWaveMon_PllCtrlVolCfg(gMmwMssMCB.monPllVolEnaMask);
        if(retVal < 0)
        {
            CLI_write("Incorrect PLL Control Voltage Monitor Cfg\n");
            DebugP_assert(0);
        }
    }
    /* TX Power Monitor requires Tx Backoff to be <= TX_BACKOFF_PWR_MON_MAX. 
     * And when configured Tx Backoff is between DUAL_CAL_TX_BACKOFF_MIN and DUAL_CAL_TX_BACKOFF_MAX, override mode (c_MonTxCodesSel = 2) is used with DEFAULT_TX_CLPC_BACKOFF_SEL Backoff(c_TxBackoffMap = (2 * DEFAULT_TX_CLPC_BACKOFF_SEL)). 
     * The 2-byte h_MonTxBiasCodes is extracted from the 16-byte h_TxStgCodes CLPC result by indexing with the configured TX chain. */
    if (gMmwMssMCB.mmWaveCfg.calibCfg.txBackoffSel <= TX_BACKOFF_PWR_MON_MAX) 
    {
        /*Configuring Power Monitors Tx0A if its enabled*/  
        if(gMmwMssMCB.ismonTxpwrCfg[0] == 1U)
        {
            if((gMmwMssMCB.mmWaveCfg.calibCfg.txBackoffSel > DUAL_CAL_TX_BACKOFF_MIN) && (gMmwMssMCB.mmWaveCfg.calibCfg.txBackoffSel < DUAL_CAL_TX_BACKOFF_MAX))
            {
                gMmwMssMCB.monTxpwrCfg[0].txBiasSel = 2U;
                gMmwMssMCB.monTxpwrCfg[0].txBackoff = DEFAULT_TX_CLPC_BACKOFF_SEL;
                gMmwMssMCB.monTxpwrCfg[0].txBiasCode = gMmwMssMCB.mmWaveCfg.clpc13dBTxCode[0];
            }
            retVal = MMWaveMon_TxnPowCfg(0,&gMmwMssMCB.monTxpwrCfg[0]);
            if(retVal < 0)
            {
                CLI_write("Incorrect Tx0 Monitor Power Cfg\n");
                DebugP_assert(0);
            }
        }
        /*Configuring Power Monitors Tx0B if its enabled*/  
        if(gMmwMssMCB.ismonTxpwrCfg[1] == 1U)
        {
            if((gMmwMssMCB.mmWaveCfg.calibCfg.txBackoffSel > DUAL_CAL_TX_BACKOFF_MIN) && (gMmwMssMCB.mmWaveCfg.calibCfg.txBackoffSel < DUAL_CAL_TX_BACKOFF_MAX))
            {
                gMmwMssMCB.monTxpwrCfg[1].txBiasSel = 2U;
                gMmwMssMCB.monTxpwrCfg[1].txBackoff = DEFAULT_TX_CLPC_BACKOFF_SEL;
                gMmwMssMCB.monTxpwrCfg[1].txBiasCode = gMmwMssMCB.mmWaveCfg.clpc13dBTxCode[1];
            }
            retVal = MMWaveMon_TxnPowCfg(1,&gMmwMssMCB.monTxpwrCfg[1]);
            if(retVal < 0)
            {
                CLI_write("Incorrect Tx1 Monitor Power Cfg\n");
                DebugP_assert(0);
            }
        }
        /*Configuring Power Monitors Tx1A if its enabled*/  
        if(gMmwMssMCB.ismonTxpwrCfg[2] == 1U)
        {
            if((gMmwMssMCB.mmWaveCfg.calibCfg.txBackoffSel > DUAL_CAL_TX_BACKOFF_MIN) && (gMmwMssMCB.mmWaveCfg.calibCfg.txBackoffSel < DUAL_CAL_TX_BACKOFF_MAX))
            {
                gMmwMssMCB.monTxpwrCfg[2].txBiasSel = 2U;
                gMmwMssMCB.monTxpwrCfg[2].txBackoff = DEFAULT_TX_CLPC_BACKOFF_SEL;
                gMmwMssMCB.monTxpwrCfg[2].txBiasCode = gMmwMssMCB.mmWaveCfg.clpc13dBTxCode[2];
            }
            retVal = MMWaveMon_TxnPowCfg(2,&gMmwMssMCB.monTxpwrCfg[2]);
            if(retVal < 0)
            {
                CLI_write("Incorrect Tx2 Monitor Power Cfg\n");
                DebugP_assert(0);
            }
        }
        /*Configuring Power Monitors Tx1B if its enabled*/  
        if(gMmwMssMCB.ismonTxpwrCfg[3] == 1U)
        {
            if((gMmwMssMCB.mmWaveCfg.calibCfg.txBackoffSel > DUAL_CAL_TX_BACKOFF_MIN) && (gMmwMssMCB.mmWaveCfg.calibCfg.txBackoffSel < DUAL_CAL_TX_BACKOFF_MAX))
            {
                gMmwMssMCB.monTxpwrCfg[3].txBiasSel = 2U;
                gMmwMssMCB.monTxpwrCfg[3].txBackoff = DEFAULT_TX_CLPC_BACKOFF_SEL;
                gMmwMssMCB.monTxpwrCfg[3].txBiasCode = gMmwMssMCB.mmWaveCfg.clpc13dBTxCode[3];
            }
            retVal = MMWaveMon_TxnPowCfg(3,&gMmwMssMCB.monTxpwrCfg[3]);
            if(retVal < 0)
            {
                CLI_write("Incorrect Tx3 Monitor Power Cfg\n");
                DebugP_assert(0);
            }
        }
        /*Configuring Power Monitors Tx2A if its enabled*/  
        if(gMmwMssMCB.ismonTxpwrCfg[4] == 1U)
        {
            if((gMmwMssMCB.mmWaveCfg.calibCfg.txBackoffSel > DUAL_CAL_TX_BACKOFF_MIN) && (gMmwMssMCB.mmWaveCfg.calibCfg.txBackoffSel < DUAL_CAL_TX_BACKOFF_MAX))
            {
                gMmwMssMCB.monTxpwrCfg[4].txBiasSel = 2U;
                gMmwMssMCB.monTxpwrCfg[4].txBackoff = DEFAULT_TX_CLPC_BACKOFF_SEL;
                gMmwMssMCB.monTxpwrCfg[4].txBiasCode = gMmwMssMCB.mmWaveCfg.clpc13dBTxCode[4];
            }
            retVal = MMWaveMon_TxnPowCfg(4,&gMmwMssMCB.monTxpwrCfg[4]);
            if(retVal < 0)
            {
                CLI_write("Incorrect Tx4 Monitor Power Cfg\n");
                DebugP_assert(0);
            }
        }
        /*Configuring Power Monitors Tx2B if its enabled*/  
        if(gMmwMssMCB.ismonTxpwrCfg[5] == 1U)
        {
            if((gMmwMssMCB.mmWaveCfg.calibCfg.txBackoffSel > DUAL_CAL_TX_BACKOFF_MIN) && (gMmwMssMCB.mmWaveCfg.calibCfg.txBackoffSel < DUAL_CAL_TX_BACKOFF_MAX))
            {
                gMmwMssMCB.monTxpwrCfg[5].txBiasSel = 2U;
                gMmwMssMCB.monTxpwrCfg[5].txBackoff = DEFAULT_TX_CLPC_BACKOFF_SEL;
                gMmwMssMCB.monTxpwrCfg[5].txBiasCode = gMmwMssMCB.mmWaveCfg.clpc13dBTxCode[5];
            }
            retVal = MMWaveMon_TxnPowCfg(5,&gMmwMssMCB.monTxpwrCfg[5]);
            if(retVal < 0)
            {
                CLI_write("Incorrect Tx5 Monitor Power Cfg\n");
                DebugP_assert(0);
            }
        }
        /*Configuring Power Monitors Tx3A if its enabled*/  
        if(gMmwMssMCB.ismonTxpwrCfg[6] == 1U)
        {
            if((gMmwMssMCB.mmWaveCfg.calibCfg.txBackoffSel > DUAL_CAL_TX_BACKOFF_MIN) && (gMmwMssMCB.mmWaveCfg.calibCfg.txBackoffSel < DUAL_CAL_TX_BACKOFF_MAX))
            {
                gMmwMssMCB.monTxpwrCfg[6].txBiasSel = 2U;
                gMmwMssMCB.monTxpwrCfg[6].txBackoff = DEFAULT_TX_CLPC_BACKOFF_SEL;
                gMmwMssMCB.monTxpwrCfg[6].txBiasCode = gMmwMssMCB.mmWaveCfg.clpc13dBTxCode[6];
            }
            retVal = MMWaveMon_TxnPowCfg(6,&gMmwMssMCB.monTxpwrCfg[6]);
            if(retVal < 0)
            {
                CLI_write("Incorrect Tx6 Monitor Power Cfg\n");
                DebugP_assert(0);
            }
        }
        /*Configuring Power Monitors Tx3B if its enabled*/  
        if(gMmwMssMCB.ismonTxpwrCfg[7] == 1U)
        {
            if((gMmwMssMCB.mmWaveCfg.calibCfg.txBackoffSel > DUAL_CAL_TX_BACKOFF_MIN) && (gMmwMssMCB.mmWaveCfg.calibCfg.txBackoffSel < DUAL_CAL_TX_BACKOFF_MAX))
            {
                gMmwMssMCB.monTxpwrCfg[7].txBiasSel = 2U;
                gMmwMssMCB.monTxpwrCfg[7].txBackoff = DEFAULT_TX_CLPC_BACKOFF_SEL;
                gMmwMssMCB.monTxpwrCfg[7].txBiasCode = gMmwMssMCB.mmWaveCfg.clpc13dBTxCode[7];
            }
            retVal = MMWaveMon_TxnPowCfg(7,&gMmwMssMCB.monTxpwrCfg[7]);
            if(retVal < 0)
            {
                CLI_write("Incorrect Tx7 Monitor Power Cfg\n");
                DebugP_assert(0);
            }
        }
    }
    else
    {
        if(gMmwMssMCB.ismonTxpwrCfg[0] == 1U || gMmwMssMCB.ismonTxpwrCfg[1] == 1U || gMmwMssMCB.ismonTxpwrCfg[2] == 1U || gMmwMssMCB.ismonTxpwrCfg[3] == 1U || gMmwMssMCB.ismonTxpwrCfg[4] == 1U || gMmwMssMCB.ismonTxpwrCfg[5] == 1U || gMmwMssMCB.ismonTxpwrCfg[6] == 1U || gMmwMssMCB.ismonTxpwrCfg[7] == 1U)
        {
            CLI_write("Tx Power Monitors are skipped as Tx Backoff is greater than %d.\n", TX_BACKOFF_PWR_MON_MAX);
        }

    }
    
    /* Ballbreak monitor requires TX Backoff to be <= TX_BACKOFF_BALL_BREAK_MON_MAX */  
    /* Configuring Ball Break Monitors Tx0A if its enabled */
    
    if(gMmwMssMCB.ismonTxpwrBBCfg[0] == 1U)
    {
        if(gMmwMssMCB.mmWaveCfg.calibCfg.txBackoffSel <= TX_BACKOFF_BALL_BREAK_MON_MAX)
        {   
            retVal = MMWaveMon_TxnPowBBCfg(0,&gMmwMssMCB.monTxpwrBBCfg[0]);
            if(retVal < 0)
            {
                CLI_write("Incorrect Tx0 Monitor Power Ball Break Cfg\n");
                DebugP_assert(0);
            }
        }
        else
        {
            CLI_write("Ball Break Monitor skipped as Tx Backoff is > %d.\n", TX_BACKOFF_BALL_BREAK_MON_MAX);
        }
    }
    /*Configuring Ball Break Monitors Tx1A if its enabled*/  
    if(gMmwMssMCB.ismonTxpwrBBCfg[2] == 1U)
    {
        if(gMmwMssMCB.mmWaveCfg.calibCfg.txBackoffSel <= TX_BACKOFF_BALL_BREAK_MON_MAX)
        {
            retVal = MMWaveMon_TxnPowBBCfg(2,&gMmwMssMCB.monTxpwrBBCfg[2]);
            if(retVal < 0)
            {
                CLI_write("Incorrect Tx2 Monitor Power Ball Break Cfg\n");
                DebugP_assert(0);
            }
        }
        else
        {
            CLI_write("Ball Break Monitor skipped as Tx Backoff is > %d.\n", TX_BACKOFF_BALL_BREAK_MON_MAX);
        }
    }
    /*Configuring Ball Break Monitors Tx2A if its enabled*/  
    if(gMmwMssMCB.ismonTxpwrBBCfg[4] == 1U)
    {
        if(gMmwMssMCB.mmWaveCfg.calibCfg.txBackoffSel <= TX_BACKOFF_BALL_BREAK_MON_MAX)
        {
            retVal = MMWaveMon_TxnPowBBCfg(4,&gMmwMssMCB.monTxpwrBBCfg[4]);
            if(retVal < 0)
            {
                CLI_write("Incorrect Tx4 Monitor Power Ball Break Cfg\n");
                DebugP_assert(0);
            }
        }
        else
        {
            CLI_write("Ball Break Monitor skipped as Tx Backoff is > %d.\n", TX_BACKOFF_BALL_BREAK_MON_MAX);
        }
    }
    /*Configuring Ball Break Monitors Tx3A if its enabled*/  
    if(gMmwMssMCB.ismonTxpwrBBCfg[6] == 1U)
    {
        if(gMmwMssMCB.mmWaveCfg.calibCfg.txBackoffSel <= TX_BACKOFF_BALL_BREAK_MON_MAX)
        {
            retVal = MMWaveMon_TxnPowBBCfg(6,&gMmwMssMCB.monTxpwrBBCfg[6]);
            if(retVal < 0)
            {
                CLI_write("Incorrect Tx6 Monitor Power Ball Break Cfg\n");
                DebugP_assert(0);
            }
        }
        else
        {
            CLI_write("Ball Break Monitor skipped as Tx Backoff is > %d.\n", TX_BACKOFF_BALL_BREAK_MON_MAX);
        }
    }
    /*Configuring LoopBack Monitors Tx0A if its enabled*/
    if(gMmwMssMCB.ismonTxRxLbCfg[0] == 1)
    {
        retVal = MMWaveMon_TxRxLbCfg(0,&gMmwMssMCB.monTxRxLbCfg[0]);
        if(retVal < 0)
        {
            CLI_write("Incorrect Tx0A Loop back Cfg\n");
            DebugP_assert(0);
        }
    }
    /*Configuring LoopBack Monitors Tx1A if its enabled*/
    if(gMmwMssMCB.ismonTxRxLbCfg[2] == 1)
    {
        retVal = MMWaveMon_TxRxLbCfg(2,&gMmwMssMCB.monTxRxLbCfg[2]);
        if(retVal < 0)
        {
            CLI_write("Incorrect Tx1A Loop back Cfg\n");
            DebugP_assert(0);
        }
    }
     /*Configuring LoopBack Monitors Tx2A if its enabled*/
    if(gMmwMssMCB.ismonTxRxLbCfg[4] == 1)
    {
        retVal = MMWaveMon_TxRxLbCfg(4,&gMmwMssMCB.monTxRxLbCfg[4]);
        if(retVal < 0)
        {
            CLI_write("Incorrect Tx2A Loop back Cfg\n");
            DebugP_assert(0);
        }
    }
     /*Configuring LoopBack Monitors Tx3A if its enabled*/
    if(gMmwMssMCB.ismonTxRxLbCfg[6] == 1)
    {
        retVal = MMWaveMon_TxRxLbCfg(6,&gMmwMssMCB.monTxRxLbCfg[6]);
        if(retVal < 0)
        {
            CLI_write("Incorrect Tx3A Loop back Cfg\n");
            DebugP_assert(0);
        }
    }
    /*Configuring DC Signal Monitors Tx0A & Tx0B if its enabled*/  
    if(gMmwMssMCB.ismonTxDcSigCfg[0] == 1U)
    {
        retVal = MMWaveMon_TxnDcSigCfg(0,&gMmwMssMCB.monTxDcSigCfg[0]);
        if(retVal < 0)
        {
            CLI_write("Incorrect Tx0A & Tx0B Monitor Dc Sig Cfg\n");
            DebugP_assert(0);
        }
    }
    /*Configuring DC Signal Monitors Tx1A & Tx1B if its enabled*/  
    if(gMmwMssMCB.ismonTxDcSigCfg[1] == 1U)
    {
        retVal = MMWaveMon_TxnDcSigCfg(1,&gMmwMssMCB.monTxDcSigCfg[1]);
        if(retVal < 0)
        {
            CLI_write("Incorrect Tx1A & Tx1B Monitor Dc Sig Cfg\n");
            DebugP_assert(0);
        }
    }
    /*Configuring DC Signal Monitors Tx2A & Tx2B if its enabled*/  
    if(gMmwMssMCB.ismonTxDcSigCfg[2] == 1U)
    {
        retVal = MMWaveMon_TxnDcSigCfg(2,&gMmwMssMCB.monTxDcSigCfg[2]);
        if(retVal < 0)
        {
            CLI_write("Incorrect Tx2A & Tx2B Monitor Dc Sig Cfg\n");
            DebugP_assert(0);
        }
    }
    /*Configuring DC Signal Monitors Tx3A & Tx3B if its enabled*/  
    if(gMmwMssMCB.ismonTxDcSigCfg[3] == 1U)
    {
        retVal = MMWaveMon_TxnDcSigCfg(3,&gMmwMssMCB.monTxDcSigCfg[3]);
        if(retVal < 0)
        {
            CLI_write("Incorrect Tx3A & Tx3B Monitor Dc Sig Cfg\n");
            DebugP_assert(0);
        }
    }
    /*Configuring RX HPF DC Signal Monitors if its enabled*/  
    if(gMmwMssMCB.monRxHpfDcSigCfg.monenbl != 0U)
    {
        
        retVal = MMWaveMon_RxHpfDcSigCfg(&gMmwMssMCB.monRxHpfDcSigCfg);
        if(retVal < 0)
        {
            CLI_write("Incorrect RX HPF Dc Sig Monitor Cfg\n");
            DebugP_assert(0);
        }
    }
    /*Configuring PM CLK DC Monitors if its enabled*/  
    if(gMmwMssMCB.monPmClkDcStFreqGhz != 0U)
    {
        
        retVal = MMWaveMon_PmClkDcSigCfg(gMmwMssMCB.monPmClkDcStFreqGhz);
        if(retVal < 0)
        {
            CLI_write("Incorrect Pm Clk Dc Sig Monitor Cfg\n");
            DebugP_assert(0);
        }
    }
    retVal = MMWave_monitorsCfg(gMmwMssMCB.rfMonEnbl, mmwDemoMonitorISR);
    if(retVal < 0)
    {
        CLI_write("Monitors Configuration failed!!\n");
        DebugP_assert(0);
    }
}

/**
*  @b Description
*  @n
 *      This function is used to get the results of RF monitors that are enabled.
 *
 */
void mmwDemo_GetMonRes()
{  
    /* Get Status of all Enabled monitors */
    gMmwMssMCB.rfMonRes.monStat = MMWaveMon_getStatus(gMmwMssMCB.rfMonEnbl);

    /* If PLL Vol Monitor is enabled and monitor is passed, read the value */
    if( (gMmwMssMCB.rfMonRes.monStat & (0x1U << M_RL_MON_PLL_CTRL_VOLT)) & (gMmwMssMCB.rfMonEnbl & (0x1U << M_RL_MON_PLL_CTRL_VOLT)))
    {
        gMmwMssMCB.rfMonRes.pllvolres = MMWaveMon_getPllVolMonres(gMmwMssMCB.monPllVolEnaMask);
        /* Comparing results with Spec Values and setting Result Status bits */
            if(gMmwMssMCB.rfMonRes.pllvolres.apllV >= gMmwMssMCB.SpecVal.APLLVSpecMin 
                && gMmwMssMCB.rfMonRes.pllvolres.apllV <= gMmwMssMCB.SpecVal.APLLVSpecMax )
            {
                gMmwMssMCB.rfMonRes.status_pllvolres=(gMmwMssMCB.rfMonRes.status_pllvolres | 0x1);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_pllvolres=(gMmwMssMCB.rfMonRes.status_pllvolres & 0xFE);
            }

            if(gMmwMssMCB.rfMonRes.pllvolres.synthMinV >= gMmwMssMCB.SpecVal.SynthMinVSpecMin)
            {
                gMmwMssMCB.rfMonRes.status_pllvolres=(gMmwMssMCB.rfMonRes.status_pllvolres | 0x2);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_pllvolres=(gMmwMssMCB.rfMonRes.status_pllvolres & 0xFD);
            }

            if(gMmwMssMCB.rfMonRes.pllvolres.synthMaxV <= gMmwMssMCB.SpecVal.SynthMaxVSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_pllvolres=(gMmwMssMCB.rfMonRes.status_pllvolres | 0x4);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_pllvolres=(gMmwMssMCB.rfMonRes.status_pllvolres & 0xFB);
            }
    }
    
    if(gMmwMssMCB.mmWaveCfg.calibCfg.txBackoffSel <= TX_BACKOFF_PWR_MON_MAX) 
    {
        /* If TX0 Power monitor is enabled and monitor is passed, read the value */
        if( (gMmwMssMCB.rfMonRes.monStat & (0x1U << M_RL_MON_TX0A_POWER)) & (gMmwMssMCB.rfMonEnbl & (0x1U << M_RL_MON_TX0A_POWER)))
        {
            gMmwMssMCB.rfMonRes.txPower[0] = MMWaveMon_getTXnPow(0);
            /* Comparing results with Spec Values and setting Result Status bits */ 
            if(gMmwMssMCB.rfMonRes.txPower[0] >= gMmwMssMCB.SpecVal.TxPowSpecMin[0])
            {
                gMmwMssMCB.rfMonRes.status_txPower=(gMmwMssMCB.rfMonRes.status_txPower | 0x1);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txPower=(gMmwMssMCB.rfMonRes.status_txPower & 0xFFFE);
            }
        }
        /* If TX1 Power monitor is enabled and monitor is passed, read the value */
        if( (gMmwMssMCB.rfMonRes.monStat & (0x1U << M_RL_MON_TX0B_POWER)) & (gMmwMssMCB.rfMonEnbl & (0x1U << M_RL_MON_TX0B_POWER)))
        {
            gMmwMssMCB.rfMonRes.txPower[1] = MMWaveMon_getTXnPow(1);
            gMmwMssMCB.rfMonRes.measGainMismatch[0] = gMmwMssMCB.rfMonRes.txPower[1] - gMmwMssMCB.rfMonRes.txPower[0];
            gMmwMssMCB.rfMonRes.refGainMismatch[0] = rfMonResFactCal.txPower[1] - rfMonResFactCal.txPower[0];
            gMmwMssMCB.rfMonRes.gainMismatchVar[0] = gMmwMssMCB.rfMonRes.measGainMismatch[0] - gMmwMssMCB.rfMonRes.refGainMismatch[0];
            /* Comparing results with Spec Values and setting Result Status bits */ 
            if(gMmwMssMCB.rfMonRes.txPower[1] >= gMmwMssMCB.SpecVal.TxPowSpecMin[1])
            {
                gMmwMssMCB.rfMonRes.status_txPower=(gMmwMssMCB.rfMonRes.status_txPower | 0x2);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txPower=(gMmwMssMCB.rfMonRes.status_txPower & 0xFFFD);
            }
            if((gMmwMssMCB.rfMonRes.gainMismatchVar[0] >= gMmwMssMCB.SpecVal.TxPowGainMismatchSpecMin) && (gMmwMssMCB.rfMonRes.gainMismatchVar[0] <= gMmwMssMCB.SpecVal.TxPowGainMismatchSpecMax))
            {
                gMmwMssMCB.rfMonRes.status_txPower=(gMmwMssMCB.rfMonRes.status_txPower | 0x100);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txPower=(gMmwMssMCB.rfMonRes.status_txPower & 0xFEFF);
            }
        }
        /* If TX2 Power monitor is enabled and monitor is passed, read the value */
        if( (gMmwMssMCB.rfMonRes.monStat & (0x1U << M_RL_MON_TX1A_POWER)) & (gMmwMssMCB.rfMonEnbl & (0x1U << M_RL_MON_TX1A_POWER)))
        {
            gMmwMssMCB.rfMonRes.txPower[2] = MMWaveMon_getTXnPow(2);
            gMmwMssMCB.rfMonRes.measGainMismatch[1] = gMmwMssMCB.rfMonRes.txPower[2] - gMmwMssMCB.rfMonRes.txPower[0];
            gMmwMssMCB.rfMonRes.refGainMismatch[1] = rfMonResFactCal.txPower[2] - rfMonResFactCal.txPower[0];
            gMmwMssMCB.rfMonRes.gainMismatchVar[1] = gMmwMssMCB.rfMonRes.measGainMismatch[1] - gMmwMssMCB.rfMonRes.refGainMismatch[1];
            /* Comparing results with Spec Values and setting Result Status bits */ 
            if(gMmwMssMCB.rfMonRes.txPower[2] >= gMmwMssMCB.SpecVal.TxPowSpecMin[2])
            {
                gMmwMssMCB.rfMonRes.status_txPower=(gMmwMssMCB.rfMonRes.status_txPower | 0x4);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txPower=(gMmwMssMCB.rfMonRes.status_txPower & 0xFFFB);
            }
            if((gMmwMssMCB.rfMonRes.gainMismatchVar[1] >= gMmwMssMCB.SpecVal.TxPowGainMismatchSpecMin) && (gMmwMssMCB.rfMonRes.gainMismatchVar[1] <= gMmwMssMCB.SpecVal.TxPowGainMismatchSpecMax))
            {
                gMmwMssMCB.rfMonRes.status_txPower=(gMmwMssMCB.rfMonRes.status_txPower | 0x200);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txPower=(gMmwMssMCB.rfMonRes.status_txPower & 0xFDFF);
            }
        }
        /* If TX3 Power monitor is enabled and monitor is passed, read the value */
        if( (gMmwMssMCB.rfMonRes.monStat & (0x1U << M_RL_MON_TX1B_POWER)) & (gMmwMssMCB.rfMonEnbl & (0x1U << M_RL_MON_TX1B_POWER)))
        {
            gMmwMssMCB.rfMonRes.txPower[3] = MMWaveMon_getTXnPow(3);
            gMmwMssMCB.rfMonRes.measGainMismatch[2] = gMmwMssMCB.rfMonRes.txPower[3] - gMmwMssMCB.rfMonRes.txPower[0];
            gMmwMssMCB.rfMonRes.refGainMismatch[2] = rfMonResFactCal.txPower[3] - rfMonResFactCal.txPower[0];
            gMmwMssMCB.rfMonRes.gainMismatchVar[2] = gMmwMssMCB.rfMonRes.measGainMismatch[2] - gMmwMssMCB.rfMonRes.refGainMismatch[2];
            /* Comparing results with Spec Values and setting Result Status bits */ 
            if(gMmwMssMCB.rfMonRes.txPower[3] >= gMmwMssMCB.SpecVal.TxPowSpecMin[3])
            {
                gMmwMssMCB.rfMonRes.status_txPower=(gMmwMssMCB.rfMonRes.status_txPower | 0x08);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txPower=(gMmwMssMCB.rfMonRes.status_txPower & 0xFFF7);
            }
            if((gMmwMssMCB.rfMonRes.gainMismatchVar[2] >= gMmwMssMCB.SpecVal.TxPowGainMismatchSpecMin) && (gMmwMssMCB.rfMonRes.gainMismatchVar[2] <= gMmwMssMCB.SpecVal.TxPowGainMismatchSpecMax))
            {
                gMmwMssMCB.rfMonRes.status_txPower=(gMmwMssMCB.rfMonRes.status_txPower | 0x400);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txPower=(gMmwMssMCB.rfMonRes.status_txPower & 0xFBFF);
            }
        }
        /* If TX4 Power monitor is enabled and monitor is passed, read the value */
        if( (gMmwMssMCB.rfMonRes.monStat & (0x1U << M_RL_MON_TX2A_POWER)) & (gMmwMssMCB.rfMonEnbl & (0x1U << M_RL_MON_TX2A_POWER)))
        {
            gMmwMssMCB.rfMonRes.txPower[4] = MMWaveMon_getTXnPow(4);
            gMmwMssMCB.rfMonRes.measGainMismatch[3] = gMmwMssMCB.rfMonRes.txPower[4] - gMmwMssMCB.rfMonRes.txPower[0];
            gMmwMssMCB.rfMonRes.refGainMismatch[3] = rfMonResFactCal.txPower[4] - rfMonResFactCal.txPower[0];
            gMmwMssMCB.rfMonRes.gainMismatchVar[3] = gMmwMssMCB.rfMonRes.measGainMismatch[3] - gMmwMssMCB.rfMonRes.refGainMismatch[3];
            /* Comparing results with Spec Values and setting Result Status bits */ 
            if(gMmwMssMCB.rfMonRes.txPower[4] >= gMmwMssMCB.SpecVal.TxPowSpecMin[4])
            {
                gMmwMssMCB.rfMonRes.status_txPower=(gMmwMssMCB.rfMonRes.status_txPower | 0x10);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txPower=(gMmwMssMCB.rfMonRes.status_txPower & 0xFFEF);
            }
            if((gMmwMssMCB.rfMonRes.gainMismatchVar[3] >= gMmwMssMCB.SpecVal.TxPowGainMismatchSpecMin) && (gMmwMssMCB.rfMonRes.gainMismatchVar[3] <= gMmwMssMCB.SpecVal.TxPowGainMismatchSpecMax))
            {
                gMmwMssMCB.rfMonRes.status_txPower=(gMmwMssMCB.rfMonRes.status_txPower | 0x800);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txPower=(gMmwMssMCB.rfMonRes.status_txPower & 0xF7FF);
            }
        }
        /* If TX5 Power monitor is enabled and monitor is passed, read the value */
        if( (gMmwMssMCB.rfMonRes.monStat & (0x1U << M_RL_MON_TX2B_POWER)) & (gMmwMssMCB.rfMonEnbl & (0x1U << M_RL_MON_TX2B_POWER)))
        {
            gMmwMssMCB.rfMonRes.txPower[5] = MMWaveMon_getTXnPow(5);
            gMmwMssMCB.rfMonRes.measGainMismatch[4] = gMmwMssMCB.rfMonRes.txPower[5] - gMmwMssMCB.rfMonRes.txPower[0];
            gMmwMssMCB.rfMonRes.refGainMismatch[4] = rfMonResFactCal.txPower[5] - rfMonResFactCal.txPower[0];
            gMmwMssMCB.rfMonRes.gainMismatchVar[4] = gMmwMssMCB.rfMonRes.measGainMismatch[4] - gMmwMssMCB.rfMonRes.refGainMismatch[4];
            /* Comparing results with Spec Values and setting Result Status bits */ 
            if(gMmwMssMCB.rfMonRes.txPower[5] >= gMmwMssMCB.SpecVal.TxPowSpecMin[5])
            {
                gMmwMssMCB.rfMonRes.status_txPower=(gMmwMssMCB.rfMonRes.status_txPower | 0x20);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txPower=(gMmwMssMCB.rfMonRes.status_txPower & 0xFFDF);
            }
            if((gMmwMssMCB.rfMonRes.gainMismatchVar[4] >= gMmwMssMCB.SpecVal.TxPowGainMismatchSpecMin) && (gMmwMssMCB.rfMonRes.gainMismatchVar[4] <= gMmwMssMCB.SpecVal.TxPowGainMismatchSpecMax))
            {
                gMmwMssMCB.rfMonRes.status_txPower=(gMmwMssMCB.rfMonRes.status_txPower | 0x1000);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txPower=(gMmwMssMCB.rfMonRes.status_txPower & 0xEFFF);
            }
        }
        /* If TX6 Power monitor is enabled and monitor is passed, read the value */
        if( (gMmwMssMCB.rfMonRes.monStat & (0x1U << M_RL_MON_TX3A_POWER)) & (gMmwMssMCB.rfMonEnbl & (0x1U << M_RL_MON_TX3A_POWER)))
        {
            gMmwMssMCB.rfMonRes.txPower[6] = MMWaveMon_getTXnPow(6);
            gMmwMssMCB.rfMonRes.measGainMismatch[5] = gMmwMssMCB.rfMonRes.txPower[6] - gMmwMssMCB.rfMonRes.txPower[0];
            gMmwMssMCB.rfMonRes.refGainMismatch[5] = rfMonResFactCal.txPower[6] - rfMonResFactCal.txPower[0];
            gMmwMssMCB.rfMonRes.gainMismatchVar[5] = gMmwMssMCB.rfMonRes.measGainMismatch[5] - gMmwMssMCB.rfMonRes.refGainMismatch[5];
            /* Comparing results with Spec Values and setting Result Status bits */ 
            if(gMmwMssMCB.rfMonRes.txPower[6] >= gMmwMssMCB.SpecVal.TxPowSpecMin[6])
            {
                gMmwMssMCB.rfMonRes.status_txPower=(gMmwMssMCB.rfMonRes.status_txPower | 0x40);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txPower=(gMmwMssMCB.rfMonRes.status_txPower & 0xFFBF);
            }
            if((gMmwMssMCB.rfMonRes.gainMismatchVar[5] >= gMmwMssMCB.SpecVal.TxPowGainMismatchSpecMin) && (gMmwMssMCB.rfMonRes.gainMismatchVar[5] <= gMmwMssMCB.SpecVal.TxPowGainMismatchSpecMax))
            {
                gMmwMssMCB.rfMonRes.status_txPower=(gMmwMssMCB.rfMonRes.status_txPower | 0x2000);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txPower=(gMmwMssMCB.rfMonRes.status_txPower & 0xDFFF);
            }
        }
        /* If TX7 Power monitor is enabled and monitor is passed, read the value */
        if( (gMmwMssMCB.rfMonRes.monStat & (0x1U << M_RL_MON_TX3B_POWER)) & (gMmwMssMCB.rfMonEnbl & (0x1U << M_RL_MON_TX3B_POWER)))
        {
            gMmwMssMCB.rfMonRes.txPower[7] = MMWaveMon_getTXnPow(7);
            gMmwMssMCB.rfMonRes.measGainMismatch[6] = gMmwMssMCB.rfMonRes.txPower[7] - gMmwMssMCB.rfMonRes.txPower[0];
            gMmwMssMCB.rfMonRes.refGainMismatch[6] = rfMonResFactCal.txPower[7] - rfMonResFactCal.txPower[0];
            gMmwMssMCB.rfMonRes.gainMismatchVar[6] = gMmwMssMCB.rfMonRes.measGainMismatch[6] - gMmwMssMCB.rfMonRes.refGainMismatch[6];
            /* Comparing results with Spec Values and setting Result Status bits */ 
            if(gMmwMssMCB.rfMonRes.txPower[7] >= gMmwMssMCB.SpecVal.TxPowSpecMin[7])
            {
                gMmwMssMCB.rfMonRes.status_txPower=(gMmwMssMCB.rfMonRes.status_txPower | 0x80);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txPower=(gMmwMssMCB.rfMonRes.status_txPower & 0xFF7F);
            }
            if((gMmwMssMCB.rfMonRes.gainMismatchVar[6] >= gMmwMssMCB.SpecVal.TxPowGainMismatchSpecMin) && (gMmwMssMCB.rfMonRes.gainMismatchVar[6] <= gMmwMssMCB.SpecVal.TxPowGainMismatchSpecMax))
            {
                gMmwMssMCB.rfMonRes.status_txPower=(gMmwMssMCB.rfMonRes.status_txPower | 0x4000);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txPower=(gMmwMssMCB.rfMonRes.status_txPower & 0xBFFF);
            }
        }
    }
    /* If TX0 Power ball break monitor is enabled and monitor is passed, read the value */
    if( (gMmwMssMCB.rfMonRes.monStat & (0x1U << M_RL_MON_TX0A_BB)) & (gMmwMssMCB.rfMonEnbl & (0x1U << M_RL_MON_TX0A_BB)))
    {
        if(gMmwMssMCB.mmWaveCfg.calibCfg.txBackoffSel <= TX_BACKOFF_BALL_BREAK_MON_MAX)
        {   
            gMmwMssMCB.rfMonRes.txPowerBB[0] = MMWaveMon_getTXnPowBB(0);
            /* Calculating variation from Factory Cal Data*/
            gMmwMssMCB.rfMonRes.txPowerBBretlossMismatch[0]= gMmwMssMCB.rfMonRes.txPowerBB[0].txReturnLoss - rfMonResFactCal.txBBReturnLoss[0];
            /* Comparing results with Spec Values and setting Result Status bits */ 
            if((gMmwMssMCB.rfMonRes.txPowerBB[0].txReturnLoss <= gMmwMssMCB.SpecVal.TxBBRetLossSpec
                || gMmwMssMCB.rfMonRes.txPowerBBretlossMismatch[0] <= gMmwMssMCB.SpecVal.TxBBRetLossVarSpec ) && gMmwMssMCB.rfMonRes.txPowerBB[0].txIncPow >= gMmwMssMCB.SpecVal.TxPowSpecMin[0])
            {
                gMmwMssMCB.rfMonRes.status_txPowerBB=(gMmwMssMCB.rfMonRes.status_txPowerBB | 0x1);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txPowerBB=(gMmwMssMCB.rfMonRes.status_txPowerBB & 0xFE);
            }
        }
    }
    /* If TX2 Power ball break monitor is enabled and monitor is passed, read the value */
    if( (gMmwMssMCB.rfMonRes.monStat & (0x1U << M_RL_MON_TX1A_BB)) & (gMmwMssMCB.rfMonEnbl & (0x1U << M_RL_MON_TX1A_BB)))
    {
        if(gMmwMssMCB.mmWaveCfg.calibCfg.txBackoffSel <= TX_BACKOFF_BALL_BREAK_MON_MAX)
        {   
            gMmwMssMCB.rfMonRes.txPowerBB[2] = MMWaveMon_getTXnPowBB(2);
            gMmwMssMCB.rfMonRes.txPowerBBretlossMismatch[2]= gMmwMssMCB.rfMonRes.txPowerBB[2].txReturnLoss - rfMonResFactCal.txBBReturnLoss[2];
            /* Comparing results with Spec Values and setting Result Status bits */ 
            if((gMmwMssMCB.rfMonRes.txPowerBB[2].txReturnLoss <= gMmwMssMCB.SpecVal.TxBBRetLossSpec
                || gMmwMssMCB.rfMonRes.txPowerBBretlossMismatch[2] <= gMmwMssMCB.SpecVal.TxBBRetLossVarSpec ) && gMmwMssMCB.rfMonRes.txPowerBB[2].txIncPow >= gMmwMssMCB.SpecVal.TxPowSpecMin[2])
            {
                gMmwMssMCB.rfMonRes.status_txPowerBB=(gMmwMssMCB.rfMonRes.status_txPowerBB | 0x4);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txPowerBB=(gMmwMssMCB.rfMonRes.status_txPowerBB & 0xFB);
            }
        }
    }
    /* If TX4 Power ball break monitor is enabled and monitor is passed, read the value */
    if( (gMmwMssMCB.rfMonRes.monStat & (0x1U << M_RL_MON_TX2A_BB)) & (gMmwMssMCB.rfMonEnbl & (0x1U << M_RL_MON_TX2A_BB)))
    {
        if(gMmwMssMCB.mmWaveCfg.calibCfg.txBackoffSel <= TX_BACKOFF_BALL_BREAK_MON_MAX)
        {   
            gMmwMssMCB.rfMonRes.txPowerBB[4] = MMWaveMon_getTXnPowBB(4);
            gMmwMssMCB.rfMonRes.txPowerBBretlossMismatch[4]= gMmwMssMCB.rfMonRes.txPowerBB[4].txReturnLoss - rfMonResFactCal.txBBReturnLoss[4];
            /* Comparing results with Spec Values and setting Result Status bits */ 
            if((gMmwMssMCB.rfMonRes.txPowerBB[2].txReturnLoss <= gMmwMssMCB.SpecVal.TxBBRetLossSpec
                || gMmwMssMCB.rfMonRes.txPowerBBretlossMismatch[4] <= gMmwMssMCB.SpecVal.TxBBRetLossVarSpec ) && gMmwMssMCB.rfMonRes.txPowerBB[4].txIncPow >= gMmwMssMCB.SpecVal.TxPowSpecMin[4])
            {
                gMmwMssMCB.rfMonRes.status_txPowerBB=(gMmwMssMCB.rfMonRes.status_txPowerBB | 0x10);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txPowerBB=(gMmwMssMCB.rfMonRes.status_txPowerBB & 0xEF);
            }
        }
    }
    /* If TX6 Power ball break monitor is enabled and monitor is passed, read the value */
    if( (gMmwMssMCB.rfMonRes.monStat & (0x1U << M_RL_MON_TX3A_BB)) & (gMmwMssMCB.rfMonEnbl & (0x1U << M_RL_MON_TX3A_BB)))
    {
        if(gMmwMssMCB.mmWaveCfg.calibCfg.txBackoffSel <= TX_BACKOFF_BALL_BREAK_MON_MAX)
        {   
            gMmwMssMCB.rfMonRes.txPowerBB[6] = MMWaveMon_getTXnPowBB(6);
            gMmwMssMCB.rfMonRes.txPowerBBretlossMismatch[6]= gMmwMssMCB.rfMonRes.txPowerBB[6].txReturnLoss - rfMonResFactCal.txBBReturnLoss[6];
            /* Comparing results with Spec Values and setting Result Status bits */ 
            if((gMmwMssMCB.rfMonRes.txPowerBB[6].txReturnLoss <= gMmwMssMCB.SpecVal.TxBBRetLossSpec
                || gMmwMssMCB.rfMonRes.txPowerBBretlossMismatch[6] <= gMmwMssMCB.SpecVal.TxBBRetLossVarSpec ) && gMmwMssMCB.rfMonRes.txPowerBB[6].txIncPow >= gMmwMssMCB.SpecVal.TxPowSpecMin[6])
            {
                gMmwMssMCB.rfMonRes.status_txPowerBB=(gMmwMssMCB.rfMonRes.status_txPowerBB | 0x40);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txPowerBB=(gMmwMssMCB.rfMonRes.status_txPowerBB & 0xBF);
            }
        }
    }

    /* If TX0 Loop Back monitor is enabled and monitor is passed, read the value */
    if( (gMmwMssMCB.rfMonRes.monStat & (0x1 << M_RL_MON_TX0A_RX_LB)) & (gMmwMssMCB.rfMonEnbl & (0x1 << M_RL_MON_TX0A_RX_LB)))
    {   
            gMmwMssMCB.rfMonRes.txRxLb[0] = MMWaveMon_getTxRxLbres(0);
            /*Setting values for TX0 Loop Back monitor which depend upon on factory cal data*/
            gMmwMssMCB.rfMonRes.rxlbGaintx0MismatchVar[0]= gMmwMssMCB.rfMonRes.txRxLb[0].lbRxGainMismatch[2]- rfMonResFactCal.tx0LbRxGainMismatch[2];
            gMmwMssMCB.rfMonRes.rxlbGaintx0MismatchVar[1]= gMmwMssMCB.rfMonRes.txRxLb[0].lbRxGainMismatch[3]- rfMonResFactCal.tx0LbRxGainMismatch[3];
            gMmwMssMCB.rfMonRes.rxlbGaintx0MismatchVar[2]= gMmwMssMCB.rfMonRes.txRxLb[0].lbRxGainMismatch[6]- rfMonResFactCal.tx0LbRxGainMismatch[6];
            
            gMmwMssMCB.rfMonRes.rxlbPhasetx0MismatchVar[0]= MMWaveMon_lbPhaseError(gMmwMssMCB.rfMonRes.txRxLb[0].lbRxPhaseMismatch[2]- rfMonResFactCal.tx0LbRxPhaseMismatch[2]);
            gMmwMssMCB.rfMonRes.rxlbPhasetx0MismatchVar[1]= MMWaveMon_lbPhaseError(gMmwMssMCB.rfMonRes.txRxLb[0].lbRxPhaseMismatch[3]- rfMonResFactCal.tx0LbRxPhaseMismatch[3]);
            gMmwMssMCB.rfMonRes.rxlbPhasetx0MismatchVar[2]= MMWaveMon_lbPhaseError(gMmwMssMCB.rfMonRes.txRxLb[0].lbRxPhaseMismatch[6]- rfMonResFactCal.tx0LbRxPhaseMismatch[6]);

            /* Comparing results with Spec Values and setting Result Status bits */    
            if(gMmwMssMCB.rfMonRes.txRxLb[0].lbNoisedbm[0] <= gMmwMssMCB.SpecVal.lbNoiseSpecMax )
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] | 0x00000001);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] & 0xFFFFFFFE);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[0].lbNoisedbm[3] <= gMmwMssMCB.SpecVal.lbNoiseSpecMax )
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] | 0x00000002);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] & 0xFFFFFFFD);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[0].lbNoisedbm[4] <= gMmwMssMCB.SpecVal.lbNoiseSpecMax )
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] | 0x00000004);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] & 0xFFFFFFFB);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[0].lbNoisedbm[7] <= gMmwMssMCB.SpecVal.lbNoiseSpecMax )
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] | 0x00000008);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] & 0xFFFFFFF7);
            }

            if (gMmwMssMCB.mmWaveCfg.calibCfg.txBackoffSel <= TX_BACKOFF_TX_BPM_MON_MAX)
            {
                if(gMmwMssMCB.rfMonRes.txRxLb[0].lbBpmNoisedbm[0] <= gMmwMssMCB.SpecVal.lbBPMNoiseSpecMax)
                {
                    gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] | 0x00000010);
                }
                else
                {
                    gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] & 0xFFFFFFEF);
                }

                if(gMmwMssMCB.rfMonRes.txRxLb[0].lbBpmNoisedbm[3] <= gMmwMssMCB.SpecVal.lbBPMNoiseSpecMax)
                {
                    gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] | 0x00000020);
                }
                else
                {
                    gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] & 0xFFFFFFDF);
                }

                if(gMmwMssMCB.rfMonRes.txRxLb[0].lbBpmNoisedbm[4] <= gMmwMssMCB.SpecVal.lbBPMNoiseSpecMax)
                {
                    gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] | 0x00000040);
                }
                else
                {
                    gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] & 0xFFFFFFBF);
                }

                if(gMmwMssMCB.rfMonRes.txRxLb[0].lbBpmNoisedbm[7] <= gMmwMssMCB.SpecVal.lbBPMNoiseSpecMax)
                {
                    gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] | 0x00000080);
                }
                else
                {
                    gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] & 0xFFFFFF7F);
                }
                if(gMmwMssMCB.rfMonRes.txRxLb[0].lbBPMGainError[0] >= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMin &&
                gMmwMssMCB.rfMonRes.txRxLb[0].lbBPMGainError[0] <= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMax)
                {
                    gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] | 0x00000100);
                }
                else
                {
                    gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] & 0xFFFFFEFF);
                }

                if(gMmwMssMCB.rfMonRes.txRxLb[0].lbBPMGainError[3] >= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMin &&
                gMmwMssMCB.rfMonRes.txRxLb[0].lbBPMGainError[3] <= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMax)
                {
                    gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] | 0x00000200);
                }
                else
                {
                    gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] & 0xFFFFFDFF);
                }

                if(gMmwMssMCB.rfMonRes.txRxLb[0].lbBPMGainError[4] >= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMin &&
                gMmwMssMCB.rfMonRes.txRxLb[0].lbBPMGainError[4] <= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMax)
                {
                    gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] | 0x00000400);
                }
                else
                {
                    gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] & 0xFFFFFBFF);
                }

                if(gMmwMssMCB.rfMonRes.txRxLb[0].lbBPMGainError[7] >= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMin &&
                gMmwMssMCB.rfMonRes.txRxLb[0].lbBPMGainError[7] <= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMax)
                {
                    gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] | 0x00000800);
                }
                else
                {
                    gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] & 0xFFFFF7FF);
                }

                if(gMmwMssMCB.rfMonRes.txRxLb[0].lbBPMPhaseError[0] >= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMin &&
                gMmwMssMCB.rfMonRes.txRxLb[0].lbBPMPhaseError[0] <= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMax)
                {
                    gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] | 0x00001000);
                }
                else
                {
                    gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] & 0xFFFFEFFF);
                }

                if(gMmwMssMCB.rfMonRes.txRxLb[0].lbBPMPhaseError[3] >= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMin &&
                gMmwMssMCB.rfMonRes.txRxLb[0].lbBPMPhaseError[3] <= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMax)
                {
                    gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] | 0x00002000);
                }
                else
                {
                    gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] & 0xFFFFDFFF);
                }

                if(gMmwMssMCB.rfMonRes.txRxLb[0].lbBPMPhaseError[4] >= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMin &&
                gMmwMssMCB.rfMonRes.txRxLb[0].lbBPMPhaseError[4] <= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMax)
                {
                    gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] | 0x00004000);
                }
                else
                {
                    gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] & 0xFFFFBFFF);
                }

                if(gMmwMssMCB.rfMonRes.txRxLb[0].lbBPMPhaseError[7] >= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMin &&
                gMmwMssMCB.rfMonRes.txRxLb[0].lbBPMPhaseError[7] <= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMax)
                {
                    gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] | 0x00008000);
                }
                else
                {
                    gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] & 0xFFFF7FFF);
                }
            }

            if(gMmwMssMCB.rfMonRes.rxlbGaintx0MismatchVar[0] >= gMmwMssMCB.SpecVal.RxlbGainMisVarSpecMin &&
            gMmwMssMCB.rfMonRes.rxlbGaintx0MismatchVar[0] <= gMmwMssMCB.SpecVal.RxlbGainMisVarSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] | 0x00010000);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] & 0xFFFEFFFF);
            }

            if(gMmwMssMCB.rfMonRes.rxlbGaintx0MismatchVar[1] >= gMmwMssMCB.SpecVal.RxlbGainMisVarSpecMin &&
            gMmwMssMCB.rfMonRes.rxlbGaintx0MismatchVar[1] <= gMmwMssMCB.SpecVal.RxlbGainMisVarSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] | 0x00020000);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] & 0xFFFDFFFF);
            }

            if(gMmwMssMCB.rfMonRes.rxlbGaintx0MismatchVar[2] >= gMmwMssMCB.SpecVal.RxlbGainMisVarSpecMin &&
            gMmwMssMCB.rfMonRes.rxlbGaintx0MismatchVar[2] <= gMmwMssMCB.SpecVal.RxlbGainMisVarSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] | 0x00040000);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] & 0xFFFBFFFF);
            }

            if(gMmwMssMCB.rfMonRes.rxlbPhasetx0MismatchVar[0] >= gMmwMssMCB.SpecVal.RxlbPhaseMisVarSpecMin &&
            gMmwMssMCB.rfMonRes.rxlbPhasetx0MismatchVar[0] <= gMmwMssMCB.SpecVal.RxlbPhaseMisVarSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] | 0x00100000);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] & 0xFFEFFFFF);
            }

            if(gMmwMssMCB.rfMonRes.rxlbPhasetx0MismatchVar[1] >= gMmwMssMCB.SpecVal.RxlbPhaseMisVarSpecMin &&
            gMmwMssMCB.rfMonRes.rxlbPhasetx0MismatchVar[1] <= gMmwMssMCB.SpecVal.RxlbPhaseMisVarSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] | 0x00200000);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] & 0xFFDFFFFF);
            }

            if(gMmwMssMCB.rfMonRes.rxlbPhasetx0MismatchVar[2] >= gMmwMssMCB.SpecVal.RxlbPhaseMisVarSpecMin &&
            gMmwMssMCB.rfMonRes.rxlbPhasetx0MismatchVar[2] <= gMmwMssMCB.SpecVal.RxlbPhaseMisVarSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] | 0x00400000);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[0]=(gMmwMssMCB.rfMonRes.status_txRxLb[0] & 0xFFBFFFFF);
            }

    }
    /* If TX2 Loop Back monitor is enabled and monitor is passed, read the value */
    if( (gMmwMssMCB.rfMonRes.monStat & (0x1 << M_RL_MON_TX1A_RX_LB)) & (gMmwMssMCB.rfMonEnbl & (0x1 << M_RL_MON_TX1A_RX_LB)))
    {   
        gMmwMssMCB.rfMonRes.txRxLb[2] = MMWaveMon_getTxRxLbres(2);

        /* Comparing results with Spec Values and setting Result Status bits */    
        if(gMmwMssMCB.rfMonRes.txRxLb[2].lbNoisedbm[0] <= gMmwMssMCB.SpecVal.lbNoiseSpecMax )
        {
            gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] | 0x00000001);
        }
        else
        {
            gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] & 0xFFFFFFFE);
        }

        if(gMmwMssMCB.rfMonRes.txRxLb[2].lbNoisedbm[3] <= gMmwMssMCB.SpecVal.lbNoiseSpecMax )
        {
            gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] | 0x00000002);
        }
        else
        {
            gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] & 0xFFFFFFFD);
        }

        if(gMmwMssMCB.rfMonRes.txRxLb[2].lbNoisedbm[4] <= gMmwMssMCB.SpecVal.lbNoiseSpecMax )
        {
            gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] | 0x00000004);
        }
        else
        {
            gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] & 0xFFFFFFFB);
        }

        if(gMmwMssMCB.rfMonRes.txRxLb[2].lbNoisedbm[7] <= gMmwMssMCB.SpecVal.lbNoiseSpecMax )
        {
            gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] | 0x00000008);
        }
        else
        {
            gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] & 0xFFFFFFF7);
        }

        if (gMmwMssMCB.mmWaveCfg.calibCfg.txBackoffSel <= TX_BACKOFF_TX_BPM_MON_MAX)
        {
            if(gMmwMssMCB.rfMonRes.txRxLb[2].lbBpmNoisedbm[0] <= gMmwMssMCB.SpecVal.lbBPMNoiseSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] | 0x00000010);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] & 0xFFFFFFEF);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[2].lbBpmNoisedbm[3] <= gMmwMssMCB.SpecVal.lbBPMNoiseSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] | 0x00000020);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] & 0xFFFFFFDF);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[2].lbBpmNoisedbm[4] <= gMmwMssMCB.SpecVal.lbBPMNoiseSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] | 0x00000040);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] & 0xFFFFFFBF);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[2].lbBpmNoisedbm[7] <= gMmwMssMCB.SpecVal.lbBPMNoiseSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] | 0x00000080);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] & 0xFFFFFF7F);
            }
            if(gMmwMssMCB.rfMonRes.txRxLb[2].lbBPMGainError[0] >= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMin &&
            gMmwMssMCB.rfMonRes.txRxLb[2].lbBPMGainError[0] <= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] | 0x00000100);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] & 0xFFFFFEFF);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[2].lbBPMGainError[3] >= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMin &&
            gMmwMssMCB.rfMonRes.txRxLb[2].lbBPMGainError[3] <= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] | 0x00000200);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] & 0xFFFFFDFF);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[2].lbBPMGainError[4] >= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMin &&
            gMmwMssMCB.rfMonRes.txRxLb[2].lbBPMGainError[4] <= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] | 0x00000400);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] & 0xFFFFFBFF);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[2].lbBPMGainError[7] >= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMin &&
            gMmwMssMCB.rfMonRes.txRxLb[2].lbBPMGainError[7] <= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] | 0x00000800);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] & 0xFFFFF7FF);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[2].lbBPMPhaseError[0] >= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMin &&
            gMmwMssMCB.rfMonRes.txRxLb[2].lbBPMPhaseError[0] <= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] | 0x00001000);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] & 0xFFFFEFFF);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[2].lbBPMPhaseError[3] >= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMin &&
            gMmwMssMCB.rfMonRes.txRxLb[2].lbBPMPhaseError[3] <= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] | 0x00002000);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] & 0xFFFFDFFF);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[2].lbBPMPhaseError[4] >= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMin &&
            gMmwMssMCB.rfMonRes.txRxLb[2].lbBPMPhaseError[4] <= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] | 0x00004000);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] & 0xFFFFBFFF);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[2].lbBPMPhaseError[7] >= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMin &&
            gMmwMssMCB.rfMonRes.txRxLb[2].lbBPMPhaseError[7] <= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] | 0x00008000);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[1]=(gMmwMssMCB.rfMonRes.status_txRxLb[1] & 0xFFFF7FFF);
            }
        }

    }
    /* If TX4 Loop Back monitor is enabled and monitor is passed, read the value */
    if( (gMmwMssMCB.rfMonRes.monStat & (0x1 << M_RL_MON_TX2A_RX_LB)) & (gMmwMssMCB.rfMonEnbl & (0x1 << M_RL_MON_TX2A_RX_LB)))
    {   
        gMmwMssMCB.rfMonRes.txRxLb[4] = MMWaveMon_getTxRxLbres(4);

        /* Comparing results with Spec Values and setting Result Status bits */    
        if(gMmwMssMCB.rfMonRes.txRxLb[4].lbNoisedbm[0] <= gMmwMssMCB.SpecVal.lbNoiseSpecMax )
        {
            gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] | 0x00000001);
        }
        else
        {
            gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] & 0xFFFFFFFE);
        }

        if(gMmwMssMCB.rfMonRes.txRxLb[4].lbNoisedbm[3] <= gMmwMssMCB.SpecVal.lbNoiseSpecMax )
        {
            gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] | 0x00000002);
        }
        else
        {
            gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] & 0xFFFFFFFD);
        }

        if(gMmwMssMCB.rfMonRes.txRxLb[4].lbNoisedbm[4] <= gMmwMssMCB.SpecVal.lbNoiseSpecMax )
        {
            gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] | 0x00000004);
        }
        else
        {
            gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] & 0xFFFFFFFB);
        }

        if(gMmwMssMCB.rfMonRes.txRxLb[4].lbNoisedbm[7] <= gMmwMssMCB.SpecVal.lbNoiseSpecMax )
        {
            gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] | 0x00000008);
        }
        else
        {
            gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] & 0xFFFFFFF7);
        }

        if(gMmwMssMCB.mmWaveCfg.calibCfg.txBackoffSel <= TX_BACKOFF_TX_BPM_MON_MAX)
        {
            if(gMmwMssMCB.rfMonRes.txRxLb[4].lbBpmNoisedbm[0] <= gMmwMssMCB.SpecVal.lbBPMNoiseSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] | 0x00000010);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] & 0xFFFFFFEF);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[4].lbBpmNoisedbm[3] <= gMmwMssMCB.SpecVal.lbBPMNoiseSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] | 0x00000020);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] & 0xFFFFFFDF);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[4].lbBpmNoisedbm[4] <= gMmwMssMCB.SpecVal.lbBPMNoiseSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] | 0x00000040);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] & 0xFFFFFFBF);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[4].lbBpmNoisedbm[7] <= gMmwMssMCB.SpecVal.lbBPMNoiseSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] | 0x00000080);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] & 0xFFFFFF7F);
            }
            if(gMmwMssMCB.rfMonRes.txRxLb[4].lbBPMGainError[0] >= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMin &&
            gMmwMssMCB.rfMonRes.txRxLb[4].lbBPMGainError[0] <= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] | 0x00000100);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] & 0xFFFFFEFF);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[4].lbBPMGainError[3] >= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMin &&
            gMmwMssMCB.rfMonRes.txRxLb[4].lbBPMGainError[3] <= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] | 0x00000200);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] & 0xFFFFFDFF);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[4].lbBPMGainError[4] >= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMin &&
            gMmwMssMCB.rfMonRes.txRxLb[4].lbBPMGainError[4] <= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] | 0x00000400);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] & 0xFFFFFBFF);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[4].lbBPMGainError[7] >= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMin &&
            gMmwMssMCB.rfMonRes.txRxLb[4].lbBPMGainError[7] <= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] | 0x00000800);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] & 0xFFFFF7FF);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[4].lbBPMPhaseError[0] >= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMin &&
            gMmwMssMCB.rfMonRes.txRxLb[4].lbBPMPhaseError[0] <= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] | 0x00001000);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] & 0xFFFFEFFF);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[4].lbBPMPhaseError[3] >= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMin &&
            gMmwMssMCB.rfMonRes.txRxLb[4].lbBPMPhaseError[3] <= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] | 0x00002000);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] & 0xFFFFDFFF);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[4].lbBPMPhaseError[4] >= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMin &&
            gMmwMssMCB.rfMonRes.txRxLb[4].lbBPMPhaseError[4] <= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] | 0x00004000);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] & 0xFFFFBFFF);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[4].lbBPMPhaseError[7] >= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMin &&
            gMmwMssMCB.rfMonRes.txRxLb[4].lbBPMPhaseError[7] <= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] | 0x00008000);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[2]=(gMmwMssMCB.rfMonRes.status_txRxLb[2] & 0xFFFF7FFF);
            }
        }
    
    }
    /* If TX6 Loop Back monitor is enabled and monitor is passed, read the value */
    if( (gMmwMssMCB.rfMonRes.monStat & (0x1 << M_RL_MON_TX3A_RX_LB)) & (gMmwMssMCB.rfMonEnbl & (0x1 << M_RL_MON_TX3A_RX_LB)))
    {   
        gMmwMssMCB.rfMonRes.txRxLb[6] = MMWaveMon_getTxRxLbres(6);

        /* Comparing results with Spec Values and setting Result Status bits */    
        if(gMmwMssMCB.rfMonRes.txRxLb[6].lbNoisedbm[0] <= gMmwMssMCB.SpecVal.lbNoiseSpecMax )
        {
            gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] | 0x00000001);
        }
        else
        {
            gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] & 0xFFFFFFFE);
        }

        if(gMmwMssMCB.rfMonRes.txRxLb[6].lbNoisedbm[3] <= gMmwMssMCB.SpecVal.lbNoiseSpecMax )
        {
            gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] | 0x00000002);
        }
        else
        {
            gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] & 0xFFFFFFFD);
        }

        if(gMmwMssMCB.rfMonRes.txRxLb[6].lbNoisedbm[4] <= gMmwMssMCB.SpecVal.lbNoiseSpecMax )
        {
            gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] | 0x00000004);
        }
        else
        {
            gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] & 0xFFFFFFFB);
        }

        if(gMmwMssMCB.rfMonRes.txRxLb[6].lbNoisedbm[7] <= gMmwMssMCB.SpecVal.lbNoiseSpecMax )
        {
            gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] | 0x00000008);
        }
        else
        {
            gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] & 0xFFFFFFF7);
        }
        if(gMmwMssMCB.mmWaveCfg.calibCfg.txBackoffSel <= TX_BACKOFF_TX_BPM_MON_MAX)
        {
            if(gMmwMssMCB.rfMonRes.txRxLb[6].lbBpmNoisedbm[0] <= gMmwMssMCB.SpecVal.lbBPMNoiseSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] | 0x00000010);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] & 0xFFFFFFEF);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[6].lbBpmNoisedbm[3] <= gMmwMssMCB.SpecVal.lbBPMNoiseSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] | 0x00000020);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] & 0xFFFFFFDF);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[6].lbBpmNoisedbm[4] <= gMmwMssMCB.SpecVal.lbBPMNoiseSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] | 0x00000040);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] & 0xFFFFFFBF);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[6].lbBpmNoisedbm[7] <= gMmwMssMCB.SpecVal.lbBPMNoiseSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] | 0x00000080);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] & 0xFFFFFF7F);
            }
            if(gMmwMssMCB.rfMonRes.txRxLb[6].lbBPMGainError[0] >= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMin &&
            gMmwMssMCB.rfMonRes.txRxLb[6].lbBPMGainError[0] <= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] | 0x00000100);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] & 0xFFFFFEFF);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[6].lbBPMGainError[3] >= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMin &&
            gMmwMssMCB.rfMonRes.txRxLb[6].lbBPMGainError[3] <= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] | 0x00000200);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] & 0xFFFFFDFF);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[6].lbBPMGainError[4] >= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMin &&
            gMmwMssMCB.rfMonRes.txRxLb[6].lbBPMGainError[4] <= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] | 0x00000400);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] & 0xFFFFFBFF);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[6].lbBPMGainError[7] >= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMin &&
            gMmwMssMCB.rfMonRes.txRxLb[6].lbBPMGainError[7] <= gMmwMssMCB.SpecVal.lbBPMGainErrSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] | 0x00000800);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] & 0xFFFFF7FF);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[6].lbBPMPhaseError[0] >= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMin &&
            gMmwMssMCB.rfMonRes.txRxLb[6].lbBPMPhaseError[0] <= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] | 0x00001000);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] & 0xFFFFEFFF);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[6].lbBPMPhaseError[3] >= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMin &&
            gMmwMssMCB.rfMonRes.txRxLb[6].lbBPMPhaseError[3] <= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] | 0x00002000);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] & 0xFFFFDFFF);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[6].lbBPMPhaseError[4] >= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMin &&
            gMmwMssMCB.rfMonRes.txRxLb[6].lbBPMPhaseError[4] <= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] | 0x00004000);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] & 0xFFFFBFFF);
            }

            if(gMmwMssMCB.rfMonRes.txRxLb[6].lbBPMPhaseError[7] >= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMin &&
            gMmwMssMCB.rfMonRes.txRxLb[6].lbBPMPhaseError[7] <= gMmwMssMCB.SpecVal.lbBPMPhaseErrSpecMax)
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] | 0x00008000);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_txRxLb[3]=(gMmwMssMCB.rfMonRes.status_txRxLb[3] & 0xFFFF7FFF);
            }
        }

    }

    /* If TX0 DC Signal monitor is enabled and monitor is passed, read the value */
    if( (gMmwMssMCB.rfMonRes.monStat & (0x1U << MON_TX0_DC_MON)) & (gMmwMssMCB.rfMonEnbl & (0x1U << MON_TX0_DC_MON)))
    {
        gMmwMssMCB.rfMonRes.txDcSig[0] = MMWaveMon_getTXnDcSig(0);
        /* Comparing results with Spec Values and setting Result Status bits */ 
        if(gMmwMssMCB.rfMonRes.txDcSig[0] == gMmwMssMCB.SpecVal.TxDCSigResSpec)
        {
            gMmwMssMCB.rfMonRes.status_txDcSig=(gMmwMssMCB.rfMonRes.status_txDcSig | 0x1);
        }
        else
        {
            gMmwMssMCB.rfMonRes.status_txDcSig=(gMmwMssMCB.rfMonRes.status_txDcSig & 0xFE);
        }
    }
    /* If TX2 DC Signal monitor is enabled and monitor is passed, read the value */
    if( (gMmwMssMCB.rfMonRes.monStat & (0x1U << MON_TX1_DC_MON)) & (gMmwMssMCB.rfMonEnbl & (0x1U << MON_TX1_DC_MON)))
    {
        gMmwMssMCB.rfMonRes.txDcSig[1] = MMWaveMon_getTXnDcSig(1);
        /* Comparing results with Spec Values and setting Result Status bits */ 
        if(gMmwMssMCB.rfMonRes.txDcSig[1] == gMmwMssMCB.SpecVal.TxDCSigResSpec)
        {
            gMmwMssMCB.rfMonRes.status_txDcSig=(gMmwMssMCB.rfMonRes.status_txDcSig | 0x2);
        }
        else
        {
            gMmwMssMCB.rfMonRes.status_txDcSig=(gMmwMssMCB.rfMonRes.status_txDcSig & 0xFD);
        }

    }
    /* If TX4 DC Signal monitor is enabled and monitor is passed, read the value */
    if( (gMmwMssMCB.rfMonRes.monStat & (0x1U << MON_TX2_DC_MON)) & (gMmwMssMCB.rfMonEnbl & (0x1U << MON_TX2_DC_MON)))
    {
        gMmwMssMCB.rfMonRes.txDcSig[2] = MMWaveMon_getTXnDcSig(2);
        /* Comparing results with Spec Values and setting Result Status bits */ 
        if(gMmwMssMCB.rfMonRes.txDcSig[2] == gMmwMssMCB.SpecVal.TxDCSigResSpec)
        {
            gMmwMssMCB.rfMonRes.status_txDcSig=(gMmwMssMCB.rfMonRes.status_txDcSig | 0x4);
        }
        else
        {
            gMmwMssMCB.rfMonRes.status_txDcSig=(gMmwMssMCB.rfMonRes.status_txDcSig & 0xFB);
        }
    }
    /* If TX6 DC Signal monitor is enabled and monitor is passed, read the value */
    if( (gMmwMssMCB.rfMonRes.monStat & (0x1U << MON_TX3_DC_MON)) & (gMmwMssMCB.rfMonEnbl & (0x1U << MON_TX3_DC_MON)))
    {
        gMmwMssMCB.rfMonRes.txDcSig[3] = MMWaveMon_getTXnDcSig(3);
        /* Comparing results with Spec Values and setting Result Status bits */ 
        if(gMmwMssMCB.rfMonRes.txDcSig[3] == gMmwMssMCB.SpecVal.TxDCSigResSpec)
        {
            gMmwMssMCB.rfMonRes.status_txDcSig=(gMmwMssMCB.rfMonRes.status_txDcSig | 0x8);
        }
        else
        {
            gMmwMssMCB.rfMonRes.status_txDcSig=(gMmwMssMCB.rfMonRes.status_txDcSig & 0xF7);
        }
    }
    
    /* If RX HPF DC Signal monitor is enabled and monitor is passed, read the value */
    if( (gMmwMssMCB.rfMonRes.monStat & ((uint64_t)0x1U << MON_RX_HPF_INTRNAL_DC_SIG)) & (gMmwMssMCB.rfMonEnbl & ((uint64_t)0x1U << MON_RX_HPF_INTRNAL_DC_SIG)))
    {
        MMWaveMon_getRxHpfDcSig(&gMmwMssMCB.rfMonRes.rxHpfDcsigres);
        /* Comparing results with Spec Values and setting Result Status bits */ 
            if(gMmwMssMCB.rfMonRes.rxHpfDcsigres.RxHpfCutoffAtten[0] >= gMmwMssMCB.SpecVal.RxHPFAttnSpecMin 
                && gMmwMssMCB.rfMonRes.rxHpfDcsigres.RxHpfCutoffAtten[0] <= gMmwMssMCB.SpecVal.RxHPFAttnSpecMax )
            {
                gMmwMssMCB.rfMonRes.status_rxHpfDCsigres=(gMmwMssMCB.rfMonRes.status_rxHpfDCsigres | 0x1);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_rxHpfDCsigres=(gMmwMssMCB.rfMonRes.status_rxHpfDCsigres & 0xFE);
            }

            if(gMmwMssMCB.rfMonRes.rxHpfDcsigres.RxHpfCutoffAtten[1] >= gMmwMssMCB.SpecVal.RxHPFAttnSpecMin 
                && gMmwMssMCB.rfMonRes.rxHpfDcsigres.RxHpfCutoffAtten[1] <= gMmwMssMCB.SpecVal.RxHPFAttnSpecMax )
            {
                gMmwMssMCB.rfMonRes.status_rxHpfDCsigres=(gMmwMssMCB.rfMonRes.status_rxHpfDCsigres | 0x2);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_rxHpfDCsigres=(gMmwMssMCB.rfMonRes.status_rxHpfDCsigres & 0xFD);
            }

            if(gMmwMssMCB.rfMonRes.rxHpfDcsigres.RxHpfCutoffAtten[2] >= gMmwMssMCB.SpecVal.RxHPFAttnSpecMin 
                && gMmwMssMCB.rfMonRes.rxHpfDcsigres.RxHpfCutoffAtten[2] <= gMmwMssMCB.SpecVal.RxHPFAttnSpecMax )
            {
                gMmwMssMCB.rfMonRes.status_rxHpfDCsigres=(gMmwMssMCB.rfMonRes.status_rxHpfDCsigres | 0x4);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_rxHpfDCsigres=(gMmwMssMCB.rfMonRes.status_rxHpfDCsigres & 0xFB);
            }

            if(gMmwMssMCB.rfMonRes.rxHpfDcsigres.RxHpfCutoffAtten[3] >= gMmwMssMCB.SpecVal.RxHPFAttnSpecMin 
                && gMmwMssMCB.rfMonRes.rxHpfDcsigres.RxHpfCutoffAtten[3] <= gMmwMssMCB.SpecVal.RxHPFAttnSpecMax )
            {
                gMmwMssMCB.rfMonRes.status_rxHpfDCsigres=(gMmwMssMCB.rfMonRes.status_rxHpfDCsigres | 0x8);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_rxHpfDCsigres=(gMmwMssMCB.rfMonRes.status_rxHpfDCsigres & 0xF7);
            }

            if(gMmwMssMCB.rfMonRes.rxHpfDcsigres.RxHpfInbandPwrdB[0] >= gMmwMssMCB.SpecVal.rxHPFInBandPwrdBMin 
                && gMmwMssMCB.rfMonRes.rxHpfDcsigres.RxHpfInbandPwrdB[0] <= gMmwMssMCB.SpecVal.rxHPFInBandPwrdBMax )
            {
                gMmwMssMCB.rfMonRes.status_rxHpfDCsigres=(gMmwMssMCB.rfMonRes.status_rxHpfDCsigres | 0x10);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_rxHpfDCsigres=(gMmwMssMCB.rfMonRes.status_rxHpfDCsigres & 0xEF);
            }

            if(gMmwMssMCB.rfMonRes.rxHpfDcsigres.RxHpfInbandPwrdB[1] >= gMmwMssMCB.SpecVal.rxHPFInBandPwrdBMin 
                && gMmwMssMCB.rfMonRes.rxHpfDcsigres.RxHpfInbandPwrdB[1] <= gMmwMssMCB.SpecVal.rxHPFInBandPwrdBMax )
            {
                gMmwMssMCB.rfMonRes.status_rxHpfDCsigres=(gMmwMssMCB.rfMonRes.status_rxHpfDCsigres | 0x20);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_rxHpfDCsigres=(gMmwMssMCB.rfMonRes.status_rxHpfDCsigres & 0xDF);
            }

            if(gMmwMssMCB.rfMonRes.rxHpfDcsigres.RxHpfInbandPwrdB[2] >= gMmwMssMCB.SpecVal.rxHPFInBandPwrdBMin 
                && gMmwMssMCB.rfMonRes.rxHpfDcsigres.RxHpfInbandPwrdB[2] <= gMmwMssMCB.SpecVal.rxHPFInBandPwrdBMax )
            {
                gMmwMssMCB.rfMonRes.status_rxHpfDCsigres=(gMmwMssMCB.rfMonRes.status_rxHpfDCsigres | 0x40);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_rxHpfDCsigres=(gMmwMssMCB.rfMonRes.status_rxHpfDCsigres & 0xBF);
            }

            if(gMmwMssMCB.rfMonRes.rxHpfDcsigres.RxHpfInbandPwrdB[3] >= gMmwMssMCB.SpecVal.rxHPFInBandPwrdBMin 
                && gMmwMssMCB.rfMonRes.rxHpfDcsigres.RxHpfInbandPwrdB[3] <= gMmwMssMCB.SpecVal.rxHPFInBandPwrdBMax )
            {
                gMmwMssMCB.rfMonRes.status_rxHpfDCsigres=(gMmwMssMCB.rfMonRes.status_rxHpfDCsigres | 0x80);
            }
            else
            {
                gMmwMssMCB.rfMonRes.status_rxHpfDCsigres=(gMmwMssMCB.rfMonRes.status_rxHpfDCsigres & 0x7F);
            }
    }
    /* If PM CLK DC Signal is enabled and monitor is passed, read the value */
    if( (gMmwMssMCB.rfMonRes.monStat & ((uint64_t)0x1U << MON_PM_CLK_INTRNAL_DC_SIG)) & (gMmwMssMCB.rfMonEnbl & ((uint64_t)0x1U << MON_PM_CLK_INTRNAL_DC_SIG)))
    {
        MMWaveMon_getPmClkDcMonres(&gMmwMssMCB.rfMonRes.pmClkDcSigres);
        /* Comparing results with Spec Values and setting Result Status bits */ 
        if(gMmwMssMCB.rfMonRes.pmClkDcSigres.pmClkDcMonstat == gMmwMssMCB.SpecVal.PMClkDCSigStatSpec)
        {
            gMmwMssMCB.rfMonRes.status_pmClkDcSig=(gMmwMssMCB.rfMonRes.status_pmClkDcSig | 0x1);
        }
        else
        {
            gMmwMssMCB.rfMonRes.status_pmClkDcSig=(gMmwMssMCB.rfMonRes.status_pmClkDcSig & 0xFE);
        }
    }

    if ((gMmwMssMCB.rfMonRes.monStat & ((uint64_t)0x1U << MON_DFE_BIST_FFT_CHECK)) & (gMmwMssMCB.rfMonEnbl & ((uint64_t)0x1U << MON_DFE_BIST_FFT_CHECK)))
    {
        gMmwMssMCB.rfMonRes.status_dfeBistFFT = (gMmwMssMCB.rfMonRes.status_dfeBistFFT | 0x1);
    }
    else
    {
        gMmwMssMCB.rfMonRes.status_dfeBistFFT = (gMmwMssMCB.rfMonRes.status_dfeBistFFT | 0x0);
    }

    if ((gMmwMssMCB.rfMonRes.monStat & ((uint64_t)0x1U << MON_STATIC_REG_READBACK)) & (gMmwMssMCB.rfMonEnbl & ((uint64_t)0x1U << MON_STATIC_REG_READBACK)))
    {
        gMmwMssMCB.rfMonRes.status_staticRegRb = (gMmwMssMCB.rfMonRes.status_staticRegRb | 0x1);
    }
    else
    {
        gMmwMssMCB.rfMonRes.status_staticRegRb = (gMmwMssMCB.rfMonRes.status_staticRegRb | 0x0);
    }
    
    #if (PRINT_MON_RES == 1)
    /*Printing Result Status Bits*/
    if(gMmwMssMCB.apllFreqShiftEnable == FALSE)
    {
        /* If APLL frequecny shift is disabled print all the monitor results */
        CLI_write("PLL Monitor: %x \r\n",gMmwMssMCB.rfMonRes.status_pllvolres);
        CLI_write("Power monitor: %x \r\n",gMmwMssMCB.rfMonRes.status_txPower);
        CLI_write("DC Signal monitor: %x \r\n",gMmwMssMCB.rfMonRes.status_txDcSig);
        CLI_write("PM CLK DC Signal monitor: %x \r\n",gMmwMssMCB.rfMonRes.status_pmClkDcSig);
        CLI_write("RX HPF monitor: %x \r\n",gMmwMssMCB.rfMonRes.status_rxHpfDCsigres);
        CLI_write("TX Ball Break monitor: %x \r\n",gMmwMssMCB.rfMonRes.status_txPowerBB);
        CLI_write("TX Loopback monitor: %x, %x, %x, %x  \r\n",gMmwMssMCB.rfMonRes.status_txRxLb[0],gMmwMssMCB.rfMonRes.status_txRxLb[1],gMmwMssMCB.rfMonRes.status_txRxLb[2],gMmwMssMCB.rfMonRes.status_txRxLb[3]);
        CLI_write("DFE BIST FFT monitor: %x \r\n",gMmwMssMCB.rfMonRes.status_dfeBistFFT);
        CLI_write("Static Reg Readback monitor: %x \r\n",gMmwMssMCB.rfMonRes.status_staticRegRb);
    }
    else
    {
        if((gMmwMssMCB.rfMonRes.monStat & ((uint64_t)0x1U << M_RL_MON_PLL_CTRL_VOLT)) & (gMmwMssMCB.rfMonEnbl & ((uint64_t)0x1U << M_RL_MON_PLL_CTRL_VOLT)))
        {
            /* When APLL frequency shift and PLL Control Voltage monitoring are both active, display just the PLL Control Voltage monitor results since only PLL Control Voltage monitor is run at 396MHz APLL */
            CLI_write("PLL Monitor: %x \r\n",gMmwMssMCB.rfMonRes.status_pllvolres);
        }
        else
        {
            /* If APLL frequency shift is enabled, print all the monitor results which are run at 400MHz APLL */
            CLI_write("Power monitor: %x \r\n",gMmwMssMCB.rfMonRes.status_txPower);
            CLI_write("DC Signal monitor: %x \r\n",gMmwMssMCB.rfMonRes.status_txDcSig);
            CLI_write("PM CLK DC Signal monitor: %x \r\n",gMmwMssMCB.rfMonRes.status_pmClkDcSig);
            CLI_write("RX HPF monitor: %x \r\n",gMmwMssMCB.rfMonRes.status_rxHpfDCsigres);
            CLI_write("TX Ball Break monitor: %x \r\n",gMmwMssMCB.rfMonRes.status_txPowerBB);
            CLI_write("TX Loopback monitor: %x, %x, %x, %x  \r\n",gMmwMssMCB.rfMonRes.status_txRxLb[0],gMmwMssMCB.rfMonRes.status_txRxLb[1],gMmwMssMCB.rfMonRes.status_txRxLb[2],gMmwMssMCB.rfMonRes.status_txRxLb[3]);
            CLI_write("DFE BIST FFT monitor: %x \r\n",gMmwMssMCB.rfMonRes.status_dfeBistFFT);
            CLI_write("Static Reg Readback monitor: %x \r\n",gMmwMssMCB.rfMonRes.status_staticRegRb);
        }
    }
    #endif
}
#endif
