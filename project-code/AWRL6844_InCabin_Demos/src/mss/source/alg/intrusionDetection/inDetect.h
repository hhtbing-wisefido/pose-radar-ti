/*! 
 *  \file   inDetect.h
 *
 *  \brief  Header file for intrusion detection module
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

/** Intrusion detection algorithm
 *	This code is an implementation of the high-level processing logic of the intrusion detection algorithm
 *	The algorithm is designed to detect if an intrusion event occurs inside a vehicle 
 *
 *  Input/output
 *	Algorithm inputs the signal-noise-ratio (SNR) heatmap generated in 3D range-azimuth-elevation domain
 *  It is expected that the 3D SNR heatmap is created in the memory as [Elevation][Azimuth][Range] format
 *    The range samples for each azimuth-elevation bin are located in the continuous memory
 *    The next azimuth bin is located in the following memory block
 *    The next elevation bins is located in the following memory block
 *	Algorithm also expects the definitions of the target boxes where the occupancy is expected. A reference box should also be defined
 *	Algorithm outputs the final occupancy signal level for each target boundary box defined. At the application domain, this signal level can then be compared with a threshold to make a binary decision
 *  If this threshold is also provided to the algorithm along with some other state parameters, it can also generate the final binary decision within the algorithm implementation
 *
 *  External API
 *  Application includes the following algorithm header
      #include <inDetect.h>
 *	All resources are allocated at create time during the inDetect_create call
 *	All resources are freed at delete time time during the inDetect_delete call
 *  Application is expected to implement the design pattern as described in the pseudo code bellow:
      h = inDetect_create(params);                 // Creates an instance of the algorithm with a desired configuration
      while(running) {
          inDetect_compute(h, heatmap, &out);    // Runs a single step of the given alrorithm instance with input SNR heatmap
      }
      inDetect_delete(h);                          // Delete the algorithm instance
 *
 *  Dependencies
 *  Library is platform independent
 */

#ifndef INTRDETECT_H
#define INTRDETECT_H


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
#define IDETECT_ERRNO_BASE		(-8000)                     /* Base error code for IDETECT algorithm */
#define IDETECT_EOK			    (0)                         /* Error code: No errors */
#define IDETECT_EINVAL			(IDETECT_ERRNO_BASE-1)      /* Error Code: Invalid argument */
#define IDETECT_ENOTIMPL		(IDETECT_ERRNO_BASE-2)      /* Error Code: Operation is not implemented */
#define IDETECT_ENOMEM			(IDETECT_ERRNO_BASE-3)      /* Error Code: Out of memory */


/* Maximum supported configurations */
#define IDETECT_RANGE_BINS_MAX		(64U)  /* Defines maximum number of range bins the algorithm will accept at configuration time */
#define IDETECT_AZIM_BINS_MAX		(32U)  /* Defines maximum number of azimuth bins the algorithm will accept at configuration time */
#define IDETECT_ELEV_BINS_MAX		(32U)  /* Defines maximum number of elevation bins the algorithm will accept at configuration time */


/* Heatmap in log2 domain: (1) Yes, (0) No, it is in linear */
#define HEATMAP_LOG2_FLAG           (1U)


/* Boundary boxes
 *  Application can configure the algorithm with scene boundries. Boundaries are defined as a boxes.
 */
#define IDETECT_MIN_OCCUPANCY_BOXES   (2U)    /* Minimum number of occupancy boxes required. At least a reference zone and an occupancy zone must be defined */
#define IDETECT_MAX_OCCUPANCY_BOXES   (16U)   /* Maximum number of occupancy boxes. Intrusion detection algorithm will determine whether box is occupied */


/* Benchmarking results. During runtime execution, intrusion detection step function can optionally return cycle counts for the sub-functions defined below
 * Each count is 32bit unsigned integer value representing a timestamp of free runing clock
 */
#define IDETECT_BENCHMARK_CREATE    (0U)    /* Cycle count at create function */
#define IDETECT_BENCHMARK_COMPUTE   (1U)    /* Cycle count after compute function */
#define IDETECT_BENCHMARK_SIZE      (IDETECT_BENCHMARK_COMPUTE)    /* Size of benchmarking array */


/* Internal scratch buffer size requred from the application domain 
 * If not provided by the application domain, this buffer will be allocated internally
 */
#define SCRATCH_BUFFER_SIZE_NEEDED (0U)


/* Target box definitions, Target box IDs are uint8_t, with valid IDs can range from 0 to 254. Other values as defined below */
#define IDETECT_POINT_REFERENCE_ZONE      (0U)      /* Point is associated to the reference occupancy box */
#define IDETECT_POINT_NOT_ASSOCIATED      (255U)    /* Point is not associated in any target occupancy box */


/* Sensor position structure
 *  Application can configure algorithm with sensor position. Position is in cartesian space relative to the [3-dimentional] world.
 */
typedef union {
    float a[3];
    struct
	{
        float x;     /* X dimension (left-right), m */
        float y;     /* Y dimension (near-far), m */
        float z;     /* Z dimension (height), m */
    };
} IDETECT_sensorPosition;


/* Sensor orientation structure
 *  Application can configure algorithm with sensor orientation. Orientation is defined as boresight angular tilts.
 */
typedef union {
    float a[3];
    struct
	{
        float xTilt;  /* Tilt around X axis (i.e., elevation tilt), in radians */
        float yTilt;  /* Tilt around Y axis (i.e., yaw tilt), in radians */
        float zTilt;  /* Tilt around Z axis (i.e., azimuth tilt), in radians */
    };
} IDETECT_sensorOrientation; 


/* Boundary box structure
 *  The structure defines the box element used to describe the scenery 
 */
typedef union {
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
} IDETECT_boundaryBox; 


/* Scenery parameters
 *  Scenery uses 3-dimensional Cartesian coordinate system
 *  It is expected that the Z=0 plane corresponds to the scene floor
 *  The X coordinates are left (negative)-right; the Y ccordinates are near-far
 *  Origin (O) is typically colocated with sensor projection to Z=0 plane
 *
 *  - Sensor Position is 3 dimentional coordinate of the sensor
 *    For example, (0,0,2) will indicate that sensor is directly above the origin at the height of 2m
 *  - Sensor Orientation is sensor's boresight rotation: elevation tilt and azimuth tilt
 *
 *  User can define up to IDETECT_MAX_OCCUPANCY_BOXES occupancy boxes
 *  - Occupancy boxes are used to define area of interest
 *  - The first occupancy boxes is always the reference zone where an occupancy is not expected to occur
 */
typedef struct {
    IDETECT_sensorPosition       sensorPosition;  /* Sensor position, set to (x, y, z) for 3D, all in m */
    IDETECT_sensorOrientation    sensorOrientation; /* Sensor orientation along (x, y, z) axes, all in radians */
    
    uint8_t  numOccupancyBoxes;     /* Number of occupancy boundary boxes. If defined (numOccupancyBoxes > 0), this box will be used to create occupancy signals */
    IDETECT_boundaryBox  occupancyBox[IDETECT_MAX_OCCUPANCY_BOXES];  /* Scene occupancy boxes */
} IDETECT_sceneryParams;


/* The structure describes the thresholds for state changing counters */
typedef struct {
	float occupancyThre[IDETECT_MAX_OCCUPANCY_BOXES];          /* Level of the occupancy signal threshold to decide a HIT */
	
    uint16_t free2activeThre[IDETECT_MAX_OCCUPANCY_BOXES];     /* FREE => ACTIVE threshold. This is a threshold for the number of continuous HITS to transition from DETECT to ACTIVE state */
    uint16_t active2freeThre[IDETECT_MAX_OCCUPANCY_BOXES];     /* ACTIVE => FREE threshold. This is a threshold for the number of continuous MISSES to transition from ACTIVE to FREE state */
} IDETECT_stateParams;


/* The structure describes the additional signal processing parameters */
typedef struct {
    uint8_t localPeakCheck[IDETECT_MAX_OCCUPANCY_BOXES];        /* Apply local peak check. (0) to disable, (1) Reserved, (2) Apply 2D local peak check across azimuth-elevation */
    float sidelobeThre[IDETECT_MAX_OCCUPANCY_BOXES];            /* Sidelobe threshold if applied (between 0 to 1). (0) to disable sidelobe check */
    uint8_t peakExpSamples[IDETECT_MAX_OCCUPANCY_BOXES];        /* Number of samples to be expanded around the local peak. (0) to disable */
} IDETECT_sigProcParams;


/* Intrusion detection algorithm configuration */
typedef struct
{
	uint16_t numRangeBins;    /* Number of range bins in for the SNR heatmap input. The library will allocate memories based on this parameter */
    uint16_t numAzimBins;     /* Number of azimuth bins in for the SNR heatmap input. The library will allocate memories based on this parameter */
    uint16_t numElevBins;     /* Number of elevation bins in for the SNR heatmap input. The library will allocate memories based on this parameter */
    
    float    rangeStep;       /* Conversion factor from index to range, in m */
    float    rangeBias;       /* Range bias applied into the range spectrum, in m */

    float    azimuthGrid[IDETECT_AZIM_BINS_MAX];    /* Azimuth grid, in radians */
    float    elevationGrid[IDETECT_ELEV_BINS_MAX];  /* Elevation grid, in radians */
    bool     isGridInMuNuDomain;                    /* Set this true if the azimuth-elevation grid is defined in mu-nu domain. Set to false if it is in angle domain */

    IDETECT_sceneryParams       sceneryParams;      /* Scenery parameters */
	IDETECT_stateParams         stateParams;        /* State parameters */
    IDETECT_sigProcParams       sigProcParams;      /* Signal processing parameters */

    float *scratchBuffer;   /* Scratch memory needed for computations. Keep NULL to request internal allocation */
    uint32_t scratchBufferSizeInBytes; /* Module needs SCRATCH_BUFFER_SIZE_NEEDED bytes */
} IDETECT_moduleConfig;


/* Feature output structure */
typedef struct
{
    float occBoxSignal[IDETECT_MAX_OCCUPANCY_BOXES];        /* Computed signal for each occupancy box */
    uint8_t occBoxDecision[IDETECT_MAX_OCCUPANCY_BOXES];    /* Final binary decision for each occupancy box */
} IDETECT_output;


/* Compute Spectrum Features Function Declaration */
extern void *inDetect_create(IDETECT_moduleConfig *config, int32_t *errCode);
extern void inDetect_compute(void *handle, const void *heatmap, IDETECT_output *out);
extern void inDetect_delete(void *handle);


/* Declarations of memory allocation functions. Call these functions to allocate or free memory */
extern void *inDetect_malloc(uint32_t sizeInBytes);
extern void inDetect_free(void *pFree, uint32_t sizeInBytes);


/* Cycle counter function */
#ifdef SUBSYS_MSS
#if defined (__GNUC__) && !defined(__ti__)
static inline uint32_t inDetect_getCycleCount (void)
{
    uint32_t value;
    // Read CCNT Register
    asm volatile ("MRC p15, 0, %0, c9, c13, 0\t\n": "=r"(value));
    return value;
}
#else
#define inDetect_getCycleCount() __MRC(15, 0, 9, 13, 0)
#endif
#endif


#ifdef __cplusplus
}
#endif

#endif
