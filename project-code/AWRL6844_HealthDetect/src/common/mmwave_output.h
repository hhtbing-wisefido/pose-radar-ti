/**
 * @file mmwave_output.h
 * @brief TLV (Tag-Length-Value) Output Definitions
 * 
 * Defines UART output format for health detection results.
 * Reference: mmw_demo TLV output structures
 * Adapted for: Three-layer architecture
 */

#ifndef MMWAVE_OUTPUT_H
#define MMWAVE_OUTPUT_H

#include <stdint.h>

/*----------------------------------------------------------------------------*/
/* TLV Message Header (UART Output)                                          */
/*----------------------------------------------------------------------------*/

#define MMWAVE_OUTPUT_MAGIC_WORD    0x0102030405060708ULL

/**
 * @brief TLV Packet Header
 * Same format as mmw_demo for compatibility
 */
typedef struct __attribute__((packed)) MmwaveOutput_Header_t {
    uint64_t magicWord;          /* 0x0102030405060708 */
    uint32_t version;            /* Format version */
    uint32_t totalPacketLen;     /* Bytes including header */
    uint32_t platform;           /* 0x6844 for AWRL6844 */
    uint32_t frameNumber;
    uint32_t timeCpuCycles;      /* SOC timestamp */
    uint32_t numDetectedObjects;
    uint32_t numTLVs;            /* TLV count in this packet */
    uint32_t subFrameNumber;     /* Always 0 for single-frame */
} MmwaveOutput_Header_t;

/*----------------------------------------------------------------------------*/
/* TLV Type Codes                                                            */
/*----------------------------------------------------------------------------*/

typedef enum {
    /* Standard mmw_demo TLVs */
    TLV_TYPE_DETECTED_POINTS        = 1,  /* Point cloud */
    TLV_TYPE_RANGE_PROFILE          = 2,
    TLV_TYPE_NOISE_PROFILE          = 3,
    TLV_TYPE_AZIMUTH_STATIC_HEATMAP = 4,
    TLV_TYPE_RANGE_DOPPLER_HEATMAP  = 5,
    TLV_TYPE_STATS                  = 6,
    
    /* Health Detection Custom TLVs (100+) */
    TLV_TYPE_HEALTH_FEATURES        = 100, /* Feature extraction results */
    TLV_TYPE_PRESENCE_STATE         = 101, /* Presence detection */
    TLV_TYPE_POSE_STATE             = 102, /* Pose estimation */
    TLV_TYPE_FALL_STATE             = 103, /* Fall detection */
    TLV_TYPE_VITAL_SIGNS            = 104, /* Heart rate, breathing */
    
} TLV_Type_e;

/**
 * @brief TLV Element Header
 */
typedef struct __attribute__((packed)) TLV_Header_t {
    uint32_t type;               /* TLV_Type_e */
    uint32_t length;             /* Payload bytes (excluding this header) */
} TLV_Header_t;

/*----------------------------------------------------------------------------*/
/* TLV Payload Structures                                                    */
/*----------------------------------------------------------------------------*/

/**
 * @brief TLV Type 1: Detected Points (Point Cloud)
 * Format: Array of (x, y, z, velocity, snr)
 */
typedef struct __attribute__((packed)) TLV_DetectedPoint_t {
    float x;                     /* Range (m) */
    float y;                     /* Azimuth (m) */
    float z;                     /* Elevation (m) */
    float velocity;              /* Doppler (m/s) */
    float snr;                   /* Signal-to-noise ratio (dB) */
} TLV_DetectedPoint_t;

/**
 * @brief TLV Type 6: Statistics
 */
typedef struct __attribute__((packed)) TLV_Stats_t {
    uint32_t interFrameProcessingTime; /* CPU cycles */
    uint32_t transmitOutputTime;
    uint32_t interFrameProcessingMargin;
    uint32_t interChirpProcessingMargin;
    uint32_t activeFrameCpuLoad;
    uint32_t interFrameCpuLoad;
} TLV_Stats_t;

/**
 * @brief TLV Type 100: Health Features (Custom)
 * Payload: HealthDetect_PointCloudFeatures_t from health_detect_types.h
 */

/**
 * @brief TLV Type 101: Presence State (Custom)
 */
typedef struct __attribute__((packed)) TLV_PresenceState_t {
    uint8_t  state;              /* PresenceState_e */
    float    confidence;         /* 0.0-1.0 */
    uint32_t lastMotionTimestamp;
} TLV_PresenceState_t;

/*----------------------------------------------------------------------------*/
/* TLV Builder Helper Macros                                                 */
/*----------------------------------------------------------------------------*/

#define TLV_HEADER_SIZE sizeof(TLV_Header_t)

#define WRITE_TLV_HEADER(ptr, type_val, len_val) \
    do { \
        ((TLV_Header_t*)(ptr))->type = (type_val); \
        ((TLV_Header_t*)(ptr))->length = (len_val); \
    } while(0)

#endif /* MMWAVE_OUTPUT_H */
