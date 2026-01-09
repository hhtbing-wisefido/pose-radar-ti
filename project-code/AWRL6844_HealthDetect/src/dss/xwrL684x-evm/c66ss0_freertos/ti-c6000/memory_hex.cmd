/*----------------------------------------------------------------------------*/
/* memory_hex.cmd                                                             */
/*                                                                            */
/* (c) Texas Instruments 2024, All rights reserved.                           */
/*                                                                            */
/* Health Detection Demo - DSS (C66 DSP) Hex Generation Memory Map            */
/* Reference: AWRL6844_InCabin_Demos/src/dss/.../memory_hex.cmd               */
/*                                                                            */
/* Created: 2026-01-09                                                        */
/*----------------------------------------------------------------------------*/

/*
 * SPECIFY THE SYSTEM MEMORY MAP, KEEP ALL MEMORY SIZE multiple of 8 bytes(64 bits) to generate ECC
 */

ROMS
{
    ROW1        : org = 0x00800000     len = 0x00060000     romwidth=32        /* DSS L2 Memory (384KB) */
    files = { temp/health_detect_dss_l2.hex }
    ROW2        : org = 0x44000000     len = 0x000003CE     romwidth=32        /* Mailbox HSM */
    files = { temp/health_detect_mailbox_hsm.hex }
    ROW3        : org = 0x44000400     len = 0x000003CE     romwidth=32        /* Mailbox R5F */
    files = { temp/health_detect_mailbox_r5f.hex }
    ROW4        : org = 0x88000000     len = 0x00200000     romwidth=32        /* DSS L3 Memory (2MB) */
    files = { temp/health_detect_dss_l3.hex }
    ROW5        : org = 0xC02E8000     len = 0x00004000     romwidth=32        /* User Shared Memory */
    files = { temp/health_detect_user_shm_mem.hex }
    ROW6        : org = 0xC02EC000     len = 0x00004000     romwidth=32        /* Log Shared Memory */
    files = { temp/health_detect_log_shm_mem.hex }
    ROW7        : org = 0xC5000200     len = 0x00001C80     romwidth=32        /* RTOS IPC Shared Memory */
    files = { temp/health_detect_rtos_nortos_ipc_shm_mem.hex }
}


/*
 * END OF .cmd file
 */
