/**
 * @file data_path.h
 * @brief DPC Data Path Structures Definition
 *
 * Reference: AWRL6844_InCabin_Demos/src/common_mss_dss/dpc_common.h
 * Reference: mmw_demo_SDK_reference/source/mmwave_demo.h
 * Adapted for: Health Detection three-layer architecture
 *
 * RTOS: FreeRTOS (L-SDK mandatory)
 * Note: L-SDK uses FreeRTOS, NOT TI-RTOS/BIOS
 *
 * Created: 2026-01-08
 */

#ifndef DATA_PATH_H
#define DATA_PATH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/*===========================================================================*/
/*                         Profile Configuration                              */
/*===========================================================================*/

/**
 * @brief Profile Configuration Structure
 * Defines chirp profile parameters
 */
typedef struct Profile_Config_t
{
    uint16_t    profileId;          /**< Profile identifier (0-3) */
    float       startFreqGHz;       /**< Start frequency in GHz (60-64 for AWRL6844) */
    float       idleTimeUs;         /**< Idle time in microseconds */
    float       adcStartTimeUs;     /**< ADC start time in microseconds */
    float       rampEndTimeUs;      /**< Ramp end time in microseconds */
    float       freqSlopeConst;     /**< Frequency slope in MHz/us */
    uint16_t    txOutPower;         /**< TX output power backoff */
    uint16_t    txPhaseShifter;     /**< TX phase shifter config */
    uint16_t    numAdcSamples;      /**< Number of ADC samples per chirp */
    uint16_t    digOutSampleRate;   /**< Digital output sample rate (ksps) */
    uint8_t     hpfCornerFreq1;     /**< HPF1 corner frequency config */
    uint8_t     hpfCornerFreq2;     /**< HPF2 corner frequency config */
    uint8_t     rxGain;             /**< RX gain in dB */
} Profile_Config_t;

/**
 * @brief Chirp Configuration Structure
 * Defines individual chirp parameters
 */
typedef struct Chirp_Config_t
{
    uint16_t    chirpStartIdx;      /**< Start chirp index */
    uint16_t    chirpEndIdx;        /**< End chirp index */
    uint16_t    profileId;          /**< Associated profile ID */
    float       startFreqVar;       /**< Start frequency variation (MHz) */
    float       freqSlopeVar;       /**< Frequency slope variation (kHz/us) */
    float       idleTimeVar;        /**< Idle time variation (us) */
    float       adcStartTimeVar;    /**< ADC start time variation (us) */
    uint8_t     txEnable;           /**< TX antenna enable mask */
} Chirp_Config_t;

/**
 * @brief Frame Configuration Structure
 * Defines frame timing and structure
 */
typedef struct Frame_Config_t
{
    uint16_t    numChirpsPerFrame;  /**< Number of chirps per frame */
    uint16_t    chirpStartIdx;      /**< Starting chirp index */
    uint16_t    chirpEndIdx;        /**< Ending chirp index */
    uint16_t    numLoops;           /**< Number of loops per frame */
    uint16_t    numFrames;          /**< Number of frames (0 = infinite) */
    float       framePeriodMs;      /**< Frame period in milliseconds */
    float       triggerDelay;       /**< Trigger delay in microseconds */
    uint8_t     triggerSelect;      /**< Trigger selection */
} Frame_Config_t;

/*===========================================================================*/
/*                         CFAR Configuration                                 */
/*===========================================================================*/

/**
 * @brief CFAR Configuration Structure
 * Constant False Alarm Rate detection parameters
 */
typedef struct CFAR_Config_t
{
    uint8_t     cfarMethod;         /**< CFAR method (CA, CASO, CAGO, OS) */
    uint8_t     guardLen;           /**< Guard cells length */
    uint8_t     noiseLen;           /**< Noise/training cells length */
    float       thresholdScale;     /**< Threshold scale factor (dB) */
    uint8_t     peakGrouping;       /**< Peak grouping enable */
    float       sidelobeThreshold;  /**< Sidelobe threshold */
} CFAR_Config_t;

/**
 * @brief CFAR Range Configuration
 */
typedef struct CFAR_Range_Config_t
{
    CFAR_Config_t config;           /**< CFAR parameters */
    uint16_t    minRangeBin;        /**< Minimum range bin to search */
    uint16_t    maxRangeBin;        /**< Maximum range bin to search */
} CFAR_Range_Config_t;

/**
 * @brief CFAR Doppler Configuration
 */
typedef struct CFAR_Doppler_Config_t
{
    CFAR_Config_t config;           /**< CFAR parameters */
    uint16_t    minDopplerBin;      /**< Minimum Doppler bin to search */
    uint16_t    maxDopplerBin;      /**< Maximum Doppler bin to search */
} CFAR_Doppler_Config_t;

/*===========================================================================*/
/*                         AOA (Angle of Arrival) Configuration               */
/*===========================================================================*/

/**
 * @brief AOA Configuration Structure
 * Angle of Arrival estimation parameters
 */
typedef struct AOA_Config_t
{
    uint8_t     enabled;            /**< AOA processing enable */
    uint8_t     fftSize;            /**< AOA FFT size (log2) */
    float       minAzimuthDeg;      /**< Minimum azimuth angle (degrees) */
    float       maxAzimuthDeg;      /**< Maximum azimuth angle (degrees) */
    float       minElevationDeg;    /**< Minimum elevation angle (degrees) */
    float       maxElevationDeg;    /**< Maximum elevation angle (degrees) */
} AOA_Config_t;

/*===========================================================================*/
/*                         DPC Configuration                                  */
/*===========================================================================*/

/**
 * @brief DPC Static Configuration
 * Configuration that doesn't change during runtime
 */
typedef struct DPC_StaticConfig_t
{
    Profile_Config_t    profile;        /**< Chirp profile */
    Chirp_Config_t      chirp;          /**< Chirp configuration */
    Frame_Config_t      frame;          /**< Frame configuration */
    uint8_t             numTxAntennas;  /**< Number of TX antennas enabled */
    uint8_t             numRxAntennas;  /**< Number of RX antennas enabled */
    uint16_t            numRangeBins;   /**< Number of range bins */
    uint16_t            numDopplerBins; /**< Number of Doppler bins */
} DPC_StaticConfig_t;

/**
 * @brief DPC Dynamic Configuration
 * Configuration that can change during runtime
 */
typedef struct DPC_DynamicConfig_t
{
    CFAR_Range_Config_t     cfarRangeCfg;   /**< CFAR range config */
    CFAR_Doppler_Config_t   cfarDopplerCfg; /**< CFAR Doppler config */
    AOA_Config_t            aoaCfg;         /**< AOA config */
    float                   compRxChanCfg[16][2]; /**< RX channel compensation (re, im) */
} DPC_DynamicConfig_t;

/**
 * @brief Complete DPC Configuration
 */
typedef struct DPC_Config_t
{
    DPC_StaticConfig_t      staticCfg;      /**< Static configuration */
    DPC_DynamicConfig_t     dynamicCfg;     /**< Dynamic configuration */
    uint8_t                 isValid;        /**< Configuration valid flag */
} DPC_Config_t;

/*===========================================================================*/
/*                         SubFrame Configuration                             */
/*===========================================================================*/

/**
 * @brief SubFrame Configuration Structure
 * Configuration parameters for each subframe
 */
typedef struct SubFrame_Cfg_t
{
    /* Antenna Configuration */
    uint8_t     numTxAntennas;              /**< Number of TX antennas enabled */
    uint8_t     numRxAntennas;              /**< Number of RX antennas enabled */
    uint16_t    numVirtualAntennas;         /**< Number of virtual antennas */
    
    /* Range Configuration */
    uint16_t    numRangeBins;               /**< Number of range bins */
    uint16_t    numAdcSamples;              /**< Number of ADC samples per chirp */
    
    /* Doppler Configuration */
    uint16_t    numDopplerBins;             /**< Number of Doppler bins */
    uint16_t    numChirpsPerFrame;          /**< Total chirps per frame */
    
    /* Frame Timing */
    float       framePeriodMs;              /**< Frame period in milliseconds */
    float       chirpDurationUs;            /**< Single chirp duration in microseconds */
    
    /* Processing Configuration */
    DPC_StaticConfig_t  staticCfg;          /**< Static DPC configuration */
    DPC_DynamicConfig_t dynamicCfg;         /**< Dynamic DPC configuration */
    
    /* Memory Addresses */
    void        *radarCubeAddr;             /**< Radar cube memory address */
    uint32_t    radarCubeSize;              /**< Radar cube size in bytes */
    
    /* Flags */
    uint8_t     isValid;                    /**< Configuration valid flag */
} SubFrame_Cfg_t;

/*===========================================================================*/
/*                         Point Cloud Structures                             */
/*===========================================================================*/

/**
 * @brief Point Cloud Cartesian Coordinates
 * Single detected point in Cartesian coordinates
 */
typedef struct PointCloud_Cartesian_t
{
    float       x;                  /**< X coordinate in meters */
    float       y;                  /**< Y coordinate in meters */
    float       z;                  /**< Z coordinate in meters */
    float       velocity;           /**< Radial velocity in m/s */
} PointCloud_Cartesian_t;

/**
 * @brief Point Cloud Spherical Coordinates
 * Single detected point in spherical coordinates
 */
typedef struct PointCloud_Spherical_t
{
    float       range;              /**< Range in meters */
    float       azimuth;            /**< Azimuth angle in degrees */
    float       elevation;          /**< Elevation angle in degrees */
    float       velocity;           /**< Radial velocity in m/s */
} PointCloud_Spherical_t;

/**
 * @brief Point Cloud Side Information
 * Additional information for each detected point
 */
typedef struct PointCloud_SideInfo_t
{
    uint16_t    snr;                /**< Signal-to-noise ratio (0.1 dB) */
    uint16_t    noise;              /**< Noise level (0.1 dB) */
} PointCloud_SideInfo_t;

/**
 * @brief Generic Point Cloud Point
 * Generic point structure used for processing (alias to Cartesian)
 */
typedef PointCloud_Cartesian_t PointCloud_Point_t;

/**
 * @brief Complete Point Cloud Output
 */
typedef struct PointCloud_Output_t
{
    uint32_t                numDetectedPoints;  /**< Number of detected points */
    PointCloud_Cartesian_t  *points;            /**< Array of detected points */
    PointCloud_SideInfo_t   *sideInfo;          /**< Array of side information */
} PointCloud_Output_t;

/*===========================================================================*/
/*                         DPC Result Structures                              */
/*===========================================================================*/

/**
 * @brief DPC Processing Statistics
 */
typedef struct DPC_Stats_t
{
    uint32_t    frameStartTimeUs;       /**< Frame start timestamp (us) */
    uint32_t    framePeriodUs;          /**< Measured frame period (us) */
    uint32_t    chirpingTimeUs;         /**< Total chirping time (us) */
    uint32_t    activeTimeUs;           /**< Total active processing time (us) */
    uint32_t    interFrameTimeUs;       /**< Inter-frame processing time (us) */
    uint32_t    transmitTimeUs;         /**< Data transmission time (us) */
} DPC_Stats_t;

/**
 * @brief DPC Execute Result
 * Output from DPC processing
 */
typedef struct DPC_Result_t
{
    uint32_t                frameNum;           /**< Current frame number */
    PointCloud_Output_t     pointCloud;         /**< Point cloud output */
    uint16_t                *rangeProfile;      /**< Range profile data */
    uint16_t                rangeProfileSize;   /**< Range profile size */
    DPC_Stats_t             stats;              /**< Processing statistics */
    int32_t                 errorCode;          /**< Error code (0 = success) */
} DPC_Result_t;

/*===========================================================================*/
/*                         Derived Parameters                                 */
/*===========================================================================*/

/**
 * @brief Calculate range resolution in meters
 * @param bandwidth_MHz Chirp bandwidth in MHz
 * @return Range resolution in meters
 */
static inline float DataPath_getRangeResolution(float bandwidth_MHz)
{
    /* c / (2 * BW) where c = 3e8 m/s */
    return (3.0e8f / (2.0f * bandwidth_MHz * 1.0e6f));
}

/**
 * @brief Calculate velocity resolution in m/s
 * @param wavelength_m Wavelength in meters
 * @param numChirps Number of chirps per frame
 * @param framePeriod_s Frame period in seconds
 * @return Velocity resolution in m/s
 */
static inline float DataPath_getVelocityResolution(float wavelength_m, 
                                                    uint16_t numChirps, 
                                                    float framePeriod_s)
{
    return (wavelength_m / (2.0f * numChirps * framePeriod_s));
}

#ifdef __cplusplus
}
#endif

#endif /* DATA_PATH_H */
