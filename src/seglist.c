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

#define MIN_BLOCK  (2 * ALIGNMENT)
#define EXACT_MAX  (MIN_BLOCK + ALIGNMENT * (NUM_SIZE_CLASSES - 2))

/* One list head per size class: exact-size classes (16B apart) up to
 * EXACT_MAX, then a single catch-all bin for larger blocks. */
static block_t *g_buckets[NUM_SIZE_CLASSES];

void freelist_init(void) {
    for (int i = 0; i < NUM_SIZE_CLASSES; i++)
        g_buckets[i] = NULL;
}

int seglist_index_for_size(size_t size) {
    if (size <= MIN_BLOCK) return 0;
    if (size > EXACT_MAX) return NUM_SIZE_CLASSES - 1;
    return (int)((size - MIN_BLOCK) / ALIGNMENT);
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

    if (prev == NULL)
        g_buckets[i] = next;
    else
        node(prev)->next = next;

    if (next != NULL)
        node(next)->prev = prev;
}

block_t *freelist_find_fit(size_t size) {
    for (int j = seglist_index_for_size(size); j < NUM_SIZE_CLASSES; j++) {
        block_t *head = g_buckets[j];

        // Only search if we're in the catch-all category
        // where allocating without checking can be wasteful
        if (j == NUM_SIZE_CLASSES - 1) {
            while (head != NULL) {
                if (block_get_size(head) >= size)
                    return head;
                head = node(head)->next;
            }
        } else if (head != NULL) {
            return head;
        }
    }

    return NULL;
}

int freelist_check(void) {
    for (int j = 0; j < NUM_SIZE_CLASSES; j++) {
        block_t *curr = g_buckets[j];
        while (curr != NULL) {
            if (!block_is_free(curr)) return -1;
            if (seglist_index_for_size(block_get_size(curr)) != j) return -1;
            curr = node(curr)->next;
        }
    }

    return 0;
}

void freelist_dump(void) {
    printf("(freelist_dump: not implemented)\n");
}
