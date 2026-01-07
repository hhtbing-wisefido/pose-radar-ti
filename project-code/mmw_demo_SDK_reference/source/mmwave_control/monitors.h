/*
 * Copyright (C) 2025 Texas Instruments Incorporated
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 *   Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the
 *   distribution.
 *
 *   Neither the name of Texas Instruments Incorporated nor the names of
 *   its contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @brief Macro to enable/disable the monitors.
 *
 * @details
 * This macro is used to enable/disable the monitors.
 * If set to 1, the monitors are enabled.
 * If set to 0, the monitors are disabled.
 */
#define ENABLE_MONITORS                             (0)

/**
 * @brief Macro to enable/disable the printing of the monitor results.
 *
 * @details
 * This macro is used to enable/disable the printing of the monitor results.
 * If set to 1, the monitor results are printed.
 * If set to 0, the monitor results are not printed.
 */
#define PRINT_MON_RES                               (0)

/**
 * @brief Macro representing the monitor for TX0 DC.
 *
 * @details
 * This macro represents the monitor for TX0 DC.
 * It is used as a bit mask to enable the monitor.
 */
#define MON_TX0_DC_MON                              (25U)

/**
 * @brief Macro representing the monitor for TX1 DC.
 *
 * @details
 * This macro represents the monitor for TX1 DC.
 * It is used as a bit mask to enable the monitor.
 */
#define MON_TX1_DC_MON                              (26U)

/**
 * @brief Macro representing the monitor for TX2 DC.
 *
 * @details
 * This macro represents the monitor for TX2 DC.
 * It is used as a bit mask to enable the monitor.
 */
#define MON_TX2_DC_MON                              (27U)

/**
 * @brief Macro representing the monitor for TX3 DC.
 *
 * @details
 * This macro represents the monitor for TX3 DC.
 * It is used as a bit mask to enable the monitor.
 */
#define MON_TX3_DC_MON                              (28U)

/**
 * @brief Macro representing the monitor for RX HPF internal DC.
 *
 * @details
 * This macro represents the monitor for RX HPF internal DC.
 * It is used as a bit mask to enable the monitor.
 */
#define MON_RX_HPF_INTRNAL_DC_SIG                   (29U)

/**
 * @brief Macro representing the monitor for PM clock internal DC.
 *
 * @details
 * This macro represents the monitor for PM clock internal DC.
 * It is used as a bit mask to enable the monitor.
 */
#define MON_PM_CLK_INTRNAL_DC_SIG                   (30U)

/**
 * @brief Macro representing the monitor for DFE BIST FFT check.
 *
 * @details
 * This macro represents the monitor for DFE BIST FFT check.
 * It is used as a bit mask to enable the monitor.
 */
#define MON_DFE_BIST_FFT_CHECK                      (31U)

/**
 * @brief Macro representing the monitor for static register readback.
 *
 * @details
 * This macro represents the monitor for static register readback.
 * It is used as a bit mask to enable the monitor.
 */
#define MON_STATIC_REG_READBACK                     (32U)

/**
 * @brief Macro representing the LUT RAM offset for RX Saturation monitor.
 */
#define MON_RX_SAT_LUT_OFFSET                       (4096U)

#if (ENABLE_MONITORS==1)
/*The function is used to configure live monitors.*/
void mmwDemo_LiveMonConfig();


/*The function is used to configure the RF monitors.*/
void mmwDemo_MonitorConfig (void);
/*This function is used to get the results of RF monitors that are enabled*/
void mmwDemo_GetMonRes();
#endif
