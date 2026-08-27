#ifndef SEGLIST_H
#define SEGLIST_H

#include <stddef.h>
#include "block.h"

#define NUM_SIZE_CLASSES 16

void seglist_init(void);
int seglist_index_for_size(size_t size);
void seglist_insert(block_t *b);
void seglist_remove(block_t *b);
block_t *seglist_find_fit(size_t size);
int seglist_check(void);
void seglist_dump(void);

#endif /* SEGLIST_H */
