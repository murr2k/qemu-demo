/*
 * Raspberry Pi Pico board emulation
 *
 * Copyright (c) 2025 QEMU RP2040 Development Team
 *
 * This code is licensed under the GPL version 2 or later.
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/boards.h"
#include "hw/qdev-properties.h"
#include "hw/arm/boot.h"
#include "exec/address-spaces.h"
#include "hw/arm/rp2040.h"

#define TYPE_PICO_MACHINE MACHINE_TYPE_NAME("raspberrypi-pico")
OBJECT_DECLARE_SIMPLE_TYPE(PicoMachineState, PICO_MACHINE)

typedef struct PicoMachineState {
    MachineState parent_obj;
    RP2040State soc;
} PicoMachineState;

static void pico_init(MachineState *machine)
{
    PicoMachineState *s = PICO_MACHINE(machine);
    
    /* Initialize SoC */
    object_initialize_child(OBJECT(machine), "soc", &s->soc, TYPE_RP2040_SOC);
    qdev_realize(DEVICE(&s->soc), NULL, &error_fatal);
    
    /* Load firmware if provided */
    if (machine->firmware) {
        /* Load firmware to XIP flash region */
        if (load_image_targphys(machine->firmware, 
                               RP2040_XIP_BASE, 
                               RP2040_XIP_SIZE) < 0) {
            error_report("Could not load firmware '%s'", machine->firmware);
            exit(1);
        }
    } else if (machine->kernel_filename) {
        /* Load kernel (ELF or binary) */
        uint64_t entry, lowaddr, highaddr;
        int kernel_size;
        
        kernel_size = load_elf(machine->kernel_filename, NULL, NULL, NULL,
                              &entry, &lowaddr, &highaddr, NULL,
                              0, EM_ARM, 1, 0);
        
        if (kernel_size < 0) {
            /* Try loading as raw binary to XIP region */
            kernel_size = load_image_targphys(machine->kernel_filename,
                                             RP2040_XIP_BASE,
                                             RP2040_XIP_SIZE);
            if (kernel_size < 0) {
                error_report("Could not load kernel '%s'", 
                            machine->kernel_filename);
                exit(1);
            }
        }
    }
    
    /* TODO: Set up boot ROM if needed */
}

static void pico_machine_init(MachineClass *mc)
{
    mc->desc = "Raspberry Pi Pico (RP2040)";
    mc->init = pico_init;
    mc->max_cpus = 2;
    mc->default_cpus = 2;
    mc->default_ram_size = 264 * 1024;  /* 264KB SRAM */
    mc->default_ram_id = "rp2040.sram";
}

DEFINE_MACHINE("raspberrypi-pico", pico_machine_init)