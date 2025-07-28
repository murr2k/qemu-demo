/*
 * RP2040 UART Test Program
 * Tests basic UART functionality in QEMU
 */

#include <stdint.h>

/* UART0 Registers */
#define UART0_BASE     0x40034000
#define UART0_DR       (UART0_BASE + 0x000)
#define UART0_FR       (UART0_BASE + 0x018)
#define UART0_IBRD     (UART0_BASE + 0x024)
#define UART0_FBRD     (UART0_BASE + 0x028)
#define UART0_LCR_H    (UART0_BASE + 0x02C)
#define UART0_CR       (UART0_BASE + 0x030)

/* Flag Register bits */
#define UART_FR_TXFE   (1 << 7)  /* TX FIFO empty */
#define UART_FR_RXFE   (1 << 4)  /* RX FIFO empty */

/* Control Register bits */
#define UART_CR_UARTEN (1 << 0)  /* UART enable */
#define UART_CR_TXE    (1 << 8)  /* TX enable */
#define UART_CR_RXE    (1 << 9)  /* RX enable */

/* Line Control bits */
#define UART_LCR_H_FEN (1 << 4)  /* FIFO enable */
#define UART_LCR_H_WLEN_8 (3 << 5)  /* 8 bits */

void uart_init(void) {
    /* Disable UART */
    *(volatile uint32_t*)UART0_CR = 0;
    
    /* Set baud rate divisors for 115200 baud
     * Assuming 48MHz UART clock:
     * BRD = 48000000 / (16 * 115200) = 26.042
     * IBRD = 26, FBRD = 0.042 * 64 = 3
     */
    *(volatile uint32_t*)UART0_IBRD = 26;
    *(volatile uint32_t*)UART0_FBRD = 3;
    
    /* Set 8N1, enable FIFOs */
    *(volatile uint32_t*)UART0_LCR_H = UART_LCR_H_FEN | UART_LCR_H_WLEN_8;
    
    /* Enable UART, TX and RX */
    *(volatile uint32_t*)UART0_CR = UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE;
}

void uart_putc(char c) {
    /* Wait for TX FIFO to have space */
    while (!(*(volatile uint32_t*)UART0_FR & UART_FR_TXFE));
    
    /* Write character */
    *(volatile uint32_t*)UART0_DR = c;
}

void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') {
            uart_putc('\r');
        }
        uart_putc(*s++);
    }
}

char uart_getc(void) {
    /* Wait for RX FIFO to have data */
    while (*(volatile uint32_t*)UART0_FR & UART_FR_RXFE);
    
    /* Read character */
    return *(volatile uint32_t*)UART0_DR & 0xFF;
}

int main(void) {
    uart_init();
    
    uart_puts("RP2040 UART Test Program\n");
    uart_puts("========================\n\n");
    
    uart_puts("Testing UART output...\n");
    uart_puts("This message should appear on the console.\n\n");
    
    uart_puts("Testing character output: ");
    for (char c = 'A'; c <= 'Z'; c++) {
        uart_putc(c);
    }
    uart_puts("\n\n");
    
    uart_puts("Testing numbers: ");
    for (int i = 0; i < 10; i++) {
        uart_putc('0' + i);
        uart_putc(' ');
    }
    uart_puts("\n\n");
    
    uart_puts("UART test complete!\n");
    
    /* Infinite loop */
    while (1) {
        __asm__("wfi");
    }
    
    return 0;
}