/*
 * RP2040 GPIO emulation
 *
 * Copyright (c) 2025 QEMU RP2040 Development Team
 *
 * This code is licensed under the GPL version 2 or later.
 */

#include "qemu/osdep.h"
#include "hw/gpio/rp2040_gpio.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "migration/vmstate.h"
#include "qemu/log.h"
#include "trace.h"

#define GPIO_NUM_PINS 30

/* GPIO control registers */
#define GPIO_STATUS(pin)    (0x000 + (pin) * 8)
#define GPIO_CTRL(pin)      (0x004 + (pin) * 8)

/* Interrupt registers */
#define INTR0               0x0F0
#define INTR1               0x0F4
#define INTR2               0x0F8
#define INTR3               0x0FC

#define PROC0_INTE0         0x100
#define PROC0_INTE1         0x104
#define PROC0_INTE2         0x108
#define PROC0_INTE3         0x10C

#define PROC0_INTF0         0x110
#define PROC0_INTF1         0x114
#define PROC0_INTF2         0x118
#define PROC0_INTF3         0x11C

#define PROC0_INTS0         0x120
#define PROC0_INTS1         0x124
#define PROC0_INTS2         0x128
#define PROC0_INTS3         0x12C

#define PROC1_INTE0         0x130
#define PROC1_INTE1         0x134
#define PROC1_INTE2         0x138
#define PROC1_INTE3         0x13C

#define PROC1_INTF0         0x140
#define PROC1_INTF1         0x144
#define PROC1_INTF2         0x148
#define PROC1_INTF3         0x14C

#define PROC1_INTS0         0x150
#define PROC1_INTS1         0x154
#define PROC1_INTS2         0x158
#define PROC1_INTS3         0x15C

#define DORMANT_WAKE_INTE0  0x160
#define DORMANT_WAKE_INTE1  0x164
#define DORMANT_WAKE_INTE2  0x168
#define DORMANT_WAKE_INTE3  0x16C

#define DORMANT_WAKE_INTF0  0x170
#define DORMANT_WAKE_INTF1  0x174
#define DORMANT_WAKE_INTF2  0x178
#define DORMANT_WAKE_INTF3  0x17C

#define DORMANT_WAKE_INTS0  0x180
#define DORMANT_WAKE_INTS1  0x184
#define DORMANT_WAKE_INTS2  0x188
#define DORMANT_WAKE_INTS3  0x18C

/* GPIO control function select */
#define FUNCSEL_SPI         1
#define FUNCSEL_UART        2
#define FUNCSEL_I2C         3
#define FUNCSEL_PWM         4
#define FUNCSEL_SIO         5
#define FUNCSEL_PIO0        6
#define FUNCSEL_PIO1        7
#define FUNCSEL_USB         9
#define FUNCSEL_NULL        31

/* Interrupt types */
#define INT_LEVEL_LOW       (1 << 0)
#define INT_LEVEL_HIGH      (1 << 1)
#define INT_EDGE_LOW        (1 << 2)
#define INT_EDGE_HIGH       (1 << 3)

static void rp2040_gpio_update_irq(RP2040GPIOState *s)
{
    uint32_t proc0_status = 0;
    uint32_t proc1_status = 0;
    
    /* Check each GPIO pin for interrupts */
    for (int i = 0; i < GPIO_NUM_PINS; i++) {
        int reg = i / 8;
        int bit = (i % 8) * 4;
        
        if (s->intr[reg] & (0xF << bit)) {
            if (s->proc0_inte[reg] & (0xF << bit)) {
                proc0_status = 1;
            }
            if (s->proc1_inte[reg] & (0xF << bit)) {
                proc1_status = 1;
            }
        }
    }
    
    qemu_set_irq(s->proc0_irq, proc0_status);
    qemu_set_irq(s->proc1_irq, proc1_status);
}

static uint64_t rp2040_gpio_read(void *opaque, hwaddr offset, unsigned size)
{
    RP2040GPIOState *s = opaque;
    uint32_t val = 0;
    
    if (offset < 0xF0) {
        /* GPIO control registers */
        int pin = offset / 8;
        if (pin < GPIO_NUM_PINS) {
            if ((offset & 7) == 0) {
                /* STATUS register */
                val = s->status[pin];
            } else {
                /* CTRL register */
                val = s->ctrl[pin];
            }
        }
    } else {
        switch (offset) {
        case INTR0:
            val = s->intr[0];
            break;
        case INTR1:
            val = s->intr[1];
            break;
        case INTR2:
            val = s->intr[2];
            break;
        case INTR3:
            val = s->intr[3];
            break;
            
        case PROC0_INTE0:
            val = s->proc0_inte[0];
            break;
        case PROC0_INTE1:
            val = s->proc0_inte[1];
            break;
        case PROC0_INTE2:
            val = s->proc0_inte[2];
            break;
        case PROC0_INTE3:
            val = s->proc0_inte[3];
            break;
            
        case PROC0_INTF0:
            val = s->proc0_intf[0];
            break;
        case PROC0_INTF1:
            val = s->proc0_intf[1];
            break;
        case PROC0_INTF2:
            val = s->proc0_intf[2];
            break;
        case PROC0_INTF3:
            val = s->proc0_intf[3];
            break;
            
        case PROC0_INTS0:
            val = s->intr[0] & s->proc0_inte[0];
            break;
        case PROC0_INTS1:
            val = s->intr[1] & s->proc0_inte[1];
            break;
        case PROC0_INTS2:
            val = s->intr[2] & s->proc0_inte[2];
            break;
        case PROC0_INTS3:
            val = s->intr[3] & s->proc0_inte[3];
            break;
            
        case PROC1_INTE0:
            val = s->proc1_inte[0];
            break;
        case PROC1_INTE1:
            val = s->proc1_inte[1];
            break;
        case PROC1_INTE2:
            val = s->proc1_inte[2];
            break;
        case PROC1_INTE3:
            val = s->proc1_inte[3];
            break;
            
        case PROC1_INTF0:
            val = s->proc1_intf[0];
            break;
        case PROC1_INTF1:
            val = s->proc1_intf[1];
            break;
        case PROC1_INTF2:
            val = s->proc1_intf[2];
            break;
        case PROC1_INTF3:
            val = s->proc1_intf[3];
            break;
            
        case PROC1_INTS0:
            val = s->intr[0] & s->proc1_inte[0];
            break;
        case PROC1_INTS1:
            val = s->intr[1] & s->proc1_inte[1];
            break;
        case PROC1_INTS2:
            val = s->intr[2] & s->proc1_inte[2];
            break;
        case PROC1_INTS3:
            val = s->intr[3] & s->proc1_inte[3];
            break;
            
        default:
            qemu_log_mask(LOG_GUEST_ERROR,
                         "rp2040_gpio: bad read offset 0x%" HWADDR_PRIx "\n",
                         offset);
        }
    }
    
    return val;
}

static void rp2040_gpio_write(void *opaque, hwaddr offset,
                             uint64_t value, unsigned size)
{
    RP2040GPIOState *s = opaque;
    
    if (offset < 0xF0) {
        /* GPIO control registers */
        int pin = offset / 8;
        if (pin < GPIO_NUM_PINS) {
            if ((offset & 7) == 0) {
                /* STATUS register - read only */
            } else {
                /* CTRL register */
                s->ctrl[pin] = value;
                /* Update function selection */
                int funcsel = value & 0x1F;
                if (funcsel == FUNCSEL_SIO) {
                    /* GPIO is under software control */
                    s->status[pin] = (s->status[pin] & ~0x1F) | funcsel;
                }
            }
        }
    } else {
        switch (offset) {
        case INTR0:
        case INTR1:
        case INTR2:
        case INTR3:
            /* Clear interrupt bits by writing 1 */
            s->intr[(offset - INTR0) / 4] &= ~value;
            rp2040_gpio_update_irq(s);
            break;
            
        case PROC0_INTE0:
            s->proc0_inte[0] = value;
            rp2040_gpio_update_irq(s);
            break;
        case PROC0_INTE1:
            s->proc0_inte[1] = value;
            rp2040_gpio_update_irq(s);
            break;
        case PROC0_INTE2:
            s->proc0_inte[2] = value;
            rp2040_gpio_update_irq(s);
            break;
        case PROC0_INTE3:
            s->proc0_inte[3] = value;
            rp2040_gpio_update_irq(s);
            break;
            
        case PROC0_INTF0:
            s->proc0_intf[0] = value;
            break;
        case PROC0_INTF1:
            s->proc0_intf[1] = value;
            break;
        case PROC0_INTF2:
            s->proc0_intf[2] = value;
            break;
        case PROC0_INTF3:
            s->proc0_intf[3] = value;
            break;
            
        case PROC1_INTE0:
            s->proc1_inte[0] = value;
            rp2040_gpio_update_irq(s);
            break;
        case PROC1_INTE1:
            s->proc1_inte[1] = value;
            rp2040_gpio_update_irq(s);
            break;
        case PROC1_INTE2:
            s->proc1_inte[2] = value;
            rp2040_gpio_update_irq(s);
            break;
        case PROC1_INTE3:
            s->proc1_inte[3] = value;
            rp2040_gpio_update_irq(s);
            break;
            
        case PROC1_INTF0:
            s->proc1_intf[0] = value;
            break;
        case PROC1_INTF1:
            s->proc1_intf[1] = value;
            break;
        case PROC1_INTF2:
            s->proc1_intf[2] = value;
            break;
        case PROC1_INTF3:
            s->proc1_intf[3] = value;
            break;
            
        default:
            qemu_log_mask(LOG_GUEST_ERROR,
                         "rp2040_gpio: bad write offset 0x%" HWADDR_PRIx "\n",
                         offset);
        }
    }
}

static const MemoryRegionOps rp2040_gpio_ops = {
    .read = rp2040_gpio_read,
    .write = rp2040_gpio_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

/* Called from SIO when GPIO state changes */
void rp2040_gpio_set_input(RP2040GPIOState *s, int pin, int level)
{
    if (pin >= GPIO_NUM_PINS) {
        return;
    }
    
    uint32_t old_level = (s->status[pin] >> 17) & 1;
    
    /* Update input level in status register */
    if (level) {
        s->status[pin] |= (1 << 17);
    } else {
        s->status[pin] &= ~(1 << 17);
    }
    
    /* Check for interrupt conditions */
    int reg = pin / 8;
    int bit = (pin % 8) * 4;
    
    if ((s->ctrl[pin] & 0x1F) == FUNCSEL_SIO) {
        uint32_t int_mask = (s->ctrl[pin] >> 28) & 0xF;
        uint32_t int_status = 0;
        
        if ((int_mask & INT_LEVEL_LOW) && !level) {
            int_status |= INT_LEVEL_LOW;
        }
        if ((int_mask & INT_LEVEL_HIGH) && level) {
            int_status |= INT_LEVEL_HIGH;
        }
        if ((int_mask & INT_EDGE_LOW) && old_level && !level) {
            int_status |= INT_EDGE_LOW;
        }
        if ((int_mask & INT_EDGE_HIGH) && !old_level && level) {
            int_status |= INT_EDGE_HIGH;
        }
        
        if (int_status) {
            s->intr[reg] |= (int_status << bit);
            rp2040_gpio_update_irq(s);
        }
    }
}

static void rp2040_gpio_reset(DeviceState *dev)
{
    RP2040GPIOState *s = RP2040_GPIO(dev);
    
    memset(s->status, 0, sizeof(s->status));
    memset(s->ctrl, 0, sizeof(s->ctrl));
    memset(s->intr, 0, sizeof(s->intr));
    memset(s->proc0_inte, 0, sizeof(s->proc0_inte));
    memset(s->proc0_intf, 0, sizeof(s->proc0_intf));
    memset(s->proc1_inte, 0, sizeof(s->proc1_inte));
    memset(s->proc1_intf, 0, sizeof(s->proc1_intf));
    
    /* Set all pins to NULL function by default */
    for (int i = 0; i < GPIO_NUM_PINS; i++) {
        s->ctrl[i] = FUNCSEL_NULL;
        s->status[i] = FUNCSEL_NULL;
    }
}

static void rp2040_gpio_init(Object *obj)
{
    RP2040GPIOState *s = RP2040_GPIO(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    
    memory_region_init_io(&s->mmio, obj, &rp2040_gpio_ops, s,
                         TYPE_RP2040_GPIO, 0x1000);
    sysbus_init_mmio(sbd, &s->mmio);
    
    sysbus_init_irq(sbd, &s->proc0_irq);
    sysbus_init_irq(sbd, &s->proc1_irq);
}

static const VMStateDescription vmstate_rp2040_gpio = {
    .name = TYPE_RP2040_GPIO,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(status, RP2040GPIOState, GPIO_NUM_PINS),
        VMSTATE_UINT32_ARRAY(ctrl, RP2040GPIOState, GPIO_NUM_PINS),
        VMSTATE_UINT32_ARRAY(intr, RP2040GPIOState, 4),
        VMSTATE_UINT32_ARRAY(proc0_inte, RP2040GPIOState, 4),
        VMSTATE_UINT32_ARRAY(proc0_intf, RP2040GPIOState, 4),
        VMSTATE_UINT32_ARRAY(proc1_inte, RP2040GPIOState, 4),
        VMSTATE_UINT32_ARRAY(proc1_intf, RP2040GPIOState, 4),
        VMSTATE_END_OF_LIST()
    }
};

static void rp2040_gpio_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    
    dc->reset = rp2040_gpio_reset;
    dc->vmsd = &vmstate_rp2040_gpio;
}

static const TypeInfo rp2040_gpio_info = {
    .name          = TYPE_RP2040_GPIO,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(RP2040GPIOState),
    .instance_init = rp2040_gpio_init,
    .class_init    = rp2040_gpio_class_init,
};

static void rp2040_gpio_register_types(void)
{
    type_register_static(&rp2040_gpio_info);
}

type_init(rp2040_gpio_register_types)