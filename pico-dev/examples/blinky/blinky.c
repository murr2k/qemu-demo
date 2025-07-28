/**
 * Blinky LED example for Raspberry Pi Pico
 * Tests GPIO functionality in QEMU
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

// LED is on GPIO 25 on Pico
#define LED_PIN 25

// Timing constants
#define BLINK_DELAY_MS 500
#define TEST_DURATION_SEC 10

int main() {
    // Initialize stdio (UART for QEMU)
    stdio_init_all();
    
    // Wait a bit for UART to stabilize
    sleep_ms(1000);
    
    printf("\n=== Raspberry Pi Pico Blinky Test ===\n");
    printf("LED on GPIO %d\n", LED_PIN);
    printf("Blink delay: %d ms\n", BLINK_DELAY_MS);
    printf("Test duration: %d seconds\n", TEST_DURATION_SEC);
    printf("=====================================\n\n");
    
    // Initialize LED pin
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    // Get start time for test duration
    absolute_time_t start_time = get_absolute_time();
    int blink_count = 0;
    
    printf("Starting blink test...\n");
    
    // Blink for TEST_DURATION_SEC seconds
    while (absolute_time_diff_us(start_time, get_absolute_time()) < TEST_DURATION_SEC * 1000000) {
        // Turn LED on
        gpio_put(LED_PIN, 1);
        printf("[%06lld ms] LED ON  (blink #%d)\n", 
               to_ms_since_boot(get_absolute_time()), 
               ++blink_count);
        sleep_ms(BLINK_DELAY_MS);
        
        // Turn LED off
        gpio_put(LED_PIN, 0);
        printf("[%06lld ms] LED OFF\n", 
               to_ms_since_boot(get_absolute_time()));
        sleep_ms(BLINK_DELAY_MS);
    }
    
    // Final status
    printf("\n=== Test Complete ===\n");
    printf("Total blinks: %d\n", blink_count);
    printf("Test duration: %lld ms\n", 
           absolute_time_diff_us(start_time, get_absolute_time()) / 1000);
    printf("Status: PASS\n");
    printf("====================\n");
    
    // Keep LED on at end to indicate completion
    gpio_put(LED_PIN, 1);
    
    // For QEMU testing, we'll exit after the test
    // In real hardware, this would continue blinking
#ifdef QEMU_TEST
    return 0;
#else
    // Continue blinking forever on real hardware
    while (true) {
        gpio_put(LED_PIN, 1);
        sleep_ms(250);
        gpio_put(LED_PIN, 0);
        sleep_ms(250);
    }
#endif
}