#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>

int seg_malloc_init(void);
void *seg_malloc(size_t size);
void seg_free(void *ptr);
void *seg_calloc(size_t nmemb, size_t size);
void *seg_realloc(void *ptr, size_t size);
int seg_heap_check(void);

#endif /* ALLOCATOR_H */
