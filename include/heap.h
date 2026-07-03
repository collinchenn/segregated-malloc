#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>

int heap_init(void);
void *heap_extend(size_t size);
void *heap_start(void);
void *heap_end(void);

#endif /* HEAP_H */
