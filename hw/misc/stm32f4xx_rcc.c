/*
 * STM32F4XX RCC
 *
 * Copyright (c) 2014 Alistair Francis <alistair@alistair23.me>
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
#include "hw/misc/stm32f4xx_rcc.h"
#include "qemu/log.h"

#ifndef STM_RCC_ERR_DEBUG
#define STM_RCC_ERR_DEBUG 0
#endif

#define BIT_IS_SET(x,n) ((x) & (1ULL << (n)))

#define DB_PRINT_L(lvl, fmt, args...) do { \
    if (STM_RCC_ERR_DEBUG >= lvl) { \
        qemu_log("%s: " fmt, __func__, ## args); \
    } \
} while (0)

#define DB_PRINT(fmt, args...) DB_PRINT_L(1, fmt, ## args)

static void stm32f4xx_rcc_reset(DeviceState *dev)
{
    STM32F4XXRccState *s = STM32F4XX_RCC(dev);

    s->rcc_cr = 0x00000083;
    s->rcc_pllcfgr = 0x24003010;
    s->rcc_cfgr = 0x00000000;
    s->rcc_cir = 0x00000000;
    s->rcc_ahb1rstr = 0x00000000;
    s->rcc_ahb2rstr = 0x00000000;
    s->rcc_ahb3rstr = 0x00000000;
    s->rcc_apb1rstr = 0x00000000;
    s->rcc_apb2rstr = 0x00000000;
    s->rcc_ahb1enr = 0x00100000;
    s->rcc_ahb2enr = 0x00000000;
    s->rcc_ahb3enr = 0x00000000;
    s->rcc_apb1enr = 0x00000000;
    s->rcc_apb2enr = 0x00000000;
    s->rcc_ahb1lpenr = 0x7EEF97FF;
    s->rcc_ahb2lpenr = 0x000000F1;
    s->rcc_ahb3lpenr = 0x00000003;
    s->rcc_apb1lpenr = 0xF6FEC9FF;
    s->rcc_apb2lpenr = 0x0C777F33;
    s->rcc_bdcr = 0x00000000;
    s->rcc_csr = 0x0E000000;
    s->rcc_sscgr = 0x00000000;
    s->rcc_plli2scfgr = 0x24003000;
    s->rcc_pll_saicfgr = 0x24003000;
    s->rcc_dckcfgr = 0x00000000;
}

static uint64_t stm32f4xx_rcc_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
    STM32F4XXRccState *s = opaque;

    DB_PRINT("0x%"HWADDR_PRIx"\n", addr);

    switch (addr) {
    case RCC_CR:
    	/* set the following bits high to prevent infinite software polling
    	 * HSIRDY (bit 1) if HSION (bit 0) is set
    	 * HSERDY (bit 17) if HSEON (bit 16) is set
    	 * PLLRDY (bit 25) if PLLON (bit 24) is set
    	 * PLLI2SRDY (bit 27) if PLLI2SON (bit 26) is set
    	 * PLLSAIRDY (bit 29) if PLLSAION (bit 28) is set
    	 */
    	return
    		0
    		| ( BIT_IS_SET(s->rcc_cr, 0) ? (1 <<  0) : 0 )
    		| ( BIT_IS_SET(s->rcc_cr,16) ? (1 << 17) : 0 )
    		| ( BIT_IS_SET(s->rcc_cr,24) ? (1 << 25) : 0 )
    		| ( BIT_IS_SET(s->rcc_cr,26) ? (1 << 27) : 0 )
    		| ( BIT_IS_SET(s->rcc_cr,28) ? (1 << 29) : 0 )
    		| s->rcc_cr;
    case RCC_PLLCFGR:
    	return s->rcc_pllcfgr;
    case RCC_CFGR:
    	/* set the following bits high to prevent infinite software polling
    	 * SWS[1:0] <= 0b00 when SW[1:0] is 0b00
    	 * SWS[1:0] <= 0b01 when SW[1:0] is 0b01
    	 * SWS[1:0] <= 0b10 when SW[1:0] is 0b10
    	 * SWS[1:0] <= 0b10 when SW[1:0] is 0b11
    	 */
    	return
    		0
    		| ( (0x1 == (s->rcc_cfgr & 0x3)) ? (0x1 << 2): 0x0 )
    		| ( (0x2 == (s->rcc_cfgr & 0x2)) ? (0x2 << 2): 0x0 )
    		| s->rcc_cfgr;
    case RCC_CIR:
    	return s->rcc_cir;
    case RCC_AHB1RSTR:
    	return s->rcc_ahb1rstr;
    case RCC_AHB2RSTR:
    	return s->rcc_ahb2rstr;
    case RCC_AHB3RSTR:
    	return s->rcc_ahb3rstr;
    case RCC_APB1RSTR:
    	return s->rcc_apb1rstr;
    case RCC_APB2RSTR:
    	return s->rcc_apb2rstr;
    case RCC_AHB1ENR:
    	return s->rcc_ahb1enr;
    case RCC_AHB2ENR:
    	return s->rcc_ahb2enr;
    case RCC_AHB3ENR:
    	return s->rcc_ahb3enr;
    case RCC_APB1ENR:
    	return s->rcc_apb1enr;
    case RCC_APB2ENR:
    	return s->rcc_apb2enr;
    case RCC_AHB1LPENR:
    	return s->rcc_ahb1lpenr;
    case RCC_AHB2LPENR:
    	return s->rcc_ahb2lpenr;
    case RCC_AHB3LPENR:
    	return s->rcc_ahb3lpenr;
    case RCC_APB1LPENR:
    	return s->rcc_apb1lpenr;
    case RCC_APB2LPENR:
    	return s->rcc_apb2lpenr;
    case RCC_BDCR:
    	return s->rcc_bdcr;
    case RCC_CSR:
    	return s->rcc_csr;
    case RCC_SSCGR:
    	return s->rcc_sscgr;
    case RCC_PLLI2SCFGR:
    	return s->rcc_plli2scfgr;
    case RCC_PLLSAICFGR:
    	return s->rcc_pll_saicfgr;
    case RCC_DCKCFGR:
    	return s->rcc_dckcfgr;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Bad offset 0x%"HWADDR_PRIx"\n", __func__, addr);
        return 0;
    }

    return 0;
}

static void stm32f4xx_rcc_write(void *opaque, hwaddr addr,
                       uint64_t val64, unsigned int size)
{
    STM32F4XXRccState *s = opaque;
    uint32_t value = val64;

    DB_PRINT("0x%x, 0x%"HWADDR_PRIx"\n", value, addr);

    qemu_log_mask(LOG_UNIMP,
                  "%s: STM32F479 RCC 0x%"HWADDR_PRIx" Unimplemented\n", __func__, addr);

    switch (addr) {
    case RCC_CR:
    	s->rcc_cr = value & 0x150D00F9;
    	return;
    case RCC_PLLCFGR:
    	s->rcc_pllcfgr = value & 0x7F437FFF;
    	return;
    case RCC_CFGR:
    	s->rcc_cfgr = value & 0xBFDFFCF3;
    	return;
    case RCC_CIR:
    	s->rcc_cir = value & 0x00FF7F00;
    	return;
    case RCC_AHB1RSTR:
    	s->rcc_ahb1rstr = value & 0x22E017FF;
    	return;
    case RCC_AHB2RSTR:
    	s->rcc_ahb2rstr = value & 0x000000F1;
    	return;
    case RCC_AHB3RSTR:
    	s->rcc_ahb3rstr = value & 0x00000003;
    	return;
    case RCC_APB1RSTR:
    	s->rcc_apb1rstr = value & 0xF2FEC9FF;
    	return;
    case RCC_APB2RSTR:
    	s->rcc_apb2rstr = value & 0x0C777933;
    	return;
    case RCC_AHB1ENR:
    	s->rcc_ahb1enr = value & 0x7E6417FF;
    	return;
    case RCC_AHB2ENR:
    	s->rcc_ahb2enr = value & 0x000000F1;
    	return;
    case RCC_AHB3ENR:
    	s->rcc_ahb3enr = value & 0x00000003;
    	return;
    case RCC_APB1ENR:
    	s->rcc_apb1enr = value & 0xF6FEC9FF;
    	return;
    case RCC_APB2ENR:
    	s->rcc_apb2enr = value & 0x0C77EF33;
    	return;
    case RCC_AHB1LPENR:
    	s->rcc_ahb1lpenr = value & 0x7EEF97FF;
    	return;
    case RCC_AHB2LPENR:
    	s->rcc_ahb2lpenr = value & 0x000000F1;
    	return;
    case RCC_AHB3LPENR:
    	s->rcc_ahb3lpenr = value & 0x00000003;
    	return;
    case RCC_APB1LPENR:
    	s->rcc_apb1lpenr = value & 0xF6FEC9FF;
    	return;
    case RCC_APB2LPENR:
    	s->rcc_apb2lpenr = value & 0x0C777F33;
    	return;
    case RCC_BDCR:
    	s->rcc_bdcr = value & 0x0001830D;
    	return;
    case RCC_CSR:
    	s->rcc_csr = value & 0xFF000001;
    	return;
    case RCC_SSCGR:
    	s->rcc_sscgr = value & 0xCEFFFFFF;
    	return;
    case RCC_PLLI2SCFGR:
    	s->rcc_plli2scfgr = value & 0x7F007FC0;
    	return;
    case RCC_PLLSAICFGR:
    	s->rcc_pll_saicfgr = value & 0x0F037FC0;
    	return;
    case RCC_DCKCFGR:
    	s->rcc_dckcfgr = value & 0x39F31F1F;
    	return;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Bad offset 0x%"HWADDR_PRIx"\n", __func__, addr);
    }
}

static const MemoryRegionOps stm32f4xx_rcc_ops = {
    .read = stm32f4xx_rcc_read,
    .write = stm32f4xx_rcc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void stm32f4xx_rcc_init(Object *obj)
{
    STM32F4XXRccState *s = STM32F4XX_RCC(obj);

    sysbus_init_irq(SYS_BUS_DEVICE(obj), &s->irq);

    memory_region_init_io(&s->mmio, obj, &stm32f4xx_rcc_ops, s,
                          TYPE_STM32F4XX_RCC, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);
}

static void stm32f4xx_rcc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32f4xx_rcc_reset;
}

static const TypeInfo stm32f4xx_rcc_info = {
    .name          = TYPE_STM32F4XX_RCC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32F4XXRccState),
    .instance_init = stm32f4xx_rcc_init,
    .class_init    = stm32f4xx_rcc_class_init,
};

static void stm32f4xx_rcc_register_types(void)
{
    type_register_static(&stm32f4xx_rcc_info);
}

type_init(stm32f4xx_rcc_register_types)
