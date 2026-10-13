#include <stdint.h>
#include "images_batch.h"

#define NPIX (WIDTH * HEIGHT)
#define REG32(addr) (*(volatile uint32_t *)(uintptr_t)(addr))
#define UART_TXDATA REG32(0x64000000)
#define UART_TXCTRL REG32(0x64000008)
#define GPIO_OUTPUT_EN REG32(0x64002008)
#define GPIO_OUTPUT_VAL REG32(0x6400200c)

#ifndef GAP_CYCLES
#define GAP_CYCLES 5000000ULL
#endif

/* Volatile outputs keep the original Sobel + RGB-overlay work observable. */
static volatile uint8_t edges[NPIX];
static volatile uint8_t overlay[NPIX * 3];

static inline void io_fence(void) {
    __asm__ volatile ("fence iorw, iorw" ::: "memory");
}

static inline uint64_t cycle_now(void) {
    uint64_t value;
    __asm__ volatile ("rdcycle %0" : "=r"(value) :: "memory");
    return value;
}

static void uart_putc(char c) {
    if (c == '\n') uart_putc('\r');
    while (UART_TXDATA & 0x80000000U) {}
    UART_TXDATA = (uint8_t)c;
}

static void uart_puts(const char *s) {
    while (*s) uart_putc(*s++);
}

static void uart_dec(uint64_t value) {
    char digits[20];
    unsigned count = 0;
    do {
        digits[count++] = '0' + value % 10;
        value /= 10;
    } while (value);
    while (count) uart_putc(digits[--count]);
}

static void gap(void) {
    uint64_t begin = cycle_now();
    while (cycle_now() - begin < GAP_CYCLES) {}
}

static int iabs(int value) { return value < 0 ? -value : value; }

static void process_image(const uint8_t *pixels) {
    const int Gx[3][3] = {{-1,0,1}, {-2,0,2}, {-1,0,1}};
    const int Gy[3][3] = {{-1,-2,-1}, {0,0,0}, {1,2,1}};
    for (int y = 1; y < HEIGHT - 1; ++y) {
        for (int x = 1; x < WIDTH - 1; ++x) {
            int sumX = 0, sumY = 0;
            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    int pixel = pixels[(y + ky) * WIDTH + x + kx];
                    sumX += pixel * Gx[ky + 1][kx + 1];
                    sumY += pixel * Gy[ky + 1][kx + 1];
                }
            }
            int magnitude = iabs(sumX) + iabs(sumY);
            edges[y * WIDTH + x] = magnitude > 255 ? 255 : (uint8_t)magnitude;
        }
    }
    for (int i = 0; i < NPIX; ++i) {
        uint8_t value = pixels[i];
        overlay[3*i] = value;
        overlay[3*i+1] = value;
        overlay[3*i+2] = value;
        if (edges[i] > 100) {
            overlay[3*i] = 255;
            overlay[3*i+1] = 0;
            overlay[3*i+2] = 0;
        }
    }
}

int main(void) {
    UART_TXCTRL = 1;
    GPIO_OUTPUT_VAL &= ~1U;
    GPIO_OUTPUT_EN |= 1U;
    io_fence();
    uart_puts("BATCH_BEGIN images=");
    uart_dec(IMAGE_COUNT);
    uart_putc('\n');
    for (unsigned image = 0; image < IMAGE_COUNT; ++image) {
        uart_puts("IMAGE_START ");
        uart_dec(image + 1);
        uart_putc(' ');
        uart_puts(images[image].name);
        uart_putc('\n');
        /* Low interval gives serial output time to finish before measurement.
           It does not clear processor caches. */
        gap();
        GPIO_OUTPUT_VAL |= 1U;
        io_fence();
        uint64_t start = cycle_now();
        process_image(images[image].pixels);
        io_fence();
        uint64_t end = cycle_now();
        GPIO_OUTPUT_VAL &= ~1U;
        io_fence();
        uart_puts("IMAGE_DONE ");
        uart_dec(image + 1);
        uart_puts(" cycles=");
        uart_dec(end - start);
        uart_putc('\n');
    }
    uart_puts("BATCH_DONE\n");
    return 0;
}
