/*
 * STM32F479 SoC
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
#include "qapi/error.h"
#include "qemu-common.h"
#include "hw/arm/arm.h"
#include "exec/address-spaces.h"
#include "hw/arm/stm32f479_soc.h"

#include "qemu/config-file.h"
#include "qemu/option.h"

static const uint32_t timer_addr[STM_NUM_TIMERS] = {0x40010000, 0x40000000,
    0x40000400, 0x40000800, 0x40000c00, 0x40001000, 0x40001400, 0x40010400,
	0x40014000, 0x40014400, 0x40014800, 0x40001800, 0x40001C00, 0x40002000};
static const uint32_t usart_addr[STM_NUM_USARTS] = { 0x40011000, 0x40004400,
    0x40004800, 0x40004C00, 0x40005000, 0x40011400, 0x40007800, 0x40007C00 };
static const uint32_t adc_addr[STM_NUM_ADCS] = { 0x40012000, 0x40012100,
    0x40012200 };
static const uint32_t spi_addr[STM_NUM_SPIS] = { 0x40013000, 0x40003800,
    0x40003C00, 0x40013400, 0x40015000, 0x40015400 };

static const int timer_irq[STM_NUM_TIMERS] = {27, 28, 29, 30, 50, 54, 55, 46,
    24, 25, 26, 43, 44, 45};
static const int usart_irq[STM_NUM_USARTS] = {37, 38, 39, 52, 53, 71, 82, 83};
#define ADC_IRQ 18
static const int spi_irq[STM_NUM_SPIS] = {35, 36, 51, 84, 85, 86};

static void stm32f479_soc_initfn(Object *obj)
{
    STM32F479State *s = STM32F479_SOC(obj);
    int i;

    sysbus_init_child_obj(obj, "armv7m", &s->armv7m, sizeof(s->armv7m),
                          TYPE_ARMV7M);

    sysbus_init_child_obj(obj, "syscfg", &s->syscfg, sizeof(s->syscfg),
                          TYPE_STM32F4XX_SYSCFG);

    sysbus_init_child_obj(obj, "fmc", &s->fmc, sizeof(s->fmc),
                          TYPE_STM32F4XX_FMC);

    sysbus_init_child_obj(obj, "rcc", &s->rcc, sizeof(s->rcc),
                          TYPE_STM32F4XX_RCC);

    for (i = 0; i < STM_NUM_TIMERS; i++) {
        sysbus_init_child_obj(obj, "timer[*]", &s->timer[i],
                              sizeof(s->timer[i]), TYPE_STM32F4XX_TIMER);
    }

    for (i = 0; i < STM_NUM_USARTS; i++) {
        sysbus_init_child_obj(obj, "usart[*]", &s->usart[i],
                              sizeof(s->usart[i]), TYPE_STM32F4XX_USART);
    }

    s->adc_irqs = OR_IRQ(object_new(TYPE_OR_IRQ));

    for (i = 0; i < STM_NUM_ADCS; i++) {
        sysbus_init_child_obj(obj, "adc[*]", &s->adc[i], sizeof(s->adc[i]),
                              TYPE_STM32F4XX_ADC);
    }

    for (i = 0; i < STM_NUM_SPIS; i++) {
        sysbus_init_child_obj(obj, "spi[*]", &s->spi[i], sizeof(s->spi[i]),
                              TYPE_STM32F4XX_SPI);
    }
}

static void stm32f479_soc_realize(DeviceState *dev_soc, Error **errp)
{
    STM32F479State *s = STM32F479_SOC(dev_soc);
    DeviceState *dev, *armv7m;
    SysBusDevice *busdev;
    Error *err = NULL;
    QemuOpts *opts;
    uint64_t sdram_size;
    int i;

    /* With "-m 0", qemu defaults to 128 MiB of SDRAM for memory.size */
    opts = qemu_find_opts_singleton("memory");
    sdram_size = qemu_opt_get_size(opts, "size", SDRAM_SIZE_MAX);
    sdram_size = sdram_size >= SDRAM_SIZE_MAX ? SDRAM_SIZE_MAX : sdram_size;

    MemoryRegion *system_memory = get_system_memory();
    MemoryRegion *sram = g_new(MemoryRegion, 1);
    MemoryRegion *flash = g_new(MemoryRegion, 1);
    MemoryRegion *sdram = g_new(MemoryRegion, 1);
    MemoryRegion *sdram_alias = g_new(MemoryRegion, 1);

    memory_region_init_ram(flash, NULL, "STM32F479.flash", FLASH_SIZE,
                           &error_fatal);

    memory_region_set_readonly(flash, true);

    memory_region_add_subregion(system_memory, FLASH_BASE_ADDRESS, flash);

    memory_region_init_ram(sram, NULL, "STM32F479.sram", SRAM_SIZE,
                           &error_fatal);
    memory_region_add_subregion(system_memory, SRAM_BASE_ADDRESS, sram);

    memory_region_init_ram(sdram, NULL, "STM32F479.sdram", sdram_size,
                           &error_fatal);
    memory_region_init_alias(sdram_alias, NULL, "STM32F479.sdram.alias",
                             sdram, SDRAM_ALIAS_BASE_ADDRESS,
                             sdram_size > SDRAM_ALIAS_SIZE_MAX
                             ? SDRAM_ALIAS_SIZE_MAX : sdram_size );

    memory_region_add_subregion(system_memory, SDRAM_BASE_ADDRESS, sdram);
    memory_region_add_subregion(system_memory, SDRAM_ALIAS_BASE_ADDRESS,
                                sdram_alias);

    armv7m = DEVICE(&s->armv7m);
    qdev_prop_set_uint32(armv7m, "num-irq", 92);
    qdev_prop_set_string(armv7m, "cpu-type", s->cpu_type);
    object_property_set_link(OBJECT(&s->armv7m), OBJECT(get_system_memory()),
                                     "memory", &error_abort);
    object_property_set_bool(OBJECT(&s->armv7m), true, "realized", &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }

    /* System configuration controller */
    dev = DEVICE(&s->syscfg);
    object_property_set_bool(OBJECT(&s->syscfg), true, "realized", &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }
    busdev = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(busdev, 0, 0x40013800);
    sysbus_connect_irq(busdev, 0, qdev_get_gpio_in(armv7m, 71));

    /* Flexible memory controller */
    dev = DEVICE(&s->fmc);
    object_property_set_bool(OBJECT(&s->fmc), true, "realized", &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }
    busdev = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(busdev, 0, 0xA0000000);
    //sysbus_connect_irq(busdev, 0, qdev_get_gpio_in(armv7m, 71));

    /* Reset and clock control */
    dev = DEVICE(&s->rcc);
    object_property_set_bool(OBJECT(&s->rcc), true, "realized", &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }
    busdev = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(busdev, 0, 0x40023800);
    //sysbus_connect_irq(busdev, 0, qdev_get_gpio_in(armv7m, 71));

    /* Timer 2 to 5 */
    for (i = 0; i < STM_NUM_TIMERS; i++) {
        dev = DEVICE(&(s->timer[i]));
        qdev_prop_set_uint64(dev, "clock-frequency", 1000000000);
        object_property_set_bool(OBJECT(&s->timer[i]), true, "realized", &err);
        if (err != NULL) {
            error_propagate(errp, err);
            return;
        }
        busdev = SYS_BUS_DEVICE(dev);
        sysbus_mmio_map(busdev, 0, timer_addr[i]);
        sysbus_connect_irq(busdev, 0, qdev_get_gpio_in(armv7m, timer_irq[i]));
    }

    /* Attach UART (uses USART registers) and USART controllers */
    for (i = 0; i < STM_NUM_USARTS; i++) {
        dev = DEVICE(&(s->usart[i]));
        qdev_prop_set_chr(dev, "chardev", serial_hd(i));
        object_property_set_bool(OBJECT(&s->usart[i]), true, "realized", &err);
        if (err != NULL) {
            error_propagate(errp, err);
            return;
        }
        busdev = SYS_BUS_DEVICE(dev);
        sysbus_mmio_map(busdev, 0, usart_addr[i]);
        sysbus_connect_irq(busdev, 0, qdev_get_gpio_in(armv7m, usart_irq[i]));
    }

    /* ADC 1 to 3 */
    object_property_set_int(OBJECT(s->adc_irqs), STM_NUM_ADCS,
                            "num-lines", &err);
    object_property_set_bool(OBJECT(s->adc_irqs), true, "realized", &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }
    qdev_connect_gpio_out(DEVICE(s->adc_irqs), 0,
                          qdev_get_gpio_in(armv7m, ADC_IRQ));

    for (i = 0; i < STM_NUM_ADCS; i++) {
        dev = DEVICE(&(s->adc[i]));
        object_property_set_bool(OBJECT(&s->adc[i]), true, "realized", &err);
        if (err != NULL) {
            error_propagate(errp, err);
            return;
        }
        busdev = SYS_BUS_DEVICE(dev);
        sysbus_mmio_map(busdev, 0, adc_addr[i]);
        sysbus_connect_irq(busdev, 0,
                           qdev_get_gpio_in(DEVICE(s->adc_irqs), i));
    }

    /* SPI 1 and 2 */
    for (i = 0; i < STM_NUM_SPIS; i++) {
        dev = DEVICE(&(s->spi[i]));
        object_property_set_bool(OBJECT(&s->spi[i]), true, "realized", &err);
        if (err != NULL) {
            error_propagate(errp, err);
            return;
        }
        busdev = SYS_BUS_DEVICE(dev);
        sysbus_mmio_map(busdev, 0, spi_addr[i]);
        sysbus_connect_irq(busdev, 0, qdev_get_gpio_in(armv7m, spi_irq[i]));
    }
}

static Property stm32f479_soc_properties[] = {
    DEFINE_PROP_STRING("cpu-type", STM32F479State, cpu_type),
    DEFINE_PROP_END_OF_LIST(),
};

static void stm32f479_soc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = stm32f479_soc_realize;
    dc->props = stm32f479_soc_properties;
}

static const TypeInfo stm32f479_soc_info = {
    .name          = TYPE_STM32F479_SOC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32F479State),
    .instance_init = stm32f479_soc_initfn,
    .class_init    = stm32f479_soc_class_init,
};

static void stm32f479_soc_types(void)
{
    type_register_static(&stm32f479_soc_info);
}

type_init(stm32f479_soc_types)
