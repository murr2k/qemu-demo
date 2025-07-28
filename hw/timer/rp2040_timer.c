/*
 * RP2040 Timer emulation
 *
 * Copyright (c) 2025 QEMU RP2040 Development Team
 *
 * This code is licensed under the GPL version 2 or later.
 */

#include "qemu/osdep.h"
#include "hw/timer/rp2040_timer.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "migration/vmstate.h"
#include "qemu/log.h"
#include "qemu/timer.h"
#include "trace.h"

/* Timer registers */
#define TIMEHW          0x00
#define TIMELW          0x04
#define TIMEHR          0x08
#define TIMELR          0x0C
#define ALARM0          0x10
#define ALARM1          0x14
#define ALARM2          0x18
#define ALARM3          0x1C
#define ARMED           0x20
#define TIMERAWH        0x24
#define TIMERAWL        0x28
#define DBGPAUSE        0x2C
#define PAUSE           0x30
#define INTR            0x34
#define INTE            0x38
#define INTF            0x3C
#define INTS            0x40

static uint64_t rp2040_timer_get_count(RP2040TimerState *s)
{
    return qemu_clock_get_us(QEMU_CLOCK_VIRTUAL) - s->time_base;
}

static void rp2040_timer_update_alarm(RP2040TimerState *s, int alarm)
{
    uint64_t now = rp2040_timer_get_count(s);
    uint64_t alarm_time = ((uint64_t)s->alarm_high[alarm] << 32) | s->alarm[alarm];
    
    if (!(s->armed & (1 << alarm))) {
        timer_del(s->alarm_timer[alarm]);
        return;
    }
    
    if (alarm_time <= now) {
        /* Alarm should fire immediately */
        s->intr |= (1 << alarm);
        s->armed &= ~(1 << alarm);
        timer_del(s->alarm_timer[alarm]);
    } else {
        /* Schedule alarm */
        timer_mod(s->alarm_timer[alarm], 
                 qemu_clock_get_us(QEMU_CLOCK_VIRTUAL) + (alarm_time - now));
    }
}

static void rp2040_timer_alarm_cb(void *opaque)
{
    RP2040TimerState *s = opaque;
    int alarm = (intptr_t)opaque - (intptr_t)s;
    alarm = alarm / sizeof(s->alarm_timer[0]);
    
    s->intr |= (1 << alarm);
    s->armed &= ~(1 << alarm);
    
    if (s->inte & (1 << alarm)) {
        qemu_set_irq(s->irq[alarm], 1);
    }
}

static uint64_t rp2040_timer_read(void *opaque, hwaddr offset, unsigned size)
{
    RP2040TimerState *s = opaque;
    uint64_t count;
    uint32_t val = 0;
    
    switch (offset) {
    case TIMEHW:
        count = rp2040_timer_get_count(s);
        s->latched_count = count;
        val = count >> 32;
        break;
        
    case TIMELW:
        val = s->latched_count & 0xFFFFFFFF;
        break;
        
    case TIMEHR:
        count = rp2040_timer_get_count(s);
        val = count >> 32;
        break;
        
    case TIMELR:
        count = rp2040_timer_get_count(s);
        val = count & 0xFFFFFFFF;
        break;
        
    case ALARM0:
        val = s->alarm[0];
        break;
    case ALARM1:
        val = s->alarm[1];
        break;
    case ALARM2:
        val = s->alarm[2];
        break;
    case ALARM3:
        val = s->alarm[3];
        break;
        
    case ARMED:
        val = s->armed;
        break;
        
    case TIMERAWH:
        count = rp2040_timer_get_count(s);
        val = count >> 32;
        break;
        
    case TIMERAWL:
        count = rp2040_timer_get_count(s);
        val = count & 0xFFFFFFFF;
        break;
        
    case DBGPAUSE:
        val = s->dbgpause;
        break;
        
    case PAUSE:
        val = s->pause;
        break;
        
    case INTR:
        val = s->intr;
        break;
        
    case INTE:
        val = s->inte;
        break;
        
    case INTF:
        val = s->intf;
        break;
        
    case INTS:
        val = s->intr & s->inte;
        break;
        
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                     "rp2040_timer: bad read offset 0x%" HWADDR_PRIx "\n",
                     offset);
    }
    
    return val;
}

static void rp2040_timer_write(void *opaque, hwaddr offset,
                              uint64_t value, unsigned size)
{
    RP2040TimerState *s = opaque;
    
    switch (offset) {
    case TIMELW:
        /* Write to lower 32 bits of timer - sets new base */
        s->time_base = qemu_clock_get_us(QEMU_CLOCK_VIRTUAL) - value;
        /* Update alarms with new time base */
        for (int i = 0; i < 4; i++) {
            rp2040_timer_update_alarm(s, i);
        }
        break;
        
    case ALARM0:
        s->alarm[0] = value;
        s->alarm_high[0] = rp2040_timer_get_count(s) >> 32;
        s->armed |= (1 << 0);
        rp2040_timer_update_alarm(s, 0);
        break;
    case ALARM1:
        s->alarm[1] = value;
        s->alarm_high[1] = rp2040_timer_get_count(s) >> 32;
        s->armed |= (1 << 1);
        rp2040_timer_update_alarm(s, 1);
        break;
    case ALARM2:
        s->alarm[2] = value;
        s->alarm_high[2] = rp2040_timer_get_count(s) >> 32;
        s->armed |= (1 << 2);
        rp2040_timer_update_alarm(s, 2);
        break;
    case ALARM3:
        s->alarm[3] = value;
        s->alarm_high[3] = rp2040_timer_get_count(s) >> 32;
        s->armed |= (1 << 3);
        rp2040_timer_update_alarm(s, 3);
        break;
        
    case ARMED:
        /* Writing 1 clears the armed bit */
        s->armed &= ~value;
        for (int i = 0; i < 4; i++) {
            if (value & (1 << i)) {
                timer_del(s->alarm_timer[i]);
            }
        }
        break;
        
    case DBGPAUSE:
        s->dbgpause = value & 0x3;
        break;
        
    case PAUSE:
        s->pause = value & 0x1;
        break;
        
    case INTR:
        /* Clear interrupt bits by writing 1 */
        s->intr &= ~value;
        for (int i = 0; i < 4; i++) {
            if (value & (1 << i)) {
                qemu_set_irq(s->irq[i], 0);
            }
        }
        break;
        
    case INTE:
        s->inte = value & 0xF;
        break;
        
    case INTF:
        s->intf = value & 0xF;
        break;
        
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                     "rp2040_timer: bad write offset 0x%" HWADDR_PRIx "\n",
                     offset);
    }
}

static const MemoryRegionOps rp2040_timer_ops = {
    .read = rp2040_timer_read,
    .write = rp2040_timer_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static void rp2040_timer_reset(DeviceState *dev)
{
    RP2040TimerState *s = RP2040_TIMER(dev);
    
    s->time_base = qemu_clock_get_us(QEMU_CLOCK_VIRTUAL);
    s->latched_count = 0;
    s->armed = 0;
    s->dbgpause = 0;
    s->pause = 0;
    s->intr = 0;
    s->inte = 0;
    s->intf = 0;
    
    for (int i = 0; i < 4; i++) {
        s->alarm[i] = 0;
        s->alarm_high[i] = 0;
        timer_del(s->alarm_timer[i]);
    }
}

static void rp2040_timer_init(Object *obj)
{
    RP2040TimerState *s = RP2040_TIMER(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    
    memory_region_init_io(&s->mmio, obj, &rp2040_timer_ops, s,
                         TYPE_RP2040_TIMER, 0x1000);
    sysbus_init_mmio(sbd, &s->mmio);
    
    /* Initialize IRQs for 4 alarms */
    for (int i = 0; i < 4; i++) {
        sysbus_init_irq(sbd, &s->irq[i]);
    }
}

static void rp2040_timer_realize(DeviceState *dev, Error **errp)
{
    RP2040TimerState *s = RP2040_TIMER(dev);
    
    /* Create timers for alarms */
    for (int i = 0; i < 4; i++) {
        s->alarm_timer[i] = timer_new_us(QEMU_CLOCK_VIRTUAL,
                                        rp2040_timer_alarm_cb,
                                        &s->alarm_timer[i]);
    }
}

static const VMStateDescription vmstate_rp2040_timer = {
    .name = TYPE_RP2040_TIMER,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT64(time_base, RP2040TimerState),
        VMSTATE_UINT64(latched_count, RP2040TimerState),
        VMSTATE_UINT32_ARRAY(alarm, RP2040TimerState, 4),
        VMSTATE_UINT32_ARRAY(alarm_high, RP2040TimerState, 4),
        VMSTATE_UINT32(armed, RP2040TimerState),
        VMSTATE_UINT32(dbgpause, RP2040TimerState),
        VMSTATE_UINT32(pause, RP2040TimerState),
        VMSTATE_UINT32(intr, RP2040TimerState),
        VMSTATE_UINT32(inte, RP2040TimerState),
        VMSTATE_UINT32(intf, RP2040TimerState),
        VMSTATE_TIMER_PTR_ARRAY(alarm_timer, RP2040TimerState, 4),
        VMSTATE_END_OF_LIST()
    }
};

static void rp2040_timer_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    
    dc->realize = rp2040_timer_realize;
    dc->reset = rp2040_timer_reset;
    dc->vmsd = &vmstate_rp2040_timer;
}

static const TypeInfo rp2040_timer_info = {
    .name          = TYPE_RP2040_TIMER,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(RP2040TimerState),
    .instance_init = rp2040_timer_init,
    .class_init    = rp2040_timer_class_init,
};

static void rp2040_timer_register_types(void)
{
    type_register_static(&rp2040_timer_info);
}

type_init(rp2040_timer_register_types)