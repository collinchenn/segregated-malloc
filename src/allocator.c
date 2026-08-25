#include "allocator.h"
#include "seglist.h"
#include "block.h"
#include "heap.h"

#define PAD 8
#define MIN_BLOCK 32

// private APIs

static void split_block(block_t *b, size_t size) {
    (void)b; (void)size; /* TODO */
}

static block_t *coalesce(block_t *b) {
    (void)b; /* TODO */
    return NULL;
}

// public APIs

int seg_malloc_init(void) {
    if (heap_init() == -1) return -1;

    seglist_init();
    heap_extend(PAD);

    return 0;
}

void *seg_malloc(size_t size) {
    if (size == 0) return NULL;

    size_t need = block_size_for_payload(size);
    block_t *b = seglist_find_fit(need);
    
    if (b != NULL) {
        // We were able to find a block for
        // the requested payload size to malloc
        seglist_remove(b);
        split_block(b, need);

    } else {
        // We need to reqeust for more memory
        // in order to fufill the request
        b = heap_extend(need);
        if (b == NULL) return NULL;
        block_set_size(b, need);
    }

    block_set_free(b, false);
    return block_payload(b);
}

void seg_free(void *ptr) {
    (void)ptr;
}

void *seg_calloc(size_t nmemb, size_t size) {
    (void)nmemb; (void)size;
    return NULL; /* stub */
}

void *seg_realloc(void *ptr, size_t size) {
    (void)ptr; (void)size;
    return NULL; /* stub */
}

int seg_heap_check(void) {
    (void)split_block; (void)coalesce;
    return 0;
}
