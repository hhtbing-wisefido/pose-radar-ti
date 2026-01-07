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

#include <source/mmw_cli.h>
#include <source/mmwave_demo.h>
#include <source/dpc/dpc.h>

#include <ti_drivers_config.h>
#include <ti_drivers_open_close.h>
#include <ti_board_open_close.h>
#include <ti_board_config.h>

#define MMWDEMO_RFPARSER_SPEED_OF_LIGHT_IN_METERS_PER_SEC (3e8)
extern MmwDemo_MSS_MCB gMmwMssMCB;

/**
 *  @b Description
 *  @n
 *      Utility function to do a parabolic/quadratic fit on 3 input points
 *      and return the coordinates of the peak. This is used to accurately estimate
 *      range bias.
 *
 *  @param[in]  x Pointer to array of 3 elements representing the x-coordinate
 *              of the points to fit
 *  @param[in]  y Pointer to array of 3 elements representing the y-coordinate
 *              of the points to fit
 *  @param[out] xv Pointer to output x-coordinate of the peak value
 *  @param[out] yv Pointer to output y-coordinate of the peak value
 *
 *  @retval   None
 *
 */
void rangeBiasMeasure_quadfit(float *x, float*y, float *xv, float *yv)
{
    float a, b, c, denom;
    float x0 = x[0];
    float x1 = x[1];
    float x2 = x[2];
    float y0 = y[0];
    float y1 = y[1];
    float y2 = y[2];

    denom = (x0 - x1)*(x0 - x2)*(x1 - x2);
    if (denom != 0.)
    {
        a = (x2 * (y1 - y0) + x1 * (y0 - y2) + x0 * (y2 - y1)) / denom;
        b = (x2*x2 * (y0 - y1) + x1*x1 * (y2 - y0) + x0*x0 * (y1 - y2)) / denom;
        c = (x1 * x2 * (x1 - x2) * y0 + x2 * x0 * (x2 - x0) * y1 + x0 * x1 * (x0 - x1) * y2) / denom;
    }
    else
    {
        *xv = x[1];
        *yv = y[1];
        return;
    }
    if (a != 0.)
    {
        *xv = -b/(2*a);
        *yv = c - b*b/(4*a);
    }
    else
    {
        *xv = x[1];
        *yv = y[1];
    }
}

/**
 *  @b Description
 *  @n
 *      The function initializes parameters for the measurement procedure for range bias and rx channel compensation
 * 
 *  @retval
 *      Success -   0
 *  @retval
 *      Error   -   <0
 *
 */
int32_t  MmwDemo_rangeBiasRxChPhaseMeasureConfig ()
{
    int32_t retVal = 0;
    int32_t rngSearchIdx;
    float slope;
    float targetDistanceMts = gMmwMssMCB.measureRxChannelBiasCliCfg.targetDistanceMts;
    float searchWinSizeMts= gMmwMssMCB.measureRxChannelBiasCliCfg.searchWinSizeMts;

    /* Check for CLI configuration, TDM MIMO mode, all antennas active: 4Tx 4Rx */
    if ((gMmwMssMCB.mmWaveCfg.profileComCfg.chirpTxMimoPatSel != 1) ||
        (gMmwMssMCB.numTxAntennas != SYS_COMMON_NUM_TX_ANTENNAS) ||
        (gMmwMssMCB.numRxAntennas != SYS_COMMON_NUM_RX_CHANNEL))
    {
        retVal = -1;
    }
    else
    {
        /* Range step (meters/bin)*/
        slope = (float)(gMmwMssMCB.mmWaveCfg.profileTimeCfg.chirpSlope * 1.e12);
        gMmwMssMCB.measureRxChannelBiasParams.rangeStep = (MMWDEMO_RFPARSER_SPEED_OF_LIGHT_IN_METERS_PER_SEC * (gMmwMssMCB.adcSamplingRate * 1.e6)) /
                                                        (2.f * slope * (2*gMmwMssMCB.numRangeBins));
        gMmwMssMCB.measureRxChannelBiasParams.oneOverRangeStep = 1 / gMmwMssMCB.measureRxChannelBiasParams.rangeStep;


        /* Target position in bins */
        gMmwMssMCB.measureRxChannelBiasParams.trueBinPosition = targetDistanceMts  * gMmwMssMCB.measureRxChannelBiasParams.oneOverRangeStep;

        /* Find the search range for the peak of the target at the bore sight */
        rngSearchIdx = (int32_t) ((targetDistanceMts - searchWinSizeMts/2.) * gMmwMssMCB.measureRxChannelBiasParams.oneOverRangeStep + 0.5);
        if (rngSearchIdx < 1)
        {
            rngSearchIdx = 1;
        }
        gMmwMssMCB.measureRxChannelBiasParams.rngSearchLeftIdx = (int16_t) rngSearchIdx;
        rngSearchIdx = (int32_t) ((targetDistanceMts + searchWinSizeMts/2.) * gMmwMssMCB.measureRxChannelBiasParams.oneOverRangeStep + 0.5);
        gMmwMssMCB.measureRxChannelBiasParams.rngSearchRightIdx = (int16_t) rngSearchIdx;
    }

    return retVal;
}

/**
 *  @b Description
 *  @n
 *      Computes the range bias and rx phase compensation coefficients
 *      Sensor must be configured in 4Tx 4Rx TDM-MIMO mode
 *
 *  @retval   None
 *
 */
void MmwDemo_rangeBiasRxChPhaseMeasure()
{
    cmplx16ImRe_t rxSym[SYS_COMMON_NUM_TX_ANTENNAS*SYS_COMMON_NUM_RX_CHANNEL];
    cmplx16ImRe_t *tempPtr;
    float sumSqr, sumSqrMax;
    float xMagSq[SYS_COMMON_NUM_TX_ANTENNAS*SYS_COMMON_NUM_RX_CHANNEL] = {0};
    float xMagSqMin, xMagSqRootMin;
    float scal;
    float truePosition = gMmwMssMCB.measureRxChannelBiasParams.trueBinPosition;
    float rangeStep = gMmwMssMCB.measureRxChannelBiasParams.rangeStep;
    float y[3];
    float x[3];
    float estPeakPos, estPeakVal;
    int32_t rngSearchIdx, rngSearchIdxMax, ind, rxChPhaseCompIdx;
    int32_t txIdx, rxIdx, antIdx;

    DPIF_compRxChannelBiasFloatCfg *compRxChanCfg = &gMmwMssMCB.compRxChannelBiasCfgMeasureOut;
    cmplx16ImRe_t *symbolMatrix = (cmplx16ImRe_t *) gMmwMssMCB.radarCube;
    uint32_t numRxAntennas = gMmwMssMCB.numRxAntennas;
    uint32_t numTxAntennas = gMmwMssMCB.numTxAntennas;
    uint32_t numVirtualAntennas = numRxAntennas * numTxAntennas;
    uint32_t numColInSymbolMatrix = gMmwMssMCB.mmWaveCfg.frameCfg.numOfChirpsInBurst * gMmwMssMCB.mmWaveCfg.frameCfg.numOfBurstsInFrame * gMmwMssMCB.numRxAntennas;


    /**** Range calibration ****/
    rngSearchIdxMax = gMmwMssMCB.measureRxChannelBiasParams.rngSearchLeftIdx;
    sumSqrMax = 0;
    for (rngSearchIdx = gMmwMssMCB.measureRxChannelBiasParams.rngSearchLeftIdx; rngSearchIdx <= gMmwMssMCB.measureRxChannelBiasParams.rngSearchRightIdx; rngSearchIdx++)
    {
        sumSqr = 0.0;
        for (antIdx = 0; antIdx < numVirtualAntennas; antIdx++)
        {
            tempPtr = (cmplx16ImRe_t *) &symbolMatrix[rngSearchIdx * numColInSymbolMatrix + antIdx];
            sumSqr += (float) tempPtr->real * (float) tempPtr->real +
                      (float) tempPtr->imag * (float) tempPtr->imag;
        }

        if (sumSqr > sumSqrMax)
        {
            sumSqrMax = sumSqr;
            rngSearchIdxMax = rngSearchIdx;
        }
    }

    /* Fine estimate of the peak position using quadratic fit */
    ind = 0;
    for (rngSearchIdx = rngSearchIdxMax-1; rngSearchIdx <= rngSearchIdxMax+1; rngSearchIdx++)
    {
        sumSqr = 0.0;
        for (antIdx = 0; antIdx < numVirtualAntennas; antIdx++)
        {
            tempPtr = (cmplx16ImRe_t *) &symbolMatrix[rngSearchIdx * numColInSymbolMatrix + antIdx];
            sumSqr += (float) tempPtr->real * (float) tempPtr->real +
                      (float) tempPtr->imag * (float) tempPtr->imag;
        }

        y[ind] = sqrtf(sumSqr);
        x[ind] = (float)rngSearchIdx;
        ind++;
    }

    rangeBiasMeasure_quadfit(x, y, &estPeakPos, &estPeakVal);
    compRxChanCfg->rangeBias = (estPeakPos - truePosition) * rangeStep;

    /*** Calculate Rx channel phase/gain compensation coefficients ***/
    for (antIdx = 0; antIdx < numVirtualAntennas; antIdx++)
    {
        tempPtr = (cmplx16ImRe_t *) &symbolMatrix[rngSearchIdxMax * numColInSymbolMatrix + antIdx];
        rxSym[antIdx].real = tempPtr->real;
        rxSym[antIdx].imag = tempPtr->imag;
        xMagSq[antIdx] = (float) rxSym[antIdx].real * (float) rxSym[antIdx].real +
                    (float) rxSym[antIdx].imag * (float) rxSym[antIdx].imag;
    }

    xMagSqMin = xMagSq[0];
    for (antIdx = 1; antIdx < numVirtualAntennas; antIdx++)
    {
        if (xMagSq[antIdx] < xMagSqMin)
        {
            xMagSqMin = xMagSq[antIdx];
        }
    }

    if (xMagSqMin > 0.)
    {
        xMagSqRootMin = sqrt(xMagSqMin);
        for (txIdx=0; txIdx < numTxAntennas; txIdx++)
        {
            for (rxIdx=0; rxIdx < numRxAntennas; rxIdx++)
            {
                float temp;
                rxChPhaseCompIdx = txIdx * numRxAntennas + rxIdx;
                scal = 1./ xMagSq[rxChPhaseCompIdx] * xMagSqRootMin;

                temp = scal * rxSym[rxChPhaseCompIdx].real;
                compRxChanCfg->rxChPhaseComp[2*rxChPhaseCompIdx] = temp;

                temp = -scal * rxSym[rxChPhaseCompIdx].imag;
                compRxChanCfg->rxChPhaseComp[2*rxChPhaseCompIdx+1] = temp;
            }
        }
    }
    else
    {
        for (rxChPhaseCompIdx=0; rxChPhaseCompIdx < (2*numVirtualAntennas); rxChPhaseCompIdx++)
        {
                compRxChanCfg->rxChPhaseComp[rxChPhaseCompIdx] = 0.;
        }
    }
}