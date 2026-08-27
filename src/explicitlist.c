#include "freelist.h"
#include "block.h"

#include <stddef.h>
#include <stdio.h>

typedef struct free_node {
    block_t *next;
    block_t *prev;
} free_node_t;

static free_node_t *node(block_t *b) {
    return (free_node_t *)block_payload(b);
}

static block_t *g_free_list;

void freelist_init(void) {
    g_free_list = NULL;
}

void freelist_insert(block_t *b) {
    block_t *old_head = g_free_list;
    node(b)->next = old_head;
    if (old_head) node(old_head)->prev = b;
    node(b)->prev = NULL;
    g_free_list = b;
}

void freelist_remove(block_t *b) {
    (void)b;
}

block_t *freelist_find_fit(size_t size) {
    (void)size;
    return NULL;
}

int freelist_check(void) {
    return 0;
}

void freelist_dump(void) {
    printf("(freelist_dump: not implemented)\n");
}
