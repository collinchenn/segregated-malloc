# segregated-malloc

A from-scratch C memory allocator that uses a **segregated free list** design

In the systems programming classes I took at university, one of the projects involved
implementing a memory allocator, but it was pretty slow because it used an **explicit free list**
design. My professor mentioned that modern library allocators use **segregated free lists**, so I
was curious just how much of a difference it would make. I decided to implement one in my own time
to find out and to learn performance benchmarking while I was at it

The result beats the explicit-list allocator on **throughput, memory utilization, and tail
latency** simultaneously, and holds its own against the system `malloc`

---

## Results, at a high level

Note that all of these benchmarks were measured on an Apple M3 Pro (macOS, single-threaded, `-O2` flag), 
against the same allocator built with an explicit free list and against the system `malloc`

- **Throughput:** up to 6x faster than the explicit list on small-allocation workloads
- **Memory utilization:** 87–95% versus the explicit list's ~74%, across every working-set size
- **Worst-case malloc latency:** stays flat (~7 µs) as the working set grows to 262k live
  allocations, while the explicit list degrades to ~2.9 ms (a ~400x difference in the tail!)

The full results of the benchmarks can be found at the bottom of the README (or [here](#benchmarks))

---

## Architecture

The allocator is built as four layers, each depending only on the ones below it. This isolates
pointer arithmetic to one section and makes the layers independently testable

```
allocator   seg_malloc, seg_free, seg_calloc, seg_realloc orchestration 
    |        
    v
 seglist    segregated free lists implementation. this backend is swappable
    │       on link-time with explicitlist as they share the same interface
    v
  block     block model, header/footer, size + alloc bit, boundary tags
    │       all pointer arithmetic done here
    v
  heap      the only layer that talks to the OS (reserve + bump via mmap)
```

### 1. `heap`: OS memory

Reserves a large virtual region up front with `mmap` and hands out memory by bumping a pointer.
Because the pages are lazily backed by physical memory only when first touched, the reservation is
effectively free until used. Everything above treats the heap as one contiguous growable region

### 2. `block`: the block model

The heap is carved into blocks. Each block has an 8-byte header storing its size and free
bit. Each block also carries a 8-byte footer with the same info. Since sizes are
aligned to 16 bytes, the low bits of the size word are always zero, so the lowest bit is reused as
the allocated flag

The boundary tag makes coalescing cheap: from any block you can read the previous
block's footer to find its size and free bit, so merging with the physical neighbor on the left is
instant. Payloads are 16-byte aligned (valid for any C type), with a small prologue pad so the very
first payload lands on an aligned boundary

### 3. `seglist`: the segregated free lists

An array of 64 size-class buckets, each an intrusive doubly-linked list. This means `next`/`prev`
pointers live inside the free block's own payloads, so the free lists cost zero extra memory

- Small blocks (<= 512 B): one exact-size bucket per 16-byte size. Each
  bucket holds a single size, so a fit is found instantly with no scan
- Large blocks (> 512 B): fine log-spaced buckets. Size classes double per octave, and each
  octave is split into 4 equal sub-buckets. This
  keeps the count of buckets logarithmic while keeping each bucket's list short

### 4. `allocator`: public API

Implements malloc, free, and others. It orchestrates the lower layers and owns two helpers:

A consistency checker, `seg_heap_check`, walks the whole heap and all free lists asserting no
overlaps, that free/allocated bits agree with list membership, and that sizes stay aligned. The test
suite calls it after every operation

---

## Design decisions

### Segregated storage

The single most important decision. A naive segregated list uses range buckets (e.g. one bucket
per power-of-two band) and does a first-fit search within a bucket. However, real workloads
cluster their sizes, so nearly all blocks fall into the same one or two wide buckets. The problem is that
now we just have explicit free lists in disguise, plus the overhead of managing all the empty
buckets. My first range-bucketed version was actually slower than the explicit list design

The fix is to make each bucket narrow enough that its list stays short:

- Small sizes have few distinct values (~30 up to 512 B), so I can afford an exact bucket per
  size. A fit is the list head, always, with zero scanning
- Large sizes span millions of distinct values, so exact buckets are infeasible. Instead I use
  fine log-spaced buckets (4 per octave), narrow enough so lists stay short

In other words, buckets only help when they're narrow relative to the workload's size spread. A wide
bucket is an explicit list wearing a costume. This is also visible in the data: a coarse
power-of-two version of the large buckets ran at ~4 Mops/s (worse than explicit); narrowing to 4
sub-buckets per octave took it to ~18 Mops/s

### A swappable free-list backend

`heap`, `block`, and `allocator` are backend-independent. Additionally, the free-list layer is defined by a single
interface (`freelist_insert` / `freelist_remove` / `freelist_find_fit` / …), which are implemented by the two
data structures (explicit list and segregated list). This allows us to select one at link time
(`make ... FREELIST=explicit`), making the head-to-head benchmark a more unbiased comparison since we have identical block layer, coalescing, and drivers, with only the freelist
strategy swapped

---

## Benchmarks

### Methodology

These benchmarks were ran in an environment that was single-threaded and compiled at `-O2`. Each run replays an identical, 
pre-generated, randomized, trace of 1,000,000 `malloc`/`free` operations against each allocator. `num_slots` is the maximum number of
simultaneously-live allocations (the working set), which controls how long the free lists get, which
is what determines whether segregation can pay off

There are three metrics:
- Throughput is the best of 10 timed runs after a memory paging warm-up
- Utilization is peak live payload / peak heap size
- Latency is measured per `malloc`; reported as the worst-case observed. The tail is where the
  designs differ since 99% of all allocations finish in under 1 µs for every allocator

There are two types of allocations that we are concerned with here: small allocations (16–512 B, which live entirely in the
exact-size buckets) and large allocations (1024–16384 B, which exercise the log-spaced buckets)

### Small allocations (16–512 B)

Throughput (Mops/s)

| Working set (`num_slots`) | Explicit | Segregated | system `malloc` |
| ------------------------- | -------- | ---------- | --------------- |
| 1,024                     | 48       | 49         | **68**          |
| 16,384                    | 31       | **58**     | 48              |
| 65,536                    | 15       | **57**     | 43              |
| 262,144                   | 7.3      | **43**     | 24              |

Utilization (%)

| Working set (`num_slots`) | Explicit | Segregated | system `malloc` |
| ------------------------- | -------- | ---------- | --------------- |
| 1,024                     | 74       | **87**     | N/A             |
| 16,384                    | 74       | **90**     | N/A             |
| 65,536                    | 73       | **91**     | N/A             |
| 262,144                   | 74       | **91**     | N/A             |

Worst-case `malloc` latency (µs)

| Working set (`num_slots`) | Explicit  | Segregated | system `malloc` |
| ------------------------- | --------- | ---------- | --------------- |
| 1,024                     | 7         | 8          | **5**           |
| 16,384                    | 21        | **1**      | 14              |
| 65,536                    | 133       | **9**      | 15              |
| 262,144                   | **~2,864**| **7**      | 65              |

### Large allocations (1024–16384 B)

Throughput (Mops/s)

| Working set (`num_slots`) | Explicit | Segregated | system `malloc` |
| ------------------------- | -------- | ---------- | --------------- |
| 1,024                     | 46       | 33         | **48**          |
| 8,192                     | 22       | 22         | **40**          |
| 16,384                    | 17       | **18**     | **37**          |

Utilization (%): segregated holds 92–95% across the board vs the explicit list's 79–80%.

Worst-case latency (µs): segregated stays flat (~11–19 µs), but the explicit list spikes to
250–450 µs and the system `malloc` to ~200 µs on the large working sets

---

## Analysis

- Utilization and latency is strictly better with segregated lists. Exact/narrow buckets return a block that's
  the right size, so there's little wasted space (high utilization), and a lookup never degrades into
  a long scan (flat tail latency). The explicit list's worst case grows without bound as the working
  set does because a single unlucky allocation may walk a very long list

- Throughput depends on free-list length. Segregation's whole job is to eliminate or reduce the search.
  When the working set is small, the explicit list's search is already short, so there's nothing to
  eliminate and it wins on constant factors. As the working set grows, the explicit list's search
  gets long and segregation pulls ahead

### Room for improvement

- A small caching tier (a short per-size free-block cache in front of the buckets) to close the
  large-allocation throughput gap with the system `malloc`
- Deferred coalescing to make frees even cheaper under churn
- Thread safety for a multi-threaded comparison with system malloc

---

## Build & test

```sh
make                       # build build/libsegmalloc-seg.a
make test-all              # run every layer's unit tests (segregated backend)
make test-alloc            # public API tests only
make test-alloc FREELIST=explicit   # same tests against the explicit-list backend
make clean
```

Compiler flags are strict (`-std=c11 -Wall -Wextra -Wpedantic`). To chase memory bugs, uncomment a
sanitizer line in the `Makefile` (`-fsanitize=address` or `-fsanitize=undefined`)

### Reproducing the benchmarks

```sh
make bench                 # run all three backends once at default settings
make bench-sweep           # sweep the working-set size across all three backends

# manual: <workload> <num_ops> <reps> <num_slots> [min_size] [max_size]
./build/bench-seg      random 1000000 10 65536 16 512      # small workload
./build/bench-explicit random 1000000 10 16384 1024 16384  # large workload
```
