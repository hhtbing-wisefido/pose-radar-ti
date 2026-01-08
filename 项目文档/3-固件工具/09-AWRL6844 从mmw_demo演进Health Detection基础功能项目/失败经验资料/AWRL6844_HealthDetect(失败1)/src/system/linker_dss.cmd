/*----------------------------------------------------------------------------*/
/* linker_dss.cmd - DSS/C66x Linker Script                                   */
/*                                                                            */
/* AWRL6844 Health Detection Project                                         */
/* Based on: TI mmWave SDK c66_linker.cmd                                    */
/* Modified: 2026-01-07                                                       */
/*----------------------------------------------------------------------------*/

/* Stack and Heap Configuration for C66x DSP */
--stack_size=0x2000   /* 8KB stack for DSP */
--heap_size=0x8000    /* 32KB heap for DSP algorithms */

/* Entry point for C66x */
-e_c_int00

/* C66x runs in little-endian mode */
-c

/*----------------------------------------------------------------------------*/
/* Memory Map - AWRL6844 DSS/C66x                                            */
/*----------------------------------------------------------------------------*/
MEMORY{
    /* C66x Local Memory - L1/L2 Cache */
    L2_RAM         (RWX): origin=0x00800000 length=0x00040000   /* L2 RAM 256 KB */
    
    /* DSS L3 RAM (shared with HWA and MSS) */
    L3_RAM         (RWX): origin=0x88000400 length=0x0015FC00   /* 1407 KB */
    
    /* L3 Shared RAM - Health Detection Feature Data (896 KB) */
    /* Base: 0x51000000, partitioned as per src/common/shared_memory.h */
    L3_SHARED_RAM  (RW) : origin=0x51000000 length=0x000E0000   /* 896 KB total */
    
    /* Partitions within L3_SHARED_RAM (see shared_memory.h for details) */
    /* 0x51000000-0x51001000: DPC Config (4KB)       - MSS writes, DSS reads */
    /* 0x51001000-0x51003000: Point Cloud (8KB)      - DSS writes, MSS reads */
    /* 0x51003000-0x51004000: Feature Data (4KB)     - DSS writes, MSS reads */
    /* 0x51004000-0x51005000: DPC Result (4KB)       - DSS writes, MSS reads */
    /* 0x51005000-0x510E0000: Scratch Buffer (876KB) - DSS internal use */
}

/*----------------------------------------------------------------------------*/
/* Section Configuration                                                      */
/*----------------------------------------------------------------------------*/
SECTIONS{
    /* Exception Vectors */
    .vecs > L2_RAM
    
    /* Code Sections */
    .text:   > L2_RAM | L3_RAM     /* Executable code */
    .const:  > L2_RAM | L3_RAM     /* Constants */
    .switch: > L2_RAM | L3_RAM     /* Switch tables */
    
    /* Data Sections */
    .data:   > L2_RAM | L3_RAM     /* Initialized data */
    .bss:    > L2_RAM | L3_RAM     /* Uninitialized data */
    .cinit:  > L2_RAM | L3_RAM     /* Initialization tables */
    .cio:    > L2_RAM              /* C I/O buffer */
    
    /* Stack and Heap */
    .stack:  > L2_RAM              /* DSP stack */
    .sysmem: > L3_RAM              /* Heap for malloc() */
    .far:    > L3_RAM              /* Far data */
    
    /* L3 Shared RAM Section - Health Detection Feature Data */
    /* DSS writes feature extraction results here */
    /* MSS reads from this section */
    /* Use #pragma DATA_SECTION(var, ".l3_shared_ram") in C code */
    .l3_shared_ram : {} > L3_SHARED_RAM
    
    /* DPC Configuration Section (DSS reads) */
    /* Use #pragma DATA_SECTION(var, ".dpc_config") to place data here */
    .dpc_config : {
        . = ALIGN(8);
    } > L3_SHARED_RAM
    
    /* Point Cloud Output Section (DSS writes) */
    /* Use #pragma DATA_SECTION(var, ".point_cloud") to place data here */
    .point_cloud : {
        . = ALIGN(8);
    } > L3_SHARED_RAM
    
    /* Feature Data Section (DSS writes) */
    /* Use #pragma DATA_SECTION(var, ".feature_data") to place data here */
    .feature_data : {
        . = ALIGN(8);
    } > L3_SHARED_RAM
}
/*----------------------------------------------------------------------------*/

/* Notes:
 * 1. L2_RAM (256KB) is preferred for frequently accessed code/data
 * 2. L3_RAM (1407KB) is used for large buffers and algorithms
 * 3. L3_SHARED_RAM (896KB) is for MSS-DSS communication
 * 4. DSS should write to .feature_data section after feature extraction
 * 5. Refer to src/common/shared_memory.h for exact memory layout
 */
