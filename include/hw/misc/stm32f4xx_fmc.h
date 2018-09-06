/*
 * STM32F4XX FMC
 *
 * Copyright (c) 2018 Friedt Professional Engineering Services, Inc <info@friedt.co>
 * Written by Christopher Friedt
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef HW_STM32F4XX_FMC_H
#define HW_STM32F4XX_FMC_H

#include "hw/sysbus.h"
#include "hw/hw.h"

#define FMC_BCR1  0x000
#define FMC_BTR1  0x004
#define FMC_BCR2  0x008
#define FMC_BTR2  0x00C
#define FMC_BCR3  0x010
#define FMC_BTR3  0x014
#define FMC_BCR4  0x018
#define FMC_BTR4  0x01C
#define FMC_PCR   0x080
#define FMC_SR    0x084
#define FMC_PMEM  0x088
#define FMC_PATT  0x08C
#define FMC_ECCR  0x094
#define FMC_BWTR1 0x104
#define FMC_BWTR2 0x10C
#define FMC_BWTR3 0x114
#define FMC_BWTR4 0x11C
#define FMC_SDCR1 0x140
#define FMC_SDCR2 0x144
#define FMC_SDTR1 0x148
#define FMC_SDTR2 0x14C
#define FMC_SDCMR 0x150
#define FMC_SDRTR 0x154
#define FMC_SDSR  0x158

#define TYPE_STM32F4XX_FMC "stm32f4xx-fmc"
#define STM32F4XX_FMC(obj) \
    OBJECT_CHECK(STM32F4XXFmcState, (obj), TYPE_STM32F4XX_FMC)

typedef struct {
    /* <private> */
    SysBusDevice parent_obj;

    /* <public> */
    MemoryRegion mmio;

    uint32_t fmc_bcr1;
    uint32_t fmc_btr1;
    uint32_t fmc_bcr2;
    uint32_t fmc_btr2;
    uint32_t fmc_bcr3;
    uint32_t fmc_btr3;
    uint32_t fmc_bcr4;
    uint32_t fmc_btr4;
    uint32_t fmc_pcr;
    uint32_t fmc_sr;
    uint32_t fmc_pmem;
    uint32_t fmc_patt;
    uint32_t fmc_eccr;
    uint32_t fmc_bwtr1;
    uint32_t fmc_bwtr2;
    uint32_t fmc_bwtr3;
    uint32_t fmc_bwtr4;
    uint32_t fmc_sdcr1;
    uint32_t fmc_sdcr2;
    uint32_t fmc_sdtr1;
    uint32_t fmc_sdtr2;
    uint32_t fmc_sdcmr;
    uint32_t fmc_sdrtr;
    uint32_t fmc_sdsr;

    qemu_irq irq;
} STM32F4XXFmcState;

#endif /* HW_STM32F4XX_FMC_H */
