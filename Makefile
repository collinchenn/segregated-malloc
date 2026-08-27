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

LIB          := $(OBJ_DIR)/libsegmalloc-$(FREELIST).a
TEST         := $(OBJ_DIR)/test_allocator
TEST_BLOCK   := $(OBJ_DIR)/test_block
TEST_SEGLIST := $(OBJ_DIR)/test_seglist

.PHONY: all clean test test-alloc test-block test-seglist test-all

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

# Build and run the seglist-layer unit tests (segregated backend only)
test-seglist: $(LIB)
	$(CC) $(CFLAGS) $(TEST_DIR)/test_seglist.c $(LIB) -o $(TEST_SEGLIST)
	./$(TEST_SEGLIST)

# Build and run every layer's tests
test-all: test-block test-seglist test-alloc

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)
