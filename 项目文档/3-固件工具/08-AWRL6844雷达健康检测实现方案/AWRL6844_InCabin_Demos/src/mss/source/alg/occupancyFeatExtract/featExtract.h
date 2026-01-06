/*! 
 *  \file   featExtract.h
 *
 *  \brief  Header file for occupancy detection system feature extraction module
 *
 * Copyright (C) 2024 Texas Instruments Incorporated - http://www.ti.com/ 
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

/** Occupancy detection system feature extraction algorithm
 *	This code is an implementation of the feature extraction logic of the occupancy detection algorithm
 *	The algorithm is designed to buffer point cloud data across frames and extract statistical features
 *  to be fed into the classifier layer. Each measurement point carries detection informations, including,
 *  range, azimuth, elevation, Doppler, and signal-to-noise ratio.
 *
 *  Input/output
 *	Algorithm inputs the point cloud generated in the detection layer
 *	Algorithm also expects the definitions of the target boxes (i.e. zones) and the sensor mounting parameters
 *	Algorithm outputs the statistical features computed from the point clud data for each zone defined
 *
 *  External API
 *  Application includes the following algorithm header
      #include <featExtract.h>
 *	All resources are allocated at create time during the featExtract_create call
 *	All resources are freed at delete time time during the featExtract_delete call
 *  Application is expected to implement the design pattern as described in the pseudo code bellow:
      h = featExtract_create(params);              // Creates an instance of the algorithm with a desired configuration
      while(running) {
          featExtract_compute(h, pCloud, &out);    // Runs a single step of the given algorithm instance with input
      }
      featExtract_delete(h);                       // Delete the algorithm instance
 *
 *  Dependencies
 *  Library is platform independent
 */

#ifndef FEATEXTRACT_H
#define FEATEXTRACT_H


#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <float.h>


/* Algorithm error codes */
#define FEXTRACT_ERRNO_BASE		(-8000)                      /* Base error code for algorithm */
#define FEXTRACT_EOK			(0)                          /* Error code: No errors */
#define FEXTRACT_EINVAL			(FEXTRACT_ERRNO_BASE-1)      /* Error Code: Invalid argument */
#define FEXTRACT_ENOTIMPL		(FEXTRACT_ERRNO_BASE-2)      /* Error Code: Operation is not implemented */
#define FEXTRACT_ENOMEM			(FEXTRACT_ERRNO_BASE-3)      /* Error Code: Out of memory */


/* Number of points limitations */
#define FEXTRACT_MAX_NUM_POINTS_PER_ZONE            (3200U) /* Defines maximum possible number of points expected per zone across multiple frames (needed to limit total point cloud buffer size) */
#define FEXTRACT_MAX_NUM_POINTS_PER_ZONE_PER_FRAME  (200U)  /* Defines maximum possible number of points expected per zone and per frame (needed to limit the clustering algorithm per frame) */
#define FEXTRACT_RATIO_OF_POINTS_ACROSS_FRAMES      (0.8f)  /* The ratio of total points expected to be buffered across frames */


/* Set the flag to 1 if points across frames are buffered in compressed format. Othwerwise (if 0), original points in uncompressed format will be used */
#define FEXTRACT_COMPRESS_POINTS    3


/* Boundary boxes limitations
 *  Application can configure the algorithm with scene boundries. Boundaries are defined as a boxes.
 */
#define FEXTRACT_MAX_OCCUPANCY_BOXES            (8U)   /* Maximum number of occupancy boxes */
#define FEXTRACT_MAX_CUBOID_PER_OCCUPANCY_BOX   (3U)   /* Maximum number of cuboids available per occupancy boxes */
#define FEXTRACT_MAX_TOTAL_CUBOIDS              (FEXTRACT_MAX_OCCUPANCY_BOXES * FEXTRACT_MAX_CUBOID_PER_OCCUPANCY_BOX) /* Maximum number of total cuboid definitions */


/* Benchmarking results. During runtime execution, the implementation can optionally return cycle counts for the sub-functions defined below
 * Each count is 32bit unsigned integer value representing a timestamp of free runing clock
 */
#define FEXTRACT_BENCHMARK_CREATE    (0U)    /* Cycle count at create function */
#define FEXTRACT_BENCHMARK_COMPUTE   (1U)    /* Cycle count after compute function */
#define FEXTRACT_BENCHMARK_SIZE      (FEXTRACT_BENCHMARK_COMPUTE)    /* Size of benchmarking array */


/* The structure defines a position in spherical coordinates */
typedef union
{
    float a[3];
    struct
	{
        float range;    /* Range, m */
        float azimuth;  /* Azimuth, rad */
        float elev;     /* Elevation, rad */
    };
} FEXTRACT_spherical_position;


/* The structure defines a position in cartesian coordinates */
typedef union
{
    float a[3];
    struct
	{
        float posX;     /* X dimension (left-right), m */
        float posY;     /* Y dimension (near-far), m */
        float posZ;     /* Z dimension (height), m */
    };
} FEXTRACT_cartesian_position;


/* Measurement point structure
 *  The structure describes measurement point format
 */
typedef union
{
    float a[5];
    struct
    {
        union
        {
           FEXTRACT_spherical_position vectorSph;       /* Spherical vector */
           FEXTRACT_cartesian_position vectorCart;      /* Cartesian vector */
        };
        float doppler;    /* Radial velocity, m/s */
        float snr;        /* Signal-to-noise ratio, linear */
    };
} FEXTRACT_measurementPoint;


/* Sensor position structure
 *  Application can configure algorithm with sensor position. Position is in cartesian space relative to the [3-dimentional] world.
 */
typedef union
{
    float a[3];
    struct
	{
        float x;     /* X dimension (left-right), m */
        float y;     /* Y dimension (near-far), m */
        float z;     /* Z dimension (height), m */
    };
} FEXTRACT_sensorPosition;


typedef union
{
    float a[2];
    struct
    {
        float doppler;    /* Radial velocity, m/s */
        float snr;        /* Signal-to-noise ratio, linear */
    };
} FEXTRACT_sideInfo;


/* Sensor orientation structure
 *  Application can configure algorithm with sensor orientation. Orientation is defined as boresight angular tilts.
 */
typedef union
{
    float a[3];
    struct
	{
        float xTilt;  /* Tilt around X axis (i.e., elevation tilt), in radians */
        float yTilt;  /* Tilt around Y axis (i.e., yaw tilt), in radians */
        float zTilt;  /* Tilt around Z axis (i.e., azimuth tilt), in radians */
    };
} FEXTRACT_sensorOrientation; 


/* Boundary box structure
 *  The structure defines the box element used to describe the scenery 
 */
typedef union
{
    float a[6];
    struct
	{
        float x1;    /* Left boundary, m */
        float x2;    /* Right boundary, m */
        float y1;    /* Near boundary, m */
        float y2;    /* Far boundary, m */
        float z1;    /* Bottom boundary, m */
        float z2;    /* Top boundary, m */
    };
} FEXTRACT_boundaryBox; 


/* Scenery parameters
 *  Scenery uses 3-dimensional Cartesian coordinate system
 *  It is expected that the Z=0 plane corresponds to the scene floor
 *  The X coordinates are left (negative)-right; the Y ccordinates are near-far
 *  Origin (O) is typically colocated with sensor projection to Z=0 plane
 *
 *  - Sensor Position is 3 dimentional coordinate of the sensor
 *    For example, (0,0,2) will indicate that sensor is directly above the origin at the height of 2m
 *  - Sensor Orientation is sensor's boresight rotation: elevation tilt (around X axis), azimuth tilt (around Z axis), and optionally (not common in a real world scenarios) yaw tilt (around Y axis)
 *
 *  User can define up to FEXTRACT_MAX_OCCUPANCY_BOXES occupancy boxes
 *  - Occupancy boxes are used to define area of interest
 */
typedef struct
{
    FEXTRACT_sensorPosition       sensorPosition;  /* Sensor position, set to (x, y, z) for 3D, all in m */
    FEXTRACT_sensorOrientation    sensorOrientation; /* Sensor orientation along (x, y, z) axes, all in radians */
    
    uint8_t  numOccupancyBoxes;                                         /* Number of occupancy boxes (i.e., zones) */
    uint8_t  numCuboidsPerOccupancyBox[FEXTRACT_MAX_OCCUPANCY_BOXES];   /* Number of cuboids per occupancy box */
    
    FEXTRACT_boundaryBox  cuboidDefs[FEXTRACT_MAX_TOTAL_CUBOIDS];        /* All the cuboid definitions */
} FEXTRACT_sceneryParams;

/* The structure defines the unit of each position in compressed format */
typedef struct
{
    float xUnit;        /* X dimension unit, m */
    float yUnit;        /* Y dimension unit, m */
    float zUnit;        /* Z dimension unit, m */
    float dopplerUnit;  /* Doppler dimension unit, m/s */
    float snrdBUnit;    /* SNR in dB dimension unit, */
} FEXTRACT_point_cloud_compressed_unit;


/* Algorithm module configuration */
typedef struct
{
	uint16_t maxNumPointsPerZonePerFrame;   /* Maximum number of measurement points expected per zone and per frame */
    uint16_t numFramesProc;                 /* Number of frames to be buffered in the processing */
       
    FEXTRACT_sceneryParams    sceneryParams;      /* Scenery parameters */

    bool cartesianInput;                /* The input point cloud format: (0) Spherical or (1) Cartesian */

    bool offsetCorrection;                          /* The flag to enable xy-domain offset correction: (true) to enable */
    float zOffset[FEXTRACT_MAX_OCCUPANCY_BOXES];    /* Z offsets for each occupancy box */
    
    bool dbScanFiltering;               /* The flag to enable 1-stage DBSCAN algorithm to filter noisy points: (true) to enable */
    float dbScanEpsilon;                /* DBSCAN epsilon for each stage */
    uint16_t dbScanMinPts;              /* DBSCAN minimum number of points for each stage */

    uint8_t *scratchBuffer;             /* Scratch memory needed for computations. Keep NULL to request internal allocation */
    uint32_t scratchBufferSizeInBytes;  /* The amount of bytes needed by the module for per frame scratch processing */

    FEXTRACT_point_cloud_compressed_unit pointCloudCompressionUnit; /* Point cloud compression units */
} FEXTRACT_moduleConfig;


/* Feature output per zone */
typedef struct
{
    float numPtsMean;       /* Mean of the number of points  */

    float xCordMean;        /* Mean of the x-coordinates of the point cloud */
    float yCordMean;        /* Mean of the y-coordinates of the point cloud */
    float zCordMean;        /* Mean of the z-coordinates of the point cloud */

    float xCordRms;         /* Root-mean-square of the x-coordinates of the point cloud */
    float yCordRms;         /* Root-mean-square of the y-coordinates of the point cloud */
    float zCordRms;         /* Root-mean-square of the z-coordinates of the point cloud */

    float xCordStd;         /* Standard deviation of the x-coordinates of the point cloud */
    float yCordStd;         /* Standard deviation of the y-coordinates of the point cloud */
    float zCordStd;         /* Standard deviation of the z-coordinates of the point cloud */

    float xCordMax;         /* Max of the x-coordinates of the point cloud */
    float yCordMax;         /* Max of the y-coordinates of the point cloud */
    float zCordMax;         /* Max of the z-coordinates of the point cloud */

    float xCordMin;         /* Min of the x-coordinates of the point cloud */
    float yCordMin;         /* Min of the y-coordinates of the point cloud */
    float zCordMin;         /* Min of the z-coordinates of the point cloud */

    float xCordSize;        /* Size of the x-coordinates of the point cloud */
    float yCordSize;        /* Size of the y-coordinates of the point cloud */
    float zCordSize;        /* Size of the z-coordinates of the point cloud */

    float volume;           /* Volume of the point cloud */

    float doppMean;         /* Mean of the Doppler of the point cloud */
    float snrMean;          /* Mean of the SNR of the point cloud */
    float doppRms;         /* Root-mean-square of the z-coordinates of the point cloud */
    float snrRms;         /* Root-mean-square of the z-coordinates of the point cloud */
    float doppStd;         /* Standard deviation of the z-coordinates of the point cloud */
    float snrStd;         /* Standard deviation of the z-coordinates of the point cloud */
    float doppMax;         /* Max of the z-coordinates of the point cloud */
    float snrMax;         /* Max of the z-coordinates of the point cloud */
    float doppMin;         /* Min of the z-coordinates of the point cloud */
    float snrMin;         /* Min of the z-coordinates of the point cloud */
    float doppSize;        /* Size of the z-coordinates of the point cloud */
    float snrSize;        /* Size of the z-coordinates of the point cloud */

} FEXTRACT_outputPerZone;


/* Feature output */
typedef struct
{
    bool featuresComputed;                                                  /* The flag to indicate if the algorithm computed the features */
    FEXTRACT_outputPerZone  featsPerZone[FEXTRACT_MAX_OCCUPANCY_BOXES];     /* Algorithm output per zone */
} FEXTRACT_output;


/* External API function declarations */
extern void *featExtract_create(FEXTRACT_moduleConfig *config, int32_t *errCode);
extern void featExtract_compute(void *handle, FEXTRACT_measurementPoint *points, uint16_t numPoints, FEXTRACT_output *featOut);
extern void featExtract_delete(void *handle);
extern int32_t featExtract_updateZoffsets(void *handle, float *zOffset, uint8_t numZoffsets);


/* Declarations of memory allocation functions. Call these functions to allocate or free memory */
extern void *featExtract_malloc(uint32_t sizeInBytes);
extern void featExtract_free(void *pFree, uint32_t sizeInBytes);
extern  uint32_t Cycleprofiler_getTimeStamp(void);

/* Cycle counter function */
#ifdef SUBSYS_MSS
#if defined (__GNUC__) && !defined(__ti__)
static inline uint32_t featExtract_getCycleCount (void)
{
    uint32_t value;
    // Read CCNT Register
    asm volatile ("MRC p15, 0, %0, c9, c13, 0\t\n": "=r"(value));
    return value;
}
#else
#define featExtract_getCycleCount() __MRC(15, 0, 9, 13, 0)
#endif
#endif


#ifdef __cplusplus
}
#endif

#endif
