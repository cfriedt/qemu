/*
 * STM32F4XX ENET
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

#ifndef HW_STM32F4XX_ENET_H
#define HW_STM32F4XX_ENET_H

#include "hw/sysbus.h"
#include "hw/hw.h"
#include "net/net.h"

#define ETH_MACCR 0x0000
#define ETH_MACFFR 0x0004
#define ETH_MACHTHR 0x0008
#define ETH_MACHTLR 0x000c
#define ETH_MACMIIAR 0x0010
#define ETH_MACMIIDR 0x0014
#define ETH_MACFCR 0x0018
#define ETH_MACVLANTR 0x001c
#define ETH_MACRWUFFR 0x0028
#define ETH_MACPMTCSR 0x002c
#define ETH_MACDBGR 0x0034
#define ETH_MACSR 0x0038
#define ETH_MACIMR 0x003c
#define ETH_MACA0HR 0x0040
#define ETH_MACA0LR 0x0044
#define ETH_MACA1HR 0x0048
#define ETH_MACA1LR 0x004c
#define ETH_MACA2HR 0x0050
#define ETH_MACA2LR 0x0054
#define ETH_MACA3HR 0x0058
#define ETH_MACA3LR 0x005c
#define ETH_MMCCR 0x0100
#define ETH_MMCRIR 0x0104
#define ETH_MMCTIR 0x0108
#define ETH_MMCRIMR 0x010c
#define ETH_MMCTIMR 0x0110
#define ETH_MMCTGFSCCR 0x014c
#define ETH_MMCTGFMSCCR 0x0150
#define ETH_MMCTGFCR 0x0168
#define ETH_MMCRFCECR 0x0194
#define ETH_MMCRGUFCR 0x01c4
#define ETH_PTPTSCR 0x0700
#define ETH_PTPSSIR 0x0704
#define ETH_PTPTSHR 0x0708
#define ETH_PTPTSLR 0x070c
#define ETH_PTPTSHUR 0x0710
#define ETH_PTPTSLUR 0x0714
#define ETH_PTPTSAR 0x0718
#define ETH_PTPTTHR 0x071c
#define ETH_PTPTTLR 0x0720
#define ETH_PTPTSSR 0x0728
#define ETH_PTPPPSCR 0x072c
#define ETH_DMABMR 0x1000
#define ETH_DMATPDR 0x1004
#define ETH_DMARPDR 0x1008
#define ETH_DMARDLAR 0x100c
#define ETH_DMATDLAR 0x1010
#define ETH_DMASR 0x1014
#define ETH_DMAOMR 0x1018
#define ETH_DMAIER 0x101c
#define ETH_DMAMFBOCR 0x1020
#define ETH_DMARSWTR 0x1024
#define ETH_DMACHTDR 0x1048
#define ETH_DMACHRDR 0x104c
#define ETH_DMACHTBAR 0x1050
#define ETH_DMACHRBAR 0x1054

#define TYPE_STM32F4XX_ENET "stm32f4xx-enet"
#define STM32F4XX_ENET(obj) \
    OBJECT_CHECK(STM32F4XXEnetState, (obj), TYPE_STM32F4XX_ENET)

typedef struct {
    /* <private> */
    SysBusDevice parent_obj;

    /* <public> */
    MemoryRegion mmio;

	uint32_t eth_maccr;
	uint32_t eth_macffr;
	uint32_t eth_machthr;
	uint32_t eth_machtlr;
	uint32_t eth_macmiiar;
	uint32_t eth_macmiidr;
	uint32_t eth_macfcr;
	uint32_t eth_macvlantr;
	uint32_t eth_macrwuffr;
	uint32_t eth_macpmtcsr;
	uint32_t eth_macdbgr;
	uint32_t eth_macsr;
	uint32_t eth_macimr;
	uint32_t eth_maca0hr;
	uint32_t eth_maca0lr;
	uint32_t eth_maca1hr;
	uint32_t eth_maca1lr;
	uint32_t eth_maca2hr;
	uint32_t eth_maca2lr;
	uint32_t eth_maca3hr;
	uint32_t eth_maca3lr;
	uint32_t eth_mmccr;
	uint32_t eth_mmcrir;
	uint32_t eth_mmctir;
	uint32_t eth_mmcrimr;
	uint32_t eth_mmctimr;
	uint32_t eth_mmctgfsccr;
	uint32_t eth_mmctgfmsccr;
	uint32_t eth_mmctgfcr;
	uint32_t eth_mmcrfcecr;
	uint32_t eth_mmcrgufcr;
	uint32_t eth_ptptscr;
	uint32_t eth_ptpssir;
	uint32_t eth_ptptshr;
	uint32_t eth_ptptslr;
	uint32_t eth_ptptshur;
	uint32_t eth_ptptslur;
	uint32_t eth_ptptsar;
	uint32_t eth_ptptthr;
	uint32_t eth_ptpttlr;
	uint32_t eth_ptptssr;
	uint32_t eth_ptpppscr;
	uint32_t eth_dmabmr;
	uint32_t eth_dmatpdr;
	uint32_t eth_dmarpdr;
	uint32_t eth_dmardlar;
	uint32_t eth_dmatdlar;
	uint32_t eth_dmasr;
	uint32_t eth_dmaomr;
	uint32_t eth_dmaier;
	uint32_t eth_dmamfbocr;
	uint32_t eth_dmarswtr;
	uint32_t eth_dmachtdr;
	uint32_t eth_dmachrdr;
	uint32_t eth_dmachtbar;
	uint32_t eth_dmachrbar;

    NICState *nic;
    NICConf conf;

    qemu_irq irq;
} STM32F4XXEnetState;

#endif /* HW_STM32F4XX_ENET_H */
