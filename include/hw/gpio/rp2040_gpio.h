/*
 * RP2040 GPIO emulation
 *
 * Copyright (c) 2025 QEMU RP2040 Development Team
 *
 * This code is licensed under the GPL version 2 or later.
 */

#ifndef HW_GPIO_RP2040_GPIO_H
#define HW_GPIO_RP2040_GPIO_H

#include "hw/sysbus.h"
#include "qom/object.h"

#define TYPE_RP2040_GPIO "rp2040-gpio"
OBJECT_DECLARE_SIMPLE_TYPE(RP2040GPIOState, RP2040_GPIO)

#define GPIO_NUM_PINS 30

typedef struct RP2040GPIOState {
    SysBusDevice parent_obj;
    
    MemoryRegion mmio;
    qemu_irq proc0_irq;
    qemu_irq proc1_irq;
    
    /* GPIO registers */
    uint32_t status[GPIO_NUM_PINS];
    uint32_t ctrl[GPIO_NUM_PINS];
    
    /* Interrupt registers */
    uint32_t intr[4];
    uint32_t proc0_inte[4];
    uint32_t proc0_intf[4];
    uint32_t proc1_inte[4];
    uint32_t proc1_intf[4];
} RP2040GPIOState;

/* Interface functions */
void rp2040_gpio_set_input(RP2040GPIOState *s, int pin, int level);

#endif /* HW_GPIO_RP2040_GPIO_H */