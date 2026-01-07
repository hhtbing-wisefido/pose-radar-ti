/*
 * Copyright (C) 2022-24 Texas Instruments Incorporated
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

#include <kernel/dpl/SemaphoreP.h>
#include <kernel/dpl/CacheP.h>
#include <kernel/dpl/ClockP.h>
#include <kernel/dpl/DebugP.h>
#include <kernel/dpl/HwiP.h>
#include <kernel/dpl/AddrTranslateP.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
/* mmwave SDK files */
#include <control/mmwave/mmwave.h>
#include <drivers/prcm.h>
#include <drivers/hw_include/cslr_soc.h>

#include <source/mmw_cli.h>
#include <source/mmwave_control/interrupts.h>
#include <source/mmwave_demo.h>

#include <ti_drivers_config.h>
#include <ti_drivers_open_close.h>
#include <ti_board_open_close.h>
#include <ti_board_config.h>

#define FRAME_REF_TIMER_CLOCK_MHZ  40
#define LOW_PWR_MODE_DISABLE (0)
#define LOW_PWR_MODE_ENABLE (1)

extern MmwDemo_MSS_MCB gMmwMssMCB;

HwiP_Object gHwiChirpAvailableHwiObject;
HwiP_Object gHwiFrameStartHwiObject;

#if (_DEBUG_ == 1)
/* In debug build, in order to debug target code (set breakpoints, step over...) below variable is set to 1.
 * It will prevent ISR mmwDemoFrameStartISR from forcing the code to stop */
volatile uint32_t gDebugTargetCode = 1;
#else
volatile uint32_t gDebugTargetCode = 0;
#endif

volatile uint32_t gMmwDemoChirpCnt = 0;
#if 0
/* For debugging purposes */
volatile uint32_t gMmwDemoChirpStartCnt = 0;
volatile uint32_t gMmwDemoBurstCnt = 0;
#endif

/* For debugging purposes */
#if 0
/**
 *  @b Description
 *  @n
 *      This is to register Burst Interrupt
 */
int32_t MmwDemo_registerBurstInterrupt(void)
{
    int32_t           regVal, retVal = 0;
    int32_t           status = SystemP_SUCCESS;
    HwiP_Params       hwiPrms;

    // Configure the interrupt for Burst End
    regVal = HW_RD_REG32(CSL_APP_CTRL_U_BASE + CSL_APP_CTRL_APPSS_IRQ_REQ_SEL);
    regVal = regVal | 0x1000;
    HW_WR_REG32((CSL_APP_CTRL_U_BASE + CSL_APP_CTRL_APPSS_IRQ_REQ_SEL), regVal);

    /* Register interrupt */
    HwiP_Params_init(&hwiPrms);
    hwiPrms.intNum      = CSL_APPSS_INTR_FECSS_CHIRPTIMER_AND_BURST_START_AND_BURST_END;
    hwiPrms.callback    = mmwDemoBurstISR;
    /* Use this to change the priority */
    //hwiPrms.priority    = 0;
    hwiPrms.args        = NULL;
    status              = HwiP_construct(&gHwiChirpAvailableHwiObject, &hwiPrms);

    if(SystemP_SUCCESS != status)
    {
        retVal = SystemP_FAILURE;
    }

    return retVal;
}

/**
 *  @b Description
 *  @n
 *      This is to register Chirpt Interrupt
 */
int32_t MmwDemo_registerChirpInterrupt(void)
{
    int32_t           retVal = 0;
    int32_t           status = SystemP_SUCCESS;
    HwiP_Params       hwiPrms;

    /* Register interrupt */
    HwiP_Params_init(&hwiPrms);
    hwiPrms.intNum      = CSL_APPSS_INTR_FECSS_CHIRPTIMER_AND_CHIRP_START_AND_CHIRP_END;
    hwiPrms.callback    = mmwDemoChirpStartISR;
    /* Use this to change the priority */
    //hwiPrms.priority    = 0;
    hwiPrms.args        = NULL;
    status              = HwiP_construct(&gHwiChirpAvailableHwiObject, &hwiPrms);

    if(SystemP_SUCCESS != status)
    {
        retVal = SystemP_FAILURE;
    }

    return retVal;
}
#endif
/**
 *  @b Description
 *  @n
 *      This is to register Chirp Available Interrupt
 */
int32_t MmwDemo_registerChirpAvailableInterrupts(void)
{
    int32_t           retVal = 0;
    int32_t           status = SystemP_SUCCESS;
    HwiP_Params       hwiPrms;

    /* Register interrupt */
    HwiP_Params_init(&hwiPrms);
    hwiPrms.intNum      = CSL_APPSS_INTR_FECSS_CHIRP_AVAIL_IRQ_AND_ADC_VALID_START_AND_SYNC_IN;
    hwiPrms.callback    = mmwDemoChirpISR;
    /* Use this to change the priority */
    hwiPrms.priority    = 5;
    hwiPrms.args        = NULL;
    status              = HwiP_construct(&gHwiChirpAvailableHwiObject, &hwiPrms);

    if(SystemP_SUCCESS != status)
    {
        retVal = SystemP_FAILURE;
    }

    return retVal;
}

/**
 *  @b Description
 *  @n
 *      This is to register Frame Start Interrupt
 */
int32_t MmwDemo_registerFrameStartInterrupt(void)
{
    int32_t           retVal = 0;
    int32_t           status = SystemP_SUCCESS;
    HwiP_Params       hwiPrms;

    /* Register interrupt */
    HwiP_Params_init(&hwiPrms);
    hwiPrms.intNum      = CSL_APPSS_INTR_FECSS_FRAMETIMER_FRAME_START;
    hwiPrms.callback    = mmwDemoFrameStartISR;
    /* Use this to change the priority */
    //hwiPrms.priority    = 0;
    hwiPrms.args        = (void *) &gMmwMssMCB;
    status              = HwiP_construct(&gHwiFrameStartHwiObject, &hwiPrms);

    if(SystemP_SUCCESS != status)
    {
        retVal = SystemP_FAILURE;
    }

    return retVal;
}
/* For debugging purposes*/
#if 0
/**
*  @b Description
*  @n
*    Burst ISR
*/
void mmwDemoBurstISR(void *arg)
{
    HwiP_clearInt(CSL_APPSS_INTR_FECSS_CHIRPTIMER_AND_BURST_START_AND_BURST_END);
    gMmwDemoBurstCnt++;
}

/**
*  @b Description
*  @n
*    Chirp Start ISR
*/
void mmwDemoChirpStartISR(void *arg)
{
    HwiP_clearInt(CSL_APPSS_INTR_FECSS_CHIRPTIMER_AND_CHIRP_START_AND_CHIRP_END);
    gMmwDemoChirpStartCnt++;
}
#endif
/**
*  @b Description
*  @n
*    Chirp ISR
*/
static void mmwDemoChirpISR(void *arg)
{
    uint32_t randDelay = 0;
    CSL_dss_ctrlRegs *ptrDssCtrlRegs = (CSL_dss_ctrlRegs*)CSL_DSS_CTRL_U_BASE;
    HwiP_clearInt(CSL_APPSS_INTR_FECSS_CHIRP_AVAIL_IRQ_AND_ADC_VALID_START_AND_SYNC_IN);
    if(gMmwMssMCB.adcDataDithEnable)
    {
        /* configure the variable amount of delay */
        randDelay = rand() % ADC_PER_CHIRP_DITHER_RANGE;
        ptrDssCtrlRegs->ADCBUFCFG1_EXTD = randDelay + MIN_ADC_PER_CHIRP_DELAY;
    }
    gMmwDemoChirpCnt++;
    /* If ADC logging via LVDS is enabled, trigger the edma transfer from adcbuf to cbuff, so that LVDS stream will start upon start of chirp */
    if(gMmwMssMCB.adcLogging.enable == 1)
    {
        MmwDemo_configLVDSDataTrigger();
    }
}

/**
*  @b Description
*  @n
*    Frame start ISR
*/
static void mmwDemoFrameStartISR(void *arg)
{
    uint64_t l_demoStartTimeUs;
    unsigned long long ll_startTimeSlowClk;
    MmwDemo_MSS_MCB *mmwMssMCB = (MmwDemo_MSS_MCB *) arg;

    HwiP_clearInt(CSL_APPSS_INTR_FECSS_FRAMETIMER_FRAME_START);

    /* Capture the frame start time using FreeRTOS timer */
    l_demoStartTimeUs = ClockP_getTimeUsec();
    /* Capture the frame start time using the Slow Clock. This is needed when Low power mode is enabled */
    ll_startTimeSlowClk = PRCMSlowClkCtrGet();
    if(gMmwMssMCB.lowPowerMode == LOW_PWR_MODE_DISABLE)
    {
        /* Use FreeRTOS clock (Driven by RTI) when Low power mode is disabled as this is very accurate */
        mmwMssMCB->stats.measuredframePeriodUs = (l_demoStartTimeUs - mmwMssMCB->stats.frameStartTimeStampUs);
    }
    else
    {
        /* FreeRTOS timer is shutdown during Low Power mode. Hence Slow Clock has to be used when Low power mode is enabled */
        mmwMssMCB->stats.measuredframePeriodUs = round((ll_startTimeSlowClk - mmwMssMCB->stats.frameStartTimeStampSlowClk) * M_TICKS_TO_USEC_SLOWCLK);
    }
    
    mmwMssMCB->stats.frameStartTimeStampUs = l_demoStartTimeUs;

    mmwMssMCB->stats.frameStartTimeStampSlowClk = PRCMSlowClkCtrGet();

    /* By default R5 runs at 200Mhz. To lower power consumption, the R5 clock is set to 40 MHz during chirping by selecting the OSC Clock as the source with a divider of 1. 
     * After chirping, the R5 clock is restored to 200 MHz by switching the source to Fast Clock 1 with a divider of 1. 
     * Since ADC logging requires the R5 to run at 200 MHz, this is done only when ADC logging is turned off.
     */
    if (gMmwMssMCB.adcLogging.enable == 0)
    {
        SOC_rcmSetR5Clock(SOC_RCM_XTAL_CLK_40MHZ,SOC_RCM_XTAL_CLK_40MHZ, SOC_RcmR5ClockSource_OSC_CLK);
    }

    if (gDebugTargetCode == 0)
    {
        DebugP_assert(mmwMssMCB->interSubFrameProcToken == 0);
    }

    if(mmwMssMCB->interSubFrameProcToken > 0)
    {
        mmwMssMCB->interSubFrameProcOverflowCntr++;
    }

    mmwMssMCB->interSubFrameProcToken++;

    mmwMssMCB->stats.frameStartIntCounter++;
}