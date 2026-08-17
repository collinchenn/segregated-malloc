#include <sys/mman.h>

#include "heap.h"

#define MAX_HEAP ((size_t)256 * 1024 * 1024) // 256 mb

static char *g_heap_start = NULL;
static char *g_heap_end   = NULL;
static char *g_heap_cap   = NULL;

int heap_init(void) {
    void *p = mmap(NULL, MAX_HEAP, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANON, -1, 0);
    if (p == MAP_FAILED) return -1;
    g_heap_start = p;
    g_heap_end   = p;
    g_heap_cap   = (char *)p + MAX_HEAP;
    return 0;
}

void *heap_extend(size_t size) {
    char *old_end = g_heap_end;
    char *new_end = old_end + size;

    if (new_end > g_heap_cap) return NULL;

    g_heap_end = new_end;
    return old_end;
}

void *heap_start(void) { return g_heap_start; }
void *heap_end(void)   { return g_heap_end;   }
