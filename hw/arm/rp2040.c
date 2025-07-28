/*
 * Raspberry Pi RP2040 SoC emulation
 *
 * Copyright (c) 2025 QEMU RP2040 Development Team
 *
 * This code is licensed under the GPL version 2 or later.
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/arm/boot.h"
#include "hw/arm/armv7m.h"
#include "hw/boards.h"
#include "hw/misc/unimp.h"
#include "hw/char/pl011.h"
#include "hw/sysbus.h"
#include "hw/qdev-properties.h"
#include "sysemu/sysemu.h"
#include "hw/char/rp2040_uart.h"
#include "hw/gpio/rp2040_gpio.h"
#include "hw/timer/rp2040_timer.h"

#define TYPE_RP2040_SOC "rp2040-soc"
OBJECT_DECLARE_SIMPLE_TYPE(RP2040State, RP2040_SOC)

#define RP2040_NUM_CORES 2

/* Memory map from RP2040 datasheet */
#define RP2040_ROM_BASE         0x00000000
#define RP2040_ROM_SIZE         (16 * 1024)  /* 16KB */

#define RP2040_XIP_BASE         0x10000000
#define RP2040_XIP_SIZE         (16 * 1024 * 1024)  /* 16MB max */

#define RP2040_SRAM_BASE        0x20000000
#define RP2040_SRAM_SIZE        (264 * 1024)  /* 264KB total */

#define RP2040_APB_BASE         0x40000000
#define RP2040_AHB_BASE         0x50000000

#define RP2040_SIO_BASE         0xD0000000
#define RP2040_PPB_BASE         0xE0000000

/* APB Peripherals */
#define RP2040_SYSINFO_BASE     0x40000000
#define RP2040_SYSCFG_BASE      0x40004000
#define RP2040_CLOCKS_BASE      0x40008000
#define RP2040_RESETS_BASE      0x4000C000
#define RP2040_PSM_BASE         0x40010000
#define RP2040_IO_BANK0_BASE    0x40014000
#define RP2040_IO_QSPI_BASE     0x40018000
#define RP2040_PADS_BANK0_BASE  0x4001C000
#define RP2040_PADS_QSPI_BASE   0x40020000
#define RP2040_XOSC_BASE        0x40024000
#define RP2040_PLL_SYS_BASE     0x40028000
#define RP2040_PLL_USB_BASE     0x4002C000
#define RP2040_BUSCTRL_BASE     0x40030000
#define RP2040_UART0_BASE       0x40034000
#define RP2040_UART1_BASE       0x40038000
#define RP2040_SPI0_BASE        0x4003C000
#define RP2040_SPI1_BASE        0x40040000
#define RP2040_I2C0_BASE        0x40044000
#define RP2040_I2C1_BASE        0x40048000
#define RP2040_ADC_BASE         0x4004C000
#define RP2040_PWM_BASE         0x40050000
#define RP2040_TIMER_BASE       0x40054000
#define RP2040_WATCHDOG_BASE    0x40058000
#define RP2040_RTC_BASE         0x4005C000
#define RP2040_ROSC_BASE        0x40060000
#define RP2040_VREG_CHIP_RESET_BASE 0x40064000
#define RP2040_TBMAN_BASE       0x4006C000

/* AHB-Lite Peripherals */
#define RP2040_DMA_BASE         0x50000000
#define RP2040_USBCTRL_BASE     0x50100000
#define RP2040_PIO0_BASE        0x50200000
#define RP2040_PIO1_BASE        0x50300000
#define RP2040_XIP_AUX_BASE     0x50400000

/* NVIC IRQ assignments */
#define RP2040_TIMER_IRQ_0      0
#define RP2040_TIMER_IRQ_1      1
#define RP2040_TIMER_IRQ_2      2
#define RP2040_TIMER_IRQ_3      3
#define RP2040_PWM_IRQ_WRAP     4
#define RP2040_USBCTRL_IRQ      5
#define RP2040_XIP_IRQ          6
#define RP2040_PIO0_IRQ_0       7
#define RP2040_PIO0_IRQ_1       8
#define RP2040_PIO1_IRQ_0       9
#define RP2040_PIO1_IRQ_1       10
#define RP2040_DMA_IRQ_0        11
#define RP2040_DMA_IRQ_1        12
#define RP2040_IO_IRQ_BANK0     13
#define RP2040_IO_IRQ_QSPI      14
#define RP2040_SIO_IRQ_PROC0    15
#define RP2040_SIO_IRQ_PROC1    16
#define RP2040_CLOCKS_IRQ       17
#define RP2040_SPI0_IRQ         18
#define RP2040_SPI1_IRQ         19
#define RP2040_UART0_IRQ        20
#define RP2040_UART1_IRQ        21
#define RP2040_ADC_IRQ_FIFO     22
#define RP2040_I2C0_IRQ         23
#define RP2040_I2C1_IRQ         24
#define RP2040_RTC_IRQ          25

typedef struct RP2040State {
    SysBusDevice parent_obj;

    ARMv7MState cpu[RP2040_NUM_CORES];
    
    MemoryRegion rom;
    MemoryRegion sram;
    MemoryRegion xip;
    MemoryRegion peripherals;
    
    /* Core peripherals */
    RP2040UARTState uart[2];
    RP2040GPIOState gpio;
    RP2040TimerState timer;
    
    uint32_t num_cpus;
} RP2040State;

static void rp2040_soc_init(Object *obj)
{
    RP2040State *s = RP2040_SOC(obj);
    
    /* Initialize CPU cores */
    for (int i = 0; i < RP2040_NUM_CORES; i++) {
        object_initialize_child(obj, 
                               i == 0 ? "cpu0" : "cpu1", 
                               &s->cpu[i], 
                               TYPE_ARMV7M);
    }
    
    /* Initialize memory regions */
    memory_region_init_rom(&s->rom, obj, "rp2040.rom", 
                          RP2040_ROM_SIZE, &error_fatal);
    memory_region_init_ram(&s->sram, obj, "rp2040.sram", 
                          RP2040_SRAM_SIZE, &error_fatal);
    memory_region_init_ram(&s->xip, obj, "rp2040.xip", 
                          RP2040_XIP_SIZE, &error_fatal);
                          
    /* Initialize peripherals */
    object_initialize_child(obj, "uart0", &s->uart[0], TYPE_RP2040_UART);
    object_initialize_child(obj, "uart1", &s->uart[1], TYPE_RP2040_UART);
    object_initialize_child(obj, "gpio", &s->gpio, TYPE_RP2040_GPIO);
    object_initialize_child(obj, "timer", &s->timer, TYPE_RP2040_TIMER);
}

static void rp2040_soc_realize(DeviceState *dev_soc, Error **errp)
{
    RP2040State *s = RP2040_SOC(dev_soc);
    SysBusDevice *sbd;
    Error *err = NULL;
    
    /* Configure and realize CPU cores */
    for (int i = 0; i < s->num_cpus; i++) {
        DeviceState *cpu_dev = DEVICE(&s->cpu[i]);
        
        /* Configure CPU properties */
        qdev_prop_set_uint32(cpu_dev, "num-irq", 32);
        qdev_prop_set_string(cpu_dev, "cpu-type", ARM_CPU_TYPE_NAME("cortex-m0"));
        qdev_prop_set_bit(cpu_dev, "enable-bitband", false);
        
        /* Set initial PC and SP from vector table */
        object_property_set_link(OBJECT(&s->cpu[i]), "memory",
                                OBJECT(get_system_memory()), &error_abort);
        
        sysbus_realize(SYS_BUS_DEVICE(&s->cpu[i]), &err);
        if (err) {
            error_propagate(errp, err);
            return;
        }
    }
    
    /* Map memories */
    memory_region_add_subregion(get_system_memory(), 
                               RP2040_ROM_BASE, &s->rom);
    memory_region_add_subregion(get_system_memory(), 
                               RP2040_SRAM_BASE, &s->sram);
    memory_region_add_subregion(get_system_memory(), 
                               RP2040_XIP_BASE, &s->xip);
    
    /* Realize and connect peripherals */
    
    /* UART0 */
    sysbus_realize(SYS_BUS_DEVICE(&s->uart[0]), &err);
    if (err) {
        error_propagate(errp, err);
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->uart[0]), 0, RP2040_UART0_BASE);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->uart[0]), 0,
                      qdev_get_gpio_in(DEVICE(&s->cpu[0]), RP2040_UART0_IRQ));
    
    /* UART1 */
    sysbus_realize(SYS_BUS_DEVICE(&s->uart[1]), &err);
    if (err) {
        error_propagate(errp, err);
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->uart[1]), 0, RP2040_UART1_BASE);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->uart[1]), 0,
                      qdev_get_gpio_in(DEVICE(&s->cpu[0]), RP2040_UART1_IRQ));
    
    /* GPIO */
    sysbus_realize(SYS_BUS_DEVICE(&s->gpio), &err);
    if (err) {
        error_propagate(errp, err);
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->gpio), 0, RP2040_IO_BANK0_BASE);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->gpio), 0,
                      qdev_get_gpio_in(DEVICE(&s->cpu[0]), RP2040_IO_IRQ_BANK0));
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->gpio), 1,
                      qdev_get_gpio_in(DEVICE(&s->cpu[1]), RP2040_IO_IRQ_BANK0));
    
    /* Timer */
    sysbus_realize(SYS_BUS_DEVICE(&s->timer), &err);
    if (err) {
        error_propagate(errp, err);
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->timer), 0, RP2040_TIMER_BASE);
    for (int i = 0; i < 4; i++) {
        sysbus_connect_irq(SYS_BUS_DEVICE(&s->timer), i,
                          qdev_get_gpio_in(DEVICE(&s->cpu[0]), RP2040_TIMER_IRQ_0 + i));
    }
    
    /* Create unimplemented device regions for remaining peripherals */
    create_unimplemented_device("rp2040.sysinfo", 
                               RP2040_SYSINFO_BASE, 0x1000);
    create_unimplemented_device("rp2040.syscfg", 
                               RP2040_SYSCFG_BASE, 0x1000);
    create_unimplemented_device("rp2040.clocks", 
                               RP2040_CLOCKS_BASE, 0x1000);
    create_unimplemented_device("rp2040.resets", 
                               RP2040_RESETS_BASE, 0x1000);
    create_unimplemented_device("rp2040.psm", 
                               RP2040_PSM_BASE, 0x1000);
    create_unimplemented_device("rp2040.pads_bank0", 
                               RP2040_PADS_BANK0_BASE, 0x1000);
    create_unimplemented_device("rp2040.watchdog", 
                               RP2040_WATCHDOG_BASE, 0x1000);
    create_unimplemented_device("rp2040.sio", 
                               RP2040_SIO_BASE, 0x1000);
}

static Property rp2040_soc_properties[] = {
    DEFINE_PROP_UINT32("num-cpus", RP2040State, num_cpus, RP2040_NUM_CORES),
    DEFINE_PROP_END_OF_LIST(),
};

static void rp2040_soc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    
    dc->realize = rp2040_soc_realize;
    device_class_set_props(dc, rp2040_soc_properties);
}

static const TypeInfo rp2040_soc_info = {
    .name          = TYPE_RP2040_SOC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(RP2040State),
    .instance_init = rp2040_soc_init,
    .class_init    = rp2040_soc_class_init,
};

static void rp2040_soc_register_types(void)
{
    type_register_static(&rp2040_soc_info);
}

type_init(rp2040_soc_register_types)