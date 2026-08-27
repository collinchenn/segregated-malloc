#ifndef BLOCK_H
#define BLOCK_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define ALIGNMENT 16

typedef struct block block_t;

size_t align_up(size_t size);
size_t block_size_for_payload(size_t payload_size);

size_t block_get_size(const block_t *b);
size_t block_get_payload_size(const block_t *b);
bool   block_is_free(const block_t *b);
bool   block_is_consistent(const block_t *b);
void  *block_payload(block_t *b);

void block_set_size(block_t *b, size_t size);
void block_set_free(block_t *b, bool is_free);

block_t *block_next(block_t *b);
block_t *block_prev(block_t *b);
block_t *block_from_payload(void *payload);

#endif /* BLOCK_H */
