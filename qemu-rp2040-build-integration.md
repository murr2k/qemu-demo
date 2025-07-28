# QEMU RP2040 Build System Integration

## Meson Build Files

### hw/arm/meson.build
```meson
arm_ss.add(when: 'CONFIG_RASPI', if_true: files(
  'bcm2835_peripherals.c',
  'bcm2836.c',
  'raspi.c',
))

# Add RP2040 support
arm_ss.add(when: 'CONFIG_RP2040', if_true: files(
  'rp2040.c',
  'rp2040_soc.c',
))

hw_arch += {'arm': arm_ss}
```

### hw/misc/meson.build
```meson
# RP2040 specific peripherals
softmmu_ss.add(when: 'CONFIG_RP2040', if_true: files(
  'rp2040_sio.c',
  'rp2040_pio.c',
  'rp2040_resets.c',
  'rp2040_clocks.c',
  'rp2040_xosc.c',
  'rp2040_rosc.c',
  'rp2040_pll.c',
  'rp2040_syscfg.c',
))
```

### hw/char/meson.build
```meson
softmmu_ss.add(when: 'CONFIG_RP2040', if_true: files('rp2040_uart.c'))
```

### hw/timer/meson.build
```meson
softmmu_ss.add(when: 'CONFIG_RP2040', if_true: files(
  'rp2040_timer.c',
  'rp2040_watchdog.c',
  'rp2040_rtc.c',
))
```

### hw/gpio/meson.build
```meson
softmmu_ss.add(when: 'CONFIG_RP2040', if_true: files('rp2040_gpio.c'))
```

### hw/ssi/meson.build
```meson
softmmu_ss.add(when: 'CONFIG_RP2040', if_true: files('rp2040_spi.c'))
```

### hw/i2c/meson.build
```meson
softmmu_ss.add(when: 'CONFIG_RP2040', if_true: files('rp2040_i2c.c'))
```

### hw/dma/meson.build
```meson
softmmu_ss.add(when: 'CONFIG_RP2040', if_true: files('rp2040_dma.c'))
```

### hw/adc/meson.build
```meson
softmmu_ss.add(when: 'CONFIG_RP2040', if_true: files('rp2040_adc.c'))
```

## Kconfig Configuration

### hw/arm/Kconfig
```kconfig
config RP2040
    bool
    select ARM_V7M
    select UNIMP
    select SSI
    select I2C
    select USB_EHCI_SYSBUS
    select SERIAL

config RASPBERRY_PI_PICO
    bool
    default y
    depends on TCG && ARM
    select RP2040
```

### default-configs/devices/arm-softmmu.mak
```makefile
# Raspberry Pi RP2040
CONFIG_RP2040=y
CONFIG_RASPBERRY_PI_PICO=y
```

## Header File Organization

### include/hw/arm/rp2040.h
```c
/*
 * Raspberry Pi RP2040 SoC
 *
 * Copyright (c) 2024 QEMU contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HW_ARM_RP2040_H
#define HW_ARM_RP2040_H

#include "hw/sysbus.h"
#include "hw/arm/armv7m.h"
#include "qom/object.h"

#define TYPE_RP2040 "rp2040"
OBJECT_DECLARE_TYPE(RP2040State, RP2040Class, RP2040)

/* Forward declarations for peripheral types */
typedef struct RP2040UARTState RP2040UARTState;
typedef struct RP2040GPIOState RP2040GPIOState;
typedef struct RP2040SIOState RP2040SIOState;
typedef struct RP2040PIOState RP2040PIOState;
typedef struct RP2040DMAState RP2040DMAState;
typedef struct RP2040TimerState RP2040TimerState;
/* ... other peripherals ... */

struct RP2040State {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/
    
    ARMv7MState cpu[2];
    
    /* Peripherals */
    RP2040UARTState *uart[2];
    RP2040GPIOState *gpio;
    RP2040SIOState *sio;
    RP2040PIOState *pio[2];
    RP2040DMAState *dma;
    RP2040TimerState *timer;
    /* ... other peripherals ... */
    
    /* Memory regions */
    MemoryRegion rom;
    MemoryRegion sram[6];
    MemoryRegion peripherals;
    MemoryRegion xip;
};

struct RP2040Class {
    /*< private >*/
    SysBusDeviceClass parent_class;
    /*< public >*/
    
    /* Class methods if needed */
};

#endif /* HW_ARM_RP2040_H */
```

### include/hw/misc/rp2040_sio.h
```c
/*
 * RP2040 Single-cycle I/O (SIO) block
 */

#ifndef HW_MISC_RP2040_SIO_H
#define HW_MISC_RP2040_SIO_H

#include "hw/sysbus.h"
#include "qom/object.h"

#define TYPE_RP2040_SIO "rp2040-sio"
OBJECT_DECLARE_SIMPLE_TYPE(RP2040SIOState, RP2040_SIO)

/* Public interface for SIO */
void rp2040_sio_set_gpio_out(RP2040SIOState *s, int cpu, uint32_t value);
uint32_t rp2040_sio_get_gpio_in(RP2040SIOState *s, int cpu);
void rp2040_sio_fifo_push(RP2040SIOState *s, int cpu, uint32_t value);
uint32_t rp2040_sio_fifo_pop(RP2040SIOState *s, int cpu);

#endif /* HW_MISC_RP2040_SIO_H */
```

## Directory Structure

```
qemu/
├── hw/
│   ├── arm/
│   │   ├── rp2040.c                 # Board definitions (Pico, etc.)
│   │   └── rp2040_soc.c             # SoC integration
│   ├── char/
│   │   └── rp2040_uart.c            # UART implementation
│   ├── gpio/
│   │   └── rp2040_gpio.c            # GPIO banks
│   ├── misc/
│   │   ├── rp2040_sio.c             # Single-cycle I/O
│   │   ├── rp2040_pio.c             # Programmable I/O
│   │   ├── rp2040_resets.c          # Reset controller
│   │   ├── rp2040_clocks.c          # Clock controller
│   │   ├── rp2040_syscfg.c          # System configuration
│   │   ├── rp2040_xosc.c            # Crystal oscillator
│   │   ├── rp2040_rosc.c            # Ring oscillator
│   │   └── rp2040_pll.c             # PLL
│   ├── timer/
│   │   ├── rp2040_timer.c           # Timer
│   │   ├── rp2040_watchdog.c        # Watchdog
│   │   └── rp2040_rtc.c             # Real-time clock
│   ├── ssi/
│   │   └── rp2040_spi.c             # SPI controller
│   ├── i2c/
│   │   └── rp2040_i2c.c             # I2C controller
│   ├── dma/
│   │   └── rp2040_dma.c             # DMA controller
│   ├── adc/
│   │   └── rp2040_adc.c             # ADC + temperature sensor
│   └── usb/
│       └── rp2040_usb.c             # USB controller
├── include/
│   └── hw/
│       ├── arm/
│       │   └── rp2040.h             # Main SoC header
│       ├── char/
│       │   └── rp2040_uart.h        # UART interface
│       ├── gpio/
│       │   └── rp2040_gpio.h        # GPIO interface
│       ├── misc/
│       │   ├── rp2040_sio.h         # SIO interface
│       │   └── rp2040_pio.h         # PIO interface
│       └── ...                      # Other peripheral headers
└── tests/
    └── qtest/
        ├── rp2040-test.c            # Basic functionality tests
        └── pico-test.c              # Board-specific tests
```

## Testing Infrastructure

### tests/qtest/rp2040-test.c
```c
/*
 * QTest testcase for RP2040
 */

#include "qemu/osdep.h"
#include "libqtest.h"
#include "qemu/module.h"
#include "libqos/qgraph.h"

/* Test basic boot */
static void test_rp2040_boot(void)
{
    QTestState *qts;
    
    qts = qtest_init("-machine pico");
    
    /* Check CPU is running */
    g_assert_cmpuint(qtest_readl(qts, 0xd0000000), ==, 0); /* SIO CPUID for core 0 */
    
    qtest_quit(qts);
}

/* Test GPIO */
static void test_rp2040_gpio(void)
{
    QTestState *qts;
    
    qts = qtest_init("-machine pico");
    
    /* Set GPIO0 as output */
    qtest_writel(qts, 0xd0000000 + 0x024, 0x1); /* SIO GPIO_OE_SET */
    
    /* Set GPIO0 high */
    qtest_writel(qts, 0xd0000000 + 0x014, 0x1); /* SIO GPIO_OUT_SET */
    
    /* Check it's high */
    g_assert_cmpuint(qtest_readl(qts, 0xd0000000 + 0x010) & 0x1, ==, 1);
    
    qtest_quit(qts);
}

/* Test inter-core FIFO */
static void test_rp2040_fifo(void)
{
    QTestState *qts;
    uint32_t status;
    
    qts = qtest_init("-machine pico");
    
    /* Check FIFO is ready */
    status = qtest_readl(qts, 0xd0000000 + 0x050); /* SIO FIFO_ST */
    g_assert_cmpuint(status & 0x2, ==, 0x2); /* RDY bit */
    
    /* Write to FIFO */
    qtest_writel(qts, 0xd0000000 + 0x054, 0x12345678); /* SIO FIFO_WR */
    
    /* Should now have valid data for other core */
    status = qtest_readl(qts, 0xd0000000 + 0x050);
    g_assert_cmpuint(status & 0x1, ==, 0x1); /* VLD bit */
    
    qtest_quit(qts);
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    
    qtest_add_func("/rp2040/boot", test_rp2040_boot);
    qtest_add_func("/rp2040/gpio", test_rp2040_gpio);
    qtest_add_func("/rp2040/fifo", test_rp2040_fifo);
    
    return g_test_run();
}
```

### tests/qtest/meson.build
```meson
qtests_arm = \
  (config_all_devices.has_key('CONFIG_RP2040') ? ['rp2040-test'] : []) + \
  # ... other ARM tests ...
```

## Documentation

### docs/system/arm/rp2040.rst
```rst
Raspberry Pi RP2040 (``pico``)
===============================

The RP2040 SoC contains a dual Cortex-M0+ processor, 264KB of SRAM,
and a comprehensive set of peripherals including UART, SPI, I2C, ADC,
PWM, and the unique Programmable I/O (PIO) blocks.

QEMU models the following RP2040 peripherals:

- Dual Cortex-M0+ cores at 133MHz
- 264KB multi-bank SRAM
- 2x UART
- 2x SPI
- 2x I2C  
- 2x Programmable I/O blocks
- 1x USB 1.1 controller
- 12-channel DMA controller
- Timer with 4 alarms
- RTC
- ADC with temperature sensor
- 30 GPIO pins

Boot options
------------

The RP2040 machine can boot from:

- Flash XIP (execute-in-place) region using ``-drive`` or ``-blockdev``
- Direct kernel loading using ``-kernel``
- GDB stub for debugging bare-metal code

Example usage::

    $ qemu-system-arm -M pico -kernel firmware.elf

Debugging::

    $ qemu-system-arm -M pico -kernel firmware.elf -s -S
    $ arm-none-eabi-gdb firmware.elf
    (gdb) target remote :1234

Machine-specific options
------------------------

The following machine-specific options are supported:

- ``revision=N`` - Board revision (default 1)

Implementation Notes
--------------------

The PIO (Programmable I/O) blocks are unique to the RP2040 and provide
a way to implement custom digital interfaces. The QEMU implementation
models the PIO instruction execution and state machines.

The boot ROM is not currently implemented - the machine starts executing
from the beginning of the XIP flash region or loaded kernel.
```

## Integration Checklist

1. **Create directory structure** for RP2040 files
2. **Add Kconfig entries** to enable RP2040 support
3. **Update meson.build files** in each subdirectory
4. **Create header files** in include/hw/
5. **Implement core SoC** (rp2040_soc.c)
6. **Add board definition** (rp2040.c for Pico board)
7. **Implement peripherals** incrementally
8. **Add tests** for each peripheral
9. **Write documentation** in docs/system/arm/
10. **Update MAINTAINERS** file to add RP2040 section

## MAINTAINERS Entry

```
Raspberry Pi RP2040
M: Your Name <your.email@example.com>
L: qemu-arm@nongnu.org
S: Maintained
F: hw/*/rp2040*
F: include/hw/*/rp2040*
F: docs/system/arm/rp2040.rst
F: tests/qtest/*rp2040*
F: tests/qtest/pico*
```

This structure follows QEMU's established patterns and makes it easy to incrementally add RP2040 support to the codebase.