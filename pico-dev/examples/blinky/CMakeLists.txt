add_executable(blinky
    blinky.c
)

# Pull in basic dependencies
target_link_libraries(blinky 
    pico_stdlib
    hardware_gpio
    hardware_timer
)

# Create map/bin/hex file etc.
pico_add_extra_outputs(blinky)

# Enable USB output, disable UART output
pico_enable_stdio_usb(blinky 0)
pico_enable_stdio_uart(blinky 1)

# Add URL via pico_set_program_url
pico_set_program_url(blinky "https://github.com/murr2k/qemu-demo")