/*
 * Raspberry Pi RP2040 SoC emulation - Implementation Example
 *
 * This demonstrates the core patterns for implementing RP2040 in QEMU
 * based on existing ARM Cortex-M implementations like STM32.
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/arm/boot.h"
#include "hw/sysbus.h"
#include "hw/arm/armv7m.h"
#include "hw/qdev-properties.h"
#include "hw/misc/unimp.h"
#include "hw/char/serial.h"
#include "sysemu/sysemu.h"

/* RP2040 Memory Map */
#define RP2040_ROM_BASE        0x00000000
#define RP2040_ROM_SIZE        0x00004000  /* 16KB Boot ROM */

#define RP2040_XIP_BASE        0x10000000  /* Execute in place from flash */
#define RP2040_XIP_SIZE        0x10000000  /* 256MB addressable */

#define RP2040_SRAM_BASE       0x20000000
#define RP2040_SRAM_SIZE       0x00042000  /* 264KB total (6x 64KB banks) */
#define RP2040_SRAM_BANK_SIZE  0x00010000  /* 64KB per bank */

/* Peripherals */
#define RP2040_APB_BASE        0x40000000
#define RP2040_SYSINFO_BASE    0x40000000
#define RP2040_SYSCFG_BASE     0x40004000
#define RP2040_CLOCKS_BASE     0x40008000
#define RP2040_RESETS_BASE     0x4000c000
#define RP2040_PSM_BASE        0x40010000
#define RP2040_IO_BANK0_BASE   0x40014000
#define RP2040_IO_QSPI_BASE    0x40018000
#define RP2040_PADS_BANK0_BASE 0x4001c000
#define RP2040_PADS_QSPI_BASE  0x40020000
#define RP2040_XOSC_BASE       0x40024000
#define RP2040_PLL_SYS_BASE    0x40028000
#define RP2040_PLL_USB_BASE    0x4002c000
#define RP2040_BUSCTRL_BASE    0x40030000
#define RP2040_UART0_BASE      0x40034000
#define RP2040_UART1_BASE      0x40038000
#define RP2040_SPI0_BASE       0x4003c000
#define RP2040_SPI1_BASE       0x40040000
#define RP2040_I2C0_BASE       0x40044000
#define RP2040_I2C1_BASE       0x40048000
#define RP2040_ADC_BASE        0x4004c000
#define RP2040_PWM_BASE        0x40050000
#define RP2040_TIMER_BASE      0x40054000
#define RP2040_WATCHDOG_BASE   0x40058000
#define RP2040_RTC_BASE        0x4005c000
#define RP2040_ROSC_BASE       0x40060000
#define RP2040_VREG_BASE       0x40064000
#define RP2040_TBMAN_BASE      0x4006c000

#define RP2040_DMA_BASE        0x50000000
#define RP2040_USBCTRL_BASE    0x50100000
#define RP2040_PIO0_BASE       0x50200000
#define RP2040_PIO1_BASE       0x50300000
#define RP2040_XIP_AUX_BASE    0x50400000

#define RP2040_SIO_BASE        0xd0000000  /* Single-cycle I/O */
#define RP2040_PPB_BASE        0xe0000000  /* Cortex-M0+ internal peripherals */

/* RP2040 SoC State Structure */
#define TYPE_RP2040_SOC "rp2040-soc"
OBJECT_DECLARE_SIMPLE_TYPE(RP2040State, RP2040_SOC)

struct RP2040State {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/
    
    /* Dual Cortex-M0+ cores */
    ARMv7MState core[2];
    
    /* Memory regions */
    MemoryRegion rom;
    MemoryRegion sram[6];  /* 6x 64KB banks */
    MemoryRegion xip;
    MemoryRegion container;
    
    /* Core peripherals */
    /* TODO: Add peripheral state structures as they're implemented */
    
    /* Properties */
    char *cpu_type;
    uint32_t sram_size;
};

/* RP2040 UART Stub - Minimal Implementation */
#define TYPE_RP2040_UART "rp2040-uart"
OBJECT_DECLARE_SIMPLE_TYPE(RP2040UARTState, RP2040_UART)

typedef struct RP2040UARTState {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/
    
    MemoryRegion mmio;
    CharBackend chr;
    qemu_irq irq;
    
    /* Registers */
    uint32_t dr;      /* Data register */
    uint32_t rsr;     /* Receive status */
    uint32_t fr;      /* Flag register */
    uint32_t ilpr;    /* IrDA low-power counter */
    uint32_t ibrd;    /* Integer baud rate */
    uint32_t fbrd;    /* Fractional baud rate */
    uint32_t lcr_h;   /* Line control */
    uint32_t cr;      /* Control register */
    uint32_t ifls;    /* Interrupt FIFO level select */
    uint32_t imsc;    /* Interrupt mask */
    uint32_t ris;     /* Raw interrupt status */
    uint32_t mis;     /* Masked interrupt status */
    uint32_t icr;     /* Interrupt clear */
    uint32_t dmacr;   /* DMA control */
} RP2040UARTState;

/* UART Register Offsets */
#define UART_DR     0x000
#define UART_RSR    0x004
#define UART_FR     0x018
#define UART_ILPR   0x020
#define UART_IBRD   0x024
#define UART_FBRD   0x028
#define UART_LCR_H  0x02C
#define UART_CR     0x030
#define UART_IFLS   0x034
#define UART_IMSC   0x038
#define UART_RIS    0x03C
#define UART_MIS    0x040
#define UART_ICR    0x044
#define UART_DMACR  0x048

/* UART Flag Register Bits */
#define UART_FR_RXFE    (1 << 4)  /* Receive FIFO empty */
#define UART_FR_TXFF    (1 << 5)  /* Transmit FIFO full */
#define UART_FR_RXFF    (1 << 6)  /* Receive FIFO full */
#define UART_FR_TXFE    (1 << 7)  /* Transmit FIFO empty */
#define UART_FR_BUSY    (1 << 3)  /* UART busy */

/* UART Control Register Bits */
#define UART_CR_UARTEN  (1 << 0)  /* UART enable */
#define UART_CR_TXE     (1 << 8)  /* Transmit enable */
#define UART_CR_RXE     (1 << 9)  /* Receive enable */

static uint64_t rp2040_uart_read(void *opaque, hwaddr offset, unsigned size)
{
    RP2040UARTState *s = opaque;
    
    switch (offset) {
    case UART_DR:
        s->fr |= UART_FR_RXFE;
        return s->dr;
    case UART_FR:
        return s->fr;
    case UART_CR:
        return s->cr;
    case UART_IMSC:
        return s->imsc;
    /* Add other registers as needed */
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                     "rp2040_uart: Bad offset 0x%" HWADDR_PRIx "\n", offset);
        return 0;
    }
}

static void rp2040_uart_write(void *opaque, hwaddr offset,
                              uint64_t value, unsigned size)
{
    RP2040UARTState *s = opaque;
    unsigned char ch;
    
    switch (offset) {
    case UART_DR:
        if (s->cr & UART_CR_UARTEN && s->cr & UART_CR_TXE) {
            ch = value;
            qemu_chr_fe_write_all(&s->chr, &ch, 1);
        }
        break;
    case UART_CR:
        s->cr = value;
        break;
    case UART_IMSC:
        s->imsc = value;
        break;
    /* Add other registers as needed */
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                     "rp2040_uart: Bad offset 0x%" HWADDR_PRIx "\n", offset);
    }
}

static const MemoryRegionOps rp2040_uart_ops = {
    .read = rp2040_uart_read,
    .write = rp2040_uart_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static void rp2040_uart_init(Object *obj)
{
    RP2040UARTState *s = RP2040_UART(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    
    memory_region_init_io(&s->mmio, obj, &rp2040_uart_ops, s,
                          TYPE_RP2040_UART, 0x1000);
    sysbus_init_mmio(sbd, &s->mmio);
    sysbus_init_irq(sbd, &s->irq);
}

static void rp2040_uart_realize(DeviceState *dev, Error **errp)
{
    RP2040UARTState *s = RP2040_UART(dev);
    
    /* Initialize default register values */
    s->fr = UART_FR_TXFE | UART_FR_RXFE;
}

/* RP2040 SoC Implementation */
static void rp2040_soc_init(Object *obj)
{
    RP2040State *s = RP2040_SOC(obj);
    int i;
    
    /* Initialize dual Cortex-M0+ cores */
    for (i = 0; i < 2; i++) {
        object_initialize_child(obj, "core[*]", &s->core[i], TYPE_ARMV7M);
    }
    
    /* Initialize memory regions */
    memory_region_init(&s->container, obj, "rp2040-container", UINT64_MAX);
    
    /* TODO: Initialize peripherals as child objects */
}

static void rp2040_soc_realize(DeviceState *dev, Error **errp)
{
    RP2040State *s = RP2040_SOC(dev);
    MemoryRegion *system_memory = get_system_memory();
    Error *err = NULL;
    int i;
    
    /* Configure and realize CPU cores */
    for (i = 0; i < 2; i++) {
        char *core_name = g_strdup_printf("core%d", i);
        
        /* Configure the ARMv7M object (Cortex-M0+) */
        object_property_set_str(OBJECT(&s->core[i]), "cpu-type", 
                               s->cpu_type, &err);
        if (err) {
            error_propagate(errp, err);
            return;
        }
        
        /* RP2040 has 26 external interrupts + 10 internal = 36 total */
        object_property_set_int(OBJECT(&s->core[i]), "num-irq", 36, &err);
        if (err) {
            error_propagate(errp, err);
            return;
        }
        
        /* Each core sees the same memory map */
        object_property_set_link(OBJECT(&s->core[i]), "memory",
                                OBJECT(system_memory), &error_abort);
        
        /* Start second core in WFE state */
        if (i == 1) {
            object_property_set_bool(OBJECT(&s->core[i]), "start-powered-off",
                                    true, &error_abort);
        }
        
        /* Realize the core */
        sysbus_realize(SYS_BUS_DEVICE(&s->core[i]), &err);
        if (err) {
            error_propagate(errp, err);
            return;
        }
        
        g_free(core_name);
    }
    
    /* Boot ROM */
    memory_region_init_rom(&s->rom, OBJECT(dev), "rp2040.rom",
                          RP2040_ROM_SIZE, &err);
    if (err) {
        error_propagate(errp, err);
        return;
    }
    memory_region_add_subregion(system_memory, RP2040_ROM_BASE, &s->rom);
    
    /* SRAM Banks - 6x 64KB */
    for (i = 0; i < 6; i++) {
        char *name = g_strdup_printf("rp2040.sram%d", i);
        memory_region_init_ram(&s->sram[i], OBJECT(dev), name,
                              RP2040_SRAM_BANK_SIZE, &err);
        if (err) {
            error_propagate(errp, err);
            return;
        }
        memory_region_add_subregion(system_memory,
                                   RP2040_SRAM_BASE + (i * RP2040_SRAM_BANK_SIZE),
                                   &s->sram[i]);
        g_free(name);
    }
    
    /* XIP (Execute In Place) region for external flash */
    memory_region_init_rom(&s->xip, OBJECT(dev), "rp2040.xip",
                          16 * MiB, &err);  /* Default 16MB flash */
    if (err) {
        error_propagate(errp, err);
        return;
    }
    memory_region_add_subregion(system_memory, RP2040_XIP_BASE, &s->xip);
    
    /* Create unimplemented devices for now */
    create_unimplemented_device("rp2040.sysinfo", RP2040_SYSINFO_BASE, 0x1000);
    create_unimplemented_device("rp2040.syscfg", RP2040_SYSCFG_BASE, 0x1000);
    create_unimplemented_device("rp2040.clocks", RP2040_CLOCKS_BASE, 0x1000);
    create_unimplemented_device("rp2040.resets", RP2040_RESETS_BASE, 0x1000);
    create_unimplemented_device("rp2040.io_bank0", RP2040_IO_BANK0_BASE, 0x1000);
    create_unimplemented_device("rp2040.pads_bank0", RP2040_PADS_BANK0_BASE, 0x1000);
    create_unimplemented_device("rp2040.xosc", RP2040_XOSC_BASE, 0x1000);
    create_unimplemented_device("rp2040.pll_sys", RP2040_PLL_SYS_BASE, 0x1000);
    create_unimplemented_device("rp2040.pll_usb", RP2040_PLL_USB_BASE, 0x1000);
    create_unimplemented_device("rp2040.timer", RP2040_TIMER_BASE, 0x1000);
    create_unimplemented_device("rp2040.watchdog", RP2040_WATCHDOG_BASE, 0x1000);
    create_unimplemented_device("rp2040.rtc", RP2040_RTC_BASE, 0x1000);
    create_unimplemented_device("rp2040.dma", RP2040_DMA_BASE, 0x1000);
    create_unimplemented_device("rp2040.pio0", RP2040_PIO0_BASE, 0x1000);
    create_unimplemented_device("rp2040.pio1", RP2040_PIO1_BASE, 0x1000);
    create_unimplemented_device("rp2040.sio", RP2040_SIO_BASE, 0x1000);
    
    /* TODO: Add actual peripheral implementations */
}

static Property rp2040_soc_properties[] = {
    DEFINE_PROP_STRING("cpu-type", RP2040State, cpu_type),
    DEFINE_PROP_UINT32("sram-size", RP2040State, sram_size, RP2040_SRAM_SIZE),
    DEFINE_PROP_END_OF_LIST(),
};

static void rp2040_soc_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    
    dc->realize = rp2040_soc_realize;
    device_class_set_props(dc, rp2040_soc_properties);
    dc->desc = "Raspberry Pi RP2040 SoC";
}

static void rp2040_uart_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    
    dc->realize = rp2040_uart_realize;
    dc->desc = "RP2040 UART";
}

/* Machine Definition - Raspberry Pi Pico Board */
#define TYPE_PICO_MACHINE "pico-machine"
OBJECT_DECLARE_SIMPLE_TYPE(PicoMachineState, PICO_MACHINE)

typedef struct PicoMachineState {
    MachineState parent_obj;
    RP2040State soc;
} PicoMachineState;

static void pico_machine_init(MachineState *machine)
{
    PicoMachineState *s = PICO_MACHINE(machine);
    
    /* Initialize SoC */
    object_initialize_child(OBJECT(machine), "soc", &s->soc, TYPE_RP2040_SOC);
    object_property_set_str(OBJECT(&s->soc), "cpu-type",
                           ARM_CPU_TYPE_NAME("cortex-m0"), &error_abort);
    sysbus_realize(SYS_BUS_DEVICE(&s->soc), &error_abort);
    
    /* Load firmware if provided */
    if (machine->firmware) {
        /* Load into XIP flash region */
        if (load_image_targphys(machine->firmware, RP2040_XIP_BASE,
                               16 * MiB) < 0) {
            error_report("Failed to load firmware '%s'", machine->firmware);
            exit(1);
        }
    }
}

static void pico_machine_class_init(ObjectClass *oc, void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);
    
    mc->desc = "Raspberry Pi Pico (RP2040)";
    mc->init = pico_machine_init;
    mc->max_cpus = 2;
    mc->default_cpu_type = ARM_CPU_TYPE_NAME("cortex-m0");
}

/* Type Registration */
static const TypeInfo rp2040_soc_types[] = {
    {
        .name           = TYPE_RP2040_SOC,
        .parent         = TYPE_SYS_BUS_DEVICE,
        .instance_size  = sizeof(RP2040State),
        .instance_init  = rp2040_soc_init,
        .class_init     = rp2040_soc_class_init,
    }, {
        .name           = TYPE_RP2040_UART,
        .parent         = TYPE_SYS_BUS_DEVICE,
        .instance_size  = sizeof(RP2040UARTState),
        .instance_init  = rp2040_uart_init,
        .class_init     = rp2040_uart_class_init,
    }, {
        .name           = TYPE_PICO_MACHINE,
        .parent         = TYPE_MACHINE,
        .instance_size  = sizeof(PicoMachineState),
        .class_init     = pico_machine_class_init,
    }
};

DEFINE_TYPES(rp2040_soc_types)