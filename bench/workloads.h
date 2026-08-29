#ifndef WORKLOADS_H
#define WORKLOADS_H

#include <stddef.h>

typedef enum { OP_MALLOC, OP_FREE } op_kind_t;

typedef struct {
    op_kind_t kind;
    int       slot;
    size_t    size;
} op_t;

typedef struct {
    op_t       *ops;
    size_t      num_ops;
    int         num_slots;
    const char *name;
} workload_t;

workload_t workload_fixed_size(size_t block_size, int num_slots,
                               size_t num_ops, unsigned seed);
workload_t workload_random(size_t min_size, size_t max_size, int num_slots,
                           size_t num_ops, unsigned seed);
void workload_free(workload_t *w);

#endif /* WORKLOADS_H */
