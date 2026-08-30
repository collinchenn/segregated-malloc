CC      := cc
CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -g -O0 -Iinclude
# CFLAGS += -fsanitize=address
# CFLAGS += -fsanitize=undefined

SRC_DIR   := src
OBJ_DIR   := build
TEST_DIR  := tests

# Free-list backend: seg (segregated, default) or explicit (single list).
# Select with e.g. `make test-alloc FREELIST=explicit`.
# NOTE: switching FREELIST changes the library name, so no `make clean` is
# needed between backends.
FREELIST ?= seg
ifeq ($(FREELIST),explicit)
  FREELIST_SRC := $(SRC_DIR)/explicitlist.c
else
  FREELIST_SRC := $(SRC_DIR)/seglist.c
endif

# heap/block/allocator are backend-independent; the free-list backend is swapped.
SRCS := $(SRC_DIR)/heap.c $(SRC_DIR)/block.c $(SRC_DIR)/allocator.c $(FREELIST_SRC)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

LIB           := $(OBJ_DIR)/libsegmalloc-$(FREELIST).a
TEST          := $(OBJ_DIR)/test_allocator
TEST_BLOCK    := $(OBJ_DIR)/test_block
TEST_SEGLIST  := $(OBJ_DIR)/test_seglist
TEST_FREELIST := $(OBJ_DIR)/test_freelist

.PHONY: all clean test test-alloc test-block test-seglist test-freelist test-all

all: $(LIB)

# Compile each source file to an object file
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Bundle the object files into a static library (fresh archive each time)
$(LIB): $(OBJS)
	rm -f $@
	ar rcs $@ $^

# Build and run the full allocator (public API) tests against the chosen backend
test test-alloc: $(LIB)
	$(CC) $(CFLAGS) $(TEST_DIR)/test_allocator.c $(LIB) -o $(TEST)
	./$(TEST)

# Build and run the block-layer unit tests
test-block: $(LIB)
	$(CC) $(CFLAGS) $(TEST_DIR)/test_block.c $(LIB) -o $(TEST_BLOCK)
	./$(TEST_BLOCK)

# Build and run the seglist-specific unit tests (index_for_size; seg backend only)
test-seglist: $(LIB)
	$(CC) $(CFLAGS) $(TEST_DIR)/test_seglist.c $(LIB) -o $(TEST_SEGLIST)
	./$(TEST_SEGLIST)

# Build and run the free-list interface tests against the chosen backend
test-freelist: $(LIB)
	$(CC) $(CFLAGS) $(TEST_DIR)/test_freelist.c $(LIB) -o $(TEST_FREELIST)
	./$(TEST_FREELIST)

# Build and run every layer's tests (segregated backend)
test-all: test-block test-seglist test-freelist test-alloc

# --- Benchmarks (built at -O2, separate from the -O0 test build) ---
BENCH_DIR    := bench
BENCH_CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -O2 -Iinclude -I$(BENCH_DIR)
BENCH_DRV    := $(BENCH_DIR)/driver.c $(BENCH_DIR)/workloads.c
BENCH_CORE   := $(SRC_DIR)/heap.c $(SRC_DIR)/block.c $(SRC_DIR)/allocator.c
BENCH_BINS   := $(OBJ_DIR)/bench-seg $(OBJ_DIR)/bench-explicit $(OBJ_DIR)/bench-glibc

# Live-set sizes (num_slots) to sweep in bench-sweep
SWEEP_SLOTS  := 1024 16384 65536 262144

.PHONY: bench bench-seg bench-explicit bench-glibc bench-sweep

# --- build the benchmark binaries ---
$(OBJ_DIR)/bench-seg: $(BENCH_DRV) $(BENCH_CORE) $(SRC_DIR)/seglist.c | $(OBJ_DIR)
	$(CC) $(BENCH_CFLAGS) -DBENCH_LABEL='"seg"' $^ -o $@

$(OBJ_DIR)/bench-explicit: $(BENCH_DRV) $(BENCH_CORE) $(SRC_DIR)/explicitlist.c | $(OBJ_DIR)
	$(CC) $(BENCH_CFLAGS) -DBENCH_LABEL='"explicit"' $^ -o $@

$(OBJ_DIR)/bench-glibc: $(BENCH_DRV) | $(OBJ_DIR)
	$(CC) $(BENCH_CFLAGS) -DBENCH_LABEL='"glibc"' -DBENCH_GLIBC $^ -o $@

# --- run each once at default settings ---
bench: $(BENCH_BINS)
	@for b in $(BENCH_BINS); do $$b; done

bench-seg: $(OBJ_DIR)/bench-seg
	./$<
bench-explicit: $(OBJ_DIR)/bench-explicit
	./$<
bench-glibc: $(OBJ_DIR)/bench-glibc
	./$<

# --- sweep the live-set size across all three backends ---
bench-sweep: $(BENCH_BINS)
	@for n in $(SWEEP_SLOTS); do \
	  echo "=== num_slots=$$n ==="; \
	  for b in $(BENCH_BINS); do $$b random 1000000 5 $$n; done; \
	  echo ""; \
	done

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)
