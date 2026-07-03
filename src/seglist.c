#include "seglist.h"
#include <stdio.h>

/* One list head per size class. Each head points at the first free
 * block in that bucket (intrusive doubly-linked list). */
static block_t *g_buckets[NUM_SIZE_CLASSES];

void seglist_init(void) {
    for (int i = 0; i < NUM_SIZE_CLASSES; i++)
        g_buckets[i] = NULL;
}

int seglist_index_for_size(size_t size) {
    (void)size;
    return 0;
}

void seglist_insert(block_t *b) {
    (void)b;
}

void seglist_remove(block_t *b) {
    (void)b;
}

block_t *seglist_find_fit(size_t size) {
    (void)size;
    return NULL; /* stub */
}

void seglist_dump(void) {
    printf("(seglist_dump: not implemented)\n");
}
