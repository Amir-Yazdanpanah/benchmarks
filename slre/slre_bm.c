#include <stdint.h>
#include <stddef.h>
#include "encoding.h"

/* --------------------------------------------------------------------------
 * Memory-Mapped I/O Definitions (UART0 and GPIO)
 * -------------------------------------------------------------------------- */
#define GPIO0           0x64002000
#define GPIO_VALUE      (*(volatile uint32_t *)(GPIO0))
#define GPIO_OUTPUT_EN  (*(volatile uint32_t *)(GPIO0 + 0x08))

#define UART0           0x64000000
#define UART_TXDATA     (*(volatile uint32_t *)(UART0))
#define UART_TXCTRL     (*(volatile uint32_t *)(UART0 + 0x08))

/* --------------------------------------------------------------------------
 * Bare-Metal UART Output Utilities
 * -------------------------------------------------------------------------- */
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

/* --------------------------------------------------------------------------
 * Minimal Helper Functions (Freestanding / Nostdlib Support)
 * -------------------------------------------------------------------------- */
static size_t slre_strlen(const char *s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

static int slre_tolower(int c) {
    if (c >= 'A' && c <= 'Z') return c + ('a' - 'A');
    return c;
}

static int slre_isdigit(int c) {
    return c >= '0' && c <= '9';
}

static int slre_isspace(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static int slre_isalpha(int c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static int slre_isalnum(int c) {
    return slre_isalpha(c) || slre_isdigit(c);
}

/* --------------------------------------------------------------------------
 * Embench SLRE (Super Light Regular Expression) Core Engine
 * -------------------------------------------------------------------------- */
#define SLRE_MAX_BRACKET  16

enum {
    SLRE_END = 0,
    SLRE_BRANCH,
    SLRE_EXACT,
    SLRE_ANY,
    SLRE_ANYOF,
    SLRE_ANYBUT,
    SLRE_DIGIT,
    SLRE_SPACE,
    SLRE_ALPHA,
    SLRE_NONSPACE
};

struct cap {
    const char *ptr;
    int len;
};

struct slre {
    unsigned char code[256];
    int code_len;
    int num_caps;
};

static int compile_bracket(const char **re, unsigned char *code, int *code_len) {
    const char *p = *re;
    int is_neg = 0;
    
    if (*p == '^') {
        is_neg = 1;
        p++;
    }

    code[(*code_len)++] = is_neg ? SLRE_ANYBUT : SLRE_ANYOF;
    int count_idx = (*code_len)++;
    int count = 0;

    while (*p && *p != ']') {
        code[(*code_len)++] = (unsigned char)*p++;
        count++;
    }

    if (*p != ']') return 0;
    *re = p;
    code[count_idx] = (unsigned char)count;
    return 1;
}

static int slre_compile(struct slre *r, const char *re) {
    r->code_len = 0;
    r->num_caps = 0;

    while (*re) {
        if (r->code_len >= (int)sizeof(r->code) - 4) return 0;

        switch (*re) {
            case '.':
                r->code[r->code_len++] = SLRE_ANY;
                break;
            case '[':
                re++;
                if (!compile_bracket(&re, r->code, &r->code_len)) return 0;
                break;
            case '\\':
                re++;
                if (*re == 'd') r->code[r->code_len++] = SLRE_DIGIT;
                else if (*re == 's') r->code[r->code_len++] = SLRE_SPACE;
                else if (*re == 'S') r->code[r->code_len++] = SLRE_NONSPACE;
                else if (*re == 'a') r->code[r->code_len++] = SLRE_ALPHA;
                else {
                    r->code[r->code_len++] = SLRE_EXACT;
                    r->code[r->code_len++] = (unsigned char)*re;
                }
                break;
            default:
                r->code[r->code_len++] = SLRE_EXACT;
                r->code[r->code_len++] = (unsigned char)*re;
                break;
        }
        re++;
    }
    r->code[r->code_len++] = SLRE_END;
    return 1;
}

static int match_node(const unsigned char *code, const char *buf, int buf_len, int *consumed) {
    int i;
    switch (code[0]) {
        case SLRE_END:
            *consumed = 0;
            return 1;
        case SLRE_ANY:
            if (buf_len < 1) return 0;
            *consumed = 1;
            return 1;
        case SLRE_EXACT:
            if (buf_len < 1 || (unsigned char)buf[0] != code[1]) return 0;
            *consumed = 1;
            return 2;
        case SLRE_DIGIT:
            if (buf_len < 1 || !slre_isdigit((unsigned char)buf[0])) return 0;
            *consumed = 1;
            return 1;
        case SLRE_SPACE:
            if (buf_len < 1 || !slre_isspace((unsigned char)buf[0])) return 0;
            *consumed = 1;
            return 1;
        case SLRE_NONSPACE:
            if (buf_len < 1 || slre_isspace((unsigned char)buf[0])) return 0;
            *consumed = 1;
            return 1;
        case SLRE_ALPHA:
            if (buf_len < 1 || !slre_isalpha((unsigned char)buf[0])) return 0;
            *consumed = 1;
            return 1;
        case SLRE_ANYOF:
            if (buf_len < 1) return 0;
            for (i = 0; i < code[1]; i++) {
                if ((unsigned char)buf[0] == code[2 + i]) {
                    *consumed = 1;
                    return 2 + code[1];
                }
            }
            return 0;
        case SLRE_ANYBUT:
            if (buf_len < 1) return 0;
            for (i = 0; i < code[1]; i++) {
                if ((unsigned char)buf[0] == code[2 + i]) return 0;
            }
            *consumed = 1;
            return 2 + code[1];
        default:
            return 0;
    }
}

static int slre_match(struct slre *r, const char *buf, int buf_len) {
    int code_idx = 0;
    int buf_idx = 0;

    while (code_idx < r->code_len && r->code[code_idx] != SLRE_END) {
        int consumed = 0;
        int step = match_node(&r->code[code_idx], &buf[buf_idx], buf_len - buf_idx, &consumed);
        if (step == 0) return 0;
        code_idx += step;
        buf_idx += consumed;
    }
    return 1;
}

/* --------------------------------------------------------------------------
 * Benchmark Workload Data (Embench SLRE Test Case Pattern Sets)
 * -------------------------------------------------------------------------- */
static const char *test_patterns[] = {
    "\\d\\d\\d-\\d\\d\\d\\d",
    "[a-z]loader\\s\\d",
    "boot_rom_\\a\\a",
    "\\S\\S\\S\\S-\\d\\d"
};

static const char *test_strings[] = {
    "Call 555-0199 for info",
    "riscv loader 1 active",
    "boot_rom_vc target set",
    "VC707-01 board online"
};

#define NUM_TESTS (sizeof(test_patterns) / sizeof(test_patterns[0]))
#define BENCHMARK_ITERATIONS 1000

/* --------------------------------------------------------------------------
 * Main Entry Point
 * -------------------------------------------------------------------------- */
int main(void) {
    uint64_t start, end;
    volatile unsigned int *gpio = (unsigned int *)GPIO0;

    // Set GPIO Output Enable for Pin 0
    gpio[2] = 0x1;
    // Drive Pin 0 LOW initially
    gpio[3] = 0x0;

    uart_puts("[INFO] Starting Embench SLRE benchmark...\n");

    // Signal execution start to hardware telemetry (Pin 0 HIGH)
    gpio[3] = 0x1;
    start = read_csr(cycle);

    // Carry out the benchmark loop over test patterns and payload strings
    int matches = 0;
    for (int iter = 0; iter < BENCHMARK_ITERATIONS; iter++) {
        for (size_t i = 0; i < NUM_TESTS; i++) {
            struct slre rx;
            if (slre_compile(&rx, test_patterns[i])) {
                size_t len = slre_strlen(test_strings[i]);
                for (size_t offset = 0; offset < len; offset++) {
                    if (slre_match(&rx, test_strings[i] + offset, len - offset)) {
                        matches++;
                    }
                }
            }
        }
    }

    end = read_csr(cycle);
    // Signal execution end to hardware telemetry (Pin 0 LOW)
    gpio[3] = 0x0;

    uart_puts("Embench SLRE finished\n");
    uart_puts("Total Matches: ");
    uart_print_dec(matches);
    uart_puts("\nElapsed Cycles: ");
    uart_print_dec(end - start);
    uart_puts("\n");

    return 0;
}