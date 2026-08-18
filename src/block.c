#include "block.h"

/*
 *
 *   struct block {
 *       size_t header;   // size in high bits, alloc flag in low bit
 *       // payload follows; footer sits at the end of the block
 *   };
 *
 */
struct block {
    size_t header;
};

size_t align_up(size_t size) {
    // we must ensure that the payloads are 16-byte aligned
    // so we need to round up to the nearest 16th byte 
    return (size + 15) & ~(size_t)15;
}

size_t block_size_for_payload(size_t payload_size) {
    (void)payload_size;
    return 0;
}

size_t block_get_size(const block_t *b) { (void)b; return 0; }
bool   block_is_free(const block_t *b)  { (void)b; return false; }
void  *block_payload(block_t *b)        { (void)b; return NULL; }

void block_set_size(block_t *b, size_t size) { (void)b; (void)size; }
void block_set_free(block_t *b, bool is_free) { (void)b; (void)is_free; }

block_t *block_next(block_t *b) { (void)b; return NULL; }
block_t *block_prev(block_t *b) { (void)b; return NULL; }

block_t *block_from_payload(void *payload) { (void)payload; return NULL; }
