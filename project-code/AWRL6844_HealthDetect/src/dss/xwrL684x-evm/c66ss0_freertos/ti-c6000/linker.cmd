/*----------------------------------------------------------------------------*/
/* c66_linker.cmd                                                             */
/*                                                                            */
/* (c) Texas Instruments 2024, All rights reserved.                           */
/*                                                                            */
/* Health Detection Demo - DSS (C66 DSP) Linker Command File                  */
/* Reference: AWRL6844_InCabin_Demos/src/dss/.../linker.cmd                   */
/*                                                                            */
/* Created: 2026-01-08                                                        */
/*----------------------------------------------------------------------------*/

/* This is the stack that is used by code running within main()
 * In case of NORTOS,
 * - This means all the code outside of ISR uses this stack
 * In case of FreeRTOS
 * - This means all the code until vTaskStartScheduler() is called in main()
 *   uses this stack.
 * - After vTaskStartScheduler() each task created in FreeRTOS has its own stack
 */
--stack_size=2048

/* This is the heap size for malloc() API in NORTOS and FreeRTOS
 * This is also the heap used by pvPortMalloc in FreeRTOS
 */
--heap_size=8192

--retain=_vectors

/*----------------------------------------------------------------------------*/
/* Memory Map Definitions                                                     */
/*----------------------------------------------------------------------------*/

/* L3 Memory Layout for Health Detection
 * Total L3: 0x160000 (1.4MB)
 *
 * Layout matches MSS linker.cmd for proper shared memory access
 */
#define MSS_L3RAM_SIZE (0xB0000 + 0x70000)
#define L1P_CACHE_SIZE (32*1024)
#define L1D_CACHE_SIZE (16*1024)

MEMORY
{
    /* DSS L1 Data Memory (minus cache) */
    DSS_L1D:      ORIGIN = 0xF00000, LENGTH = 0x00008000-L1D_CACHE_SIZE

    /* DSS L2 Memory - 384KB total */
    DSS_L2:       ORIGIN = 0x800000, LENGTH = 0x60000

    /* L3 Shared Memory Regions */
    DSS_L3_IPC:   ORIGIN = 0x88000000, LENGTH = 0x00000400              /* IPC Mailbox (1KB) */
    DSS_L3_MSS:   ORIGIN = 0x88000400, LENGTH = MSS_L3RAM_SIZE          /* MSS owned L3 */
    DSS_L3:       ORIGIN = (0x88000400 + MSS_L3RAM_SIZE), LENGTH = 0x160000 - (MSS_L3RAM_SIZE + 0x400)  /* DSS owned L3 */

    /* Shared memories for RTOS cores
     * On C66: make sure these are mapped as non-cache in MAR bits
     */
    USER_SHM_MEM:           ORIGIN = 0xC02E8000, LENGTH = 0x00004000
    LOG_SHM_MEM:            ORIGIN = 0xC02EC000, LENGTH = 0x00004000
}

/*----------------------------------------------------------------------------*/
/* Section Configuration                                                      */
/*----------------------------------------------------------------------------*/

SECTIONS
{
    /* Vector table - hard address at start of L2
     * Must be aligned to 1024 bytes
     */
    .text:vectors: {. = align(1024); } > 0x00800000

    /* Code sections -> L2 */
    .text:      {} > DSS_L2
    .const:     {} > DSS_L2
    .cinit:     {} > DSS_L2
    .data:      {} > DSS_L2
    .stack:     {} > DSS_L2
    .switch:    {} > DSS_L2
    .cio:       {} > DSS_L2
    .sysmem:    {} > DSS_L2
    .fardata:   {} > DSS_L2
    .far:       {} > DSS_L3

    /* These should be grouped together to avoid STATIC_BASE relative relocation linker error */
    GROUP {
        .rodata:    {}
        .bss:       {}
        .neardata:  {}
    } > DSS_L2

    /* Sections needed for C++ projects */
    GROUP {
        .c6xabi.exidx:  {} palign(8)   /* Needed for C++ exception handling */
        .init_array:    {} palign(8)   /* Contains function pointers called before main */
        .fini_array:    {} palign(8)   /* Contains function pointers called after main */
    } > DSS_L2

    /* L3 Data Sections */
    .bss.dss_l3 {} > DSS_L3

    /* Shared memory sections */
    .bss.user_shared_mem (NOLOAD) : {} > USER_SHM_MEM
    .bss.log_shared_mem  (NOLOAD) : {} > LOG_SHM_MEM

    /* Memory heap sections for signal processing */
    .ddrHeap:       {} >> DSS_L3
    .L2heap:        {} >> DSS_L2
    .L2ScratchSect: {} >> DSS_L3
    .L1ScratchSect: {} >> DSS_L1D
    .L1heap:        {} >> DSS_L1D
    .ovly           {} >  DSS_L2

    /* Health Detection specific sections */
    .bss.radar_cube     {} > DSS_L3      /* Radar cube data (accessed from MSS) */
    .bss.detect_matrix  {} > DSS_L3      /* Detection matrix */
    .bss.point_cloud    {} > DSS_L3      /* Point cloud output */
    .bss.health_data    {} > DSS_L3      /* Health feature data */
}
/*----------------------------------------------------------------------------*/
