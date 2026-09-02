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

#define MIN_BLOCK      (2 * ALIGNMENT)
#define SMALL_MAX      512
#define EXACT_COUNT    (((SMALL_MAX - MIN_BLOCK) / ALIGNMENT) + 1)
#define FIRST_LOG_EXP  9

/* Exact-size classes (16B apart) up to SMALL_MAX, then power-of-two
 * (log-spaced) classes for larger blocks, with the last as a catch-all. */
static block_t *g_buckets[NUM_SIZE_CLASSES];

static int highest_bit(size_t x) {
    return 63 - __builtin_clzll((unsigned long long)x);
}

void freelist_init(void) {
    for (int i = 0; i < NUM_SIZE_CLASSES; i++)
        g_buckets[i] = NULL;
}

int seglist_index_for_size(size_t size) {
    if (size <= MIN_BLOCK) return 0;
    if (size <= SMALL_MAX)
        return (int)((size - MIN_BLOCK) / ALIGNMENT);

    size_t m = size - 1;
    int e = highest_bit(m);
    int sub = (int)((m >> (e - 2)) & 0x3);
    int idx = EXACT_COUNT + (e - FIRST_LOG_EXP) * 4 + sub;
    return idx < NUM_SIZE_CLASSES ? idx : NUM_SIZE_CLASSES - 1;
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
    int start = seglist_index_for_size(size);
    for (int j = start; j < NUM_SIZE_CLASSES; j++) {
        block_t *head = g_buckets[j];

        if (j == start) {
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
