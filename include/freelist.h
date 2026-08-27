#ifndef FREELIST_H
#define FREELIST_H

#include <stddef.h>
#include "block.h"

// Generic free-list interface used by the allocator. Two implementations
// satisfy it: seglist.c (segregated free lists) and explicitlist.c (a
// single explicit free list). Exactly one is linked in, chosen at build
// time with `make FREELIST=seg` (default) or `make FREELIST=explicit`.

void     freelist_init(void);
void     freelist_insert(block_t *b);
void     freelist_remove(block_t *b);
block_t *freelist_find_fit(size_t size);
int      freelist_check(void);
void     freelist_dump(void);

#endif /* FREELIST_H */
