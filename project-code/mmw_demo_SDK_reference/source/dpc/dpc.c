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
#include <drivers/power.h>
#include <drivers/prcm.h>
#include <drivers/hwa.h>
#include <drivers/edma.h>
#include <utils/mathutils/mathutils.h>

#include <source/mmw_cli.h>
#include <source/mmwave_demo.h>
#include <source/mmw_res.h>
#include <source/dpc/dpc.h>
#include <source/mmwave_control/interrupts.h>
#include <source/calibrations/range_phase_bias_measurement.h>
#include <source/lvds_streaming/mmw_lvds_stream.h>

#include <ti_drivers_config.h>
#include <ti_drivers_open_close.h>
#include <ti_board_open_close.h>
#include <ti_board_config.h>





/**************************************************************************
 *************************** Macros Definitions ***************************
 **************************************************************************/


#define HWA_MAX_NUM_DMA_TRIG_CHANNELS 16
#define MAX_NUM_DETECTIONS          (MMWDEMO_OUTPUT_POINT_CLOUD_LIST_MAX_SIZE)

#define LOW_PWR_MODE_DISABLE (0)
#define LOW_PWR_MODE_ENABLE (1)

#define FRAME_REF_TIMER_CLOCK_MHZ  40

#define DPC_DPU_DOPPLERPROC_FFT_WINDOW_TYPE MATHUTILS_WIN_HANNING
#define DPC_OBJDET_QFORMAT_DOPPLER_FFT 17

#define MMWDEMO_RFPARSER_SPEED_OF_LIGHT_IN_METERS_PER_SEC (3e8)

#define DPC_DPU_RANGEPROC_FFT_WINDOW_TYPE MATHUTILS_WIN_BLACKMAN
#define DPC_OBJDET_QFORMAT_RANGE_FFT 17
#define MMW_DEMO_TEST_ADC_BUFF_SIZE 1024

#define DPC_OBJDET_POINT_CLOUD_CARTESIAN_BYTE_ALIGNMENT       (MAX(DPU_AOAPROCHWA_POINT_CLOUD_CARTESIAN_BYTE_ALIGNMENT, \
                                                                   DPIF_POINT_CLOUD_CARTESIAN_CPU_BYTE_ALIGNMENT))

#define DPC_OBJDET_POINT_CLOUD_SIDE_INFO_BYTE_ALIGNMENT       (MAX(DPU_AOAPROCHWA_POINT_CLOUD_SIDE_INFO_BYTE_ALIGNMENT, \
                                                                   DPIF_POINT_CLOUD_SIDE_INFO_CPU_BYTE_ALIGNMENT))

#define DPC_OBJDET_DET_OBJ_ELEVATION_ANGLE_BYTE_ALIGNMENT     (MAX(DPU_AOAPROCHWA_DET_OBJ_ELEVATION_ANGLE_BYTE_ALIGNMENT, \
                                                                   sizeof(float)))                                                                                                                           

/**************************************************************************
 *************************** Extern Definitions ***************************
 **************************************************************************/

extern MmwDemo_MSS_MCB gMmwMssMCB;
extern HWA_Handle gHwaHandle;

/*! L3 RAM Buffer to store the radar cube and other processed signals*/
extern uint8_t gMmwL3[L3_MEM_SIZE]  __attribute((section(".bss.l3")));

/*! Local RAM buffer for object detection DPC */
#define MMWDEMO_OBJDET_CORE_LOCAL_MEM_SIZE ((8U+6U+4U+2U+8U) * 1024U)
extern uint8_t gMmwCoreLocMem[MMWDEMO_OBJDET_CORE_LOCAL_MEM_SIZE];

/* User defined heap memory and handle */
#define MMWDEMO_OBJDET_CORE_LOCAL_MEM3_SIZE  (2*1024u)
extern uint8_t gMmwCoreLocMem3[MMWDEMO_OBJDET_CORE_LOCAL_MEM3_SIZE] __attribute__((aligned(HeapP_BYTE_ALIGNMENT)));

// LED config
extern uint32_t gSPIHostIntrBaseAddrLed, gSPIHostIntrPinNumLed;
extern MMWave_temperatureStats  gTempStats;

extern int32_t MmwDemo_registerFrameStartInterrupt(void);
extern int32_t MmwDemo_registerChirpAvailableInterrupts(void);
extern int32_t  MmwDemo_rangeBiasRxChPhaseMeasureConfig ();
extern void MmwDemo_rangeBiasRxChPhaseMeasure ();
extern int32_t MmwDemo_configAndEnableApll(float apllFreqMHz, uint8_t saveRestoreCalData);

/**************************************************************************
 *************************** Global Definitions ***************************
 **************************************************************************/

/*! @brief     DPU Configuraiton Objects */
DPU_RangeProcHWA_Config     gRangeProcDpuCfg;
DPU_DopplerProcHWA_Config   gDopplerProcDpuCfg;
DPU_CFARProcHWA_Config      gCfarProcDpuCfg;
DPU_AoAProcHWA_Config       gAoa2dProcDpuCfg;

/*! @brief     EDMA interrupt objects for DPUs */
Edma_IntrObject             gEdmaIntrObjRng;
Edma_IntrObject             gEdmaIntrObjDoppler;

volatile unsigned long long test;

/**
*  @b Description
*  @n
*        Function configuring and executing DPC
*/
void MmwDemo_dpcTask();

/**
 *  @b Description
 *  @n
 *      Allocates Shawdow paramset
 */
static void DPC_ObjDet_AllocateEDMAShadowChannel(uint32_t *param)
{
    int32_t             testStatus = SystemP_SUCCESS;

    testStatus = EDMA_allocParam(gEdmaHandle[0], param);
    DebugP_assert(testStatus == SystemP_SUCCESS);

    return;
}

/**
 *  @b Description
 *  @n
 *      The function allocates HWA DMA source channel from the pool
 *
 *  @param[in]  pool Handle to pool object.
 *
 *  @retval
 *      channel Allocated HWA trigger source channel
 */
uint8_t DPC_ObjDet_HwaDmaTrigSrcChanPoolAlloc(HwaDmaTrigChanPoolObj *pool)
{
    uint8_t channel = 0xFF;
    if(pool->dmaTrigSrcNextChan < HWA_MAX_NUM_DMA_TRIG_CHANNELS)
    {
        channel = pool->dmaTrigSrcNextChan;
        pool->dmaTrigSrcNextChan++;
    }
    return channel;
}

/**
 *  @b Description
 *  @n
 *      The function resets HWA DMA source channel pool
 *
 *  @param[in]  pool Handle to pool object.
 *
 *  @retval
 *      none
 */
void DPC_ObjDet_HwaDmaTrigSrcChanPoolReset(HwaDmaTrigChanPoolObj *pool)
{
    pool->dmaTrigSrcNextChan = 0;
}

/**
 *  @b Description
 *  @n
 *      The function allocates memory in HWA RAM memory pool
 *
 *  @param[in]  pool Handle to pool object.
 *
 *  @retval
 *      startSampleIndex sample index in the HWA RAM memory
 */
int16_t DPC_ObjDet_HwaWinRamMemoryPoolAlloc(HwaWinRamMemoryPoolObj *pool, uint16_t numSamples)
{
    int16_t startSampleIndex = -1;
    if((pool->memStartSampleIndex + numSamples) < (CSL_DSS_HWA_WINDOW_RAM_U_SIZE/sizeof(uint32_t)))
    {
        startSampleIndex = pool->memStartSampleIndex;
        pool->memStartSampleIndex += numSamples;
    }
    return startSampleIndex;
}

/**
 *  @b Description
 *  @n
 *      The function resets HWA DMA source channel pool
 *
 *  @param[in]  pool Handle to pool object.
 *
 *  @retval
 *      none
 */
void DPC_ObjDet_HwaWinRamMemoryPoolReset(HwaWinRamMemoryPoolObj *pool)
{
    pool->memStartSampleIndex = 0;
}

/**
 *  @b Description
 *  @n
 *      Utility function for reseting memory pool.
 *
 *  @param[in]  pool Handle to pool object.
 *
 *  \ingroup DPC_OBJDET__INTERNAL_FUNCTION
 *
 *  @retval
 *      none.
 */
void DPC_ObjDet_MemPoolReset(MemPoolObj *pool)
{
    pool->currAddr = (uintptr_t)pool->cfg.addr;
    pool->maxCurrAddr = pool->currAddr;
}


/**
 *  @b Description
 *  @n
 *      Utility function for setting memory pool to desired address in the pool.
 *      Helps to rewind for example.
 *
 *  @param[in]  pool Handle to pool object.
 *  @param[in]  addr Address to assign to the pool's current address.
 *
 *  \ingroup DPC_OBJDET__INTERNAL_FUNCTION
 *
 *  @retval
 *      None
 */
void DPC_ObjDet_MemPoolSet(MemPoolObj *pool, void *addr)
{
    pool->currAddr = (uintptr_t)addr;
    pool->maxCurrAddr = MAX(pool->currAddr, pool->maxCurrAddr);
}

/**
 *  @b Description
 *  @n
 *      Utility function for getting memory pool current address.
 *
 *  @param[in]  pool Handle to pool object.
 *
 *  \ingroup DPC_OBJDET__INTERNAL_FUNCTION
 *
 *  @retval
 *      pointer to current address of the pool (from which next allocation will
 *      allocate to the desired alignment).
 */
void *DPC_ObjDet_MemPoolGet(MemPoolObj *pool)
{
    return((void *)pool->currAddr);
}

/**
 *  @b Description
 *  @n
 *      Utility function for getting maximum memory pool usage.
 *
 *  @param[in]  pool Handle to pool object.
 *
 *  \ingroup DPC_OBJDET__INTERNAL_FUNCTION
 *
 *  @retval
 *      Amount of pool used in bytes.
 */
uint32_t DPC_ObjDet_MemPoolGetMaxUsage(MemPoolObj *pool)
{
    return((uint32_t)(pool->maxCurrAddr - (uintptr_t)pool->cfg.addr));
}

/**
 *  @b Description
 *  @n
 *      Utility function for allocating from a static memory pool.
 *
 *  @param[in]  pool Handle to pool object.
 *  @param[in]  size Size in bytes to be allocated.
 *  @param[in]  align Alignment in bytes
 *
 *  \ingroup DPC_OBJDET__INTERNAL_FUNCTION
 *
 *  @retval
 *      pointer to beginning of allocated block. NULL indicates could not
 *      allocate.
 */
void *DPC_ObjDet_MemPoolAlloc(MemPoolObj *pool,
                              uint32_t size,
                              uint8_t align)
{
    void *retAddr = NULL;
    uintptr_t addr;

    addr = MEM_ALIGN(pool->currAddr, align);
    if ((addr + size) <= ((uintptr_t)pool->cfg.addr + pool->cfg.size))
    {
        retAddr = (void *)addr;
        pool->currAddr = addr + size;
        pool->maxCurrAddr = MAX(pool->currAddr, pool->maxCurrAddr);
    }

    return(retAddr);
}

/**
*  @b Description
*  @n
*    Select coordinates of active virtual antennas and calculate the size of the 2D virtual antenna pattern,
*    i.e. number of antenna rows and number of antenna columns.
*/
void MmwDemo_calcActiveAntennaGeometry()
{
    int32_t txInd, rxInd, ind;
    int32_t rowMax, colMax;
    int32_t rowMin, colMin;
    /* Select only active antennas */
    ind = 0;
    for (txInd = 0; txInd < gMmwMssMCB.numTxAntennas; txInd++)
    {
        for (rxInd = 0; rxInd < gMmwMssMCB.numRxAntennas; rxInd++)
        {
            gMmwMssMCB.activeAntennaGeometryCfg.ant[ind] = gMmwMssMCB.antennaGeometryCfg.ant[gMmwMssMCB.rxAntOrder[rxInd] + (gMmwMssMCB.txAntOrder[txInd] * SYS_COMMON_NUM_RX_CHANNEL)];
            ind++;
        }
    }

    /* Calculate virtual antenna 2D array size */
    ind = 0;
    rowMax = 0;
    colMax = 0;
    rowMin = 127;
    colMin = 127;
    for (txInd = 0; txInd < gMmwMssMCB.numTxAntennas; txInd++)
    {
        for (rxInd = 0; rxInd < gMmwMssMCB.numRxAntennas; rxInd++)
        {
            if (gMmwMssMCB.activeAntennaGeometryCfg.ant[ind].row > rowMax)
            {
                rowMax = gMmwMssMCB.activeAntennaGeometryCfg.ant[ind].row;
            }
            if (gMmwMssMCB.activeAntennaGeometryCfg.ant[ind].col > colMax)
            {
                colMax = gMmwMssMCB.activeAntennaGeometryCfg.ant[ind].col;
            }
            if (gMmwMssMCB.activeAntennaGeometryCfg.ant[ind].row < rowMin)
            {
                rowMin = gMmwMssMCB.activeAntennaGeometryCfg.ant[ind].row;
            }
            if (gMmwMssMCB.activeAntennaGeometryCfg.ant[ind].col < colMin)
            {
                colMin = gMmwMssMCB.activeAntennaGeometryCfg.ant[ind].col;
            }
            ind++;
        }
    }
    ind = 0;
    for (txInd = 0; txInd < gMmwMssMCB.numTxAntennas; txInd++)
    {
        for (rxInd = 0; rxInd < gMmwMssMCB.numRxAntennas; rxInd++)
        {
            gMmwMssMCB.activeAntennaGeometryCfg.ant[ind].row -= rowMin;
            gMmwMssMCB.activeAntennaGeometryCfg.ant[ind].col -= colMin;
            ind++;
        }
    }
    gMmwMssMCB.numAntRow = rowMax - rowMin + 1;
    gMmwMssMCB.numAntCol = colMax - colMin + 1;
}

/**
 *  @b Description
 *  @n
 *     Compress point cloud list which is transferred to the Host via UART.
 *     Floating point values are converted to int16
 *
 * @param[out] pointCloudOut        Compressed point cloud list
 * @param[in]  pointCloudUintRecip  Scales used for conversion from float values to integer value
 * @param[in]  pointCloudIn         Input point cloud list, generated by CFAR DPU
 * @param[in]  numPoints            Number of points in the point cloud list
 *
 *  @retval
 *      Not Applicable.
 */
void MmwDemo_compressPointCloudList(MmwDemo_output_message_UARTpointCloud *pointCloudOut,
                                    MmwDemo_output_message_point_unit *pointCloudUintRecip,
                                    DPIF_PointCloudCartesianExt *pointCloudIn,
                                    uint32_t numPoints)
{
    uint32_t i;
    float xyzUnitScale = pointCloudUintRecip->xyzUnit;
    float dopplerScale = pointCloudUintRecip->dopplerUnit;
    float snrScale = pointCloudUintRecip->snrUint;
    float noiseScale = pointCloudUintRecip->noiseUint;
    uint32_t tempVal;

    for (i = 0; i < numPoints; i++)
    {
        pointCloudOut->point[i].x = (int16_t) roundf(pointCloudIn[i].x * xyzUnitScale); //saturate the values
        pointCloudOut->point[i].y = (int16_t) roundf(pointCloudIn[i].y * xyzUnitScale);
        pointCloudOut->point[i].z = (int16_t) roundf(pointCloudIn[i].z * xyzUnitScale);
        pointCloudOut->point[i].doppler = (int16_t) roundf(pointCloudIn[i].velocity * dopplerScale);
        tempVal = (uint32_t) roundf(pointCloudIn[i].snr * snrScale);
        if (tempVal > 255)
        {
            tempVal = 255;
        }
        pointCloudOut->point[i].snr = (uint8_t) tempVal;
        tempVal = (uint32_t) roundf(pointCloudIn[i].noise * noiseScale);
        if (tempVal > 255)
        {
            tempVal = 255;
        }
        pointCloudOut->point[i].noise = (uint8_t) tempVal;
    }
}

/*  @b Description
*  @n
*    Range processing DPU Initialization
*/
void DPC_ObjDet_RngDpuInit()
{
    int32_t errorCode = 0;
    DPU_RangeProcHWA_InitParams initParams;
    initParams.hwaHandle = gHwaHandle;

    /* generate the dpu handler*/
    gMmwMssMCB.rangeProcDpuHandle = DPU_RangeProcHWA_init(&initParams, &errorCode);
    if (gMmwMssMCB.rangeProcDpuHandle == NULL)
    {
        CLI_write("Error: RangeProc DPU initialization returned error %d\n", errorCode);
        DebugP_assert(0);
        return;
    }
}

/*  @b Description
*  @n
*    Doppler processing DPU Initialization
*/
void DPC_ObjDet_DopplerDpuInit()
{
    int32_t  errorCode = 0;
    DPU_DopplerProcHWA_InitParams initParams;
    initParams.hwaHandle =  gHwaHandle;
    /* generate the dpu handler*/
    gMmwMssMCB.dopplerProcDpuHandle =  DPU_DopplerProcHWA_init(&initParams, &errorCode);
    if (gMmwMssMCB.dopplerProcDpuHandle == NULL)
    {
        CLI_write ("Error: Doppler Proc DPU initialization returned error %d\n", errorCode);
        DebugP_assert (0);
        return;
    }
}

/**
*  @b Description
*  @n
*    CFAR DPU Initialization
*/
void DPC_ObjDet_CfarDpuInit()
{
    int32_t  errorCode = 0;
    DPU_CFARProcHWA_InitParams initParams;
    initParams.hwaHandle =  gHwaHandle;
    /* generate the dpu handler*/
    gMmwMssMCB.cfarProcDpuHandle =  DPU_CFARProcHWA_init(&initParams, &errorCode);
    if (gMmwMssMCB.cfarProcDpuHandle == NULL)
    {
        CLI_write ("Error: CFAR Proc DPU initialization returned error %d\n", errorCode);
        DebugP_assert (0);
        return;
    }
}

/**
*  @b Description
*  @n
*    AOA2D DPU Initialization
*/
void DPC_ObjDet_AoaDpuInit()
{
    int32_t  errorCode = 0;
    DPU_AoAProcHWA_InitParams initParams;
    initParams.hwaHandle =  gHwaHandle;
    /* generate the dpu handler*/
    gMmwMssMCB.aoa2dProcDpuHandle =  DPU_AoAProcHWA_init(&initParams, &errorCode);
    if (gMmwMssMCB.aoa2dProcDpuHandle == NULL)
    {
        CLI_write ("Error: AOA2D Proc DPU initialization returned error %d\n", errorCode);
        DebugP_assert (0);
        return;
    }
}

/**
*  @b Description
*  @n
*    Based on the configuration, set up the range processing DPU configurations
*/
int32_t DPC_ObjDet_RngDpuCfg_Parser()
{
    int32_t retVal = 0;
    DPU_RangeProcHWA_HW_Resources  *pHwConfig;
    DPU_RangeProcHWA_StaticConfig  *params;
    uint32_t index;
    uint32_t bytesPerRxChan;
    uint16_t numBytesPerInputSample;
    uint32_t dmaCh, tcc, param;

    memset((void *)&gRangeProcDpuCfg, 0, sizeof(DPU_RangeProcHWA_Config));

    pHwConfig = &gRangeProcDpuCfg.hwRes;
    params = &gRangeProcDpuCfg.staticCfg;

    /****************** Static configurations ******************/
    params->numTxAntennas = gMmwMssMCB.numTxAntennas;
    params->numVirtualAntennas = gMmwMssMCB.numTxAntennas * gMmwMssMCB.numRxAntennas;
    params->numChirpsPerFrame = gMmwMssMCB.mmWaveCfg.frameCfg.numOfBurstsInFrame * gMmwMssMCB.mmWaveCfg.frameCfg.numOfChirpsInBurst;
    params->isChirpDataReal = 1; /*This device supports only real ADC data*/
    
    numBytesPerInputSample = sizeof(int16_t);
    params->numRangeBins = gMmwMssMCB.numRangeBins;
    params->numFFTBins = mathUtils_pow2roundup(gMmwMssMCB.mmWaveCfg.profileComCfg.numOfAdcSamples);
    
    /* windowing */
    params->windowSize = sizeof(uint32_t) * ((gMmwMssMCB.mmWaveCfg.profileComCfg.numOfAdcSamples + 1) / 2); 
    params->window =  (int32_t *)DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.CoreLocalRamObj,
                                                         params->windowSize,
                                                         sizeof(uint32_t));
    if (params->window == NULL)
    {
        retVal = DPC_OBJECTDETECTION_ENOMEM__CORE_LOCAL_RAM_RANGE_HWA_WINDOW;
        goto exit;
    }
 
    params->ADCBufData.dataProperty.numAdcSamples = gMmwMssMCB.mmWaveCfg.profileComCfg.numOfAdcSamples;
    params->ADCBufData.dataProperty.numRxAntennas = gMmwMssMCB.numRxAntennas;

    if (!gMmwMssMCB.oneTimeConfigDone)
    {
        mathUtils_genWindow((uint32_t *)params->window,
                            (uint32_t) params->ADCBufData.dataProperty.numAdcSamples,
                            params->windowSize/sizeof(uint32_t),
                            DPC_DPU_RANGEPROC_FFT_WINDOW_TYPE,
                            DPC_OBJDET_QFORMAT_RANGE_FFT);
    }

    params->rangeFFTtuning.fftOutputDivShift = 2;
    params->rangeFFTtuning.numLastButterflyStagesToScale = 0; /* no scaling needed as ADC is 16-bit and we have 8 bits to grow */

    bytesPerRxChan = gMmwMssMCB.mmWaveCfg.profileComCfg.numOfAdcSamples * numBytesPerInputSample;
    bytesPerRxChan = (bytesPerRxChan + 15) / 16 * 16;

    for (index = 0; index < SYS_COMMON_NUM_RX_CHANNEL; index++)
    {
        params->ADCBufData.dataProperty.rxChanOffset[index] = index * bytesPerRxChan;
    }

    params->ADCBufData.dataProperty.interleave = DPIF_RXCHAN_NON_INTERLEAVE_MODE;
    
    /* adc buffer buffer, format fixed, interleave, size will change */
    params->ADCBufData.dataProperty.dataFmt = DPIF_DATAFORMAT_REAL16;

    params->ADCBufData.dataProperty.adcBits = 2U;
    params->ADCBufData.dataProperty.numChirpsPerChirpEvent = 1U;

    if(gMmwMssMCB.adcDataSourceCfg.source == 0)
    {
        params->ADCBufData.data = (void *)CSL_DSS_ADCBUF_READ_U_BASE;
    }
    else
    {
        gMmwMssMCB.adcTestBuff  = (uint8_t *) DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.L3RamObj,
                                                                            MMW_DEMO_TEST_ADC_BUFF_SIZE,
                                                                            sizeof(uint32_t));
        if(gMmwMssMCB.adcTestBuff == NULL)
        {
            retVal = DPC_OBJECTDETECTION_ENOMEM__L3_RAM_ADC_TEST_BUFF;
            goto exit;
        }
        params->ADCBufData.data = (void *)gMmwMssMCB.adcTestBuff;

    }

    /****************** Dynamic or HW resource configurations ******************/
    pHwConfig->intrObj = &gEdmaIntrObjRng;
    /* HWA configurations, not related to per test, common to all test */
    pHwConfig->hwaCfg.paramSetStartIdx = gMmwMssMCB.numUsedHwaParamSets;
    pHwConfig->hwaCfg.numParamSet = DPU_RANGEPROCHWA_NUM_HWA_PARAM_SETS;
    pHwConfig->hwaCfg.hwaWinRamOffset  = DPC_ObjDet_HwaWinRamMemoryPoolAlloc(&gMmwMssMCB.HwaWinRamMemoryPoolObj,
                                                                               mathUtils_pow2roundup(gMmwMssMCB.mmWaveCfg.profileComCfg.numOfAdcSamples)/2);
    pHwConfig->hwaCfg.hwaWinSym = 1;
    pHwConfig->hwaCfg.dataInputMode = DPU_RangeProcHWA_InputMode_ISOLATED;

    /* edma configuration */
    pHwConfig->edmaHandle  = gEdmaHandle[0];

    /* Data Input EDMA */
    dmaCh = EDMA_DSS_TPCC_A_CHIRP_AVAIL_IRQ;
    tcc   = EDMA_DSS_TPCC_A_CHIRP_AVAIL_IRQ;
    param = EDMA_DSS_TPCC_A_CHIRP_AVAIL_IRQ;
    DPEDMA_allocateEDMAChannel(gEdmaHandle[0], &dmaCh, &tcc, &param);
    pHwConfig->edmaInCfg.dataIn.channel         = dmaCh;
    pHwConfig->edmaInCfg.dataIn.paramId         = param;
    pHwConfig->edmaInCfg.dataIn.tcc             = tcc;

    param = DPC_OBJDET_DPU_RANGEPROC_EDMA_1DIN_SHADOW_LINK_CH_ID;
    DPC_ObjDet_AllocateEDMAShadowChannel(&param);
    pHwConfig->edmaInCfg.dataIn.shadowPramId   = param;
    pHwConfig->edmaInCfg.dataIn.eventQueue      = 0;

    dmaCh = DPC_OBJDET_DPU_RANGEPROC_EDMA_1DINSIGNATURE_CH_ID;
    tcc   = DPC_OBJDET_DPU_RANGEPROC_EDMA_1DINSIGNATURE_CH_ID;
    param = DPC_OBJDET_DPU_RANGEPROC_EDMA_1DINSIGNATURE_CH_ID;
    DPEDMA_allocateEDMAChannel(gEdmaHandle[0], &dmaCh, &tcc, &param);
    pHwConfig->edmaInCfg.dataInSignature.channel         = dmaCh;
    pHwConfig->edmaInCfg.dataInSignature.paramId         = param;
    pHwConfig->edmaInCfg.dataInSignature.tcc             = tcc;

    param = DPC_OBJDET_DPU_RANGEPROC_EDMA_1DINSIGNATURE_PING_SHADOW_LINK_CH_ID;
    DPC_ObjDet_AllocateEDMAShadowChannel(&param);
    pHwConfig->edmaInCfg.dataInSignature.shadowPramId   = param;
    pHwConfig->edmaInCfg.dataInSignature.eventQueue      = 0;

    /* Output Ping*/
    dmaCh = DPC_OBJDET_DPU_RANGEPROC_EDMA_1D_PING_CH_ID;
    tcc   = DPC_OBJDET_DPU_RANGEPROC_EDMA_1D_PING_CH_ID;
    param = DPC_OBJDET_DPU_RANGEPROC_EDMA_1D_PING_CH_ID;
    DPEDMA_allocateEDMAChannel(gEdmaHandle[0], &dmaCh, &tcc, &param);
    pHwConfig->edmaOutCfg.u.fmt2.dataOutPing.channel         = dmaCh;
    pHwConfig->edmaOutCfg.u.fmt2.dataOutPing.paramId         = param;
    pHwConfig->edmaOutCfg.u.fmt2.dataOutPing.tcc             = tcc;

    param = DPC_OBJDET_DPU_RANGEPROC_EDMA_1D_PING_SHADOW_LINK_CH_ID;
    DPC_ObjDet_AllocateEDMAShadowChannel(&param);
    pHwConfig->edmaOutCfg.u.fmt2.dataOutPing.shadowPramId   = param;
    pHwConfig->edmaOutCfg.u.fmt2.dataOutPing.eventQueue= 0;

    /* Output Pong*/
    dmaCh = DPC_OBJDET_DPU_RANGEPROC_EDMA_1D_PONG_CH_ID;
    tcc   = DPC_OBJDET_DPU_RANGEPROC_EDMA_1D_PONG_CH_ID;
    param = DPC_OBJDET_DPU_RANGEPROC_EDMA_1D_PONG_CH_ID;
    DPEDMA_allocateEDMAChannel(gEdmaHandle[0], &dmaCh, &tcc, &param);
    pHwConfig->edmaOutCfg.u.fmt2.dataOutPong.channel         = dmaCh;
    pHwConfig->edmaOutCfg.u.fmt2.dataOutPong.paramId         = param;
    pHwConfig->edmaOutCfg.u.fmt2.dataOutPong.tcc             = tcc;

    param = DPC_OBJDET_DPU_RANGEPROC_EDMA_1D_PONG_SHADOW_LINK_CH_ID;
    DPC_ObjDet_AllocateEDMAShadowChannel(&param);
    pHwConfig->edmaOutCfg.u.fmt2.dataOutPong.shadowPramId   = param;
    pHwConfig->edmaOutCfg.u.fmt2.dataOutPong.eventQueue       = 0;
        
    /* Output signature channel */
    dmaCh = DPC_OBJDET_DPU_RANGEPROC_EDMA_1D_PING_CHAIN_CH_ID;
    tcc   = DPC_OBJDET_DPU_RANGEPROC_EDMA_1D_PING_CHAIN_CH_ID;
    param = DPC_OBJDET_DPU_RANGEPROC_EDMA_1D_PING_CHAIN_CH_ID;
    DPEDMA_allocateEDMAChannel(gEdmaHandle[0], &dmaCh, &tcc, &param);
    pHwConfig->edmaOutCfg.dataOutSignature.channel  = dmaCh;
    pHwConfig->edmaOutCfg.dataOutSignature.paramId  = param;
    pHwConfig->edmaOutCfg.dataOutSignature.tcc      = tcc;

    param = DPC_OBJDET_DPU_RANGEPROC_EDMA_1D_PING_ONE_HOT_SHADOW_LINK_CH_ID;
    DPC_ObjDet_AllocateEDMAShadowChannel(&param);
    pHwConfig->edmaOutCfg.dataOutSignature.shadowPramId = param;
    pHwConfig->edmaOutCfg.dataOutSignature.eventQueue = 0;

    /* radar cube */
    pHwConfig->radarCube.dataSize = params->numRangeBins * gMmwMssMCB.numRxAntennas * sizeof(uint32_t) * params->numChirpsPerFrame;
    pHwConfig->radarCube.datafmt = DPIF_RADARCUBE_FORMAT_2; /*Fmt7 for increased chirps processing is not supported by DPC and is currently only a DPU level demonstration*/

    gMmwMssMCB.radarCube  = (cmplx16ImRe_t *) DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.L3RamObj,
                                                                               pHwConfig->radarCube.dataSize,
                                                                               sizeof(uint32_t));
    if(gMmwMssMCB.radarCube == NULL)
    {
        retVal = DPC_OBJECTDETECTION_ENOMEM__L3_RAM_RADAR_CUBE;
        goto exit;
    }

    pHwConfig->radarCube.data  = (cmplx16ImRe_t *) gMmwMssMCB.radarCube;    

exit:
    return retVal;
}

/**
*  @b Description
*  @n
*    Based on the configuration, set up the Doppler processing DPU configurations
*/
int32_t DPC_ObjDet_DopplerDpuCfg_Parser()
{   
    uint32_t retVal = 0;
    DPU_DopplerProcHWA_HW_Resources  *pHwConfig;
    DPU_DopplerProcHWA_StaticConfig  *params;
    uint32_t dmaCh, tcc, param;

    memset((void *)&gDopplerProcDpuCfg, 0, sizeof(DPU_DopplerProcHWA_Config));
    pHwConfig = &gDopplerProcDpuCfg.hwRes; 
    params = &gDopplerProcDpuCfg.staticCfg;
    
    /* Static configurations */
    params->numTxAntennas = gMmwMssMCB.numTxAntennas;
    params->numRxAntennas = gMmwMssMCB.numRxAntennas;
    params->numVirtualAntennas = gMmwMssMCB.numTxAntennas * gMmwMssMCB.numRxAntennas;
    params->numRangeBins = gMmwMssMCB.numRangeBins;
    params->numDopplerChirps = (gMmwMssMCB.mmWaveCfg.frameCfg.numOfBurstsInFrame * gMmwMssMCB.mmWaveCfg.frameCfg.numOfChirpsInBurst)/params->numTxAntennas;
    params->numDopplerBins = mathUtils_pow2roundup(params->numDopplerChirps);
    params->numPingPongPath = 2; /*Option1 for increased chirps processing is not supported by DPC and is currently only a DPU level demonstration*/
    params->isDetMatrixLogScale = 1; /*Currently DPC supports only log scale detection matrix*/
    params->log2NumDopplerBins = mathUtils_ceilLog2(params->numDopplerBins);

    /* clutter removal is implemented by zeroing the first Doppler bin */
    if(gMmwMssMCB.staticClutterRemovalEnable)
    {
        params->isStaticClutterRemovalEnabled = 1;
        gMmwMssMCB.numDopplerBins = params->numDopplerBins - 1;
    }
    else 
    {
        params->isStaticClutterRemovalEnabled = 0;
        gMmwMssMCB.numDopplerBins = params->numDopplerBins;
    }

    /* Dynamic or HW resources configurations */
    /* windowing */
    pHwConfig->hwaCfg.winSym = HWA_FFT_WINDOW_SYMMETRIC;
    pHwConfig->hwaCfg.winRamOffset = DPC_ObjDet_HwaWinRamMemoryPoolAlloc(&gMmwMssMCB.HwaWinRamMemoryPoolObj,
                                                                           params->numDopplerChirps/2);

    pHwConfig->hwaCfg.firstStageScaling = DPU_DOPPLERPROCHWA_FIRST_SCALING_DISABLED;

    if (pHwConfig->hwaCfg.winSym == HWA_FFT_WINDOW_NONSYMMETRIC)
    {
        pHwConfig->hwaCfg.windowSize = params->numDopplerChirps * sizeof(int32_t);
    }
    else
    {
        pHwConfig->hwaCfg.windowSize = ((params->numDopplerChirps + 1) / 2) * sizeof(int32_t);
    }

    pHwConfig->hwaCfg.window = (int32_t *)DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.CoreLocalRamObj,
                                                         pHwConfig->hwaCfg.windowSize,
                                                         sizeof(uint32_t));

    if (pHwConfig->hwaCfg.window == NULL)
    {
        retVal = DPC_OBJECTDETECTION_ENOMEM__CORE_LOCAL_RAM_DOPPLER_HWA_WINDOW;
        goto exit;
    }

    if (!gMmwMssMCB.oneTimeConfigDone)
    {
        mathUtils_genWindow((uint32_t *)pHwConfig->hwaCfg.window,
                            (uint32_t) params->numDopplerChirps,
                            pHwConfig->hwaCfg.windowSize/sizeof(uint32_t),
                            DPC_DPU_DOPPLERPROC_FFT_WINDOW_TYPE,
                            DPC_OBJDET_QFORMAT_DOPPLER_FFT);
    }

    pHwConfig->radarCube.datafmt = DPIF_RADARCUBE_FORMAT_2;

    if (pHwConfig->radarCube.datafmt == DPIF_RADARCUBE_FORMAT_2)
    {
        pHwConfig->hwaCfg.numParamSets =
                DPU_DOPPLERPROCHWA_NUM_HWA_PARAMS_FMT2(params->numPingPongPath);
    }
    else
    {
        pHwConfig->hwaCfg.numParamSets =
                DPU_DOPPLERPROCHWA_NUM_HWA_PARAMS_FMT7(params->numPingPongPath, params->numTxAntennas);
    }

    pHwConfig->edmaCfg.edmaHandle = gEdmaHandle[0];
    pHwConfig->edmaCfg.intrObj = &gEdmaIntrObjDoppler;

    dmaCh = DPC_OBJDET_DPU_DOPPLERPROC_EDMA_DOPPLERPROC_IN_PING;
    tcc   = DPC_OBJDET_DPU_DOPPLERPROC_EDMA_DOPPLERPROC_IN_PING;
    param = DPC_OBJDET_DPU_DOPPLERPROC_EDMA_DOPPLERPROC_IN_PING;
    DPEDMA_allocateEDMAChannel(gEdmaHandle[0], &dmaCh, &tcc, &param);
    pHwConfig->edmaCfg.edmaIn.pingPong[0].channel = dmaCh;
    pHwConfig->edmaCfg.edmaIn.pingPong[0].paramId = param;
    pHwConfig->edmaCfg.edmaIn.pingPong[0].tcc     = tcc;

    param = DPC_OBJDET_DPU_DOPPLERPROC_EDMA_DOPPLERPROC_IN_PING_SHADOW;
    DPC_ObjDet_AllocateEDMAShadowChannel(&param);
    pHwConfig->edmaCfg.edmaIn.pingPong[0].shadowPramId = param;
    pHwConfig->edmaCfg.edmaIn.pingPong[0].eventQueue = 0;


    dmaCh = DPC_OBJDET_DPU_DOPPLERPROC_EDMA_DOPPLERPROC_IN_PONG;
    tcc   = DPC_OBJDET_DPU_DOPPLERPROC_EDMA_DOPPLERPROC_IN_PONG;
    param = DPC_OBJDET_DPU_DOPPLERPROC_EDMA_DOPPLERPROC_IN_PONG;
    DPEDMA_allocateEDMAChannel(gEdmaHandle[0], &dmaCh, &tcc, &param);
    pHwConfig->edmaCfg.edmaIn.pingPong[1].channel = dmaCh;
    pHwConfig->edmaCfg.edmaIn.pingPong[1].paramId = param;
    pHwConfig->edmaCfg.edmaIn.pingPong[1].tcc     = tcc;

    param = DPC_OBJDET_DPU_DOPPLERPROC_EDMA_DOPPLERPROC_IN_PONG_SHADOW;
    DPC_ObjDet_AllocateEDMAShadowChannel(&param);
    pHwConfig->edmaCfg.edmaIn.pingPong[1].shadowPramId = param;
    pHwConfig->edmaCfg.edmaIn.pingPong[1].eventQueue = 0;

    dmaCh = DPC_OBJDET_DPU_DOPPLERPROC_EDMA_DOPPLERPROC_OUT_PING;
    tcc   = DPC_OBJDET_DPU_DOPPLERPROC_EDMA_DOPPLERPROC_OUT_PING;
    param = DPC_OBJDET_DPU_DOPPLERPROC_EDMA_DOPPLERPROC_OUT_PING;
    DPEDMA_allocateEDMAChannel(gEdmaHandle[0], &dmaCh, &tcc, &param);
    pHwConfig->edmaCfg.edmaOut.pingPong[0].channel = dmaCh;
    pHwConfig->edmaCfg.edmaOut.pingPong[0].paramId = param;
    pHwConfig->edmaCfg.edmaOut.pingPong[0].tcc     = tcc;

    param = DPC_OBJDET_DPU_DOPPLERPROC_EDMA_DOPPLERPROC_OUT_PING_SHADOW;
    DPC_ObjDet_AllocateEDMAShadowChannel(&param);
    pHwConfig->edmaCfg.edmaOut.pingPong[0].shadowPramId = param;
    pHwConfig->edmaCfg.edmaOut.pingPong[0].eventQueue = 0;

    dmaCh = DPC_OBJDET_DPU_DOPPLERPROC_EDMA_DOPPLERPROC_OUT_PONG;
    tcc   = DPC_OBJDET_DPU_DOPPLERPROC_EDMA_DOPPLERPROC_OUT_PONG;
    param = DPC_OBJDET_DPU_DOPPLERPROC_EDMA_DOPPLERPROC_OUT_PONG;
    DPEDMA_allocateEDMAChannel(gEdmaHandle[0], &dmaCh, &tcc, &param);
    pHwConfig->edmaCfg.edmaOut.pingPong[1].channel = dmaCh;
    pHwConfig->edmaCfg.edmaOut.pingPong[1].paramId = param;
    pHwConfig->edmaCfg.edmaOut.pingPong[1].tcc     = tcc;

    param = DPC_OBJDET_DPU_DOPPLERPROC_EDMA_DOPPLERPROC_OUT_PONG_SHADOW;
    DPC_ObjDet_AllocateEDMAShadowChannel(&param);
    pHwConfig->edmaCfg.edmaOut.pingPong[1].shadowPramId = param;
    pHwConfig->edmaCfg.edmaOut.pingPong[1].eventQueue = 0;

    dmaCh = DPC_OBJDET_DPU_DOPPLERPROC_EDMA_DOPPLERPROC_HOTSIG_PING;
    tcc   = DPC_OBJDET_DPU_DOPPLERPROC_EDMA_DOPPLERPROC_HOTSIG_PING;
    param = DPC_OBJDET_DPU_DOPPLERPROC_EDMA_DOPPLERPROC_HOTSIG_PING;
    DPEDMA_allocateEDMAChannel(gEdmaHandle[0], &dmaCh, &tcc, &param);
    pHwConfig->edmaCfg.edmaHotSig.pingPong[0].channel = dmaCh;
    pHwConfig->edmaCfg.edmaHotSig.pingPong[0].paramId = param;
    pHwConfig->edmaCfg.edmaHotSig.pingPong[0].tcc     = tcc;

    param = DPC_OBJDET_DPU_DOPPLERPROC_EDMA_DOPPLERPROC_HOTSIG_PING_SHADOW;
    DPC_ObjDet_AllocateEDMAShadowChannel(&param);
    pHwConfig->edmaCfg.edmaHotSig.pingPong[0].shadowPramId = param;
    pHwConfig->edmaCfg.edmaHotSig.pingPong[0].eventQueue = 0;

    dmaCh = DPC_OBJDET_DPU_DOPPLERPROC_EDMA_DOPPLERPROC_HOTSIG_PONG;
    tcc   = DPC_OBJDET_DPU_DOPPLERPROC_EDMA_DOPPLERPROC_HOTSIG_PONG;
    param = DPC_OBJDET_DPU_DOPPLERPROC_EDMA_DOPPLERPROC_HOTSIG_PONG;
    DPEDMA_allocateEDMAChannel(gEdmaHandle[0], &dmaCh, &tcc, &param);
    pHwConfig->edmaCfg.edmaHotSig.pingPong[1].channel = dmaCh;
    pHwConfig->edmaCfg.edmaHotSig.pingPong[1].paramId = param;
    pHwConfig->edmaCfg.edmaHotSig.pingPong[1].tcc     = tcc;

    param = DPC_OBJDET_DPU_DOPPLERPROC_EDMA_DOPPLERPROC_HOTSIG_PONG_SHADOW;
    DPC_ObjDet_AllocateEDMAShadowChannel(&param);
    pHwConfig->edmaCfg.edmaHotSig.pingPong[1].shadowPramId = param;
    pHwConfig->edmaCfg.edmaHotSig.pingPong[1].eventQueue = 0;
   
    pHwConfig->hwaCfg.paramSetStartIdx = gMmwMssMCB.numUsedHwaParamSets;

    /* cube input*/
    pHwConfig->radarCube.dataSize = params->numTxAntennas * params->numRangeBins * params->numDopplerBins * params->numRxAntennas * 4;
    pHwConfig->radarCube.data = (cmplx16ImRe_t *)gMmwMssMCB.radarCube;

    /* output */
    pHwConfig->detMatrix.datafmt = DPIF_DETMATRIX_FORMAT_1;
    pHwConfig->detMatrix.dataSize = params->numRangeBins * gMmwMssMCB.numDopplerBins * sizeof(uint16_t);
    
    if (params->isDetMatrixLogScale == 1)
    {
        gMmwMssMCB.detMatrix = (uint16_t *) DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.L3RamObj,
                                                            pHwConfig->detMatrix.dataSize,
                                                            sizeof(uint32_t));
    }
    else
    {
        gMmwMssMCB.detMatrix = NULL;
    }

    if (gMmwMssMCB.detMatrix == NULL)
    {
        retVal = DPC_OBJECTDETECTION_ENOMEM__L3_RAM_DET_MATRIX;
        goto exit;
    }

    pHwConfig->detMatrix.data = (uint16_t *)gMmwMssMCB.detMatrix;

exit:
    return retVal;
}

/**
*  @b Description
*  @n
*    Based on the configuration, set up the CFAR detection processing DPU configurations
*/
int32_t DPC_ObjDet_CfarDpuCfg_Parser()
{
    int32_t retVal = 0;
    float adcStart, startFreq, slope, bandwidth;
    DPU_CFARProcHWA_HW_Resources *pHwConfig;
    DPU_CFARProcHWA_StaticConfig  *params;
    DPU_CFARProcHWA_DynamicConfig *dynCfg;
    uint32_t dmaCh, tcc, param;

    memset((void *)&gCfarProcDpuCfg, 0, sizeof(DPU_CFARProcHWA_Config));
    
    /* CFARproc DPU based on Range/Doppler heatmap */
    pHwConfig = &gCfarProcDpuCfg.res;
    params = &gCfarProcDpuCfg.staticCfg;
    dynCfg = &gCfarProcDpuCfg.dynCfg;

    /* Static configurations */
    params->numDopplerBins = mathUtils_pow2roundup((gMmwMssMCB.mmWaveCfg.frameCfg.numOfBurstsInFrame * gMmwMssMCB.mmWaveCfg.frameCfg.numOfChirpsInBurst)/gMmwMssMCB.numTxAntennas);
    params->numRangeBins = gMmwMssMCB.numRangeBins;
    params->log2NumDopplerBins = mathUtils_floorLog2(params->numDopplerBins);
    
    gMmwMssMCB.adcStartTime         =   (gMmwMssMCB.mmWaveCfg.profileTimeCfg.chirpAdcStartTime >> 10) * (1/gMmwMssMCB.adcSamplingRate); //us
    adcStart                        =   (gMmwMssMCB.adcStartTime * 1.e-6);
    startFreq                       =   (float)(gMmwMssMCB.mmWaveCfg.profileTimeCfg.startFreqGHz * 1.e9);
    slope                           =   (float)(gMmwMssMCB.mmWaveCfg.profileTimeCfg.chirpSlope * 1.e12);
    bandwidth                       =   (slope * gMmwMssMCB.mmWaveCfg.profileComCfg.numOfAdcSamples)/(gMmwMssMCB.adcSamplingRate * 1.e6);
    gMmwMssMCB.centerFreq           =   startFreq + bandwidth * 0.5f + adcStart * slope;

    params->rangeStep               =   (MMWDEMO_RFPARSER_SPEED_OF_LIGHT_IN_METERS_PER_SEC * (gMmwMssMCB.adcSamplingRate * 1.e6)) /
                                        (2.f * slope * (2*params->numRangeBins));

    if (gMmwMssMCB.mmWaveCfg.frameCfg.numOfBurstsInFrame > 1)
    {
        /* Burst mode: Assumes h_NumOfBurstsInFrame > 1, h_NumOfChirpsInBurst = numTx. 
         * Below calculation may not be accurate for other combinations of h_NumOfChirpsInBurst 
         * and may need more robust technique to estimate doppler step, based on the use case */
        params->dopplerStep          =   MMWDEMO_RFPARSER_SPEED_OF_LIGHT_IN_METERS_PER_SEC /
                                            (2.f * params->numDopplerBins *
                                            gMmwMssMCB.centerFreq * (gMmwMssMCB.mmWaveCfg.frameCfg.burstPeriodus * 1e-6));
    }
    else
    {
        /* Normal mode: h_NumOfBurstsInFrame = 1, h_NumOfChirpsInBurst >= 2 */
        params->dopplerStep          =   MMWDEMO_RFPARSER_SPEED_OF_LIGHT_IN_METERS_PER_SEC /
                                            (2.f * gMmwMssMCB.mmWaveCfg.frameCfg.numOfChirpsInBurst *
                                            gMmwMssMCB.centerFreq * ((gMmwMssMCB.mmWaveCfg.profileTimeCfg.chirpIdleTimeus + gMmwMssMCB.mmWaveCfg.profileComCfg.chirpRampEndTimeus) * 1e-6));
        if(gMmwMssMCB.mmWaveCfg.frameCfg.numOfChirpsAccum != 0)
        {
            /* When numOfChirpsAccum is greater than zero, the chirping window will increase acccording to numOfChirpsAccum selected. */ 
            params->dopplerStep = params->dopplerStep/gMmwMssMCB.mmWaveCfg.frameCfg.numOfChirpsAccum;
        }
    }

    /* clutter removal is implemented by zeroing the first Doppler bin */
    if(gMmwMssMCB.staticClutterRemovalEnable)
    {
        params->numDopplerBins = params->numDopplerBins - 1;
    }
    
    dynCfg->cfarCfgDoppler = &gMmwMssMCB.cfarDopplerCfg;
    dynCfg->cfarCfgRange = &gMmwMssMCB.cfarRangeCfg;
    dynCfg->fovRange = &gMmwMssMCB.fovRange;
    dynCfg->fovDoppler = &gMmwMssMCB.fovDoppler;
    
    /* Dynamic or HW resources configurations */
    pHwConfig->edmaHandle = gEdmaHandle[CONFIG_EDMA0]; //edmaHandle;

    dmaCh = DPC_OBJDET_DPU_CFAR_PROC_EDMAIN_CH;
    tcc   = DPC_OBJDET_DPU_CFAR_PROC_EDMAIN_CH;
    param = DPC_OBJDET_DPU_CFAR_PROC_EDMAIN_CH;
    DPEDMA_allocateEDMAChannel(gEdmaHandle[CONFIG_EDMA0], &dmaCh, &tcc, &param);
    pHwConfig->edmaHwaIn.channel = dmaCh;
    pHwConfig->edmaHwaIn.paramId = param;
    pHwConfig->edmaHwaIn.tcc     = tcc;

    param = DPC_OBJDET_DPU_CFAR_PROC_EDMAIN_SHADOW;
    DPC_ObjDet_AllocateEDMAShadowChannel(&param);
    pHwConfig->edmaHwaIn.shadowPramId = param;
    pHwConfig->edmaHwaIn.eventQueue = DPC_OBJDET_DPU_CFAR_PROC_EDMAIN_EVENT_QUE;

    dmaCh = DPC_OBJDET_DPU_CFAR_PROC_EDMAIN_SIG_CH;
    tcc   = DPC_OBJDET_DPU_CFAR_PROC_EDMAIN_SIG_CH;
    param = DPC_OBJDET_DPU_CFAR_PROC_EDMAIN_SIG_CH;
    DPEDMA_allocateEDMAChannel(gEdmaHandle[CONFIG_EDMA0], &dmaCh, &tcc, &param);
    pHwConfig->edmaHwaInSignature.channel = dmaCh;
    pHwConfig->edmaHwaInSignature.paramId = param;
    pHwConfig->edmaHwaInSignature.tcc     = tcc;

    param = DPC_OBJDET_DPU_CFAR_PROC_EDMAIN_SIG_SHADOW;
    DPC_ObjDet_AllocateEDMAShadowChannel(&param);
    pHwConfig->edmaHwaInSignature.shadowPramId = param;
    pHwConfig->edmaHwaInSignature.eventQueue = DPC_OBJDET_DPU_CFAR_PROC_EDMAIN_SIG_EVENT_QUE;

    pHwConfig->hwaCfg.numParamSet = DPU_CFARPROCHWA_NUM_HWA_PARAMS;
    pHwConfig->hwaCfg.paramSetStartIdx = gMmwMssMCB.numUsedHwaParamSets; 

    pHwConfig->detMatrix.datafmt = DPIF_DETMATRIX_FORMAT_1;
    pHwConfig->detMatrix.dataSize = params->numDopplerBins * params->numRangeBins * sizeof(uint16_t); 
    pHwConfig->detMatrix.data = (uint16_t *)gMmwMssMCB.detMatrix;

    /* Give M0 and M1 memory banks for detection matrix scratch. */
    pHwConfig->hwaMemInp = (uint16_t *) CSL_DSS_HWA_DMA0_RAM_BANK0_BASE;
    pHwConfig->hwaMemInpSize = (CSL_DSS_HWA_BANK_SIZE * 2) / sizeof(uint16_t);

    /* Entire M2 bank for doppler output */
    pHwConfig->hwaMemOutDoppler = (DPU_CFARProcHWA_CfarDetOutput *)CSL_DSS_HWA_DMA0_RAM_BANK2_BASE;
    pHwConfig->hwaMemOutDopplerSize = CSL_DSS_HWA_BANK_SIZE / sizeof(DPU_CFARProcHWA_CfarDetOutput);

    /* Entire M3 bank for range output */
    pHwConfig->hwaMemOutRange = (DPU_CFARProcHWA_CfarDetOutput *)CSL_DSS_HWA_DMA0_RAM_BANK3_BASE;
    pHwConfig->hwaMemOutRangeSize = CSL_DSS_HWA_BANK_SIZE / sizeof(DPU_CFARProcHWA_CfarDetOutput);
    
    pHwConfig->cfarDopplerDetOutBitMaskSize = (params->numRangeBins * params->numDopplerBins) / 32;
    /* Avoid cfarDopplerDetOutBitMaskSize to round down if (numRangeBins * numDopplerBins) is not a multiple of 32 */
    if(0 != ((params->numRangeBins * params->numDopplerBins) % 32))
    {
    	pHwConfig->cfarDopplerDetOutBitMaskSize += 1U;
    }

    pHwConfig->cfarDopplerDetOutBitMask = (uint32_t *) DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.CoreLocalRamObj,
                                                                                pHwConfig->cfarDopplerDetOutBitMaskSize * sizeof(uint32_t),
                                                                                sizeof(uint32_t));

    if (pHwConfig->cfarDopplerDetOutBitMask == NULL)
    {
        retVal = DPC_OBJECTDETECTION_ENOMEM__CORE_LOCAL_RAM_CFAR_DOPPLER_DET_OUT_BIT_MASK;
        goto exit;
    }
    
    pHwConfig->cfarRngDopSnrListSize = MAX_NUM_DETECTIONS;
    gMmwMssMCB.cfarRngDopSnrList = DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.CoreLocalRamObj,
                                                        pHwConfig->cfarRngDopSnrListSize * sizeof(DPIF_CFARDetList),
                                                        sizeof(int16_t));
    
    pHwConfig->cfarRngDopSnrList = gMmwMssMCB.cfarRngDopSnrList;
    if (pHwConfig->cfarRngDopSnrList == NULL)
    {
        retVal = DPC_OBJECTDETECTION_ENOMEM__CORE_LOCAL_RAM_CFAR_OUT_DET_LIST;
        goto exit;
    }

exit:
    return retVal;
}

/**
*  @b Description
*  @n
*    Based on the configuration, set up the aoa2d processing DPU configurations
*/
int32_t DPC_ObjDet_AoaDpuCfg_Parser()
{
    int32_t retVal = 0;
    DPU_AoAProcHWA_StaticConfig  *params;
    DPU_AoAProcHWA_HW_Resources  *pHwConfig;
    DPU_AoAProc_DynamicConfig   *dynCfg;
    uint8_t txInd, rxInd, ind;
    float slope;

    memset((void *)&gAoa2dProcDpuCfg, 0, sizeof(DPU_AoAProcHWA_Config));

    pHwConfig = &gAoa2dProcDpuCfg.res;
    params = &gAoa2dProcDpuCfg.staticCfg;
    dynCfg = &gAoa2dProcDpuCfg.dynCfg;

    /* Static configurations */
    params->numTxAntennas      = gMmwMssMCB.numTxAntennas;
    params->numRxAntennas      = gMmwMssMCB.numRxAntennas;
    params->numDopplerChirps   = (gMmwMssMCB.mmWaveCfg.frameCfg.numOfBurstsInFrame * gMmwMssMCB.mmWaveCfg.frameCfg.numOfChirpsInBurst)/params->numTxAntennas;
    params->numDopplerBins     = mathUtils_pow2roundup(params->numDopplerChirps);
    params->numRangeBins       = gMmwMssMCB.numRangeBins;

    slope                      =   (float)(gMmwMssMCB.mmWaveCfg.profileTimeCfg.chirpSlope * 1.e12);

    params->rangeStep          = (MMWDEMO_RFPARSER_SPEED_OF_LIGHT_IN_METERS_PER_SEC * (gMmwMssMCB.adcSamplingRate * 1.e6)) /
                                    (2.f * slope * (2*params->numRangeBins));

    if (gMmwMssMCB.mmWaveCfg.frameCfg.numOfBurstsInFrame > 1)
    {
        /* Burst mode: Assumes h_NumOfBurstsInFrame > 1, h_NumOfChirpsInBurst = numTx. 
         * Below calculation may not be accurate for other combinations of h_NumOfChirpsInBurst 
         * and may need more robust technique to estimate doppler step, based on the use case */
        params->dopplerStep    =   MMWDEMO_RFPARSER_SPEED_OF_LIGHT_IN_METERS_PER_SEC /
                                    (2.f * params->numDopplerBins *
                                    gMmwMssMCB.centerFreq * (gMmwMssMCB.mmWaveCfg.frameCfg.burstPeriodus * 1e-6));
    }
    else
    {
        /* Normal mode: h_NumOfBurstsInFrame = 1, h_NumOfChirpsInBurst >= 2 */
        params->dopplerStep    =   MMWDEMO_RFPARSER_SPEED_OF_LIGHT_IN_METERS_PER_SEC /
                                    (2.f * gMmwMssMCB.mmWaveCfg.frameCfg.numOfChirpsInBurst *
                                    gMmwMssMCB.centerFreq * ((gMmwMssMCB.mmWaveCfg.profileTimeCfg.chirpIdleTimeus + gMmwMssMCB.mmWaveCfg.profileComCfg.chirpRampEndTimeus) * 1e-6));
    }

    if(gMmwMssMCB.staticClutterRemovalEnable)
    {
        params->isStaticClutterRemovalEnabled = 1;
    }

    if (gMmwMssMCB.antennaGeometryCfg.antDistanceXdimMts == 0.)
    {
        params->lambdaOverDistX = 2.0;
    }
    else
    {
        params->lambdaOverDistX = 3e8 / (gMmwMssMCB.centerFreq * gMmwMssMCB.antennaGeometryCfg.antDistanceXdimMts);
    }

    if (gMmwMssMCB.antennaGeometryCfg.antDistanceZdimMts == 0.)
    {
        params->lambdaOverDistZ = 2.0;
    }
    else
    {
        params->lambdaOverDistZ = 3e8 / (gMmwMssMCB.centerFreq * gMmwMssMCB.antennaGeometryCfg.antDistanceZdimMts);
    }

    params->numVirtualAnt       = params->numTxAntennas * params->numRxAntennas;

    ind = 0;
    for (txInd = 0; txInd < params->numTxAntennas; txInd++)
    {
        for (rxInd = 0; rxInd < params->numRxAntennas; rxInd++)
        {
            params->antForwardMapLUT[ind].rowIdx = gMmwMssMCB.activeAntennaGeometryCfg.ant[ind].row; 
            params->antForwardMapLUT[ind].colIdx = gMmwMssMCB.activeAntennaGeometryCfg.ant[ind].col;
            ind++;
        }
    }
    
    params->numAntRow = gMmwMssMCB.numAntRow;
    params->numAntCol = gMmwMssMCB.numAntCol;

    params->azimuthFftSize = gMmwMssMCB.aoaProcCfg.azimuthFftSize;
    params->elevationFftSize = gMmwMssMCB.aoaProcCfg.elevationFftSize;
    
    if(gMmwMssMCB.mmWaveCfg.profileComCfg.chirpTxMimoPatSel == 4)
    {
        params->isBpmEnabled = 1;
    }
    
    /* Dynamic or HW resources configurations */
    /* hwaCfg */
    pHwConfig->hwaCfg.numParamSet = DPU_AOAPROCHWA_NUM_HWA_PARAMS;
    pHwConfig->hwaCfg.paramSetStartIdx = gMmwMssMCB.numUsedHwaParamSets;

    pHwConfig->hwaCfg.winSym = HWA_FFT_WINDOW_SYMMETRIC;
    pHwConfig->hwaCfg.winRamOffset = DPC_ObjDet_HwaWinRamMemoryPoolAlloc(&gMmwMssMCB.HwaWinRamMemoryPoolObj,
                                                                           params->numDopplerChirps/2);

    if (pHwConfig->hwaCfg.winSym == HWA_FFT_WINDOW_NONSYMMETRIC)
    {
        pHwConfig->hwaCfg.windowSize = params->numDopplerChirps * sizeof(int32_t);
    }
    else
    {
        pHwConfig->hwaCfg.windowSize = ((params->numDopplerChirps + 1) / 2) * sizeof(int32_t);
    }

    pHwConfig->hwaCfg.window = (int32_t *)DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.CoreLocalRamObj,
                                                         pHwConfig->hwaCfg.windowSize,
                                                         sizeof(uint32_t));

    if (pHwConfig->hwaCfg.window == NULL)
    {
        retVal = DPC_OBJECTDETECTION_ENOMEM__CORE_LOCAL_RAM_AOA2D_HWA_WINDOW;
        goto exit;
    }

    if (!gMmwMssMCB.oneTimeConfigDone)
    {
        mathUtils_genWindow((uint32_t *)pHwConfig->hwaCfg.window,
                            (uint32_t) params->numDopplerChirps,
                            pHwConfig->hwaCfg.windowSize/sizeof(uint32_t),
                            DPC_DPU_DOPPLERPROC_FFT_WINDOW_TYPE,
                            DPC_OBJDET_QFORMAT_DOPPLER_FFT);
    }

    pHwConfig->radarCube.dataSize = params->numTxAntennas * params->numRangeBins * params->numDopplerBins * params->numRxAntennas * 4;
    pHwConfig->radarCube.data     = (cmplx16ImRe_t *)gMmwMssMCB.radarCube;
    pHwConfig->radarCube.datafmt  = DPIF_RADARCUBE_FORMAT_2;
    
    pHwConfig->cfarRngDopSnrList = gMmwMssMCB.cfarRngDopSnrList;
    pHwConfig->cfarRngDopSnrListSize = MAX_NUM_DETECTIONS;

    pHwConfig->detObjOutMaxSize = MAX_NUM_DETECTIONS;
    pHwConfig->detObjOut = gMmwMssMCB.dpcAoAObjOut;
    pHwConfig->detObjOutSideInfo = gMmwMssMCB.dpcAoAObjSideInfo;
    
    pHwConfig->detObj2dAzimIdx = DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.CoreLocalRamObj,
                                                        pHwConfig->detObjOutMaxSize *sizeof(uint8_t),
                                                        1);
    if (pHwConfig->detObj2dAzimIdx == NULL)
    {
        retVal = DPC_OBJECTDETECTION_ENOMEM__CORE_LOCAL_RAM_AOA_DET_OBJ_2_AZIM_IDX;
        goto exit;
    }

    pHwConfig->detObjElevationAngle = DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.CoreLocalRamObj,
                                                            pHwConfig->detObjOutMaxSize *sizeof(float),
                                                            DPC_OBJDET_DET_OBJ_ELEVATION_ANGLE_BYTE_ALIGNMENT);
    if (pHwConfig->detObjElevationAngle == NULL)
    {
        retVal = DPC_OBJECTDETECTION_ENOMEM__CORE_LOCAL_RAM_AOA_DET_OBJ_ELEVATION_ANGLE;
        goto exit;
    }

	/* Allocate buffers for ping and pong paths: */
    pHwConfig->localScratchBufferSizeBytes = DPU_AOAPROCHWA_NUM_LOCAL_SCRATCH_BUFFER_SIZE_BYTES(params->numTxAntennas);
    ind = 0;
    for (ind = 0; ind < DPU_AOAPROCHWA_NUM_LOCAL_SCRATCH_BUFFERS; ind++)
    {
        pHwConfig->localScratchBuffer[ind] = DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.CoreLocalRamObj,
                                         pHwConfig->localScratchBufferSizeBytes,
                                         DPU_AOAPROCHWA_LOCAL_SCRATCH_BYTE_ALIGNMENT);
       if (pHwConfig->localScratchBuffer[ind] == NULL)
       {
           retVal = DPC_OBJECTDETECTION_ENOMEM__CORE_LOCAL_RAM_AOA_SCRATCH_BUFFER;
           goto exit;
       }
    }

    /* hwRes - edmaCfg */
    pHwConfig->edmaHandle = gEdmaHandle[0];

    /* For main data processing ping/pong paths */
    pHwConfig->edmaHwaExt[0].chIn.channel =               DPC_OBJDET_DPU_AOA_PROC_EDMA_CH_0;
    pHwConfig->edmaHwaExt[0].chIn.eventQueue =            DPC_OBJDET_DPU_AOA_PROC_EDMAIN_PING_EVENT_QUE;
    pHwConfig->edmaHwaExt[0].chOut.channel =              DPC_OBJDET_DPU_AOA_PROC_EDMA_HWA_OUTPUT_CH_0;
    pHwConfig->edmaHwaExt[0].chOut.eventQueue =           DPC_OBJDET_DPU_AOA_PROC_EDMAOUT_PING_EVENT_QUE;
    pHwConfig->edmaHwaExt[0].stage[0].paramIn =           DPC_OBJDET_DPU_AOA_PROC_EDMA_VIRT_CH_0;
    pHwConfig->edmaHwaExt[0].stage[0].paramInSignature =  DPC_OBJDET_DPU_AOA_PROC_EDMA_VIRT_CH_1;
    pHwConfig->edmaHwaExt[0].stage[0].paramOut =          DPC_OBJDET_DPU_AOA_PROC_EDMA_VIRT_CH_2;
    pHwConfig->edmaHwaExt[0].stage[1].paramIn =           DPC_OBJDET_DPU_AOA_PROC_EDMA_VIRT_CH_3;
    pHwConfig->edmaHwaExt[0].stage[1].paramInSignature =  DPC_OBJDET_DPU_AOA_PROC_EDMA_VIRT_CH_4;
    pHwConfig->edmaHwaExt[0].stage[1].paramOut =          DPC_OBJDET_DPU_AOA_PROC_EDMA_VIRT_CH_5;
    pHwConfig->edmaHwaExt[0].stage[1].paramPeakCnt =      DPC_OBJDET_DPU_AOA_PROC_EDMA_VIRT_CH_12;
    pHwConfig->edmaHwaExt[0].stage[1].paramHwaContinue =  DPC_OBJDET_DPU_AOA_PROC_EDMA_VIRT_CH_14;
    pHwConfig->edmaHwaExt[0].eventQueue = 0;

    pHwConfig->edmaHwaExt[1].chIn.channel =               DPC_OBJDET_DPU_AOA_PROC_EDMA_CH_1;
    pHwConfig->edmaHwaExt[1].chIn.eventQueue =            DPC_OBJDET_DPU_AOA_PROC_EDMAIN_PONG_EVENT_QUE;
    pHwConfig->edmaHwaExt[1].chOut.channel =              DPC_OBJDET_DPU_AOA_PROC_EDMA_HWA_OUTPUT_CH_1;
    pHwConfig->edmaHwaExt[1].chOut.eventQueue =           DPC_OBJDET_DPU_AOA_PROC_EDMAOUT_PONG_EVENT_QUE;
    pHwConfig->edmaHwaExt[1].stage[0].paramIn =           DPC_OBJDET_DPU_AOA_PROC_EDMA_VIRT_CH_6;
    pHwConfig->edmaHwaExt[1].stage[0].paramInSignature =  DPC_OBJDET_DPU_AOA_PROC_EDMA_VIRT_CH_7;
    pHwConfig->edmaHwaExt[1].stage[0].paramOut =          DPC_OBJDET_DPU_AOA_PROC_EDMA_VIRT_CH_8;
    pHwConfig->edmaHwaExt[1].stage[1].paramIn =           DPC_OBJDET_DPU_AOA_PROC_EDMA_VIRT_CH_9;
    pHwConfig->edmaHwaExt[1].stage[1].paramInSignature =  DPC_OBJDET_DPU_AOA_PROC_EDMA_VIRT_CH_10;
    pHwConfig->edmaHwaExt[1].stage[1].paramOut =          DPC_OBJDET_DPU_AOA_PROC_EDMA_VIRT_CH_11;
    pHwConfig->edmaHwaExt[1].stage[1].paramPeakCnt =      DPC_OBJDET_DPU_AOA_PROC_EDMA_VIRT_CH_13;
    pHwConfig->edmaHwaExt[1].stage[1].paramHwaContinue =  DPC_OBJDET_DPU_AOA_PROC_EDMA_VIRT_CH_15;
    pHwConfig->edmaHwaExt[1].eventQueue = 0;

    /* dynamic config */
    gMmwMssMCB.multiObjBeamFormingCfg.enabled = 0;
    gMmwMssMCB.multiObjBeamFormingCfg.multiPeakThrsScal = 0.5;
    dynCfg->fovAoaCfg                  = &gMmwMssMCB.fovAoaCfg;
    dynCfg->multiObjBeamFormingCfg     = &gMmwMssMCB.multiObjBeamFormingCfg;

    /* Rx compensation coefficients */
    dynCfg->compRxChanCfg = &gMmwMssMCB.compRxChannelBiasCfg;

exit:
return retVal;
}

/**
*  @b Description
*  @n
*        Function configuring range processing DPU
*/
void DPC_ObjDet_RngDpuCfg()
{
    int32_t retVal = 0;
    uint8_t numUsedHwaParamSets;

    retVal = DPC_ObjDet_RngDpuCfg_Parser();
    if (retVal < 0)
    {
        CLI_write("Error in setting up range profile:%d \n", retVal);
        DebugP_assert(0);
    }

    retVal = DPU_RangeProcHWA_config(gMmwMssMCB.rangeProcDpuHandle, &gRangeProcDpuCfg);
    if (retVal < 0)
    {
        CLI_write("Error: RANGE DPU config return error:%d \n", retVal);
        DebugP_assert(0);
    }

    /* Get number of used HWA param sets by this DPU */
    numUsedHwaParamSets = DPU_RANGEPROCHWA_NUM_HWA_PARAM_SETS;
    
    /* Update number of used HWA param sets */
    gMmwMssMCB.numUsedHwaParamSets += numUsedHwaParamSets;
}

/**
*  @b Description
*  @n
*        Function configuring dopplerproc
*/
void DPC_ObjDet_DopplerDpuCfg()
{
    int32_t retVal = 0;
    uint8_t numUsedHwaParamSets, numPingPongPath;

    numPingPongPath = 2;

    retVal = DPC_ObjDet_DopplerDpuCfg_Parser();
    if (retVal < 0)
    {
        CLI_write("Error: Error in setting up doppler profile:%d \n", retVal);
        DebugP_assert(0);
    }

    retVal = DPU_DopplerProcHWA_config (gMmwMssMCB.dopplerProcDpuHandle, &gDopplerProcDpuCfg);
    if (retVal < 0)
    {
        CLI_write("Doppler DPU config return error:%d \n", retVal);
        DebugP_assert(0);
    }

    /* Get number of used HWA param sets by this DPU */
    numUsedHwaParamSets = DPU_DOPPLERPROCHWA_NUM_HWA_PARAMS_FMT2(numPingPongPath);
    
    /* Update number of used HWA param sets */
    gMmwMssMCB.numUsedHwaParamSets += numUsedHwaParamSets;
}

/**
*  @b Description
*  @n
*        Function configuring CFAR DPU
*/
void DPC_ObjDet_CfarDpuCfg()
{
    int32_t retVal = 0;
    uint8_t numUsedHwaParamSets;

    retVal = DPC_ObjDet_CfarDpuCfg_Parser();
    if (retVal < 0)
    {
        CLI_write("Error in setting up CFAR profile:%d \n", retVal);
        DebugP_assert(0);
    }

    retVal = DPU_CFARProcHWA_config(gMmwMssMCB.cfarProcDpuHandle, &gCfarProcDpuCfg);
    if (retVal < 0)
    {
        CLI_write("CFAR DPU config return error:%d \n", retVal);
        DebugP_assert(0);
    }

    /* Get number of used HWA param sets by this DPU */
    numUsedHwaParamSets = DPU_CFARPROCHWA_NUM_HWA_PARAMS;
    
    /* Update number of used HWA param sets */
    gMmwMssMCB.numUsedHwaParamSets += numUsedHwaParamSets;
}

/**
*  @b Description
*  @n
*        Function configuring AOA2D DPU
*/
void DPC_ObjDet_AoaDpuCfg()
{
    int32_t retVal = 0;
    uint8_t numUsedHwaParamSets;

    retVal = DPC_ObjDet_AoaDpuCfg_Parser();
    if (retVal < 0)
    {
        CLI_write("Error: Error in setting up aoa2d profile:%d \n", retVal);
        DebugP_assert(0);
    }

    retVal = DPU_AoAProcHWA_config (gMmwMssMCB.aoa2dProcDpuHandle, &gAoa2dProcDpuCfg);
    if (retVal < 0)
    {
        CLI_write("AOA2D DPU config return error:%d \n", retVal);
        DebugP_assert(0);
    }

    /* Get number of used HWA param sets by this DPU */
    numUsedHwaParamSets = DPU_AOAPROCHWA_NUM_HWA_PARAMS;

    /* Update number of used HWA param sets */
    gMmwMssMCB.numUsedHwaParamSets += numUsedHwaParamSets;
}

/**
*  @b Description
*  @n
*        Function initiliazing all indvidual DPUs
*/
void DPC_Init()
{
    /* hwa, edma, and DPU initialization*/

    /* Register Frame Start Interrupt */
    if(MmwDemo_registerFrameStartInterrupt() != 0){
        CLI_write("Error: Failed to register frame start interrupts\n");
        DebugP_assert(0);
    }

    if(MmwDemo_registerChirpAvailableInterrupts() != 0){
        CLI_write("Failed to register chirp available interrupts\n");
        DebugP_assert(0);
    }
#if 0
    /* For debugging purposes*/
    MmwDemo_registerChirpInterrupt();
    MmwDemo_registerBurstInterrupt();
#endif
    int32_t status = SystemP_SUCCESS;

    /* Shared memory pool */
    gMmwMssMCB.L3RamObj.cfg.addr = (void *)&gMmwL3[0];
    gMmwMssMCB.L3RamObj.cfg.size = sizeof(gMmwL3);

    /* Local memory pool */
    gMmwMssMCB.CoreLocalRamObj.cfg.addr = (void *)&gMmwCoreLocMem[0];
    gMmwMssMCB.CoreLocalRamObj.cfg.size = sizeof(gMmwCoreLocMem);

    gHwaHandle = HWA_open(0, NULL, &status);
    if (gHwaHandle == NULL)
    {
        CLI_write("Error: Unable to open the HWA Instance err:%d\n", status);
        DebugP_assert(0);
    }

    DPC_ObjDet_RngDpuInit();
    DPC_ObjDet_DopplerDpuInit();
    DPC_ObjDet_CfarDpuInit();
    DPC_ObjDet_AoaDpuInit();
}


/**
*  @b Description
*  @n

*        Function configuring all DPUs
*/
void DPC_Config()
{

    int32_t retVal;
    
    DPC_ObjDet_MemPoolReset(&gMmwMssMCB.L3RamObj);
    DPC_ObjDet_MemPoolReset(&gMmwMssMCB.CoreLocalRamObj);
    DPC_ObjDet_HwaDmaTrigSrcChanPoolReset(&gMmwMssMCB.HwaDmaChanPoolObj);
    DPC_ObjDet_HwaWinRamMemoryPoolReset(&gMmwMssMCB.HwaWinRamMemoryPoolObj);

    gMmwMssMCB.dpcAoAObjOut = (DPIF_PointCloudCartesian *) DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.L3RamObj,
                                                                                       MAX_NUM_DETECTIONS * sizeof(DPIF_PointCloudCartesian),
                                                                                       DPC_OBJDET_POINT_CLOUD_CARTESIAN_BYTE_ALIGNMENT);
    if (gMmwMssMCB.dpcAoAObjOut == NULL)
    {
        CLI_write("DPC configuration: memory allocation failed\n");
        DebugP_assert(0);
    }

    gMmwMssMCB.dpcAoAObjSideInfo = (DPIF_PointCloudSideInfo *) DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.L3RamObj,
                                                                                       MAX_NUM_DETECTIONS * sizeof(DPIF_PointCloudSideInfo),
                                                                                       DPC_OBJDET_POINT_CLOUD_SIDE_INFO_BYTE_ALIGNMENT);
    if (gMmwMssMCB.dpcAoAObjSideInfo == NULL)
    {
        CLI_write("DPC configuration: memory allocation failed\n");
        DebugP_assert(0);
    }
    /* Select active antennas from available antennas and calculate number of antennas rows and columns */
    MmwDemo_calcActiveAntennaGeometry();

    /* Configure DPUs */
    gMmwMssMCB.numUsedHwaParamSets = 0;
    DPC_ObjDet_RngDpuCfg();
    DPC_ObjDet_DopplerDpuCfg();
    DPC_ObjDet_CfarDpuCfg();
    DPC_ObjDet_AoaDpuCfg();

    if(gMmwMssMCB.measureRxChannelBiasCliCfg.enabled)
    {
        retVal = MmwDemo_rangeBiasRxChPhaseMeasureConfig();
        if (retVal != 0)
        {
            CLI_write("DPC configuration: Invalid Rx channel compensation procedure configuration \n");
            DebugP_assert(0);
        }
    }

    if (!gMmwMssMCB.oneTimeConfigDone)
    {
        /* Report RAM usage */
        gMmwMssMCB.memUsage.CoreLocalRamUsage = DPC_ObjDet_MemPoolGetMaxUsage(&gMmwMssMCB.CoreLocalRamObj);
        gMmwMssMCB.memUsage.L3RamUsage = DPC_ObjDet_MemPoolGetMaxUsage(&gMmwMssMCB.L3RamObj);
        
        gMmwMssMCB.memUsage.L3RamTotal = gMmwMssMCB.L3RamObj.cfg.size;
        gMmwMssMCB.memUsage.CoreLocalRamTotal = gMmwMssMCB.CoreLocalRamObj.cfg.size;
    
        if(gMmwMssMCB.lowPowerMode == LOW_PWR_MODE_DISABLE)
        {
            DebugP_log(" ========== Memory Stats ==========\n");
            DebugP_log("%20s %12s %12s %12s\n", " ", "Size", "Used", "Free");

            DebugP_log("%20s %12d %12d %12d\n", "L3",
                      sizeof(gMmwL3),
                      gMmwMssMCB.memUsage.L3RamUsage,
                      sizeof(gMmwL3) - gMmwMssMCB.memUsage.L3RamUsage);

            DebugP_log("%20s %12d %12d %12d\n", "Local",
                      sizeof(gMmwCoreLocMem),
                      gMmwMssMCB.memUsage.CoreLocalRamUsage,
                      sizeof(gMmwCoreLocMem) - gMmwMssMCB.memUsage.CoreLocalRamUsage);
        }
    }

}

/**
 *  @b Description
 *  @n  DPC processing chain execute function.
 *
 */
void DPC_Execute()
{
    int32_t retVal;
    int32_t errCode = 0;
    int32_t i;
    
    DSSHWACCRegs *ctrlBaseAddr = (DSSHWACCRegs *)gHwaObjectPtr[0]->hwAttrs->ctrlBaseAddr;

    DPU_RangeProcHWA_OutParams outParms;
    DPU_DopplerProcHWA_OutParams outParmsDoppler;
    DPU_CFARProcHWA_OutParams outParmsCfar;
    DPU_AoAProcHWA_OutParams outParmsAoa2d;
#if (ADC_DATA_LOGGING_VIA_SPI == 1)  
    /* Variables require for ADC Data Logging via SPI */
    MCSPI_Transaction   spiTransaction;
    int32_t             transferOK;
    uint32_t totalSizeToTfr,tempSize;
    uint8_t count;
    uint32_t* adc_data = (uint32_t*)adcbuffer;
#endif
    DPC_ObjectDetection_ExecuteResult *result = &gMmwMssMCB.dpcResult;
    uint32_t numDetectedPoints;
    /* give initial trigger for the first frame */
    errCode = DPU_RangeProcHWA_control(gMmwMssMCB.rangeProcDpuHandle,
                 DPU_RangeProcHWA_Cmd_triggerProc, NULL, 0);
    if(errCode < 0)
    {
        CLI_write("Error: Range control execution failed [Error code %d]\n", errCode);
    }
    
    result->objOut = gMmwMssMCB.dpcAoAObjOut;
    result->objOutSideInfo = gMmwMssMCB.dpcAoAObjSideInfo;
    result->rngDopplerHeatMap = (uint16_t *) gMmwMssMCB.detMatrix;

    /* Send signal to CLI task that this is ready */
    SemaphoreP_post(&gMmwMssMCB.dpcTaskConfigDoneSemHandle);

    while(true){

        memset((void *)&outParms, 0, sizeof(DPU_RangeProcHWA_OutParams));
        retVal = DPU_RangeProcHWA_process(gMmwMssMCB.rangeProcDpuHandle, &outParms);
        if(retVal != 0)
        {
            CLI_write("DPU_RangeProcHWA_process failed with error code %d", retVal);
            DebugP_assert(0);
        }

        /* To lower power consumption, the R5 clock is set to 40 MHz during chirping by selecting the OSC Clock as the source with a divider of 1. 
         * After chirping, the R5 clock is restored to 200 MHz by switching the source to Fast Clock 1 with a divider of 1. 
         * Since ADC logging requires the R5 to run at 200 MHz, this is done only when ADC logging is turned off.
         */
        if (gMmwMssMCB.adcLogging.enable == 0)
        {
            SOC_rcmSetR5Clock(SOC_R5FCoreFreq200MHz,SOC_FastClk1Freq200MHz, SOC_RcmR5ClockSource_FAST_CLK1);
        }

        /***************************ADC Streaming Via SPI***********************************************************/
        /* When ADC logging via SPI is enabled, this section allows the streaming of raw ADC data over SPI interface every frame during the frame idle time. */
#if (ADC_DATA_LOGGING_VIA_SPI == 1)
        if(gMmwMssMCB.adcLogging.enable == 2)
        {
            totalSizeToTfr = adcDataPerFrame;
            tempSize = adcDataPerFrame;
            count = 0;
            while(totalSizeToTfr > 0)
            {
                if(totalSizeToTfr > MAXSPISIZEFTDI)
                {
                    tempSize=MAXSPISIZEFTDI;
                }
                else
                {
                    tempSize = totalSizeToTfr;
                }
                
                MCSPI_Transaction_init(&spiTransaction);
                spiTransaction.channel  = gConfigMcspi0ChCfg[0].chNum;
                spiTransaction.dataSize = 32;
                spiTransaction.csDisable = TRUE;
                spiTransaction.count    = tempSize/4;
                spiTransaction.txBuf    = (void *)(&adc_data[(MAXSPISIZEFTDI/4)*count]);
                spiTransaction.rxBuf    = NULL;
                spiTransaction.args     = NULL;

                GPIO_pinWriteLow(gSPIHostIntrBaseAddrLed, gSPIHostIntrPinNumLed);

                transferOK = MCSPI_transfer(gMcspiHandle[CONFIG_MCSPI0], &spiTransaction);
                if(transferOK != 0)
                {
                    CLI_write("SPI Raw Data Transfer Failed\r\n");
                }
                GPIO_pinWriteHigh(gSPIHostIntrBaseAddrLed, gSPIHostIntrPinNumLed);
                totalSizeToTfr  =   totalSizeToTfr  -   tempSize;
                count++;
            }
        }
#endif
    /********************************************************************/
        /* Chirping finished start interframe processing */
        gMmwMssMCB.stats.ProcessingStartTimeStampUs = ClockP_getTimeUsec();
        gMmwMssMCB.stats.chirpingTimeUs = (gMmwMssMCB.stats.ProcessingStartTimeStampUs - gMmwMssMCB.stats.frameStartTimeStampUs);

        if(gMmwMssMCB.gpAdcCfg.channelEnable != 0)
        {
            /* Read the GPADC Data */
            retVal = MMWave_readGPADC(&gMmwMssMCB.gpAdcCfg.gpAdcValVolts[0], &gMmwMssMCB.gpAdcCfg.gpAdcValVolts[1], &gMmwMssMCB.gpAdcCfg.gpAdcValVolts[2], &gMmwMssMCB.gpAdcCfg.gpAdcValVolts[3]);
            if(retVal != 0)
            {
                CLI_write("Error: GPADC measurement failed with error code %d", retVal);
                DebugP_assert(0);
            }
            /* GPADC voltage prints */
            if(gMmwMssMCB.gpAdcCfg.volPrintsEnable)
            {
                if((gMmwMssMCB.gpAdcCfg.channelEnable & GPADCPIN1_ENABLE) == GPADCPIN1_ENABLE)
                {
                    CLI_write("\r\n GPADC 1 Reading: %f\r\n",gMmwMssMCB.gpAdcCfg.gpAdcValVolts[0]);
                }
                if((gMmwMssMCB.gpAdcCfg.channelEnable & GPADCPIN2_ENABLE) == GPADCPIN2_ENABLE)
                {
                    CLI_write("\r\n GPADC 2 Reading: %f\r\n",gMmwMssMCB.gpAdcCfg.gpAdcValVolts[1]);
                }
                if((gMmwMssMCB.gpAdcCfg.channelEnable & GPADCPIN3_ENABLE) == GPADCPIN3_ENABLE)
                {
                    CLI_write("\r\n GPADC 3 Reading: %f\r\n",gMmwMssMCB.gpAdcCfg.gpAdcValVolts[2]);
                }
                if((gMmwMssMCB.gpAdcCfg.channelEnable & GPADCPIN4_ENABLE) == GPADCPIN4_ENABLE)
                {
                    CLI_write("\r\n GPADC 4 Reading: %f\r\n",gMmwMssMCB.gpAdcCfg.gpAdcValVolts[3]);
                }
            }
        }
        /* Read the temperature */
        MMWave_getTemperatureReport(&gTempStats);

        #if (ENABLE_MONITORS==1)
        /* When APLL frequency shifting and monitors(if any) are enabled:
         * 1. Run PLL control voltage monitor(if enabled) at 396MHz APLL frequency
         * 2. Restore APLL frequency to 400MHz
         * 3. Run all other enabled monitors at 400MHz APLL frequency
         * 4. If lowpowermode is disabled restore APLL frequency to 396MHz for chirping */
        if(gMmwMssMCB.apllFreqShiftEnable == TRUE)
        {
            /* Check if PLL Control Voltage monitor is enabled */
            if (gMmwMssMCB.rfMonEnbl & ((uint64_t)0x1U << M_RL_MON_PLL_CTRL_VOLT))
            {
                /* Trigger PLL Control Voltage monitor */
                MMWave_enableMonitors(gMmwMssMCB.rfMonEnbl & ((uint64_t)0x1U << M_RL_MON_PLL_CTRL_VOLT));
                /* Wait till monitor is done */
                SemaphoreP_pend(&gMmwMssMCB.rfmonSemHandle, SystemP_WAIT_FOREVER);
            }
            /* Set APLL back to 400MHz frequency */
            retVal = MmwDemo_configAndEnableApll(APLL_FREQ_400MHZ, RESTORE_APLL_CALIB_DATA);
            if(retVal != M_DFP_RET_CODE_OK)
            {
                CLI_write ("Error: FECSS/APLL Clock Turn ON failed\r\n");
                retVal = SystemP_FAILURE;
                MmwDemo_debugAssert (0);
            }

            /* Trigger any other enabled monitors at 400MHz APLL frequency */
            if(gMmwMssMCB.rfMonEnbl & (~((uint64_t)0x1U << M_RL_MON_PLL_CTRL_VOLT)))
            {
                /* Trigger remaining monitors at 400MHz APLL frequency */
                MMWave_enableMonitors(gMmwMssMCB.rfMonEnbl & (~((uint64_t)0x1U << M_RL_MON_PLL_CTRL_VOLT)));
                /* Wait till monitor is done */
                SemaphoreP_pend(&gMmwMssMCB.rfmonSemHandle, SystemP_WAIT_FOREVER);
            }

            /* Restore APLL frequency to 396MHz for chirping, when lowPowerMode is disabled */
            if(gMmwMssMCB.lowPowerMode == LOW_PWR_MODE_DISABLE)
            {
                /* Set APLL back to 396MHz frequency */
                retVal = MmwDemo_configAndEnableApll(APLL_FREQ_396MHZ, RESTORE_APLL_CALIB_DATA);
                if(retVal != M_DFP_RET_CODE_OK)
                {
                    CLI_write ("Error: FECSS/APLL Clock Turn ON failed\r\n");
                    retVal = SystemP_FAILURE;
                    MmwDemo_debugAssert (0);
                }
            }
        }

        /* If APLL frequency shifting is off and any monitors are enabled, run all configured monitors at default 400MHz APLL frequency */
        if((gMmwMssMCB.apllFreqShiftEnable == FALSE) && (gMmwMssMCB.rfMonEnbl != 0))
        {
            /* Trigger monitors */
            MMWave_enableMonitors(gMmwMssMCB.rfMonEnbl);
            /* Wait till monitor is done */
            SemaphoreP_pend(&gMmwMssMCB.rfmonSemHandle, SystemP_WAIT_FOREVER);
        }

        /* If Synth Frequency Monitor is enabled read the value*/
        if((gMmwMssMCB.mmWaveCfg.strtCfg.frameLivMonEn & 0x1U) == 0x1U)
        {
            gMmwMssMCB.rfMonRes.synthFreqres = MMWaveMon_getSynthFreqMonres();
            #if (PRINT_MON_RES == 1)
            CLI_write("Synth Frequency monitor: %x \r\n",gMmwMssMCB.rfMonRes.synthFreqres.status);
            #endif
        }

        /* If Rx Sat Live Monitor is enabled read the value*/
        if((gMmwMssMCB.mmWaveCfg.strtCfg.frameLivMonEn & 0x2U) == 0x2U)
        {
            /*Base Address of Rx Saturation Live Monitor Results */
            uint32_t *baseAddrRxSatLive; 
            gMmwMssMCB.rfMonRes.rxSatLiveres.rxSatLivePtr = MMWaveMon_getRxSatLiveMonres(MON_RX_SAT_LUT_OFFSET); 
            /*Each chirp will have 1 byte data for each RX, max 4 RX is supported (4th Rx is reserved)*/
            for (int i=0; i<(gMmwMssMCB.mmWaveCfg.frameCfg.numOfChirpsInBurst * gMmwMssMCB.mmWaveCfg.frameCfg.numOfBurstsInFrame);i+=1U)
            {
                /* Incrementing Address by 32 bits */
                baseAddrRxSatLive = gMmwMssMCB.rfMonRes.rxSatLiveres.rxSatLivePtr + i;
                /*Checking Rx Saturation Live Monitor status*/
                if(*baseAddrRxSatLive==0U)
                {
                    gMmwMssMCB.rfMonRes.status_rxSatLive=0x1U; // No Saturation
                }
                else
                {
                    gMmwMssMCB.rfMonRes.status_rxSatLive=0x0U;
                    break;
                }
            }
            #if (PRINT_MON_RES == 1)
            if(gMmwMssMCB.rfMonRes.status_rxSatLive == 0U)
            {
                CLI_write("Rx Saturation Live monitor: Saturation is occurring \r\n");
            }
            else
            {
                CLI_write("Rx Saturation Live monitor: No Saturation  \r\n");
            }
            #endif
        }
        #endif

        if(gMmwMssMCB.lowPowerMode == LOW_PWR_MODE_ENABLE)
        {
            /* Shutdown the FECSS after chirping in Low Power Mode */
            int32_t err;
            
            /* To-Do the retention list Retain FECSS Code Memory */
            PRCMSetSRAMRetention((PRCM_FEC_PD_SRAM_CLUSTER_1 | PRCM_FEC_PD_SRAM_CLUSTER_2 | PRCM_FEC_PD_SRAM_CLUSTER_3));
            
            /* Delay to account for RFS Processing time before shutting it down */
            ClockP_usleep(RFS_PROC_END_TIME);
            
            /* Reset The FrameTimer for next frame */
            HW_WR_REG32(CSL_APP_RCM_U_BASE + CSL_APP_RCM_BLOCKRESET2, 0x1c0);

            /* To-Do Give a proper delay */
            for(int i =0;i<10;i++)
            {
                test = PRCMSlowClkCtrGet();
            }

            HW_WR_REG32(CSL_APP_RCM_U_BASE + CSL_APP_RCM_BLOCKRESET2, 0x0);
            
            /* Front End Shutdown in preparation for Low power state */
            MMWave_stop(gMmwMssMCB.ctrlHandle,&gMmwMssMCB.mmWaveCfg.strtCfg,&err);
            MMWave_close(gMmwMssMCB.ctrlHandle,&err);
            MMWave_deinit(gMmwMssMCB.ctrlHandle,&err);
        }

        /* Procedure for range bias measurement and Rx channels gain/phase offset measurement */
        if(gMmwMssMCB.measureRxChannelBiasCliCfg.enabled)
        {
            MmwDemo_rangeBiasRxChPhaseMeasure();
        }
        
        /* Doppler DPU */
        memset((void *)&outParmsDoppler, 0, sizeof(DPU_DopplerProcHWA_OutParams));
        retVal = DPU_DopplerProcHWA_process(gMmwMssMCB.dopplerProcDpuHandle, &outParmsDoppler);
        if(retVal != 0){
            CLI_write("DPU_DopplerProc_process failed with error code %d", retVal);
            DebugP_assert(0);
        }

        /********* TODO: Known Errata - MMWSOC_IWRL68XX-1900. Dynamic clock gating disabled for HWA CFAR engine ************/
        CSL_FINSR(ctrlBaseAddr->HWACCREG1,
                    HWACCREG1_ACCDYNCLKEN_BIT_END,
                    HWACCREG1_ACCDYNCLKEN_BIT_START,
                    0x0U);
        /******************************************************************************************************************/                    
        /* CFAR DPU */
        numDetectedPoints = 0;
        memset((void *)&outParmsCfar, 0, sizeof(DPU_CFARProcHWA_OutParams));
        
        retVal = DPU_CFARProcHWA_process(gMmwMssMCB.cfarProcDpuHandle,
                                         &outParmsCfar);
        numDetectedPoints = outParmsCfar.numCfarDetectedPoints;

        result->numObjOut = numDetectedPoints;
        if(retVal != 0){
            CLI_write("DPU_CFARProcHWA_process failed with error code %d", retVal);
            DebugP_assert(0);
        }
        if(gMmwMssMCB.multiObjBeamFormingCfg.enabled == 0)
        {
            /********* TODO: Known Errata - MMWSOC_IWRL68XX-1900. Dynamic clock gating disabled for HWA CFAR engine ************/
            CSL_FINSR(ctrlBaseAddr->HWACCREG1,
                        HWACCREG1_ACCDYNCLKEN_BIT_END,
                        HWACCREG1_ACCDYNCLKEN_BIT_START,
                        0x1U);
            /******************************************************************************************************************/
        }
        
        /* Prepare range gates for AoA */
        memset((void *)&outParmsAoa2d, 0, sizeof(DPU_AoAProcHWA_OutParams));
        
        retVal = DPU_AoAProcHWA_process(gMmwMssMCB.aoa2dProcDpuHandle,
                                       numDetectedPoints,
                                       &outParmsAoa2d);
        if(retVal != 0){
            CLI_write("DPU_Aoa2dProc_process failed with error code %d", retVal);
            DebugP_assert(0);
        }

        result->numObjOut = outParmsAoa2d.numAoADetectedPoints;
        if(gMmwMssMCB.multiObjBeamFormingCfg.enabled == 1)
        {
            /********* TODO: Known Errata - MMWSOC_IWRL68XX-1900. Dynamic clock gating disabled for HWA CFAR engine ************/
            CSL_FINSR(ctrlBaseAddr->HWACCREG1,
                        HWACCREG1_ACCDYNCLKEN_BIT_END,
                        HWACCREG1_ACCDYNCLKEN_BIT_START,
                        0x1U);
            /******************************************************************************************************************/
        }

        if(gMmwMssMCB.guiMonSel.pointCloud == 1)
        {
            for(i=0; i < result->numObjOut; i++)
            {
                result->objOutSideInfo[i].snr = (int16_t) (10. * result->objOutSideInfo[i].snr); //steps of 0.1dB
                result->objOutSideInfo[i].noise = (int16_t) (10. * result->objOutSideInfo[i].noise); //steps of 0.1dB
            }
        }
        
        /* Give initial trigger for the next frame */
        retVal = DPU_RangeProcHWA_control(gMmwMssMCB.rangeProcDpuHandle,
                    DPU_RangeProcHWA_Cmd_triggerProc, NULL, 0);
        if(retVal < 0)
        {
            CLI_write("Error: DPU_RangeProcHWA_control failed with error code %d", retVal);
            DebugP_assert(0);
        }


        /* Interframe processing finished */
        gMmwMssMCB.stats.ProcessingEndTimeStampUs = ClockP_getTimeUsec();
        gMmwMssMCB.outStats.interFrameProcessingTimeUs = (gMmwMssMCB.stats.ProcessingEndTimeStampUs - gMmwMssMCB.stats.ProcessingStartTimeStampUs);

        /* If ADC logging via LVDS is enabled, Pend for completion of session, generally this will not wait
        * because of time spent doing inter-frame processing is expected to
        * be bigger than the transmission of the session */
        if (gMmwMssMCB.adcLogging.enable == 1)
        {
            SemaphoreP_pend(&gMmwMssMCB.lvdsStream.frameDoneSemHandle, SystemP_WAIT_FOREVER);
            if(gMmwMssMCB.lvdsStream.frameDoneCount == (gMmwMssMCB.mmWaveCfg.frameCfg.numOfChirpsInBurst * gMmwMssMCB.mmWaveCfg.frameCfg.numOfBurstsInFrame))
            {
                gMmwMssMCB.lvdsStream.frameDoneCount = 0;
            }
            else 
            {
                CLI_write("Error: Some chirps are not transmitted successfully through LVDS\n");
                DebugP_assert(0);
            }
        }

        /* Trigger UART task to send TLVs to host */
        SemaphoreP_post(&gMmwMssMCB.tlvSemHandle);
    }
}

/**
*  @b Description
*  @n
*        Function configuring and executing DPC
*/
void MmwDemo_dpcTask()
{
    /*On R5F, by default task switch does not save/restore FPU (floating point unit) registers, tasks which need FPU need to call `vPortTaskUsesFPU`
    once before using FPU operations.*/
    vPortTaskUsesFPU();
    
    DPC_Config();

    DPC_Execute();

    /* Never return for this task. */
    SemaphoreP_pend(&gMmwMssMCB.TestSemHandle, SystemP_WAIT_FOREVER);
}