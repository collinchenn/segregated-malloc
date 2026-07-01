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

## Design & specifications

### Alignment

Payloads are aligned to **16 bytes** to keep it consistent with the project
that was completed in my class (following CS:APP). Additionally that's the 
alignment real `malloc` guarantees and the minimum that works for any C type.

### Block layout

The heap is a contiguous run of **blocks**, each framed by an 8-byte
**header** and an 8-byte **footer** (boundary tags). Both store the block's
size; because sizes are a multiple of 16, the low bits of the size word are
free to reuse as flags (e.g. the allocated bit).

```
      addr ≡ 8 (mod 16)      addr ≡ 0 (mod 16)
            │                      │
            v                      v
          ┌────────┬───────────────────────────┬────────┐
          │ header │          payload          │ footer │
          │  8 B   │  (16-aligned, user data)  │  8 B   │
          └────────┴───────────────────────────┴────────┘
```

A **free** block reuses the first 16 bytes of its payload to store the
`next`/`prev` pointers of its free list — the lists are *intrusive*, so they
cost no extra memory.

```
   free block
  ┌────────┬────────┬────────┬────────┐
  │ header │  next  │  prev  │ footer │
  │  8 B   │  8 B   │  8 B   │  8 B   │
  └────────┴────────┴────────┴────────┘
```

### Block sizes

The **minimum block size is 32 bytes**, and every block size is a multiple of
16. The minimum is set by the free block above: even a 1-byte request must
leave room for a header, two list pointers, and a footer once it's freed.

### Boundary tags & navigation

Storing the size in both the header and footer lets the allocator walk the
heap in either direction:

- **Next block:** `header + block_size` lands on the next header.
- **Previous block:** `header - 8` reads the previous block's footer, giving
  its size and therefore its header.

Bidirectional navigation is what makes coalescing possible.

### Segregated free lists

Free blocks are indexed by **size class** into an array of list heads
(`NUM_SIZE_CLASSES` buckets). Classes are roughly power-of-two ranges
(`≤32`, `33–64`, `65–128`, …), with the last bucket a catch-all for everything
larger. New free blocks are pushed onto the **head** of their bucket (LIFO
insertion, O(1)).

### Placement & splitting

- **Fit:** first-fit *within the size-class bucket*. Because a bucket only
  holds blocks near the requested size, first-fit there approximates a
  global best-fit while staying near O(1).
- **Search:** start at the bucket for the requested size; if it's empty or has
  no fit, fall through to larger buckets.
- **Splitting:** if the chosen block is larger than needed, split off the
  remainder and return it to the free lists — but only if that remainder is at
  least the 32-byte minimum. Otherwise hand over the whole block.

### Coalescing

Coalescing is **immediate**: on every `free`, the block is merged with any
free physical neighbor on the left and/or right before being inserted. Using
the previous/next block's allocated bit, there are four cases:

| Left neighbor | Right neighbor | Result                     |
| ------------- | -------------- | -------------------------- |
| allocated     | allocated      | no merge — insert as-is    |
| allocated     | free           | merge with right           |
| free          | allocated      | merge with left            |
| free          | free           | merge all three into one   |

Merging removes the free neighbor(s) from their buckets, rewrites the combined
block's header/footer, and inserts the result into the correct bucket.

### Growing the heap

When no bucket can satisfy a request, the allocator asks the OS for more
memory (via `sbrk`/`mmap`), growing by `max(request, CHUNKSIZE)` (default 4 KB)
to amortize syscalls. The heap is framed by a **prologue** and **epilogue**
sentinel so the first and last real blocks never need special-casing during
navigation or coalescing.

### Planned optimization

TBD

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
