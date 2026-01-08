/*
 * shared_memory.ld
 * 
 * Shared Memory Region Definitions for Health Detection Project
 * 
 * Reference: AWRL6844_InCabin_Demos shared memory layout
 * Adapted for: Health Detection three-layer architecture
 *
 * This file defines the shared memory regions between MSS (R5F) and DSS (C66x)
 * for L3 RAM based inter-processor communication.
 *
 * Created: 2026-01-08
 */

/*============================================================================*/
/*                        L3 RAM Memory Layout                                 */
/*============================================================================*/

/*
 * AWRL6844 L3 RAM: 1408 KB total (0x160000 bytes)
 * Base Address: 0x88000000
 *
 * Layout:
 * ┌──────────────────────────────────────────────────────────────┐
 * │ 0x88000000  IPC Mailbox (1KB)                                │
 * ├──────────────────────────────────────────────────────────────┤
 * │ 0x88000400  MSS L3 Region (1152KB)                           │
 * │             - DPC Config (4KB)                               │
 * │             - DPC Result (4KB)                               │
 * │             - Point Cloud (64KB)                             │
 * │             - Radar Cube (704KB)                             │
 * │             - Detection Matrix (128KB)                       │
 * │             - Range Profile (32KB)                           │
 * │             - Health Features (8KB)                          │
 * │             - Reserved                                       │
 * ├──────────────────────────────────────────────────────────────┤
 * │ 0x88120400  DSS L3 Region (~255KB)                           │
 * │             - DSP Processing Buffers                         │
 * │             - Algorithm Scratch                              │
 * └──────────────────────────────────────────────────────────────┘
 */

/*============================================================================*/
/*                        Memory Regions                                       */
/*============================================================================*/

/* L3 RAM Base and Size */
L3_RAM_BASE         = 0x88000000;
L3_RAM_TOTAL_SIZE   = 0x160000;     /* 1408 KB */

/*----------------------------------------------------------------------------*/
/* IPC Mailbox Region */
/*----------------------------------------------------------------------------*/
L3_IPC_MBOX_BASE    = 0x88000000;
L3_IPC_MBOX_SIZE    = 0x400;        /* 1 KB */

/*----------------------------------------------------------------------------*/
/* MSS L3 Region */
/*----------------------------------------------------------------------------*/
L3_MSS_BASE         = 0x88000400;
L3_MSS_SIZE         = 0x120000;     /* 1152 KB (0xB0000 + 0x70000) */

/* MSS Sub-regions */
L3_DPC_CONFIG_BASE      = 0x88000400;
L3_DPC_CONFIG_SIZE      = 0x1000;   /* 4 KB */

L3_DPC_RESULT_BASE      = 0x88001400;
L3_DPC_RESULT_SIZE      = 0x1000;   /* 4 KB */

L3_POINT_CLOUD_BASE     = 0x88002400;
L3_POINT_CLOUD_SIZE     = 0x10000;  /* 64 KB */

L3_RADAR_CUBE_BASE      = 0x88012400;
L3_RADAR_CUBE_SIZE      = 0xB0000;  /* 704 KB */

L3_DET_MATRIX_BASE      = 0x880C2400;
L3_DET_MATRIX_SIZE      = 0x20000;  /* 128 KB */

L3_RANGE_PROFILE_BASE   = 0x880E2400;
L3_RANGE_PROFILE_SIZE   = 0x8000;   /* 32 KB */

L3_HEALTH_FEATURES_BASE = 0x880EA400;
L3_HEALTH_FEATURES_SIZE = 0x2000;   /* 8 KB */

L3_MSS_RESERVED_BASE    = 0x880EC400;
L3_MSS_RESERVED_SIZE    = 0x34000;  /* Remaining MSS region */

/*----------------------------------------------------------------------------*/
/* DSS L3 Region */
/*----------------------------------------------------------------------------*/
L3_DSS_BASE         = 0x88120400;
L3_DSS_SIZE         = 0x3FC00;      /* ~255 KB (remaining) */

/* DSS Sub-regions */
L3_DSS_SCRATCH_BASE     = 0x88120400;
L3_DSS_SCRATCH_SIZE     = 0x20000;  /* 128 KB scratch */

L3_DSS_ALGO_BASE        = 0x88140400;
L3_DSS_ALGO_SIZE        = 0x1FC00;  /* ~127 KB algorithm buffers */

/*============================================================================*/
/*                        IPC Message Structures                               */
/*============================================================================*/

/*
 * IPC Mailbox Layout (1KB total):
 * 
 * Offset   Size    Description
 * ------   ----    -----------
 * 0x000    64B     MSS -> DSS Command
 * 0x040    64B     DSS -> MSS Response
 * 0x080    64B     Frame Info
 * 0x0C0    64B     Status Flags
 * 0x100    768B    Reserved
 */

IPC_MSS_TO_DSS_CMD_OFFSET   = 0x000;
IPC_MSS_TO_DSS_CMD_SIZE     = 0x040;    /* 64 bytes */

IPC_DSS_TO_MSS_RSP_OFFSET   = 0x040;
IPC_DSS_TO_MSS_RSP_SIZE     = 0x040;    /* 64 bytes */

IPC_FRAME_INFO_OFFSET       = 0x080;
IPC_FRAME_INFO_SIZE         = 0x040;    /* 64 bytes */

IPC_STATUS_FLAGS_OFFSET     = 0x0C0;
IPC_STATUS_FLAGS_SIZE       = 0x040;    /* 64 bytes */

/*============================================================================*/
/*                        Data Structure Alignment                             */
/*============================================================================*/

/* Cache line alignment for DMA and cache coherency */
CACHE_LINE_SIZE         = 64;

/* Point cloud entry size (aligned) */
POINT_CLOUD_ENTRY_SIZE  = 32;           /* 32 bytes per point (padded) */

/* Maximum number of points */
MAX_POINT_CLOUD_POINTS  = 2048;         /* 64KB / 32B = 2048 points */

/*============================================================================*/
/*                        Memory Section Assignments                           */
/*============================================================================*/

/*
 * Usage in linker.cmd:
 *
 * MEMORY {
 *     L3_IPC_MBOX    (RW) : origin = 0x88000000, length = 0x400
 *     L3_MSS         (RW) : origin = 0x88000400, length = 0x120000
 *     L3_DSS         (RW) : origin = 0x88120400, length = 0x3FC00
 * }
 *
 * SECTIONS {
 *     .ipcMailbox    > L3_IPC_MBOX
 *     .dpcConfig     > L3_MSS
 *     .pointCloud    > L3_MSS
 *     .radarCube     > L3_MSS
 *     .dssHeap       > L3_DSS
 * }
 */
