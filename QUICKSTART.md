# Quick Start Guide - QEMU RP2040 Demo

This guide helps you quickly get started with the QEMU RP2040 (Raspberry Pi Pico) emulation.

## Overview

This implementation provides:
- RP2040 SoC emulation with dual Cortex-M0+ cores
- Basic peripherals: UART, GPIO, Timer
- Test programs demonstrating peripheral functionality

## Project Structure

```
qemu-demo/
├── hw/                    # Hardware emulation source
│   ├── arm/              # ARM board and SoC implementations
│   │   ├── rp2040.c      # RP2040 SoC
│   │   └── raspberrypi-pico.c  # Pico board
│   ├── char/             # Character devices
│   │   └── rp2040_uart.c # UART implementation
│   ├── gpio/             # GPIO devices
│   │   └── rp2040_gpio.c # GPIO implementation
│   └── timer/            # Timer devices
│       └── rp2040_timer.c # Timer implementation
├── include/              # Header files
│   └── hw/
│       ├── arm/
│       │   └── rp2040.h
│       ├── char/
│       │   └── rp2040_uart.h
│       ├── gpio/
│       │   └── rp2040_gpio.h
│       └── timer/
│           └── rp2040_timer.h
├── tests/                # Test programs
│   └── rp2040/
│       ├── test_uart.c   # UART test
│       ├── test_gpio.c   # GPIO test
│       └── test_timer.c  # Timer test
└── README.md             # Full documentation

```

## Quick Test (Without Building QEMU)

If you want to understand the implementation:

1. **Review the SoC implementation:**
   ```bash
   cat hw/arm/rp2040.c
   ```

2. **Check peripheral implementations:**
   ```bash
   cat hw/char/rp2040_uart.c
   cat hw/gpio/rp2040_gpio.c
   cat hw/timer/rp2040_timer.c
   ```

3. **Look at test programs:**
   ```bash
   cat tests/rp2040/test_uart.c
   ```

## Integration with QEMU

To use this in a real QEMU build:

1. **Copy files to QEMU source tree:**
   ```bash
   cp -r hw/* /path/to/qemu/hw/
   cp -r include/* /path/to/qemu/include/
   ```

2. **Configure and build QEMU:**
   ```bash
   cd /path/to/qemu
   mkdir build && cd build
   ../configure --target-list=arm-softmmu
   make -j$(nproc)
   ```

3. **Run QEMU with RP2040:**
   ```bash
   ./qemu-system-arm -machine raspberrypi-pico \
       -kernel test.elf -serial stdio
   ```

## Building Test Programs

1. **Install ARM toolchain:**
   ```bash
   # Ubuntu/Debian
   sudo apt install gcc-arm-none-eabi
   
   # macOS
   brew install arm-none-eabi-gcc
   ```

2. **Build tests:**
   ```bash
   cd tests/rp2040
   make all
   ```

3. **Run tests (requires built QEMU):**
   ```bash
   make run-test_uart
   make run-test_gpio
   make run-test_timer
   ```

## Key Implementation Details

### Memory Map
- `0x00000000`: Boot ROM (16KB)
- `0x10000000`: Flash/XIP (16MB)
- `0x20000000`: SRAM (264KB)
- `0x40034000`: UART0
- `0x40014000`: GPIO
- `0x40054000`: Timer

### Implemented Features
- ✅ Basic CPU emulation (Cortex-M0+)
- ✅ Memory regions (ROM, RAM, Flash)
- ✅ UART (serial console)
- ✅ GPIO (30 pins)
- ✅ Timer (microsecond counter, 4 alarms)
- ❌ PIO (Programmable I/O)
- ❌ DMA controller
- ❌ SPI/I2C
- ❌ USB

## Next Steps

1. **Extend the implementation:**
   - Add PIO support (unique RP2040 feature)
   - Implement dual-core functionality
   - Add DMA controller

2. **Create more examples:**
   - Blink LED program
   - Multi-core communication
   - Interrupt handling demo

3. **Upstream contribution:**
   - Clean up code for QEMU coding standards
   - Add proper documentation
   - Submit patches to qemu-devel mailing list

## Troubleshooting

**Q: How do I see what's implemented?**
```bash
grep -r "unimplemented_device" hw/arm/rp2040.c
```

**Q: How do I enable debug output?**
```bash
qemu-system-arm -machine raspberrypi-pico -d guest_errors,unimp
```

**Q: Where's the boot sequence?**
Check `hw/arm/rp2040.c` - boot starts at 0x10000000 (XIP flash).

## Resources

- [RP2040 Datasheet](https://datasheets.raspberrypi.org/rp2040/rp2040-datasheet.pdf)
- [QEMU Documentation](https://www.qemu.org/docs/master/)
- [Pico SDK](https://github.com/raspberrypi/pico-sdk)