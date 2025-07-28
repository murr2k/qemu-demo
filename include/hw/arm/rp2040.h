/*
 * Raspberry Pi RP2040 SoC emulation
 *
 * Copyright (c) 2025 QEMU RP2040 Development Team
 *
 * This code is licensed under the GPL version 2 or later.
 */

#ifndef HW_ARM_RP2040_H
#define HW_ARM_RP2040_H

#include "hw/sysbus.h"
#include "hw/arm/armv7m.h"
#include "qom/object.h"

#define TYPE_RP2040_SOC "rp2040-soc"
OBJECT_DECLARE_SIMPLE_TYPE(RP2040State, RP2040_SOC)

#define RP2040_NUM_CORES 2

/* Memory map constants */
#define RP2040_ROM_BASE         0x00000000
#define RP2040_ROM_SIZE         (16 * 1024)

#define RP2040_XIP_BASE         0x10000000
#define RP2040_XIP_SIZE         (16 * 1024 * 1024)

#define RP2040_SRAM_BASE        0x20000000
#define RP2040_SRAM_SIZE        (264 * 1024)

/* Forward declarations for peripherals */
typedef struct RP2040UARTState RP2040UARTState;
typedef struct RP2040GPIOState RP2040GPIOState;
typedef struct RP2040TimerState RP2040TimerState;
typedef struct RP2040DMAState RP2040DMAState;
typedef struct RP2040PIOState RP2040PIOState;
typedef struct RP2040SIOState RP2040SIOState;

typedef struct RP2040State {
    SysBusDevice parent_obj;

    ARMv7MState cpu[RP2040_NUM_CORES];
    
    MemoryRegion rom;
    MemoryRegion sram;
    MemoryRegion xip;
    MemoryRegion peripherals;
    
    /* Peripherals */
    RP2040UARTState *uart[2];
    RP2040GPIOState *gpio;
    RP2040TimerState *timer;
    RP2040DMAState *dma;
    RP2040PIOState *pio[2];
    RP2040SIOState *sio;
    
    uint32_t num_cpus;
} RP2040State;

#endif /* HW_ARM_RP2040_H */