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
#include "hw/net/mii.h"
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

static const char *stm32f4xx_enet_reg_to_string( uint16_t offs ) {
#if STM_ENET_ERR_DEBUG >= 1
	struct istr {
		unsigned i;
		const char *s;
	};
#define _x( y ) { .i = ETH_ ## y, .s = "ETH_" #y, }
	static const struct istr map[] = {
		_x( MACCR ),
		_x( MACFFR ),
		_x( MACHTHR ),
		_x( MACHTLR ),
		_x( MACMIIAR ),
		_x( MACMIIDR ),
		_x( MACFCR ),
		_x( MACVLANTR ),
		_x( MACRWUFFR ),
		_x( MACPMTCSR ),
		_x( MACDBGR ),
		_x( MACSR ),
		_x( MACIMR ),
		_x( MACA0HR ),
		_x( MACA0LR ),
		_x( MACA1HR ),
		_x( MACA1LR ),
		_x( MACA2HR ),
		_x( MACA2LR ),
		_x( MACA3HR ),
		_x( MACA3LR ),
		_x( MMCCR ),
		_x( MMCRIR ),
		_x( MMCTIR ),
		_x( MMCRIMR ),
		_x( MMCTIMR ),
		_x( MMCTGFSCCR ),
		_x( MMCTGFMSCCR ),
		_x( MMCTGFCR ),
		_x( MMCRFCECR ),
		_x( MMCRGUFCR ),
		_x( PTPTSCR ),
		_x( PTPSSIR ),
		_x( PTPTSHR ),
		_x( PTPTSLR ),
		_x( PTPTSHUR ),
		_x( PTPTSLUR ),
		_x( PTPTSAR ),
		_x( PTPTTHR ),
		_x( PTPTTLR ),
		_x( PTPTSSR ),
		_x( PTPPPSCR ),
		_x( DMABMR ),
		_x( DMATPDR ),
		_x( DMARPDR ),
		_x( DMARDLAR ),
		_x( DMATDLAR ),
		_x( DMASR ),
		_x( DMAOMR ),
		_x( DMAIER ),
		_x( DMAMFBOCR ),
		_x( DMARSWTR ),
		_x( DMACHTDR ),
		_x( DMACHRDR ),
		_x( DMACHTBAR ),
		_x( DMACHRBAR ),
	};
#undef _x

	size_t i;

	for( i = 0; i < ARRAY_SIZE( map ); i++ ) {
		if ( offs == map[ i ].i ) {
			return map[ i ].s;
		}
	}
#endif

	return "";
}

static const char *stm32f4xx_mii_reg_to_string( uint8_t offs ) {
#if STM_ENET_ERR_DEBUG >= 1
	struct istr {
		unsigned i;
		const char *s;
	};
#define _x( y ) { .i = MII_ ## y, .s = "MII_" #y, }
	static const struct istr map[] = {
		_x( BMCR ),
		_x( BMSR ),
		_x( PHYID1 ),
		_x( PHYID2 ),
		_x( ANAR ),
		_x( ANLPAR ),
		_x( ANER ),
		_x( ANNP ),
		_x( ANLPRNP ),
		_x( CTRL1000 ),
		_x( STAT1000 ),
		_x( MDDACR ),
		_x( MDDAADR ),
		_x( EXTSTAT ),
	};
#undef _x

	size_t i;

	for( i = 0; i < ARRAY_SIZE( map ); i++ ) {
		if ( offs == map[ i ].i ) {
			return map[ i ].s;
		}
	}
#endif

	return "";
}

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
static void stm32f4xx_enet_send(NetClientState *nc) {

	STM32F4XXEnetState *s = qemu_get_nic_opaque(nc);

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


static void stm32f4xx_mii_set_link( STM32F4XXEnetState *s, bool link_ok ) {
    if (link_ok) {
        s->mii[MII_BMSR] |= MII_BMSR_LINK_ST;
        s->mii[MII_ANLPAR] |= MII_ANLPAR_TXFD | MII_ANLPAR_TX |
            MII_ANLPAR_10FD | MII_ANLPAR_10 | MII_ANLPAR_CSMACD;
    } else {
        s->mii[MII_BMSR] &= ~MII_BMSR_LINK_ST;
        s->mii[MII_ANLPAR] &= 0x01ff;
    }
    s->link_ok = link_ok;
}

static void stm32f4xx_mii_reset( STM32F4XXEnetState *s ) {

	DB_PRINT( "" );

    memset(s->mii, 0, sizeof(s->mii));
    s->mii[MII_BMCR] = MII_BMCR_AUTOEN;
    s->mii[MII_BMSR] = MII_BMSR_100TX_FD | MII_BMSR_100TX_HD |
        MII_BMSR_10T_FD | MII_BMSR_10T_HD | MII_BMSR_MFPS |
        MII_BMSR_AN_COMP | MII_BMSR_AUTONEG;
    s->mii[MII_PHYID1] = RTL8201CP_PHYID1;
    s->mii[MII_PHYID2] = RTL8201CP_PHYID2;
    s->mii[MII_ANAR] = MII_ANAR_TXFD | MII_ANAR_TX |
        MII_ANAR_10FD | MII_ANAR_10 | MII_ANAR_CSMACD;
    stm32f4xx_mii_set_link(s, s->link_ok);
}

static void stm32f4xx_enet_set_link_status(NetClientState *nc) {
	STM32F4XXEnetState *s = qemu_get_nic_opaque(nc);

//    if (GET_REGBIT(s, MIICOMMAND, SCANSTAT)) {
//        SET_REGFIELD(s, MIISTATUS, LINKFAIL, nc->link_down);
//    }
    stm32f4xx_mii_set_link( s, !nc->link_down );
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

    s->mii[MII_BMCR] = MII_BMCR_AUTOEN;
    s->mii[MII_BMSR] = MII_BMSR_100TX_FD | MII_BMSR_100TX_HD |
        MII_BMSR_10T_FD | MII_BMSR_10T_HD | MII_BMSR_MFPS |
        MII_BMSR_AN_COMP | MII_BMSR_AUTONEG;
    s->mii[MII_PHYID1] = RTL8201CP_PHYID1;
    s->mii[MII_PHYID2] = RTL8201CP_PHYID2;
    s->mii[MII_ANAR] = MII_ANAR_TXFD | MII_ANAR_TX |
        MII_ANAR_10FD | MII_ANAR_10 | MII_ANAR_CSMACD;

    stm32f4xx_mii_reset( s );

    stm32f4xx_mii_set_link( s, false );

    DB_PRINT("mii phy id: %04x%04x\n", s->mii[MII_PHYID1], s->mii[MII_PHYID2] );
}

static uint16_t stm32f4xx_mii_read( STM32F4XXEnetState *s, uint8_t mii_addr ) {
	uint16_t value;
	value =
		mii_addr < MII_NSR
		? s->mii[ mii_addr ]
		: 0xffff
	;
	DB_PRINT("  0x%04x <= %s (0x%02x)\n", value, stm32f4xx_mii_reg_to_string( mii_addr ), mii_addr);
	return value;
}

static void stm32f4xx_mii_write( STM32F4XXEnetState *s, uint8_t mii_addr, uint16_t value ) {

	if ( mii_addr < MII_NSR ) {

		switch( mii_addr ) {
		case MII_BMSR:
		case MII_PHYID1:
		case MII_PHYID2:
			break;
		case MII_BMCR:
			if ( value & MII_BMCR_RESET ) {
				stm32f4xx_mii_reset( s );
				break;
			}
			/* no break */
		default:
			DB_PRINT("0x%04x => %s (0x%02x)\n", value, stm32f4xx_mii_reg_to_string( mii_addr ), mii_addr);
			s->mii[ mii_addr ] = value;
			break;
		}
	}
}

static uint64_t stm32f4xx_enet_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
    STM32F4XXEnetState *s = opaque;

    uint32_t value;

    switch (addr) {
	case ETH_MACCR: value = s->eth_maccr & 0x02cf7efc; break;
	case ETH_MACFFR: value = s->eth_macffr & 0x800007ff; break;
	case ETH_MACHTHR: value = s->eth_machthr & 0xffffffff; break;
	case ETH_MACHTLR: value = s->eth_machtlr & 0xffffffff; break;
	case ETH_MACMIIAR: value = s->eth_macmiiar & 0x0000ffdf; break;
	case ETH_MACMIIDR: value = s->eth_macmiidr & 0x0000ffff; break;
	case ETH_MACFCR: value = s->eth_macfcr & 0xffff00bf; break;
	case ETH_MACVLANTR: value = s->eth_macvlantr & 0x0001ffff; break;
	case ETH_MACRWUFFR: value = s->eth_macrwuffr & 0xffffffff; break;
	case ETH_MACPMTCSR: value = s->eth_macpmtcsr & 0x000002c7; break;
	case ETH_MACDBGR: value = s->eth_macdbgr & 0x037f037e; break;
	case ETH_MACSR: value = s->eth_macsr & 0x00000278; break;
	case ETH_MACIMR: value = s->eth_macimr & 0x00000208; break;
	case ETH_MACA0HR:
		//value = s->eth_maca0hr & 0x80000ffff;
		value =
			0
			| ((uint32_t)s->conf.macaddr.a[4] << 0)
			| ((uint32_t)s->conf.macaddr.a[5] << 8);
		break;
	case ETH_MACA0LR:
		value =
			0
			| ((uint32_t)s->conf.macaddr.a[0] << 0)
			| ((uint32_t)s->conf.macaddr.a[1] << 8)
			| ((uint32_t)s->conf.macaddr.a[2] << 16)
			| ((uint32_t)s->conf.macaddr.a[3] << 24);
		//value = s->eth_maca0lr & 0xffffffff;
		break;
//	case ETH_MACA1HR: value = s->eth_maca1hr & 0xff00ffffff; break;
//	case ETH_MACA1LR: value = s->eth_maca1lr & 0xffffffff; break;
//	case ETH_MACA2HR: value = s->eth_maca2hr & 0xff00ffffff; break;
//	case ETH_MACA2LR: value = s->eth_maca2lr & 0xffffffff; break;
//	case ETH_MACA3HR: value = s->eth_maca3hr & 0xff00ffffff; break;
//	case ETH_MACA3LR: value = s->eth_maca3lr & 0xffffffff; break;
	case ETH_MMCCR: value = s->eth_mmccr & 0x0000003f; break;
	case ETH_MMCRIR: value = s->eth_mmcrir & 0x000200c0; break;
	case ETH_MMCTIR: value = s->eth_mmctir & 0x0020c000; break;
	case ETH_MMCRIMR: value = s->eth_mmcrimr & 0x000200c0; break;
	case ETH_MMCTIMR: value = s->eth_mmctimr & 0x0010c000; break;
	case ETH_MMCTGFSCCR: value = s->eth_mmctgfsccr & 0xffffffff; break;
	case ETH_MMCTGFMSCCR: value = s->eth_mmctgfmsccr & 0xffffffff; break;
	case ETH_MMCTGFCR: value = s->eth_mmctgfcr & 0xffffffff; break;
	case ETH_MMCRFCECR: value = s->eth_mmcrfcecr & 0xffffffff; break;
	case ETH_MMCRGUFCR: value = s->eth_mmcrgufcr & 0xffffffff; break;
	case ETH_PTPTSCR: value = s->eth_ptptscr & 0x0003ff3f; break;
	case ETH_PTPSSIR: value = s->eth_ptpssir & 0x000000ff; break;
	case ETH_PTPTSHR: value = s->eth_ptptshr & 0xffffffff; break;
	case ETH_PTPTSLR: value = s->eth_ptptslr & 0xffffffff; break;
	case ETH_PTPTSHUR: value = s->eth_ptptshur & 0xffffffff; break;
	case ETH_PTPTSLUR: value = s->eth_ptptslur & 0xffffffff; break;
	case ETH_PTPTSAR: value = s->eth_ptptsar & 0xffffffff; break;
	case ETH_PTPTTHR: value = s->eth_ptptthr & 0xffffffff; break;
	case ETH_PTPTTLR: value = s->eth_ptpttlr & 0xffffffff; break;
	case ETH_PTPTSSR: value = s->eth_ptptssr & 0x00000003; break;
	case ETH_PTPPPSCR: value = s->eth_ptpppscr & 0x0000000f; break;
	case ETH_DMABMR: value = s->eth_dmabmr & 0x07ffffff; break;
	case ETH_DMATPDR: value = s->eth_dmatpdr & 0xffffffff; break;
	case ETH_DMARPDR: value = s->eth_dmarpdr & 0xffffffff; break;
	case ETH_DMARDLAR: value = s->eth_dmardlar & 0xffffffff; break;
	case ETH_DMATDLAR: value = s->eth_dmatdlar & 0xffffffff; break;
	case ETH_DMASR: value = s->eth_dmasr & 0x3bffe7ff; break;
	case ETH_DMAOMR: value = s->eth_dmaomr & 0x0731e0de; break;
	case ETH_DMAIER: value = s->eth_dmaier & 0x0001e7ff; break;
	case ETH_DMAMFBOCR: value = s->eth_dmamfbocr & 0x1fffffff; break;
	case ETH_DMARSWTR: value = s->eth_dmarswtr & 0x000000ff; break;
	case ETH_DMACHTDR: value = s->eth_dmachtdr & 0xffffffff; break;
	case ETH_DMACHRDR: value = s->eth_dmachrdr & 0xffffffff; break;
	case ETH_DMACHTBAR: value = s->eth_dmachtbar & 0xffffffff; break;
	case ETH_DMACHRBAR: value = s->eth_dmachrbar & 0xffffffff; break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Bad offset 0x%"HWADDR_PRIx"\n", __func__, addr);
        return 0;
    }

	DB_PRINT(" 0x%04x <= %s (0x%04"HWADDR_PRIx")\n", value, stm32f4xx_enet_reg_to_string( addr ), addr);

    return value;
}

static void stm32f4xx_enet_write(void *opaque, hwaddr addr,
                       uint64_t val64, unsigned int size)
{
    STM32F4XXEnetState *s = opaque;
    uint32_t value = val64;
    //uint8_t phy_addr;
    uint8_t mii_addr;

    DB_PRINT("0x%04x => %s (0x%04"HWADDR_PRIx")\n", value, stm32f4xx_enet_reg_to_string( addr ), addr);

    // stm32f4xx_enet_send(s);

    switch (addr) {
	case ETH_MACCR: s->eth_maccr = value & 0x02cf7efc; break;
	case ETH_MACFFR: s->eth_macffr = value & 0x800007ff; break;
	case ETH_MACHTHR: s->eth_machthr = value & 0xffffffff; break;
	case ETH_MACHTLR: s->eth_machtlr = value & 0xffffffff; break;
	case ETH_MACMIIAR:
		if (!(value & 0x2)) {
			// set up data register for subsequent read operation
			mii_addr = ( value >> 6 ) & 0x1f;
			s->eth_macmiidr = stm32f4xx_mii_read( s, mii_addr );
		}
		// clear busy bit to indicate completion
		value &= ~0x1;
		s->eth_macmiiar = value & 0x0000ffff;
		break;
	case ETH_MACMIIDR:
		value &= 0xffff;
		// assume previous mii_addr in eth_macmiiar is valid
		mii_addr = ( s->eth_macmiiar >> 6 ) & 0x1f;
		stm32f4xx_mii_write( s, mii_addr, value );
		s->eth_macmiidr = value & 0x0000ffff;
		break;
	case ETH_MACFCR: s->eth_macfcr = value & 0xffff00bf; break;
	case ETH_MACVLANTR: s->eth_macvlantr = value & 0x0001ffff; break;
	case ETH_MACRWUFFR: s->eth_macrwuffr = value & 0xffffffff; break;
	case ETH_MACPMTCSR: s->eth_macpmtcsr = value & 0x800002c7; break;
	case ETH_MACDBGR: s->eth_macdbgr = value & 0x00000000; break;
	case ETH_MACSR: s->eth_macsr = value & 0x00000000; break;
	case ETH_MACIMR: s->eth_macimr = value & 0x00000208; break;
	case ETH_MACA0HR:
		//s->eth_maca0hr = value & 0x00000ffff;
		s->conf.macaddr.a[ 5 ] = ( value >> 8 ) & 0xff;
		s->conf.macaddr.a[ 4 ] = ( value >> 0 ) & 0xff;
		break;
	case ETH_MACA0LR:
		//s->eth_maca0lr = value & 0xffffffff;
		s->conf.macaddr.a[ 3 ] = ( value >> 24 ) & 0xff;
		s->conf.macaddr.a[ 2 ] = ( value >> 16 ) & 0xff;
		s->conf.macaddr.a[ 1 ] = ( value >> 8 ) & 0xff;
		s->conf.macaddr.a[ 0 ] = ( value >> 0 ) & 0xff;
		break;
//	case ETH_MACA1HR: s->eth_maca1hr = value & 0xff00ffffff; break;
//	case ETH_MACA1LR: s->eth_maca1lr = value & 0xffffffff; break;
//	case ETH_MACA2HR: s->eth_maca2hr = value & 0xff00ffffff; break;
//	case ETH_MACA2LR: s->eth_maca2lr = value & 0xffffffff; break;
//	case ETH_MACA3HR: s->eth_maca3hr = value & 0xff00ffffff; break;
//	case ETH_MACA3LR: s->eth_maca3lr = value & 0xffffffff; break;
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
	.link_status_changed = stm32f4xx_enet_set_link_status,
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
    DB_PRINT( "%s\n", qemu_get_queue(s->nic)->info_str );

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
