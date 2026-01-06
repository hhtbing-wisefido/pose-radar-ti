
/**
 *  \file   RADARDEMO_aoaEst2DCaponBF_heatmapEst.c
 *
 *   \brief   Estimate the range-angle heatmap using Capon BF.
 *
 * Copyright (C) 2019 Texas Instruments Incorporated - http://www.ti.com/ 
 * 
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
 *
*/

#include <source/dpu/capon3d_overhead/modules/DoA/CaponBF2D/src/RADARDEMO_aoaEst2DCaponBF_priv.h>

#define ONLY_16x16

#ifdef _TMS320C6X
#include "c6x.h"
#endif


/*!
 *   \fn     RADARDEMO_aoaEst2DCaponBF_raHeatmap
 *
 *   \brief   Use Capon beamforming to generate range angle heatmap per range bin.
 *
 *   \param[in]    bfFlag
 *               Flag to indicate which covariance matrix based beamforming will be performed. 
 *               If set to 1, Capon BF.
 *               If set to 0, conventional BF.
 *
 *   \param[in]    nRxAnt
 *               number of antenna
 *
 *   \param[in]    steerVecAnts
 *               number of antenna for steering vectors, it is the same as total number of virtual antennas in the system.
 *
 *   \param[in]    numAzimuthBins
 *               number of Azimuth bins
 *
  *   \param[in]    numElevationBins
 *               number of elevation bins
 *
*   \param[in]    steeringVecAzim
 *              steering vector for azimuth domain beamforming.
 *
*   \param[in]    steeringVecElev
 *              steering vector for elevtion domain beamforming.
 *
*   \param[in]    ant2Proc
 *              Antennas to process in the azimuth domain.
 *
 *   \param[in]    scratch
 *               scratch memory, must be of size of nRxAnt * nRxAnt * 2 * 4 bytes.
 *               Must be aligned to 8-byte boundary.
 *
 *   \param[in]    invRnMatrices
 *               Output inverse of covariance matrices or the current range bin, in order of upper triangle of nRxAnt x nRxAnt Hermitian matrix.
 *               Must be aligned to 8-byte boundary.
 *
 *   \param[out]    maxValPerRngBin
 *               Output peak value in angle domain, per range bin.
 *               Must be aligned to 8-byte boundary.
 *
 *   \param[out]    rangeAzimuthHeatMap
 *               Output range azimuth heatmap per range bin length of number of angle bins
 *               Must be aligned to 8-byte boundary.
 *
 *   \ret       none
 *
 *   \pre       none
 *
 *   \post      none
 *
 *
 */

void RADARDEMO_aoaEst2DCaponBF_raHeatmap(
                IN uint8_t bfFlag,
                IN int32_t  nRxAnt,
                IN int32_t  steerVecAnts,
                IN int32_t  numAzimuthBins,
                IN int32_t  numElevationBins,
                IN cplxf_t * RESTRICT steeringVecAzim,
                IN cplxf_t * RESTRICT steeringVecElev,
                IN uint8_t * RESTRICT ant2Proc,
                IN int32_t * scratch,
                IN cplxf_t * RESTRICT invRnMatrices,
                IN float   * RESTRICT maxValPerRngBin,
                OUT float  * RESTRICT rangeAzimuthHeatMap)
{
    int32_t     i, j, angleIdx, rnIdx, rnIdxRec;
    __float2_t  * RESTRICT steeringVecPtr;
    __float2_t  f2temp, acc0f2;
    float       output, output1, result, maxVal, diagSum;

#ifdef _TMS320C6X
    _nassert(nRxAnt %4          ==  0);
    _nassert(steerVecAnts %4    ==  0);
    _nassert(numAzimuthBins     >=  30);
#endif

    j               =   0;
    //diagSum           =   _hif2(_amem8_f2(&invRnMatrices[0]));
    diagSum         =   0;
    for (i = 0; i < nRxAnt; i++)
    {
        diagSum     +=  _hif2(_amem8_f2(&invRnMatrices[i * nRxAnt + i - j]));
        j           +=  i + 1;
    }
    diagSum         =   diagSum * 0.5f;

#ifndef ONLY_16x16
    if (nRxAnt == 4)
    {
        maxVal                              =   0.f;
        for (angleIdx = 0; angleIdx < numAzimuthBins; angleIdx++ )
        {
            steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
            rnIdx                   =   0;

            // skip 00
            rnIdx++;

            // 01 and 10
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[0]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[1]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 =   (_hif2(acc0f2) - _lof2(acc0f2));

            // 02 and 20
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[0]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[2]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 03 and 30
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[0]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[3]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // skip 11
            rnIdx++;

            // 12 and 21
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[1]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[2]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 13 and 31
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[1]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[3]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // skip 22
            rnIdx++;

            // 23 and 32
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[2]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[3]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // skip 33
            output                  =   2.f * (diagSum + output1);

            result                  =   _rcpsp(output);
            result                  =   result * (2.f - output * result);
            result                  =   result * (2.f - output * result);

            if (!bfFlag)
                result              =   output;
            rangeAzimuthHeatMap[angleIdx]   =   result;
            if (maxVal < result)
                maxVal              =   result;
        }
    }
    else if (nRxAnt == 8)
    {
        // full manual unroll, multiple loops, temp results stored in rangeAzimuthHeatMap buffer

        // for 01 to 07
        maxVal                              =   0.f;
        for (angleIdx = 0; angleIdx < numAzimuthBins; angleIdx++ )
        {
            steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
            rnIdx                   =   0;

            // skip 00
            rnIdx++;

            // 01 and 10
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[0]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[1]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 =   (_hif2(acc0f2) - _lof2(acc0f2));

            // 02 and 20
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[0]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[2]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 03 and 30
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[0]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[3]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 04 and 40
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[0]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[4]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 05 and 50
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[0]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[5]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 06 and 60
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[0]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[6]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 07 and 70
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[0]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[7]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));
            rangeAzimuthHeatMap[angleIdx]   =   output1 + diagSum;
        }
        rnIdxRec                    =   rnIdx;

        // for 12 to 17
        for (angleIdx = 0; angleIdx < numAzimuthBins; angleIdx++ )
        {
            steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
            rnIdx                   =   rnIdxRec;

            // skip 11
            rnIdx++;

            // 12 and 21
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[1]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[2]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 =   (_hif2(acc0f2) - _lof2(acc0f2));

            // 13 and 31
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[1]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[3]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 14 and 41
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[1]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[4]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 15 and 51
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[1]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[5]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 16 and 61
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[1]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[6]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 17 and 71
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[1]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[7]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            rangeAzimuthHeatMap[angleIdx]   +=  output1;
        }
        rnIdxRec                    =   rnIdx;

        // for 23 to 37
        for (angleIdx = 0; angleIdx < numAzimuthBins; angleIdx++ )
        {
            steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
            rnIdx                   =   rnIdxRec;

            // skip 22
            rnIdx++;

            // 23 and 32
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[2]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[3]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 =   (_hif2(acc0f2) - _lof2(acc0f2));

            // 24 and 42
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[2]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[4]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 25 and 52
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[2]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[5]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 26 and 62
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[2]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[6]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 27 and 72
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[2]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[7]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // skip 33
            rnIdx++;

            // 34 and 43
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[3]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[4]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 35 and 53
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[3]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[5]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 36 and 63
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[3]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[6]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 37 and 73
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[3]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[7]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            rangeAzimuthHeatMap[angleIdx]   +=  output1;
        }
        rnIdxRec                    =   rnIdx;

        // 45 to end
        for (angleIdx = 0; angleIdx < numAzimuthBins; angleIdx++ )
        {
            steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
            rnIdx                   =   rnIdxRec;
            output1                 =   rangeAzimuthHeatMap[angleIdx];

            // skip 44
            rnIdx++;

            // 45 and 54
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[4]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[5]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 46 and 64
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[4]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[6]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 47 and 74
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[4]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[7]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // skip 55
            rnIdx++;

            // 56 and 65
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[5]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[6]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 57 and 75
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[5]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[7]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // skip 66
            rnIdx++;

            // 67 and 76
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[6]]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[ant2Proc[7]]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            output                  =   2.f * output1;

            result                  =   _rcpsp(output);
            result                  =   result * (2.f - output * result);
            result                  =   result * (2.f - output * result);

            if (!bfFlag)
                result              =   output;
            rangeAzimuthHeatMap[angleIdx]   =   result;
            if (maxVal < result)
                maxVal              =   result;
        }
    }
    else if (nRxAnt == 12)
    {
        int32_t     numAngleBins;

        numAngleBins            =   numAzimuthBins * numElevationBins;
#ifdef _TMS320C6X
        _nassert(numAngleBins >= 80);
#endif
        maxVal                  =   0.f;
        // complete manual unroll for 12 antennas
        // 01 to 06
        #ifdef _TMS320C6X
        #pragma MUST_ITERATE(80,,);
        #endif
        for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
        {
            steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
            rnIdx                   =   0;

            // skip 00
            rnIdx++;

            // 01 and 10
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[0]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[1]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 =   (_hif2(acc0f2) - _lof2(acc0f2));

            // 02 and 20
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[0]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[2]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 03 and 30
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[0]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[3]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 04 and 40
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[0]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[4]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 05 and 50
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[0]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[5]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 06 and 60
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[0]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[6]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            rangeAzimuthHeatMap[angleIdx]   =   output1 + diagSum;
        }
        rnIdxRec                    =   rnIdx;

        // for 07 to 12
        #ifdef _TMS320C6X
        #pragma MUST_ITERATE(80,,);
        #endif
        for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
        {
            steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
            rnIdx                   =   rnIdxRec;
            output1                 =   rangeAzimuthHeatMap[angleIdx];

            // 07 and 70
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[0]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[7]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 08 and 80
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[0]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[8]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // 09 and 90
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[0]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[9]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 0a and a0
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[0]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[10]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 0b and b0
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[0]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[11]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // skip 11
            rnIdx++;

            // 12 and 21
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[1]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[2]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            rangeAzimuthHeatMap[angleIdx]   =   output1;
        }

        rnIdxRec                    =   rnIdx;

        // for 13 to 18
        #ifdef _TMS320C6X
        #pragma MUST_ITERATE(80,,);
        #endif
        for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
        {
            steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
            rnIdx                   =   rnIdxRec;
            output1                 =   rangeAzimuthHeatMap[angleIdx];

            // 13 and 31
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[1]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[3]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 14 and 41
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[1]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[4]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 15 and 51
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[1]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[5]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 16 and 61
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[1]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[6]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // 17 and 71
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[1]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[7]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 18 and 81
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[1]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[8]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            rangeAzimuthHeatMap[angleIdx]   =   output1;
        }
        rnIdxRec                    =   rnIdx;

        // for 19 to 25
        #ifdef _TMS320C6X
        #pragma MUST_ITERATE(80,,);
        #endif
        for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
        {
            steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
            rnIdx                   =   rnIdxRec;
            output1                 =   rangeAzimuthHeatMap[angleIdx];

            // 19 and 91
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[1]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[9]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

                // 1a and a1
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[1]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[10]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 1b and b1
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[1]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[11]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // skip 22
            rnIdx++;

            // 23 and 32
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[2]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[3]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 24 and 42
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[2]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[4]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 25 and 52
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[2]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[5]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            rangeAzimuthHeatMap[angleIdx]   =   output1;
        }
        rnIdxRec                    =   rnIdx;

        // for 26 to 2b
        #ifdef _TMS320C6X
        #pragma MUST_ITERATE(80,,);
        #endif
        for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
        {
            steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
            rnIdx                   =   rnIdxRec;
            output1                 =   rangeAzimuthHeatMap[angleIdx];

            // 26 and 62
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[2]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[6]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 27 and 72
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[2]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[7]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 28 and 82
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[2]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[8]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 29 and 92
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[2]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[9]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 2a and a2
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[2]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[10]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 2b and b2
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[2]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[11]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // skip 33
            rnIdx++;

            rangeAzimuthHeatMap[angleIdx]   =   output1;
        }
        rnIdxRec                    =   rnIdx;

            // for 34 to 39
        #ifdef _TMS320C6X
        #pragma MUST_ITERATE(80,,);
        #endif
        for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
        {
            steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
            rnIdx                   =   rnIdxRec;
            output1                 =   rangeAzimuthHeatMap[angleIdx];

            // 34 and 43
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[3]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[4]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

            // 35 and 53
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[3]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[5]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // 36 and 63
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[3]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[6]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // 37 and 73
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[3]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[7]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // 38 and 83
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[3]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[8]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // 39 and 93
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[3]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[9]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            rangeAzimuthHeatMap[angleIdx]   =   output1;
        }

        rnIdxRec                    =   rnIdx;

        // for 3a to 48
        #ifdef _TMS320C6X
        #pragma MUST_ITERATE(80,,);
        #endif
        for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
        {
            steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
            rnIdx                   =   rnIdxRec;
            output1                 =   rangeAzimuthHeatMap[angleIdx];

            // 3a and a3
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[3]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[10]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // 3b and b3
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[3]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[11]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // skip 44
            rnIdx++;

            // 45 and 54
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[4]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[5]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // 46 and 64
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[4]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[6]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // 47 and 74
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[4]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[7]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // 48 and 84
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[4]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[8]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            rangeAzimuthHeatMap[angleIdx]   =   output1;
        }

        rnIdxRec                    =   rnIdx;

        // for 49 to 58
        #ifdef _TMS320C6X
        #pragma MUST_ITERATE(80,,);
        #endif
        for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
        {
            steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
            rnIdx                   =   rnIdxRec;
            output1                 =   rangeAzimuthHeatMap[angleIdx];

            // 49 and 94
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[4]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[9]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // 4a and a4
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[4]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[10]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // 4b and b4
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[4]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[11]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // skip 55
            rnIdx++;

            // 56 and 65
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[5]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[6]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // 57 and 75
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[5]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[7]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // 58 and 85
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[5]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[8]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            rangeAzimuthHeatMap[angleIdx]   =   output1;
        }
        rnIdxRec                    =   rnIdx;

        // for 59 to 69
        #ifdef _TMS320C6X
        #pragma MUST_ITERATE(80,,);
        #endif
        for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
        {
            steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
            rnIdx                   =   rnIdxRec;
            output1                 =   rangeAzimuthHeatMap[angleIdx];

            // 59 and 95
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[5]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[9]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // 5a and a5
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[5]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[10]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // 5b and b5
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[5]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[11]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // skip 66
            rnIdx++;

                // 67 and 76
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[6]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[7]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // 68 and 86
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[6]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[8]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // 69 and 96
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[6]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[9]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            rangeAzimuthHeatMap[angleIdx]   =   output1;
        }
        rnIdxRec                    =   rnIdx;

        // for 6a to 8a
        #ifdef _TMS320C6X
        #pragma MUST_ITERATE(80,,);
        #endif
        for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
        {
            steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
            rnIdx                   =   rnIdxRec;
            output1                 =   rangeAzimuthHeatMap[angleIdx];

            // 6a and a6
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[6]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[10]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // 6b and b6
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[6]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[11]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // skip 77
            rnIdx++;

            // 78 and 87
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[7]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[8]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // 79 and 97
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[7]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[9]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // 7a and a7
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[7]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[10]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // 7b and b7
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[7]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[11]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // skip 88
            rnIdx++;

            rangeAzimuthHeatMap[angleIdx]   =   output1;
        }
        rnIdxRec                    =   rnIdx;

        // for 89 to end
        #ifdef _TMS320C6X
        #pragma MUST_ITERATE(80,,);
        #endif
        for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
        {
            steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
            rnIdx                   =   rnIdxRec;
            output1                 =   rangeAzimuthHeatMap[angleIdx];

            // 89 and 98
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[8]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[9]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // 8a and a8
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[8]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[10]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // 8b and b8
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[8]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[11]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // skip 99
            rnIdx++;

            // 9a and a9
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[9]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[10]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // 9b and b9
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[9]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[11]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

            // skip aa
            rnIdx++;

            // ab and ba
            f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
            acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[10]), f2temp);
            f2temp                  =   _amem8_f2(&steeringVecPtr[11]);
            acc0f2                  =   _dmpysp(f2temp, acc0f2);
            output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));


            output                  =   2.f * output1;

            result                  =   _rcpsp(output);
            result                  =   result * (2.f - output * result);
            result                  =   result * (2.f - output * result);

            if (!bfFlag)
                result              =   output;
            rangeAzimuthHeatMap[angleIdx]   =   result;
            if (maxVal < result)
            {
                maxVal              =   result;
            }
        }
    }

    else if (nRxAnt == 16)
#else
    if (nRxAnt == 16)
#endif
    {
        int32_t     numAngleBins;

           numAngleBins            =   numAzimuthBins * numElevationBins;
   #ifdef _TMS320C6X
           _nassert(numAngleBins >= 80);
   #endif
           maxVal                  =   0.f;
           // complete manual unroll for 16 antennas
           // 01 to 06
           #ifdef _TMS320C6X
           #pragma MUST_ITERATE(80,,);
           #endif
           for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
           {
               steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
               rnIdx                   =   0;

               // skip 00
               rnIdx++;

               // 01 and 10
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]); // take first entry of invR
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[0]), f2temp); // multiply invR with conjugate of steering vector
               f2temp                  =   _amem8_f2(&steeringVecPtr[1]); // take second steering vector entry
               acc0f2                  =   _dmpysp(f2temp, acc0f2); // multiply by previous product
               output1                 =   (_hif2(acc0f2) - _lof2(acc0f2)); 

               // 02 and 20
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[0]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[2]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

               // 03 and 30
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[0]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[3]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

               // 04 and 40
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[0]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[4]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

               // 05 and 50
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[0]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[5]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

               // 06 and 60
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[0]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[6]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

               rangeAzimuthHeatMap[angleIdx]   =   output1 + diagSum;
           }
           rnIdxRec                    =   rnIdx;

           // for 07 to 0c
           #ifdef _TMS320C6X
           #pragma MUST_ITERATE(80,,);
           #endif
           for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
           {
               steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
               rnIdx                   =   rnIdxRec;
               output1                 =   rangeAzimuthHeatMap[angleIdx];

               // 07 and 70
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[0]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[7]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

               // 08 and 80
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[0]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[8]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 09 and 90
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[0]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[9]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

               // 0a and a0
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[0]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[10]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

               // 0b and b0
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[0]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[11]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

               // 0c and c0
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[0]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[12]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

               rangeAzimuthHeatMap[angleIdx]   =   output1;
           }

           rnIdxRec                    =   rnIdx;

           // for 0d to 14
           #ifdef _TMS320C6X
           #pragma MUST_ITERATE(80,,);
           #endif
           for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
           {
               steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
               rnIdx                   =   rnIdxRec;
               output1                 =   rangeAzimuthHeatMap[angleIdx];

               // 0d and d0
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[0]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[13]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

               // 0e and e0
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[0]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[14]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

               // 0f and f0
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[0]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[15]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

               // skip 11
               rnIdx++;

               // 12 and 21
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[1]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[2]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 13 and 31
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[1]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[3]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

               // 14 and 41
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[1]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[4]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

               rangeAzimuthHeatMap[angleIdx]   =   output1;
           }
           rnIdxRec                    =   rnIdx;

           // for 15 to 1a
           #ifdef _TMS320C6X
           #pragma MUST_ITERATE(80,,);
           #endif
           for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
           {
               steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
               rnIdx                   =   rnIdxRec;
               output1                 =   rangeAzimuthHeatMap[angleIdx];

               // 15 and 51
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[1]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[5]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

               // 16 and 61
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[1]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[6]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

               // 17 and 71
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[1]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[7]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

               // 18 and 81
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[1]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[8]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

               // 19 and 91
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[1]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[9]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

               // 1a and a1
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[1]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[10]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               rangeAzimuthHeatMap[angleIdx]   =   output1;
           }
           rnIdxRec                    =   rnIdx;

           // for 1b to 24
           #ifdef _TMS320C6X
           #pragma MUST_ITERATE(80,,);
           #endif
           for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
           {
               steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
               rnIdx                   =   rnIdxRec;
               output1                 =   rangeAzimuthHeatMap[angleIdx];

               // 1b and b1
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[1]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[11]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

               // 1c and c1
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[1]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[12]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

               // 1d and d1
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[1]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[13]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

               // 1e and e1
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[1]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[14]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

               // 1f and f1
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[1]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[15]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

               // skip 22
               rnIdx++;

               // 23 and 32
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[2]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[3]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

               // 24 and 42
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[2]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[4]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

               rangeAzimuthHeatMap[angleIdx]   =   output1;
           }
           rnIdxRec                    =   rnIdx;

           // for 25 to 2a
           #ifdef _TMS320C6X
           #pragma MUST_ITERATE(80,,);
           #endif
           for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
           {
               steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
               rnIdx                   =   rnIdxRec;
               output1                 =   rangeAzimuthHeatMap[angleIdx];

               // 25 and 52
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[2]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[5]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=  (_hif2(acc0f2) - _lof2(acc0f2));

               // 26 and 62
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[2]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[6]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 27 and 72
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[2]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[7]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 28 and 82
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[2]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[8]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 29 and 92
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[2]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[9]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 2a and a2
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[2]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[10]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               rangeAzimuthHeatMap[angleIdx]   =   output1;
           }

           rnIdxRec                    =   rnIdx;

           // for 2b to 35
           #ifdef _TMS320C6X
           #pragma MUST_ITERATE(80,,);
           #endif
           for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
           {
               steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
               rnIdx                   =   rnIdxRec;
               output1                 =   rangeAzimuthHeatMap[angleIdx];

               // 2b and b2
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[2]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[11]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 2c and c2
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[2]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[12]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 2d and d2
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[2]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[13]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 2e and e2
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[2]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[14]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 2f and f2
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[2]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[15]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // skip 33
               rnIdx++;

               // 34 and 43
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[3]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[4]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               rangeAzimuthHeatMap[angleIdx]   =   output1;
           }

           rnIdxRec                    =   rnIdx;

           // for 35 to 3a
           #ifdef _TMS320C6X
           #pragma MUST_ITERATE(80,,);
           #endif
           for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
           {
               steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
               rnIdx                   =   rnIdxRec;
               output1                 =   rangeAzimuthHeatMap[angleIdx];

               // 35 and 53
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[3]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[5]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 36 and 63
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[3]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[6]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 37 and 73
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[3]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[7]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 38 and 83
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[3]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[8]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 39 and 93
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[3]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[9]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 3a and a3
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[3]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[10]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               rangeAzimuthHeatMap[angleIdx]   =   output1;
           }
           rnIdxRec                    =   rnIdx;

           // for 3b to 45
           #ifdef _TMS320C6X
           #pragma MUST_ITERATE(80,,);
           #endif
           for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
           {
               steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
               rnIdx                   =   rnIdxRec;
               output1                 =   rangeAzimuthHeatMap[angleIdx];

               // 3b and b3
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[3]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[11]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 3c and c3
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[3]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[12]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 3d and d3
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[3]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[13]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 3e and e3
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[3]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[14]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 3f and f3
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[3]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[15]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // skip 44
               rnIdx++;

               // 45 and 54
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[4]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[5]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               rangeAzimuthHeatMap[angleIdx]   =   output1;
           }
           rnIdxRec                    =   rnIdx;

           // for 46 to 4b
           #ifdef _TMS320C6X
           #pragma MUST_ITERATE(80,,);
           #endif
           for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
           {
               steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
               rnIdx                   =   rnIdxRec;
               output1                 =   rangeAzimuthHeatMap[angleIdx];

               // 46 and 64
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[4]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[6]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 47 and 74
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[4]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[7]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 48 and 84
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[4]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[8]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 49 and 94
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[4]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[9]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 4a and a4
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[4]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[10]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 4b and b4
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[4]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[11]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               rangeAzimuthHeatMap[angleIdx]   =   output1;
           }
           rnIdxRec                    =   rnIdx;

           // for 4c to 57
           #ifdef _TMS320C6X
           #pragma MUST_ITERATE(80,,);
           #endif
           for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
           {
               steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
               rnIdx                   =   rnIdxRec;
               output1                 =   rangeAzimuthHeatMap[angleIdx];

               // 4c and c4
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[4]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[12]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 4d and d4
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[4]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[13]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 4e and e4
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[4]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[14]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));


               // 4f and f4
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[4]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[15]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // skip 55
               rnIdx++;

               // 56 and 65
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[5]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[6]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 57 and 75
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[5]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[7]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));
               rangeAzimuthHeatMap[angleIdx]   =   output1;
           }
           rnIdxRec                    =   rnIdx;

           // for 58 to 5d
           #ifdef _TMS320C6X
           #pragma MUST_ITERATE(80,,);
           #endif
           for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
           {
               steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
               rnIdx                   =   rnIdxRec;
               output1                 =   rangeAzimuthHeatMap[angleIdx];

               // 58 and 85
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[5]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[8]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 59 and 95
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[5]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[9]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 5a and a5
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[5]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[10]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 5b and b5
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[5]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[11]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 5c and c5
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[5]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[12]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));


               // 5d and d5
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[5]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[13]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));
               rangeAzimuthHeatMap[angleIdx]   =   output1;
           }
           rnIdxRec                    =   rnIdx;

           // for 5e to 6a
           #ifdef _TMS320C6X
           #pragma MUST_ITERATE(80,,);
           #endif
           for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
           {
               steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
               rnIdx                   =   rnIdxRec;
               output1                 =   rangeAzimuthHeatMap[angleIdx];

               // 5e and e5
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[5]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[14]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 5f and f5
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[5]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[15]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // skip 66
               rnIdx++;

               // 67 and 76
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[6]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[7]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 68 and 86
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[6]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[8]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));


               // 69 and 96
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[6]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[9]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 6a and a6
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[6]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[10]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));
               rangeAzimuthHeatMap[angleIdx]   =   output1;
           }
           rnIdxRec                    =   rnIdx;

           // for 6b to 78
           #ifdef _TMS320C6X
           #pragma MUST_ITERATE(80,,);
           #endif
           for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
           {
               steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
               rnIdx                   =   rnIdxRec;
               output1                 =   rangeAzimuthHeatMap[angleIdx];

               // 6b and b6
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[6]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[11]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 6c and c6
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[6]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[12]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 6d and d6
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[6]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[13]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 6e and e6
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[6]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[14]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 6f and f6
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[6]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[15]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // skip 77
               rnIdx++;

               // 78 and 87
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[7]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[8]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));
               rangeAzimuthHeatMap[angleIdx]   =   output1;
           }
           rnIdxRec                    =   rnIdx;

           // for 79 to 7e
           #ifdef _TMS320C6X
           #pragma MUST_ITERATE(80,,);
           #endif
           for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
           {
               steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
               rnIdx                   =   rnIdxRec;
               output1                 =   rangeAzimuthHeatMap[angleIdx];

               // 79 and 97
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[7]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[9]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 7a and a7
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[7]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[10]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 7b and b7
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[7]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[11]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 7c and c7
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[7]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[12]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 7d and d7
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[7]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[13]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 7e and e7
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[7]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[14]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));
               rangeAzimuthHeatMap[angleIdx]   =   output1;
           }
           rnIdxRec                    =   rnIdx;

           // for 7f to 8d
           #ifdef _TMS320C6X
           #pragma MUST_ITERATE(80,,);
           #endif
           for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
           {
               steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
               rnIdx                   =   rnIdxRec;
               output1                 =   rangeAzimuthHeatMap[angleIdx];

               // 7f and f7
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[7]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[15]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // skip 88
               rnIdx++;

               // 89 and 98
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[8]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[9]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 8a and a8
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[8]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[10]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 8b and b8
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[8]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[11]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 8c and c8
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[8]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[12]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 8d and d8
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[8]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[13]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));
               rangeAzimuthHeatMap[angleIdx]   =   output1;
           }
           rnIdxRec                    =   rnIdx;

           // for 8e to 9d
           #ifdef _TMS320C6X
           #pragma MUST_ITERATE(80,,);
           #endif
           for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
           {
               steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
               rnIdx                   =   rnIdxRec;
               output1                 =   rangeAzimuthHeatMap[angleIdx];

               // 8e and e8
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[8]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[14]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 8f and f8
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[8]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[15]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // skip 99
               rnIdx++;

               // 9a and a9
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[9]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[10]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 9b and b9
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[9]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[11]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 9c and c9
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[9]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[12]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));


               // 9d and d9
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[9]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[13]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));
               rangeAzimuthHeatMap[angleIdx]   =   output1;
           }
           rnIdxRec                    =   rnIdx;

           // for 9e to ae
           #ifdef _TMS320C6X
           #pragma MUST_ITERATE(80,,);
           #endif
           for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
           {
               steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
               rnIdx                   =   rnIdxRec;
               output1                 =   rangeAzimuthHeatMap[angleIdx];

               // 9e and e9
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[9]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[14]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // 9f and f9
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[9]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[15]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // skip aa
               rnIdx++;

               // ab and ba
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[10]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[11]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // ac and ca
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[10]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[12]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // ad and da
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[10]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[13]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

               // ae and ea
               f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
               acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[10]), f2temp);
               f2temp                  =   _amem8_f2(&steeringVecPtr[14]);
               acc0f2                  =   _dmpysp(f2temp, acc0f2);
               output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));
               rangeAzimuthHeatMap[angleIdx]   =   output1;
           }
           rnIdxRec                    =   rnIdx;

           // for af to cd
            #ifdef _TMS320C6X
                #pragma MUST_ITERATE(80,,);
            #endif
           for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
           {
              steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
              rnIdx                   =   rnIdxRec;
              output1                 =   rangeAzimuthHeatMap[angleIdx];

              // af and fa
              f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
              acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[10]), f2temp);
              f2temp                  =   _amem8_f2(&steeringVecPtr[15]);
              acc0f2                  =   _dmpysp(f2temp, acc0f2);
              output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

              // skip bb
              rnIdx++;

              // bc and cb
              f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
              acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[11]), f2temp);
              f2temp                  =   _amem8_f2(&steeringVecPtr[12]);
              acc0f2                  =   _dmpysp(f2temp, acc0f2);
              output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));



              // bd and db
              f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
              acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[11]), f2temp);
              f2temp                  =   _amem8_f2(&steeringVecPtr[13]);
              acc0f2                  =   _dmpysp(f2temp, acc0f2);
              output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

              // be and eb
              f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
              acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[11]), f2temp);
              f2temp                  =   _amem8_f2(&steeringVecPtr[14]);
              acc0f2                  =   _dmpysp(f2temp, acc0f2);
              output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

              // bf and fb
              f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
              acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[11]), f2temp);
              f2temp                  =   _amem8_f2(&steeringVecPtr[15]);
              acc0f2                  =   _dmpysp(f2temp, acc0f2);
              output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

              // skip cc
              rnIdx++;

              // cd and dc
              f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
              acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[12]), f2temp);
              f2temp                  =   _amem8_f2(&steeringVecPtr[13]);
              acc0f2                  =   _dmpysp(f2temp, acc0f2);
              output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));
              rangeAzimuthHeatMap[angleIdx]   =   output1;
          }
          rnIdxRec                    =   rnIdx;

          // for ce to ff
            #ifdef _TMS320C6X
                #pragma MUST_ITERATE(80,,);
            #endif
           for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
           {
              steeringVecPtr          =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
              rnIdx                   =   rnIdxRec;
              output1                 =   rangeAzimuthHeatMap[angleIdx];

              // ce and ec
              f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
              acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[12]), f2temp);
              f2temp                  =   _amem8_f2(&steeringVecPtr[14]);
              acc0f2                  =   _dmpysp(f2temp, acc0f2);
              output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

              // cf and fc
              f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
              acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[12]), f2temp);
              f2temp                  =   _amem8_f2(&steeringVecPtr[15]);
              acc0f2                  =   _dmpysp(f2temp, acc0f2);
              output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

              // skip dd
              rnIdx++;

              // de and ed
              f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
              acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[13]), f2temp);
              f2temp                  =   _amem8_f2(&steeringVecPtr[14]);
              acc0f2                  =   _dmpysp(f2temp, acc0f2);
              output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

              // df and fd
              f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
              acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[13]), f2temp);
              f2temp                  =   _amem8_f2(&steeringVecPtr[15]);
              acc0f2                  =   _dmpysp(f2temp, acc0f2);
              output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

              // skip ee
              rnIdx++;

              // ef and fe
              f2temp                  =   _amem8_f2(&invRnMatrices[rnIdx++]);
              acc0f2                  =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[14]), f2temp);
              f2temp                  =   _amem8_f2(&steeringVecPtr[15]);
              acc0f2                  =   _dmpysp(f2temp, acc0f2);
              output1                 +=   (_hif2(acc0f2) - _lof2(acc0f2));

              // skip ff
              rnIdx++;

                output                  =   2.f * output1;

                result                  =   _rcpsp(output);
                result                  =   result * (2.f - output * result);
                result                  =   result * (2.f - output * result);

                if (!bfFlag)
                   result              =   output;
                rangeAzimuthHeatMap[angleIdx]   =   result;
                if (maxVal < result)
                {
                   maxVal              =   result;
                }
            }
    }



#ifndef ONLY_16x16

    else
    {
        int32_t     numAngleBins;

        numAngleBins            =   numAzimuthBins * numElevationBins;
        maxVal                  =   0.f;
        #pragma MUST_ITERATE(1,,);
        for (angleIdx = 0; angleIdx < numAngleBins; angleIdx++ )
        {
            steeringVecPtr      =   (__float2_t *) &steeringVecAzim[angleIdx * steerVecAnts];
            rnIdx               =   0;
            output              =   0.f;
            #pragma MUST_ITERATE(1,,);
            for (i = 0; i < nRxAnt-1; i++)
            {
                output          +=  _hif2(_amem8_f2(&invRnMatrices[rnIdx++]));
                #pragma MUST_ITERATE(1,,);
                for (j = i + 1; j < nRxAnt; j++)
                {
                    f2temp      =   _amem8_f2(&invRnMatrices[rnIdx++]);
                    acc0f2      =   _complex_conjugate_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[i]]), f2temp);
                    acc0f2      =   _complex_mpysp(_amem8_f2(&steeringVecPtr[ant2Proc[j]]), acc0f2);
                    output      +=  2.f * _hif2(acc0f2);
                }
            }

            result              =   _rcpsp(output);
            result              =   result * (2.f - output * result);
            result              =   result * (2.f - output * result);

            if (!bfFlag)
                result          =   output;
            rangeAzimuthHeatMap[angleIdx]   =   result;
            if (maxVal < result)
                maxVal          =   result;
        }
    }
#endif
    *maxValPerRngBin            =   maxVal;
}



