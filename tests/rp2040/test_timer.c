/*
 * RP2040 Timer Test Program
 * Tests timer and alarm functionality in QEMU
 */

#include <stdint.h>

/* Timer Registers */
#define TIMER_BASE     0x40054000
#define TIMEHW         (TIMER_BASE + 0x00)
#define TIMELW         (TIMER_BASE + 0x04)
#define TIMEHR         (TIMER_BASE + 0x08)
#define TIMELR         (TIMER_BASE + 0x0C)
#define ALARM0         (TIMER_BASE + 0x10)
#define ALARM1         (TIMER_BASE + 0x14)
#define ALARM2         (TIMER_BASE + 0x18)
#define ALARM3         (TIMER_BASE + 0x1C)
#define ARMED          (TIMER_BASE + 0x20)
#define TIMERAWH       (TIMER_BASE + 0x24)
#define TIMERAWL       (TIMER_BASE + 0x28)
#define DBGPAUSE       (TIMER_BASE + 0x2C)
#define PAUSE          (TIMER_BASE + 0x30)
#define INTR           (TIMER_BASE + 0x34)
#define INTE           (TIMER_BASE + 0x38)
#define INTF           (TIMER_BASE + 0x3C)
#define INTS           (TIMER_BASE + 0x40)

/* NVIC for interrupt handling */
#define NVIC_ISER      0xE000E100
#define NVIC_ICER      0xE000E180
#define NVIC_ISPR      0xE000E200
#define NVIC_ICPR      0xE000E280

/* Timer IRQ numbers */
#define TIMER_IRQ_0    0
#define TIMER_IRQ_1    1
#define TIMER_IRQ_2    2
#define TIMER_IRQ_3    3

/* Simple UART functions */
#define UART0_BASE     0x40034000
#define UART0_DR       (UART0_BASE + 0x000)
#define UART0_FR       (UART0_BASE + 0x018)
#define UART_FR_TXFE   (1 << 7)

volatile uint32_t alarm_fired[4] = {0, 0, 0, 0};

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

void uart_putdec(uint32_t val) {
    char buf[12];
    int i = 11;
    buf[i] = '\0';
    
    if (val == 0) {
        uart_putc('0');
        return;
    }
    
    while (val > 0 && i > 0) {
        buf[--i] = '0' + (val % 10);
        val /= 10;
    }
    
    uart_puts(&buf[i]);
}

uint32_t timer_get_us(void) {
    return *(volatile uint32_t*)TIMELR;
}

uint64_t timer_get_us_64(void) {
    uint32_t hi = *(volatile uint32_t*)TIMEHW;
    uint32_t lo = *(volatile uint32_t*)TIMELW;
    return ((uint64_t)hi << 32) | lo;
}

void timer_set_alarm(uint32_t alarm_num, uint32_t target_us) {
    volatile uint32_t *alarm_reg = (volatile uint32_t*)(ALARM0 + alarm_num * 4);
    *alarm_reg = target_us;
}

void timer_enable_irq(uint32_t alarm_num) {
    /* Enable interrupt in timer */
    *(volatile uint32_t*)INTE |= (1 << alarm_num);
    /* Enable interrupt in NVIC */
    *(volatile uint32_t*)NVIC_ISER = (1 << (TIMER_IRQ_0 + alarm_num));
}

void timer_clear_irq(uint32_t alarm_num) {
    /* Clear interrupt flag */
    *(volatile uint32_t*)INTR = (1 << alarm_num);
}

/* Timer interrupt handlers */
void timer_irq_handler(uint32_t alarm_num) {
    alarm_fired[alarm_num] = 1;
    timer_clear_irq(alarm_num);
    
    uart_puts("  - Alarm ");
    uart_putdec(alarm_num);
    uart_puts(" fired at ");
    uart_putdec(timer_get_us());
    uart_puts(" us\n");
}

void timer0_irq_handler(void) { timer_irq_handler(0); }
void timer1_irq_handler(void) { timer_irq_handler(1); }
void timer2_irq_handler(void) { timer_irq_handler(2); }
void timer3_irq_handler(void) { timer_irq_handler(3); }

/* Vector table */
extern void _start(void);
__attribute__((section(".vectors")))
void (* const vectors[])(void) = {
    (void*)0x20042000,     /* Initial stack pointer */
    _start,                /* Reset handler */
    0,                     /* NMI */
    0,                     /* HardFault */
    0,0,0,0,0,0,0,        /* Reserved */
    0,                     /* SVC */
    0,0,                   /* Reserved */
    0,                     /* PendSV */
    0,                     /* SysTick */
    /* External interrupts */
    timer0_irq_handler,    /* IRQ 0 - Timer 0 */
    timer1_irq_handler,    /* IRQ 1 - Timer 1 */
    timer2_irq_handler,    /* IRQ 2 - Timer 2 */
    timer3_irq_handler,    /* IRQ 3 - Timer 3 */
};

void delay_us(uint32_t us) {
    uint32_t start = timer_get_us();
    while ((timer_get_us() - start) < us);
}

int main(void) {
    /* Initialize UART */
    *(volatile uint32_t*)(UART0_BASE + 0x030) = 0x301;
    
    uart_puts("\nRP2040 Timer Test Program\n");
    uart_puts("=========================\n\n");
    
    /* Test 1: Basic timer reading */
    uart_puts("Test 1: Reading timer values...\n");
    uint32_t time1 = timer_get_us();
    uart_puts("  - Current time: ");
    uart_putdec(time1);
    uart_puts(" us\n");
    
    delay_us(1000000); /* 1 second */
    
    uint32_t time2 = timer_get_us();
    uart_puts("  - After 1 second: ");
    uart_putdec(time2);
    uart_puts(" us\n");
    uart_puts("  - Elapsed: ");
    uart_putdec(time2 - time1);
    uart_puts(" us\n\n");
    
    /* Test 2: 64-bit timer reading */
    uart_puts("Test 2: 64-bit timer reading...\n");
    uint64_t time64 = timer_get_us_64();
    uart_puts("  - 64-bit time: ");
    uart_putdec(time64 >> 32);
    uart_puts(":");
    uart_putdec(time64 & 0xFFFFFFFF);
    uart_puts("\n\n");
    
    /* Test 3: Timer delays */
    uart_puts("Test 3: Testing delay function...\n");
    for (int i = 1; i <= 5; i++) {
        uart_puts("  - Delay ");
        uart_putdec(i * 100);
        uart_puts(" ms...");
        
        uint32_t start = timer_get_us();
        delay_us(i * 100000);
        uint32_t actual = timer_get_us() - start;
        
        uart_puts(" actual: ");
        uart_putdec(actual);
        uart_puts(" us\n");
    }
    
    /* Test 4: Alarm functionality */
    uart_puts("\nTest 4: Testing alarms...\n");
    
    /* Enable interrupts globally */
    __asm__ volatile ("cpsie i");
    
    /* Set up alarms */
    uint32_t now = timer_get_us();
    uart_puts("  - Setting up 4 alarms:\n");
    
    for (int i = 0; i < 4; i++) {
        uint32_t target = now + (i + 1) * 500000; /* 0.5, 1, 1.5, 2 seconds */
        timer_set_alarm(i, target);
        timer_enable_irq(i);
        
        uart_puts("    Alarm ");
        uart_putdec(i);
        uart_puts(" set for ");
        uart_putdec(target);
        uart_puts(" us\n");
    }
    
    /* Wait for alarms */
    uart_puts("  - Waiting for alarms to fire...\n");
    
    uint32_t timeout = now + 3000000; /* 3 second timeout */
    while (timer_get_us() < timeout) {
        int all_fired = 1;
        for (int i = 0; i < 4; i++) {
            if (!alarm_fired[i]) {
                all_fired = 0;
                break;
            }
        }
        if (all_fired) break;
        
        __asm__ volatile ("wfi"); /* Wait for interrupt */
    }
    
    /* Check results */
    uart_puts("\n  - Alarm results:\n");
    for (int i = 0; i < 4; i++) {
        uart_puts("    Alarm ");
        uart_putdec(i);
        uart_puts(": ");
        uart_puts(alarm_fired[i] ? "FIRED" : "NOT FIRED");
        uart_puts("\n");
    }
    
    /* Test 5: Timer pause functionality */
    uart_puts("\nTest 5: Testing timer pause...\n");
    uint32_t before_pause = timer_get_us();
    uart_puts("  - Time before pause: ");
    uart_putdec(before_pause);
    uart_puts(" us\n");
    
    /* Pause timer */
    *(volatile uint32_t*)PAUSE = 1;
    uart_puts("  - Timer paused\n");
    
    /* Busy wait (timer is paused) */
    for (volatile int i = 0; i < 1000000; i++);
    
    uint32_t during_pause = timer_get_us();
    uart_puts("  - Time during pause: ");
    uart_putdec(during_pause);
    uart_puts(" us (should be same)\n");
    
    /* Unpause timer */
    *(volatile uint32_t*)PAUSE = 0;
    uart_puts("  - Timer resumed\n");
    
    delay_us(100000);
    uint32_t after_pause = timer_get_us();
    uart_puts("  - Time after resume: ");
    uart_putdec(after_pause);
    uart_puts(" us\n");
    
    uart_puts("\nTimer test complete!\n");
    
    while (1) {
        __asm__ volatile ("wfi");
    }
    
    return 0;
}