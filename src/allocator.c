#include "allocator.h"
#include "seglist.h"
#include "block.h"
#include "heap.h"

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
    return -1; /* stub */
}

void *seg_malloc(size_t size) {
    (void)size;
    return NULL; /* stub */
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
