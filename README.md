# segregated-malloc

This is a from-scratch C memory allocator that uses a **segregated free list** design.
In my systems programming classes I took at university, one of the projects involved 
implementing a memory allocator, but it was pretty slow because it used an **explicit free list** design.

My professor mentioned that modern library memory allocators use **segregated free lists**,
so I was curious just how much of a difference it would make. I decided to implement it in my own time to find out
and learn performance benchmarking while I was at it. Below are the results and the project architecture.

## Benchmarks

### Methodology

Single-threaded, complied with -O2 flag. The workload is 1,000,000 `malloc`/`free` operations 
of random sizes (16–512 B), replayed from an identical pre-generated trace against each
allocator. `num_slots` = the maximum number of simultaneously-live allocations.

Throughput is the best of 5 timed runs (after a warm-up).
Utilization is the result of peak live payload / peak heap. Latency is per-`malloc` timing.
These were all measured on Apple Silicon (macOS). 

### Throughput (Mops/s)

| Working set (`num_slots`) | Explicit list | Segregated list | system `malloc` |
| ------------------------- | ------------- | --------------- | --------------- |
| 1,024                     | 44            | 48              | 62              |
| 16,384                    | 29            | 55              | 44              |
| 65,536                    | 14            | 56              | 39              |
| 262,144                   | 7.5           | 38              | 25              |

### Memory utilization (%)

| Working set (`num_slots`) | Explicit list | Segregated list | system `malloc` |
| ------------------------- | ------------- | --------------- | --------------- |
| 1,024                     | 74            | 87              | N/A             |
| 16,384                    | 74            | 90              | N/A             |
| 65,536                    | 73            | 91              | N/A             |
| 262,144                   | 74            | 91              | N/A             |

_Note: system `malloc` utilization is N/A because the heap size isn't observable from the driver_

### Worst-case `malloc` latency (µs)

| Working set (`num_slots`) | Explicit list | Segregated list | system `malloc` |
| ------------------------- | ------------- | --------------- | --------------- |
| 1,024                     | 5             | 5               | 8               |
| 16,384                    | 36            | 1               | 4               |
| 65,536                    | 121           | 6               | 9               |
| 262,144                   | ~2,000        | 8               | 20              |

_Note: 99% of allocations complete in under 1 µs for all three allocators. The difference is
entirely in the extreme tail. The explicit list's worst case blows up to ~2 ms with large working sets
while the segregated list stays flat_

