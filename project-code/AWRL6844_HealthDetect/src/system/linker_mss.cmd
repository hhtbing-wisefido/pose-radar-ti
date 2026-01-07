/*----------------------------------------------------------------------------*/
/* linker_mss.cmd - MSS/R5F Linker Script                                    */
/*                                                                            */
/* AWRL6844 Health Detection Project                                         */
/* Based on: TI mmWave SDK r5f_linker.cmd                                    */
/* Modified: 2026-01-07                                                       */
/*----------------------------------------------------------------------------*/

/* Stack and Heap Configuration */
--stack_size=4096  /* Main stack for application code */
--heap_size=4096   /* Heap for malloc() and FreeRTOS pvPortMalloc */

/* Retain stack sections for each CPU mode */
--retain="*(.irqStack)"
--retain="*(.fiqStack)"
--retain="*(.abortStack)"
--retain="*(.undStack)"
--retain="*(.svcStack)"

/* Entry point - must be at address 0x0 */
-e_vectors

/* Stack Sizes for R5F CPU modes */
__IRQ_STACK_SIZE = 256;
__FIQ_STACK_SIZE = 256;
__ABORT_STACK_SIZE = 256;
__UNDEFINED_STACK_SIZE = 256;
__SVC_STACK_SIZE = 4096;

/*----------------------------------------------------------------------------*/
/* Memory Map - AWRL6844 MSS/R5F                                             */
/*----------------------------------------------------------------------------*/
MEMORY{
PAGE 0:
    /* R5F Local Memory */
    RESET_VECTORS  (X)  : origin=0x00000000 length= (0x00000100 - 0x10)                 /* Exception vectors - 240B */
    TCMA_RAM       (RX) : origin=0x00000100 length= (0x00080000 - 0x100 - 0x10)         /* TCMA RAM 512 KB (code+data) */
    TCMB_RBL_Reserv(RW) : origin=0x08000000 length= (0x00009000 - 0x10)                 /* TCMB RAM 36 KB reserved by RBL */
    TCMB_RAM       (RX) : origin=0x08009000 length= (0x00040000 - 0x9000 - 0x10)        /* TCMB RAM 220 KB (code+data) */
    
    /* Shared Memory for MSS-DSS Communication */
    IPC_SH_MEM     (RW) : origin=0x88000000 length= (0x00000400 - 0x10)                 /* 1 KB IPC mailbox */
    DSS_L3         (RW) : origin=0x88000400 length= (0x00160000 - 0x0400 - 0x10)        /* DSS L3 RAM 1407 KB */
    
    /* L3 Shared RAM - Health Detection Feature Data (896 KB) */
    /* Base: 0x51000000, partitioned as per src/common/shared_memory.h */
    L3_SHARED_RAM  (RW) : origin=0x51000000 length=0x000E0000                           /* 896 KB total */
}

/*----------------------------------------------------------------------------*/
/* Section Configuration                                                      */
/*----------------------------------------------------------------------------*/
SECTIONS{
    /* Exception Vectors - must be at 0x0 */
    .vectors:{} palign(8) > RESET_VECTORS

    /* R5F Boot Code */
    GROUP {
        .text.hwi: palign(8)      /* Hardware interrupt handlers */
        .text.cache: palign(8)    /* Cache operations */
        .text.mpu: palign(8)      /* MPU configuration */
        .text.boot: palign(8)     /* Boot code */
        .text:abort: palign(8)    /* Abort handlers (for XIP) */
    } > TCMA_RAM

    /* Code and Read-Only Data */
    GROUP {
        .text:   {} align(8)   /* Executable code */
        .rodata: {} align(8)   /* Constants and string literals */
    } >> TCMA_RAM | TCMB_RAM

    /* Initialized Data */
    GROUP {
        .data:   {} align(8)   /* Initialized global and static variables */
    } >> TCMA_RAM | TCMB_RAM

    /* Uninitialized Data (BSS) */
    GROUP {
        .bss:    {} align(8)   /* Uninitialized global and static variables */
        RUN_START(__BSS_START)
        RUN_END(__BSS_END)
    } >> TCMA_RAM | TCMB_RAM

    /* CPU Mode Stacks */
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
        
        .sysmem: {} align(8)   /* malloc() heap */
        .stack:  {} align(8)   /* main() stack */
    } > TCMA_RAM

    /* DSS L3 RAM Section (not recommended for MSS use) */
    .bss.l3 {} > DSS_L3

    /* L3 Shared RAM Section - Health Detection Feature Data */
    /* Use #pragma DATA_SECTION(var, ".l3_shared_ram") in C code */
    .l3_shared_ram : {} > L3_SHARED_RAM

    /* IPC Shared Memory Section */
    /* Use #pragma DATA_SECTION(var, ".ipc_sh_mem") in C code */
    .ipc_sh_mem {} > IPC_SH_MEM
}
/*----------------------------------------------------------------------------*/
