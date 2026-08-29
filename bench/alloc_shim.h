#ifndef ALLOC_SHIM_H
#define ALLOC_SHIM_H

#include <stddef.h>

#ifdef BENCH_GLIBC

#include <stdlib.h>

static inline int    bench_init(void)       { return 0; }
static inline void  *bench_malloc(size_t s) { return malloc(s); }
static inline void   bench_free(void *p)    { free(p); }
static inline size_t bench_heap_bytes(void) { return 0; }

#else

#include "allocator.h"
#include "heap.h"

static inline int    bench_init(void)       { return seg_malloc_init(); }
static inline void  *bench_malloc(size_t s) { return seg_malloc(s); }
static inline void   bench_free(void *p)    { seg_free(p); }
static inline size_t bench_heap_bytes(void) {
    return (size_t)((char *)heap_end() - (char *)heap_start());
}

#endif

#endif /* ALLOC_SHIM_H */
