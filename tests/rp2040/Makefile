# Makefile for RP2040 QEMU test programs

# Toolchain
CROSS_COMPILE ?= arm-none-eabi-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

# Compiler flags
CFLAGS = -mcpu=cortex-m0plus -mthumb -Wall -O2 -g
CFLAGS += -nostdlib -nostartfiles -ffreestanding
CFLAGS += -ffunction-sections -fdata-sections

# Linker flags
LDFLAGS = -nostdlib -T link.ld -Wl,--gc-sections

# Source files
SOURCES = test_uart.c test_gpio.c test_timer.c

# Build targets
TARGETS = $(SOURCES:.c=.elf) $(SOURCES:.c=.bin)

all: $(TARGETS)

# Build rules
%.elf: %.c startup.s link.ld
	$(CC) $(CFLAGS) -o $@ startup.s $< $(LDFLAGS)
	$(OBJDUMP) -d $@ > $(@:.elf=.lst)

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

# Startup code
startup.s: 
	@echo "Creating minimal startup code..."
	@echo '.syntax unified' > $@
	@echo '.cpu cortex-m0plus' >> $@
	@echo '.thumb' >> $@
	@echo '' >> $@
	@echo '.section .vectors, "ax"' >> $@
	@echo '.global _vectors' >> $@
	@echo '_vectors:' >> $@
	@echo '    .word 0x20042000  /* Initial SP */' >> $@
	@echo '    .word _start + 1  /* Reset handler */' >> $@
	@echo '' >> $@
	@echo '.section .text' >> $@
	@echo '.global _start' >> $@
	@echo '.type _start, %function' >> $@
	@echo '_start:' >> $@
	@echo '    /* Clear BSS */' >> $@
	@echo '    ldr r0, =__bss_start' >> $@
	@echo '    ldr r1, =__bss_end' >> $@
	@echo '    movs r2, #0' >> $@
	@echo '.L_clear_bss:' >> $@
	@echo '    cmp r0, r1' >> $@
	@echo '    bge .L_clear_bss_done' >> $@
	@echo '    str r2, [r0]' >> $@
	@echo '    adds r0, r0, #4' >> $@
	@echo '    b .L_clear_bss' >> $@
	@echo '.L_clear_bss_done:' >> $@
	@echo '    /* Call main */' >> $@
	@echo '    bl main' >> $@
	@echo '    /* Hang if main returns */' >> $@
	@echo '.L_hang:' >> $@
	@echo '    wfi' >> $@
	@echo '    b .L_hang' >> $@
	@echo '.size _start, . - _start' >> $@

# Run tests in QEMU
run-%: %.elf
	qemu-system-arm -machine raspberrypi-pico -kernel $< \
		-serial stdio -monitor none -nographic

debug-%: %.elf
	qemu-system-arm -machine raspberrypi-pico -kernel $< \
		-serial stdio -s -S &
	$(CROSS_COMPILE)gdb $< -ex "target remote :1234"

clean:
	rm -f *.elf *.bin *.lst *.o startup.s

.PHONY: all clean run-% debug-%