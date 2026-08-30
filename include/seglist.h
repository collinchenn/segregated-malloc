#ifndef SEGLIST_H
#define SEGLIST_H

#include <stddef.h>
#include "block.h"

/* Exact-size classes (16B apart) up to a cap, plus one large catch-all. */
#define NUM_SIZE_CLASSES 64

/* seglist-specific helper, not part of the generic freelist interface.
 * Exposed for unit testing (see tests/test_seglist.c). */
int seglist_index_for_size(size_t size);

#endif /* SEGLIST_H */
