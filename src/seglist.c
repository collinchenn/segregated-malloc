#include "freelist.h"
#include "seglist.h"
#include <stdio.h>

typedef struct free_node {
    block_t *next;
    block_t *prev;
} free_node_t;

static free_node_t *node(block_t *b) {
    return (free_node_t *)block_payload(b);
}

/* One list head per size class. Each head points at the first free
 * block in that bucket (intrusive doubly-linked list). */
static block_t *g_buckets[NUM_SIZE_CLASSES];

void freelist_init(void) {
    for (int i = 0; i < NUM_SIZE_CLASSES; i++)
        g_buckets[i] = NULL;
}

int seglist_index_for_size(size_t size) {
    size_t threshold = 32;
    for (int i = 0; i < NUM_SIZE_CLASSES - 1; i++) {
        if (size <= threshold)
            return i;
        threshold *= 2;
    }
    return NUM_SIZE_CLASSES - 1;
}

void freelist_insert(block_t *b) {
    int i = seglist_index_for_size(block_get_size(b));
    block_t *head = g_buckets[i];

    node(b)->next = head;
    node(b)->prev = NULL;

    if (head != NULL)
        node(head)->prev = b;

    g_buckets[i] = b;
}

void freelist_remove(block_t *b) {
    int i = seglist_index_for_size(block_get_size(b));
    block_t *prev = node(b)->prev;
    block_t *next = node(b)->next;

    if (prev == NULL) {
        g_buckets[i] = next;
    } else {
        node(prev)->next = next;
    }

    if (next != NULL)
        node(next)->prev = prev;
}

block_t *freelist_find_fit(size_t size) {
    for (int i = seglist_index_for_size(size); i < NUM_SIZE_CLASSES; i++) {
        block_t *head = g_buckets[i];

        while (head != NULL) {
            if (block_get_size(head) >= size)
                return head;

            head = node(head)->next;
        }
    }

    return NULL;
}

int freelist_check(void) {
    for (int i = 0; i < NUM_SIZE_CLASSES; i++) {
        block_t *curr = g_buckets[i];

        while (curr) {
            if (seglist_index_for_size(block_get_size(curr)) != i) return -1;
            if (!block_is_free(curr)) return -1;

            curr = node(curr)->next;
        }
    }

    return 0;
}

void freelist_dump(void) {
    printf("(freelist_dump: not implemented)\n");
}
