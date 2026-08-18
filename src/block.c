#include "block.h"

/*
 *
 *   struct block {
 *       size_t header;   // size in high bits, alloc flag in low bit
 *       // payload follows; footer sits at the end of the block
 *   };
 *
 */

#define HEADER_SIZE 8
#define FOOTER_SIZE 8

struct block {
    size_t header;
};

size_t align_up(size_t size) {
    // we must ensure that the payloads are 16-byte aligned
    // so we need to round up to the nearest 16th byte 
    return (size + 15) & ~(size_t)15;
}

size_t block_size_for_payload(size_t payload_size) {
    return HEADER_SIZE + align_up(payload_size) + FOOTER_SIZE;
}

size_t block_get_size(const block_t *b) {  
    return b->header & ~(size_t)0xF;
}

bool   block_is_free(const block_t *b)  {
    return (b->header & (size_t)0x1) == 0;
}

void  *block_payload(block_t *b)        {
    return (char*)b + HEADER_SIZE;
}

void block_set_size(block_t *b, size_t size) { 
    b->header = size | (b->header & (size_t)0xF);
}

void block_set_free(block_t *b, bool is_free) { 
    b->header = (size_t)(!is_free) | (b->header & ~(size_t)0x1);
}

block_t *block_next(block_t *b) { (void)b; return NULL; }
block_t *block_prev(block_t *b) { (void)b; return NULL; }

block_t *block_from_payload(void *payload) { (void)payload; return NULL; }
