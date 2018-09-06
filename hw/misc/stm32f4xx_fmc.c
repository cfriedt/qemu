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

#include "qemu/osdep.h"
#include "hw/misc/stm32f4xx_fmc.h"
#include "qemu/log.h"

#ifndef STM_FMC_ERR_DEBUG
#define STM_FMC_ERR_DEBUG 0
#endif

#define DB_PRINT_L(lvl, fmt, args...) do { \
    if (STM_FMC_ERR_DEBUG >= lvl) { \
        qemu_log("%s: " fmt, __func__, ## args); \
    } \
} while (0)

#define DB_PRINT(fmt, args...) DB_PRINT_L(1, fmt, ## args)

static void stm32f4xx_fmc_reset(DeviceState *dev)
{
    STM32F4XXFmcState *s = STM32F4XX_FMC(dev);

    s->fmc_bcr1  = 0x000030DB;
    s->fmc_btr1  = 0x00FFFFFF;
    s->fmc_bcr2  = 0x000030D2;
    s->fmc_btr2  = 0x00FFFFFF;
    s->fmc_bcr3  = 0x000030D2;
    s->fmc_btr3  = 0x00FFFFFF;
    s->fmc_bcr4  = 0x000030D2;
    s->fmc_btr4  = 0x00FFFFFF;
    s->fmc_pcr   = 0x00000018;
    s->fmc_sr    = 0x00000040;
    s->fmc_pmem  = 0xFCFCFCFC;
    s->fmc_patt  = 0xFCFCFCFC;
    s->fmc_eccr  = 0x00000000;
    s->fmc_bwtr1 = 0x000FFFFF;
    s->fmc_bwtr2 = 0x000FFFFF;
    s->fmc_bwtr3 = 0x000FFFFF;
    s->fmc_bwtr4 = 0x000FFFFF;
    s->fmc_sdcr1 = 0x0000D020;
    s->fmc_sdcr2 = 0x0000D020;
    s->fmc_sdtr1 = 0x0FFFFFFF;
    s->fmc_sdtr2 = 0x0FFFFFFF;
    s->fmc_sdcmr = 0x00000000;
    s->fmc_sdrtr = 0x00000000;
    s->fmc_sdsr  = 0x00000000;
}

static uint64_t stm32f4xx_fmc_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
    STM32F4XXFmcState *s = opaque;

    DB_PRINT("0x%"HWADDR_PRIx"\n", addr);

    switch (addr) {
    case FMC_BCR1:
    	return s->fmc_bcr1;
    case FMC_BTR1:
    	return s->fmc_btr1;
    case FMC_BCR2:
    	return s->fmc_bcr2;
    case FMC_BTR2:
    	return s->fmc_btr2;
    case FMC_BCR3:
    	return s->fmc_bcr3;
    case FMC_BTR3:
    	return s->fmc_btr3;
    case FMC_BCR4:
    	return s->fmc_bcr4;
    case FMC_BTR4:
    	return s->fmc_btr4;
    case FMC_PCR:
    	return s->fmc_pcr;
    case FMC_SR:
    	return s->fmc_sr;
    case FMC_PMEM:
    	return s->fmc_pmem;
    case FMC_PATT:
    	return s->fmc_patt;
    case FMC_ECCR:
    	return s->fmc_eccr;
    case FMC_BWTR1:
    	return s->fmc_bwtr1;
    case FMC_BWTR2:
    	return s->fmc_bwtr2;
    case FMC_BWTR3:
    	return s->fmc_bwtr3;
    case FMC_BWTR4:
    	return s->fmc_bwtr4;
    case FMC_SDCR1:
    	return s->fmc_sdcr1;
    case FMC_SDCR2:
    	return s->fmc_sdcr2;
    case FMC_SDTR1:
    	return s->fmc_sdtr1;
    case FMC_SDTR2:
    	return s->fmc_sdtr2;
    case FMC_SDCMR:
    	return s->fmc_sdcmr;
    case FMC_SDRTR:
    	return s->fmc_sdrtr;
    case FMC_SDSR:
    	return s->fmc_sdsr;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Bad offset 0x%"HWADDR_PRIx"\n", __func__, addr);
        return 0;
    }

    return 0;
}

static void stm32f4xx_fmc_write(void *opaque, hwaddr addr,
                       uint64_t val64, unsigned int size)
{
    STM32F4XXFmcState *s = opaque;
    uint32_t value = val64;

    DB_PRINT("0x%x, 0x%"HWADDR_PRIx"\n", value, addr);

    qemu_log_mask(LOG_UNIMP,
                  "%s: STM32F479 FMC 0x%"HWADDR_PRIx" Unimplemented\n", __func__, addr);

    switch (addr) {
    case FMC_BCR1:
    	s->fmc_bcr1  = value & 0x003FFB7F;
    	return;
    case FMC_BTR1:
    	s->fmc_btr1  = value & 0x3FFFFFFF;
    	return;
    case FMC_BCR2:
    	s->fmc_bcr2  = value & 0x000FFB7F;
    	return;
    case FMC_BTR2:
    	s->fmc_btr2  = value & 0x3FFFFFFF;
    	return;
    case FMC_BCR3:
    	s->fmc_bcr3  = value & 0x000FFB7F;
    	return;
    case FMC_BTR3:
    	s->fmc_btr3  = value & 0x3FFFFFFF;
    	return;
    case FMC_BCR4:
    	s->fmc_bcr4  = value & 0x000FFB7F;
    	return;
    case FMC_BTR4:
    	s->fmc_btr4  = value & 0x3FFFFFFF;
    	return;
    case FMC_PCR:
    	s->fmc_pcr   = value & 0x0007FF7E;
    	return;
    case FMC_SR:
    	s->fmc_sr    = value & 0x0000007F;
    	return;
    case FMC_PMEM:
    	s->fmc_pmem  = value & 0xFFFFFFFF;
    	return;
    case FMC_PATT:
    	s->fmc_patt  = value & 0xFFFFFFFF;
    	return;
    case FMC_ECCR:
    	s->fmc_eccr  = value & 0xFFFFFFFF;
    	return;
    case FMC_BWTR1:
    	s->fmc_bwtr1 = value & 0x300FFFFF;
    	return;
    case FMC_BWTR2:
    	s->fmc_bwtr2 = value & 0x300FFFFF;
    	return;
    case FMC_BWTR3:
    	s->fmc_bwtr3 = value & 0x300FFFFF;
    	return;
    case FMC_BWTR4:
    	s->fmc_bwtr4 = value & 0x300FFFFF;
    	return;
    case FMC_SDCR1:
    	s->fmc_sdcr1 = value & 0x00007FFF;
    	return;
    case FMC_SDCR2:
    	s->fmc_sdcr2 = value & 0x00001FFF;
    	return;
    case FMC_SDTR1:
    	s->fmc_sdtr1 = value & 0x0FFFFFFF;
    	return;
    case FMC_SDTR2:
    	s->fmc_sdtr2 = value & 0x0FFFFFFF;
    	return;
    case FMC_SDCMR:
    	s->fmc_sdcmr = value & 0x003FFFFF;
    	return;
    case FMC_SDRTR:
    	s->fmc_sdrtr = value & 0x00007FFF;
    	return;
    case FMC_SDSR:
    	s->fmc_sdsr  = value & 0x0000003E;
    	return;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Bad offset 0x%"HWADDR_PRIx"\n", __func__, addr);
    }
}

static const MemoryRegionOps stm32f4xx_fmc_ops = {
    .read = stm32f4xx_fmc_read,
    .write = stm32f4xx_fmc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void stm32f4xx_fmc_init(Object *obj)
{
    STM32F4XXFmcState *s = STM32F4XX_FMC(obj);

    sysbus_init_irq(SYS_BUS_DEVICE(obj), &s->irq);

    memory_region_init_io(&s->mmio, obj, &stm32f4xx_fmc_ops, s,
                          TYPE_STM32F4XX_FMC, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);
}

static void stm32f4xx_fmc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32f4xx_fmc_reset;
}

static const TypeInfo stm32f4xx_fmc_info = {
    .name          = TYPE_STM32F4XX_FMC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32F4XXFmcState),
    .instance_init = stm32f4xx_fmc_init,
    .class_init    = stm32f4xx_fmc_class_init,
};

static void stm32f4xx_fmc_register_types(void)
{
    type_register_static(&stm32f4xx_fmc_info);
}

type_init(stm32f4xx_fmc_register_types)
