# segregated-malloc

A from-scratch C memory allocator using a **segregated free list** design.
I'm building this project to improve a memory allocator I built in my systems
programming class, which was architected with an **explicit free list** design,
and improve fluency with performance benchmarking.

## Benchmarks

Measured against my earlier explicit-list allocator and glibc `malloc`
across a set of allocation traces.

| Metric                    | Explicit list | Segregated list | system `malloc` |
| ------------------------- | ------------- | --------------- | --------------- |
| Throughput (Kops/s)       | TBD           | TBD             | TBD             |
| Memory utilization (%)    | TBD           | TBD             | TBD             |
| Avg. `malloc` latency (ns)| TBD           | TBD             | TBD             |

## Background on Explicit free lists

An allocator has to track which blocks of the heap are free so it can hand
them back out efficiently. The explicit free list threads a single doubly-linked list
through every free block.

```
  head
   │
   v
 ┌──────┐     ┌──────┐     ┌──────┐
 │ free │ <-> │ free │ <-> │ free │ ->
 │  48  │     │  64  │     │  16  │     
 └──────┘     └──────┘     └──────┘
```

However, allocation is slow. When using `malloc()`, the allocator walks the list
looking for the first (or best) block that fits, an **O(number of free
blocks)** scan. As the heap fills with many small free blocks, that search
dominates, and unrelated sizes sitting in one list worsens fragmentation.

## Switching to Segregated free lists

A segregated free list replaces the single list with an **array of lists**,
one per *size class*. Each free block lives in the bucket for its size range,
so finding a fit means jumping straight to the right bucket and scanning only
blocks already known to be close to the requested size.

```
  size class        free lists
 ┌──────────┐
 │  16– 32  │ -> [32] -> [16]
 │  33– 64  │ -> NULL
 │  65–128  │ -> [128] -> [96] -> [80]
 │   ...    │
 │  >1024   │ -> [4096]
 └──────────┘
  index by size, then scan one short bucket, near O(1) allocation
```

Grouping blocks by size makes allocation fast and keeps similar sizes
together, which reduces fragmentation compared to the explicit list.

## Project Specifications

- Segregated Free List:
- Instant Coalescing:
- Block definition
- Byte alignment?

## Layout

```
include/       public + internal headers
  heap.h         layer 1: get memory from the OS
  block.h        layer 2: block metadata & navigation
  seglist.h      layer 3: segregated free lists
  allocator.h    layer 4: malloc/free/realloc/calloc API
src/           implementations (currently stubs)
tests/         test harness
Makefile       build the static lib + tests
```

## Build & test

```sh
make          # build the static library
make test     # build & run the tests
make clean
```
