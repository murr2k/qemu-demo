# QEMU ARM Cortex-M Architecture Analysis for RP2040 Implementation

## 1. ARM Cortex-M0/M0+ CPU Support in QEMU

### CPU Model Definition
QEMU handles ARM Cortex-M CPUs through the following structure:

```c
// In target/arm/cpu.c or target/arm/cpu_tcg.c
static void cortex_m0_initfn(Object *obj)
{
    ARMCPU *cpu = ARM_CPU(obj);
    
    set_feature(&cpu->env, ARM_FEATURE_V6);
    set_feature(&cpu->env, ARM_FEATURE_M);
    set_feature(&cpu->env, ARM_FEATURE_THUMB_DSP);
    
    cpu->midr = 0x410cc200;  // Cortex-M0 MIDR
    cpu->pmsav7_dregion = 8;  // MPU regions
    cpu->id_pfr0 = 0x00000030;
    cpu->id_pfr1 = 0x00000200;
    // ... other ID registers
}

// CPU registration
static const ARMCPUInfo arm_tcg_cpus[] = {
    { .name = "cortex-m0", .initfn = cortex_m0_initfn },
    { .name = "cortex-m0-plus", .initfn = cortex_m0p_initfn },
    // ...
};
```

### Key CPU Features for Cortex-M0+
- ARMv6-M architecture
- Thumb-2 instruction set only
- No FPU
- Optional MPU (Memory Protection Unit)
- NVIC (Nested Vectored Interrupt Controller) integration
- System Control Block (SCB)

## 2. Structure of Existing Cortex-M Machine Models

### STM32 Example Structure

```c
// In hw/arm/stm32f205_soc.c
typedef struct STM32F205State {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/

    ARMv7MState armv7m;  // Core CPU and NVIC
    
    // Peripherals
    STM32F2XXSyscfgState syscfg;
    STM32F2XXUsartState usart[STM_NUM_USARTS];
    STM32F2XXTimerState timer[STM_NUM_TIMERS];
    STM32F2XXADCState adc[STM_NUM_ADCS];
    STM32F2XXSPIState spi[STM_NUM_SPIS];
    
    MemoryRegion sram;
    MemoryRegion flash;
    MemoryRegion flash_alias;
} STM32F205State;

// SoC initialization
static void stm32f205_soc_realize(DeviceState *dev_soc, Error **errp)
{
    STM32F205State *s = STM32F205_SOC(dev_soc);
    
    // Initialize ARM core
    object_property_set_str(OBJECT(&s->armv7m), "cpu-type", 
                           ARM_CPU_TYPE_NAME("cortex-m3"), &error_abort);
    object_property_set_int(OBJECT(&s->armv7m), "num-irq", 96, &error_abort);
    
    // Memory setup
    memory_region_init_ram(&s->sram, OBJECT(dev_soc), "STM32F205.sram",
                          SRAM_SIZE, &error_fatal);
    memory_region_add_subregion(system_memory, SRAM_BASE_ADDRESS, &s->sram);
    
    // Flash memory
    memory_region_init_rom(&s->flash, OBJECT(dev_soc), "STM32F205.flash",
                          FLASH_SIZE, &error_fatal);
    memory_region_add_subregion(system_memory, FLASH_BASE_ADDRESS, &s->flash);
    
    // Create and map peripherals
    create_unimplemented_device("timer[2]", 0x40000000, 0x400);
    
    // USART setup
    for (i = 0; i < STM_NUM_USARTS; i++) {
        object_initialize_child(OBJECT(dev_soc), "usart[*]", &s->usart[i],
                               TYPE_STM32F2XX_USART);
        qdev_prop_set_chr(DEVICE(&s->usart[i]), "chardev", serial_hd(i));
        sysbus_realize(SYS_BUS_DEVICE(&s->usart[i]), &error_fatal);
        sysbus_mmio_map(SYS_BUS_DEVICE(&s->usart[i]), 0, usart_addr[i]);
        sysbus_connect_irq(SYS_BUS_DEVICE(&s->usart[i]), 0,
                          qdev_get_gpio_in(DEVICE(&s->armv7m), usart_irq[i]));
    }
}
```

## 3. Peripheral Emulation Implementation Patterns

### Typical Peripheral Structure

```c
// In hw/char/stm32f2xx_usart.c
typedef struct {
    /* <private> */
    SysBusDevice parent_obj;
    /* <public> */

    MemoryRegion mmio;
    
    uint32_t usart_sr;
    uint32_t usart_dr;
    uint32_t usart_brr;
    uint32_t usart_cr1;
    uint32_t usart_cr2;
    uint32_t usart_cr3;
    
    CharBackend chr;
    qemu_irq irq;
} STM32F2XXUsartState;

// Memory-mapped I/O operations
static uint64_t stm32f2xx_usart_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
    STM32F2XXUsartState *s = opaque;
    
    switch (addr) {
    case USART_SR:
        return s->usart_sr;
    case USART_DR:
        s->usart_sr &= ~USART_SR_RXNE;
        return s->usart_dr;
    case USART_BRR:
        return s->usart_brr;
    // ... other registers
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                     "%s: Bad offset 0x%" HWADDR_PRIx "\n", __func__, addr);
        return 0;
    }
}

static void stm32f2xx_usart_write(void *opaque, hwaddr addr,
                                  uint64_t val64, unsigned int size)
{
    STM32F2XXUsartState *s = opaque;
    uint32_t value = val64;
    
    switch (addr) {
    case USART_SR:
        s->usart_sr = value;
        break;
    case USART_DR:
        if (s->usart_cr1 & USART_CR1_TE) {
            qemu_chr_fe_write_all(&s->chr, &ch, 1);
        }
        s->usart_sr |= USART_SR_TC;
        break;
    // ... other registers
    }
}

static const MemoryRegionOps stm32f2xx_usart_ops = {
    .read = stm32f2xx_usart_read,
    .write = stm32f2xx_usart_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};
```

## 4. Multi-Core Support Patterns

### For RP2040 Dual-Core Implementation

```c
// Conceptual dual-core SoC structure
typedef struct RP2040State {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/
    
    // Dual Cortex-M0+ cores
    ARMv7MState core[2];
    
    // Shared peripherals
    RP2040SIOState sio;  // Single-cycle I/O block (inter-core FIFO, spinlocks)
    RP2040DMAState dma;
    RP2040PIOState pio[2];
    RP2040UARTState uart[2];
    RP2040SPIState spi[2];
    RP2040I2CState i2c[2];
    RP2040PWMState pwm;
    RP2040ADCState adc;
    RP2040RTCState rtc;
    RP2040TimerState timer;
    RP2040WatchdogState watchdog;
    
    // Memory regions
    MemoryRegion sram[6];  // 6x 64KB SRAM banks
    MemoryRegion rom;      // 16KB boot ROM
    MemoryRegion flash_xip; // Execute-in-place flash region
} RP2040State;

// Multi-core initialization pattern
static void rp2040_realize(DeviceState *dev, Error **errp)
{
    RP2040State *s = RP2040(dev);
    int i;
    
    // Initialize both cores
    for (i = 0; i < 2; i++) {
        object_initialize_child(OBJECT(s), "core[*]", &s->core[i],
                               TYPE_ARMV7M);
        object_property_set_str(OBJECT(&s->core[i]), "cpu-type",
                               ARM_CPU_TYPE_NAME("cortex-m0-plus"), 
                               &error_abort);
        object_property_set_int(OBJECT(&s->core[i]), "num-irq", 32,
                               &error_abort);
        
        // Core-specific memory view
        object_property_set_link(OBJECT(&s->core[i]), "memory",
                                OBJECT(get_system_memory()), &error_abort);
        
        sysbus_realize(SYS_BUS_DEVICE(&s->core[i]), &error_fatal);
    }
    
    // Inter-core communication via SIO
    create_sio_device(s);
}
```

## 5. Memory Mapping and Device Registration

### RP2040 Memory Map Implementation

```c
// Memory map definitions
#define RP2040_ROM_BASE        0x00000000
#define RP2040_ROM_SIZE        0x00004000  // 16KB

#define RP2040_XIP_BASE        0x10000000
#define RP2040_XIP_SIZE        0x10000000  // 256MB

#define RP2040_SRAM_BASE       0x20000000
#define RP2040_SRAM_SIZE       0x00042000  // 264KB total

#define RP2040_APB_BASE        0x40000000
#define RP2040_AHB_BASE        0x50000000

#define RP2040_SIO_BASE        0xd0000000
#define RP2040_PPB_BASE        0xe0000000

// Device registration
static void rp2040_map_peripherals(RP2040State *s, MemoryRegion *sysmem)
{
    // Create peripheral container
    MemoryRegion *periph = g_new(MemoryRegion, 1);
    memory_region_init(periph, OBJECT(s), "rp2040-peripherals", 
                      RP2040_APB_BASE + 0x10000000);
    
    // Map individual peripherals
    memory_region_add_subregion(periph, RP2040_UART0_BASE - RP2040_APB_BASE,
                               sysbus_mmio_get_region(SYS_BUS_DEVICE(&s->uart[0]), 0));
    
    memory_region_add_subregion(periph, RP2040_SPI0_BASE - RP2040_APB_BASE,
                               sysbus_mmio_get_region(SYS_BUS_DEVICE(&s->spi[0]), 0));
    
    // Add container to system memory
    memory_region_add_subregion(sysmem, RP2040_APB_BASE, periph);
    
    // Create unimplemented device placeholders
    create_unimplemented_device("rp2040.xosc", 0x40024000, 0x1000);
    create_unimplemented_device("rp2040.pll_sys", 0x40028000, 0x1000);
}
```

## 6. Build System Integration

### Meson Build Configuration

```meson
# In hw/arm/meson.build
arm_ss.add(when: 'CONFIG_RP2040', if_true: files(
  'rp2040.c',
  'rp2040_soc.c',
))

# In hw/char/meson.build  
softmmu_ss.add(when: 'CONFIG_RP2040', if_true: files('rp2040_uart.c'))

# In hw/ssi/meson.build
softmmu_ss.add(when: 'CONFIG_RP2040', if_true: files('rp2040_spi.c'))

# In hw/misc/meson.build
softmmu_ss.add(when: 'CONFIG_RP2040', if_true: files(
  'rp2040_sio.c',
  'rp2040_pio.c',
))
```

### Kconfig Configuration

```kconfig
# In hw/arm/Kconfig
config RP2040
    bool
    select ARM_V7M
    select UNIMP
    
config RASPBERRY_PI_PICO
    bool
    default y
    depends on TCG && ARM
    select RP2040
```

## Key Implementation Patterns to Follow

### 1. SysBusDevice Pattern
All peripherals should inherit from SysBusDevice for proper MMIO and IRQ handling.

### 2. QOM (QEMU Object Model)
Use proper QOM type definitions and initialization:
```c
#define TYPE_RP2040_UART "rp2040-uart"
OBJECT_DECLARE_SIMPLE_TYPE(RP2040UARTState, RP2040_UART)
```

### 3. Memory Region Operations
Define read/write operations with proper logging for unimplemented features:
```c
static const MemoryRegionOps rp2040_uart_ops = {
    .read = rp2040_uart_read,
    .write = rp2040_uart_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};
```

### 4. Reset Handling
Implement proper reset functions:
```c
static void rp2040_uart_reset(DeviceState *dev)
{
    RP2040UARTState *s = RP2040_UART(dev);
    
    s->ctrl = 0;
    s->status = UART_STATUS_TXFE | UART_STATUS_RXFE;
    // ... reset all registers
}
```

### 5. Migration Support
Add VMState descriptions for save/restore:
```c
static const VMStateDescription vmstate_rp2040_uart = {
    .name = "rp2040_uart",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(ctrl, RP2040UARTState),
        VMSTATE_UINT32(status, RP2040UARTState),
        // ... other fields
        VMSTATE_END_OF_LIST()
    }
};
```

## Recommended Implementation Order

1. **Basic SoC structure** with single core
2. **Memory mapping** (ROM, SRAM, peripherals)
3. **UART** for basic I/O
4. **GPIO** for digital I/O
5. **Timer** for delays and timing
6. **SIO** for inter-core communication
7. **Second core** enablement
8. **PIO** (Programmable I/O) - unique to RP2040
9. **DMA** controller
10. **SPI/I2C** interfaces
11. **ADC, PWM, RTC** and other peripherals

This architecture provides a solid foundation for implementing RP2040 emulation in QEMU while following established patterns and best practices.