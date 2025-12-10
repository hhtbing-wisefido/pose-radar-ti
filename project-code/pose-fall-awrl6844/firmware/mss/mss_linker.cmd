/*
 * pose_mss_linker.cmd
 *
 * Linker command file for AWRL6844 MSS (Cortex-R4F)
 */

/* 
 * 内存映射 (参考 AWRL6844 TRM)
 * TCMA: 0x00000000 (512KB) - Vector Table & Code
 * TCMB: 0x08000000 (512KB) - Data & Stack
 * L3 RAM: 0x88000000 (2MB) - Shared Data (Radar Cube, etc.)
 */

MEMORY
{
    /* R4F Tightly Coupled Memory A (Code) */
    VECTORS (X)   : origin=0x00000000 length=0x00000100
    FLASH_CODE (RX) : origin=0x00000100 length=0x0007FF00

    /* R4F Tightly Coupled Memory B (Data) */
    TCMA_RAM (RW) : origin=0x08000000 length=0x00080000

    /* Shared L3 Memory */
    L3_RAM (RW)   : origin=0x88000000 length=0x00200000
    
    /* Hardware Accelerator (HWA) Memory */
    HWA_RAM (RW)  : origin=0x50080000 length=0x00010000
}

SECTIONS
{
    .intvecs : {} > VECTORS
    .text    : {} > FLASH_CODE
    .const   : {} > FLASH_CODE
    .cinit   : {} > FLASH_CODE
    .pinit   : {} > FLASH_CODE

    .bss     : {} > TCMA_RAM
    .data    : {} > TCMA_RAM
    .stack   : {} > TCMA_RAM
    .sysmem  : {} > TCMA_RAM
    
    /* 共享数据段 (IPC & Data Path) */
    .radarCube      : {} > L3_RAM
    .detectionMatrix : {} > L3_RAM
    .pointCloud     : {} > L3_RAM
    .trackerTargets : {} > L3_RAM
    .ipcData        : {} > L3_RAM
}
