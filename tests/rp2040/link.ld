/*
 * Linker script for RP2040 test programs
 * Memory layout based on RP2040 specifications
 */

MEMORY
{
    /* Boot ROM - 16KB at 0x00000000 */
    ROM (rx)     : ORIGIN = 0x00000000, LENGTH = 16K
    
    /* XIP Flash - 16MB at 0x10000000 */
    FLASH (rx)   : ORIGIN = 0x10000000, LENGTH = 16M
    
    /* SRAM - 264KB at 0x20000000 */
    RAM (rwx)    : ORIGIN = 0x20000000, LENGTH = 264K
    
    /* Stack starts at end of RAM */
    STACK (rw)   : ORIGIN = 0x20041000, LENGTH = 4K
}

/* Entry point */
ENTRY(_start)

SECTIONS
{
    /* Vector table goes in Flash */
    .vectors : {
        KEEP(*(.vectors))
    } > FLASH
    
    /* Code and read-only data */
    .text : {
        *(.text*)
        *(.rodata*)
        
        /* ARM-Thumb interworking */
        *(.glue_7)
        *(.glue_7t)
        
        /* C++ exception handling */
        *(.eh_frame)
        KEEP(*(.init))
        KEEP(*(.fini))
        
        . = ALIGN(4);
    } > FLASH
    
    /* Initialized data - load from Flash, run from RAM */
    .data : {
        __data_start = .;
        *(.data*)
        . = ALIGN(4);
        __data_end = .;
    } > RAM AT > FLASH
    
    /* Uninitialized data */
    .bss : {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > RAM
    
    /* Stack */
    .stack : {
        . = ALIGN(8);
        __stack_start = .;
        . = . + LENGTH(STACK);
        __stack_end = .;
        __StackTop = .;
    } > STACK
    
    /* Remove debug information */
    /DISCARD/ : {
        *(.ARM.exidx*)
        *(.gnu.linkonce.armexidx.*)
    }
}

/* Provide stack location for startup code */
PROVIDE(__stack = ORIGIN(STACK) + LENGTH(STACK));