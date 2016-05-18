/*
 * MimasV2 board description
 *
 * Copyright (c) 2016 Christopher Friedt
 *
 * Based on SHIX 2.0 board description by
 * Copyright (c) 2005 Samuel Tardieu
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
/*
   Numato Mimas v2 board
   http://numato.com/mimas-v2-spartan-6-fpga-development-board-with-ddr-sdram/
*/
#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu-common.h"
#include "cpu.h"
#include "hw/hw.h"
#include "hw/sh4/sh.h"
#include "sysemu/sysemu.h"
#include "sysemu/qtest.h"
#include "hw/boards.h"
#include "hw/loader.h"
#include "exec/address-spaces.h"
#include "qemu/error-report.h"

#define BIOS_FILENAME "mimasv2_bios.bin"
#define BIOS_ADDRESS 0xA0000000

#define SRAM_BASE 0x00000000
#define SRAM_SIZE 0x00008000

#define DRAM_BASE    0x01000000
#define DRAM_SIZE    0x04000000

static void mimasv2_init(MachineState *machine)
{
    const char *cpu_model = machine->cpu_model;
    int ret;
    SuperHCPU *cpu;
    struct SH7750State *s;
    MemoryRegion *sysmem = get_system_memory();
    MemoryRegion *sram = g_new(MemoryRegion, 1);
    MemoryRegion *dram = g_new(MemoryRegion, 1);
    
    if (!cpu_model)
        cpu_model = "any";

    cpu = cpu_sh4_init(cpu_model);
    if (cpu == NULL) {
        fprintf(stderr, "Unable to find CPU definition\n");
        exit(1);
    }

    /* Allocate memory space */
    memory_region_init_ram(sram, NULL, "mimasv2.sram", SRAM_SIZE, &error_fatal);
    vmstate_register_ram_global(sram);
    memory_region_add_subregion(sysmem, SRAM_BASE, sram);
    memory_region_init_ram(&dram[0], NULL, "mimasv2.dram", DRAM_SIZE,
                           &error_fatal);
    vmstate_register_ram_global(&dram[0]);
    memory_region_add_subregion(sysmem, DRAM_BASE, &dram[0]);

    if (bios_name == NULL) {
        bios_name = BIOS_FILENAME;
    }
    ret = load_image_targphys(bios_name, SRAM_BASE, SRAM_SIZE);
    if (ret < 0 && !qtest_enabled()) {
        error_report("Could not load MimasV2 bios '%s'", bios_name);
        exit(1);
    }

    // XXX: @CF: read-in the initial program counter and initial stack pointer from ROM
    cpu->env.pc = ntohl( *( (uint32_t *) rom_ptr( SRAM_BASE ) ) );
    cpu->env.gregs[ 15 ] = ntohl( *( (uint32_t *) rom_ptr( SRAM_BASE + sizeof(long) ) ) );
    cpu->env.features |= SH_FEATURE_CASL;

    /* Register peripherals */
    s = sh7750_init(cpu, sysmem);
    /* XXXXX Check success */
    //tc58128_init(s, "mimasv2_linux_nand.bin", NULL);
}

static void mimasv2_machine_init(MachineClass *mc)
{
    mc->desc = "mimasv2 card";
    mc->init = mimasv2_init;
    mc->is_default = 1;
}

DEFINE_MACHINE("mimasv2", mimasv2_machine_init)
