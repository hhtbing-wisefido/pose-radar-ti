/*
 *  Copyright (C) 2021 Texas Instruments Incorporated
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Auto generated file 
 */
#include "ti_drivers_config.h"
#include <drivers/pinmux.h>

static Pinmux_PerCfg_t gPinMuxMainDomainCfg[] = {
            /* QSPI0 pin config */
    /* QSPI_DOUT -> PAD_AC (A12) */
    {
        PIN_PAD_AC,
        ( PIN_MODE(0) | PIN_PULL_DISABLE )
    },
    /* QSPI_DIN -> PAD_AD (B13) */
    {
        PIN_PAD_AD,
        ( PIN_MODE(0) | PIN_PULL_DISABLE )
    },
    /* QSPI_QDIN0 -> PAD_AE (B12) */
    {
        PIN_PAD_AE,
        ( PIN_MODE(0) | PIN_PULL_DISABLE )
    },
    /* QSPI_QDIN1 -> PAD_AF (B11) */
    {
        PIN_PAD_AF,
        ( PIN_MODE(0) | PIN_PULL_DISABLE )
    },
    /* QSPI_CLK -> PAD_AA (B14) */
    {
        PIN_PAD_AA,
        ( PIN_MODE(0) | PIN_PULL_DISABLE )
    },
    /* QSPI_CS_N -> PAD_AB (A13) */
    {
        PIN_PAD_AB,
        ( PIN_MODE(0) | PIN_PULL_DISABLE )
    },

            /* GPIO pin config */
    /* GPIO5 -> PAD_AV (U16) */
    {
        PIN_PAD_AV,
        ( PIN_MODE(0) | PIN_PULL_DISABLE )
    },
            /* GPIO pin config */
    /* GPIO6 -> PAD_AW (U15) */
    {
        PIN_PAD_AW,
        ( PIN_MODE(1) | PIN_PULL_DISABLE )
    },

            /* I2C0 pin config */
    /* I2C_SCL -> PAD_BC (N16) */
    {
        PIN_PAD_BC,
        ( PIN_MODE(0) | PIN_PULL_DISABLE )
    },
    /* I2C_SDA -> PAD_BD (N17) */
    {
        PIN_PAD_BD,
        ( PIN_MODE(0) | PIN_PULL_DISABLE )
    },

            /* MCSPIA pin config */
    /* MCSPIA_CLK -> PAD_AG (A16) */
    {
        PIN_PAD_AG,
        ( PIN_MODE(0) | PIN_PULL_DISABLE )
    },
    /* MCSPIA_MOSI -> PAD_AI (A15) */
    {
        PIN_PAD_AI,
        ( PIN_MODE(0) | PIN_PULL_DISABLE )
    },
    /* MCSPIA_MISO -> PAD_AJ (B15) */
    {
        PIN_PAD_AJ,
        ( PIN_MODE(0) | PIN_PULL_DISABLE )
    },

            /* MCSPIA_CS0 pin config */
    /* MCSPIA_CS0 -> PAD_AH (B17) */
    {
        PIN_PAD_AH,
        ( PIN_MODE(0) | PIN_PULL_DISABLE )
    },


            /* UARTB pin config */
    /* UARTB_RX -> PAD_AP (R14) */
    {
        PIN_PAD_AP,
        ( PIN_MODE(2) | PIN_PULL_DISABLE )
    },
    /* UARTB_TX -> PAD_AO (P15) */
    {
        PIN_PAD_AO,
        ( PIN_MODE(2) | PIN_PULL_DISABLE )
    },
            /* UARTA pin config */
    /* UARTA_RX -> PAD_AM (R10) */
    {
        PIN_PAD_AM,
        ( PIN_MODE(0) | PIN_PULL_DISABLE )
    },
    /* UARTA_TX -> PAD_AN (T10) */
    {
        PIN_PAD_AN,
        ( PIN_MODE(0) | PIN_PULL_DISABLE )
    },

    {PINMUX_END, PINMUX_END}
};


/*
 * Pinmux
 */
void Pinmux_init(void)
{
    Pinmux_config(gPinMuxMainDomainCfg, PINMUX_DOMAIN_ID_MAIN);
}

