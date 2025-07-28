/*
 * RP2040 UART emulation
 *
 * Copyright (c) 2025 QEMU RP2040 Development Team
 *
 * This code is licensed under the GPL version 2 or later.
 */

#ifndef HW_CHAR_RP2040_UART_H
#define HW_CHAR_RP2040_UART_H

#include "hw/sysbus.h"
#include "chardev/char-fe.h"
#include "qom/object.h"

#define TYPE_RP2040_UART "rp2040-uart"
OBJECT_DECLARE_SIMPLE_TYPE(RP2040UARTState, RP2040_UART)

#define FIFO_SIZE 32

typedef struct RP2040UARTState {
    SysBusDevice parent_obj;
    
    MemoryRegion mmio;
    CharBackend chr;
    qemu_irq irq;
    
    /* Registers */
    uint32_t dr;      /* Data register */
    uint32_t rsr;     /* Receive status register */
    uint32_t fr;      /* Flag register */
    uint32_t ilpr;    /* IrDA low-power counter */
    uint32_t ibrd;    /* Integer baud rate */
    uint32_t fbrd;    /* Fractional baud rate */
    uint32_t lcr_h;   /* Line control */
    uint32_t cr;      /* Control register */
    uint32_t ifls;    /* Interrupt FIFO level select */
    uint32_t imsc;    /* Interrupt mask set/clear */
    uint32_t ris;     /* Raw interrupt status */
    uint32_t mis;     /* Masked interrupt status */
    uint32_t dmacr;   /* DMA control */
    
    /* FIFOs */
    uint8_t rx_fifo[FIFO_SIZE];
    uint32_t rx_fifo_len;
    uint32_t rx_fifo_rd;
    uint32_t rx_fifo_wr;
    uint32_t tx_fifo_len;
} RP2040UARTState;

#endif /* HW_CHAR_RP2040_UART_H */