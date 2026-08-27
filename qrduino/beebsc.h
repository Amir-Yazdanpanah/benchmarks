#ifndef BEEBSC_H
#define BEEBSC_H

#include <stddef.h>

void init_heap_beebs(void *heap_ptr, size_t size);
void *malloc_beebs(size_t size);
void *calloc_beebs(size_t num, size_t size);
void free_beebs(void *ptr);
int check_heap_beebs(void *heap_ptr);

#endif