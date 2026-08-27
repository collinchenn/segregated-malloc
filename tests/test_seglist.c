#include "seglist.h"
#include "freelist.h"
#include "block.h"

#include <stdalign.h>
#include <stddef.h>
#include <stdio.h>

static int g_total = 0;
static int g_total_fail = 0;

static const char *s_name = "";
static int s_checks = 0;
static int s_fails = 0;

#define SECTION(name)                                                      \
    do {                                                                   \
        s_name = (name);                                                   \
        s_checks = 0;                                                      \
        s_fails = 0;                                                       \
    } while (0)

#define CHECK(cond)                                                        \
    do {                                                                   \
        s_checks++;                                                        \
        g_total++;                                                         \
        if (!(cond)) {                                                     \
            s_fails++;                                                     \
            g_total_fail++;                                                \
            printf("    FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);     \
        }                                                                  \
    } while (0)

#define SECTION_END()                                                      \
    do {                                                                   \
        printf("  [%s] %-28s %d/%d passed\n",                              \
               s_fails == 0 ? "PASS" : "FAIL", s_name,                     \
               s_checks - s_fails, s_checks);                              \
    } while (0)

alignas(16) static unsigned char g_arena[1 << 16];

static block_t *make_free_block(size_t offset, size_t size) {
    block_t *b = (block_t *)(g_arena + offset);
    block_set_size(b, size);
    block_set_free(b, true);
    return b;
}

static void test_index_for_size(void) {
    SECTION("index_for_size");
    CHECK(seglist_index_for_size(16) == 0);
    CHECK(seglist_index_for_size(32) == 0);
    CHECK(seglist_index_for_size(33) == 1);
    CHECK(seglist_index_for_size(64) == 1);
    CHECK(seglist_index_for_size(65) == 2);
    CHECK(seglist_index_for_size(128) == 2);
    CHECK(seglist_index_for_size(129) == 3);
    CHECK(seglist_index_for_size(256) == 3);
    CHECK(seglist_index_for_size(257) == 4);
    CHECK(seglist_index_for_size(512) == 4);
    CHECK(seglist_index_for_size(1024) == 5);
    CHECK(seglist_index_for_size(262144) == 13);
    CHECK(seglist_index_for_size(262145) == 14);
    CHECK(seglist_index_for_size(524288) == 14);
    CHECK(seglist_index_for_size(524289) == 15);
    CHECK(seglist_index_for_size(1u << 24) == 15);
    SECTION_END();
}

static void test_insert_find_basic(void) {
    SECTION("insert + find basic");
    freelist_init();
    block_t *b = make_free_block(8, 64);
    freelist_insert(b);

    CHECK(freelist_find_fit(64) == b);
    CHECK(freelist_find_fit(48) == b);
    CHECK(freelist_find_fit(32) == b);
    CHECK(freelist_find_fit(128) == NULL);
    SECTION_END();
}

static void test_find_does_not_remove(void) {
    SECTION("find does not remove");
    freelist_init();
    block_t *b = make_free_block(8, 64);
    freelist_insert(b);

    CHECK(freelist_find_fit(64) == b);
    CHECK(freelist_find_fit(64) == b);
    freelist_remove(b);
    CHECK(freelist_find_fit(64) == NULL);
    SECTION_END();
}

static void test_find_checks_size_within_bucket(void) {
    SECTION("size check within bucket");
    freelist_init();
    block_t *b = make_free_block(8, 48);
    freelist_insert(b);

    CHECK(freelist_find_fit(48) == b);
    CHECK(freelist_find_fit(32) == b);
    CHECK(freelist_find_fit(64) == NULL);
    SECTION_END();
}

static void test_find_searches_larger_buckets(void) {
    SECTION("search larger buckets");
    freelist_init();
    block_t *big = make_free_block(8, 128);
    freelist_insert(big);

    CHECK(freelist_find_fit(48) == big);
    CHECK(freelist_find_fit(128) == big);
    CHECK(freelist_find_fit(256) == NULL);
    SECTION_END();
}

static void test_remove_middle(void) {
    SECTION("remove from middle");
    freelist_init();
    block_t *a = make_free_block(8, 64);
    block_t *b = make_free_block(1024 + 8, 64);
    block_t *c = make_free_block(2048 + 8, 64);
    freelist_insert(a);
    freelist_insert(b);
    freelist_insert(c);

    freelist_remove(b);

    block_t *found[4] = {0};
    int n = 0;
    block_t *f;
    while ((f = freelist_find_fit(64)) != NULL && n < 4) {
        found[n++] = f;
        freelist_remove(f);
    }

    CHECK(n == 2);
    bool saw_a = false, saw_c = false, saw_b = false;
    for (int i = 0; i < n; i++) {
        if (found[i] == a) saw_a = true;
        if (found[i] == b) saw_b = true;
        if (found[i] == c) saw_c = true;
    }
    CHECK(saw_a);
    CHECK(saw_c);
    CHECK(!saw_b);
    CHECK(freelist_find_fit(64) == NULL);
    SECTION_END();
}

static void test_reinsert_after_drain(void) {
    SECTION("reinsert after drain");
    freelist_init();
    block_t *a = make_free_block(8, 64);
    freelist_insert(a);
    freelist_remove(a);
    CHECK(freelist_find_fit(64) == NULL);

    freelist_insert(a);
    CHECK(freelist_find_fit(64) == a);
    SECTION_END();
}

int main(void) {
    printf("running seglist tests\n");
    test_index_for_size();
    test_insert_find_basic();
    test_find_does_not_remove();
    test_find_checks_size_within_bucket();
    test_find_searches_larger_buckets();
    test_remove_middle();
    test_reinsert_after_drain();

    printf("\n");
    if (g_total_fail == 0)
        printf("ALL PASSED: %d/%d checks\n", g_total, g_total);
    else
        printf("FAILED: %d/%d checks passed, %d failed\n",
               g_total - g_total_fail, g_total, g_total_fail);

    return g_total_fail == 0 ? 0 : 1;
}
