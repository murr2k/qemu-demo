/*
 * RP2040 UART emulation
 *
 * Copyright (c) 2025 QEMU RP2040 Development Team
 *
 * This code is licensed under the GPL version 2 or later.
 */

#include "qemu/osdep.h"
#include "hw/char/rp2040_uart.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-properties-system.h"
#include "migration/vmstate.h"
#include "qemu/log.h"
#include "chardev/char-fe.h"
#include "trace.h"

#define UART_DR     0x000  /* Data Register */
#define UART_RSR    0x004  /* Receive Status Register */
#define UART_ECR    0x004  /* Error Clear Register (write) */
#define UART_FR     0x018  /* Flag Register */
#define UART_ILPR   0x020  /* IrDA Low-Power Counter */
#define UART_IBRD   0x024  /* Integer Baud Rate */
#define UART_FBRD   0x028  /* Fractional Baud Rate */
#define UART_LCR_H  0x02C  /* Line Control */
#define UART_CR     0x030  /* Control Register */
#define UART_IFLS   0x034  /* Interrupt FIFO Level Select */
#define UART_IMSC   0x038  /* Interrupt Mask Set/Clear */
#define UART_RIS    0x03C  /* Raw Interrupt Status */
#define UART_MIS    0x040  /* Masked Interrupt Status */
#define UART_ICR    0x044  /* Interrupt Clear */
#define UART_DMACR  0x048  /* DMA Control */

/* Flag Register bits */
#define FR_CTS      (1 << 0)
#define FR_DSR      (1 << 1)
#define FR_DCD      (1 << 2)
#define FR_BUSY     (1 << 3)
#define FR_RXFE     (1 << 4)  /* RX FIFO empty */
#define FR_TXFF     (1 << 5)  /* TX FIFO full */
#define FR_RXFF     (1 << 6)  /* RX FIFO full */
#define FR_TXFE     (1 << 7)  /* TX FIFO empty */
#define FR_RI       (1 << 8)

/* Control Register bits */
#define CR_UARTEN   (1 << 0)  /* UART enable */
#define CR_SIREN    (1 << 1)  /* SIR enable */
#define CR_SIRLP    (1 << 2)  /* SIR low power */
#define CR_LBE      (1 << 7)  /* Loopback enable */
#define CR_TXE      (1 << 8)  /* Transmit enable */
#define CR_RXE      (1 << 9)  /* Receive enable */
#define CR_DTR      (1 << 10)
#define CR_RTS      (1 << 11)
#define CR_OUT1     (1 << 12)
#define CR_OUT2     (1 << 13)
#define CR_RTSEN    (1 << 14) /* RTS hardware flow control */
#define CR_CTSEN    (1 << 15) /* CTS hardware flow control */

/* Interrupt bits */
#define INT_RIM     (1 << 0)
#define INT_CTSM    (1 << 1)
#define INT_DCDM    (1 << 2)
#define INT_DSRM    (1 << 3)
#define INT_RX      (1 << 4)
#define INT_TX      (1 << 5)
#define INT_RT      (1 << 6)
#define INT_FE      (1 << 7)
#define INT_PE      (1 << 8)
#define INT_BE      (1 << 9)
#define INT_OE      (1 << 10)

#define FIFO_SIZE   32

static void rp2040_uart_update(RP2040UARTState *s)
{
    uint32_t flags = 0;
    
    /* Update flag register */
    if (s->rx_fifo_len == 0) {
        flags |= FR_RXFE;
    }
    if (s->rx_fifo_len == FIFO_SIZE) {
        flags |= FR_RXFF;
    }
    if (s->tx_fifo_len == 0) {
        flags |= FR_TXFE;
    }
    if (s->tx_fifo_len == FIFO_SIZE) {
        flags |= FR_TXFF;
    }
    
    s->fr = flags;
    
    /* Update interrupts */
    s->mis = s->ris & s->imsc;
    qemu_set_irq(s->irq, s->mis != 0);
}

static uint64_t rp2040_uart_read(void *opaque, hwaddr offset, unsigned size)
{
    RP2040UARTState *s = opaque;
    uint32_t val = 0;
    
    switch (offset) {
    case UART_DR:
        if (s->rx_fifo_len > 0) {
            val = s->rx_fifo[s->rx_fifo_rd];
            s->rx_fifo_rd = (s->rx_fifo_rd + 1) % FIFO_SIZE;
            s->rx_fifo_len--;
            if (s->rx_fifo_len == 0) {
                s->ris &= ~INT_RX;
            }
            rp2040_uart_update(s);
        }
        break;
        
    case UART_RSR:
        val = s->rsr;
        break;
        
    case UART_FR:
        val = s->fr;
        break;
        
    case UART_ILPR:
        val = s->ilpr;
        break;
        
    case UART_IBRD:
        val = s->ibrd;
        break;
        
    case UART_FBRD:
        val = s->fbrd;
        break;
        
    case UART_LCR_H:
        val = s->lcr_h;
        break;
        
    case UART_CR:
        val = s->cr;
        break;
        
    case UART_IFLS:
        val = s->ifls;
        break;
        
    case UART_IMSC:
        val = s->imsc;
        break;
        
    case UART_RIS:
        val = s->ris;
        break;
        
    case UART_MIS:
        val = s->mis;
        break;
        
    case UART_DMACR:
        val = s->dmacr;
        break;
        
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                     "rp2040_uart: bad read offset 0x%" HWADDR_PRIx "\n",
                     offset);
    }
    
    return val;
}

static void rp2040_uart_write(void *opaque, hwaddr offset,
                             uint64_t value, unsigned size)
{
    RP2040UARTState *s = opaque;
    unsigned char ch;
    
    switch (offset) {
    case UART_DR:
        ch = value;
        if (s->cr & CR_UARTEN) {
            if (s->cr & CR_TXE) {
                qemu_chr_fe_write(&s->chr, &ch, 1);
                s->ris |= INT_TX;
                rp2040_uart_update(s);
            }
        }
        break;
        
    case UART_ECR:
        s->rsr = 0;
        break;
        
    case UART_FR:
        /* Read only */
        break;
        
    case UART_ILPR:
        s->ilpr = value;
        break;
        
    case UART_IBRD:
        s->ibrd = value;
        break;
        
    case UART_FBRD:
        s->fbrd = value;
        break;
        
    case UART_LCR_H:
        s->lcr_h = value;
        break;
        
    case UART_CR:
        s->cr = value;
        break;
        
    case UART_IFLS:
        s->ifls = value;
        break;
        
    case UART_IMSC:
        s->imsc = value;
        rp2040_uart_update(s);
        break;
        
    case UART_ICR:
        s->ris &= ~value;
        rp2040_uart_update(s);
        break;
        
    case UART_DMACR:
        s->dmacr = value;
        break;
        
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                     "rp2040_uart: bad write offset 0x%" HWADDR_PRIx "\n",
                     offset);
    }
}

static const MemoryRegionOps rp2040_uart_ops = {
    .read = rp2040_uart_read,
    .write = rp2040_uart_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static void rp2040_uart_rx(void *opaque, const uint8_t *buf, int size)
{
    RP2040UARTState *s = opaque;
    
    if (!(s->cr & CR_UARTEN) || !(s->cr & CR_RXE)) {
        return;
    }
    
    for (int i = 0; i < size; i++) {
        if (s->rx_fifo_len < FIFO_SIZE) {
            s->rx_fifo[s->rx_fifo_wr] = buf[i];
            s->rx_fifo_wr = (s->rx_fifo_wr + 1) % FIFO_SIZE;
            s->rx_fifo_len++;
            s->ris |= INT_RX;
        } else {
            s->ris |= INT_OE;
            break;
        }
    }
    
    rp2040_uart_update(s);
}

static int rp2040_uart_can_rx(void *opaque)
{
    RP2040UARTState *s = opaque;
    
    if (!(s->cr & CR_UARTEN) || !(s->cr & CR_RXE)) {
        return 0;
    }
    
    return FIFO_SIZE - s->rx_fifo_len;
}

static void rp2040_uart_event(void *opaque, QEMUChrEvent event)
{
    /* Handle character device events if needed */
}

static void rp2040_uart_reset(DeviceState *dev)
{
    RP2040UARTState *s = RP2040_UART(dev);
    
    s->dr = 0;
    s->rsr = 0;
    s->fr = FR_TXFE | FR_RXFE;
    s->ilpr = 0;
    s->ibrd = 0;
    s->fbrd = 0;
    s->lcr_h = 0;
    s->cr = CR_TXE | CR_RXE;
    s->ifls = 0x12;
    s->imsc = 0;
    s->ris = 0;
    s->mis = 0;
    s->dmacr = 0;
    
    s->rx_fifo_len = 0;
    s->rx_fifo_rd = 0;
    s->rx_fifo_wr = 0;
    s->tx_fifo_len = 0;
    
    rp2040_uart_update(s);
}

static void rp2040_uart_init(Object *obj)
{
    RP2040UARTState *s = RP2040_UART(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    
    memory_region_init_io(&s->mmio, obj, &rp2040_uart_ops, s,
                         TYPE_RP2040_UART, 0x1000);
    sysbus_init_mmio(sbd, &s->mmio);
    sysbus_init_irq(sbd, &s->irq);
}

static void rp2040_uart_realize(DeviceState *dev, Error **errp)
{
    RP2040UARTState *s = RP2040_UART(dev);
    
    qemu_chr_fe_set_handlers(&s->chr, rp2040_uart_can_rx,
                            rp2040_uart_rx, rp2040_uart_event,
                            NULL, s, NULL, true);
}

static const VMStateDescription vmstate_rp2040_uart = {
    .name = TYPE_RP2040_UART,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(dr, RP2040UARTState),
        VMSTATE_UINT32(rsr, RP2040UARTState),
        VMSTATE_UINT32(fr, RP2040UARTState),
        VMSTATE_UINT32(ilpr, RP2040UARTState),
        VMSTATE_UINT32(ibrd, RP2040UARTState),
        VMSTATE_UINT32(fbrd, RP2040UARTState),
        VMSTATE_UINT32(lcr_h, RP2040UARTState),
        VMSTATE_UINT32(cr, RP2040UARTState),
        VMSTATE_UINT32(ifls, RP2040UARTState),
        VMSTATE_UINT32(imsc, RP2040UARTState),
        VMSTATE_UINT32(ris, RP2040UARTState),
        VMSTATE_UINT32(mis, RP2040UARTState),
        VMSTATE_UINT32(dmacr, RP2040UARTState),
        VMSTATE_BUFFER(rx_fifo, RP2040UARTState),
        VMSTATE_UINT32(rx_fifo_len, RP2040UARTState),
        VMSTATE_UINT32(rx_fifo_rd, RP2040UARTState),
        VMSTATE_UINT32(rx_fifo_wr, RP2040UARTState),
        VMSTATE_UINT32(tx_fifo_len, RP2040UARTState),
        VMSTATE_END_OF_LIST()
    }
};

static Property rp2040_uart_properties[] = {
    DEFINE_PROP_CHR("chardev", RP2040UARTState, chr),
    DEFINE_PROP_END_OF_LIST(),
};

static void rp2040_uart_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    
    dc->realize = rp2040_uart_realize;
    dc->reset = rp2040_uart_reset;
    dc->vmsd = &vmstate_rp2040_uart;
    device_class_set_props(dc, rp2040_uart_properties);
}

static const TypeInfo rp2040_uart_info = {
    .name          = TYPE_RP2040_UART,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(RP2040UARTState),
    .instance_init = rp2040_uart_init,
    .class_init    = rp2040_uart_class_init,
};

static void rp2040_uart_register_types(void)
{
    type_register_static(&rp2040_uart_info);
}

type_init(rp2040_uart_register_types)