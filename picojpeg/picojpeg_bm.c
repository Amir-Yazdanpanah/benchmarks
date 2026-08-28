#include <stdint.h>
#include <stddef.h>
#include "encoding.h"
#include "picojpeg.h"

#define GPIO0 0x64002000
#define GPIO_VALUE      (*(volatile uint32_t *)(GPIO0))
#define GPIO_OUTPUT_EN  (*(volatile uint32_t *)(GPIO0 + 0x08))

#define UART0 0x64000000
#define UART_TXDATA (*(volatile uint32_t *)(UART0))
#define UART_TXCTRL (*(volatile uint32_t *)(UART0 + 0x08))

// Sample JPEG image payload embedded directly in memory (from Embench dataset)
static const uint8_t g_jpeg_data[] = {
    0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01,
    0x01, 0x01, 0x00, 0x48, 0x00, 0x48, 0x00, 0x00, 0xFF, 0xDB, 0x00, 0x43,
    0x00, 0x03, 0x02, 0x02, 0x03, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x04,
    0x03, 0x03, 0x04, 0x05, 0x08, 0x05, 0x05, 0x04, 0x04, 0x05, 0x0A, 0x07,
    0x07, 0x06, 0x08, 0x0C, 0x0A, 0x0C, 0x0C, 0x0B, 0x0A, 0x0B, 0x0B, 0x0D,
    0x0E, 0x12, 0x0F, 0x0D, 0x0E, 0x11, 0x0E, 0x0B, 0x0B, 0x10, 0x16, 0x10,
    0x11, 0x13, 0x14, 0x15, 0x15, 0x15, 0x0C, 0x0F, 0x17, 0x18, 0x16, 0x14,
    0x18, 0x12, 0x14, 0x15, 0x14, 0xFF, 0xC0, 0x00, 0x0B, 0x08, 0x00, 0x08,
    0x00, 0x08, 0x01, 0x01, 0x11, 0x00, 0xFF, 0xC4, 0x00, 0x1F, 0x00, 0x00,
    0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01, 0x00, 0x00, 0x3F,
    0x00, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xD9
};

static const uint32_t g_jpeg_size = sizeof(g_jpeg_data);
static uint32_t g_jpeg_bytes_pos = 0;

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

// In-memory stream reader callback for picojpeg
static unsigned char pjpg_need_bytes_callback(unsigned char *pBuf, unsigned char buf_size, unsigned char *pBytes_read, void *pCallback_data) {
    (void)pCallback_data;
    
    uint32_t bytes_left = g_jpeg_size - g_jpeg_bytes_pos;
    uint32_t bytes_to_read = (uint32_t)buf_size;
    
    if (bytes_to_read > bytes_left) {
        bytes_to_read = bytes_left;
    }
    
    for (uint32_t i = 0; i < bytes_to_read; i++) {
        pBuf[i] = g_jpeg_data[g_jpeg_bytes_pos++];
    }
    
    *pBytes_read = (unsigned char)bytes_to_read;
    return 0;
}

int main() {
    volatile unsigned int* gpio = (unsigned int*)GPIO0;
    
    // Set Output Enable for GPIO pin 0
    gpio[2] = 0x1;
    // Set Pin Value to LOW initially
    gpio[3] = 0x0;

    uart_puts("[INFO] Kick off picojpeg benchmark...\n");

    pjpg_image_info_t image_info;
    g_jpeg_bytes_pos = 0;

    // Toggle GPIO HIGH right before initiating execution
    gpio[3] = 0x1;
    uint64_t start_cycles = read_csr(cycle);

    uint8_t status = pjpg_decode_init(&image_info, pjpg_need_bytes_callback, NULL, 0);

    if (status != 0) {
        gpio[3] = 0x0;
        uart_puts("[ERROR] Header parsing failed with status: ");
        uart_print_dec(status);
        uart_puts("\n");
        return -1;
    }

    // Process all Minimum Coded Units (MCUs)
    while (status == 0) {
        status = pjpg_decode_mcu();
    }

    uint64_t end_cycles = read_csr(cycle);
    // Toggle GPIO LOW after wrapping up decode routine
    gpio[3] = 0x0;

    if (status == PJPG_NO_MORE_BLOCKS) {
        uart_puts("[INFO] Decoding completed successfully!\n");
    } else {
        uart_puts("[ERROR] Decoding failed mid-stream with status: ");
        uart_print_dec(status);
        uart_puts("\n");
    }

    uart_puts("[RESULT] Execution Cycles: ");
    uart_print_dec(end_cycles - start_cycles);
    uart_puts("\n");

    return 0;
}