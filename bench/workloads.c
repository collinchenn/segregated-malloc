#include "workloads.h"

#include <stdbool.h>
#include <stdlib.h>

// Pseudo-random number generator
// used for reproducability when generating
// a seed as opposed to rand()
static unsigned xorshift(unsigned *state) {
    unsigned x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

// Simulate a valid workload with `num_slots` number
// of allocated blocks, with `num_ops` operations. 
// trace is built using `ops`
static workload_t generate(size_t min_size, size_t max_size, int num_slots,
                           size_t num_ops, unsigned seed, const char *name) {
    op_t *ops = malloc(num_ops * sizeof(op_t));
    bool *live = calloc((size_t)num_slots, sizeof(bool));
    unsigned state = seed ? seed : 1u;
    size_t span = max_size - min_size + 1;

    for (size_t i = 0; i < num_ops; i++) {
        // Pick a random slot, if it's free, FREE it. If
        // it's occupied, then MALLOC with a random size
        int slot = (int)(xorshift(&state) % (unsigned)num_slots);
        if (live[slot]) {
            ops[i].kind = OP_FREE;
            ops[i].slot = slot;
            ops[i].size = 0;
            live[slot] = false;
        } else {
            ops[i].kind = OP_MALLOC;
            ops[i].slot = slot;
            ops[i].size = min_size + (size_t)(xorshift(&state) % (unsigned)span);
            live[slot] = true;
        }
    }

    free(live);
    workload_t w = { ops, num_ops, num_slots, name };
    return w;
}

workload_t workload_fixed_size(size_t block_size, int num_slots,
                               size_t num_ops, unsigned seed) {
    return generate(block_size, block_size, num_slots, num_ops, seed, "fixed");
}

workload_t workload_random(size_t min_size, size_t max_size, int num_slots,
                           size_t num_ops, unsigned seed) {
    return generate(min_size, max_size, num_slots, num_ops, seed, "random");
}

void workload_free(workload_t *w) {
    free(w->ops);
    w->ops = NULL;
}
