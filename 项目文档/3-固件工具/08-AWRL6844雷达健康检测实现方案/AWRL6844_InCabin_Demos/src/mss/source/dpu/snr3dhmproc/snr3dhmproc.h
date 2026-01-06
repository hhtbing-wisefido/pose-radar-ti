/**
 *   @file  snr3dhmprochwa.h
 *
 *   @brief
 *      Implements snr3dhm - snr 3D-heatmap generation DPU.
 *
 *  \par
 *  NOTE:
 *      (C) Copyright 2018-2024 Texas Instruments, Inc.
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

/**************************************************************************
 *************************** Include Files ********************************
 **************************************************************************/
#ifndef SNR3DHM_PROC_H
#define SNR3DHM_PROC_H

/* Standard Include Files. */
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* mmWave SDK Driver/Common Include Files */
#include <drivers/hwa.h>
#include <drivers/edma.h>


/* Datapath files */
#include <datapath/dpif/dpif_detmatrix.h>
#include <datapath/dpif/dpif_pointcloud.h>
#include <datapath/dpedma/v1/dpedma.h>
#include <datapath/dpif/dp_error.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup DPU_SNR3DHMPROC_EXTERNAL_DEFINITIONS
 *
 @{ */

/*! @brief Alignment for memory allocation purpose of detection matrix.
 *         There is CPU access of detection matrix in the implementation.
 */
#define DPU_SNR3DHM_DET_MATRIX_BYTE_ALIGNMENT (sizeof(uint32_t))

/*! @brief Alignment for memory allocation purpose. There is CPU access of this buffer
 *         in the implementation.
 */
#define DPU_SNR3DHM_CFAR_DET_LIST_BYTE_ALIGNMENT    (sizeof(uint32_t))

/*! @brief Alignment for memory allocation purpose. There is CPU access of this buffer
 *         in the implementation. This is the maximum field size of the
 *         @ref DPU_SNR3DHM_CfarDetOutput structure.
 */
#define DPU_SNR3DHM_HWA_MEM_OUT_DOPPLER_BYTE_ALIGNMENT    (sizeof(uint32_t))

/*! @brief Alignment for memory allocation purpose. There is CPU access of thi buffers
 *         in the implementation. This is the maximum field size of the
 *         @ref DPU_SNR3DHM_CfarDetOutput structure.
 */
#define DPU_SNR3DHM_HWA_MEM_OUT_RANGE_BYTE_ALIGNMENT    (sizeof(uint32_t))

/*! @brief Alignment for memory allocation purpose. There is CPU access of this buffer
 *         in the implementation.
 */
#define DPU_SNR3DHM_DOPPLER_DET_OUT_BIT_MASK_BYTE_ALIGNMENT    (sizeof(uint32_t))

/**
 * @brief   Number of HWA memory banks needed
 */
#define DPU_SNR3DHM_NUM_HWA_MEMBANKS  4

/**
@}
*/

/** @addtogroup DPU_SNR3DHMPROC_ERROR_CODE
 *  Base error code for the snr3dProc DPU is defined in the
 *  \include ti/datapath/dpif/dp_error.h
 @{ */


#define DP_ERRNO_SNR3D_PROC_BASE            (MMWAVE_ERRNO_DPU_BASE -1500)

/**
 * @brief   Error Code: Invalid argument
 */
#define DPU_SNR3DHM_EINVAL                  (DP_ERRNO_SNR3D_PROC_BASE-1)

/**
 * @brief   Error Code: Invalid detection matrix format argument
 */
#define DPU_SNR3DHM_EINVAL__DET_MATRIX_FORMAT (DP_ERRNO_SNR3D_PROC_BASE-2)

/**
  * @brief   Error Code: Invalid number of param sets
  */
#define DPU_SNR3DHM_EINVAL__NUM_PARAM_SETS    (DP_ERRNO_SNR3D_PROC_BASE-3)

/**
 * @brief   Error Code: Out of memory when allocating using MemoryP_osal
 */
#define DPU_SNR3DHM_ENOMEM                  (DP_ERRNO_SNR3D_PROC_BASE-10)

/**
 * @brief   Error Code: HWA input memory for detection matrix is not sufficient.
 */
#define DPU_SNR3DHM_ENOMEM__DET_MATRIX_EXCEEDS_HWA_INP_MEM  (DP_ERRNO_SNR3D_PROC_BASE-11)

 /**
  * @brief   Error Code: Memory not aligned for detection matrix (detMatrix.data)
  */
#define DPU_SNR3DHM_ENOMEMALIGN_DET_MATRIX                  (DP_ERRNO_SNR3D_PROC_BASE-12)

/**
 * @brief   Error Code: Memory not aligned for @ref DPU_SNR3DHM_HW_Resources::hwaMemOutDoppler
 */
#define DPU_SNR3DHM_ENOMEMALIGN_HWA_MEM_OUT_DOPPLER         (DP_ERRNO_SNR3D_PROC_BASE-14)

/**
 * @brief   Error Code: Memory not aligned for @ref DPU_SNR3DHM_HW_Resources::hwaMemOutDetList
 */
#define DPU_SNR3DHM_ENOMEMALIGN_HWA_MEM_OUT_RANGE           (DP_ERRNO_SNR3D_PROC_BASE-15)

/**
 * @brief   Error Code: Insufficient memory allocated to @ref DPU_SNR3DHM_HW_Resources::cfarDopplerDetOutBitMask.
 */
#define DPU_SNR3DHM_ENOMEM__INSUFFICIENT_DOP_DET_OUT_BIT_MASK  (DP_ERRNO_SNR3D_PROC_BASE-16)

/**
 * @brief   Error Code: Memory not aligned for @ref DPU_SNR3DHM_HW_Resources::cfarDopplerDetOutBitMask
 */
#define DPU_SNR3DHM_ENOMEMALIGN_DOPPLER_DET_OUT_BIT_MASK   (DP_ERRNO_SNR3D_PROC_BASE-17)

/**
 * @brief   Error Code: Memory
 */
#define DPU_SNR3DHM_NUM_RANGE_BINS_EXCEDED_LIMIT           (DP_ERRNO_SNR3D_PROC_BASE-18)

/**
 * @brief   Error Code: Semaphore
 */
#define DPU_SNR3DHM_ESEMA                  (DP_ERRNO_SNR3D_PROC_BASE-19)

/**
 * @brief   Error Code: Internal error
 */
#define DPU_SNR3DHM_EINTERNAL               (DP_ERRNO_SNR3D_PROC_BASE-20)

/**
 * @brief   Error Code: Insufficient memory to save HWA param sets
 */
#define DPU_SNR3DHM_EHWA_PARAM_SAVE_LOC_SIZE (DP_ERRNO_SNR3D_PROC_BASE-21)

/**
 * @brief   Error Code: Not implemented
 */
#define DPU_SNR3DHM_ENOTIMPL                (DP_ERRNO_SNR3D_PROC_BASE-30)

 /**
 @}
 */

/**
 * @brief   Number HWA params used
 */
#define DPU_SNR3DHM_SHIFT_Q8  8
#define DPU_SNR3DHM_ONE_Q8    (1<<DPU_SNR3DHM_SHIFT_Q8)

/**
 * @brief   Number HWA params used
 */
#define DPU_SNR3DHM_MAX_NUM_HWA_PARAMSET  16



/*! @brief   CFAR detection in range domain */
#define DPU_CFAR_RANGE_DOMAIN   0

/*! * @brief   CFAR detection in Doppler domain */
#define DPU_CFAR_DOPPLER_DOMAIN 1

/*! @brief Peak grouping scheme of CFAR detected objects based on peaks of neighboring cells taken from detection matrix */
#define DPU_CFAR_PEAK_GROUPING_DET_MATRIX_BASED 1

/*! @brief Peak grouping scheme of CFAR detected objects based only on peaks of neighboring cells that are already detected by CFAR */
#define DPU_CFAR_PEAK_GROUPING_CFAR_PEAK_BASED  2

/*! @brief  Convert peak/noise value to log10 value in 0.1dB
       Since, val = log2(|.|)* 2^Qformat = log10(|.|) / log10(2) * 2^Qformat
       Equation: output = 1/0.1 * 10log10(|.|^2) = 10 * [ val * 20log10(2) / 2^Qformat ] = val * 6.0 / 2^Qformat * 10
 */
#define CFARCADSP_CONV_PEAK_TO_LOG(val, QFormat)        (val * 6.0 /(float)(1<<QFormat) * 10.0)

/**
 * @brief
 *  CFAR Configuration
 *
 * @details
 *  The structure contains the cfar configuration used in data path
 */
typedef struct DPU_SNR3DProc_CfarCfg_t
{
    /*! @brief    CFAR threshold scale */
    uint32_t       thresholdScale;

    /*! @brief    CFAR averagining mode 0-CFAR_CA, 1-CFAR_CAGO, 2-CFAR_CASO */
    uint8_t        averageMode;

    /*! @brief    CFAR noise averaging one sided window length */
    uint8_t        winLen;

    /*! @brief    CFAR one sided guard length*/
    uint8_t        guardLen;

    /*! @brief    CFAR cumulative noise sum divisor
                  CFAR_CA:
                        noiseDivShift should account for both left and right noise window
                        ex: noiseDivShift = ceil(log2(2 * winLen))
                  CFAR_CAGO/_CASO:
                        noiseDivShift should account for only one sided noise window
                        ex: noiseDivShift = ceil(log2(winLen))
     */
    uint8_t        noiseDivShift;

    /*! @brief    CFAR 0-cyclic mode disabled, 1-cyclic mode enabled */
    uint8_t        cyclicMode;

    /*! @brief    Peak grouping scheme 1-based on neighboring peaks from detection matrix
     *                                 2-based on on neighboring CFAR detected peaks.
     *            Scheme 2 is not supported on the HWA version (cfarcaprochwa.h) */
    uint8_t        peakGroupingScheme;

    /*! @brief     Peak grouping, 0- disabled, 1-enabled */
    uint8_t        peakGroupingEn;


    int16_t sideLobeThresholdScaleQ8; //In Q8 format
    bool enableLocalMaxRange;
    bool enableLocalMaxAzimuth;
    bool enableInterpRangeDom;
    bool enableInterpAzimuthDom;

} DPU_SNR3DProc_CfarCfg;

/**
 * @brief
 *  CFAR Configuration
 *
 * @details
 *  The structure contains the cfar configuration used in data path
 */
typedef struct DPU_SNR3DProc_CfarScndPassCfg_t
{
    /*! @brief    CFAR threshold scale */
    uint32_t       thresholdScale;

    /*! @brief    CFAR threshold in dB*/
    float       threshold_dB;

    /*! @brief    CFAR averagining mode 0-CFAR_CA, 1-CFAR_CAGO, 2-CFAR_CASO */
    uint8_t        averageMode;

    /*! @brief    CFAR noise averaging one sided window length */
    uint8_t        winLen;

    /*! @brief    CFAR one sided guard length*/
    uint8_t        guardLen;

    /*! @brief    CFAR cumulative noise sum divisor
                  CFAR_CA:
                        noiseDivShift should account for both left and right noise window
                        ex: noiseDivShift = ceil(log2(2 * winLen))
                  CFAR_CAGO/_CASO:
                        noiseDivShift should account for only one sided noise window
                        ex: noiseDivShift = ceil(log2(winLen))
     */
    uint8_t        noiseDivShift;

    /*! @brief    CFAR 0-cyclic mode disabled, 1-cyclic mode enabled */
    uint8_t        cyclicMode;

    /*! @brief     Peak grouping, 0- disabled, 1-enabled */
    uint8_t        peakGroupingEn;

    /*! @brief     Second pass CFAR Enabled flag, 0- disabled, 1-enabled */
    uint8_t        enabled;
} DPU_SNR3DProc_CfarScndPassCfg;

/**
 * @brief
 *  Data processing Unit statistics
 *
 * @details
 *  The structure is used to hold the statistics of the DPU 
 *
 *  \ingroup INTERNAL_DATA_STRUCTURE
 */
typedef struct DPU_SNR3DHMProc_Stats_t
{
    /*! @brief total number of calls of DPU processing */
    uint32_t            numProcess;

    /*! @brief total processing time during all chirps in a frame excluding EDMA waiting time*/
    uint32_t            processingTime;

    /*! @brief total wait time for EDMA data transfer during all chirps in a frame*/
    uint32_t            waitTime;
}DPU_SNR3DHMProc_Stats;



 /**
  * @brief
  *  snr3dProc control command
  *
  * @details
  *  The enum defines the snr3dProc supported run time command
  *
  *  \ingroup DPU_SNR3DPROC_EXTERNAL_DATA_STRUCTURE
  */
 typedef enum DPU_SNR3DHM_Cmd_e
 {

     /*! @brief Command to update CFAR configuration in range domain */
     DPU_SNR3DHM_Cmd_CfarCfg,

     /*! @brief Command to update field of view in range domain, minimum and maximum range limits */
     DPU_SNR3DHM_Cmd_FovRangeCfg,

     /*! @brief Command to update field of view in angle domain, minimum and maximum angle limits (for azimuth and elevation) */
     DPU_SNR3DHM_Cmd_FovAoaCfg
 }DPU_SNR3DHM_Cmd;


 /**
  * @brief
  *  Point cloud definition in Cartesian coordinate system
  */
 typedef struct DPU_SNR3DHM_PointCloudCartesianExt_t
 {
     /*! @brief  x - coordinate in meters. This axis is parallel to the sensor plane
      *          and makes the azimuth plane with y-axis. Positive x-direction is rightward
      *          in the azimuth plane when observed from the sensor towards the scene
      *          and negative is the opposite direction. */
     float  x;

     /*! @brief  y - coordinate in meters. This axis is perpendicular to the
      *          sensor plane with positive direction from the sensor towards the scene */
     float  y;

     /*! @brief  z - coordinate in meters. This axis is parallel to the sensor plane
      *          and makes the elevation plane with the y-axis. Positive z direction
      *          is above the sensor and negative below the sensor */
     float  z;

     /*! @brief  Doppler velocity estimate in m/s. Positive velocity means target
      *          is moving away from the sensor and negative velocity means target
      *          is moving towards the sensor. */
     float  velocity;

     /*! @brief  snr - CFAR cell to side noise ratio in dB expressed in 0.1 steps of dB */
     int16_t  snr;

     /*! @brief  type 0-major motion, 1-minor motion */
     uint16_t type;
 }DPU_SNR3DHM_PointCloudCartesianExt;

 /**
  * @brief
  *  Structure for the HWA Params save location
  *
  *
  *  \ingroup DPU_SNR3DPROC_EXTERNAL_DATA_STRUCTURE
  */
 typedef struct DPU_SNR3DHM_HwaParamSaveLoc_t
 {
     /*! @brief  Pointer to save location for HWA PARAM sets */
     void        *data;

     /*! @brief  Size of the save location for HWA PARAM sets in Bytes*/
     uint32_t    sizeBytes;
 } DPU_SNR3DHM_HwaParamSaveLoc;

/**
 * @brief
 *  CFAR HWA configuration
 *
 * @details
 *  The structure is used to hold the HWA configuration needed for CFAR
 *
 *  \ingroup DPU_SNR3DPROC_EXTERNAL_DATA_STRUCTURE
 */
typedef struct DPU_SNR3DHM_HwaCfarConfig_t
{
    /*! @brief     HWA paramset Start index */
    uint8_t    paramSetStartIdx;

    /*! @brief     number of HWA paramset */
    uint8_t    numParamSet;

    /*! @brief HWA Params save location
    */
    DPU_SNR3DHM_HwaParamSaveLoc hwaParamsSaveLoc;

    uint8_t     dmaTriggerSource[2];
}DPU_SNR3DHM_HwaCfarConfig;

/*!
 *  @brief    Detected object parameters filled by HWA CFAR
 *
 *  \ingroup DPU_SNR3DPROC_EXTERNAL_DATA_STRUCTURE
 *
 */
typedef volatile struct DPU_SNR3DHM_CfarDetOutput_t
{
    uint32_t   noise;           /*!< @brief Noise energy in CFAR cell */
    uint32_t   cellIdx  : 12;   /*!< @brief Sample index (i.e. cell under test index) */
    uint32_t   iterNum  : 12;   /*!< @brief Iteration number (i.e. REG_BCNT counter value) */
    uint32_t   reserved :  8;   /*!< @brief Reserved */
} DPU_SNR3DHM_CfarDetOutput;

/*!
 *  @brief    Detected object parameters filled by HWA CFAR
 *
 *  \ingroup DPU_SNR3DPROC_EXTERNAL_DATA_STRUCTURE
 *
 */
typedef volatile struct DPU_SNR3DHM_CfarRawOutput_t
{
    uint32_t   signal;           /*!< @brief signal (CUT) */
    uint32_t   noise;           /*!< @brief Noise  */
} DPU_SNR3DHM_CfarRawOutput;


/*!
 *  @brief    Maximum peaks filled by HWA statistics block
 *
 *  \ingroup DPU_SNR3DPROC_EXTERNAL_DATA_STRUCTURE
 *
 */
typedef volatile struct DPU_SNR3DHM_HwaMaxOutput_t
{
    uint32_t   maxInd;      /*!< @brief Maximum peak index position */
    uint32_t   peak;        /*!< @brief Maximum peak value */
}  DPU_SNR3DHM_HwaMaxOutput;

/**
 * @brief
 *  SNR3DHM DPU initial configuration parameters
 *
 * @details
 *  The structure is used to hold the DPU initial configurations.
 *
 *  \ingroup DPU_SNR3DPROC_EXTERNAL_DATA_STRUCTURE
 *
 */
typedef struct DPU_SNR3DHM_InitParams_t
{
    /*! @brief HWA Handle */
    HWA_Handle  hwaHandle;
}DPU_SNR3DHM_InitParams;

/**
 * @brief
 *  CFAR Hardware resources
 *
 * @details
 *  CFAR Hardware resources
 *
 *  \ingroup DPU_SNR3DPROC_EXTERNAL_DATA_STRUCTURE
 *
 *
 */
typedef struct DPU_SNR3DHM_Resources_t
{
    /*! @brief     EDMA Handle */
    EDMA_Handle         edmaHandle;

    /*! @brief     EDMA configuration for CFAR data In */
    DPEDMA_ChanCfg       edmaHwaIn[2];

    /*! @brief     EDMA configuration for CFAR data Out */
    DPEDMA_ChanCfg       edmaHwaOut[2];

    /*! @brief     EDMA configuration for EDMA In to  trigger HWA*/
    DPEDMA_ChanCfg       edmaHwaInSignature[2];

    /*! @brief     HWA Configuration */
    DPU_SNR3DHM_HwaCfarConfig   hwaCfg;

    /*! @brief Pointer to range/Doppler log2 magnitude detection matrix. The data buffer
     *         must be aligned to @ref DPU_SNR3DHM_DET_MATRIX_BYTE_ALIGNMENT */
    DPIF_DetMatrix      detMatrix;

    /*! @brief HWA scratch memory to page-in detection matrix. Note two contiguous M
     *         memory banks (of the 4 banks) could be allocated to this. */
    uint16_t  *hwaMemInp;

    /*! @brief Number of elements of type uint16_t of HWA memory to hold detection matrix
     *         (associated with @ref hwaMemInp) */
    uint32_t   hwaMemInpSize;

    /*! @brief HWA scratch memory for producing intermediate cfar detection list in Range domain,
     *         cannot be overlaid with other HWA scratch memory inputs for this DPU.
     *         Must be different memory bank than bank(s) of @ref hwaMemInp.
     *         Must be aligned to @ref DPU_SNR3DHM_HWA_MEM_OUT_RANGE_BYTE_ALIGNMENT.
     *         Note this need not be the start of a HWA memory bank but typically it is,
     *         and is therefore naturally aligned to this alignment requirement */
    DPU_SNR3DHM_CfarDetOutput *hwaMemOutDetList;

    /*! @brief Maximum number of elements of type @ref DPU_SNR3DHM_CfarDetOutput of
     *         HWA memory for cfar detection list in Range domain */
    uint32_t hwaMemOutDetListSize;


    /*! @brief      Detected objects output list sized to @ref detObjOutMaxSize elements,
     *              must be aligned to @ref DPU_AOAPROCHWA_POINT_CLOUD_CARTESIAN_BYTE_ALIGNMENT  */
    DPU_SNR3DHM_PointCloudCartesianExt *detObjOut;

    /*! @brief      HWA memory to store range profile */
    DPU_SNR3DHM_HwaMaxOutput *hwaMemOutRangeProfile;

    DPIF_DetMatrix  snrOutMatrix;

} DPU_SNR3DHM_HW_Resources;

/**
 * @brief
 *  HWA CFAR dynamic configuration
 *
 * @details
 *  The structure is used to hold the dynamic configuration used for CFAR
 *
 *  \ingroup DPU_SNR3DPROC_EXTERNAL_DATA_STRUCTURE
 *
 *
 */
typedef struct DPU_SNR3DHM_DynamicConfig_t
{
    DPU_SNR3DProc_CfarCfg   *cfarCfg;
    DPU_SNR3DProc_CfarScndPassCfg *cfarScndPassCfg;

} DPU_SNR3DHM_DynamicConfig;

/**
 * @brief
 *  HWA CFAR static configuration
 *
 * @details
 *  The structure is used to hold the static configuration used for CFAR.
 * The following condition must be satisfied:
 *
 *    @verbatim
      numRangeBins * numDopplerBins * sizeof(uint16_t) <= 32 KB (two HWA memory banks)
      @endverbatim
 *
 *  \ingroup DPU_SNR3DPROC_EXTERNAL_DATA_STRUCTURE
 *
 */
typedef struct DPU_SNR3DHM_StaticConfig_t
{
    /*! @brief  Number of range bins */
    uint16_t    numRangeBins;

    /*! @brief  Number of doppler bins */
    uint16_t    numDopplerBins;

    /*! @brief  Azimuth FFT  size */
     uint16_t    azimuthFftSize;

     /*! @brief  Elevation FFT  size */
     uint16_t    elevationFftSize;

    /*! @brief      */
    bool        secondPassCfar;

    /*! @brief  Scale down before log2 operation, right shift value*/
    uint8_t     divShiftBeforeLog;

    /*! @brief     Load HWA params sets before execution */
    bool loadHwaParamSetsBeforeExec;

} DPU_SNR3DHM_StaticConfig;

/**
 * @brief
 *  HWA CFAR configuration
 *
 * @details
 *  The structure is used to hold the HWA configuration used for CFAR
 *
 *  \ingroup DPU_SNR3DPROC_EXTERNAL_DATA_STRUCTURE
 *
 *
 */
typedef struct DPU_SNR3DHM_Config_t
{
    /*! @brief  Hardware resources */
    DPU_SNR3DHM_HW_Resources res;

    /*! @brief  Dynamic configuration */
    DPU_SNR3DHM_DynamicConfig dynCfg;

    /*! @brief  Static configuration */
    DPU_SNR3DHM_StaticConfig staticCfg;
}DPU_SNR3DHM_Config;

/**
 * @brief
 *  Output parameters populated during Processing time
 *
 * @details
 *  The structure is used to hold the output parameters
 *
 *
 *  \ingroup DPU_SNR3DPROC_EXTERNAL_DATA_STRUCTURE
 *
 */
typedef struct DPU_SNR3DHM_OutParams_t
{
    /*! @brief     SNR3DHMProc statistics */
    DPU_SNR3DHMProc_Stats stats;

    /*! @brief      point-cloud output list */
    DPU_SNR3DHM_PointCloudCartesianExt     *detObjOut;

    /*! @brief      point-cloud output list size */
    uint32_t                      detObjOutMaxSize;

    /*! @brief      range profile */
    uint32_t *rangeProfile;

}DPU_SNR3DHM_OutParams;

/**
 * @brief
 *  CFARHwa DPU Handle
 *
 *
 *  \ingroup DPU_SNR3DPROC_EXTERNAL_DATA_STRUCTURE
 *
 */
typedef void* DPU_SNR3DHM_Handle;

/**
 *  @b Description
 *  @n
 *      The function is SNR3DHM DPU initialization function. It allocates memory to store
 *  its internal data object and returns a handle if it executes successfully.
 *
 *  @param[in]  initCfg                 Pointer to initialization configuration
 *
 *  @param[in]  errCode                 Pointer to errCode generates from the API
 *
 *  \ingroup    DPU_SNR3DHM_EXTERNAL_FUNCTION
 *
 *  @retval
 *      Success     - valid SNR3DHM handle
 *  @retval
 *      Error       - NULL
 */
DPU_SNR3DHM_Handle DPU_SNR3DHM_init
(
    DPU_SNR3DHM_InitParams *initCfg,
    int32_t*            errCode
);

/**
 *  @b Description
 *  @n
 *      The function is SNR3DHM DPU configuration function. It saves buffer pointer and configurations
 *  including system resources and configures EDMA for runtime range processing.
 *
 *  @pre    DPU_SNR3DHM_init() has been called
 *
 *  @param[in]  handle                  SNR3DHM DPU handle
 *  @param[in]  snr3dCfg              Pointer to SNR3DHM configuration data structure
 *
 *  \ingroup    DPU_SNR3DHM_EXTERNAL_FUNCTION
 *
 *  @retval
 *      Success     - 0
 *  @retval
 *      Error       - <0
 */
int32_t DPU_SNR3DHM_config
(
    DPU_SNR3DHM_Handle      handle,
    DPU_SNR3DHM_Config      *snr3dCfg
);

/**
 *  @b Description
 *  @n
 *      The function is SNR3DHM DPU process function. It performs CFAR detection using HWA
 *
 *  @pre    DPU_SNR3DHM_init() has been called
 *
 *  @param[in]  handle                  SNR3DHM DPU handle
 *  @param[in]  outParams               DPU output parameters
 *
 *  \ingroup    DPU_SNR3DHM_EXTERNAL_FUNCTION
 *
 *  @retval
 *      Success = 0
 *  @retval
 *      Error  != 0
 */
int32_t DPU_SNR3DHM_process
(
    DPU_SNR3DHM_Handle   handle,
    DPU_SNR3DHM_OutParams  *outParams
);

/**
 *  @b Description
 *  @n
 *      The function is SNR3DHM DPU control function.
 *
 *  @pre     DPU_SNR3DHM_init() has been called
 *
 *  @param[in]  handle           SNR3DHM DPU handle
 *  @param[in]  cmd              SNR3DHM DPU control command
 *  @param[in]  arg              SNR3DHM DPU control argument pointer
 *  @param[in]  argSize          SNR3DHM DPU control argument size
 *
 *  \ingroup    DPU_SNR3DHM_EXTERNAL_FUNCTION
 *
 *  @retval
 *      Success     - 0
 *  @retval
 *      Error       - <0
 */
int32_t DPU_SNR3DHM_control
(
    DPU_SNR3DHM_Handle handle,
    DPU_SNR3DHM_Cmd cmd,
    void *arg,
    uint32_t argSize
);

/**
 *  @b Description
 *  @n
 *      The function is SNR3DHM DPU deinitialization function. It frees up the
 *   resources allocated during initialization.
 *
 *  @pre    DPU_SNR3DHM_init() has been called
 *
 *  @param[in]  handle           SNR3DHM DPU handle
 *
 *  \ingroup    DPU_SNR3DHM_EXTERNAL_FUNCTION
 *
 *  @retval
 *      Success     - 0
 *  @retval
 *      Error       - <0
 */
int32_t DPU_SNR3DHM_deinit
(
    DPU_SNR3DHM_Handle handle
);

/**
 *  @b Description
 *  @n
 *      The function returns number of allocated HWA PARAM Sets.
 *
 *  @pre    DPU_SNR3DHM_config() has been called
 *
 *  @param[in]  handle           SNR3DHM DPU handle
 *
 *  @param[out]  numUsedHwaParamSets           Number of allocated HWA PARAM Sets
 *
 *  \ingroup    DPU_SNR3DHM_EXTERNAL_FUNCTION
 *
 *  @retval
 *      Success     - 0
 *  @retval
 *      Error       - <0
 */
int32_t DPU_SNR3DHM_GetNumUsedHwaParamSets
(
    DPU_SNR3DHM_Handle   handle,
    uint8_t *numUsedHwaParamSets
);

#ifdef __cplusplus
}
#endif

#endif
