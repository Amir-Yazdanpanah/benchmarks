#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include "encoding.h"

#define GPIO0           0x64002000
#define GPIO_VALUE      (*(volatile uint32_t *)(GPIO0))
#define GPIO_OUTPUT_EN  (*(volatile uint32_t *)(GPIO0 + 0x08))

#define UART0           0x64000000
#define UART_TXDATA     (*(volatile uint32_t *)(UART0))
#define UART_TXCTRL     (*(volatile uint32_t *)(UART0 + 0x08))

/* UART Utility Functions */
void uart_putc(char c) {
    if (c == '\n') {
        while (UART_TXDATA & 0x80000000);  
        UART_TXDATA = '\r';
    }
    while (UART_TXDATA & 0x80000000);  
    UART_TXDATA = c;
}

void uart_puts(const char *s) {
    while (*s) uart_putc(*s++);
}

void uart_print_dec(uint64_t v) {
    char buf[20];
    int i = 0;

    if (v == 0) {
        uart_putc('0');
        return;
    }

    while (v) {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    }

    while (i--) uart_putc(buf[i]);
}

void uart_print_hex(uint64_t value) {
    char hex[17];
    hex[16] = '\0';
    for (int i = 15; i >= 0; i--) {
        uint8_t digit = value & 0xF;
        if (digit < 10)
            hex[i] = '0' + digit;
        else
            hex[i] = 'A' + (digit - 10);
        value >>= 4;
    }
    uart_puts(hex);
}

/* Embench EDN Benchmark Data Definitions */
#define EDN_DATA_SIZE 1024
#define EDN_COEFF_SIZE 16

static int32_t test_data[EDN_DATA_SIZE];
static int32_t test_coeff[EDN_COEFF_SIZE];
static int32_t test_output[EDN_DATA_SIZE - EDN_COEFF_SIZE];

void initialise_benchmark(void) {
    for (int i = 0; i < EDN_DATA_SIZE; i++) {
        test_data[i] = (int32_t)((i * 17) % 257) - 128;
    }
    for (int i = 0; i < EDN_COEFF_SIZE; i++) {
        test_coeff[i] = (int32_t)((i * 3) % 31) - 15;
    }
}

void benchmark_body(int iterations) {
    for (int it = 0; it < iterations; it++) {
        for (int i = 0; i < (EDN_DATA_SIZE - EDN_COEFF_SIZE); i++) {
            int32_t sum = 0;
            for (int j = 0; j < EDN_COEFF_SIZE; j++) {
                sum += test_data[i + j] * test_coeff[j];
            }
            test_output[i] = sum;
        }
    }
}

int verify_benchmark(void) {
    int32_t checksum = 0;
    for (int i = 0; i < (EDN_DATA_SIZE - EDN_COEFF_SIZE); i++) {
        checksum ^= test_output[i];
    }
    return (checksum != 0);
}

int main() {
    uint64_t start_cycles, end_cycles, total_cycles;
    
    // Set up GPIO for measurement profiling
    volatile unsigned int* gpio = (unsigned int*)GPIO0;
    gpio[2] = 0x1;  // Set Pin 0 Output Enable
    gpio[3] = 0x0;  // Set Pin 0 LOW

    uart_puts("[INFO] Initializing Embench EDN benchmark...\n");
    initialise_benchmark();

    uart_puts("[INFO] Running EDN kernel...\n");
    
    // Trigger GPIO Pin 0 HIGH to signal execution start to current probe
    gpio[3] = 0x1;
    start_cycles = read_csr(cycle);

    // Carry out the benchmark computations across iterations
    benchmark_body(10);

    end_cycles = read_csr(cycle);
    // Bring GPIO Pin 0 LOW at kernel completion
    gpio[3] = 0x0;

    total_cycles = end_cycles - start_cycles;

    uart_puts("[INFO] Benchmark complete.\n");
    uart_puts("Cycles elapsed: ");
    uart_print_dec(total_cycles);
    uart_puts("\n");

    if (verify_benchmark()) {
        uart_puts("[RESULT] Verification SUCCESS\n");
    } else {
        uart_puts("[RESULT] Verification FAILED\n");
    }

    return 0;
}