/*----------------------------------------------------------------------------*/
/* r5f_linker.cmd                                                             */
/*                                                                            */
/* (c) Texas Instruments 2024, All rights reserved.                           */
/*                                                                            */
/* Health Detection Demo - MSS Linker Command File                            */
/* Reference: AWRL6844_InCabin_Demos/src/mss/.../linker.cmd                   */
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
--stack_size=4096
/* This is the heap size for malloc() API in NORTOS and FreeRTOS
 * This is also the heap used by pvPortMalloc in FreeRTOS
 */
--heap_size=4096

--retain="*(.irqStack)"
--retain="*(.fiqStack)"
--retain="*(.abortStack)"
--retain="*(.undStack)"
--retain="*(.svcStack)"

-e_vectors  /* This is the entry of the application, _vector MUST be placed starting address 0x0 */

/* Stack Sizes for various R5F modes */
__IRQ_STACK_SIZE = 256;
__FIQ_STACK_SIZE = 256;
__ABORT_STACK_SIZE = 256;
__UNDEFINED_STACK_SIZE = 256;
__SVC_STACK_SIZE = 4096;

/* L3 Memory Layout for Health Detection
 * Total L3: 0x160000 (1.4MB)
 * 
 * Layout:
 * - IPC Mailbox:       0x000400 (1KB)
 * - Radar Cube:        0x070000 (448KB) - ADC data storage
 * - Detection Matrix:  0x020000 (128KB) - Range-Doppler matrix
 * - Point Cloud:       0x008000 (32KB)  - Detected points
 * - Tracker Output:    0x010000 (64KB)  - Tracking results
 * - Health Features:   0x002000 (8KB)   - Vital signs data
 * - MSS Reserved:      Remaining for MSS use
 * - DSS Reserved:      Remaining for DSS use
 */
#define MSS_L3_SIZE (0xB0000 + 0x70000)

/*----------------------------------------------------------------------------*/
/* Memory Map                                                                 */
MEMORY{
PAGE 0:
    /* R5F TCM - Tightly Coupled Memory */
    RESET_VECTORS  (X)  : origin=0x00000000 length= 0x00000100                   /* Exception vectors */
    TCMA_RAM       (RX) : origin=0x00000100 length= (0x00080000 - 0x110)         /* TCMA RAM 512 KB in eclipsed mode */
    TCMB_RBL_Reserv(RW) : origin=0x08000000 length= 0x00009000                   /* TCMB RAM 36 KB used by RBL. Do not use for Code and Data Sections */
    TCMB_RAM       (RX) : origin=0x08009000 length= (0x00040000 - 0x9000)        /* TCMB RAM 256 KB */
    
    /* L3 Shared Memory - MSS/DSS IPC and Shared Data */
    DSS_L3_MBOX    (RW) : origin=0x88000000 length= 0x400                        /* DSS L3 MBOX memory 1 KB */
    DSS_L3         (RW) : origin=0x88000400 length= MSS_L3_SIZE                  /* DSS L3 used by MSS */
    DSS_L3_DSS     (RW) : origin=(0x88000400+MSS_L3_SIZE)  length= 0x160000 - (MSS_L3_SIZE + 0x400) /* DSS L3 used by DSS */
}

/*----------------------------------------------------------------------------*/
/* Section Configuration                                                      */
SECTIONS{
    /* This has the R5F entry point and vector table, this MUST be at 0x0 */
    .vectors:{} palign(8) > RESET_VECTORS

    /* This has the R5F boot code - placed in fast TCMA */
    GROUP {
        .text.hwi: palign(8)
        .text.cache: palign(8)
        .text.mpu: palign(8)
        .text.boot: palign(8)
        .text:abort: palign(8) /* this helps in loading symbols when using XIP mode */
    } > TCMA_RAM

    /* Code and Read-Only Data - spread across TCM */
    GROUP {
        .text:   {} align(8)   /* This is where code resides */
        .rodata: {} align(8)   /* This is where const's go */
    } >> TCMA_RAM | TCMB_RAM

    /* Initialized Data */
    GROUP {
        .data:   {} align(8)   /* This is where initialized globals and static go */
    } >> TCMA_RAM | TCMB_RAM

    /* Uninitialized Data (BSS) */
    GROUP {
        .bss:    {} align(8)   /* This is where uninitialized globals go */
        RUN_START(__BSS_START)
        RUN_END(__BSS_END)
    } >> TCMA_RAM | TCMB_RAM

    /* This is where the stacks for different R5F modes go */
    GROUP {
        .irqstack: {. = . + __IRQ_STACK_SIZE;} align(8)
        RUN_START(__IRQ_STACK_START)
        RUN_END(__IRQ_STACK_END)
        .fiqstack: {. = . + __FIQ_STACK_SIZE;} align(8)
        RUN_START(__FIQ_STACK_START)
        RUN_END(__FIQ_STACK_END)
        .svcstack: {. = . + __SVC_STACK_SIZE;} align(8)
        RUN_START(__SVC_STACK_START)
        RUN_END(__SVC_STACK_END)
        .abortstack: {. = . + __ABORT_STACK_SIZE;} align(8)
        RUN_START(__ABORT_STACK_START)
        RUN_END(__ABORT_STACK_END)
        .undefinedstack: {. = . + __UNDEFINED_STACK_SIZE;} align(8)
        RUN_START(__UNDEFINED_STACK_START)
        RUN_END(__UNDEFINED_STACK_END)
        .sysmem: {} align(8)   /* This is where the malloc heap goes */
        .stack:  {} align(8)   /* This is where the main() stack goes */
    } > TCMA_RAM

    /* L3 Shared Memory Sections */
    
    /* IPC Mailbox for MSS-DSS communication */
    .bss.ipc_mailbox {} > DSS_L3_MBOX
    
    /* Any data buffer needed to be put in L3 can be assigned this section name */
    .bss.l3 {} > DSS_L3
    
    /* Health detection specific L3 sections */
    .bss.radar_cube {} > DSS_L3      /* Radar cube data */
    .bss.detect_matrix {} > DSS_L3   /* Detection matrix */
    .bss.point_cloud {} > DSS_L3     /* Point cloud output */
    .bss.tracker_output {} > DSS_L3  /* Tracker results */
    .bss.health_features {} > DSS_L3 /* Health feature data */
}
/*----------------------------------------------------------------------------*/
