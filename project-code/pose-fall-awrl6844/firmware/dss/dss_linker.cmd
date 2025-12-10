/*
 * pose_dss_linker.cmd
 *
 * Linker command file for AWRL6844 DSS (C674x DSP)
 */

MEMORY
{
    /* L2 SRAM (DSP Local) - 256KB */
    L2_SRAM (RWX) : origin=0x00800000 length=0x00040000

    /* Shared L3 Memory - 2MB */
    L3_RAM (RW)   : origin=0x88000000 length=0x00200000
}

SECTIONS
{
    .text    : {} > L2_SRAM
    .const   : {} > L2_SRAM
    .cinit   : {} > L2_SRAM
    .switch  : {} > L2_SRAM
    .stack   : {} > L2_SRAM
    .bss     : {} > L2_SRAM
    .far     : {} > L2_SRAM
    .sysmem  : {} > L2_SRAM
    
    /* 共享数据段 (IPC & Data Path) */
    .radarCube      : {} > L3_RAM
    .detectionMatrix : {} > L3_RAM
    .pointCloud     : {} > L3_RAM
}
