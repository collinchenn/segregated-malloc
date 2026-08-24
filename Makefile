CC      := cc
CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -g -O0 -Iinclude
# CFLAGS += -fsanitize=address
# CFLAGS += -fsanitize=undefined

SRC_DIR   := src
OBJ_DIR   := build
TEST_DIR  := tests

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

LIB    := $(OBJ_DIR)/libsegmalloc.a
TEST   := $(OBJ_DIR)/test_allocator
TEST_BLOCK := $(OBJ_DIR)/test_block
TEST_SEGLIST := $(OBJ_DIR)/test_seglist

.PHONY: all clean test test-alloc test-block test-seglist test-all

all: $(LIB)

# Compile each source file to an object file
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Bundle the object files into a static library
$(LIB): $(OBJS)
	ar rcs $@ $^

# Build and run the full allocator (public API) tests
test test-alloc: $(LIB)
	$(CC) $(CFLAGS) $(TEST_DIR)/test_allocator.c $(LIB) -o $(TEST)
	./$(TEST)

# Build and run the block-layer unit tests
test-block: $(LIB)
	$(CC) $(CFLAGS) $(TEST_DIR)/test_block.c $(LIB) -o $(TEST_BLOCK)
	./$(TEST_BLOCK)

# Build and run the seglist-layer unit tests
test-seglist: $(LIB)
	$(CC) $(CFLAGS) $(TEST_DIR)/test_seglist.c $(LIB) -o $(TEST_SEGLIST)
	./$(TEST_SEGLIST)

# Build and run every layer's tests
test-all: test-block test-seglist test-alloc

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)
