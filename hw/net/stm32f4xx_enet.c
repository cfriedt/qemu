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

#include "qemu/osdep.h"
#include "hw/net/stm32f4xx_enet.h"
#include "qemu/log.h"

#ifndef STM_ENET_ERR_DEBUG
#define STM_ENET_ERR_DEBUG 0
#endif

#define STMF32F4XX_ENET(obj) \
    OBJECT_CHECK(stm32f4xx_enet_state, (obj), TYPE_STMF32F4XX_ENET)

#define DB_PRINT_L(lvl, fmt, args...) do { \
    if (STM_ENET_ERR_DEBUG >= lvl) { \
        qemu_log("%s: " fmt, __func__, ## args); \
    } \
} while (0)

#define DB_PRINT(fmt, args...) DB_PRINT_L(1, fmt, ## args)

static int stm32f4xx_enet_post_load(void *opaque, int version_id)
{
    STM32F4XXEnetState *s = opaque;
    s++;
    s--;
    return -1;
/*
    int i;

    // Sanitize inbound state. Note that next_packet is an index but
    // np is a size; hence their valid upper bounds differ.
    if (s->next_packet >= ARRAY_SIZE(s->rx)) {
        return -1;
    }

    if (s->np > ARRAY_SIZE(s->rx)) {
        return -1;
    }

    for (i = 0; i < ARRAY_SIZE(s->rx); i++) {
        if (s->rx[i].len > ARRAY_SIZE(s->rx[i].data)) {
            return -1;
        }
    }

    if (s->rx_fifo_offset > ARRAY_SIZE(s->rx[0].data) - 4) {
        return -1;
    }

    if (s->tx_fifo_len > ARRAY_SIZE(s->tx_fifo)) {
        return -1;
    }

    return 0;
*/
}

static const VMStateDescription stm32f4xx_enet_vmstate = {
    .name = "stm32f4xx_enet",
    .version_id = 2,
    .minimum_version_id = 2,
    .post_load = stm32f4xx_enet_post_load,
    .fields = (VMStateField[]) {
/*
        VMSTATE_UINT32(ris, STM32F4XXEnetState),
        VMSTATE_UINT32(im, STM32F4XXEnetState),
        VMSTATE_UINT32(rctl, STM32F4XXEnetState),
        VMSTATE_UINT32(tctl, STM32F4XXEnetState),
        VMSTATE_UINT32(thr, STM32F4XXEnetState),
        VMSTATE_UINT32(mctl, STM32F4XXEnetState),
        VMSTATE_UINT32(mdv, STM32F4XXEnetState),
        VMSTATE_UINT32(mtxd, STM32F4XXEnetState),
        VMSTATE_UINT32(mrxd, STM32F4XXEnetState),
        VMSTATE_UINT32(np, STM32F4XXEnetState),
        VMSTATE_UINT32(tx_fifo_len, STM32F4XXEnetState),
        VMSTATE_UINT8_ARRAY(tx_fifo, STM32F4XXEnetState, 2048),
        VMSTATE_STRUCT_ARRAY(rx, STM32F4XXEnetState, 31, 1,
                             vmstate_rx_frame, StellarisEnetRxFrame),
        VMSTATE_UINT32(rx_fifo_offset, STM32F4XXEnetState),
        VMSTATE_UINT32(next_packet, STM32F4XXEnetState),
*/
        VMSTATE_END_OF_LIST()
    }
};

/*
static void stm32f4xx_enet_send(STM32F4XXEnetState *s)
{
    int framelen = stm32f4xx_txpacket_datalen(s);

    // Ethernet header is in the FIFO but not in the datacount.
    // We don't implement explicit CRC, so just ignore any
    // CRC value in the FIFO.

    framelen += 14;
    if ((s->tctl & SE_TCTL_PADEN) && framelen < 60) {
        memset(&s->tx_fifo[framelen + 2], 0, 60 - framelen);
        framelen = 60;
    }
    // This MIN will have no effect unless the FIFO data is corrupt
    // (eg bad data from an incoming migration); otherwise the check
    // on the datalen at the start of writing the data into the FIFO
    // will have caught this. Silently write a corrupt half-packet,
    // which is what the hardware does in FIFO underrun situations.
    framelen = MIN(framelen, ARRAY_SIZE(s->tx_fifo) - 2);
    qemu_send_packet(qemu_get_queue(s->nic), s->tx_fifo + 2, framelen);
    s->tx_fifo_len = 0;
    s->ris |= SE_INT_TXEMP;
    stm32f4xx_enet_update(s);
    DPRINTF("Done TX\n");
}
*/

static ssize_t stm32f4xx_enet_receive(NetClientState *nc, const uint8_t *buf, size_t size)
{

	STM32F4XXEnetState *s = qemu_get_nic_opaque(nc);
	s++;
	s--;
/*
    int n;
    uint8_t *p;
    uint32_t crc;
	*/
    return -1;

/*
    if ((s->rctl & SE_RCTL_RXEN) == 0)
        return -1;
    if (s->np >= 31) {
        return 0;
    }

    DPRINTF("Received packet len=%zu\n", size);
    n = s->next_packet + s->np;
    if (n >= 31)
        n -= 31;

    if (size >= sizeof(s->rx[n].data) - 6) {
        // If the packet won't fit into the
        // emulated 2K RAM, this is reported
        // as a FIFO overrun error.
        s->ris |= SE_INT_FOV;
        stm32f4xx_enet_update(s);
        return -1;
    }

    s->np++;
    s->rx[n].len = size + 6;
    p = s->rx[n].data;
    *(p++) = (size + 6);
    *(p++) = (size + 6) >> 8;
    memcpy (p, buf, size);
    p += size;
    crc = crc32(~0, buf, size);
    *(p++) = crc;
    *(p++) = crc >> 8;
    *(p++) = crc >> 16;
    *(p++) = crc >> 24;
    // Clear the remaining bytes in the last word.
    if ((size & 3) != 2) {
        memset(p, 0, (6 - size) & 3);
    }

    s->ris |= SE_INT_RX;
    stm32f4xx_enet_update(s);

    return size;
*/
}

static void stm32f4xx_enet_reset(STM32F4XXEnetState *s)
{
	s->eth_maccr = 0x00008000;
	s->eth_macffr = 0x00000000;
	s->eth_machthr = 0x00000000;
	s->eth_machtlr = 0x00000000;
	s->eth_macmiiar = 0x00000000;
	s->eth_macmiidr = 0x00000000;
	s->eth_macfcr = 0x00000000;
	s->eth_macvlantr = 0x00000000;
	s->eth_macrwuffr = 0x00000000;
	s->eth_macpmtcsr = 0x00000000;
	s->eth_macdbgr = 0x00000000;
	s->eth_macsr = 0x00000000;
	s->eth_macimr = 0x00000000;
	s->eth_maca0hr = 0x8000ffff;
	s->eth_maca0lr = 0xffffffff;
	s->eth_maca1hr = 0x0000ffff;
	s->eth_maca1lr = 0xffffffff;
	s->eth_maca2hr = 0x0000ffff;
	s->eth_maca2lr = 0xffffffff;
	s->eth_maca3hr = 0x0000ffff;
	s->eth_maca3lr = 0xffffffff;
	s->eth_mmccr = 0x00000000;
	s->eth_mmcrir = 0x00000000;
	s->eth_mmctir = 0x00000000;
	s->eth_mmcrimr = 0x00000000;
	s->eth_mmctimr = 0x00000000;
	s->eth_mmctgfsccr = 0x00000000;
	s->eth_mmctgfmsccr = 0x00000000;
	s->eth_mmctgfcr = 0x00000000;
	s->eth_mmcrfcecr = 0x00000000;
	s->eth_mmcrgufcr = 0x00000000;
	s->eth_ptptscr = 0x00002000;
	s->eth_ptpssir = 0x00000000;
	s->eth_ptptshr = 0x00000000;
	s->eth_ptptslr = 0x00000000;
	s->eth_ptptshur = 0x00000000;
	s->eth_ptptslur = 0x00000000;
	s->eth_ptptsar = 0x00000000;
	s->eth_ptptthr = 0x00000000;
	s->eth_ptpttlr = 0x00000000;
	s->eth_ptptssr = 0x00000000;
	s->eth_ptpppscr = 0x00000000;
	s->eth_dmabmr = 0x00020101;
	s->eth_dmatpdr = 0x00000000;
	s->eth_dmarpdr = 0x00000000;
	s->eth_dmardlar = 0x00000000;
	s->eth_dmatdlar = 0x00000000;
	s->eth_dmasr = 0x00000000;
	s->eth_dmaomr = 0x00000000;
	s->eth_dmaier = 0x00000000;
	s->eth_dmamfbocr = 0x00000000;
	s->eth_dmarswtr = 0x00000000;
	s->eth_dmachtdr = 0x00000000;
	s->eth_dmachrdr = 0x00000000;
	s->eth_dmachtbar = 0x00000000;
	s->eth_dmachrbar = 0x00000000;
}

static uint64_t stm32f4xx_enet_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
    STM32F4XXEnetState *s = opaque;

    DB_PRINT("0x%"HWADDR_PRIx"\n", addr);

    switch (addr) {
	case ETH_MACCR: return s->eth_maccr & 0x02cf7efc;
	case ETH_MACFFR: return s->eth_macffr & 0x800007ff;
	case ETH_MACHTHR: return s->eth_machthr & 0xffffffff;
	case ETH_MACHTLR: return s->eth_machtlr & 0xffffffff;
	case ETH_MACMIIAR: return s->eth_macmiiar & 0x0000ffdf;
	case ETH_MACMIIDR: return s->eth_macmiidr & 0x0000ffff;
	case ETH_MACFCR: return s->eth_macfcr & 0xffff00bf;
	case ETH_MACVLANTR: return s->eth_macvlantr & 0x0001ffff;
	case ETH_MACRWUFFR: return s->eth_macrwuffr & 0xffffffff;
	case ETH_MACPMTCSR: return s->eth_macpmtcsr & 0x000002c7;
	case ETH_MACDBGR: return s->eth_macdbgr & 0x037f037e;
	case ETH_MACSR: return s->eth_macsr & 0x00000278;
	case ETH_MACIMR: return s->eth_macimr & 0x00000208;
	case ETH_MACA0HR: return s->eth_maca0hr & 0x80000ffff;
	case ETH_MACA0LR: return s->eth_maca0lr & 0xffffffff;
	case ETH_MACA1HR: return s->eth_maca1hr & 0xff00ffffff;
	case ETH_MACA1LR: return s->eth_maca1lr & 0xffffffff;
	case ETH_MACA2HR: return s->eth_maca2hr & 0xff00ffffff;
	case ETH_MACA2LR: return s->eth_maca2lr & 0xffffffff;
	case ETH_MACA3HR: return s->eth_maca3hr & 0xff00ffffff;
	case ETH_MACA3LR: return s->eth_maca3lr & 0xffffffff;
	case ETH_MMCCR: return s->eth_mmccr & 0x0000003f;
	case ETH_MMCRIR: return s->eth_mmcrir & 0x000200c0;
	case ETH_MMCTIR: return s->eth_mmctir & 0x0020c000;
	case ETH_MMCRIMR: return s->eth_mmcrimr & 0x000200c0;
	case ETH_MMCTIMR: return s->eth_mmctimr & 0x0010c000;
	case ETH_MMCTGFSCCR: return s->eth_mmctgfsccr & 0xffffffff;
	case ETH_MMCTGFMSCCR: return s->eth_mmctgfmsccr & 0xffffffff;
	case ETH_MMCTGFCR: return s->eth_mmctgfcr & 0xffffffff;
	case ETH_MMCRFCECR: return s->eth_mmcrfcecr & 0xffffffff;
	case ETH_MMCRGUFCR: return s->eth_mmcrgufcr & 0xffffffff;
	case ETH_PTPTSCR: return s->eth_ptptscr & 0x0003ff3f;
	case ETH_PTPSSIR: return s->eth_ptpssir & 0x000000ff;
	case ETH_PTPTSHR: return s->eth_ptptshr & 0xffffffff;
	case ETH_PTPTSLR: return s->eth_ptptslr & 0xffffffff;
	case ETH_PTPTSHUR: return s->eth_ptptshur & 0xffffffff;
	case ETH_PTPTSLUR: return s->eth_ptptslur & 0xffffffff;
	case ETH_PTPTSAR: return s->eth_ptptsar & 0xffffffff;
	case ETH_PTPTTHR: return s->eth_ptptthr & 0xffffffff;
	case ETH_PTPTTLR: return s->eth_ptpttlr & 0xffffffff;
	case ETH_PTPTSSR: return s->eth_ptptssr & 0x00000003;
	case ETH_PTPPPSCR: return s->eth_ptpppscr & 0x0000000f;
	case ETH_DMABMR: return s->eth_dmabmr & 0x07ffffff;
	case ETH_DMATPDR: return s->eth_dmatpdr & 0xffffffff;
	case ETH_DMARPDR: return s->eth_dmarpdr & 0xffffffff;
	case ETH_DMARDLAR: return s->eth_dmardlar & 0xffffffff;
	case ETH_DMATDLAR: return s->eth_dmatdlar & 0xffffffff;
	case ETH_DMASR: return s->eth_dmasr & 0x3bffe7ff;
	case ETH_DMAOMR: return s->eth_dmaomr & 0x0731e0de;
	case ETH_DMAIER: return s->eth_dmaier & 0x0001e7ff;
	case ETH_DMAMFBOCR: return s->eth_dmamfbocr & 0x1fffffff;
	case ETH_DMARSWTR: return s->eth_dmarswtr & 0x000000ff;
	case ETH_DMACHTDR: return s->eth_dmachtdr & 0xffffffff;
	case ETH_DMACHRDR: return s->eth_dmachrdr & 0xffffffff;
	case ETH_DMACHTBAR: return s->eth_dmachtbar & 0xffffffff;
	case ETH_DMACHRBAR: return s->eth_dmachrbar & 0xffffffff;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Bad offset 0x%"HWADDR_PRIx"\n", __func__, addr);
        return 0;
    }

    return 0;
}

static void stm32f4xx_enet_write(void *opaque, hwaddr addr,
                       uint64_t val64, unsigned int size)
{
    STM32F4XXEnetState *s = opaque;
    uint32_t value = val64;

    DB_PRINT("0x%x, 0x%"HWADDR_PRIx"\n", value, addr);

    // stm32f4xx_enet_send(s);

    switch (addr) {
	case ETH_MACCR: s->eth_maccr = value & 0x02cf7efc; break;
	case ETH_MACFFR: s->eth_macffr = value & 0x800007ff; break;
	case ETH_MACHTHR: s->eth_machthr = value & 0xffffffff; break;
	case ETH_MACHTLR: s->eth_machtlr = value & 0xffffffff; break;
	case ETH_MACMIIAR: s->eth_macmiiar = value & 0x0000ffdf; break;
	case ETH_MACMIIDR: s->eth_macmiidr = value & 0x0000ffff; break;
	case ETH_MACFCR: s->eth_macfcr = value & 0xffff00bf; break;
	case ETH_MACVLANTR: s->eth_macvlantr = value & 0x0001ffff; break;
	case ETH_MACRWUFFR: s->eth_macrwuffr = value & 0xffffffff; break;
	case ETH_MACPMTCSR: s->eth_macpmtcsr = value & 0x800002c7; break;
	case ETH_MACDBGR: s->eth_macdbgr = value & 0x00000000; break;
	case ETH_MACSR: s->eth_macsr = value & 0x00000000; break;
	case ETH_MACIMR: s->eth_macimr = value & 0x00000208; break;
	case ETH_MACA0HR: s->eth_maca0hr = value & 0x00000ffff; break;
	case ETH_MACA0LR: s->eth_maca0lr = value & 0xffffffff; break;
	case ETH_MACA1HR: s->eth_maca1hr = value & 0xff00ffffff; break;
	case ETH_MACA1LR: s->eth_maca1lr = value & 0xffffffff; break;
	case ETH_MACA2HR: s->eth_maca2hr = value & 0xff00ffffff; break;
	case ETH_MACA2LR: s->eth_maca2lr = value & 0xffffffff; break;
	case ETH_MACA3HR: s->eth_maca3hr = value & 0xff00ffffff; break;
	case ETH_MACA3LR: s->eth_maca3lr = value & 0xffffffff; break;
	case ETH_MMCCR: s->eth_mmccr = value & 0x0000003f; break;
	case ETH_MMCRIR: s->eth_mmcrir = value & 0x00000000; break;
	case ETH_MMCTIR: s->eth_mmctir = value & 0x00000000; break;
	case ETH_MMCRIMR: s->eth_mmcrimr = value & 0x000200c0; break;
	case ETH_MMCTIMR: s->eth_mmctimr = value & 0x0010c000; break;
	case ETH_MMCTGFSCCR: s->eth_mmctgfsccr = value & 0x00000000; break;
	case ETH_MMCTGFMSCCR: s->eth_mmctgfmsccr = value & 0x00000000; break;
	case ETH_MMCTGFCR: s->eth_mmctgfcr = value & 0x00000000; break;
	case ETH_MMCRFCECR: s->eth_mmcrfcecr = value & 0x00000000; break;
	case ETH_MMCRGUFCR: s->eth_mmcrgufcr = value & 0x00000000; break;
	case ETH_PTPTSCR: s->eth_ptptscr = value & 0x0003ff3f; break;
	case ETH_PTPSSIR: s->eth_ptpssir = value & 0x000000ff; break;
	case ETH_PTPTSHR: s->eth_ptptshr = value & 0x00000000; break;
	case ETH_PTPTSLR: s->eth_ptptslr = value & 0x00000000; break;
	case ETH_PTPTSHUR: s->eth_ptptshur = value & 0xffffffff; break;
	case ETH_PTPTSLUR: s->eth_ptptslur = value & 0xffffffff; break;
	case ETH_PTPTSAR: s->eth_ptptsar = value & 0xffffffff; break;
	case ETH_PTPTTHR: s->eth_ptptthr = value & 0xffffffff; break;
	case ETH_PTPTTLR: s->eth_ptpttlr = value & 0xffffffff; break;
	case ETH_PTPTSSR: s->eth_ptptssr = value & 0x00000000; break;
	case ETH_PTPPPSCR: s->eth_ptpppscr = value & 0x0000000f; break;
	case ETH_DMABMR: s->eth_dmabmr = value & 0x07ffffff; break;
	case ETH_DMATPDR: s->eth_dmatpdr = value & 0xffffffff; break;
	case ETH_DMARPDR: s->eth_dmarpdr = value & 0xffffffff; break;
	case ETH_DMARDLAR: s->eth_dmardlar = value & 0xffffffff; break;
	case ETH_DMATDLAR: s->eth_dmatdlar = value & 0xffffffff; break;
	case ETH_DMASR: s->eth_dmasr = value & 0x0001e7ff; break;
	case ETH_DMAOMR: s->eth_dmaomr = value & 0x0731e0de; break;
	case ETH_DMAIER: s->eth_dmaier = value & 0x0001e7ff; break;
	case ETH_DMAMFBOCR: s->eth_dmamfbocr = value & 0x00000000; break;
	case ETH_DMARSWTR: s->eth_dmarswtr = value & 0x000000ff; break;
	case ETH_DMACHTDR: s->eth_dmachtdr = value & 0x00000000; break;
	case ETH_DMACHRDR: s->eth_dmachrdr = value & 0x00000000; break;
	case ETH_DMACHTBAR: s->eth_dmachtbar = value & 0x00000000; break;
	case ETH_DMACHRBAR: s->eth_dmachrbar = value & 0x00000000; break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Bad offset 0x%"HWADDR_PRIx"\n", __func__, addr);
    }
}

static const MemoryRegionOps stm32f4xx_enet_ops = {
    .read = stm32f4xx_enet_read,
    .write = stm32f4xx_enet_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static NetClientInfo stm32f4xx_enet_net_info = {
    .type = NET_CLIENT_DRIVER_NIC,
    .size = sizeof(NICState),
    .receive = stm32f4xx_enet_receive,
};

static int stm32f4xx_enet_init(SysBusDevice *sbd)
{
    DeviceState *dev = DEVICE(sbd);
    STM32F4XXEnetState *s = STM32F4XX_ENET(dev);

    memory_region_init_io(&s->mmio, OBJECT(s), &stm32f4xx_enet_ops, s,
                          "stm32f4xx_enet", 0x400);
    sysbus_init_mmio(sbd, &s->mmio);
    sysbus_init_irq(sbd, &s->irq);
    qemu_macaddr_default_if_unset(&s->conf.macaddr);

    s->nic = qemu_new_nic(&stm32f4xx_enet_net_info, &s->conf,
                          object_get_typename(OBJECT(dev)), dev->id, s);
    qemu_format_nic_info_str(qemu_get_queue(s->nic), s->conf.macaddr.a);

    stm32f4xx_enet_reset(s);
    return 0;
}

static Property stm32f4xx_enet_properties[] = {
    DEFINE_NIC_PROPERTIES(STM32F4XXEnetState, conf),
    DEFINE_PROP_END_OF_LIST(),
};

static void stm32f4xx_enet_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init = stm32f4xx_enet_init;
    dc->props = stm32f4xx_enet_properties;
    dc->vmsd = &stm32f4xx_enet_vmstate;
}

static const TypeInfo stm32f4xx_enet_info = {
    .name          = TYPE_STM32F4XX_ENET,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32F4XXEnetState),
    .class_init    = stm32f4xx_enet_class_init,
};

static void stm32f4xx_enet_register_types(void)
{
    type_register_static(&stm32f4xx_enet_info);
}

type_init(stm32f4xx_enet_register_types)
