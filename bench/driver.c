#include "workloads.h"
#include "alloc_shim.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#ifndef BENCH_LABEL
#define BENCH_LABEL "unknown"
#endif

static long long now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

static void replay(const workload_t *w, void **slots) {
    for (size_t i = 0; i < w->num_ops; i++) {
        const op_t *op = &w->ops[i];
        if (op->kind == OP_MALLOC)
            slots[op->slot] = bench_malloc(op->size);
        else {
            bench_free(slots[op->slot]);
            slots[op->slot] = NULL;
        }
    }
}

static void drain(void **slots, int num_slots) {
    for (int i = 0; i < num_slots; i++) {
        if (slots[i]) {
            bench_free(slots[i]);
            slots[i] = NULL;
        }
    }
}

static double measure_utilization(const workload_t *w, void **slots) {
    size_t *slot_size = calloc((size_t)w->num_slots, sizeof(size_t));
    size_t live = 0, peak = 0;

    for (size_t i = 0; i < w->num_ops; i++) {
        const op_t *op = &w->ops[i];
        if (op->kind == OP_MALLOC) {
            slots[op->slot] = bench_malloc(op->size);
            slot_size[op->slot] = op->size;
            live += op->size;
            if (live > peak) peak = live;
        } else if (slots[op->slot]) {
            live -= slot_size[op->slot];
            bench_free(slots[op->slot]);
            slots[op->slot] = NULL;
        }
    }

    size_t heap = bench_heap_bytes();
    free(slot_size);
    drain(slots, w->num_slots);
    return heap ? (double)peak / (double)heap : 0.0;
}

static long long measure_throughput_ns(const workload_t *w, void **slots, int reps) {
    long long min_ns = LLONG_MAX;

    // mmap heap is lazily paged, so physical memory isn't attatched
    // until the pages are touched. We `warm` the pages up by running
    // once but not recording the time since we will encounter page faults
    replay(w, slots);
    drain(slots, w->num_slots);

    for (int i = 0; i < reps; i++) {
        long long start_ns = now_ns();
        replay(w, slots);
        long long elapsed = now_ns() - start_ns;

        min_ns = elapsed < min_ns ? elapsed : min_ns;
        drain(slots, w->num_slots);
    }

    return min_ns;
}

int main(int argc, char **argv) {
    const char *wl   = argc > 1 ? argv[1] : "random";
    size_t num_ops   = argc > 2 ? strtoull(argv[2], NULL, 10) : 1000000;
    int    reps      = argc > 3 ? atoi(argv[3]) : 5;
    int    num_slots = argc > 4 ? atoi(argv[4]) : 1024;

    workload_t w;
    if (strcmp(wl, "fixed") == 0)
        w = workload_fixed_size(64, num_slots, num_ops, 1);
    else
        w = workload_random(16, 512, num_slots, num_ops, 1);

    if (bench_init() != 0) {
        fprintf(stderr, "bench_init failed\n");
        return 1;
    }

    void **slots = calloc((size_t)num_slots, sizeof(void *));

    double util = measure_utilization(&w, slots);
    long long min_ns = measure_throughput_ns(&w, slots, reps);

    printf("[%-8s] workload=%-6s ops=%zu reps=%d  ",
           BENCH_LABEL, w.name, w.num_ops, reps);
    if (min_ns > 0) {
        double mops = (double)w.num_ops / ((double)min_ns / 1e9) / 1e6;
        printf("throughput=%.2f Mops/s  ", mops);
    } else {
        printf("throughput=TODO       ");
    }
    if (util > 0.0)
        printf("util=%.1f%%\n", util * 100.0);
    else
        printf("util=n/a\n");

    free(slots);
    workload_free(&w);
    return 0;
}
