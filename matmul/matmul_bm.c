#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stddef.h>
#include "encoding.h"
#include "matrix_data.h"

#define GPIO0 0x64002000
#define GPIO_VALUE      (*(volatile uint32_t *)(GPIO0))
#define GPIO_OUTPUT_EN  (*(volatile uint32_t *)(GPIO0 + 0x08))

#define UART0 0x64000000
#define UART_TXDATA (*(volatile uint32_t *)(UART0))
#define UART_TXCTRL (*(volatile uint32_t *)(UART0 + 0x08))

static inline void uart_send_byte(uint8_t b) {
    while (UART_TXDATA & 0x80000000);  // wait if TX FIFO full
    UART_TXDATA = b;
}

void uart_putc(char c) {
    if (c == '\n') {
        // send carriage return first
        while (UART_TXDATA & 0x80000000);  
        UART_TXDATA = '\r';
    }
    // send actual character
    while (UART_TXDATA & 0x80000000);  
    UART_TXDATA = c;
}

void uart_puts(const char *s) {
    while (*s) uart_putc(*s++);
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

    // Print digits in reverse
    while (i--) uart_putc(buf[i]);
}

void uart_print_float(float f, int precision) {
    int int_part = (int)f;
    float frac_part = f - int_part;
    if (f < 0) {
        uart_putc('-');
        int_part = -int_part;
        frac_part = -frac_part;
    }

    uart_print_dec(int_part);
    uart_putc('.');

    for(int i=0; i<precision; i++) {
        frac_part *= 10;
        int digit = (int)frac_part;
        uart_putc('0' + digit);
        frac_part -= digit;
    }
}

int main() {
    uint64_t start, end;
    
    // Point to the base of the GPIO controller
    volatile unsigned int* gpio = (unsigned int*)GPIO0;
    
    // Set Output Enable (Offset 0x08) for pin 0
    gpio[2] = 0x1;
    
    // Set Pin Value (Offset 0x0C) to LOW
    gpio[3] = 0x0;
    
    uart_puts("[INFO] Starting matrix multiplication...\n");
    
    // Signal start of computation via GPIO HIGH
    gpio[3] = 0x1;
    
    // Record start hardware clock cycle count
    start = read_csr(cycle);

    //----------------------------------------------------------------------
    // Matrix Multiplication Computation: C = A * B
    //----------------------------------------------------------------------
    for (int i = 0; i < MATRIX_M; i++) {
        for (int j = 0; j < MATRIX_N; j++) {
            float sum = 0.0f;
            for (int k = 0; k < MATRIX_K; k++) {
                sum += matrix_A[i][k] * matrix_B[k][j];
            }
            matrix_C[i][j] = sum;
        }
    }

    // Record end hardware clock cycle count
    end = read_csr(cycle);
    
    // Signal end of computation via GPIO LOW
    gpio[3] = 0x0;
    
    uart_puts("[INFO] Matrix multiplication finished.\n");
    
    // Print total cycle elapsed count over UART
    uart_puts("Cycles elapsed: ");
    uart_print_dec(end - start);
    uart_puts("\n\n");

    //----------------------------------------------------------------------
    // Print Output Matrix C over UART
    //----------------------------------------------------------------------
    uart_puts("--- Matrix C Result ---\n");
    for (int i = 0; i < MATRIX_M; i++) {
        uart_puts("[ ");
        for (int j = 0; j < MATRIX_N; j++) {
            uart_print_float(matrix_C[i][j], 2); // 2 decimal places
            uart_puts("\t");
        }
        uart_puts("]\n");
    }
    
    return 0;
}