#include "heap.h"

static void *g_heap_start = NULL;
static void *g_heap_end   = NULL;

int heap_init(void) {
    (void)g_heap_start;
    (void)g_heap_end;
    return -1; /* stub */
}

void *heap_extend(size_t size) {
    (void)size;
    return NULL;
}

void *heap_start(void) { return g_heap_start; }
void *heap_end(void)   { return g_heap_end;   }
