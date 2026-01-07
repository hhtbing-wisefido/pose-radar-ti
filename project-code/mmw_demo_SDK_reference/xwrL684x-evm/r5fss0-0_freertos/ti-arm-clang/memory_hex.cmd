/*----------------------------------------------------------------------------*/
/* memory_hex.cmd                                                             */
/*                                                                            */
/* (c) Texas Instruments 2024, All rights reserved.                           */
/*----------------------------------------------------------------------------*/

/*
 * SPECIFY THE SYSTEM MEMORY MAP, KEEP ALL MEMORY SIZE multiple of 8 bytes(64 bits) to generate ECC
 */

ROMS
{
    /* 16 Bytes reserved at the end of each section for CRC */
    
    ROW        : org = 0x00000000     len = (0x00000F0)              romwidth=32         /* Exception vectors */
    files = { temp/mmwave_demo_reset_vectors.hex }
    ROW        : org = 0x00000100     len = (0x0007FEF0)             romwidth=32         /* TCMA RAM 512 KB in eclipsed mode */
    files = { temp/mmwave_demo_tcma_ram.hex }
    ROW        : org = 0x08000000     len = (0x00008FF0)             romwidth=32         /* TCMB RAM 36 KB used by RBL. Do not use for Code and Data Sections */
    files = { temp/mmwave_demo_tcmb_rbl_reserv.hex }
    ROW        : org = 0x08009000     len = (0x00036FF0)             romwidth=32         /* TCMB RAM 220 KB */
    files = { temp/mmwave_demo_tcmb_ram.hex }
    ROW        : org = 0x88000000     len = (0x000003F0)             romwidth=32         /* 1 KB Memory reserved for IPC */
    files = { temp/mmwave_demo_ipc_shm_mem.hex }
}


/*
 * END OF .cmd file
 */
