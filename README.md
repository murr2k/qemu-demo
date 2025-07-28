# QEMU RP2040 (Raspberry Pi Pico) Emulation

This project implements emulation support for the Raspberry Pi Pico's RP2040 microcontroller in QEMU.

## Changelog

### [1.0.0] - 2025-07-28

#### Added
- Initial release of QEMU RP2040 emulation
- RP2040 SoC implementation with dual ARM Cortex-M0+ cores
- Raspberry Pi Pico board machine model
- Core peripheral implementations:
  - UART (Universal Asynchronous Receiver/Transmitter) with FIFO support
  - GPIO (General Purpose Input/Output) controller with 30 pins
  - Timer with 64-bit microsecond counter and 4 alarm channels
- Memory regions: 16KB ROM, 264KB SRAM, 16MB XIP Flash
- Build system integration (Meson, Kconfig)
- Comprehensive test suite for all implemented peripherals
- Full documentation and quick start guide

#### Known Limitations
- PIO (Programmable I/O) not yet implemented
- Inter-core communication (SIO) not yet implemented
- DMA controller not yet implemented
- SPI, I2C, PWM, ADC, USB, RTC peripherals pending

## Features

### Implemented
- Dual ARM Cortex-M0+ cores (ARMv6-M architecture)
- 264KB SRAM memory
- 16KB Boot ROM
- 16MB XIP flash region
- UART peripherals (2x)
- GPIO controller (30 pins)
- Timer with 4 alarm channels
- Basic interrupt controller (NVIC)

### Not Yet Implemented
- PIO (Programmable I/O) blocks
- DMA controller
- SPI/I2C controllers
- PWM, ADC, RTC
- USB controller
- Inter-core communication (SIO)
- Watchdog timer

## Building QEMU with RP2040 Support

### Prerequisites
- GCC or Clang compiler
- Meson build system (>= 0.55.0)
- Ninja build tool
- Python 3.6+

### Build Instructions

1. Clone QEMU source (this would be integrated into upstream QEMU):
```bash
git clone https://gitlab.com/qemu-project/qemu.git
cd qemu
```

2. Copy the RP2040 implementation files to the QEMU source tree:
```bash
# Copy the files from this demo to the appropriate QEMU directories
cp -r /path/to/qemu-demo/hw/* hw/
cp -r /path/to/qemu-demo/include/* include/
```

3. Configure and build QEMU:
```bash
mkdir build
cd build
../configure --target-list=arm-softmmu --enable-debug
ninja
```

## Usage

### Running a Basic Program

```bash
# Run with a binary file
./qemu-system-arm -machine raspberrypi-pico -kernel program.bin

# Run with an ELF file
./qemu-system-arm -machine raspberrypi-pico -kernel program.elf

# With serial output to console
./qemu-system-arm -machine raspberrypi-pico -kernel program.elf \
    -serial stdio -monitor none

# With GDB debugging
./qemu-system-arm -machine raspberrypi-pico -kernel program.elf \
    -serial stdio -s -S
```

### QEMU Monitor Commands

Connect to QEMU monitor:
```bash
./qemu-system-arm -machine raspberrypi-pico -kernel program.elf \
    -monitor stdio
```

Useful monitor commands:
- `info qtree` - Show device tree
- `info mtree` - Show memory map
- `info irq` - Show interrupt state
- `stop` / `cont` - Pause/resume execution

## Programming for QEMU RP2040

### Memory Map
- `0x00000000` - Boot ROM (16KB)
- `0x10000000` - XIP Flash (16MB)
- `0x20000000` - SRAM (264KB)
- `0x40000000` - APB Peripherals
- `0x50000000` - AHB-Lite Peripherals
- `0xD0000000` - SIO (Single-cycle I/O)
- `0xE0000000` - Cortex-M0+ internal peripherals

### Example: Minimal Blink Program

```c
// blink.c - Minimal LED blink example
#include <stdint.h>

#define GPIO_BASE      0x40014000
#define GPIO25_CTRL    (GPIO_BASE + 0xCC)
#define GPIO25_STATUS  (GPIO_BASE + 0xC8)

#define SIO_BASE       0xD0000000
#define GPIO_OUT       (SIO_BASE + 0x10)
#define GPIO_OUT_SET   (SIO_BASE + 0x14)
#define GPIO_OUT_CLR   (SIO_BASE + 0x18)
#define GPIO_OE_SET    (SIO_BASE + 0x24)

#define TIMER_BASE     0x40054000
#define TIMELR         (TIMER_BASE + 0x0C)

void delay_ms(uint32_t ms) {
    uint32_t start = *(volatile uint32_t*)TIMELR;
    while ((*(volatile uint32_t*)TIMELR - start) < (ms * 1000));
}

int main(void) {
    // Configure GPIO25 as SIO output
    *(volatile uint32_t*)GPIO25_CTRL = 5;  // Function 5 = SIO
    *(volatile uint32_t*)GPIO_OE_SET = (1 << 25);  // Enable output
    
    while (1) {
        *(volatile uint32_t*)GPIO_OUT_SET = (1 << 25);  // LED on
        delay_ms(500);
        *(volatile uint32_t*)GPIO_OUT_CLR = (1 << 25);  // LED off
        delay_ms(500);
    }
    
    return 0;
}
```

### Building Programs

Using ARM GCC toolchain:
```bash
arm-none-eabi-gcc -mcpu=cortex-m0plus -mthumb -O2 -nostdlib \
    -T link.ld -o program.elf program.c startup.s
arm-none-eabi-objcopy -O binary program.elf program.bin
```

## Testing the Implementation

### Unit Tests
Basic peripheral tests are provided in the `tests/` directory:
- UART loopback test
- GPIO input/output test
- Timer alarm test

### Integration Tests
The Pico SDK examples can be used for testing:
```bash
git clone https://github.com/raspberrypi/pico-examples
# Build examples and run in QEMU
```

## Debugging

### GDB Debugging
```bash
# Terminal 1: Start QEMU with GDB server
./qemu-system-arm -machine raspberrypi-pico -kernel program.elf -s -S

# Terminal 2: Connect GDB
arm-none-eabi-gdb program.elf
(gdb) target remote :1234
(gdb) load
(gdb) continue
```

### QEMU Debug Output
Enable debug output for specific peripherals:
```bash
./qemu-system-arm -machine raspberrypi-pico -kernel program.elf \
    -d guest_errors,unimp
```

## Contributing

To contribute to this implementation:
1. Implement missing peripherals (see "Not Yet Implemented" section)
2. Add test cases for new peripherals
3. Update documentation
4. Submit patches to QEMU mailing list

## License

This code is licensed under the GPL version 2 or later, consistent with QEMU licensing.

## References

- [RP2040 Datasheet](https://datasheets.raspberrypi.org/rp2040/rp2040-datasheet.pdf)
- [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk)
- [QEMU Documentation](https://www.qemu.org/documentation/)
- [ARMv6-M Architecture Reference Manual](https://developer.arm.com/documentation/)