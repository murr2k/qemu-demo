/*
 * RP2040 Timer emulation
 *
 * Copyright (c) 2025 QEMU RP2040 Development Team
 *
 * This code is licensed under the GPL version 2 or later.
 */

#ifndef HW_TIMER_RP2040_TIMER_H
#define HW_TIMER_RP2040_TIMER_H

#include "hw/sysbus.h"
#include "qemu/timer.h"
#include "qom/object.h"

#define TYPE_RP2040_TIMER "rp2040-timer"
OBJECT_DECLARE_SIMPLE_TYPE(RP2040TimerState, RP2040_TIMER)

typedef struct RP2040TimerState {
    SysBusDevice parent_obj;
    
    MemoryRegion mmio;
    qemu_irq irq[4];  /* 4 alarm IRQs */
    
    /* Timer state */
    uint64_t time_base;
    uint64_t latched_count;
    
    /* Alarm registers */
    uint32_t alarm[4];
    uint32_t alarm_high[4];
    uint32_t armed;
    
    /* Control registers */
    uint32_t dbgpause;
    uint32_t pause;
    
    /* Interrupt registers */
    uint32_t intr;
    uint32_t inte;
    uint32_t intf;
    
    /* QEMU timers for alarms */
    QEMUTimer *alarm_timer[4];
} RP2040TimerState;

#endif /* HW_TIMER_RP2040_TIMER_H */