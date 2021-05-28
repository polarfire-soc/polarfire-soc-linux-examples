// SPDX-License-Identifier: MIT
/*
 * LPDDR4 DMA example for the Microchip PolarFire SoC
 *
 * Copyright (c) 2021 Microchip Technology Inc. All rights reserved.
 */
//*==========================================================================*/
#ifndef MSS_FDMA_H
#define MSS_FDMA_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------Public Data Structure----------------------------*/

/*----------------------------------FDMA--------------------------------------*/
typedef struct _fdmaregs
{
    volatile uint32_t version_reg;       /*0x00  Version Register */
    volatile uint32_t start_op_reg;      /*0x04  Start Register */
    uint32_t pad1[0x2];                  /*0x08 0x0C */
    volatile uint32_t irq_status_reg;    /*0x10  Interrupt Staus Register */
    volatile uint32_t irq_mask_reg;      /*0x14  Interrupt Mask Register */
    volatile uint32_t irq_cler_reg;      /*0x18  Interrupt Cler Register */
    uint32_t pad2[0x11];                 /*0x1C to 0x5C */
    volatile  uint32_t exec_config;      /*0x60  Active transfer type */
    volatile  uint32_t exec_bytes;       /*0x64  Number of bytes remaining. */
    volatile  uint32_t exec_source;      /*0x68  Source current address. */
    volatile  uint32_t exec_destination; /*0x6C  Destination current address. */
} fdma_t;

#define FDMA_TR_SIZE     0x10000U
#define FDMA_CONF_VAL    0x0000F005U
#define FDMA_START       0x00000001U
#define FDMA_IRQ_MASK    0x00000001U
#define FDMA_MAP_SIZE    0x1000U

#endif /* MSS_FDMA_H */
