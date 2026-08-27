

#include <stdint.h>
#include <string.h>
#include <math.h>
#include "boat.h"
#include "encoding.h"
#include <stddef.h>

#define GPIO0 0x64002000                                        // Added
#define GPIO_VALUE      (*(volatile uint32_t *)(GPIO0))         // Added
#define GPIO_OUTPUT_EN  (*(volatile uint32_t *)(GPIO0 + 0x08))  // Added

#define NPIX   (WIDTH * HEIGHT)

#define WIDTH  720
#define HEIGHT 576

static uint8_t gray[NPIX];
static uint8_t edges[NPIX];
static uint8_t overlay[NPIX * 3];   // RGB Overlay


#define UART0 0x64000000
#define UART_TXDATA (*(volatile uint32_t *)(UART0))
#define UART_TXCTRL (*(volatile uint32_t *)(UART0 + 0x08))

static inline void uart_send_byte(uint8_t b) {
    while (UART_TXDATA & 0x80000000);  // wait if TX FIFO full
    UART_TXDATA = b;
}

static void uart_send_marker(void) {
    const char marker[] = "START_EDGES\n";
    for (int i = 0; i < sizeof(marker)-1; i++)
        uart_send_byte(marker[i]);
}

static void uart_send_image(uint8_t *img, int size) {
    for (int i = 0; i < size; i++)
        uart_send_byte(img[i]);
}

static inline int iabs(int x) {
    return (x < 0) ? -x : x;
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
    // Index [2] because 0x08 / 4-byte-steps = 2
    gpio[2] = 0x1;
    
    // Set Pin Value (Offset 0x0C) to HIGH
    // Index [3] because 0x0C / 4-byte-steps = 3
    gpio[3] = 0x0;
    
    uart_puts("[INFO] Starting edge detection...\n");
    gpio[3] = 0x1;
    
    //----------------------------------------------------------------------
    // 1. Bild in eindimensionales Array kopieren
    //----------------------------------------------------------------------
    // for (int y = 0; y < HEIGHT; y++) {
    //     for (int x = 0; x < WIDTH; x++) {
    //         //gray[y * WIDTH + x] = image_data[y][x];
    //         //gray[y * WIDTH + x] = boats[y][x];
    //         gray[y * WIDTH + x] = boats[y * WIDTH + x];
    //     }
    // }

    //----------------------------------------------------------------------
    // 2. Sobel-Kernel definieren
    //----------------------------------------------------------------------


    start = read_csr(cycle);
    const int Gx[3][3] = {
        {-1, 0, 1},
        {-2, 0, 2},
        {-1, 0, 1}
    };
    const int Gy[3][3] = {
        {-1, -2, -1},
        {0,  0,  0},
        {1,  2,  1}
    };
    
    //----------------------------------------------------------------------
    // 3. Sobel-Kantenberechnung
    //----------------------------------------------------------------------
    
    
    for (int y = 1; y < HEIGHT - 1; y++) {
        for (int x = 1; x < WIDTH - 1; x++) {
            
            int sumX = 0;
            int sumY = 0;
            
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    int pixel = boats[(y + ky) * WIDTH + (x + kx)];
                    sumX += pixel * Gx[ky + 1][kx + 1];
                    sumY += pixel * Gy[ky + 1][kx + 1];
                }
            }
            
            int magnitude = iabs(sumX) + iabs(sumY);
            if (magnitude > 255) magnitude = 255;
            
            edges[y * WIDTH + x] = (uint8_t)magnitude;
        }
    }
    
    //----------------------------------------------------------------------
    // 4. Overlay-Bild erzeugen (reines RGB-Array)
    //----------------------------------------------------------------------
    for (int i = 0; i < NPIX; i++) {
        uint8_t v = gray[i];
        
        overlay[3*i + 0] = v;
        overlay[3*i + 1] = v;
        overlay[3*i + 2] = v;
        
        if (edges[i] > 100) {
            overlay[3*i + 0] = 255; // rot
            overlay[3*i + 1] = 0;
            overlay[3*i + 2] = 0;
        }
    }

    // GPIO_OUTPUT_VAL &= ~1;                                                  // Added
    // REG32(GPIO_BASE_ADDR, GPIO_VALUE) &= ~(1 << 0);
    
    uart_puts("Edge detection finished\n");
    gpio[3] = 0x0;
    
    
    //----------------------------------------------------------------------
    // 5. Ergebnis in export-Array kopieren (für Host)
    //----------------------------------------------------------------------
    
    //uart_send_marker();                     // send start marker
    //uart_send_image(edges, WIDTH * HEIGHT); // send all pixels as raw bytes
    
    
    return 0;
}



