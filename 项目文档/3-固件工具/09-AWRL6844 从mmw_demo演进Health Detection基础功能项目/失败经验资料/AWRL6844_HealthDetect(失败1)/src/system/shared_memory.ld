/*----------------------------------------------------------------------------*/
/* shared_memory.ld - L3 Shared RAM Memory Map                               */
/*                                                                            */
/* AWRL6844 Health Detection Project                                         */
/* Defines the memory layout for MSS-DSS communication                       */
/* Created: 2026-01-07                                                        */
/*----------------------------------------------------------------------------*/

/* L3 Shared RAM Base and Total Size */
L3_SRAM_BASE = 0x51000000;
L3_SRAM_SIZE = 0x000E0000;  /* 896 KB = 917504 bytes */

/*----------------------------------------------------------------------------*/
/* Memory Region Partitioning                                                */
/*----------------------------------------------------------------------------*/

/* Region 1: DPC Configuration (4 KB)
 * Direction: MSS writes → DSS reads
 * Purpose: DPC parameters, CFAR config, AOA config
 */
DPC_CONFIG_BASE   = L3_SRAM_BASE + 0x00000;
DPC_CONFIG_SIZE   = 0x1000;  /* 4 KB */

/* Region 2: Point Cloud Data (8 KB)
 * Direction: DSS writes → MSS reads
 * Purpose: DPIF_PointCloudCartesian array (up to 256 points)
 * Format: {float x, y, z, velocity} per point + side info
 */
POINT_CLOUD_BASE  = L3_SRAM_BASE + 0x01000;
POINT_CLOUD_SIZE  = 0x2000;  /* 8 KB */

/* Region 3: Feature Data (4 KB) - HEALTH DETECTION KEY REGION
 * Direction: DSS writes → MSS reads
 * Purpose: Extracted features for health detection algorithms
 * Contents:
 *   - centerX, centerY, centerZ (centroid)
 *   - spreadXY, spreadZ (distribution)
 *   - avgVelocity, maxVelocity
 *   - numPoints, frameNumber
 */
FEATURE_DATA_BASE = L3_SRAM_BASE + 0x03000;
FEATURE_DATA_SIZE = 0x1000;  /* 4 KB */

/* Region 4: DPC Result (4 KB)
 * Direction: DSS writes → MSS reads
 * Purpose: DPC execution statistics, timestamps
 */
DPC_RESULT_BASE   = L3_SRAM_BASE + 0x04000;
DPC_RESULT_SIZE   = 0x1000;  /* 4 KB */

/* Region 5: Scratch Buffer (876 KB)
 * Direction: DSS internal use only
 * Purpose: FFT intermediate results, CFAR temp data, clustering buffers
 */
SCRATCH_BUFFER_BASE = L3_SRAM_BASE + 0x05000;
SCRATCH_BUFFER_SIZE = 0x000DB000;  /* 876 KB */

/*----------------------------------------------------------------------------*/
/* Validation                                                                 */
/*----------------------------------------------------------------------------*/
/* Total allocated = 4 + 8 + 4 + 4 + 876 = 896 KB ✓ */

/*----------------------------------------------------------------------------*/
/* Usage in C Code                                                            */
/*----------------------------------------------------------------------------*/
/*
 * MSS Side (src/mss/):
 * --------------------
 * // Read feature data from DSS
 * volatile HealthDetect_PointCloudFeatures_t *pFeatures = 
 *     (HealthDetect_PointCloudFeatures_t *)FEATURE_DATA_BASE;
 * 
 * float centerX = pFeatures->centerX;
 * uint32_t numPoints = pFeatures->numPoints;
 * 
 * 
 * DSS Side (src/dss/):
 * --------------------
 * // Write feature data for MSS
 * #pragma DATA_SECTION(gFeatureData, ".feature_data")
 * HealthDetect_PointCloudFeatures_t gFeatureData;
 * 
 * void FeatureExtract_run(...) {
 *     gFeatureData.centerX = computedCenterX;
 *     gFeatureData.numPoints = numPoints;
 *     // MSS will read from FEATURE_DATA_BASE
 * }
 */

/*----------------------------------------------------------------------------*/
/* Memory Access Rules                                                        */
/*----------------------------------------------------------------------------*/
/*
 * 1. MSS writes to DPC_CONFIG_BASE → DSS reads
 * 2. DSS writes to POINT_CLOUD_BASE → MSS reads
 * 3. DSS writes to FEATURE_DATA_BASE → MSS reads (KEY for health detection)
 * 4. DSS writes to DPC_RESULT_BASE → MSS reads
 * 5. SCRATCH_BUFFER_BASE is DSS internal only
 * 6. Never read and write the same region simultaneously
 * 7. Use cache coherency mechanisms if needed
 */

/*----------------------------------------------------------------------------*/
/* References                                                                 */
/*----------------------------------------------------------------------------*/
/*
 * - src/common/shared_memory.h - C header with same definitions
 * - src/common/health_detect_types.h - Feature data structure definition
 * - docs/memory_map.md - Detailed memory usage documentation
 */
