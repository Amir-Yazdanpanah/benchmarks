#include <stdint.h>
#include <stddef.h>
#include "encoding.h"
#include "support.h"

#define GPIO0 0x64002000
#define GPIO_VALUE      (*(volatile uint32_t *)(GPIO0))
#define GPIO_OUTPUT_EN  (*(volatile uint32_t *)(GPIO0 + 0x08))

#define UART0 0x64000000
#define UART_TXDATA (*(volatile uint32_t *)(UART0))
#define UART_TXCTRL (*(volatile uint32_t *)(UART0 + 0x08))

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

int main() {
    uint64_t start, end;
    
    volatile unsigned int* gpio = (unsigned int*)GPIO0;
    gpio[2] = 0x1;
    gpio[3] = 0x0;
    
    uart_puts("[INFO] Initializing Embench qrduino benchmark...\n");
    initialise_benchmark();
    
    uart_puts("[INFO] Executing benchmark...\n");
    gpio[3] = 0x1;
    
    start = read_csr(cycle);
    benchmark();
    end = read_csr(cycle);
    
    gpio[3] = 0x0;
    uart_puts("[INFO] Benchmark complete.\n");
    
    int is_correct = verify_benchmark(0);
    if (is_correct) {
        uart_puts("[RESULT] Verification PASSED!\n");
    } else {
        uart_puts("[RESULT] Verification FAILED!\n");
    }
    
    uart_puts("Cycles elapsed: ");
    uart_print_dec(end - start);
    uart_puts("\n");
    
    while (1) {
        __asm__ volatile("nop");
    }
    
    return 0;
}