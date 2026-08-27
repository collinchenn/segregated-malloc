#include "allocator.h"
#include "seglist.h"
#include "block.h"
#include "heap.h"

#define PAD 8
#define MIN_BLOCK 32

// private APIs

static void split_block(block_t *b, size_t size) {
    int remainder = block_get_size(b) - size;
    
    // Splitting the block leaves a chunk
    // that is greater than the minimum
    if (remainder >= MIN_BLOCK) {
        block_set_size(b, size);

        block_t* remainder_b = block_next(b);
        block_set_size(remainder_b, remainder);
        block_set_free(remainder_b, true);

        seglist_insert(remainder_b);
    }
}

// public APIs

int seg_malloc_init(void) {
    if (heap_init() == -1) return -1;

    seglist_init();
    heap_extend(PAD);

    return 0;
}

void *seg_malloc(size_t size) {
    if (size == 0) return NULL;

    size_t need = block_size_for_payload(size);
    block_t *b = seglist_find_fit(need);
    
    if (b != NULL) {
        // We were able to find a block for
        // the requested payload size to malloc
        seglist_remove(b);
        split_block(b, need);

    } else {
        // We need to reqeust for more memory
        // in order to fufill the request
        b = heap_extend(need);
        if (b == NULL) return NULL;
        block_set_size(b, need);
    }

    block_set_free(b, false);
    return block_payload(b);
}

void seg_free(void *ptr) {
    if (ptr == NULL) return;

    block_t *b = block_from_payload(ptr);
    size_t b_size = block_get_size(b);

    // Left side is free - coalese-left
    block_t *left = block_prev(b);
    if ((char *)b > (char *)heap_start() + PAD && block_is_free(left)) {
        seglist_remove(left);
        size_t left_size = block_get_size(left);

        block_set_size(left, left_size + b_size);
        b = left;
        b_size += left_size;
    }

    // Right side is free - coalese-right
    block_t *right = block_next(b);
    if ((char *)right < (char *)heap_end() && block_is_free(right)) {
        seglist_remove(right);
        size_t right_size = block_get_size(right);

        block_set_size(b, b_size + right_size);
    }

    block_set_free(b, true);
    seglist_insert(b);
}

void *seg_calloc(size_t nmemb, size_t size) {
    if (nmemb != 0 && size > SIZE_MAX / nmemb) 
        return NULL;

    size_t array_size = nmemb * size;
    void *payload = seg_malloc(array_size);

    if (payload == NULL) 
        return NULL;

    if (memset(payload, 0, array_size) == NULL)
        return NULL;

    return payload;
}

void *seg_realloc(void *ptr, size_t size) {
    if (ptr == NULL)
        return seg_malloc(size);

    if (size == 0) {
        seg_free(ptr);
        return NULL;
    }

    block_t *b = block_from_payload(ptr);
    size_t payload_size = block_get_payload_size(b);
    if (payload_size < size) {
        void *new_ptr = seg_malloc(size);
        if (new_ptr == NULL) return NULL;

        memcpy(new_ptr, ptr, payload_size);
        seg_free(ptr);
        ptr = new_ptr;
    }

    return ptr;
}

int seg_heap_check(void) {
    // Walk the heap, check
    // 1. size alignment and >= min_block
    // 2. header == footer
    // 3. no two adjacent free blocks
    char *end = (char *)heap_end();
    block_t *prev = NULL;
    block_t *curr = (block_t*)((char *)heap_start() + PAD);

    while ((char *)curr < end) {
        size_t size = block_get_size(curr);
        if (size % ALIGNMENT != 0 || size < MIN_BLOCK) return -1;
        if (!block_is_consistent(curr)) return -1;
        if (block_is_free(curr) && prev && block_is_free(prev)) return -1;

        prev = curr;
        curr = block_next(curr);
    }

    if ((char *)curr != end) return -1;

    // Then, walk the seg list, check
    // 1. all blocks are marked as free
    // 2. it's in the right bucket

    return 0;
}
