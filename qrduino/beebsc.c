#include "beebsc.h"
#include <stdint.h>

static uint8_t *heap_start = 0;
static uint8_t *heap_curr = 0;
static size_t heap_total_size = 0;

void init_heap_beebs(void *heap_ptr, size_t size) {
    heap_start = (uint8_t *)heap_ptr;
    heap_curr = (uint8_t *)heap_ptr;
    heap_total_size = size;
}

void *malloc_beebs(size_t size) {
    // 8-byte alignment
    size = (size + 7) & ~((size_t)7);
    if ((heap_curr + size) > (heap_start + heap_total_size)) {
        return NULL;
    }
    void *ptr = heap_curr;
    heap_curr += size;
    return ptr;
}

void *calloc_beebs(size_t num, size_t size) {
    size_t total = num * size;
    uint8_t *ptr = (uint8_t *)malloc_beebs(total);
    if (ptr) {
        for (size_t i = 0; i < total; i++) {
            ptr[i] = 0;
        }
    }
    return ptr;
}

void free_beebs(void *ptr) {
    (void)ptr;
}

int check_heap_beebs(void *heap_ptr) {
    (void)heap_ptr;
    return 1;
}