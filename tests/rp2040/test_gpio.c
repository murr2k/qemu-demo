/*
 * RP2040 GPIO Test Program
 * Tests basic GPIO functionality in QEMU
 */

#include <stdint.h>

/* GPIO Registers */
#define GPIO_BASE      0x40014000
#define GPIO_STATUS(n) (GPIO_BASE + 0x000 + (n) * 8)
#define GPIO_CTRL(n)   (GPIO_BASE + 0x004 + (n) * 8)

/* SIO (Single-cycle I/O) Registers */
#define SIO_BASE       0xD0000000
#define GPIO_IN        (SIO_BASE + 0x04)
#define GPIO_OUT       (SIO_BASE + 0x10)
#define GPIO_OUT_SET   (SIO_BASE + 0x14)
#define GPIO_OUT_CLR   (SIO_BASE + 0x18)
#define GPIO_OUT_XOR   (SIO_BASE + 0x1C)
#define GPIO_OE        (SIO_BASE + 0x20)
#define GPIO_OE_SET    (SIO_BASE + 0x24)
#define GPIO_OE_CLR    (SIO_BASE + 0x28)

/* Timer for delays */
#define TIMER_BASE     0x40054000
#define TIMELR         (TIMER_BASE + 0x0C)

/* Function select values */
#define GPIO_FUNC_SIO  5

/* Test pins */
#define LED_PIN        25
#define TEST_OUTPUT    26
#define TEST_INPUT     27

void delay_us(uint32_t us) {
    uint32_t start = *(volatile uint32_t*)TIMELR;
    while ((*(volatile uint32_t*)TIMELR - start) < us);
}

void gpio_init_out(uint32_t pin) {
    /* Set function to SIO */
    *(volatile uint32_t*)GPIO_CTRL(pin) = GPIO_FUNC_SIO;
    /* Enable output */
    *(volatile uint32_t*)GPIO_OE_SET = (1 << pin);
}

void gpio_init_in(uint32_t pin) {
    /* Set function to SIO */
    *(volatile uint32_t*)GPIO_CTRL(pin) = GPIO_FUNC_SIO;
    /* Disable output (input mode) */
    *(volatile uint32_t*)GPIO_OE_CLR = (1 << pin);
}

void gpio_set(uint32_t pin) {
    *(volatile uint32_t*)GPIO_OUT_SET = (1 << pin);
}

void gpio_clear(uint32_t pin) {
    *(volatile uint32_t*)GPIO_OUT_CLR = (1 << pin);
}

void gpio_toggle(uint32_t pin) {
    *(volatile uint32_t*)GPIO_OUT_XOR = (1 << pin);
}

uint32_t gpio_get(uint32_t pin) {
    return (*(volatile uint32_t*)GPIO_IN >> pin) & 1;
}

uint32_t gpio_get_out(uint32_t pin) {
    return (*(volatile uint32_t*)GPIO_OUT >> pin) & 1;
}

/* Simple UART functions for output */
#define UART0_BASE     0x40034000
#define UART0_DR       (UART0_BASE + 0x000)
#define UART0_FR       (UART0_BASE + 0x018)
#define UART_FR_TXFE   (1 << 7)

void uart_putc(char c) {
    while (!(*(volatile uint32_t*)UART0_FR & UART_FR_TXFE));
    *(volatile uint32_t*)UART0_DR = c;
}

void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') uart_putc('\r');
        uart_putc(*s++);
    }
}

void uart_puthex(uint32_t val) {
    uart_puts("0x");
    for (int i = 28; i >= 0; i -= 4) {
        uint32_t nibble = (val >> i) & 0xF;
        uart_putc(nibble < 10 ? '0' + nibble : 'A' + nibble - 10);
    }
}

int main(void) {
    /* Initialize UART for debug output */
    *(volatile uint32_t*)(UART0_BASE + 0x030) = 0x301; /* Enable UART */
    
    uart_puts("\nRP2040 GPIO Test Program\n");
    uart_puts("========================\n\n");
    
    /* Test 1: Initialize GPIOs */
    uart_puts("Test 1: Initializing GPIOs...\n");
    gpio_init_out(LED_PIN);
    gpio_init_out(TEST_OUTPUT);
    gpio_init_in(TEST_INPUT);
    uart_puts("  - GPIO25 (LED) as output\n");
    uart_puts("  - GPIO26 as output\n");
    uart_puts("  - GPIO27 as input\n\n");
    
    /* Test 2: Set and clear outputs */
    uart_puts("Test 2: Testing output control...\n");
    
    gpio_set(LED_PIN);
    uart_puts("  - Set GPIO25 high: ");
    uart_puthex(gpio_get_out(LED_PIN));
    uart_puts("\n");
    
    gpio_clear(LED_PIN);
    uart_puts("  - Set GPIO25 low: ");
    uart_puthex(gpio_get_out(LED_PIN));
    uart_puts("\n");
    
    /* Test 3: Toggle function */
    uart_puts("\nTest 3: Testing toggle function...\n");
    for (int i = 0; i < 4; i++) {
        gpio_toggle(TEST_OUTPUT);
        uart_puts("  - GPIO26 state: ");
        uart_puthex(gpio_get_out(TEST_OUTPUT));
        uart_puts("\n");
        delay_us(1000);
    }
    
    /* Test 4: Read all GPIO states */
    uart_puts("\nTest 4: Reading GPIO states...\n");
    uint32_t gpio_states = *(volatile uint32_t*)GPIO_OUT;
    uart_puts("  - Output register: ");
    uart_puthex(gpio_states);
    uart_puts("\n");
    
    uint32_t gpio_inputs = *(volatile uint32_t*)GPIO_IN;
    uart_puts("  - Input register: ");
    uart_puthex(gpio_inputs);
    uart_puts("\n");
    
    /* Test 5: Blink LED */
    uart_puts("\nTest 5: Blinking LED on GPIO25...\n");
    for (int i = 0; i < 10; i++) {
        gpio_toggle(LED_PIN);
        uart_puts(gpio_get_out(LED_PIN) ? "  - LED ON\n" : "  - LED OFF\n");
        delay_us(200000); /* 200ms */
    }
    
    uart_puts("\nGPIO test complete!\n");
    
    /* Keep LED blinking */
    while (1) {
        gpio_toggle(LED_PIN);
        delay_us(500000); /* 500ms */
    }
    
    return 0;
}