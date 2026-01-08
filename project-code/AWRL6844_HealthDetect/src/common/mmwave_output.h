/**
 * @file mmwave_output.h
 * @brief TLV Output Format Definitions for mmWave Visualizer Compatibility
 *
 * Reference: mmw_demo_SDK_reference/source/mmwave_demo.h
 * Adapted for: Health Detection three-layer architecture
 *
 * RTOS: FreeRTOS (L-SDK mandatory)
 * Note: L-SDK uses FreeRTOS, NOT TI-RTOS/BIOS
 *
 * Created: 2026-01-08
 */

#ifndef MMWAVE_OUTPUT_H
#define MMWAVE_OUTPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/*===========================================================================*/
/*                         Output Message Header                              */
/*===========================================================================*/

/**
 * @brief Magic Word for output packet synchronization
 * Standard mmWave SDK magic word: 0x0102 0x0304 0x0506 0x0708
 */
#define MMWAVE_OUTPUT_MAGIC_WORD_0  (0x0102U)
#define MMWAVE_OUTPUT_MAGIC_WORD_1  (0x0304U)
#define MMWAVE_OUTPUT_MAGIC_WORD_2  (0x0506U)
#define MMWAVE_OUTPUT_MAGIC_WORD_3  (0x0708U)

/**
 * @brief Platform identifier for AWRL6844
 */
#define MMWAVE_OUTPUT_PLATFORM_AWRL6844     (0xA6844U)

/**
 * @brief Output Message Header
 * Must be 32-byte aligned for efficient DMA transfer
 */
typedef struct MmwDemo_output_message_header_t
{
    uint16_t    magicWord[4];       /**< Magic word for sync: 0x0102 0x0304 0x0506 0x0708 */
    uint32_t    version;            /**< Version of the output format */
    uint32_t    totalPacketLen;     /**< Total packet length in bytes */
    uint32_t    platform;           /**< Platform identifier */
    uint32_t    frameNumber;        /**< Frame number */
    uint32_t    timeCpuCycles;      /**< Time in CPU cycles */
    uint32_t    numDetectedObj;     /**< Number of detected objects */
    uint32_t    numTLVs;            /**< Number of TLVs in this packet */
    uint32_t    subFrameNumber;     /**< Sub-frame number (0 for basic mode) */
} MmwDemo_output_message_header_t;

/*===========================================================================*/
/*                         TLV Types                                          */
/*===========================================================================*/

/**
 * @brief TLV Type Definitions
 * Type-Length-Value identifiers for different data types
 */
typedef enum MmwDemo_output_message_type_e
{
    /** @brief Point cloud in Cartesian format (x, y, z, doppler) */
    MMWDEMO_OUTPUT_MSG_DETECTED_POINTS = 1,

    /** @brief Range profile (magnitude vs range) */
    MMWDEMO_OUTPUT_MSG_RANGE_PROFILE,

    /** @brief Noise floor profile */
    MMWDEMO_OUTPUT_MSG_NOISE_PROFILE,

    /** @brief Azimuth static heatmap */
    MMWDEMO_OUTPUT_MSG_AZIMUT_STATIC_HEAT_MAP,

    /** @brief Range-Doppler heatmap */
    MMWDEMO_OUTPUT_MSG_RANGE_DOPPLER_HEAT_MAP,

    /** @brief Processing statistics */
    MMWDEMO_OUTPUT_MSG_STATS,

    /** @brief Detected points side information (SNR, noise) */
    MMWDEMO_OUTPUT_MSG_DETECTED_POINTS_SIDE_INFO,

    /** @brief Azimuth/Elevation static heatmap */
    MMWDEMO_OUTPUT_MSG_AZIMUT_ELEVATION_STATIC_HEAT_MAP,

    /** @brief Temperature statistics */
    MMWDEMO_OUTPUT_MSG_TEMPERATURE_STATS,

    /** @brief Spherical point cloud (range, azimuth, elevation, doppler) */
    MMWDEMO_OUTPUT_MSG_SPHERICAL_POINTS = 10,

    /** @brief Target list (for tracking) */
    MMWDEMO_OUTPUT_MSG_TARGET_LIST = 11,

    /** @brief Target index (point to target association) */
    MMWDEMO_OUTPUT_MSG_TARGET_INDEX = 12,

    /* Health Detection specific TLVs (starting from 1000) */

    /** @brief Presence detection result */
    MMWDEMO_OUTPUT_MSG_PRESENCE_DETECT = 1000,

    /** @brief Health features */
    MMWDEMO_OUTPUT_MSG_HEALTH_FEATURES = 1001,

    /** @brief Vital signs (breathing, heart rate) */
    MMWDEMO_OUTPUT_MSG_VITAL_SIGNS = 1002,

    /** @brief Motion energy */
    MMWDEMO_OUTPUT_MSG_MOTION_ENERGY = 1003,

    MMWDEMO_OUTPUT_MSG_MAX
} MmwDemo_output_message_type_e;

/*===========================================================================*/
/*                         TLV Header                                         */
/*===========================================================================*/

/**
 * @brief TLV Header Structure
 */
typedef struct MmwDemo_output_message_tl_t
{
    uint32_t    type;               /**< TLV type (@ref MmwDemo_output_message_type_e) */
    uint32_t    length;             /**< TLV payload length in bytes */
} MmwDemo_output_message_tl_t;

/*===========================================================================*/
/*                         Point Cloud Output Structures                      */
/*===========================================================================*/

/**
 * @brief Point Cloud Units
 * Reporting units for point cloud data
 */
typedef struct MmwDemo_output_message_point_unit_t
{
    float       xyzUnit;            /**< x/y/z coordinates unit in meters */
    float       dopplerUnit;        /**< Doppler unit in m/s */
    float       snrUnit;            /**< SNR unit in dB */
    float       noiseUnit;          /**< Noise unit in dB */
    uint16_t    numDetectedPoints;  /**< Number of detected points */
    uint16_t    reserved;           /**< Reserved for alignment */
} MmwDemo_output_message_point_unit_t;

/**
 * @brief UART Point (compressed format)
 * Compressed point cloud format for UART transmission
 */
typedef struct MmwDemo_output_message_UARTpoint_t
{
    int16_t     x;                  /**< X coordinate in xyzUnits */
    int16_t     y;                  /**< Y coordinate in xyzUnits */
    int16_t     z;                  /**< Z coordinate in xyzUnits */
    int16_t     doppler;            /**< Doppler in dopplerUnits */
    uint8_t     snr;                /**< SNR in snrUnits */
    uint8_t     noise;              /**< Noise in noiseUnits */
} MmwDemo_output_message_UARTpoint_t;

/**
 * @brief Point Side Information
 */
typedef struct MmwDemo_output_message_point_sideInfo_t
{
    uint16_t    snr;                /**< SNR (0.1 dB resolution) */
    uint16_t    noise;              /**< Noise level (0.1 dB resolution) */
} MmwDemo_output_message_point_sideInfo_t;

/*===========================================================================*/
/*                         Statistics Output                                  */
/*===========================================================================*/

/**
 * @brief Processing Statistics
 */
typedef struct MmwDemo_output_message_stats_t
{
    uint32_t    interFrameProcessingTimeUs; /**< Inter-frame processing time (us) */
    uint32_t    transmitOutputTimeUs;       /**< Transmit output time (us) */
    uint16_t    powerMeasured[4];           /**< Power measurements (100 uW units) */
    int16_t     tempReading[4];             /**< Temperature readings (deg C) */
} MmwDemo_output_message_stats_t;

/*===========================================================================*/
/*                         Health Detection Output Structures                 */
/*===========================================================================*/

/**
 * @brief Presence Detection TLV Payload
 */
typedef struct MmwDemo_output_message_presence_t
{
    uint8_t     isPresent;          /**< Presence detected flag */
    uint8_t     isMoving;           /**< Motion detected flag */
    uint8_t     presenceState;      /**< Presence state enum */
    uint8_t     reserved;           /**< Reserved for alignment */
    uint16_t    numPointsInZone;    /**< Points in detection zone */
    uint16_t    presenceCounter;    /**< Frames with presence */
    float       avgRange_m;         /**< Average range (m) */
    float       avgVelocity_mps;    /**< Average velocity (m/s) */
    float       avgAzimuth_deg;     /**< Average azimuth (deg) */
    float       avgElevation_deg;   /**< Average elevation (deg) */
} MmwDemo_output_message_presence_t;

/**
 * @brief Health Features TLV Payload
 */
typedef struct MmwDemo_output_message_health_features_t
{
    uint32_t    frameNum;           /**< Frame number */
    float       motionEnergy;       /**< Motion energy */
    float       motionEnergySmoothed; /**< Smoothed motion energy */
    float       peakSnr_dB;         /**< Peak SNR (dB) */
    float       avgSnr_dB;          /**< Average SNR (dB) */
    float       signalQuality;      /**< Signal quality (0-1) */
    uint16_t    numValidPoints;     /**< Number of valid points */
    uint8_t     healthState;        /**< Health state enum */
    uint8_t     reserved;           /**< Reserved for alignment */
} MmwDemo_output_message_health_features_t;

/**
 * @brief Vital Signs TLV Payload
 */
typedef struct MmwDemo_output_message_vital_signs_t
{
    float       breathingRate;      /**< Breathing rate (breaths/min) */
    float       breathingConfidence;/**< Breathing detection confidence */
    float       heartRate;          /**< Heart rate (bpm) */
    float       heartRateConfidence;/**< Heart rate detection confidence */
    uint8_t     breathingValid;     /**< Breathing measurement valid */
    uint8_t     heartRateValid;     /**< Heart rate measurement valid */
    uint16_t    reserved;           /**< Reserved for alignment */
} MmwDemo_output_message_vital_signs_t;

/*===========================================================================*/
/*                         Output Packet Limits                               */
/*===========================================================================*/

/** @brief Maximum output packet size in bytes */
#define MMWAVE_OUTPUT_MAX_PACKET_SIZE       (32768U)

/** @brief Maximum number of TLVs per packet */
#define MMWAVE_OUTPUT_MAX_TLVS              (16U)

/** @brief Output packet alignment requirement */
#define MMWAVE_OUTPUT_PACKET_ALIGNMENT      (32U)

/** @brief Maximum points per frame for output */
#define MMWAVE_OUTPUT_MAX_POINTS            (500U)

/*===========================================================================*/
/*                         Helper Macros                                      */
/*===========================================================================*/

/**
 * @brief Calculate TLV total size (header + payload, aligned)
 */
#define MMWAVE_OUTPUT_TLV_SIZE(payloadSize) \
    (sizeof(MmwDemo_output_message_tl_t) + \
     (((payloadSize) + 3U) & ~3U))

/**
 * @brief Check if magic word is valid
 */
#define MMWAVE_OUTPUT_IS_MAGIC_VALID(hdr) \
    (((hdr)->magicWord[0] == MMWAVE_OUTPUT_MAGIC_WORD_0) && \
     ((hdr)->magicWord[1] == MMWAVE_OUTPUT_MAGIC_WORD_1) && \
     ((hdr)->magicWord[2] == MMWAVE_OUTPUT_MAGIC_WORD_2) && \
     ((hdr)->magicWord[3] == MMWAVE_OUTPUT_MAGIC_WORD_3))

#ifdef __cplusplus
}
#endif

#endif /* MMWAVE_OUTPUT_H */
