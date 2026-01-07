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
#include <utils/mathutils/mathutils.h>

#include <source/mmw_cli.h>
#include <source/mmwave_demo.h>
#include <source/test/ADC_testbuf.h>
#include <source/dpc/dpc.h>

#include <ti_drivers_config.h>
#include <ti_drivers_open_close.h>
#include <ti_board_open_close.h>
#include <ti_board_config.h>

#define MAX_NUM_TX_ANTENNA          (4U)
#define MAX_NUM_RX_ANTENNA          (4U)
#define MAX_AZ_FFT_SIZE             (64U)
#define MAX_NUM_RANGEBIN            (512U) 
#define MAX_NUM_CHIRPS_PERFRAME     (64U) //This is for xwrL6888

extern MmwDemo_MSS_MCB gMmwMssMCB;

int16_t *gPreStoredAdcTestBuff;
int32_t gPreStoredAdcTestBuffInd = 0;
int32_t gPreStoredAdcTestBuffRdInd = 0;

typedef struct rangeProcTestConfig_t_ {
    uint32_t numTxAntennas;
    uint32_t numRxAntennas;
    uint32_t numVirtualAntennas;
    uint32_t numAdcSamples;
    uint32_t numRangeBins;
    uint32_t numChirpsPerFrame;
    uint32_t numChirpsPerFrameRef;
    uint32_t numFrames;
} rangeProcTestConfig_t;

uint32_t  localRead(uint8_t * adcTestBuff, uint32_t sizeOfSamp,  uint32_t numSamp)
{
    memcpy((uint8_t *)adcTestBuff, (uint8_t *)&gPreStoredAdcTestBuff[gPreStoredAdcTestBuffInd], sizeOfSamp * numSamp);
    gPreStoredAdcTestBuffInd += numSamp;
    return numSamp;
}

/**
*  @b Description
*  @n
*        Function to read ADC data from file. For testing purpose only.
*        When FeatureLiteBuild is enabled offline adc data injection cannot be used.
*/
void MmwDemo_adcFileReadTask(){

     /*On R5F, by default task switch does not save/restore FPU (floating point unit) registers, tasks which need FPU need to call `vPortTaskUsesFPU`
    once before using FPU operations.*/
    vPortTaskUsesFPU();
    
    uint32_t baseAddr, regionId, numAdcSamplesPerEvt, numReadSamples = 0;
    int32_t  errorCode = 0;
    uint16_t frameCnt, i;
    bool endOfFile = false;
    FILE * fileIdAdcData;
    FILE * fileIdDetMatData;

    //FILE * fileIdDetMatDataRef;  //For CFAR offline verification

    FILE * fileIdPointCloudIndData;
    rangeProcTestConfig_t testConfig = {0};
    char fileOutName[DPC_ADC_FILENAME_MAX_LEN];
    char *ptr;
    uint16_t *detMatrixData;
    DPC_ObjectDetection_ExecuteResult *result = &gMmwMssMCB.dpcResult;
    
    detMatrixData = (uint16_t *) gMmwMssMCB.detMatrix;
    
    baseAddr = EDMA_getBaseAddr(gEdmaHandle[0]);
    DebugP_assert(baseAddr != 0);

    regionId = EDMA_getRegionId(gEdmaHandle[0]);
    DebugP_assert(regionId < SOC_EDMA_NUM_REGIONS);

    /* start the test */
    if (gMmwMssMCB.adcDataSourceCfg.source == 1)
    {
        fileIdAdcData = fopen(gMmwMssMCB.adcDataSourceCfg.fileName, "rb");
        if (fileIdAdcData == NULL)
        {
            printf("Error:  Cannot open ADC file !\n");
            exit(0);
        }

        /* Open output file for detection matrix target */
        strcpy(fileOutName, gMmwMssMCB.adcDataSourceCfg.fileName);
        ptr = strrchr(fileOutName, '/');
        if (ptr == NULL)
        {
            strcpy(fileOutName, "detMatTarget.bin");
        }
        else
        {
            strcpy(&ptr[1], "detMatTarget.bin");
        }
        fileIdDetMatData = fopen(fileOutName, "wb");
        if (fileIdDetMatData == NULL)
        {
            printf("Error:  Cannot open Detection Matrix file !\n");
            exit(0);
        }

        /* Open output file for point cloud list: range/azimuth/elevation/Doppler indices  */
        strcpy(fileOutName, gMmwMssMCB.adcDataSourceCfg.fileName);
        ptr = strrchr(fileOutName, '/');
        if (ptr == NULL)
        {
            strcpy(fileOutName, "pcloudIndTarget.bin");
        }
        else
        {
            strcpy(&ptr[1], "pcloudIndTarget.bin");
        }
        fileIdPointCloudIndData = fopen(fileOutName, "wb");
        if (fileIdPointCloudIndData == NULL)
        {
            printf("Error:  Cannot open Point Cloud file !\n");
            exit(0);
        }
    }


    /* read in test config */
    testConfig.numRxAntennas = gMmwMssMCB.numRxAntennas;

    if (gMmwMssMCB.adcDataSourceCfg.source == 1)
    {
        fread(&testConfig.numAdcSamples, sizeof(uint32_t),1,fileIdAdcData);
        fread(&testConfig.numVirtualAntennas, sizeof(uint32_t),1,fileIdAdcData);
        fread(&testConfig.numChirpsPerFrame, sizeof(uint32_t),1,fileIdAdcData);
        fread(&testConfig.numFrames, sizeof(uint32_t),1,fileIdAdcData);
    }

    if ((testConfig.numAdcSamples >= 2U) && (testConfig.numAdcSamples <= 1024U))
    {
        testConfig.numRangeBins = mathUtils_pow2roundup(testConfig.numAdcSamples)/2; //real only input
    }
    else
    {
        CLI_write("Error: Wrong test configurations \n");
        DebugP_log("numAdcSamples = %d\n", testConfig.numAdcSamples);
        exit(0);
    }

    testConfig.numTxAntennas = testConfig.numVirtualAntennas/testConfig.numRxAntennas;
    testConfig.numChirpsPerFrameRef = testConfig.numChirpsPerFrame;

    numAdcSamplesPerEvt = (testConfig.numAdcSamples * testConfig.numRxAntennas);

    if ((testConfig.numTxAntennas > MAX_NUM_TX_ANTENNA) || (testConfig.numRangeBins > MAX_NUM_RANGEBIN) || (testConfig.numChirpsPerFrame > MAX_NUM_CHIRPS_PERFRAME))
    {
        CLI_write("Error: Wrong test configurations \n");
        exit(0);
    }

    if ((testConfig.numFrames > 0) && (testConfig.numFrames < 65536))
    {
        //ToDo check that 4 params from ADC file match CLI configuration
        DebugP_log("numTxAntennas = %d\r", testConfig.numTxAntennas);
        DebugP_log("numRangeBins = %d\r", testConfig.numRangeBins);
        DebugP_log("numChirpsPerFrame = %d\n", testConfig.numChirpsPerFrame);
        DebugP_log("numFrames = %d\n", testConfig.numFrames);
    }
    else
    {
        CLI_write("Error: Wrong test configurations \n");
        DebugP_log("numFrames = %d\n", testConfig.numFrames);
        exit(0);
    }

    for(frameCnt = 0; frameCnt < testConfig.numFrames; frameCnt++)
    {
        /*To test cfar dpu*/
#if 0

        if(frameCnt >=3)
        {
            fread(detMatrixData, sizeof(uint32_t), 32*16, fileIdDetMatDataRef);
        }
#endif
        /* Read chirps from the file */
        for(i = 0; i < (testConfig.numChirpsPerFrame * testConfig.numTxAntennas); i++)
        {
            if (!endOfFile && gMmwMssMCB.adcDataSourceCfg.source != 0)
            {
                /* Read one chirp of ADC samples and to put data in ADC test buffer */
                if (gMmwMssMCB.adcDataSourceCfg.source == 1)
                {
                    numReadSamples = fread(gMmwMssMCB.adcTestBuff, sizeof(uint16_t),  numAdcSamplesPerEvt, fileIdAdcData);
                }

                if (numReadSamples != numAdcSamplesPerEvt)
                {
                    endOfFile = true;
                }
            }

            /* Manual trigger to simulate chirp avail irq */
            errorCode = EDMAEnableTransferRegion(
                            baseAddr, regionId, EDMA_DSS_TPCC_A_CHIRP_AVAIL_IRQ, EDMA_TRIG_MODE_MANUAL); //EDMA_TRIG_MODE_EVENT
            if (errorCode != 1)
            {
                CLI_write("Error: EDMA start Transfer returned %d\n",errorCode);
                return;
            }

            if (gMmwMssMCB.adcDataSourceCfg.source == 1)
            {
            ClockP_usleep(1000); //1ms sleep
            }

        } /* end of chirp loop */
        SemaphoreP_pend(&gMmwMssMCB.adcFileTaskSemHandle, SystemP_WAIT_FOREVER);
        
        if (gMmwMssMCB.adcDataSourceCfg.source == 1)
        {
            /* Write out Detection Matrix */
            fwrite(detMatrixData, sizeof(uint16_t), gMmwMssMCB.numDopplerBins * testConfig.numRangeBins, fileIdDetMatData);

            //Write out point cloud
            fwrite(&frameCnt, sizeof(uint16_t), 1, fileIdPointCloudIndData);
            fwrite(&result->numObjOut, sizeof(uint16_t), 1, fileIdPointCloudIndData);
            if(result->numObjOut > 0)
            {
                int objInd;
                for (objInd = 0; objInd < result->numObjOut; objInd++)
                {
                    fwrite(&result->objOut[objInd].x, sizeof(uint32_t), 1, fileIdPointCloudIndData);
                    fwrite(&result->objOut[objInd].y, sizeof(uint32_t), 1, fileIdPointCloudIndData);
                    fwrite(&result->objOut[objInd].z, sizeof(uint32_t), 1, fileIdPointCloudIndData);
                    fwrite(&result->objOut[objInd].velocity, sizeof(uint32_t), 1, fileIdPointCloudIndData);
                    fwrite(&result->objOutSideInfo[objInd].snr, sizeof(uint16_t), 1, fileIdPointCloudIndData);
                    fwrite(&result->objOutSideInfo[objInd].noise, sizeof(uint16_t), 1, fileIdPointCloudIndData);
                }
            }
        }

        printf("ADC file read task: Processed frame number %d\n", frameCnt);

    } /* end of frame loop */

    if (gMmwMssMCB.adcDataSourceCfg.source == 1)
    {
        fclose(fileIdAdcData);
        fclose(fileIdDetMatData);
        fclose(fileIdPointCloudIndData);
    }

    /* check the result */
    DebugP_log("Test finished!\n\r");
    DebugP_log("\n... DPC Finished, Check Output data ....  : \n\n");

}