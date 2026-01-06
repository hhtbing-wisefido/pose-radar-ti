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
/* MCU Plus Include Files. */
#include <kernel/dpl/SemaphoreP.h>
#include <kernel/dpl/CacheP.h>
#include <kernel/dpl/ClockP.h>
#include <kernel/dpl/DebugP.h>
#include <kernel/dpl/HwiP.h>
#include <kernel/dpl/AddrTranslateP.h>

#include <drivers/ipc_notify.h>

/* mmwave SDK files */
#include <control/mmwave/mmwave.h>
#include <source/mmw_cli.h>
#include "ti_drivers_config.h"
#include "ti_drivers_open_close.h"
#include "ti_board_open_close.h"
#include "ti_board_config.h"
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <drivers/power.h>
#include <drivers/prcm.h>
#include <utils/mathutils/mathutils.h>

#include <source/mmwave_demo_mss.h>
#include <source/mmw_res.h>
#include <source/dpc/dpc_mss.h>
#include <source/mmwave_control/interrupts.h>
#include <source/calibrations/range_phase_bias_measurement.h>
#include <source/utils/mmw_demo_utils.h>
#include <common_mss_dss/msg_ipc/msg_ipc.h>
#include <source/lvds_streaming/mmw_lvds_stream.h>

/**************************************************************************
 *************************** Macros Definitions ***************************
 **************************************************************************/

#define HWA_MAX_NUM_DMA_TRIG_CHANNELS 16
#define MAX_NUM_DETECTIONS          (MMWDEMO_OUTPUT_POINT_CLOUD_LIST_MAX_SIZE)

#define LOW_PWR_MODE_DISABLE (0)
#define LOW_PWR_MODE_ENABLE (1)
#define LOW_PWR_TEST_MODE (2)

#define MMW_DEMO_MAJOR_MODE 0
#define MMW_DEMO_MINOR_MODE 1

#define FRAME_REF_TIMER_CLOCK_MHZ  40

/* Max Frame Size for FTDI chip is 64KB */
#define MAXSPISIZEFTDI               (65536U)

#define DPC_DPU_DOPPLERPROC_FFT_WINDOW_TYPE MATHUTILS_WIN_HANNING
#define DOPPLER_OUTPUT_MAPPING_DOP_ROW_COL   0
#define DOPPLER_OUTPUT_MAPPING_ROW_DOP_COL   1
#define DPC_OBJDET_QFORMAT_DOPPLER_FFT 17

#define MMWDEMO_RFPARSER_SPEED_OF_LIGHT_IN_METERS_PER_SEC (3e8)

#define DPC_DPU_DOPPLERPROC_FFT_WINDOW_TYPE MATHUTILS_WIN_HANNING
#define DPC_DPU_MACRO_DOPPLERPROC_FFT_WINDOW_TYPE MATHUTILS_WIN_HANNING

#define DPC_OBJDET_QFORMAT_DOPPLER_FFT 17
#define DPC_OBJDET_QFORMAT_MACRO_DOPPLER_FFT 17

#define DPC_OBJDET_HWA_WINDOW_RAM_OFFSET 0
#define DPC_DPU_RANGEPROC_FFT_WINDOW_TYPE MATHUTILS_WIN_BLACKMAN
#define DPC_OBJDET_QFORMAT_RANGE_FFT 17
#define MMW_DEMO_TEST_ADC_BUFF_SIZE 1024  //maximum 128 real samples (int16_t), 3 Rx channels

#define DPC_OBJDET_POINT_CLOUD_CARTESIAN_BYTE_ALIGNMENT       (MAX(DPU_AOAPROCHWA_POINT_CLOUD_CARTESIAN_BYTE_ALIGNMENT, \
                                                                   DPIF_POINT_CLOUD_CARTESIAN_CPU_BYTE_ALIGNMENT))

#define DPC_OBJDET_POINT_CLOUD_SIDE_INFO_BYTE_ALIGNMENT       (MAX(DPU_AOAPROCHWA_POINT_CLOUD_SIDE_INFO_BYTE_ALIGNMENT, \
                                                                   DPIF_POINT_CLOUD_SIDE_INFO_CPU_BYTE_ALIGNMENT))

#define DPC_OBJDET_DET_OBJ_ELEVATION_ANGLE_BYTE_ALIGNMENT     (MAX(DPU_AOAPROCHWA_DET_OBJ_ELEVATION_ANGLE_BYTE_ALIGNMENT, \
                                                                   sizeof(float)))                                                                                                                           

#define DPC_OBJDETRANGE_HWA_MAX_WINDOW_RAM_SIZE_IN_SAMPLES 1024 //SOC_HWA_WINDOW_RAM_SIZE_IN_SAMPLES

/**************************************************************************
 *************************** Extern Definitions ***************************
 **************************************************************************/

extern MmwDemo_MSS_MCB gMmwMssMCB;
extern HWA_Handle gHwaHandle;

/*! L3 RAM Buffer to store the radar cube and other processed signals*/
//#define L3_MEM_SIZE 0xB0000
extern uint8_t gMmwL3[MSS_L3_MEM_SIZE]  __attribute((section(".l3")));

/*! Local RAM buffer for object detection DPC */
extern uint8_t gMmwCoreLocMem[MSS_CORE_LOCAL_MEM_SIZE];

/*! Local RAM buffer size for ID/SBR/CPD */
extern uint8_t gMmwCoreLocMem2[MSS_CORE_LOCAL_MEM2_SIZE] __attribute__((aligned(HeapP_BYTE_ALIGNMENT)));

extern TaskHandle_t    gClassifierTask;
extern StaticTask_t    gClassifierTaskObj;
extern StackType_t     gClassifierTaskStack[];


// LED config
extern uint32_t gGpioBaseAddrLed, gPinNumLed;
extern MMWave_temperatureStats  gTempStats;
extern int32_t MmwDemo_registerChirpAvailableInterrupts(void);

extern int32_t  MmwDemo_rangeBiasRxChPhaseMeasureConfig ();
extern void MmwDemo_rangeBiasRxChPhaseMeasure ();
extern void MmwDemo_ClassifierTask();

/**************************************************************************
 *************************** Local  Definitions ***************************
 **************************************************************************/
void DPC_mss_MsgHandler(uint32_t remoteCoreId, uint16_t localClientId, uint64_t msgValue, int32_t crcStatus, void *arg);

/**************************************************************************
 *************************** Global Definitions ***************************
 **************************************************************************/

/*! @brief     DPU Configuraiton Objects */
DPU_RangeProc_Config    rangeProcDpuCfg;
DPU_Doa3dProc_Config    doa3dProcDpuCfg;
DPU_SNR3DHM_Config      snr3dProcDpuCfg;
DPU_MacroDopplerProc_Config gMacroDoppProcDpuCfg;



/*! @brief     EDMA interrupt objects for DPUs */
Edma_IntrObject             gEdmaIntrObjRng;
Edma_IntrObject             gEdmaIntrObjDoppler;
Edma_IntrObject             gEdmaIntrObjCfar;
//Edma_IntrObject             gEdmaIntrObjAoa;
volatile unsigned long long test;

uint32_t gPeriodicCount = 0;
#include <control/mmwave/mmwave.h>
#include <mmwavelink/include/rl_device.h>
#include <mmwavelink/include/rl_sensor.h>

void DPC_SensorStartClockCallback(ClockP_Object *obj, void *arg)
{
    uint32_t *value = (uint32_t*)arg;

    (*value)++; /* increment number of time's this callback is called */


    /*  Restart Sensor - sensor frame trigger */
    T_RL_API_SENSOR_START_CMD sensStartCmd ={0};
    int32_t retVal = rl_sensSensorStart(0, &sensStartCmd);
    DebugP_assert(retVal == 0);

}


/* Total memory used by the intrusion detection library */
uint32_t gIntrDetectMemoryUsed = 0;
void *inDetect_malloc(uint32_t sizeInBytes)
{
    gIntrDetectMemoryUsed += sizeInBytes;
    return HeapP_alloc(&gMmwMssMCB.CoreLocalRtosHeapObj, sizeInBytes);
}
void inDetect_free(void *pFree, uint32_t sizeInBytes)
{
    gIntrDetectMemoryUsed -= sizeInBytes;
    HeapP_free(&gMmwMssMCB.CoreLocalRtosHeapObj, pFree);
}

/* Total memory used by the feature extraction library */
uint32_t gFeatExtractMemoryUsed = 0;
void *featExtract_malloc(uint32_t sizeInBytes)
{
    gFeatExtractMemoryUsed += sizeInBytes;
    return HeapP_alloc(&gMmwMssMCB.CoreLocalRtosHeapObj, sizeInBytes);
}
void featExtract_free(void *pFree, uint32_t sizeInBytes)
{
    gFeatExtractMemoryUsed -= sizeInBytes;
    HeapP_free(&gMmwMssMCB.CoreLocalRtosHeapObj, pFree);
}

/* Total memory used by the occupancy classifier library */
uint32_t gClassifierMemoryUsed = 0;
void *classifier_malloc(uint32_t sizeInBytes)
{
    gClassifierMemoryUsed += sizeInBytes;
    return HeapP_alloc(&gMmwMssMCB.CoreLocalRtosHeapObj, sizeInBytes);
}
void classifier_free(void *pFree, uint32_t sizeInBytes)
{
    gClassifierMemoryUsed -= sizeInBytes;
    HeapP_free(&gMmwMssMCB.CoreLocalRtosHeapObj, pFree);
}

/* Total memory used by the cnn classifier library */
uint32_t gcnnClassifierMemoryUsed = 0;
void *cnn_classifier_malloc(uint32_t sizeInBytes)
{
    gcnnClassifierMemoryUsed += sizeInBytes;
    return HeapP_alloc(&gMmwMssMCB.CoreLocalRtosHeapObj, sizeInBytes);
}
void cnn_classifier_free(void *pFree, uint32_t sizeInBytes)
{
    gcnnClassifierMemoryUsed -= sizeInBytes;
    HeapP_free(&gMmwMssMCB.CoreLocalRtosHeapObj, pFree);
}

uint32_t coreLocalRtosHeap_memUsage()
{
    uint32_t usedMemSizeInBytes;
    HeapP_MemStats heapStats;

    HeapP_getHeapStats(&gMmwMssMCB.CoreLocalRtosHeapObj, &heapStats);
    usedMemSizeInBytes = sizeof(gMmwCoreLocMem2) - heapStats.availableHeapSpaceInBytes;

    return usedMemSizeInBytes;
}

/**
*  @b Description
*  @n
*        Function configuring and executing DPC
*/
void MmwDemo_dpcTask();

void DPC_ObjectDetection_Profile(DPC_ObjectDetectionRangeHWA_ProfileTimeStamp *stamp)
{
    stamp->timeInUsec = (Cycleprofiler_getTimeStamp() - gMmwMssMCB.stats.frameStartTimeStamp[stamp->rdInd])/ FRAME_REF_TIMER_CLOCK_MHZ;
    stamp->rdInd = (stamp->rdInd + 1) & 0x3;
}


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
            gMmwMssMCB.activeAntennaGeometryCfg.ant[ind] = gMmwMssMCB.antennaGeometryCfg.ant[gMmwMssMCB.rxAntOrder[rxInd] + (txInd * SYS_COMMON_NUM_RX_CHANNEL)];
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

#define DOPPLER_OUTPUT_MAPPING_DOP_ROW_COL   0
#define DOPPLER_OUTPUT_MAPPING_ROW_DOP_COL   1

/**
*  @b Description
*  @n
*    Based on the activeAntennaGeometryCfg configures the table which used to configure
*    Doppler FFT HWA param sets in DoA DPU. THese param sets perform Doppler FFT and
*    at the same time mapping of input antennas into 2D row-column antenna array where columns
*    are in  azimuth dimension, and rows in elevation dimension.
*    It also calculates the size of 2D antenna array, ie. number of rows and number of columns.
*/
int32_t DPC_ObjDet_cfgDopplerParamMapping(DPU_Doa3dProc_HWA_Option_Cfg *dopplerParamCfg,
                                          MmwDemo_antennaGeometryCfg *activeAntennaGeometryCfg,
                                          uint32_t mappingOption,
                                          uint16_t numAntRow,
                                          uint16_t numAntCol,
                                          uint32_t numDopplerBins,
                                          uint32_t numTxAntennas,
                                          uint32_t numRxAntennas)
{
    int32_t ind, indNext, indNextPrev;
    int32_t row, col;
    int32_t dopParamInd;
    int32_t state;
    int16_t BT[DPU_DOA_PROC_MAX_2D_ANT_ARRAY_ELEMENTS];
    int16_t DT[DPU_DOA_PROC_MAX_2D_ANT_ARRAY_ELEMENTS];
    int16_t SCAL[DPU_DOA_PROC_MAX_2D_ANT_ARRAY_ELEMENTS];
    int8_t  DONE[DPU_DOA_PROC_MAX_2D_ANT_ARRAY_ELEMENTS];
    int32_t retVal = 0;
    int32_t rowOffset;

    if (numAntRow * numAntCol > DPU_DOA_PROC_MAX_2D_ANT_ARRAY_ELEMENTS)
    {
        retVal = DPC_OBJECTDETECTION_EANTENNA_GEOMETRY_CFG_FAILED;
        goto exit;
    }

    if (mappingOption == DOPPLER_OUTPUT_MAPPING_DOP_ROW_COL)
    {
        /*For AOA DPU, Output is */
        rowOffset =  numAntCol;
    }
    else if (mappingOption == DOPPLER_OUTPUT_MAPPING_ROW_DOP_COL)
    {
        rowOffset =  numDopplerBins * numAntCol;
    }
    else
    {
        retVal = DPC_OBJECTDETECTION_EANTENNA_GEOMETRY_CFG_FAILED;
        goto exit;
    }

    /* Initialize tables */
    for (ind = 0; ind < (numAntRow * numAntCol); ind++)
    {
        BT[ind] = 0;
        SCAL[ind] = 0;
        DONE[ind] = 0;
    }

    for (ind = 0; ind < (numTxAntennas * numRxAntennas); ind++)
    {
        row = activeAntennaGeometryCfg->ant[ind].row;
        col = activeAntennaGeometryCfg->ant[ind].col;
        BT[row * numAntCol + col] = ind;
        SCAL[row * numAntCol + col] = 1;
    }
    for (row = 0; row < numAntRow; row++)
    {
        for (col = 0; col < numAntCol; col++)
        {
            ind = row * numAntCol + col;
            DT[ind] = row * rowOffset + col;
        }
    }


    /* Configure Doppler HWA mapping params for antenna mapping */
    dopParamInd = 0;
    dopplerParamCfg->numDopFftParams = 0;
    for (ind = 0; ind < (numAntRow * numAntCol); ind++)
    {
        if (!DONE[ind])
        {
            if(dopParamInd == DPU_DOA3DPROC_MAX_NUM_DOP_FFFT_PARAMS)
            {
                retVal = DPC_OBJECTDETECTION_EANTENNA_GEOMETRY_CFG_FAILED;
                goto exit;
            }

            DONE[ind] = 1;
            dopplerParamCfg->numDopFftParams++;
            dopplerParamCfg->dopFftCfg[dopParamInd].srcBcnt = 1;
            dopplerParamCfg->dopFftCfg[dopParamInd].scale = SCAL[ind];
            if (dopplerParamCfg->dopFftCfg[dopParamInd].scale == 0)
            {
                dopplerParamCfg->dopFftCfg[dopParamInd].srcAddrOffset = 0;
            }
            else
            {
                dopplerParamCfg->dopFftCfg[dopParamInd].srcAddrOffset = BT[ind];
            }
            dopplerParamCfg->dopFftCfg[dopParamInd].dstAddrOffset = DT[ind];
            state = 1;//STATE_SECOND:
            for (indNext = ind+1; indNext < (numAntRow * numAntCol); indNext++)
            {

                if (!DONE[indNext] && (dopplerParamCfg->dopFftCfg[dopParamInd].scale == SCAL[indNext]))
                {
                    switch (state)
                    {
                        case 1://STATE_SECOND:
                            dopplerParamCfg->dopFftCfg[dopParamInd].srcBcnt++;
                            DONE[indNext] = 1;
                            if (SCAL[indNext] == 1)
                            {
                                dopplerParamCfg->dopFftCfg[dopParamInd].srcBidx = BT[indNext] - dopplerParamCfg->dopFftCfg[dopParamInd].srcAddrOffset;
                            }
                            else
                            {
                                dopplerParamCfg->dopFftCfg[dopParamInd].srcBidx = 0;
                            }
                            dopplerParamCfg->dopFftCfg[dopParamInd].dstBidx = DT[indNext] - DT[ind];
                            indNextPrev = indNext;
                            state = 2;//STATE_NEXT:
                            break;
                        case 2://STATE_NEXT:
                            if (SCAL[indNext] == 1)
                            {
                                if ((dopplerParamCfg->dopFftCfg[dopParamInd].srcBidx == (BT[indNext] - BT[indNextPrev])) &&
                                    (dopplerParamCfg->dopFftCfg[dopParamInd].dstBidx == (DT[indNext] - DT[indNextPrev])))
                                {
                                    DONE[indNext] = 1;
                                    dopplerParamCfg->dopFftCfg[dopParamInd].srcBcnt++;
                                    indNextPrev = indNext;
                                }
                            }
                            else
                            {
                                if (dopplerParamCfg->dopFftCfg[dopParamInd].dstBidx == (DT[indNext] - DT[indNextPrev]))
                                {
                                    DONE[indNext] = 1;
                                    dopplerParamCfg->dopFftCfg[dopParamInd].srcBcnt++;
                                    indNextPrev = indNext;
                                }
                            }
                            break;
                    }
                }
            }
            dopParamInd++;
        }
    }

    dopplerParamCfg->numDopFftParams = dopParamInd;

exit:
    return retVal;
}

/**
*  @b Description
*  @n
*    Range processing DPU Initialization
*/
void rangeProc_dpuInit()
{
    int32_t errorCode = 0;
    DPU_RangeProc_InitParams initParams;
    initParams.hwaHandle = gHwaHandle;

    /* generate the dpu handler*/
    gMmwMssMCB.rangeProcDpuHandle = DPU_RangeProc_init(&initParams, &errorCode);
    if (gMmwMssMCB.rangeProcDpuHandle == NULL)
    {
        CLI_write("Error: RangeProc DPU initialization returned error %d\n", errorCode);
        DebugP_assert(0);
        return;
    }
}

/**
*  @b Description
*  @n
*    DOA3D DPU Initialization
*/
void doa3dProc_dpuInit()
{
    int32_t  errorCode = 0;
    DPU_Doa3dProc_InitParams initParams;
    initParams.hwaHandle =  gHwaHandle;
    /* generate the dpu handler*/
    gMmwMssMCB.doa3dProcDpuHandle =  DPU_Doa3dProc_init(&initParams, &errorCode);
    if (gMmwMssMCB.doa3dProcDpuHandle == NULL)
    {
        CLI_write ("Error: Doa3DProc DPU initialization returned error %d\n", errorCode);
        DebugP_assert (0);
        return;
    }
}


/**
*  @b Description
*  @n
*    SNR3D DPU Initialization
*/
void snr3dProc_dpuInit()
{
    int32_t  errorCode = 0;
    DPU_SNR3DHM_InitParams initParams;
    initParams.hwaHandle =  gHwaHandle;
    /* generate the dpu handler*/
    gMmwMssMCB.snr3dProcDpuHandle =  DPU_SNR3DHM_init(&initParams, &errorCode);
    if (gMmwMssMCB.snr3dProcDpuHandle == NULL)
    {
        CLI_write ("Error: SNR3DProc DPU initialization returned error %d\n", errorCode);
        DebugP_assert (0);
        return;
    }
}

/**
*  @b Description
*  @n
*    MacroDoppler DPU Initialization
*/
void macroDoppProc_dpuInit()
{
    int32_t  errorCode = 0;
    DPU_MacroDopplerProc_InitParams initParams;
    initParams.hwaHandle =  gHwaHandle;
    /* generate the dpu handler*/
    gMmwMssMCB.macroDoppProcDpuHandle =  DPU_MacroDopplerProc_init(&initParams, &errorCode);
    if (gMmwMssMCB.macroDoppProcDpuHandle == NULL)
    {
        CLI_write ("Error: MacroDopplerProc DPU initialization returned error %d\n", errorCode);
        DebugP_assert (0);
        return;
    }
}
/**
*  @b Description
*  @n
*    Classifier DPU Initialization
*/
void classifierProc_dpuInit()
{
    int32_t  errorCode = 0;
    gMmwMssMCB.classifierDpuHandle =  DPU_ClassifierProc_init(&errorCode);
    if (gMmwMssMCB.classifierDpuHandle == NULL)
    {
        CLI_write ("Error: ClassifierProc DPU initialization returned error %d\n", errorCode);
        DebugP_assert (0);
        return;
    }
}

volatile uint32_t gOverlappedHwaParamSets = 1;
/**
*  @b Description
*  @n
*    Based on the configuration, set up the range processing DPU configurations
*/
int32_t RangeProc_configParser(uint32_t *hwaParamSetStartIdx,
                               uint32_t *hwaWinRamOffset)
{

    int32_t retVal = 0;
    DPU_RangeProc_HW_Resources *pHwConfig;
    DPU_RangeProc_StaticConfig  * params;
    uint32_t index;
    uint32_t bytesPerRxChan;
    uint32_t winGenLenInSamples;

    /* Rangeproc DPU */
    pHwConfig = &rangeProcDpuCfg.hwRes;
    params = &rangeProcDpuCfg.staticCfg;

    memset((void *)&rangeProcDpuCfg, 0, sizeof(DPU_RangeProc_Config));

    /* HWA configurations, not related to per test, common to all test */
    pHwConfig->hwaCfg.paramSetStartIdx = *hwaParamSetStartIdx;
    //pHwConfig->hwaCfg.numParamSet = DPU_RANGEPROC_NUM_HWA_PARAM_SETS;
    pHwConfig->hwaCfg.dataInputMode = DPU_RangeProc_InputMode_ISOLATED;
    for (index = 0; index < 4; index ++)
    {
        pHwConfig->hwaCfg.dmaTrigSrcChan[index] = DPC_ObjDet_HwaDmaTrigSrcChanPoolAlloc(&gMmwMssMCB.HwaDmaChanPoolObj);
    }

    /* static configuration - window */
    /* Generating 1D window, allocate first */
    winGenLenInSamples = ((gMmwMssMCB.mmWaveCfg.profileComCfg.numOfAdcSamples +1 ) / 2); //symmetric window, for real samples;
    params->windowSize = sizeof(uint32_t) * winGenLenInSamples;
    params->window     = (int32_t *)DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.CoreLocalRamObj,
                                                            params->windowSize,
                                                            sizeof(uint32_t));
    if (params->window == NULL)
    {
       retVal = DPC_OBJECTDETECTION_ENOMEM__CORE_LOCAL_RAM_RANGE_HWA_WINDOW;
       goto exit;
    }

    if (!gMmwMssMCB.oneTimeConfigDone)
    {
        mathUtils_genWindow((uint32_t *)params->window,
                            (uint32_t) gMmwMssMCB.mmWaveCfg.profileComCfg.numOfAdcSamples,
                            params->windowSize/sizeof(uint32_t),
                            DPC_DPU_RANGEPROC_FFT_WINDOW_TYPE,
                            DPC_OBJDET_QFORMAT_RANGE_FFT);
    }

    /* Allocate memory for the save location HWA param sets */
    pHwConfig->hwaCfg.hwaParamsSaveLoc.sizeBytes = DPU_RANGEPROC_NUM_HWA_PARAM_SETS * HWA_NUM_REG_PER_PARAM_SET * sizeof(uint32_t);
    pHwConfig->hwaCfg.hwaParamsSaveLoc.data = DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.CoreLocalRamObj,
                                                                      pHwConfig->hwaCfg.hwaParamsSaveLoc.sizeBytes,
                                                                      sizeof(uint32_t));
    if (pHwConfig->hwaCfg.hwaParamsSaveLoc.data == NULL)
    {
        retVal = DPC_OBJECTDETECTION_ENOMEM__RANGE_HWA_PARAM_SAVE_LOC;
        goto exit;
    }

    /* hwres - hwa */
    /* Use ISOLATED mode to support CBUFF in future */
    pHwConfig->hwaCfg.dataInputMode = DPU_RangeProc_InputMode_ISOLATED;
    pHwConfig->hwaCfg.hwaWinSym = HWA_FFT_WINDOW_SYMMETRIC;


    pHwConfig->hwaCfg.hwaWinRamOffset = (uint16_t) *hwaWinRamOffset;
    if ((pHwConfig->hwaCfg.hwaWinRamOffset + winGenLenInSamples) > DPC_OBJDETRANGE_HWA_MAX_WINDOW_RAM_SIZE_IN_SAMPLES)
    {
        retVal = DPC_OBJECTDETECTION_ENOMEM__HWA_WINDOW_RAM_INTERNAL;
        goto exit;
    }
    *hwaWinRamOffset += winGenLenInSamples;


    /* edma configuration */
    pHwConfig->edmaHandle  = gEdmaHandle[0];
    /* edma configuration depends on the interleave or non-interleave */


    /* adc buffer buffer, format fixed, interleave, size will change */
    params->ADCBufData.dataProperty.dataFmt = DPIF_DATAFORMAT_REAL16;
    params->ADCBufData.dataProperty.adcBits = 2U; // 12-bit only
    params->ADCBufData.dataProperty.numChirpsPerChirpEvent = 1U;

    #if (CLI_REMOVAL == 0)
    if(gMmwMssMCB.adcDataSourceCfg.source == 0)
    {
        params->ADCBufData.data = (void *)CSL_DSS_ADCBUF_READ_U_BASE;
    }
    else
    {
        gMmwMssMCB.adcTestBuff  = (uint8_t *) DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.L3RamObj,
                                                                      gMmwMssMCB.numRxAntennas * gMmwMssMCB.mmWaveCfg.profileComCfg.numOfAdcSamples * sizeof(int16_t),
                                                                      sizeof(uint32_t));
        if(gMmwMssMCB.adcTestBuff == NULL)
        {
            retVal = DPC_OBJECTDETECTION_ENOMEM__L3_RAM_ADC_TEST_BUFF;
            goto exit;
        }
        params->ADCBufData.data = (void *)gMmwMssMCB.adcTestBuff;

    }
    #else
    params->ADCBufData.data = (void *)CSL_DSS_ADCBUF_READ_U_BASE;
    #endif

    params->numTxAntennas = (uint8_t) gMmwMssMCB.numTxAntennas;
    params->numVirtualAntennas = (uint8_t) (gMmwMssMCB.numTxAntennas * gMmwMssMCB.numRxAntennas);
    params->numRangeBins = gMmwMssMCB.numRangeBins;
    params->numChirpsPerOneFrame = gMmwMssMCB.sigProcChainCommonCfg.numOfBurstsInFrame * gMmwMssMCB.mmWaveCfg.frameCfg.numOfChirpsInBurst;
    params->numFramesPerSlidingWindow = gMmwMssMCB.sigProcChainCommonCfg.numFrmPerSlidingWindow;
    params->numChirpsPerSlidingWindow = params->numChirpsPerOneFrame * params->numFramesPerSlidingWindow;
    params->frmCntrInSlidingWindowInitVal = gMmwMssMCB.frmCntrInSlidingWindowInitVal;

    if (gOverlappedHwaParamSets)
    {
        params->loadHwaParamSetsBeforeExec = true;
    }
    else
    {
        params->loadHwaParamSetsBeforeExec = false;
    }

    /* windowing */
    params->ADCBufData.dataProperty.numRxAntennas = (uint8_t) gMmwMssMCB.numRxAntennas;
    params->ADCBufData.dataProperty.numAdcSamples = gMmwMssMCB.mmWaveCfg.profileComCfg.numOfAdcSamples;
    params->ADCBufData.dataProperty.interleave = DPIF_RXCHAN_NON_INTERLEAVE_MODE;

    if (!gMmwMssMCB.oneTimeConfigDone)
    {
        mathUtils_genWindow((uint32_t *)params->window,
                            (uint32_t) params->ADCBufData.dataProperty.numAdcSamples,
                            params->windowSize/sizeof(uint32_t),
                            DPC_DPU_RANGEPROC_FFT_WINDOW_TYPE,
                            DPC_OBJDET_QFORMAT_RANGE_FFT);
    }

    //params->rangeFFTtuning.fftOutputDivShift = 2;
    //params->rangeFFTtuning.numLastButterflyStagesToScale = 0; /* no scaling needed as ADC is 16-bit and we have 8 bits to grow */

    params->rangeFftSize = mathUtils_pow2roundup(params->ADCBufData.dataProperty.numAdcSamples);

    bytesPerRxChan = params->ADCBufData.dataProperty.numAdcSamples * sizeof(uint16_t);
    bytesPerRxChan = (bytesPerRxChan + 15) / 16 * 16;

    for (index = 0; index < params->ADCBufData.dataProperty.numRxAntennas; index++)
    {
        params->ADCBufData.dataProperty.rxChanOffset[index] = index * bytesPerRxChan;
    }

    params->prolonedBurstingMode = gMmwMssMCB.prolongedBurstingMode;

    /* Data Input EDMA */
    if (gMmwMssMCB.prolongedBurstingMode)
    {
        /* Prolonged (continuous) bursting mode chirp available event triggers EDMA event selector which then chains to EDMA input*/
        pHwConfig->edmaInCfg.dataIn.channel                = DPC_OBJDET_PROLONGED_BURSTING_IN_CH;
    }
    else
    {
        /* Normal bursting mode - chirp available event triggers input EDMA */
        pHwConfig->edmaInCfg.dataIn.channel                = DPC_OBJDET_DPU_RANGEPROC_EDMAIN_CH;
    }
    pHwConfig->edmaInCfg.dataIn.shadowPramId          = DPC_OBJDET_DPU_RANGEPROC_EDMAIN_SHADOW;
    pHwConfig->edmaInCfg.dataIn.eventQueue             = DPC_OBJDET_DPU_RANGEPROC_EDMAIN_EVENT_QUE;
    pHwConfig->edmaInCfg.dataInSignature.channel       = DPC_OBJDET_DPU_RANGEPROC_EDMAIN_SIG_CH;
    pHwConfig->edmaInCfg.dataInSignature.shadowPramId = DPC_OBJDET_DPU_RANGEPROC_EDMAIN_SIG_SHADOW;
    pHwConfig->edmaInCfg.dataInSignature.eventQueue    = DPC_OBJDET_DPU_RANGEPROC_EDMAIN_SIG_EVENT_QUE;


    /* Ping Output EDMA */
    pHwConfig->edmaOutCfg.u.fmt1.dataOutPing.channel       = DPC_OBJDET_DPU_RANGEPROC_EDMAOUT_FMT1_PING_CH;
    pHwConfig->edmaOutCfg.u.fmt1.dataOutPing.shadowPramId = DPC_OBJDET_DPU_RANGEPROC_EDMAOUT_FMT1_PING_SHADOW;
    pHwConfig->edmaOutCfg.u.fmt1.dataOutPing.eventQueue    = DPC_OBJDET_DPU_RANGEPROC_EDMAOUT_FMT1_PING_EVENT_QUE;

    /* Pong Output EDMA */
    pHwConfig->edmaOutCfg.u.fmt1.dataOutPong.channel       = DPC_OBJDET_DPU_RANGEPROC_EDMAOUT_FMT1_PONG_CH;
    pHwConfig->edmaOutCfg.u.fmt1.dataOutPong.shadowPramId = DPC_OBJDET_DPU_RANGEPROC_EDMAOUT_FMT1_PONG_SHADOW;
    pHwConfig->edmaOutCfg.u.fmt1.dataOutPong.eventQueue    = DPC_OBJDET_DPU_RANGEPROC_EDMAOUT_FMT1_PONG_EVENT_QUE;

    /* Signature EDMA */
    pHwConfig->edmaOutCfg.dataOutSignature.channel       = DPC_OBJDET_DPU_RANGEPROC_EDMAOUT_SIG_CH;
    pHwConfig->edmaOutCfg.dataOutSignature.shadowPramId = DPC_OBJDET_DPU_RANGEPROC_EDMAOUT_SIG_SHADOW;
    pHwConfig->edmaOutCfg.dataOutSignature.eventQueue    = DPC_OBJDET_DPU_RANGEPROC_EDMAOUT_SIG_EVENT_QUE;


    /* Radar cube allocation */
    gMmwMssMCB.radarCube.dataSize = params->numRangeBins * params->ADCBufData.dataProperty.numRxAntennas * sizeof(cmplx16ReIm_t) * params->numChirpsPerSlidingWindow;
    gMmwMssMCB.radarCube.data  = (cmplx16ImRe_t *) DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.L3RamObj,
                                                                                 gMmwMssMCB.radarCube.dataSize,
                                                                                 sizeof(uint32_t));
    if(gMmwMssMCB.radarCube.data == NULL)
    {
        retVal = DPC_OBJECTDETECTION_ENOMEM__L3_RAM_RADAR_CUBE;
        goto exit;
    }

    if (gMmwMssMCB.runningMode == DPC_RUNNING_MODE_INDET)
    {
        params->butterflyScalingBitMask = 0;
        gMmwMssMCB.radarCube.datafmt = DPIF_RADARCUBE_FORMAT_6;
    }
    else if ((gMmwMssMCB.runningMode == DPC_RUNNING_MODE_SBR) || (gMmwMssMCB.runningMode == DPC_RUNNING_MODE_CPD))
    {
        //uint32_t numOfChirpsAccum = gMmwMssMCB.mmWaveCfg.frameCfg.numOfChirpsAccum;
        //if (numOfChirpsAccum == 0)
        //{
        //    numOfChirpsAccum = 1;
        //}
        params->butterflyScalingBitMask = 0; //(1 << mathUtils_ceilLog2(numOfChirpsAccum)) - 1;
        gMmwMssMCB.radarCube.datafmt = DPIF_RADARCUBE_FORMAT_2;
    }
    else
    {
        retVal = DPC_OBJECTDETECTION_ERADAR_CUBE_FORMAT_NOT_SUPPORTED;
        goto exit;
    }
    rangeProcDpuCfg.hwRes.radarCube = gMmwMssMCB.radarCube;

exit:
    return retVal;
}

/**
*  @b Description
*  @n
*    Based on the configuration, set up the doa3d processing DPU configurations
*/
int32_t Doa3dProc_configParser(uint32_t *hwaParamSetStartIdx,
                               uint32_t *hwaWinRamOffset)
{

    /* Doaproc DPU */
    DPU_Doa3dProc_EdmaCfg *edmaCfg;
    DPU_Doa3dProc_HwaCfg *hwaCfg;
    int32_t winGenLen, i;
    int32_t retVal = 0;
    DPU_Doa3dProc_StaticConfig  *doaStaticCfg;
    DPU_Doa3dProc_HW_Resources  *hwRes;
    uint32_t numOutputDopplerBins;


    memset((void *)&doa3dProcDpuCfg, 0, sizeof(DPU_Doa3dProc_Config));

    hwRes = &doa3dProcDpuCfg.hwRes;
    doaStaticCfg = &doa3dProcDpuCfg.staticCfg;
    edmaCfg = &hwRes->edmaCfg;
    hwaCfg = &hwRes->hwaCfg;

    doaStaticCfg->numDopplerChirps   = gMmwMssMCB.sigProcChainCommonCfg.numFrmPerSlidingWindow * gMmwMssMCB.sigProcChainCommonCfg.numOfBurstsInFrame * gMmwMssMCB.mmWaveCfg.frameCfg.numOfChirpsInBurst / gMmwMssMCB.numTxAntennas;
    doaStaticCfg->numDopplerBins     = mathUtils_pow2roundup(doaStaticCfg->numDopplerChirps);
    doaStaticCfg->numRangeBins       = gMmwMssMCB.numRangeBins;
    doaStaticCfg->numRxAntennas      = (uint8_t) gMmwMssMCB.numRxAntennas;
    doaStaticCfg->numVirtualAntennas = (uint8_t) (gMmwMssMCB.numTxAntennas * gMmwMssMCB.numRxAntennas);
    doaStaticCfg->log2NumDopplerBins = mathUtils_ceilLog2(doaStaticCfg->numDopplerBins);
    doaStaticCfg->numTxAntennas      = (uint8_t) gMmwMssMCB.numTxAntennas;

    doaStaticCfg->selectCoherentPeakInDopplerDim = gMmwMssMCB.intrusionSigProcChainCfg.selectCoherentPeakInDopplerDim;
    doaStaticCfg->azimuthFftSize        = gMmwMssMCB.intrusionSigProcChainCfg.azimuthFftSize;
    doaStaticCfg->elevationFftSize        = gMmwMssMCB.intrusionSigProcChainCfg.elevationFftSize;
    doaStaticCfg->isStaticClutterRemovalEnabled = gMmwMssMCB.staticClutterRemovalEnable;
    doaStaticCfg->isRxChGainPhaseCompensationEnabled   = true;

    if (gOverlappedHwaParamSets)
    {
        doaStaticCfg->loadHwaParamSetsBeforeExec = true;
    }
    else
    {
        doaStaticCfg->loadHwaParamSetsBeforeExec = false;
    }

    /* Select active antennas from available antennas and calculate number of antennas rows and columns */
    MmwDemo_calcActiveAntennaGeometry();

    doaStaticCfg->numAntRow = gMmwMssMCB.numAntRow;
    doaStaticCfg->numAntCol = gMmwMssMCB.numAntCol;

    if (doaStaticCfg->isStaticClutterRemovalEnabled)
    {
        numOutputDopplerBins = doaStaticCfg->numDopplerBins - 1;
    }
    else
    {
        numOutputDopplerBins = doaStaticCfg->numDopplerBins;
    }

    /* Populate doaRngGateCfg structure for DOPPLER/MAPPING configuration */
    DPC_ObjDet_cfgDopplerParamMapping(&hwRes->hwaCfg.doaRngGateCfg,
                                      &gMmwMssMCB.activeAntennaGeometryCfg,
                                      DOPPLER_OUTPUT_MAPPING_ROW_DOP_COL,
                                      gMmwMssMCB.numAntRow,
                                      gMmwMssMCB.numAntCol,
                                      numOutputDopplerBins,
                                      gMmwMssMCB.numTxAntennas,
                                      gMmwMssMCB.numRxAntennas);

    if (gMmwMssMCB.numAntRow == 1)
    {
        doaStaticCfg->angleDimension = 1;
    }
    else
    {
        doaStaticCfg->angleDimension = 2;
    }


    /* Allocate 3D Detection Matrix */
    gMmwMssMCB.detMatrix.dataSize = gMmwMssMCB.numRangeBins * doaStaticCfg->azimuthFftSize * doaStaticCfg->elevationFftSize * sizeof(uint32_t);
    gMmwMssMCB.detMatrix.data = DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.L3RamObj,
                                                        gMmwMssMCB.detMatrix.dataSize,
                                                        sizeof(uint32_t));
    if (gMmwMssMCB.detMatrix.data == NULL)
    {
        retVal = DPC_OBJECTDETECTION_ENOMEM__L3_RAM_DET_MATRIX;
        goto exit;
    }

    /* Allocate Range Profile */
    gMmwMssMCB.rangeProfile = DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.CoreLocalRamObj,
                                                      gMmwMssMCB.numRangeBins * sizeof(uint32_t),
                                                      sizeof(uint32_t));
    if (gMmwMssMCB.rangeProfile == NULL)
    {
        retVal = DPC_OBJECTDETECTION_ENOMEM__CORE_LOCAL_RAM_RANGE_PROFILE;
        goto exit;
    }

    if (doaStaticCfg->selectCoherentPeakInDopplerDim == 2)
    {
        /* Allocate Doppler index matrix */
        gMmwMssMCB.dopplerIndexMatrix.dataSize = gMmwMssMCB.numRangeBins * doaStaticCfg->azimuthFftSize * doaStaticCfg->elevationFftSize * sizeof(uint8_t);
        gMmwMssMCB.dopplerIndexMatrix.data = DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.L3RamObj,
                                                                     gMmwMssMCB.dopplerIndexMatrix.dataSize,
                                                                     sizeof(uint8_t));
        if (gMmwMssMCB.dopplerIndexMatrix.data == NULL)
        {
            retVal = DPC_OBJECTDETECTION_ENOMEM__L3_RAM_DET_MATRIX;
            goto exit;
        }
    }
    else
    {
        memset(&gMmwMssMCB.dopplerIndexMatrix, 0, sizeof(DPIF_DetMatrix));
    }

    /* hwRes - copy these structures */
    hwRes->radarCube = gMmwMssMCB.radarCube;
    hwRes->dopplerIndexMatrix = gMmwMssMCB.dopplerIndexMatrix;
    hwRes->detMatrix = gMmwMssMCB.detMatrix;

    /* hwRes - edmaCfg */
    edmaCfg->edmaHandle = gEdmaHandle[0];

    /* edmaIn - ping - minor motion*/
    edmaCfg->edmaIn.chunk[0].channel =            DPC_OBJDET_DPU_DOAPROC_EDMAIN_PING_CH;
    edmaCfg->edmaIn.chunk[0].shadowPramId =      DPC_OBJDET_DPU_DOAPROC_EDMAIN_PING_SHADOW;
    edmaCfg->edmaIn.chunk[0].eventQueue =         DPC_OBJDET_DPU_DOAPROC_EDMAIN_PING_EVENT_QUE;

    /* edmaIn - pong - minor motion*/
    edmaCfg->edmaIn.chunk[1].channel =            DPC_OBJDET_DPU_DOAPROC_EDMAIN_PONG_CH;
    edmaCfg->edmaIn.chunk[1].shadowPramId =      DPC_OBJDET_DPU_DOAPROC_EDMAIN_PONG_SHADOW;
    edmaCfg->edmaIn.chunk[1].eventQueue =         DPC_OBJDET_DPU_DOAPROC_EDMAIN_PONG_EVENT_QUE;

    /* edmaHotSig */
    edmaCfg->edmaHotSig.channel =             DPC_OBJDET_DPU_DOAPROC_EDMA_HOT_SIG_CH;
    edmaCfg->edmaHotSig.shadowPramId =       DPC_OBJDET_DPU_DOAPROC_EDMA_HOT_SIG_SHADOW;
    edmaCfg->edmaHotSig.eventQueue =          DPC_OBJDET_DPU_DOAPROC_EDMA_HOT_SIG_EVENT_QUE;




    /* edmaOut - Detection Matrix */
    edmaCfg->edmaDetMatOut.channel =           DPC_OBJDET_DPU_DOAPROC_EDMAOUT_DET_MATRIX_CH;
    edmaCfg->edmaDetMatOut.shadowPramId =     DPC_OBJDET_DPU_DOAPROC_EDMAOUT_DET_MATRIX_SHADOW;
    edmaCfg->edmaDetMatOut.eventQueue =        DPC_OBJDET_DPU_DOAPROC_EDMAOUT_DET_MATRIX_EVENT_QUE;

    /* edmaOut - Elevation Index Matrix */
    edmaCfg->elevIndMatOut.channel =       DPC_OBJDET_DPU_DOAPROC_EDMAOUT_ELEVIND_MATRIX_CH;
    edmaCfg->elevIndMatOut.shadowPramId = DPC_OBJDET_DPU_DOAPROC_EDMAOUT_ELEVIND_MATRIX_SHADOW;
    edmaCfg->elevIndMatOut.eventQueue =    DPC_OBJDET_DPU_DOAPROC_EDMAOUT_ELEVIND_MATRIX_EVENT_QUE;

    /* edmaOut - Doppler Index Matrix */
    edmaCfg->dopIndMatOut.channel =        DPC_OBJDET_DPU_DOAPROC_EDMAOUT_DOPIND_MATRIX_CH;
    edmaCfg->dopIndMatOut.shadowPramId =  DPC_OBJDET_DPU_DOAPROC_EDMAOUT_DOPIND_MATRIX_SHADOW;
    edmaCfg->dopIndMatOut.eventQueue =     DPC_OBJDET_DPU_DOAPROC_EDMAOUT_DOPIND_MATRIX_EVENT_QUE;

    edmaCfg->edmaInterLoopOut.channel =       DPC_OBJDET_DPU_DOAPROC_INTER_LOOP_EDMAOUT_DET_MATRIX_CH;
    edmaCfg->edmaInterLoopOut.shadowPramId = DPC_OBJDET_DPU_DOAPROC_INTER_LOOP_EDMAOUT_DET_MATRIX_SHADOW;
    edmaCfg->edmaInterLoopOut.eventQueue =    DPC_OBJDET_DPU_DOAPROC_INTER_LOOP_EDMAOUT_DET_MATRIX_EVENT_QUE;

    edmaCfg->edmaInterLoopIn.channel =       DPC_OBJDET_DPU_DOAPROC_INTER_LOOP_EDMAIN_CH;
    edmaCfg->edmaInterLoopIn.shadowPramId = DPC_OBJDET_DPU_DOAPROC_INTER_LOOP_EDMAIN_SHADOW;
    edmaCfg->edmaInterLoopIn.eventQueue =    DPC_OBJDET_DPU_DOAPROC_INTER_LOOP_EDMAIN_EVENT_QUE;

    edmaCfg->edmaInterLoopHotSig.channel =       DPC_OBJDET_DPU_DOAPROC_INTER_LOOP_EDMA_HOT_SIG_CH;
    edmaCfg->edmaInterLoopHotSig.shadowPramId = DPC_OBJDET_DPU_DOAPROC_INTER_LOOP_EDMA_HOT_SIG_SHADOW;
    edmaCfg->edmaInterLoopHotSig.eventQueue =    DPC_OBJDET_DPU_DOAPROC_INTER_LOOP_EDMA_HOT_SIG_EVENT_QUE;

    edmaCfg->edmaInterLoopChainBack.channel =       DPC_OBJDET_DPU_DOAPROC_INTER_LOOP_EDMA_CHAIN_BACK_CH;
    edmaCfg->edmaInterLoopChainBack.shadowPramId = DPC_OBJDET_DPU_DOAPROC_INTER_LOOP_EDMA_CHAIN_BACK_SHADOW;
    edmaCfg->edmaInterLoopChainBack.eventQueue =    DPC_OBJDET_DPU_DOAPROC_INTER_LOOP_EDMA_CHAIN_BACK_EVENT_QUE;

    /* hwaCfg - window */
    //Share FFT window between azimuth and elevation FFT, window = [+1 -1 +1 -1 ... ]
    winGenLen = (doaStaticCfg->azimuthFftSize > doaStaticCfg->elevationFftSize) ? doaStaticCfg->azimuthFftSize : doaStaticCfg->elevationFftSize;
    hwaCfg->windowSize = winGenLen * sizeof(int32_t);

    hwaCfg->window = (int32_t *)DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.CoreLocalRamObj,
                                                        hwaCfg->windowSize,
                                                        sizeof(uint32_t));
    if (hwaCfg->window == NULL)
    {
        retVal = DPC_OBJECTDETECTION_ENOMEM__CORE_LOCAL_RAM_DOA_HWA_WINDOW;
        goto exit;
    }

    /*Alternate 1,-1,...*/
    for (i=0; i<winGenLen; i++)
    {
        hwaCfg->window[i] = (1 - 2 * (i & 0x1)) * ((1<<17) - 1);
    }
    hwaCfg->winRamOffset = (uint16_t) *hwaWinRamOffset;
    hwaCfg->winSym = HWA_FFT_WINDOW_NONSYMMETRIC;
    *hwaWinRamOffset += winGenLen;

    hwaCfg->paramSetStartIdx = *hwaParamSetStartIdx;
    hwaCfg->dmaTrigSrcChan = DPC_ObjDet_HwaDmaTrigSrcChanPoolAlloc(&gMmwMssMCB.HwaDmaChanPoolObj);

    /* Rx compensation coefficients */
    {
        int32_t rxInd, txInd;
        int32_t ind = 0;
        doaStaticCfg->compRxChanCfg.rangeBias = gMmwMssMCB.compRxChannelBiasCfg.rangeBias;
        for (txInd = 0; txInd < doaStaticCfg->numTxAntennas; txInd++)
        {
            for (rxInd = 0; rxInd < doaStaticCfg->numRxAntennas; rxInd++)
            {
                doaStaticCfg->compRxChanCfg.rxChPhaseComp[ind++] = gMmwMssMCB.compRxChannelBiasCfg.rxChPhaseComp[gMmwMssMCB.rxAntOrder[rxInd] + (txInd * SYS_COMMON_NUM_RX_CHANNEL)];
            }
        }
    }

    /* Allocate memory for the save location HWA param sets */
    hwRes->hwaCfg.hwaParamsSaveLoc.sizeBytes = DPU_DOA3DPROC_MAX_NUM_HWA_PARAMSET * HWA_NUM_REG_PER_PARAM_SET * sizeof(uint32_t);
    hwRes->hwaCfg.hwaParamsSaveLoc.data = DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.CoreLocalRamObj,
                                                                  hwRes->hwaCfg.hwaParamsSaveLoc.sizeBytes,
                                                                  sizeof(uint32_t));
    if (hwRes->hwaCfg.hwaParamsSaveLoc.data == NULL)
    {
        retVal = DPC_OBJECTDETECTION_ENOMEM__DOA3D_HWA_PARAM_SAVE_LOC;
        goto exit;
    }

exit:
    return retVal;
}

/**
*  @b Description
*  @n
*    Based on the configuration, set up the snr3d processing DPU configurations
*/
int32_t Snr3dProc_configParser(uint32_t *hwaParamSetStartIdx,
                               uint32_t *hwaWinRamOffset)
{

    /* Doaproc DPU */
    int32_t retVal = 0;
    DPU_SNR3DHM_StaticConfig  *snr3dStaticCfg;
    DPU_SNR3DHM_HW_Resources  *hwRes;
    hwRes = &snr3dProcDpuCfg.res;
    snr3dStaticCfg = &snr3dProcDpuCfg.staticCfg;

    memset((void *)&snr3dProcDpuCfg, 0, sizeof(DPU_SNR3DHM_Config));

    /* static config */
    snr3dStaticCfg->numDopplerBins     = gMmwMssMCB.numDopplerBins;
    snr3dStaticCfg->numRangeBins       = gMmwMssMCB.numRangeBins;

    snr3dStaticCfg->azimuthFftSize     = gMmwMssMCB.intrusionSigProcChainCfg.azimuthFftSize;
    snr3dStaticCfg->elevationFftSize   = gMmwMssMCB.intrusionSigProcChainCfg.elevationFftSize;
    snr3dStaticCfg->secondPassCfar =    gMmwMssMCB.snr3dCfarScndPassCfg.enabled;
    snr3dStaticCfg->divShiftBeforeLog = 2; //ToDo tweak this...

    if (gOverlappedHwaParamSets)
    {
        snr3dStaticCfg->loadHwaParamSetsBeforeExec = true;
    }
    else
    {
        snr3dStaticCfg->loadHwaParamSetsBeforeExec = false;
    }


    /* dynamic config */
    snr3dProcDpuCfg.dynCfg.cfarCfg   = &gMmwMssMCB.snr3dCfarCfg;
    snr3dProcDpuCfg.dynCfg.cfarScndPassCfg   = &gMmwMssMCB.snr3dCfarScndPassCfg;


    /* Allocate SNR Output Matrix */
    gMmwMssMCB.snrOutMatrix.dataSize = gMmwMssMCB.numRangeBins * snr3dStaticCfg->azimuthFftSize * snr3dStaticCfg->elevationFftSize *  sizeof(int16_t);
    gMmwMssMCB.snrOutMatrix.data = DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.L3RamObj,
                                                           gMmwMssMCB.snrOutMatrix.dataSize,
                                                           sizeof(int16_t));
    if (gMmwMssMCB.snrOutMatrix.data == NULL)
    {
        retVal = DPC_OBJECTDETECTION_ENOMEM__L3_RAM_SNR3D_MATRIX;
        goto exit;
    }

    /* hwres config - Copy these structures */
    hwRes->detMatrix = gMmwMssMCB.detMatrix;
    hwRes->snrOutMatrix = gMmwMssMCB.snrOutMatrix;

    hwRes->edmaHandle = gEdmaHandle[0];

    hwRes->edmaHwaIn[0].channel =                DPC_OBJDET_DPU_SNR3D_PROC_EDMAIN_PING_CH;
    hwRes->edmaHwaIn[0].shadowPramId =          DPC_OBJDET_DPU_SNR3D_PROC_EDMAIN_PING_SHADOW;
    hwRes->edmaHwaIn[0].eventQueue =             DPC_OBJDET_DPU_SNR3D_PROC_EDMAIN_PING_EVENT_QUE;
    hwRes->edmaHwaIn[1].channel =                DPC_OBJDET_DPU_SNR3D_PROC_EDMAIN_PONG_CH;
    hwRes->edmaHwaIn[1].shadowPramId =          DPC_OBJDET_DPU_SNR3D_PROC_EDMAIN_PONG_SHADOW;
    hwRes->edmaHwaIn[1].eventQueue =             DPC_OBJDET_DPU_SNR3D_PROC_EDMAIN_PONG_EVENT_QUE;

    hwRes->edmaHwaInSignature[0].channel =       DPC_OBJDET_DPU_SNR3D_PROC_EDMAIN_PING_SIG_CH;
    hwRes->edmaHwaInSignature[0].shadowPramId = DPC_OBJDET_DPU_SNR3D_PROC_EDMAIN_PING_SIG_SHADOW;
    hwRes->edmaHwaInSignature[0].eventQueue =    DPC_OBJDET_DPU_SNR3D_PROC_EDMAIN_PING_SIG_EVENT_QUE;
    hwRes->edmaHwaInSignature[1].channel =       DPC_OBJDET_DPU_SNR3D_PROC_EDMAIN_PONG_SIG_CH;
    hwRes->edmaHwaInSignature[1].shadowPramId = DPC_OBJDET_DPU_SNR3D_PROC_EDMAIN_PONG_SIG_SHADOW;
    hwRes->edmaHwaInSignature[1].eventQueue =    DPC_OBJDET_DPU_SNR3D_PROC_EDMAIN_PONG_SIG_EVENT_QUE;

    hwRes->edmaHwaOut[0].channel = DPC_OBJDET_DPU_SNR3D_PROC_EDMAOUT_PING_CH;
    hwRes->edmaHwaOut[0].shadowPramId = DPC_OBJDET_DPU_SNR3D_PROC_EDMAOUT_PING_SHADOW;
    hwRes->edmaHwaOut[0].eventQueue = DPC_OBJDET_DPU_SNR3D_PROC_EDMAOUT_PING_EVENT_QUE;
    hwRes->edmaHwaOut[1].channel = DPC_OBJDET_DPU_SNR3D_PROC_EDMAOUT_PONG_CH;
    hwRes->edmaHwaOut[1].shadowPramId = DPC_OBJDET_DPU_SNR3D_PROC_EDMAOUT_PONG_SHADOW;
    hwRes->edmaHwaOut[1].eventQueue = DPC_OBJDET_DPU_SNR3D_PROC_EDMAOUT_PONG_EVENT_QUE;

    hwRes->hwaCfg.paramSetStartIdx = *hwaParamSetStartIdx;

    /* Give M0 and M1 memory banks for detection matrix scratch. */
    //hwRes->hwaMemInp = (uint16_t *) hwaMemBankAddr[0];
    //hwRes->hwaMemInpSize = (hwaMemBankSize * 2) / sizeof(uint16_t);

    /* M2 bank: for CFAR detection list */
    //hwRes->hwaMemOutDetList = (DPU_SNR3DHM_CfarDetOutput *) hwaMemBankAddr[2];
    //hwRes->hwaMemOutDetListSize = hwaMemBankSize /
    //                            sizeof(DPU_SNR3DHM_CfarDetOutput);

    /* M3 bank: for maximum azimuth values per range bin  (range profile) */
    //hwRes->hwaMemOutRangeProfile = (DPU_SNR3DHM_HwaMaxOutput *) hwaMemBankAddr[3];

    /* Allocate memory for the save location HWA param sets */
    hwRes->hwaCfg.hwaParamsSaveLoc.sizeBytes = DPU_SNR3DHM_MAX_NUM_HWA_PARAMSET * HWA_NUM_REG_PER_PARAM_SET * sizeof(uint32_t);
    hwRes->hwaCfg.hwaParamsSaveLoc.data = DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.CoreLocalRamObj,
                                                                  hwRes->hwaCfg.hwaParamsSaveLoc.sizeBytes,
                                                                  sizeof(uint32_t));
    if (hwRes->hwaCfg.hwaParamsSaveLoc.data == NULL)
    {
        retVal = DPC_OBJECTDETECTION_ENOMEM__SNR3D_HWA_PARAM_SAVE_LOC;
        goto exit;
    }

    hwRes->hwaCfg.dmaTriggerSource[0] = DPC_ObjDet_HwaDmaTrigSrcChanPoolAlloc(&gMmwMssMCB.HwaDmaChanPoolObj);
    hwRes->hwaCfg.dmaTriggerSource[1] = DPC_ObjDet_HwaDmaTrigSrcChanPoolAlloc(&gMmwMssMCB.HwaDmaChanPoolObj);

exit:
    return retVal;
}

/**
*  @b Description
*  @n
*    Based on the configuration, set up the MacroDoppler processing DPU configurations
*/
volatile uint32_t *gDbgMultiFrameClutterRemoval = NULL;

int32_t BF_startRangeBin[FEXTRACT_MAX_OCCUPANCY_BOXES];
float  BF_seatInfo_azim[FEXTRACT_MAX_OCCUPANCY_BOXES],  BF_seatInfo_elev[FEXTRACT_MAX_OCCUPANCY_BOXES];
float  BF_fwInfo_azim[FEXTRACT_MAX_OCCUPANCY_BOXES],  BF_fwInfo_elev[FEXTRACT_MAX_OCCUPANCY_BOXES];


int32_t macroDoppProc_configParser(uint32_t *hwaParamSetStartIdx,
                               uint32_t *hwaWinRamOffset)
{

    /* Doaproc DPU */
    int32_t retVal = 0;
    DPU_MacroDopplerProc_StaticConfig  *staticCfg;
    DPU_MacroDopplerProc_HW_Resources  *hwRes;
    hwRes = &gMacroDoppProcDpuCfg.hwRes;
    staticCfg = &gMacroDoppProcDpuCfg.staticCfg;
    uint32_t winGenLenInSamples;
    uint32_t numSteerVecPerZone;

    memset((void *)&gMacroDoppProcDpuCfg, 0, sizeof(DPU_MacroDopplerProc_Config));

    /* static config */
    staticCfg->numDopplerBins     = gMmwMssMCB.numDopplerBins;
    staticCfg->numRangeBins       = gMmwMssMCB.numRangeBins;
    staticCfg->numTxAntennas      = gMmwMssMCB.numTxAntennas;
    staticCfg->numRxAntennas      = gMmwMssMCB.numRxAntennas;
    staticCfg->numVirtualAntennas = gMmwMssMCB.numRxAntennas * gMmwMssMCB.numTxAntennas;
    staticCfg->numDopplerChirps   = gMmwMssMCB.numDopplerChirps;

    staticCfg->numVirtChirpsPerFrame = gMmwMssMCB.sigProcChainCommonCfg.numOfBurstsInFrame;
    staticCfg->numFramesPerSlidingWindow = gMmwMssMCB.sigProcChainCommonCfg.numFrmPerSlidingWindow;
    staticCfg->numVirtChirpsPerSlidingWindow = staticCfg->numVirtChirpsPerFrame * staticCfg->numFramesPerSlidingWindow;
    staticCfg->frmCntrInSlidingWindowInitVal = gMmwMssMCB.frmCntrInSlidingWindowInitVal;

    staticCfg->isRxChGainPhaseCompensationEnabled = true;

    /* Beamforming parameters */
    if (gMmwMssMCB.cliMacroDopplerSteerDbgCfg.enabled)
    {
        /* For testing - steering vectors set to boresight direction, range bins starting from bin 15 */
        /* Set 5 zones, 25 steering vectors per zone */
        staticCfg->numZones = 5;
        numSteerVecPerZone = 24;
        for (uint8_t ii = 0; ii < staticCfg->numZones; ii++)
        {
            staticCfg->zoneParam[ii].numSteerVec = numSteerVecPerZone;
            staticCfg->zoneParam[ii].steerVecOffset = ii * numSteerVecPerZone;
        }
    }
    else
    {
        /* Steering vectors based on the seat zone definitions */
        staticCfg->numZones = gMmwMssMCB.featureExtrModuleCfg.sceneryParams.numOccupancyBoxes;

        uint16_t steerVecOffset = 0;
        for (uint32_t ii = 0; ii < staticCfg->numZones; ii++)
        {
            staticCfg->zoneParam[ii].numSteerVec = gMmwMssMCB.cliMacroDopplerNumVoxel.numVoxel[ii];
            staticCfg->zoneParam[ii].steerVecOffset = steerVecOffset;
            steerVecOffset += staticCfg->zoneParam[ii].numSteerVec;
        }


    }

    staticCfg->numSteerVec = 0;
    for (uint8_t ii = 0; ii < staticCfg->numZones; ii++)
    {
        staticCfg->numSteerVec += staticCfg->zoneParam[ii].numSteerVec;
    }

    /* Total number of steering vectors must be even number */
    if (staticCfg->numSteerVec & 0x1)
    {
        retVal = DPC_OBJECTDETECTION_ENUM_STEERING_VECTORS_NOT_EVEN_NUMBER;
        goto exit;
    }

    staticCfg->steerVecLen = staticCfg->numVirtualAntennas;

    //Chirp averaging ToDo take from configuration
    staticCfg->numChirpAvg = gMmwMssMCB.cliMacroDopplerCfg.numAvgChirpsPerFrame;
    staticCfg->chirpAvgStartIdx = gMmwMssMCB.cliMacroDopplerCfg.chirpAvgStartIdx;
    staticCfg->chirpAvgStep = gMmwMssMCB.cliMacroDopplerCfg.chirpAvgStep;

    //Multi frame delay line
    staticCfg->multiFrmActualDelayLineLen = gMmwMssMCB.cliMacroDopplerCfg.multiFrmDelayLineLen;
    staticCfg->multiFrmDelayLineLen = 1 << mathUtils_ceilLog2(gMmwMssMCB.cliMacroDopplerCfg.multiFrmDelayLineLen);

    staticCfg->multiFrmDopplerFftSize = gMmwMssMCB.cliMacroDopplerCfg.multiFrmDopplerFftSize;
    staticCfg->multiFrmFftWindowEnabled = gMmwMssMCB.cliMacroDopplerCfg.multiFrmFftWindowEnabled;
    staticCfg->multiFrmPhaseDopFftEnabled = gMmwMssMCB.cliMacroDopplerCfg.multiFrmPhaseDopFftEnabled;

    /* Allocate memory for Steering vectors */
    staticCfg->steerVec = (cmplx32ReIm_t *) DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.L3RamObj,
                                                                    staticCfg->steerVecLen * staticCfg->numSteerVec * sizeof(cmplx32ReIm_t),
                                                                    sizeof(cmplx32ReIm_t));
    gMmwMssMCB.steerVec = staticCfg->steerVec;
    if (staticCfg->steerVec == NULL)
    {
        retVal = DPC_OBJECTDETECTION_ENOMEM__L3_RAM;
        goto exit;
    }

    /* Allocate memory for range bins corresponding to steering vectors */
    staticCfg->rangeBinVec = (uint16_t *) DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.L3RamObj,
                                                                  staticCfg->numSteerVec * sizeof(uint16_t),
                                                                  sizeof(uint32_t));
    gMmwMssMCB.rangeBinVec = staticCfg->rangeBinVec;
    if (staticCfg->rangeBinVec == NULL)
    {
        retVal = DPC_OBJECTDETECTION_ENOMEM__L3_RAM;
        goto exit;
    }


    if (gMmwMssMCB.cliMacroDopplerSteerDbgCfg.enabled)
    {
        /* For testing - steering vectors set to boresight direction, range bins starting from bin 15 */
        int32_t svIdx;
        for (svIdx = 0; svIdx < staticCfg->numSteerVec; svIdx++)
        {
            for (int32_t ii = 0; ii < staticCfg->steerVecLen; ii++)
            {
                int32_t sign = 1 - 2*((ii >> 2) & 0x1);
                staticCfg->steerVec[svIdx * staticCfg->steerVecLen + ii].real = sign * 1048575;
                staticCfg->steerVec[svIdx * staticCfg->steerVecLen + ii].imag = 0;
            }
            staticCfg->rangeBinVec[svIdx] = (uint16_t) (15 + (int32_t) (svIdx % numSteerVecPerZone));
        }
    }
    else
    {
        /* Generate steering vectors based on the zone definitions */
        float tempfRe, tempfIm, outfRe, outfIm;
        float tempAngle;
        float compRe, compIm;

        float x1, x2, y1, y2, z1, z2, range;
        uint8_t cuboidOffset1, cuboidOffset2, cuboidOffset3;
        mmwDemo_cartesian_position posW, posT;
        mmwDemo_worldTransformParams    invTransformParams;    /* Transformation from world coordinates to sensor coordination */

        // Generate the center of seat zone and the enter of each footwell zone
        memcpy(&invTransformParams.offset, &gMmwMssMCB.idetSceneryParams.sensorPosition, sizeof(mmwDemo_sensorPosition));
        memcpy(&invTransformParams.tilt, &gMmwMssMCB.idetSceneryParams.sensorOrientation, sizeof(mmwDemo_sensorOrientation));

        MmwDemo_computeInvRotationMatrix(&invTransformParams.tilt, invTransformParams.rotW.a);

        for (uint8_t zoneInd = 0; zoneInd < gMmwMssMCB.featureExtrModuleCfg.sceneryParams.numOccupancyBoxes; zoneInd ++ )
        {
            if (gMmwMssMCB.featureExtrModuleCfg.sceneryParams.numCuboidsPerOccupancyBox[zoneInd] != 3)
            {
                retVal = DPC_OBJECTDETECTION_ENUM_STEERING_VECTORS_INVAL_CFG; //zy for modification
                goto exit;
            }
            cuboidOffset1 = 3 * zoneInd;
            cuboidOffset2 = 3 * zoneInd + 1;
            cuboidOffset3 = 3 * zoneInd + 2;

            BF_startRangeBin[zoneInd] = gMmwMssMCB.cliMacroDopplerRngBinOffs.rngBinOffs[zoneInd];

            //cuboidOffset1 = gMmwMssMCB.featureExtrModuleCfg.sceneryParams.numCuboidsPerOccupancyBox[zoneInd];
            x1 = (gMmwMssMCB.featureExtrModuleCfg.sceneryParams.cuboidDefs[cuboidOffset1].x1 + gMmwMssMCB.featureExtrModuleCfg.sceneryParams.cuboidDefs[cuboidOffset2].x1) * 0.5;
            x2 = (gMmwMssMCB.featureExtrModuleCfg.sceneryParams.cuboidDefs[cuboidOffset1].x2 + gMmwMssMCB.featureExtrModuleCfg.sceneryParams.cuboidDefs[cuboidOffset2].x2) * 0.5;
            y1 = (gMmwMssMCB.featureExtrModuleCfg.sceneryParams.cuboidDefs[cuboidOffset1].y1 + gMmwMssMCB.featureExtrModuleCfg.sceneryParams.cuboidDefs[cuboidOffset2].y1) * 0.5;
            y2 = (gMmwMssMCB.featureExtrModuleCfg.sceneryParams.cuboidDefs[cuboidOffset1].y2 + gMmwMssMCB.featureExtrModuleCfg.sceneryParams.cuboidDefs[cuboidOffset2].y2) * 0.5;
            z1 = (gMmwMssMCB.featureExtrModuleCfg.sceneryParams.cuboidDefs[cuboidOffset1].z1 + gMmwMssMCB.featureExtrModuleCfg.sceneryParams.cuboidDefs[cuboidOffset2].z1) * 0.5;
            z2 = (gMmwMssMCB.featureExtrModuleCfg.sceneryParams.cuboidDefs[cuboidOffset1].z2 + gMmwMssMCB.featureExtrModuleCfg.sceneryParams.cuboidDefs[cuboidOffset2].z2) * 0.5;
            posW.posX = (x2 + x1) * 0.5;
            posW.posY = (y2 + y1) * 0.5;
            posW.posZ = (z2 + z1) * 0.5;
            MmwDemo_world2sensor(&posW, &invTransformParams, &posT);
            range = sqrtf(posT.posX*posT.posX + posT.posY*posT.posY + posT.posZ*posT.posZ);
            BF_seatInfo_elev[zoneInd] = (posT.posZ/range); // sin(elev)
            BF_seatInfo_azim[zoneInd] = (posT.posX/range); // sin(azim)*cos(elev)

            x1 = gMmwMssMCB.featureExtrModuleCfg.sceneryParams.cuboidDefs[cuboidOffset3].x1;
            x2 = gMmwMssMCB.featureExtrModuleCfg.sceneryParams.cuboidDefs[cuboidOffset3].x2;
            y1 = gMmwMssMCB.featureExtrModuleCfg.sceneryParams.cuboidDefs[cuboidOffset3].y1;
            y2 = gMmwMssMCB.featureExtrModuleCfg.sceneryParams.cuboidDefs[cuboidOffset3].y2;
            z1 = gMmwMssMCB.featureExtrModuleCfg.sceneryParams.cuboidDefs[cuboidOffset3].z1;
            z2 = gMmwMssMCB.featureExtrModuleCfg.sceneryParams.cuboidDefs[cuboidOffset3].z2;
            posW.posX = (x2 + x1) * 0.5;
            posW.posY = (y2 + y1) * 0.5;
            posW.posZ = (z2 + z1) * 0.5;
            MmwDemo_world2sensor(&posW, &invTransformParams, &posT);
            range = sqrtf(posT.posX*posT.posX + posT.posY*posT.posY + posT.posZ*posT.posZ);
            BF_fwInfo_elev[zoneInd] = (posT.posZ/range); // sin(elev)
            BF_fwInfo_azim[zoneInd] = (posT.posX/range); // sin(azim)*cos(elev)


            int32_t svIdx;
            int32_t steerVecOffset = staticCfg->zoneParam[zoneInd].steerVecOffset;
            int32_t numSteerVecPerZone = staticCfg->zoneParam[zoneInd].numSteerVec;
            //float tempfRe, tempfIm, outfRe, outfIm;
            for (svIdx = steerVecOffset + 0; svIdx < (steerVecOffset + numSteerVecPerZone); )
            {
                // svIdx even number
                for (int32_t ii = 0; ii < staticCfg->steerVecLen; ii++)
                {
                    tempAngle = PI * (((float)(gMmwMssMCB.dspPreStartCfgLocal.n_ind[ii]) * BF_seatInfo_elev[zoneInd]) + ((float)(gMmwMssMCB.dspPreStartCfgLocal.m_ind[ii]) * BF_seatInfo_azim[zoneInd]));
                    tempfRe    =  cosf(tempAngle);
                    tempfIm    =  sinf(tempAngle);

                    compRe = (float)(gMmwMssMCB.compRxChannelBiasCfg.rxChPhaseComp[ii].real);
                    compIm = (float)(gMmwMssMCB.compRxChannelBiasCfg.rxChPhaseComp[ii].imag);
                    outfRe     = tempfRe * compRe  - tempfIm * compIm;
                    outfIm     = tempfRe * compIm + tempfIm * compRe;

                    staticCfg->steerVec[svIdx * staticCfg->steerVecLen + ii].real = (int32_t) lroundf(outfRe);
                    staticCfg->steerVec[svIdx * staticCfg->steerVecLen + ii].imag = (int32_t) lroundf(outfIm);
                }
                staticCfg->rangeBinVec[svIdx] = (uint16_t) (BF_startRangeBin[zoneInd] + svIdx - steerVecOffset);
                svIdx ++;

                // svIdx odd number
                for (int32_t ii = 0; ii < staticCfg->steerVecLen; ii++)
                {
                    tempAngle = PI * (((float)(gMmwMssMCB.dspPreStartCfgLocal.n_ind[ii]) * BF_fwInfo_elev[zoneInd]) + ((float)(gMmwMssMCB.dspPreStartCfgLocal.m_ind[ii]) * BF_fwInfo_azim[zoneInd]));
                    tempfRe    =  cosf(tempAngle);
                    tempfIm    =  sinf(tempAngle);

                    compRe = (float)(gMmwMssMCB.compRxChannelBiasCfg.rxChPhaseComp[ii].real);
                    compIm = (float)(gMmwMssMCB.compRxChannelBiasCfg.rxChPhaseComp[ii].imag);
                    outfRe     = tempfRe * compRe  - tempfIm * compIm;
                    outfIm     = tempfRe * compIm + tempfIm * compRe;

                    staticCfg->steerVec[svIdx * staticCfg->steerVecLen + ii].real = (int32_t) lroundf(outfRe);
                    staticCfg->steerVec[svIdx * staticCfg->steerVecLen + ii].imag = (int32_t) lroundf(outfIm);
                }
                staticCfg->rangeBinVec[svIdx] = (uint16_t) (BF_startRangeBin[zoneInd] + svIdx - steerVecOffset);
                svIdx ++;
            }

        }
    }
    

    if (staticCfg->multiFrmPhaseDopFftEnabled)
    {
        /* Allocate memory for Previous voxel phases */
        staticCfg->previousVoxelPhaseArray = (int16_t *) DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.L3RamObj,
                                                                                  staticCfg->numSteerVec * staticCfg->numChirpAvg * sizeof(int16_t),
                                                                                  sizeof(uint32_t));
        if (staticCfg->previousVoxelPhaseArray == NULL)
        {
            retVal = DPC_OBJECTDETECTION_ENOMEM__L3_RAM;
            goto exit;
        }
        memset(staticCfg->previousVoxelPhaseArray, 0, staticCfg->numSteerVec * staticCfg->numChirpAvg * sizeof(int16_t));

        /* Allocate memory for Voxel phases multi frame delay line */
        staticCfg->phaseMultFrmDelayLine = (int16_t *) DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.L3RamObj,
                                                                                  staticCfg->numSteerVec *
                                                                                  staticCfg->numChirpAvg *
                                                                                  staticCfg->multiFrmDelayLineLen *
                                                                                  sizeof(int16_t),
                                                                                  sizeof(uint32_t));
        if (staticCfg->phaseMultFrmDelayLine == NULL)
        {
            retVal = DPC_OBJECTDETECTION_ENOMEM__L3_RAM;
            goto exit;
        }
        memset(staticCfg->phaseMultFrmDelayLine, 0, staticCfg->numSteerVec *
                                                    staticCfg->numChirpAvg *
                                                    staticCfg->multiFrmDelayLineLen *
                                                    sizeof(int16_t));
        /* Allocate memory for phase multi frame Doppler FFT output */
        staticCfg->phaseMultiFrmDoppOut = (uint32_t *) DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.L3RamObj,
                                                                                   staticCfg->numSteerVec *
                                                                                   staticCfg->multiFrmDopplerFftSize/2 *
                                                                                   sizeof(uint32_t),
                                                                                   sizeof(uint32_t));
        gMmwMssMCB.phaseMultiFrmDoppOut = staticCfg->phaseMultiFrmDoppOut;
        if (staticCfg->phaseMultiFrmDoppOut == NULL)
        {
         retVal = DPC_OBJECTDETECTION_ENOMEM__L3_RAM;
         goto exit;
        }
    }
    else
    {
        staticCfg->previousVoxelPhaseArray = NULL;
        staticCfg->phaseMultFrmDelayLine = NULL;
        staticCfg->phaseMultiFrmDoppOut = NULL;
    }

    /* Allocate memory for Voxel symbols multi frame delay line (ToDo check if complex 16 is sufficient) */
    staticCfg->symbMultiFrmDelayLine = (cmplx32ImRe_t *) DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.L3RamObj,
                                                                              staticCfg->numSteerVec *
                                                                              staticCfg->multiFrmDelayLineLen *
                                                                              sizeof(cmplx32ImRe_t),
                                                                              sizeof(uint32_t));
    if (staticCfg->symbMultiFrmDelayLine == NULL)
    {
        retVal = DPC_OBJECTDETECTION_ENOMEM__L3_RAM;
        goto exit;
    }
    memset(staticCfg->symbMultiFrmDelayLine, 0, staticCfg->numSteerVec *
                                                staticCfg->multiFrmDelayLineLen *
                                                sizeof(cmplx32ImRe_t));


    /* Allocate memory for symbol multi frame Doppler FFT output */
    staticCfg->symbMultiFrmDoppOut = (uint32_t *) DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.L3RamObj,
                                                                            staticCfg->numSteerVec *
                                                                            staticCfg->multiFrmDopplerFftSize *
                                                                            sizeof(uint32_t),
                                                                            sizeof(uint32_t));
    gMmwMssMCB.symbMultiFrmDoppOut = staticCfg->symbMultiFrmDoppOut;
    if (staticCfg->symbMultiFrmDoppOut == NULL)
    {
      retVal = DPC_OBJECTDETECTION_ENOMEM__L3_RAM;
      goto exit;
    }

    /* Allocate memory for symbol multi frame Doppler FFT AVERAGED output */
    staticCfg->averageSymbMultiFrmDoppOut = (uint32_t *) DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.L3RamObj,
                                                                            staticCfg->numZones *
                                                                            staticCfg->multiFrmDopplerFftSize *
                                                                            sizeof(uint32_t),
                                                                            sizeof(uint32_t));
    gMmwMssMCB.averageSymbMultiFrmDoppOut = staticCfg->averageSymbMultiFrmDoppOut;
    if (staticCfg->averageSymbMultiFrmDoppOut == NULL)
    {
      retVal = DPC_OBJECTDETECTION_ENOMEM__L3_RAM;
      goto exit;
    }


    if (gOverlappedHwaParamSets)
    {
        staticCfg->loadHwaParamSetsBeforeExec = true;
    }
    else
    {
        staticCfg->loadHwaParamSetsBeforeExec = false;
    }

    hwRes->radCubeDCvalue = (cmplx32ImRe_t *) DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.L3RamObj,
                                                           staticCfg->steerVecLen * staticCfg->numRangeBins * sizeof(cmplx32ImRe_t),
                                                           sizeof(cmplx32ImRe_t));
    if (hwRes->radCubeDCvalue == NULL)
    {
        retVal = DPC_OBJECTDETECTION_ENOMEM__L3_RAM;
        goto exit;
    }

    hwRes->voxelSymOut = (cmplx32ImRe_t *) DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.L3RamObj,
                                                                   staticCfg->numChirpAvg * 2 * sizeof(cmplx32ImRe_t), //2 because of ping/pong
                                                                   sizeof(cmplx32ImRe_t));
    if (hwRes->voxelSymOut == NULL)
    {
        retVal = DPC_OBJECTDETECTION_ENOMEM__L3_RAM;
        goto exit;
    }
    hwRes->voxelSymMeanOut = (cmplx32ImRe_t *) DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.L3RamObj,
                                                                   2 * sizeof(cmplx32ImRe_t), //2 because of ping/pong
                                                                   sizeof(cmplx32ImRe_t));
    if (hwRes->voxelSymMeanOut == NULL)
    {
        retVal = DPC_OBJECTDETECTION_ENOMEM__L3_RAM;
        goto exit;
    }


    /* hwRes - copy these structures */
    hwRes->radarCube = gMmwMssMCB.radarCube;

    hwRes->edmaCfg.edmaHandle = gEdmaHandle[DPC_OBJDET_DPU_MACRODOPP_DCEST_PROC_EDMA_INST_ID];

    //Chirp DC estimation
    hwRes->edmaCfg.edmaChirpDC.edmaIn[0].channel =       DPC_OBJDET_DPU_MACRODOPP_DCEST_EDMAIN_PING_CH;
    hwRes->edmaCfg.edmaChirpDC.edmaIn[0].shadowPramId =  DPC_OBJDET_DPU_MACRODOPP_DCEST_EDMAIN_PING_SHADOW;
    hwRes->edmaCfg.edmaChirpDC.edmaIn[0].eventQueue =    DPC_OBJDET_DPU_MACRODOPP_DCEST_EDMAIN_PING_EVENT_QUE;
    hwRes->edmaCfg.edmaChirpDC.edmaIn[1].channel =       DPC_OBJDET_DPU_MACRODOPP_DCEST_EDMAIN_PONG_CH;
    hwRes->edmaCfg.edmaChirpDC.edmaIn[1].shadowPramId =  DPC_OBJDET_DPU_MACRODOPP_DCEST_EDMAIN_PONG_SHADOW;
    hwRes->edmaCfg.edmaChirpDC.edmaIn[1].eventQueue =    DPC_OBJDET_DPU_MACRODOPP_DCEST_EDMAIN_PONG_EVENT_QUE;

    hwRes->edmaCfg.edmaChirpDC.edmaHotSig[0].channel =       DPC_OBJDET_DPU_MACRODOPP_DCEST_EDMAIN_PING_SIG_CH;
    hwRes->edmaCfg.edmaChirpDC.edmaHotSig[0].shadowPramId =  DPC_OBJDET_DPU_MACRODOPP_DCEST_EDMAIN_PING_SIG_SHADOW;
    hwRes->edmaCfg.edmaChirpDC.edmaHotSig[0].eventQueue =    DPC_OBJDET_DPU_MACRODOPP_DCEST_EDMAIN_PING_SIG_EVENT_QUE;
    hwRes->edmaCfg.edmaChirpDC.edmaHotSig[1].channel =       DPC_OBJDET_DPU_MACRODOPP_DCEST_EDMAIN_PONG_SIG_CH;
    hwRes->edmaCfg.edmaChirpDC.edmaHotSig[1].shadowPramId =  DPC_OBJDET_DPU_MACRODOPP_DCEST_EDMAIN_PONG_SIG_SHADOW;
    hwRes->edmaCfg.edmaChirpDC.edmaHotSig[1].eventQueue =    DPC_OBJDET_DPU_MACRODOPP_DCEST_EDMAIN_PONG_SIG_EVENT_QUE;

    hwRes->edmaCfg.edmaChirpDC.edmaOut[0].channel =       DPC_OBJDET_DPU_MACRODOPP_DCEST_EDMAOUT_PING_CH;
    hwRes->edmaCfg.edmaChirpDC.edmaOut[0].shadowPramId =  DPC_OBJDET_DPU_MACRODOPP_DCEST_EDMAOUT_PING_SHADOW;
    hwRes->edmaCfg.edmaChirpDC.edmaOut[0].eventQueue =    DPC_OBJDET_DPU_MACRODOPP_DCEST_EDMAOUT_PING_EVENT_QUE;
    hwRes->edmaCfg.edmaChirpDC.edmaOut[1].channel =       DPC_OBJDET_DPU_MACRODOPP_DCEST_EDMAOUT_PONG_CH;
    hwRes->edmaCfg.edmaChirpDC.edmaOut[1].shadowPramId =  DPC_OBJDET_DPU_MACRODOPP_DCEST_EDMAOUT_PONG_SHADOW;
    hwRes->edmaCfg.edmaChirpDC.edmaOut[1].eventQueue =    DPC_OBJDET_DPU_MACRODOPP_DCEST_EDMAOUT_PONG_EVENT_QUE;

    //Beamforming
    hwRes->edmaCfg.edmaBeamFrm.edmaChirpsIn.channel =           DPC_OBJDET_DPU_MACRODOPP_BBF_EDMA_CHIRP_IN_CH;
    hwRes->edmaCfg.edmaBeamFrm.edmaChirpsIn.shadowPramId =      DPC_OBJDET_DPU_MACRODOPP_BBF_EDMA_CHIRP_IN_SHADOW;
    hwRes->edmaCfg.edmaBeamFrm.edmaChirpsIn.eventQueue =        DPC_OBJDET_DPU_MACRODOPP_BBF_EDMA_CHIRP_IN_EVENT_QUE;

    hwRes->edmaCfg.edmaBeamFrm.edmaChirpsDCIn.channel =         DPC_OBJDET_DPU_MACRODOPP_BBF_EDMA_CHIRPDC_IN_CH;
    hwRes->edmaCfg.edmaBeamFrm.edmaChirpsDCIn.shadowPramId =    DPC_OBJDET_DPU_MACRODOPP_BBF_EDMA_CHIRPDC_IN_SHADOW;
    hwRes->edmaCfg.edmaBeamFrm.edmaChirpsDCIn.eventQueue =      DPC_OBJDET_DPU_MACRODOPP_BBF_EDMA_CHIRPDC_IN_EVENT_QUE;

    hwRes->edmaCfg.edmaBeamFrm.edmaSteerVecIn.channel =         DPC_OBJDET_DPU_MACRODOPP_BBF_EDMA_STEERVEC_IN_CH;
    hwRes->edmaCfg.edmaBeamFrm.edmaSteerVecIn.shadowPramId =    DPC_OBJDET_DPU_MACRODOPP_BBF_EDMA_STEERVEC_IN_SHADOW;
    hwRes->edmaCfg.edmaBeamFrm.edmaSteerVecIn.eventQueue =      DPC_OBJDET_DPU_MACRODOPP_BBF_EDMA_STEERVEC_IN_EVENT_QUE;

    hwRes->edmaCfg.edmaBeamFrm.edmaHotSig.channel =             DPC_OBJDET_DPU_MACRODOPP_BBF_EDMA_HOTSIG_CH;
    hwRes->edmaCfg.edmaBeamFrm.edmaHotSig.shadowPramId =        DPC_OBJDET_DPU_MACRODOPP_BBF_EDMA_HOTSIG_SHADOW;
    hwRes->edmaCfg.edmaBeamFrm.edmaHotSig.eventQueue =          DPC_OBJDET_DPU_MACRODOPP_BBF_EDMA_HOTSIG_EVENT_QUE;

    hwRes->edmaCfg.edmaBeamFrm.edmaVoxelsOut.channel =          DPC_OBJDET_DPU_MACRODOPP_BBF_EDMA_VOXELS_OUT_CH;
    hwRes->edmaCfg.edmaBeamFrm.edmaVoxelsOut.shadowPramId =     DPC_OBJDET_DPU_MACRODOPP_BBF_EDMA_VOXELS_OUT_SHADOW;
    hwRes->edmaCfg.edmaBeamFrm.edmaVoxelsOut.eventQueue =       DPC_OBJDET_DPU_MACRODOPP_BBF_EDMA_VOXELS_OUT_EVENT_QUE;

    hwRes->edmaCfg.edmaBeamFrm.edmaVoxelsMeanOut.channel =      DPC_OBJDET_DPU_MACRODOPP_BBF_EDMA_VOXELS_MEAN_OUT_CH;
    hwRes->edmaCfg.edmaBeamFrm.edmaVoxelsMeanOut.shadowPramId = DPC_OBJDET_DPU_MACRODOPP_BBF_EDMA_VOXELS_MEAN_OUT_SHADOW;
    hwRes->edmaCfg.edmaBeamFrm.edmaVoxelsMeanOut.eventQueue =   DPC_OBJDET_DPU_MACRODOPP_BBF_EDMA_VOXELS_MEAN_OUT_EVENT_QUE;

    hwRes->edmaCfg.edmaBeamFrm.edmaHwaEnable.channel =      DPC_OBJDET_DPU_MACRODOPP_BBF_EDMA_HWA_EN_CH;
    hwRes->edmaCfg.edmaBeamFrm.edmaHwaEnable.shadowPramId = DPC_OBJDET_DPU_MACRODOPP_BBF_EDMA_HWA_EN_SHADOW;
    hwRes->edmaCfg.edmaBeamFrm.edmaHwaEnable.eventQueue =   DPC_OBJDET_DPU_MACRODOPP_BBF_EDMA_HWA_EN_EVENT_QUE;

    //Multi Doppler FFT
    hwRes->edmaCfg.edmaMultiFrmDopFft.edmaPhaseIn.channel = DPC_OBJDET_DPU_MACRODOPP_MULTI_FRM_EDMA_PHASE_IN_CH;
    hwRes->edmaCfg.edmaMultiFrmDopFft.edmaPhaseIn.shadowPramId = DPC_OBJDET_DPU_MACRODOPP_MULTI_FRM_EDMA_PHASE_IN_SHADOW;
    hwRes->edmaCfg.edmaMultiFrmDopFft.edmaPhaseIn.eventQueue = DPC_OBJDET_DPU_MACRODOPP_MULTI_FRM_EDMA_PHASE_IN_EVENT_QUE;

    hwRes->edmaCfg.edmaMultiFrmDopFft.edmaPhaseHotSig.channel = DPC_OBJDET_DPU_MACRODOPP_MULTI_FRM_EDMA_PHASE_HOTSIG_CH;
    hwRes->edmaCfg.edmaMultiFrmDopFft.edmaPhaseHotSig.shadowPramId = DPC_OBJDET_DPU_MACRODOPP_MULTI_FRM_EDMA_PHASE_HOTSIG_SHADOW;
    hwRes->edmaCfg.edmaMultiFrmDopFft.edmaPhaseHotSig.eventQueue = DPC_OBJDET_DPU_MACRODOPP_MULTI_FRM_EDMA_PHASE_HOTSIG_EVENT_QUE;

    hwRes->edmaCfg.edmaMultiFrmDopFft.edmaPhaseOut.channel = DPC_OBJDET_DPU_MACRODOPP_MULTI_FRM_EDMA_PHASE_OUT_CH;
    hwRes->edmaCfg.edmaMultiFrmDopFft.edmaPhaseOut.shadowPramId = DPC_OBJDET_DPU_MACRODOPP_MULTI_FRM_EDMA_PHASE_OUT_SHADOW;
    hwRes->edmaCfg.edmaMultiFrmDopFft.edmaPhaseOut.eventQueue = DPC_OBJDET_DPU_MACRODOPP_MULTI_FRM_EDMA_PHASE_OUT_EVENT_QUE;

    hwRes->edmaCfg.edmaMultiFrmDopFft.edmaSymbIn.channel = DPC_OBJDET_DPU_MACRODOPP_MULTI_FRM_EDMA_SYM_IN_CH;
    hwRes->edmaCfg.edmaMultiFrmDopFft.edmaSymbIn.shadowPramId = DPC_OBJDET_DPU_MACRODOPP_MULTI_FRM_EDMA_SYM_IN_SHADOW;
    hwRes->edmaCfg.edmaMultiFrmDopFft.edmaSymbIn.eventQueue = DPC_OBJDET_DPU_MACRODOPP_MULTI_FRM_EDMA_SYM_IN_EVENT_QUE;

    hwRes->edmaCfg.edmaMultiFrmDopFft.edmaSymbHotSig.channel = DPC_OBJDET_DPU_MACRODOPP_MULTI_FRM_EDMA_SYM_HOTSIG_CH;
    hwRes->edmaCfg.edmaMultiFrmDopFft.edmaSymbHotSig.shadowPramId = DPC_OBJDET_DPU_MACRODOPP_MULTI_FRM_EDMA_SYM_HOTSIG_SHADOW;
    hwRes->edmaCfg.edmaMultiFrmDopFft.edmaSymbHotSig.eventQueue = DPC_OBJDET_DPU_MACRODOPP_MULTI_FRM_EDMA_SYM_HOTSIG_EVENT_QUE;

    hwRes->edmaCfg.edmaMultiFrmDopFft.edmaSymbOut.channel = DPC_OBJDET_DPU_MACRODOPP_MULTI_FRM_EDMA_SYM_OUT_CH;
    hwRes->edmaCfg.edmaMultiFrmDopFft.edmaSymbOut.shadowPramId = DPC_OBJDET_DPU_MACRODOPP_MULTI_FRM_EDMA_SYM_OUT_SHADOW;
    hwRes->edmaCfg.edmaMultiFrmDopFft.edmaSymbOut.eventQueue = DPC_OBJDET_DPU_MACRODOPP_MULTI_FRM_EDMA_SYM_OUT_EVENT_QUE;

    //Average Macro Doppler Outputs across voxels
    hwRes->edmaCfg.edmaAverageMacroDopFft.edmaIn.channel = DPC_OBJDET_DPU_MACRODOPP_MULTI_FRM_EDMA_AVERAGE_IN_CH;
    hwRes->edmaCfg.edmaAverageMacroDopFft.edmaIn.shadowPramId = DPC_OBJDET_DPU_MACRODOPP_MULTI_FRM_EDMA_AVERAGE_IN_SHADOW;
    hwRes->edmaCfg.edmaAverageMacroDopFft.edmaIn.eventQueue = DPC_OBJDET_DPU_MACRODOPP_MULTI_FRM_EDMA_AVERAGE_IN_EVENT_QUE;

    hwRes->edmaCfg.edmaAverageMacroDopFft.edmaHotSig.channel = DPC_OBJDET_DPU_MACRODOPP_MULTI_FRM_EDMA_AVERAGE_HOTSIG_CH;
    hwRes->edmaCfg.edmaAverageMacroDopFft.edmaHotSig.shadowPramId = DPC_OBJDET_DPU_MACRODOPP_MULTI_FRM_EDMA_AVERAGE_HOTSIG_SHADOW;
    hwRes->edmaCfg.edmaAverageMacroDopFft.edmaHotSig.eventQueue = DPC_OBJDET_DPU_MACRODOPP_MULTI_FRM_EDMA_AVERAGE_HOTSIG_EVENT_QUE;

    hwRes->edmaCfg.edmaAverageMacroDopFft.edmaOut.channel = DPC_OBJDET_DPU_MACRODOPP_MULTI_FRM_EDMA_AVERAGE_OUT_CH;
    hwRes->edmaCfg.edmaAverageMacroDopFft.edmaOut.shadowPramId = DPC_OBJDET_DPU_MACRODOPP_MULTI_FRM_EDMA_AVERAGE_OUT_SHADOW;
    hwRes->edmaCfg.edmaAverageMacroDopFft.edmaOut.eventQueue = DPC_OBJDET_DPU_MACRODOPP_MULTI_FRM_EDMA_AVERAGE_OUT_EVENT_QUE;

    hwRes->hwaCfg.paramSetStartIdx = *hwaParamSetStartIdx;


    /* Allocate memory for the save location HWA param sets */
    hwRes->hwaCfg.hwaParamsSaveLoc.sizeBytes = DPU_SNR3DHM_MAX_NUM_HWA_PARAMSET * HWA_NUM_REG_PER_PARAM_SET * sizeof(uint32_t);
    hwRes->hwaCfg.hwaParamsSaveLoc.data = DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.CoreLocalRamObj,
                                                                  hwRes->hwaCfg.hwaParamsSaveLoc.sizeBytes,
                                                                  sizeof(uint32_t));
    if (hwRes->hwaCfg.hwaParamsSaveLoc.data == NULL)
    {
        retVal = DPC_OBJECTDETECTION_ENOMEM__SNR3D_HWA_PARAM_SAVE_LOC;
        goto exit;
    }

    hwRes->hwaCfg.dmaTriggerSource[0] = DPC_ObjDet_HwaDmaTrigSrcChanPoolAlloc(&gMmwMssMCB.HwaDmaChanPoolObj);
    hwRes->hwaCfg.dmaTriggerSource[1] = DPC_ObjDet_HwaDmaTrigSrcChanPoolAlloc(&gMmwMssMCB.HwaDmaChanPoolObj);

    /* Generating Macro Doppler window, allocate first */
    winGenLenInSamples = staticCfg->multiFrmDelayLineLen;
    hwRes->hwaCfg.windowSize = sizeof(uint32_t) * winGenLenInSamples;
    hwRes->hwaCfg.winSym = HWA_FFT_WINDOW_NONSYMMETRIC;
    hwRes->hwaCfg.window     = (int32_t *)DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.CoreLocalRamObj,
                                                                  hwRes->hwaCfg.windowSize,
                                                                  sizeof(uint32_t));
    if (hwRes->hwaCfg.window == NULL)
    {
       retVal = DPC_OBJECTDETECTION_ENOMEM__CORE_LOCAL_RAM_RANGE_HWA_WINDOW;
       goto exit;
    }

    if (!gMmwMssMCB.oneTimeConfigDone)
    {
        /* zero the whole buffer */
        memset(hwRes->hwaCfg.window, 0, hwRes->hwaCfg.windowSize);
        if (staticCfg->multiFrmFftWindowEnabled)
        {
            //Use Hanning window
            mathUtils_genWindow((uint32_t *)&hwRes->hwaCfg.window[staticCfg->multiFrmDelayLineLen - staticCfg->multiFrmActualDelayLineLen],
                                (uint32_t) staticCfg->multiFrmActualDelayLineLen,
                                staticCfg->multiFrmActualDelayLineLen,
                                DPC_DPU_MACRO_DOPPLERPROC_FFT_WINDOW_TYPE,
                                DPC_OBJDET_QFORMAT_MACRO_DOPPLER_FFT);
        }
        else
        {
            //Use rectangular window
            for (int32_t i = 0; i < staticCfg->multiFrmActualDelayLineLen; i++)
            {
                hwRes->hwaCfg.window[staticCfg->multiFrmDelayLineLen - staticCfg->multiFrmActualDelayLineLen + i] = 1 << DPC_OBJDET_QFORMAT_MACRO_DOPPLER_FFT;
            }
        }
    }


    /* Allocate memory for the save location HWA param sets */
    hwRes->hwaCfg.hwaParamsSaveLoc.sizeBytes = DPU_MACRODOPPROC_MAX_NUM_HWA_PARAMSET * HWA_NUM_REG_PER_PARAM_SET * sizeof(uint32_t);
    hwRes->hwaCfg.hwaParamsSaveLoc.data = DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.CoreLocalRamObj,
                                                                  hwRes->hwaCfg.hwaParamsSaveLoc.sizeBytes,
                                                                  sizeof(uint32_t));
    if (hwRes->hwaCfg.hwaParamsSaveLoc.data == NULL)
    {
        retVal = DPC_OBJECTDETECTION_ENOMEM__MACRODOPPLER_HWA_PARAM_SAVE_LOC;
        goto exit;
    }


    hwRes->hwaCfg.winRamOffset = (uint16_t) *hwaWinRamOffset;
    if ((hwRes->hwaCfg.winRamOffset + winGenLenInSamples) > DPC_OBJDETRANGE_HWA_MAX_WINDOW_RAM_SIZE_IN_SAMPLES)
    {
        retVal = DPC_OBJECTDETECTION_ENOMEM__HWA_WINDOW_RAM_INTERNAL;
        goto exit;
    }
    *hwaWinRamOffset += winGenLenInSamples;

    /*For debug - clutter removal */
    gDbgMultiFrameClutterRemoval = (uint32_t *) DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.L3RamObj,
                                                           sizeof(uint32_t),
                                                           sizeof(uint32_t));
    gDbgMultiFrameClutterRemoval[0] = 1;

exit:
    return retVal;
}

/**
 *  @b Description
 *  @n
 *     Configure Intrusion detection.
 *
 *  @param[in]  config Intruder detection configuration
 *
 *  @retval
 *      Success -   0
 *  @retval
 *      Error   -   <0
 *
 *  \ingroup DPC_OBJDET__INTERNAL_FUNCTION
 */
int32_t IntrusionDet_configParser(IDETECT_moduleConfig *config)
{
    int32_t retVal = 0;
    int32_t ii;

    memset(config, 0, sizeof(IDETECT_moduleConfig));

    /* static config */
    // Number of bins
    config->numRangeBins = gMmwMssMCB.numRangeBins;
    config->numAzimBins = gMmwMssMCB.intrusionSigProcChainCfg.azimuthFftSize;
    config->numElevBins = gMmwMssMCB.intrusionSigProcChainCfg.elevationFftSize;
    
    // Range grid parameters
    config->rangeStep = gMmwMssMCB.rangeStep;
    config->rangeBias = gMmwMssMCB.compRxChannelBiasCfg.rangeBias;

    // Azimuth and elevation grids or steps
    for (ii = 0; ii < config->numAzimBins; ii++)
    {
        config->azimuthGrid[ii] = ( (ii - config->numAzimBins/2) * gMmwMssMCB.lambdaOverDistX / (float) config->numAzimBins );
    }
    for (ii = 0; ii < config->numElevBins; ii++)
    {
        config->elevationGrid[ii] = ( (ii - config->numElevBins/2) * gMmwMssMCB.lambdaOverDistZ / (float) config->numElevBins );
    }
    config->isGridInMuNuDomain = true;

    // Scene parameters - copy the whole structure
    config->sceneryParams = gMmwMssMCB.idetSceneryParams;

    // State parameters - copy the whole structure
    config->stateParams = gMmwMssMCB.idetStateParams;

    // Signal processing parameters - copy the whole structure
    config->sigProcParams = gMmwMssMCB.idetSigProcParams;

    // Allocate internal memory
    config->scratchBuffer = NULL;
    config->scratchBufferSizeInBytes = 0;

//exit:
        return retVal;
}


int32_t dsp_configParser()
{
    int32_t retVal = 0;
    int32_t pointCloudSize;
    DPIF_MSS_DSS_PreStartCfg *pParam_s = &gMmwMssMCB.dspPreStartCfgLocal;


    /* Allocate point cloud list passed to feature Extraction */
    pointCloudSize                       = DPIF_MAX_RESOLVED_OBJECTS_PER_FRAME * sizeof(FEXTRACT_measurementPoint);
    gMmwMssMCB.pointCloudToFeatExtr         = (FEXTRACT_measurementPoint *)DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.L3RamObj,
                                                                                                   pointCloudSize,
                                                                                                   sizeof(uint32_t));
    if (gMmwMssMCB.pointCloudToFeatExtr == NULL)
    {
        retVal = DPC_OBJECTDETECTION_ENOMEM__L3_RAM_POINTT_CLOUD_TO_FEATURE_EXTR;
        goto exit;
    }

    /* Allocate configuration for DSP */
    gMmwMssMCB.dspPreStartCfgShare = (DPIF_MSS_DSS_PreStartCfg *) DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.L3RamObj,
                                                                                     sizeof(DPIF_MSS_DSS_PreStartCfg),
                                                                                     sizeof(uint32_t));
    if (gMmwMssMCB.dspPreStartCfgShare == NULL)
    {
        retVal = DPC_OBJECTDETECTION_ENOMEM__L3_RAM_DSP_CFG;
        goto exit;
    }

    /* Complete the population of the DSP configuration gMmwMssMCB.objDetDynCfg */

    /* Populate radar processing configuration */
    pParam_s->numFrmPerSlidingWindow = gMmwMssMCB.sigProcChainCommonCfg.numFrmPerSlidingWindow;

    pParam_s->numRangeBins    = gMmwMssMCB.numRangeBins;
    pParam_s->rangeFftSize    = gMmwMssMCB.rangeFftSize;
    pParam_s->numTxAntenna    = gMmwMssMCB.numTxAntennas;
    pParam_s->numPhyRxAntenna = gMmwMssMCB.numRxAntennas;
    pParam_s->numAntenna      = pParam_s->numTxAntenna * pParam_s->numPhyRxAntenna;
    if (pParam_s->numTxAntenna > 1)
        pParam_s->mimoModeFlag = 1;
    else
        pParam_s->mimoModeFlag = 0;
    pParam_s->numAdcSamplePerChirp       = gMmwMssMCB.mmWaveCfg.profileComCfg.numOfAdcSamples;
    pParam_s->dynamicCfarConfig.rangeRes = gMmwMssMCB.rangeStep;
    pParam_s->staticCfarConfig.rangeRes  = gMmwMssMCB.rangeStep;
    pParam_s->numChirpPerFrame  = gMmwMssMCB.numDopplerChirps;// * gMmwMssMCB.sigProcChainCommonCfg.numFrmPerSlidingWindow;
    if (gMmwMssMCB.timerDrivenDpcMode == 0)
    {
        pParam_s->framePeriod       = gMmwMssMCB.mmWaveCfg.frameCfg.framePeriodicityus / 1000;
    }
    else
    {
        pParam_s->framePeriod       = gMmwMssMCB.sigProcChainCommonCfg.framePeriodicityus / 1000;
    }
    pParam_s->chirpInterval     = (gMmwMssMCB.mmWaveCfg.profileTimeCfg.chirpIdleTimeus + gMmwMssMCB.mmWaveCfg.profileComCfg.chirpRampEndTimeus) * 1e-6;
    pParam_s->bandwidth         = gMmwMssMCB.bandwidth;
    pParam_s->centerFreq        = gMmwMssMCB.centerFreq;

    pParam_s->dynamicCfarConfig.dopplerRes = gMmwMssMCB.dopplerStep;
    pParam_s->dynamicCfarConfig.cfarType   = DPIF_RADARDEMO_DETECTIONCFAR_RA_CASOCFAR; // hardcoded, only method can be used in this chain
    pParam_s->dynamicCfarConfig.inputType  = DPIF_RADARDEMO_DETECTIONCFAR_INPUTTYPE_SP; // hardcoded, only method can be used in this chain
    pParam_s->staticCfarConfig.cfarType    = DPIF_RADARDEMO_DETECTIONCFAR_RA_CASOCFARV2; // hardcoded, only method can be used in this chain
    pParam_s->staticCfarConfig.inputType   = DPIF_RADARDEMO_DETECTIONCFAR_INPUTTYPE_SP; // hardcoded, only method can be used in this chain
    pParam_s->maxNumDetObj                 = (uint16_t) DPIF_MAX_RESOLVED_OBJECTS_PER_FRAME;


    gMmwMssMCB.dspPreStartCfgLocal.radarCube = gMmwMssMCB.radarCube;

    pParam_s->exportCoarseHeatmap = gMmwMssMCB.dbgGuiMonSel.exportCoarseHeatmap;
    pParam_s->exportRawCfarDetList = gMmwMssMCB.dbgGuiMonSel.exportRawCfarDetList;
    pParam_s->exportZoomInHeatmap = gMmwMssMCB.dbgGuiMonSel.exportZoomInHeatmap;

    pParam_s->disablePointCloudGeneration = gMmwMssMCB.cliPointCloudGenDbgCfg.disablePointCloudGeneration;
exit:
    return retVal;
}
/**
*  @b Description
*  @n
*    Computes range resolution, lambda/dx, lambda/dz
*/
void mmwDemo_computeProfileParams ()
{
    float                bandwidth, centerFreq, adcStart, slope, startFreq;

    gMmwMssMCB.adcStartTime         = (gMmwMssMCB.mmWaveCfg.profileTimeCfg.chirpAdcStartTime) * (1/gMmwMssMCB.adcSamplingRate); //us
    adcStart                        =   (gMmwMssMCB.adcStartTime * 1.e-6);
    startFreq                       =   (float)(gMmwMssMCB.mmWaveCfg.profileTimeCfg.startFreqGHz * 1.e9);
    slope                           =   (float)(gMmwMssMCB.mmWaveCfg.profileTimeCfg.chirpSlope * 1.e12);
    bandwidth                       =   (slope * gMmwMssMCB.mmWaveCfg.profileComCfg.numOfAdcSamples)/(gMmwMssMCB.adcSamplingRate * 1.e6);
    centerFreq                      =   startFreq + bandwidth * 0.5f + adcStart * slope;

    /* Check the running mode: standard architecture, or timer driven RF/DPC */
    if (gMmwMssMCB.sigProcChainCommonCfg.framePeriodicityus > gMmwMssMCB.mmWaveCfg.frameCfg.framePeriodicityus)
    {
        gMmwMssMCB.timerDrivenDpcMode = 1;
    }
    else
    {
        gMmwMssMCB.timerDrivenDpcMode = 0;
    }

    if (gMmwMssMCB.sigProcChainCommonCfg.numOfBurstsInFrame == 0)
    {
            CLI_write("Error in setting number of bursts in frame\n");
            DebugP_assert(0);
    }

    if (gMmwMssMCB.timerDrivenDpcMode == 0)
    {
        if (gMmwMssMCB.sigProcChainCommonCfg.numOfBurstsInFrame > gMmwMssMCB.mmWaveCfg.frameCfg.numOfBurstsInFrame)
        {
            CLI_write("Error in setting number of bursts in frame\n");
            DebugP_assert(0);
        }
        if (gMmwMssMCB.sigProcChainCommonCfg.numOfBurstsInFrame < gMmwMssMCB.mmWaveCfg.frameCfg.numOfBurstsInFrame)
        {
            /* Prolonged (Continuous) bursting mode */
            gMmwMssMCB.prolongedBurstingMode = 1;
        }
        else
        {
            /* Normal bursting mode */
            gMmwMssMCB.prolongedBurstingMode = 0;
        }

    }
    else
    {
        if (gMmwMssMCB.sigProcChainCommonCfg.numOfBurstsInFrame < gMmwMssMCB.mmWaveCfg.frameCfg.numOfFrames)
        {
            /* Prolonged (Continuous) bursting mode */
            gMmwMssMCB.prolongedBurstingMode = 1;
        }
        else
        {
            /* Normal bursting mode */
            gMmwMssMCB.prolongedBurstingMode = 0;
        }
    }
    gMmwMssMCB.rangeStep            =   (MMWDEMO_RFPARSER_SPEED_OF_LIGHT_IN_METERS_PER_SEC * (gMmwMssMCB.adcSamplingRate * 1.e6)) /
                                            (2.f * slope * (2*gMmwMssMCB.numRangeBins));

    if (gMmwMssMCB.timerDrivenDpcMode == 0)
    {
        if (gMmwMssMCB.sigProcChainCommonCfg.numOfBurstsInFrame > 1)
        {
            /* Burst mode: h_NumOfBurstsInFrame > 1 */
            gMmwMssMCB.dopplerStep          =   MMWDEMO_RFPARSER_SPEED_OF_LIGHT_IN_METERS_PER_SEC /
                                                (2.f *  centerFreq * gMmwMssMCB.sigProcChainCommonCfg.numOfBurstsInFrame * gMmwMssMCB.mmWaveCfg.frameCfg.burstPeriodus * 1e-6);
        }
        else
        {
            /* Normal mode: h_NumOfBurstsInFrame = 1, h_NumOfChirpsInBurst >= 2 */
            gMmwMssMCB.dopplerStep          =   MMWDEMO_RFPARSER_SPEED_OF_LIGHT_IN_METERS_PER_SEC /
                                                (2.f * centerFreq * gMmwMssMCB.mmWaveCfg.frameCfg.numOfChirpsInBurst *
                                                (gMmwMssMCB.mmWaveCfg.profileTimeCfg.chirpIdleTimeus + gMmwMssMCB.mmWaveCfg.profileComCfg.chirpRampEndTimeus) * 1e-6);
        }
    }
    else
    {
        if (gMmwMssMCB.sigProcChainCommonCfg.numOfBurstsInFrame > 1)
        {
            /* Burst mode: h_NumOfBurstsInFrame > 1 */
            gMmwMssMCB.dopplerStep          =   MMWDEMO_RFPARSER_SPEED_OF_LIGHT_IN_METERS_PER_SEC /
                                                (2.f *  centerFreq * gMmwMssMCB.sigProcChainCommonCfg.numOfBurstsInFrame * gMmwMssMCB.mmWaveCfg.frameCfg.framePeriodicityus * 1e-6);
        }
        else
        {
            /* Normal mode: h_NumOfBurstsInFrame = 1, h_NumOfChirpsInBurst >= 2 */
            gMmwMssMCB.dopplerStep          =   MMWDEMO_RFPARSER_SPEED_OF_LIGHT_IN_METERS_PER_SEC /
                                                (2.f * centerFreq * gMmwMssMCB.mmWaveCfg.frameCfg.numOfChirpsInBurst *
                                                (gMmwMssMCB.mmWaveCfg.profileTimeCfg.chirpIdleTimeus + gMmwMssMCB.mmWaveCfg.profileComCfg.chirpRampEndTimeus) * 1e-6);
        }
    }
    /*outParams->dopplerResolution    =   MMWDEMO_RFPARSER_SPEED_OF_LIGHT_IN_METERS_PER_SEC /
                                        (2.f * gMmwMssMCB.frameCfg.h_NumOfBurstsInFrame * centerFreq * (gMmwMssMCB.frameCfg.w_BurstPeriodicity));*/
    gMmwMssMCB.bandwidth = bandwidth;
    gMmwMssMCB.centerFreq = centerFreq;

    if (gMmwMssMCB.antennaGeometryCfg.antDistanceXdim == 0.)
    {
        gMmwMssMCB.lambdaOverDistX = 2.0;
    }
    else
    {
        gMmwMssMCB.lambdaOverDistX = 3e8 / (centerFreq * gMmwMssMCB.antennaGeometryCfg.antDistanceXdim);
    }

    if (gMmwMssMCB.antennaGeometryCfg.antDistanceZdim == 0.)
    {
        gMmwMssMCB.lambdaOverDistZ = 2.0;
    }
    else
    {
        gMmwMssMCB.lambdaOverDistZ = 3e8 / (centerFreq * gMmwMssMCB.antennaGeometryCfg.antDistanceZdim);
    }

    gMmwMssMCB.numDopplerChirps = gMmwMssMCB.sigProcChainCommonCfg.numOfBurstsInFrame * gMmwMssMCB.mmWaveCfg.frameCfg.numOfChirpsInBurst * gMmwMssMCB.sigProcChainCommonCfg.numFrmPerSlidingWindow / gMmwMssMCB.numTxAntennas;
    gMmwMssMCB.numDopplerBins = mathUtils_pow2roundup(gMmwMssMCB.numDopplerChirps);
}



/**
*  @b Description
*  @n
*        Function configuring range processing DPU
*/
void mmwDemo_rangeProcConfig(uint32_t *hwaParamSetStartIdx,
                             uint32_t *hwaWinRamOffset)
{
    int32_t retVal = 0;
    uint8_t numUsedHwaParamSets;

    retVal = RangeProc_configParser(hwaParamSetStartIdx,
                                    hwaWinRamOffset);
    if (retVal < 0)
    {
        CLI_write("Error in setting up range profile:%d \n", retVal);
        DebugP_assert(0);
    }

    retVal = DPU_RangeProc_config(gMmwMssMCB.rangeProcDpuHandle, &rangeProcDpuCfg);
    if (retVal < 0)
    {
        CLI_write("Error: RANGE DPU config return error:%d \n", retVal);
        DebugP_assert(0);
    }

    retVal = DPU_RangeProc_GetNumUsedHwaParamSets(gMmwMssMCB.rangeProcDpuHandle,
                                                  &numUsedHwaParamSets);

    *hwaParamSetStartIdx +=  numUsedHwaParamSets;

}

/**
*  @b Description
*  @n
*        Function configuring DOA3D DPU
*/
void mmwDemo_doa3dProcConfig(uint32_t *hwaParamSetStartIdx,
                             uint32_t *hwaWinRamOffset)
{
    int32_t retVal = 0;
    uint8_t numUsedHwaParamSets;

    retVal = Doa3dProc_configParser(hwaParamSetStartIdx,
                                    hwaWinRamOffset);
    if (retVal < 0)
    {
        CLI_write("Error: Error in setting up doa3d profile:%d \n", retVal);
        DebugP_assert(0);
    }

    retVal = DPU_Doa3dProc_config (gMmwMssMCB.doa3dProcDpuHandle, &doa3dProcDpuCfg);
    if (retVal < 0)
    {
        CLI_write("DOA3D DPU configuration return error:%d \n", retVal);
        DebugP_assert(0);
    }

    retVal = DPU_Doa3dProc_GetNumUsedHwaParamSets(gMmwMssMCB.doa3dProcDpuHandle,
                                                  &numUsedHwaParamSets);
    if (retVal < 0)
    {
        CLI_write("DOA3D DPU return error:%d \n", retVal);
        DebugP_assert(0);
    }

    *hwaParamSetStartIdx += numUsedHwaParamSets;
}

/**
*  @b Description
*  @n
*        Function configuring SNR3D DPU
*/
void mmwDemo_snr3dProcConfig(uint32_t *hwaParamSetStartIdx,
                             uint32_t *hwaWinRamOffset)
{
    //uint32_t i;
    int32_t retVal = 0;
    uint8_t numUsedHwaParamSets;

    retVal = Snr3dProc_configParser(hwaParamSetStartIdx,
                                    hwaWinRamOffset);
    if (retVal < 0)
    {
        CLI_write("Error: Error in setting up snr3d profile:%d \n", retVal);
        DebugP_assert(0);
    }

    retVal = DPU_SNR3DHM_config(gMmwMssMCB.snr3dProcDpuHandle, &snr3dProcDpuCfg);
    if (retVal < 0)
    {
        CLI_write("SNR3D DPU configuration return error:%d \n", retVal);
        DebugP_assert(0);
    }

    retVal = DPU_SNR3DHM_GetNumUsedHwaParamSets(gMmwMssMCB.snr3dProcDpuHandle,
                                                &numUsedHwaParamSets);
    if (retVal < 0)
    {
        CLI_write("SNR3D DPU return error:%d \n", retVal);
        DebugP_assert(0);
    }

    *hwaParamSetStartIdx += numUsedHwaParamSets;
}

/**
*  @b Description
*  @n
*        Function configuring SNR3D DPU
*/
void mmwDemo_macroDoppProcConfig(uint32_t *hwaParamSetStartIdx,
                             uint32_t *hwaWinRamOffset)
{
    //uint32_t i;
    int32_t retVal = 0;
    uint8_t numUsedHwaParamSets;

    retVal = macroDoppProc_configParser(hwaParamSetStartIdx,
                                        hwaWinRamOffset);
    if (retVal < 0)
    {
        CLI_write("Error: Error in setting up macroDoppler parser:%d \n", retVal);
        DebugP_assert(0);
    }

    retVal = DPU_MacroDopplerProc_config(gMmwMssMCB.macroDoppProcDpuHandle, &gMacroDoppProcDpuCfg);
    if (retVal < 0)
    {
        CLI_write("MacroDoppler DPU configuration return error:%d \n", retVal);
        DebugP_assert(0);
    }

    retVal = DPU_MacroDopplerProc_GetNumUsedHwaParamSets(gMmwMssMCB.macroDoppProcDpuHandle,
                                                &numUsedHwaParamSets);
    if (retVal < 0)
    {
        CLI_write("MacroDoppler DPU return error:%d \n", retVal);
        DebugP_assert(0);
    }

    *hwaParamSetStartIdx += numUsedHwaParamSets;
}

/**
*  @b Description
*  @n
*        Function configuring Intrusion Detection Algorithm
*/
void mmwDemo_intrusionDetConfig()
{
    //uint32_t i;
    int32_t retVal = 0;
    int32_t errCode = 0;
    IDETECT_moduleConfig config;


    retVal = IntrusionDet_configParser(&config);
    if (retVal < 0)
    {
        CLI_write("Error: Error in setting up Intrusion Detection Configuration:%d \n", retVal);
        DebugP_assert(0);
    }

    gMmwMssMCB.inDetectHandle = inDetect_create(&config, &errCode);
    if (errCode < 0)
    {
        CLI_write("Intrusion Detection Create return error:%d \n", errCode);
        DebugP_assert(0);
    }
}

/**
*  @b Description
*  @n
*        Function configuring Intrusion Detection Algorithm
*/
void mmwDemo_classifierProcConfig()
{
    int32_t retVal = 0;
    DPU_ClassifierProc_Config config;

    memset(&config, 0, sizeof(config));

    gMmwMssMCB.cnnCommonScaleFactor = 140260.1472f; // Correction term: 140260.1472 = (1.0701*2^17)

    /*copy sensor position/orientation from ID to SBR cfg*/
    memcpy(&gMmwMssMCB.featureExtrModuleCfg.sceneryParams.sensorPosition, &gMmwMssMCB.idetSceneryParams.sensorPosition, sizeof(IDETECT_sensorPosition));        //ToDo: CLI saves sensor position/orientation in separate structure
    memcpy(&gMmwMssMCB.featureExtrModuleCfg.sceneryParams.sensorOrientation, &gMmwMssMCB.idetSceneryParams.sensorOrientation, sizeof(IDETECT_sensorOrientation));

    /* Set point cloud compression units for feature extraction */
    gMmwMssMCB.featureExtrModuleCfg.pointCloudCompressionUnit.xUnit =  3.0f/128.0f;
    gMmwMssMCB.featureExtrModuleCfg.pointCloudCompressionUnit.yUnit =  3.0f/128.0f;
    gMmwMssMCB.featureExtrModuleCfg.pointCloudCompressionUnit.zUnit =  3.0f/128.0f;
    gMmwMssMCB.featureExtrModuleCfg.pointCloudCompressionUnit.snrdBUnit =  .5f;
    gMmwMssMCB.featureExtrModuleCfg.pointCloudCompressionUnit.dopplerUnit =  gMmwMssMCB.dopplerStep * (float)(mathUtils_pow2roundup(gMmwMssMCB.sigProcChainCommonCfg.numOfBurstsInFrame)/2)  /128.0f;

    /* Static config */
    gMmwMssMCB.featureExtrModuleCfg.cartesianInput = false;
    config.staticCfg.featureExtrModuleCfg = gMmwMssMCB.featureExtrModuleCfg; //Copy structure

    config.staticCfg.runningMode = gMmwMssMCB.runningMode;
    config.staticCfg.cpdOption = gMmwMssMCB.cpdOption;
    config.staticCfg.macroDopplerFeatureEnabled = gMmwMssMCB.cliMacroDopplerCfg.macroDopplerFeatureEnabled;


    if (config.staticCfg.cpdOption == DPU_CLASSIFIERPROC_CPD_MODE_LPD_USING_CNN)
    {
        config.staticCfg.multiFrmDopplerFftSize = 128;  //ToDo read from CLI command
        config.res.featureMultiFrmDoppler = (float *) DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.L3RamObj,
                                                                              config.staticCfg.multiFrmDopplerFftSize * sizeof(float),
                                                                              sizeof(float));
        if (config.res.featureMultiFrmDoppler == NULL)
        {
            CLI_write("ClassifierProc DPU Configuration memoryallocation failed\n");
            DebugP_assert(0);
        }

        /* Copy macro doppler map scales */
        for (uint8_t ii = 0; ii < gMmwMssMCB.featureExtrModuleCfg.sceneryParams.numOccupancyBoxes; ii++)
        {
            if (gMmwMssMCB.cliMacroDopplerMapScale.cnnInputScale[ii] == 0)
            {
                CLI_write("ClassifierProc DPU Configuration: CNN scale equal to zero\n");  //ToDo Do we check this for not being zero?
                DebugP_assert(0);
            }
            config.staticCfg.cnnInputScale[ii] = gMmwMssMCB.cliMacroDopplerMapScale.cnnInputScale[ii] * gMmwMssMCB.cnnCommonScaleFactor; // Correction term: 140260.1472 = (1.0701*2^17)
        }
        gMmwMssMCB.cliMacroDopplerMapScaleCmdPending = 0;
    }
    else
    {
         config.res.featureMultiFrmDoppler = NULL;
    }

    retVal = DPU_ClassifierProc_config(gMmwMssMCB.classifierDpuHandle, &config);
    if (retVal != 0)
    {
        CLI_write("ClassifierProc DPU Configuration return error:%d \n", retVal);
        DebugP_assert(0);
    }

    gMmwMssMCB.cli_zOffsetCmdPending = 0;
}


/**
*  @b Description
*  @n
*        Function configuring configures DSP for SBR/CPD mode
*/
void mmwDemo_dspConfig()
{
    //uint32_t i;
    int32_t retVal = 0;
    MsgIpc_Cfg msgIpcCfg;

    if(!gMmwMssMCB.msgIpcCtrlObj.isMsgIpcInitialized)
    {
        /* Configure IPC */
        msgIpcCfg.msgChanId = 1;
        msgIpcCfg.remoteCoreId = CSL_CORE_ID_C66SS0;
        msgIpcCfg.msgCallback = (IpcNotify_FxnCallback) DPC_mss_MsgHandler;
        msgIpcCfg.arg = NULL;

        MsgIpc_Config(&gMmwMssMCB.msgIpcCtrlObj, &msgIpcCfg);

        /* Create Classifier task semaphore */
        retVal = SemaphoreP_constructBinary(&gMmwMssMCB.classifierTaskSemHandle, 0);
        DebugP_assert(SystemP_SUCCESS == retVal);
        retVal = SemaphoreP_constructBinary(&gMmwMssMCB.classifierTaskSem2Handle, 0);
        DebugP_assert(SystemP_SUCCESS == retVal);
    }
    /* Create Classifier task */
    gClassifierTask = xTaskCreateStatic(MmwDemo_ClassifierTask, /* Pointer to the function that implements the task. */
                             "Classifier_task",      /* Text name for the task.  This is to facilitate debugging only. */
                             CLASSIFIER_TASK_STACK_SIZE,   /* Stack depth in units of StackType_t typically uint32_t on 32b CPUs */
                             NULL,                  /* We are not using the task parameter. */
                             CLASSIFIER_TASK_PRIORITY,          /* task priority, 0 is lowest priority, configMAX_PRIORITIES-1 is highest */
                             gClassifierTaskStack,      /* pointer to stack base */
                             &gClassifierTaskObj);         /* pointer to statically allocated task object memory */
    configASSERT(gClassifierTask != NULL);


    /* Parser */
    retVal = dsp_configParser();
    if (retVal < 0)
    {
        CLI_write("Error: Error in setting up DSP Configuration:%d \n", retVal);
        DebugP_assert(0);
    }

    if(!gMmwMssMCB.msgIpcCtrlObj.isMsgIpcInitialized)
    {
        /* Create semaphore to wait for DSP configuration */
        SemaphoreP_constructBinary(&gMmwMssMCB.dspCfgDoneSemaphore, 0);
        /* Sync with DSP */
        MsgIpc_Sync();
        gMmwMssMCB.msgIpcCtrlObj.isMsgIpcInitialized = TRUE;
    }


    /* Copy pre start configuration from local to shared memory location */
    memcpy(gMmwMssMCB.dspPreStartCfgShare, &gMmwMssMCB.dspPreStartCfgLocal, sizeof(DPIF_MSS_DSS_PreStartCfg));

    /* Send DSP configuration */
    MsgIpc_sendMessage(&gMmwMssMCB.msgIpcCtrlObj, DPC_MSS_TO_DSS_PRE_START_CONFIG, (uint32_t) gMmwMssMCB.dspPreStartCfgShare);

    /* Wait for DSP configuration completion */
    SemaphoreP_pend(&gMmwMssMCB.dspCfgDoneSemaphore, SystemP_WAIT_FOREVER);

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
    if (gMmwMssMCB.timerDrivenDpcMode == 0)
    {
        if(MmwDemo_registerFrameStartInterrupt() != 0){
            CLI_write("Error: Failed to register frame start interrupts\n");
            DebugP_assert(0);
        }
    }

/* For debugging purposes*/
/* NOTE: Disabled for the time being */
//#ifdef ENABLE_CHIRP_AVAILABLE_INTERRUPT
//    if(MmwDemo_registerChirpAvailableInterrupts() != 0)
//    {
//        CLI_write("Failed to register chirp available interrupts\n");
//        DebugP_assert(0);
//    }
//#endif
#if 0
    MmwDemo_registerChirpInterrupt();
#endif
#ifdef ENABLE_BURST_INTERRUPT
    MmwDemo_registerBurstInterrupt();
#endif

    int32_t status = SystemP_SUCCESS;

    /* Shared memory pool */
    gMmwMssMCB.L3RamObj.cfg.addr = (void *)&gMmwL3[0];
    gMmwMssMCB.L3RamObj.cfg.size = sizeof(gMmwL3);

    /* Local memory pool */
    gMmwMssMCB.CoreLocalRamObj.cfg.addr = (void *)&gMmwCoreLocMem[0];
    gMmwMssMCB.CoreLocalRamObj.cfg.size = sizeof(gMmwCoreLocMem);

    if (!gMmwMssMCB.oneTimeConfigDone)
    {
        /* Memory pool for the ID/CPD/SBR */
        HeapP_construct(&gMmwMssMCB.CoreLocalRtosHeapObj, (void *) gMmwCoreLocMem2, MSS_CORE_LOCAL_MEM2_SIZE);
    }

    gHwaHandle = HWA_open(0, NULL, &status);
    if (gHwaHandle == NULL)
    {
        CLI_write("Error: Unable to open the HWA Instance err:%d\n", status);
        DebugP_assert(0);
    }


    rangeProc_dpuInit();
    doa3dProc_dpuInit();
    snr3dProc_dpuInit();
    macroDoppProc_dpuInit();
    classifierProc_dpuInit();

}

//For debugging ToDo: remove
//extern uint32_t gTimerDrivenArch_profile[16];
//extern uint32_t gTimerDrivenArch_profileInd;

extern uint32_t gDebugTargetCode;

/**
*  @b Description
*  @n
*    Frame start ISR - used in timer driven RF/DPC mode
*/
void DPC_FrameStartISR(Edma_IntrHandle intrHandle, void *arg)
{
    uint64_t l_demoStartTimeUs;
    unsigned long long ll_startTimeSlowClk;

    uint32_t curCycle;
    MmwDemo_MSS_MCB *mmwMssMCB = (MmwDemo_MSS_MCB *) arg;

    if (mmwMssMCB->timerDrivenArchObj.firstFrameStartIsrStarted == 0)
    {
        ClockP_start(&mmwMssMCB->timerDrivenArchObj.clockObj);
        mmwMssMCB->timerDrivenArchObj.firstFrameStartIsrStarted = 1;
    }
    /* Capture the frame start time using FreeRTOS timer */
    curCycle = Cycleprofiler_getTimeStamp();

//For debugging ToDo: remove:
//    if(gTimerDrivenArch_profileInd < 16)
//    {
//        gTimerDrivenArch_profile[gTimerDrivenArch_profileInd++] = curCycle;
//    }

    l_demoStartTimeUs = ClockP_getTimeUsec();
    /* Capture the frame start time using the Slow Clock. This is needed when Low power mode is enabled */
    ll_startTimeSlowClk = PRCMSlowClkCtrGet();

    if (gMmwMssMCB.lowPowerMode == LOW_PWR_MODE_DISABLE)
    {
        /* For testing */
        mmwMssMCB->stats.framePeriod_us = (curCycle - mmwMssMCB->stats.frameStartTimeStamp[(mmwMssMCB->stats.frameStartIntCounter-1) & 0x3])/FRAME_REF_TIMER_CLOCK_MHZ;
        mmwMssMCB->stats.frameStartTimeStamp[mmwMssMCB->stats.frameStartIntCounter & 0x3] = curCycle;


        GPIO_pinWriteHigh(gGpioBaseAddrLed, gPinNumLed);
    }
    else
    {
        /* FreeRTOS timer is shutdown during Low Power mode. Hence Slow Clock has to be used when Low power mode is enabled */
        mmwMssMCB->stats.framePeriod_us = round((ll_startTimeSlowClk - mmwMssMCB->stats.frameStartTimeStampSlowClk) * M_TICKS_TO_USEC_SLOWCLK);
    }
    mmwMssMCB->stats.frameStartTimeStampUs = l_demoStartTimeUs;

    mmwMssMCB->stats.frameStartTimeStampSlowClk = ll_startTimeSlowClk;


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

    if ((mmwMssMCB->runningMode == DPC_RUNNING_MODE_INDET) && (mmwMssMCB->lowPowerMode == LOW_PWR_MODE_ENABLE))
    {
        //Change to oscilator clock (40MHz)
        SOC_rcmSetR5Clock(SOC_RCM_XTAL_CLK_40MHZ,SOC_RCM_XTAL_CLK_40MHZ, SOC_RcmR5ClockSource_OSC_CLK);
    }

    mmwMssMCB->timerDrivenArchObj.frameStartIntCounter++;
    if (mmwMssMCB->timerDrivenArchObj.numFrames > 0)
    {
        if (mmwMssMCB->timerDrivenArchObj.frameStartIntCounter == mmwMssMCB->timerDrivenArchObj.numFrames)
        {
            ClockP_stop(&mmwMssMCB->timerDrivenArchObj.clockObj);
        }
    }
}


/**
*  @b Description
*  @n

*        For timer driven DPC architecture
*        Frame start event triggers EDMA, that calls Frame start ISR at the begining of the frame.
*/
int32_t mmwDemo_TimerDrivenDpcArchEDMAConfig(DPC_TimerDrivenArchObj *obj)
{
    EDMA_Handle edmaHandle = obj->edmaHandle;
    int32_t                 errorCode = SystemP_SUCCESS;
    uint32_t                edmaReturn;
    int32_t                 idx;
    int32_t                 numParamSets;
    uint32_t                dmaCh, tcc, param, chType;
    uint32_t                baseAddr, regionId;
    EDMACCPaRAMEntry        shadowParam;
    uint16_t                linkChId0;
    uint16_t                linkChId1;


    /* Configure Frame Start EDMA that triggers Frame Start ISR */
    baseAddr = EDMA_getBaseAddr(edmaHandle);
    DebugP_assert(baseAddr != 0);

    regionId = EDMA_getRegionId(edmaHandle);
    DebugP_assert(regionId < SOC_EDMA_NUM_REGIONS);

    chType = (uint8_t)EDMA_CHANNEL_TYPE_DMA;
    dmaCh = obj->edmaFrameStart.channel;
    param = obj->edmaFrameStart.channel;
    tcc = obj->edmaFrameStart.channel;
    if ((edmaReturn = DPEDMA_configDummyChannel(edmaHandle, chType, &dmaCh, &tcc, &param)) != SystemP_SUCCESS)
    {
        errorCode = DPC_OBJECTDETECTION_ERROR_TIMER_DRIVEN_DPC_ARCH_CFG;
        goto exit;
    }

    numParamSets = 2;

    /* Program Shadow Param Sets */
    EDMACCPaRAMEntry_init(&shadowParam);

    for (idx = 0; idx < numParamSets; idx++)
    {
        memset(&shadowParam, 0, sizeof(EDMACCPaRAMEntry));
        shadowParam.srcAddr = (uint32_t) obj->dummySrcPtr;
        shadowParam.destAddr = (uint32_t) obj->dummyDstPtr;
        shadowParam.aCnt = 4;
        shadowParam.bCnt = 1;
        if(idx==0)
        {
            shadowParam.cCnt = 1;
        }
        else
        {
            shadowParam.cCnt = obj->numInEvents - 1;
        }
        shadowParam.bCntReload = shadowParam.bCnt;

        shadowParam.srcBIdx = 0;
        shadowParam.destBIdx = 0;

        shadowParam.srcCIdx = 0;
        shadowParam.destCIdx = 0;

        if(idx==0)
        {
            shadowParam.opt          |= (EDMA_OPT_TCINTEN_MASK);
            shadowParam.opt          |=
             (((((uint32_t)tcc) << EDMA_OPT_TCC_SHIFT) & EDMA_OPT_TCC_MASK) |
             ((((uint32_t)EDMA_SYNC_AB) << EDMA_OPT_SYNCDIM_SHIFT) & EDMA_OPT_SYNCDIM_MASK));
        }
        else
        {
            shadowParam.opt          |=
            (((((uint32_t)tcc) << EDMA_OPT_TCC_SHIFT) & EDMA_OPT_TCC_MASK) |
             ((((uint32_t)EDMA_SYNC_AB) << EDMA_OPT_SYNCDIM_SHIFT) & EDMA_OPT_SYNCDIM_MASK));
        }

        EDMASetPaRAM(baseAddr,
                      obj->edmaFrameStart.ShadowPramId[idx],
                      &shadowParam);
    }
    /**********************************************/
    /* Link physical channel and param sets       */
    /**********************************************/
    /* Get LinkChan from configuraiton */
    linkChId0 = obj->edmaFrameStart.ShadowPramId[0];
    linkChId1 = obj->edmaFrameStart.ShadowPramId[1];

    /* Link 2 shadow Param sets */
    if ((errorCode = DPEDMA_linkParamSets(edmaHandle,
                                          param,
                                          linkChId0)) != SystemP_SUCCESS)
    {
        goto exit;
    }
    if ((errorCode = DPEDMA_linkParamSets(edmaHandle,
                                          linkChId0,
                                          linkChId1)) != SystemP_SUCCESS)
    {
        goto exit;
    }
    if ((errorCode = DPEDMA_linkParamSets(edmaHandle,
                                          linkChId1,
                                          linkChId0)) != SystemP_SUCCESS)
    {
        goto exit;
    }

    /********************************/
    /* Register ISR                 */
    /********************************/
    obj->intrObj.tccNum = obj->edmaFrameStart.channel;
    obj->intrObj.cbFxn  = (Edma_EventCallback) DPC_FrameStartISR;
    obj->intrObj.appData = (void *) &gMmwMssMCB;
    errorCode = EDMA_registerIntr(edmaHandle, &obj->intrObj);
    if (errorCode != SystemP_SUCCESS)
    {
        goto exit;
    }

    /********************************/
    /* Bring in the first param set */
    /********************************/
    edmaReturn = EDMAEnableTransferRegion(baseAddr, regionId, dmaCh, EDMA_TRIG_MODE_MANUAL);
    if (edmaReturn != TRUE)
    {
       errorCode = DPC_OBJECTDETECTION_ERROR_TIMER_DRIVEN_DPC_ARCH_CFG;
       goto exit;
    }

    /********************************/
    /* Enable event                 */
    /********************************/
    edmaReturn = EDMAEnableTransferRegion(baseAddr, regionId, dmaCh, EDMA_TRIG_MODE_EVENT);
    if (edmaReturn != TRUE)
    {
       errorCode = DPC_OBJECTDETECTION_ERROR_TIMER_DRIVEN_DPC_ARCH_CFG;
       goto exit;
    }

exit:
    return errorCode;
}

/**
*  @b Description
*  @n

*        Configuration for Prolonged (Continuous) bursting mode. EDMA configuration. Configures 
*        one EDMA channel with two shadow channels, EDMA channel is triggered by chirp available event, 
*        the first shadow PARAM set passses through the evnts to range DPU, the second one is dummy 
*        and gates the remaining events.
*/
int32_t DPC_ProlonedBurstingConfig(DPC_prolonedBurstingObj *obj)
{
    EDMA_Handle edmaHandle = obj->edmaHandle;
    int32_t                 errorCode = SystemP_SUCCESS;
    uint32_t                edmaReturn;
    int32_t                 idx;
    int32_t                 numParamSets;
    uint32_t                dmaCh, tcc, param, chType;
    uint32_t                baseAddr, regionId;
    EDMACCPaRAMEntry        shadowParam;
    uint16_t                linkChId0;
    uint16_t                linkChId1;
    uint16_t                linkChId2;


    baseAddr = EDMA_getBaseAddr(edmaHandle);
    DebugP_assert(baseAddr != 0);

    regionId = EDMA_getRegionId(edmaHandle);
    DebugP_assert(regionId < SOC_EDMA_NUM_REGIONS);

    chType = (uint8_t)EDMA_CHANNEL_TYPE_DMA;
    dmaCh = obj->edmaEvtSplit.channel;
    param = obj->edmaEvtSplit.channel;
    tcc = obj->edmaChainChannel;
    if ((edmaReturn = DPEDMA_configDummyChannel(edmaHandle, chType, &dmaCh, &tcc, &param)) != SystemP_SUCCESS)
    {
        errorCode = DPC_OBJECTDETECTION_ERROR_PROLONGED_BURSTING_MODE_CFG;
        goto exit;
    }

    /* Program Shadow Param Sets */
    EDMACCPaRAMEntry_init(&shadowParam);

    if (obj->startPassThroughEventIdx == 0)
    {
        numParamSets = 2;

        for (idx = 0; idx < numParamSets; idx++)
        {
            memset(&shadowParam, 0, sizeof(EDMACCPaRAMEntry));
            shadowParam.srcAddr = (uint32_t) obj->dummySrcPtr;
            shadowParam.destAddr = (uint32_t) obj->dummyDstPtr;
            shadowParam.aCnt = 4;
            shadowParam.bCnt = 1;
            if(idx==0)
            {
                shadowParam.cCnt = obj->numOutEvents;
            }
            else
            {
                shadowParam.cCnt = obj->numInEvents - obj->numOutEvents;
            }
            shadowParam.bCntReload = shadowParam.bCnt;

            shadowParam.srcBIdx = 0;
            shadowParam.destBIdx = 0;

            shadowParam.srcCIdx = 0;
            shadowParam.destCIdx = 0;

            if(idx==0)
            {
                shadowParam.opt          |=
                (EDMA_OPT_TCCHEN_MASK | EDMA_OPT_ITCCHEN_MASK |
                 ((((uint32_t)tcc) << EDMA_OPT_TCC_SHIFT) & EDMA_OPT_TCC_MASK) |
                 ((((uint32_t)EDMA_SYNC_AB) << EDMA_OPT_SYNCDIM_SHIFT) & EDMA_OPT_SYNCDIM_MASK));
            }
            else
            {
                shadowParam.opt          |=
                (((((uint32_t)tcc) << EDMA_OPT_TCC_SHIFT) & EDMA_OPT_TCC_MASK) |
                 ((((uint32_t)EDMA_SYNC_AB) << EDMA_OPT_SYNCDIM_SHIFT) & EDMA_OPT_SYNCDIM_MASK));
            }

            EDMASetPaRAM(baseAddr,
                          obj->edmaEvtSplit.ShadowPramId[idx],
                          &shadowParam);
        }
        /**********************************************/
        /* Link physical channel and param sets       */
        /**********************************************/
        /* Get LinkChan from configuraiton */
        linkChId0 = obj->edmaEvtSplit.ShadowPramId[0];
        linkChId1 = obj->edmaEvtSplit.ShadowPramId[1];

        /* Link 2 shadow Param sets */
        if ((errorCode = DPEDMA_linkParamSets(edmaHandle,
                                              param,
                                              linkChId0)) != SystemP_SUCCESS)
        {
            goto exit;
        }
        if ((errorCode = DPEDMA_linkParamSets(edmaHandle,
                                              linkChId0,
                                              linkChId1)) != SystemP_SUCCESS)
        {
            goto exit;
        }
        if ((errorCode = DPEDMA_linkParamSets(edmaHandle,
                                              linkChId1,
                                              linkChId0)) != SystemP_SUCCESS)
        {
            goto exit;
        }
    }
    else if ((obj->numInEvents - obj->startPassThroughEventIdx - obj->numOutEvents) > 0)
    {
        numParamSets = 3;
        for (idx = 0; idx < numParamSets; idx++)
        {
            memset(&shadowParam, 0, sizeof(EDMACCPaRAMEntry));
            shadowParam.srcAddr = (uint32_t) obj->dummySrcPtr;
            shadowParam.destAddr = (uint32_t) obj->dummyDstPtr;
            shadowParam.aCnt = 4;
            shadowParam.bCnt = 1;
            if(idx==0)
            {
                shadowParam.cCnt = obj->startPassThroughEventIdx;
            }
            else if(idx==1)
            {
                shadowParam.cCnt = obj->numOutEvents;
            }
            else
            {
                shadowParam.cCnt = obj->numInEvents - obj->startPassThroughEventIdx - obj->numOutEvents;
            }
            shadowParam.bCntReload = shadowParam.bCnt;

            shadowParam.srcBIdx = 0;
            shadowParam.destBIdx = 0;

            shadowParam.srcCIdx = 0;
            shadowParam.destCIdx = 0;

            if ((idx==0) || (idx==2))
            {
                shadowParam.opt          |=
                (((((uint32_t)tcc) << EDMA_OPT_TCC_SHIFT) & EDMA_OPT_TCC_MASK) |
                 ((((uint32_t)EDMA_SYNC_AB) << EDMA_OPT_SYNCDIM_SHIFT) & EDMA_OPT_SYNCDIM_MASK));
            }
            else
            {
                shadowParam.opt          |=
                (EDMA_OPT_TCCHEN_MASK | EDMA_OPT_ITCCHEN_MASK |
                 ((((uint32_t)tcc) << EDMA_OPT_TCC_SHIFT) & EDMA_OPT_TCC_MASK) |
                 ((((uint32_t)EDMA_SYNC_AB) << EDMA_OPT_SYNCDIM_SHIFT) & EDMA_OPT_SYNCDIM_MASK));
            }

            EDMASetPaRAM(baseAddr,
                          obj->edmaEvtSplit.ShadowPramId[idx],
                          &shadowParam);
        }
        /**********************************************/
        /* Link physical channel and param sets       */
        /**********************************************/
        /* Get LinkChan from configuraiton */
        linkChId0 = obj->edmaEvtSplit.ShadowPramId[0];
        linkChId1 = obj->edmaEvtSplit.ShadowPramId[1];
        linkChId2 = obj->edmaEvtSplit.ShadowPramId[2];

        /* Link 2 shadow Param sets */
        if ((errorCode = DPEDMA_linkParamSets(edmaHandle,
                                              param,
                                              linkChId0)) != SystemP_SUCCESS)
        {
            goto exit;
        }
        if ((errorCode = DPEDMA_linkParamSets(edmaHandle,
                                              linkChId0,
                                              linkChId1)) != SystemP_SUCCESS)
        {
            goto exit;
        }
        if ((errorCode = DPEDMA_linkParamSets(edmaHandle,
                                              linkChId1,
                                              linkChId2)) != SystemP_SUCCESS)
        {
            goto exit;
        }
        if ((errorCode = DPEDMA_linkParamSets(edmaHandle,
                                              linkChId2,
                                              linkChId0)) != SystemP_SUCCESS)
        {
            goto exit;
        }
    }
    else if (obj->numInEvents == (obj->startPassThroughEventIdx - obj->numOutEvents))
    {
        numParamSets = 2;

        for (idx = 0; idx < numParamSets; idx++)
        {
            memset(&shadowParam, 0, sizeof(EDMACCPaRAMEntry));
            shadowParam.srcAddr = (uint32_t) obj->dummySrcPtr;
            shadowParam.destAddr = (uint32_t) obj->dummyDstPtr;
            shadowParam.aCnt = 4;
            shadowParam.bCnt = 1;
            if(idx==0)
            {
                shadowParam.cCnt = obj->numInEvents - obj->numOutEvents;
            }
            else
            {
                shadowParam.cCnt = obj->numOutEvents;
            }
            shadowParam.bCntReload = shadowParam.bCnt;

            shadowParam.srcBIdx = 0;
            shadowParam.destBIdx = 0;

            shadowParam.srcCIdx = 0;
            shadowParam.destCIdx = 0;

            if(idx==0)
            {
                shadowParam.opt          |=
                (((((uint32_t)tcc) << EDMA_OPT_TCC_SHIFT) & EDMA_OPT_TCC_MASK) |
                 ((((uint32_t)EDMA_SYNC_AB) << EDMA_OPT_SYNCDIM_SHIFT) & EDMA_OPT_SYNCDIM_MASK));
            }
            else
            {
                shadowParam.opt          |=
                (EDMA_OPT_TCCHEN_MASK | EDMA_OPT_ITCCHEN_MASK |
                 ((((uint32_t)tcc) << EDMA_OPT_TCC_SHIFT) & EDMA_OPT_TCC_MASK) |
                 ((((uint32_t)EDMA_SYNC_AB) << EDMA_OPT_SYNCDIM_SHIFT) & EDMA_OPT_SYNCDIM_MASK));
            }

            EDMASetPaRAM(baseAddr,
                          obj->edmaEvtSplit.ShadowPramId[idx],
                          &shadowParam);
        }
        /**********************************************/
        /* Link physical channel and param sets       */
        /**********************************************/
        /* Get LinkChan from configuraiton */
        linkChId0 = obj->edmaEvtSplit.ShadowPramId[0];
        linkChId1 = obj->edmaEvtSplit.ShadowPramId[1];

        /* Link 2 shadow Param sets */
        if ((errorCode = DPEDMA_linkParamSets(edmaHandle,
                                              param,
                                              linkChId0)) != SystemP_SUCCESS)
        {
            goto exit;
        }
        if ((errorCode = DPEDMA_linkParamSets(edmaHandle,
                                              linkChId0,
                                              linkChId1)) != SystemP_SUCCESS)
        {
            goto exit;
        }
        if ((errorCode = DPEDMA_linkParamSets(edmaHandle,
                                              linkChId1,
                                              linkChId0)) != SystemP_SUCCESS)
        {
            goto exit;
        }
    }
    else
    {
       errorCode = DPC_OBJECTDETECTION_ERROR_PROLONGED_BURSTING_MODE_CFG;
       goto exit;
    }

    /********************************/
    /* Bring in the first param set */
    /********************************/
    edmaReturn = EDMAEnableTransferRegion(baseAddr, regionId, dmaCh, EDMA_TRIG_MODE_MANUAL);
    if (edmaReturn != TRUE)
    {
       errorCode = DPC_OBJECTDETECTION_ERROR_PROLONGED_BURSTING_MODE_CFG;
       goto exit;
    }

    /********************************/
    /* Enable event                 */
    /********************************/
    edmaReturn = EDMAEnableTransferRegion(baseAddr, regionId, dmaCh, EDMA_TRIG_MODE_EVENT);
    if (edmaReturn != TRUE)
    {
       errorCode = DPC_OBJECTDETECTION_ERROR_PROLONGED_BURSTING_MODE_CFG;
       goto exit;
    }

exit:
    return errorCode;
}

/**
*  @b Description
*  @n

*        Timer driven RF/DPC architecture configuration
*/
void mmwDemo_TimerDrivenDpcArchConfig()
{
    int32_t retCode;
    DPC_TimerDrivenArchObj *obj = &gMmwMssMCB.timerDrivenArchObj;
    memset(obj, 0, sizeof(DPC_TimerDrivenArchObj));


    obj->numFrames = gMmwMssMCB.sigProcChainCommonCfg.numOfFrames;

    /*EDMA related*/
    obj->edmaHandle = gEdmaHandle[0];
    obj->edmaFrameStart.channel = DPC_OBJDET_FRAME_START_CH;
    obj->edmaFrameStart.ShadowPramId[0] = DPC_OBJDET_FRAME_START_GATE_ON_SHADOW_0;
    obj->edmaFrameStart.ShadowPramId[1] = DPC_OBJDET_FRAME_START_GATE_OFF_SHADOW_1;
    obj->edmaFrameStart.eventQueue = DPC_OBJDET_PROLONGED_BURSTING_IN_EVENT_QUE;

    obj->numInEvents = gMmwMssMCB.mmWaveCfg.frameCfg.numOfFrames;

    obj->dummySrcPtr = (uint32_t *) DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.L3RamObj, sizeof(uint32_t), sizeof(uint32_t));
    if(obj->dummySrcPtr == NULL)
    {
        CLI_write("DPC L3 memory allocation failed\n");
        DebugP_assert(0);
    }

    obj->dummyDstPtr = (uint32_t *) DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.L3RamObj, sizeof(uint32_t), sizeof(uint32_t));
    if(obj->dummyDstPtr == NULL)
    {
        CLI_write("DPC L3 memory allocation failed\n");
        DebugP_assert(0);
    }

    /* Configure EDMA */
    retCode = mmwDemo_TimerDrivenDpcArchEDMAConfig(obj);
    if (retCode < 0)
    {
        CLI_write("Error: EDMA configuration for timer driven RF/DPC mode \n");
        DebugP_assert(0);
    }

    /* Configure sensor start clock */
    {
        ClockP_Params clockParams;

        ClockP_Params_init(&clockParams);

        clockParams.timeout = ClockP_usecToTicks(gMmwMssMCB.sigProcChainCommonCfg.framePeriodicityus);
        clockParams.period = ClockP_usecToTicks(gMmwMssMCB.sigProcChainCommonCfg.framePeriodicityus);
        clockParams.start = 0;
        clockParams.callback = DPC_SensorStartClockCallback;
        clockParams.args = &gPeriodicCount; /* pass address of counter which is incremented in the callback */

        ClockP_construct(&obj->clockObj, &clockParams);
    }

}

/**
*  @b Description
*  @n

*        Preparation for Prolonged (Continuous) bursting mode configuration, and configuration
*/
void mmwDemo_ProlongedBurstingConfig()
{
    int32_t retCode;
    DPC_prolonedBurstingObj *obj = &gMmwMssMCB.prolongedBurstingObj;
    memset(obj, 0, sizeof(DPC_prolonedBurstingObj));
    obj->edmaHandle = gEdmaHandle[0];

    obj->edmaEvtSplit.channel = DPC_OBJDET_DPU_RANGEPROC_EDMAIN_CH;
    obj->edmaEvtSplit.ShadowPramId[0] = DPC_OBJDET_PROLONGED_BURSTING_IN_SHADOW_0;
    obj->edmaEvtSplit.ShadowPramId[1] = DPC_OBJDET_PROLONGED_BURSTING_IN_SHADOW_1;
    obj->edmaEvtSplit.ShadowPramId[2] = DPC_OBJDET_PROLONGED_BURSTING_IN_SHADOW_2;
    obj->edmaEvtSplit.eventQueue = DPC_OBJDET_PROLONGED_BURSTING_IN_EVENT_QUE;

    obj->edmaChainChannel = DPC_OBJDET_PROLONGED_BURSTING_IN_CH;

    if(gMmwMssMCB.timerDrivenDpcMode == 0)
    {
        obj->numInEvents = gMmwMssMCB.mmWaveCfg.frameCfg.numOfBurstsInFrame * gMmwMssMCB.mmWaveCfg.frameCfg.numOfChirpsInBurst;
    }
    else
    {
        obj->numInEvents = gMmwMssMCB.mmWaveCfg.frameCfg.numOfFrames * gMmwMssMCB.mmWaveCfg.frameCfg.numOfChirpsInBurst;
    }
    obj->numOutEvents = gMmwMssMCB.sigProcChainCommonCfg.numOfBurstsInFrame * gMmwMssMCB.mmWaveCfg.frameCfg.numOfChirpsInBurst;
    obj->startPassThroughEventIdx = gMmwMssMCB.sigProcChainCommonCfg.startBurstIdx;

    DebugP_assert(((int32_t)obj->numInEvents - (int32_t)obj->startPassThroughEventIdx) >= (int32_t)obj->numOutEvents);

    obj->dummySrcPtr = (uint32_t *) DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.L3RamObj, sizeof(uint32_t), sizeof(uint32_t));
    if(obj->dummySrcPtr == NULL)
    {
        CLI_write("DPC L3 memory allocation failed\n");
        DebugP_assert(0);
    }

    obj->dummyDstPtr = (uint32_t *) DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.L3RamObj, sizeof(uint32_t), sizeof(uint32_t));
    if(obj->dummyDstPtr == NULL)
    {
        CLI_write("DPC L3 memory allocation failed\n");
        DebugP_assert(0);
    }

    retCode = DPC_ProlonedBurstingConfig(obj);
    if (retCode < 0)
    {
        CLI_write("DPC Configuration of continuous bursting mode failed\n");
        DebugP_assert(0);
    }

}

/**
*  @b Description
*  @n

*        Function configuring all DPUs
*/
void DPC_Config()
{

    int32_t retVal;
    uint32_t hwaWindowOffset;
    uint32_t hwaParamSetStartIdx;

    /*TODO Cleanup: MMWLPSDK-237*/
    
    DPC_ObjDet_MemPoolReset(&gMmwMssMCB.L3RamObj);
    DPC_ObjDet_MemPoolReset(&gMmwMssMCB.CoreLocalRamObj);
    DPC_ObjDet_HwaDmaTrigSrcChanPoolReset(&gMmwMssMCB.HwaDmaChanPoolObj);
    DPC_ObjDet_HwaWinRamMemoryPoolReset(&gMmwMssMCB.HwaWinRamMemoryPoolObj);

    if (gMmwMssMCB.adcLogging.enable == 1)
    {
        if(MmwDemo_registerChirpAvailableInterrupts() != 0)
        {
            CLI_write("Failed to register chirp available interrupts\n");
            DebugP_assert(0);
        }
    }

    if (!gMmwMssMCB.oneTimeConfigDone)
    {
        /* Reset memory for feature extraction */
        HeapP_construct(&gMmwMssMCB.CoreLocalRtosHeapObj, (void *) gMmwCoreLocMem2, MSS_CORE_LOCAL_MEM2_SIZE);
    }

    // gMmwMssMCB.dpcObjOut = (DPIF_PointCloudCartesian *) DPC_ObjDet_MemPoolAlloc(&gMmwMssMCB.L3RamObj,
    //                                                                                    MAX_NUM_DETECTIONS * sizeof(DPIF_PointCloudCartesian),
    //                                                                                    sizeof(uint32_t));
    // if (gMmwMssMCB.dpcObjOut == NULL)
    // {
    //     CLI_write("DPC configuration: memory allocation failed\n");
    //     DebugP_assert(0);
    // }

    /* Select active antennas from available antennas and calculate number of antennas rows and columns */
    MmwDemo_calcActiveAntennaGeometry();

    /* Configure DPUs */

    /* Configure DPUs */
    hwaParamSetStartIdx = 0;
    hwaWindowOffset = 0;

    mmwDemo_computeProfileParams();

    if(gMmwMssMCB.prolongedBurstingMode)
    {
        mmwDemo_ProlongedBurstingConfig();
    }

    if(gMmwMssMCB.timerDrivenDpcMode == 1)
    {
        mmwDemo_TimerDrivenDpcArchConfig();
    }

    mmwDemo_rangeProcConfig(&hwaParamSetStartIdx,
                            &hwaWindowOffset);

    if (gMmwMssMCB.runningMode == DPC_RUNNING_MODE_INDET)
    {
        /* INTRUSION DETECTION MODE */
        if (gOverlappedHwaParamSets)
        {
            hwaParamSetStartIdx = 0;
        }
        mmwDemo_doa3dProcConfig(&hwaParamSetStartIdx,
                                &hwaWindowOffset);

        if (gOverlappedHwaParamSets)
        {
            hwaParamSetStartIdx = 0;
        }
        mmwDemo_snr3dProcConfig(&hwaParamSetStartIdx,
                                &hwaWindowOffset);

        if (!gMmwMssMCB.oneTimeConfigDone)
        {
            //Initialize intrusion detection library only one time
            mmwDemo_intrusionDetConfig();
        }

    }
    else if ((gMmwMssMCB.runningMode == DPC_RUNNING_MODE_SBR) || (gMmwMssMCB.runningMode == DPC_RUNNING_MODE_CPD))
    {
        if (gMmwMssMCB.cliMacroDopplerCfg.macroDopplerFeatureEnabled)
        {
            if (gOverlappedHwaParamSets)
            {
                hwaParamSetStartIdx = 0;
            }
            mmwDemo_macroDoppProcConfig(&hwaParamSetStartIdx,
                                        &hwaWindowOffset);
        }

        mmwDemo_classifierProcConfig();

        /* SBR or CPD DETECTION MODE */
        mmwDemo_dspConfig();
    }
    else
    {
        CLI_write("DPC configuration: Invalid running mode \n");
        DebugP_assert(0);
    }

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

            DebugP_log("%20s %12d %12d %12d\n", "Local2",
                      sizeof(gMmwCoreLocMem2),
                      coreLocalRtosHeap_memUsage(),
                      sizeof(gMmwCoreLocMem2) - coreLocalRtosHeap_memUsage());

        }
    }

}

volatile uint64_t gTest;
//#define PROFILE_INTRUSION_DPUS
#ifdef PROFILE_INTRUSION_DPUS
volatile uint32_t gProfileTime[8][5];
volatile uint32_t gProfileTimeInd = 0;
volatile uint32_t gStartTime;
volatile uint32_t gEndTime;
#endif

volatile cmplx16ImRe_t *gRngBin28VecPtr;
volatile cmplx16ImRe_t gRngBin28Vec[64];
volatile uint32_t gRngBin28VecIdx = 0;

volatile uint32_t gMacroDopplerDpuTime[16];
volatile uint32_t gMacroDopplerDpuTimeIdx = 0;
/**
 *  @b Description
 *  @n  DPC processing chain execute function.
 *
 */
void DPC_Execute(){
    int32_t retVal;
    int32_t errCode = 0;
    DPU_RangeProc_OutParams outParms;
    DPU_Doa3dProc_OutParams outParmsDoa3dproc;
    DPU_SNR3DHM_OutParams   outParmsSnr3dproc;
    DPU_MacroDopplerProc_OutParams   outParamsMacroDoppProc = {0};
    IDETECT_output outIntrusionDet;
    DPC_ObjectDetection_IntrusionDetResult *pIntrusionDetResult;

    
    /* give initial trigger for the first frame */
    errCode = DPU_RangeProc_control(gMmwMssMCB.rangeProcDpuHandle,
                                    DPU_RangeProc_Cmd_triggerProc, NULL, 0);
    if(errCode < 0)
    {
        CLI_write("Error: Range control execution failed [Error code %d]\n", errCode);
    }

    /* Send signal to CLI task that this is ready */
    SemaphoreP_post(&gMmwMssMCB.dpcTaskConfigDoneSemHandle);

    while(true){

        memset((void *)&outParms, 0, sizeof(DPU_RangeProc_OutParams));
        retVal = DPU_RangeProc_process(gMmwMssMCB.rangeProcDpuHandle, &outParms);
        if(retVal != 0){
            CLI_write("DPU_RangeProcHWA_process failed with error code %d", retVal);
            DebugP_assert(0);
        }

        //DEBUG BREATHING -  REMOVE THIS
        {
            uint32_t frmCntr = (gMmwMssMCB.stats.frameStartIntCounter-1) & 0x3;
            gRngBin28VecPtr = (cmplx16ImRe_t *) gMmwMssMCB.radarCube.data;
            gRngBin28Vec[gRngBin28VecIdx] = gRngBin28VecPtr[27*128*16 + frmCntr*32*16];
            gRngBin28VecIdx = (gRngBin28VecIdx + 1) & 63;
        }

        GPIO_pinWriteLow(gGpioBaseAddrLed, gPinNumLed);
#if 1
        /* Read the temperature */
        MMWave_getTemperatureReport(&gTempStats);
#endif
        if (gMmwMssMCB.runningMode == DPC_RUNNING_MODE_INDET)
        {
            if ((gMmwMssMCB.lowPowerMode == LOW_PWR_MODE_ENABLE) || (gMmwMssMCB.lowPowerMode == LOW_PWR_TEST_MODE))
            {
                /* Shutdown the FECSS after chirping in Low Power Mode */
                int32_t err;

                //Change clock back to Fast clock (200MHz)
                SOC_rcmSetR5Clock(SOC_R5FCoreFreq200MHz,SOC_FastClk1Freq200MHz, SOC_RcmR5ClockSource_FAST_CLK1); 

#if 0
                /* To-Do the retention list Retain FECSS Code Memory */
                PRCMSetSRAMRetention((PRCM_FEC_PD_SRAM_CLUSTER_1 | PRCM_FEC_PD_SRAM_CLUSTER_2 | PRCM_FEC_PD_SRAM_CLUSTER_3), PRCM_SRAM_LPDS_RET);
#endif
                /* Reset The FrameTimer for next frame */
                HW_WR_REG32(CSL_APP_RCM_U_BASE + CSL_APP_RCM_BLOCKRESET2, 0x1c0);

                /* To-Do Give a proper delay */
                for(int i =0;i<10;i++)
                {
                    gTest = PRCMSlowClkCtrGet();
                }

                HW_WR_REG32(CSL_APP_RCM_U_BASE + CSL_APP_RCM_BLOCKRESET2, 0x0);

                /* Front End Shutdown in preparation for Low power state */
                MMWave_stop(gMmwMssMCB.ctrlHandle, &gMmwMssMCB.mmWaveCfg.strtCfg, &err);
                MMWave_close(gMmwMssMCB.ctrlHandle,&err);
                MMWave_deinit(gMmwMssMCB.ctrlHandle,&err);
            }
        }

        /* Chirping finished start interframe processing */
        gMmwMssMCB.stats.interFrameStartTimeStamp = Cycleprofiler_getTimeStamp();
#ifdef PROFILE_INTRUSION_DPUS
        gStartTime = gMmwMssMCB.stats.interFrameStartTimeStamp;
#endif
        DPC_ObjectDetection_Profile(&gMmwMssMCB.stats.chirpingCompletion);
        gMmwMssMCB.stats.chirpingTime_us = gMmwMssMCB.stats.chirpingCompletion.timeInUsec;
        
         /* Procedure for range bias measurement and Rx channels gain/phase offset measurement */
        if(gMmwMssMCB.measureRxChannelBiasCliCfg.enabled)
        {
            MmwDemo_rangeBiasRxChPhaseMeasure();
        }

#ifdef PROFILE_INTRUSION_DPUS
        gEndTime = Cycleprofiler_getTimeStamp();
        gProfileTime[gProfileTimeInd][0] = gEndTime - gStartTime;
        gStartTime = gEndTime;
#endif

        if ((gMmwMssMCB.runningMode == DPC_RUNNING_MODE_SBR) || (gMmwMssMCB.runningMode == DPC_RUNNING_MODE_CPD))
        {
            /* Send to DSP message that the radar cube is ready */
            MsgIpc_sendMessage(&gMmwMssMCB.msgIpcCtrlObj, DPC_MSS_TO_DSS_RADAR_CUBE_READY, (uint32_t) &gMmwMssMCB.radarCube);

            if (gMmwMssMCB.cliMacroDopplerCfg.macroDopplerFeatureEnabled)
            {
                uint32_t startTime = Cycleprofiler_getTimeStamp();

                retVal = DPU_MacroDopplerProc_process (gMmwMssMCB.macroDoppProcDpuHandle,
                                                       &outParamsMacroDoppProc);

                /* Check for real-time commands */
                if (gMmwMssMCB.cliMacroDopplerMapScaleCmdPending)
                {
                    uint8_t numZones = gMmwMssMCB.featureExtrModuleCfg.sceneryParams.numOccupancyBoxes;
                    MmwDemo_MacroDopplerMapScaleCfg cfg;
                    for (uint8_t i = 0; i < numZones; i++)
                    {
                        cfg.cnnInputScale[i] = gMmwMssMCB.cliMacroDopplerMapScale.cnnInputScale[i] * gMmwMssMCB.cnnCommonScaleFactor; // Correction term: 140260.1472 = (1.0701*2^17)
                    }
                    retVal = DPU_ClassifierProc_control(gMmwMssMCB.classifierDpuHandle,
                                                        DPU_ClassifierProc_Cmd_MacroDopplerMapScaleCfg,
                                                        &cfg.cnnInputScale[0],
                                                        numZones*sizeof(float));
                    if (retVal != 0)
                    {
                        CLI_write("DPU_ClassifierProcess_Ctrl failed with error code %d", retVal);
                    }
                    gMmwMssMCB.cliMacroDopplerMapScaleCmdPending = 0;
                }

                if (gMmwMssMCB.cli_zOffsetCmdPending)
                {
                    uint8_t numZones = gMmwMssMCB.featureExtrModuleCfg.sceneryParams.numOccupancyBoxes;
                    retVal = DPU_ClassifierProc_control(gMmwMssMCB.classifierDpuHandle,
                                                        DPU_ClassifierProc_Cmd_zOffsetsCfg,
                                                        &gMmwMssMCB.featureExtrModuleCfg.zOffset,
                                                        numZones*sizeof(float));
                     if (retVal != 0)
                     {
                         CLI_write("DPU_ClassifierProcess_Ctrl failed with error code %d", retVal);
                     }
                     gMmwMssMCB.cli_zOffsetCmdPending = 0;
                }


                if ((gMmwMssMCB.runningMode == DPC_RUNNING_MODE_CPD) && gMmwMssMCB.cpdOption == DPU_CLASSIFIERPROC_CPD_MODE_LPD_USING_CNN)
                {
                    /* Send signal to classifier task that macro-doppler is ready */
                    SemaphoreP_post(&gMmwMssMCB.classifierTaskSem2Handle);
                }
                gMacroDopplerDpuTime[gMacroDopplerDpuTimeIdx] = Cycleprofiler_getTimeStamp() - startTime;
                gMacroDopplerDpuTimeIdx = (gMacroDopplerDpuTimeIdx + 1) & 15;
            }

            if (retVal != 0)
            {
                CLI_write("DPU_Doa3dProc_process failed with error code %d", retVal);
                DebugP_assert(0);
            }

        }
        else
        {
            /****************************/
            /* Intrusion Detection Mode */
            /****************************/
            /* Compute 3D-heatmap */
            retVal = DPU_Doa3dProc_process(gMmwMssMCB.doa3dProcDpuHandle,
                                           &outParmsDoa3dproc);
            if (retVal != 0)
            {
                CLI_write("DPU_Doa3dProc_process failed with error code %d", retVal);
                DebugP_assert(0);
            }

#ifdef PROFILE_INTRUSION_DPUS
            gEndTime = Cycleprofiler_getTimeStamp();
            gProfileTime[gProfileTimeInd][1] = gEndTime - gStartTime;
            gStartTime = gEndTime;
#endif

            {
            /***********************************************************************************************/
            /* TODO: Known issue - MMWSOC_IWRL68XX-1900. Dynamic clock gating disabled for HWA CFAR engine */
            DSSHWACCRegs *ctrlBaseAddr = (DSSHWACCRegs *)gHwaObjectPtr[0]->hwAttrs->ctrlBaseAddr;
            CSL_FINSR(ctrlBaseAddr->HWACCREG1,
                        HWACCREG1_ACCDYNCLKEN_BIT_END,
                        HWACCREG1_ACCDYNCLKEN_BIT_START,
                        0x0U);
            /***********************************************************************************************/
            }
            /* Compute 3D-SNR */
            retVal = DPU_SNR3DHM_process(gMmwMssMCB.snr3dProcDpuHandle,
                                             &outParmsSnr3dproc);
            if (retVal != 0)
            {
                CLI_write("DPU_SNR3DHM_process failed with error code %d", retVal);
                DebugP_assert(0);
            }
            {
            /***********************************************************************************************/
            /* TODO: Known issue - MMWSOC_IWRL68XX-1900. Dynamic clock gating disabled for HWA CFAR engine */
            DSSHWACCRegs *ctrlBaseAddr = (DSSHWACCRegs *)gHwaObjectPtr[0]->hwAttrs->ctrlBaseAddr;
            CSL_FINSR(ctrlBaseAddr->HWACCREG1,
                        HWACCREG1_ACCDYNCLKEN_BIT_END,
                        HWACCREG1_ACCDYNCLKEN_BIT_START,
                        0x1U);
            /***********************************************************************************************/
            }

#ifdef PROFILE_INTRUSION_DPUS
            gEndTime = Cycleprofiler_getTimeStamp();
            gProfileTime[gProfileTimeInd][2] = gEndTime - gStartTime;
            gStartTime = gEndTime;
#endif

            {/* save range profile to send to UART output. Range profile is in bore site direction (azimuth and elevation angle equal to zero) */
                uint32_t rngInd, ind;
                uint32_t nRngBins = gMmwMssMCB.numRangeBins;
                uint32_t nAzBins = gMmwMssMCB.intrusionSigProcChainCfg.azimuthFftSize;
                uint32_t nAzBinsHalf = nAzBins >> 1;
                uint32_t nElBinsHalf = gMmwMssMCB.intrusionSigProcChainCfg.elevationFftSize >> 1;
                uint32_t nOffset = nElBinsHalf*nRngBins*nAzBins + nAzBinsHalf;

                if (gMmwMssMCB.rangeProfile != NULL)
                {
                    if (gMmwMssMCB.runningMode == DPC_RUNNING_MODE_INDET)
                    {
                        for (rngInd = 0; rngInd < nRngBins; rngInd++)
                        {
                            cmplx16ImRe_t * radCube = (cmplx16ImRe_t *) gMmwMssMCB.radarCube.data;
                            gMmwMssMCB.rangeProfile[rngInd] = (uint32_t) sqrtf((float) radCube[rngInd].real * (float) radCube[rngInd].real +
                                                                               (float) radCube[rngInd].imag * (float) radCube[rngInd].imag);
                            //gMmwMssMCB.rangeProfile[rngInd] = detMatrix[rngInd*nAzBins + nOffset];
                        }
                    }
                    else
                    {
                        nOffset = gMmwMssMCB.numRxAntennas * gMmwMssMCB.numTxAntennas * gMmwMssMCB.numDopplerChirps;
                        rngInd = 0;
                        for (ind = 0; ind < nRngBins; ind++)
                        {
                            cmplx16ImRe_t * radCube = (cmplx16ImRe_t *) gMmwMssMCB.radarCube.data;
                            gMmwMssMCB.rangeProfile[ind] = (uint32_t) sqrtf((float) radCube[rngInd].real * (float) radCube[rngInd].real +
                                                                            (float) radCube[rngInd].imag * (float) radCube[rngInd].imag);
                            rngInd += nOffset;
                            //gMmwMssMCB.rangeProfile[rngInd] = detMatrix[rngInd*nAzBins + nOffset];
                        }
                    }
                }
            }

#ifdef PROFILE_INTRUSION_DPUS
            gEndTime = Cycleprofiler_getTimeStamp();
            gProfileTime[gProfileTimeInd][3] = gEndTime - gStartTime;
            gStartTime = gEndTime;
#endif

            /* Intrusion detection */
            inDetect_compute(gMmwMssMCB.inDetectHandle, gMmwMssMCB.snrOutMatrix.data, &outIntrusionDet);
            {
                uint32_t numOccBoxes = gMmwMssMCB.idetSceneryParams.numOccupancyBoxes;
                pIntrusionDetResult = &gMmwMssMCB.intrusionDetInfoToUart.intrusionDetResult;

                pIntrusionDetResult->numberOfZones = numOccBoxes;
                memcpy(&pIntrusionDetResult->data[0], outIntrusionDet.occBoxSignal, sizeof(float)*numOccBoxes);
                memcpy(&pIntrusionDetResult->data[sizeof(float)*numOccBoxes], outIntrusionDet.occBoxDecision, sizeof(uint8_t)*numOccBoxes);

                uint32_t dataSizeInBytes = sizeof(uint32_t) + numOccBoxes * (sizeof(float) + sizeof(uint8_t));
                gMmwMssMCB.intrusionDetInfoToUart.messageTL.type = MMWDEMO_OUTPUT_MSG_INTRUSION_DET_INFO;
                gMmwMssMCB.intrusionDetInfoToUart.messageTL.length = dataSizeInBytes;

            }
            /* Interframe Intrusion detection processing finished */
            DPC_ObjectDetection_Profile(&gMmwMssMCB.stats.intrusionDetCompletion);
            gMmwMssMCB.outStats.interFrameProcessingTime = gMmwMssMCB.stats.intrusionDetCompletion.timeInUsec - gMmwMssMCB.stats.chirpingCompletion.timeInUsec;
            gMmwMssMCB.outStats.dspProcessingTime = 0;

#ifdef PROFILE_INTRUSION_DPUS
            gProfileTime[gProfileTimeInd][4] = gEndTime - gStartTime;
            gStartTime = gEndTime;
            gProfileTimeInd = (gProfileTimeInd + 1) & 0x7;
#endif

        }
        
        /* Give initial trigger for the next frame */
        retVal = DPU_RangeProc_control(gMmwMssMCB.rangeProcDpuHandle,
                                       DPU_RangeProc_Cmd_triggerProc, NULL, 0);
        if(retVal < 0)
        {
            CLI_write("Error: DPU_RangeProcHWA_control failed with error code %d", retVal);
            DebugP_assert(0);
        }


        /* Interframe processing finished */
        //gMmwMssMCB.stats.ProcessingEndTimeStampUs = ClockP_getTimeUsec();
        //gMmwMssMCB.outStats.interFrameProcessingTimeUs = (gMmwMssMCB.stats.ProcessingEndTimeStampUs - gMmwMssMCB.stats.ProcessingStartTimeStampUs);

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


        if (gMmwMssMCB.runningMode == DPC_RUNNING_MODE_INDET)
        {
            /* Trigger UART task to send TLVs to host */
            SemaphoreP_post(&gMmwMssMCB.tlvSemHandle);
        }
    }
}

/**
*  @b Description
*  @n
*        Function configuring and executing DPC
*/
void MmwDemo_dpcTask()
{
    /* Save/restore FP registers during the context switching */
    vPortTaskUsesFPU();
    
    DPC_Config();

    DPC_Execute();

    /* Never return for this task. */
    SemaphoreP_pend(&gMmwMssMCB.TestSemHandle, SystemP_WAIT_FOREVER);
}



void DPC_mss_MsgHandler(uint32_t remoteCoreId, uint16_t localClientId, uint64_t msgValue, int32_t crcStatus, void *arg)
{
    uint32_t message;
    uint32_t messageArg;


    message     = (uint32_t) ((msgValue >> 32) & 0xffff);
    messageArg  = (uint32_t) (msgValue  & 0xffffffff);

    switch (message)
    {
        case DPC_DSS_TO_MSS_CONFIGURATION_COMPLETED:
            /* Send signal to CLI task that this is ready */
            SemaphoreP_post(&gMmwMssMCB.dspCfgDoneSemaphore);
            break;

        case DPC_DSS_TO_MSS_POINT_CLOUD_READY:

            /* Get the pointer to point cloud result from DSP */
            gMmwMssMCB.outputFromDSP = (DPIF_MSS_DSS_radarProcessOutput  *) messageArg;
            /* Send signal to classifier task this is ready */
            SemaphoreP_post(&gMmwMssMCB.classifierTaskSemHandle);
            break;
    }
}


