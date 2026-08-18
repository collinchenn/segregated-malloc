#include "block.h"

#include <stdalign.h>
#include <stdint.h>
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
        printf("  [%s] %-24s %d/%d passed\n",                              \
               s_fails == 0 ? "PASS" : "FAIL", s_name,                     \
               s_checks - s_fails, s_checks);                              \
    } while (0)

static void test_align_up(void) {
    SECTION("align_up");
    CHECK(align_up(0) == 0);
    CHECK(align_up(1) == 16);
    CHECK(align_up(8) == 16);
    CHECK(align_up(15) == 16);
    CHECK(align_up(16) == 16);
    CHECK(align_up(17) == 32);
    CHECK(align_up(31) == 32);
    CHECK(align_up(32) == 32);
    CHECK(align_up(33) == 48);
    CHECK(align_up(100) == 112);
    CHECK(align_up(4096) == 4096);
    SECTION_END();
}

static void test_block_size_for_payload(void) {
    SECTION("block_size_for_payload");
    CHECK(block_size_for_payload(0) == 16);
    CHECK(block_size_for_payload(1) == 32);
    CHECK(block_size_for_payload(16) == 32);
    CHECK(block_size_for_payload(17) == 48);
    CHECK(block_size_for_payload(32) == 48);
    CHECK(block_size_for_payload(33) == 64);
    SECTION_END();
}

static void test_header_get_set(void) {
    SECTION("header get/set");
    alignas(16) static unsigned char buf[256];
    block_t *b = (block_t *)(buf + 8);

    block_set_size(b, 32);
    CHECK(block_get_size(b) == 32);

    block_set_free(b, true);
    CHECK(block_is_free(b) == true);

    block_set_free(b, false);
    CHECK(block_is_free(b) == false);
    CHECK(block_get_size(b) == 32);

    block_set_size(b, 48);
    CHECK(block_get_size(b) == 48);
    CHECK(block_is_free(b) == false);

    block_set_free(b, true);
    CHECK(block_get_size(b) == 48);
    CHECK(block_is_free(b) == true);
    SECTION_END();
}

static void test_payload_roundtrip(void) {
    SECTION("payload roundtrip");
    alignas(16) static unsigned char buf[256];
    block_t *b = (block_t *)(buf + 8);
    block_set_size(b, 64);

    CHECK(block_payload(b) == (void *)((char *)b + 8));
    CHECK(block_from_payload(block_payload(b)) == b);
    CHECK(((uintptr_t)block_payload(b) % 16) == 0);
    SECTION_END();
}

static void test_navigation(void) {
    SECTION("navigation");
    alignas(16) static unsigned char buf[256];
    block_t *a = (block_t *)(buf + 8);
    block_t *b = (block_t *)(buf + 8 + 32);
    block_t *c = (block_t *)(buf + 8 + 32 + 48);

    block_set_size(a, 32);
    block_set_free(a, false);
    block_set_size(b, 48);
    block_set_free(b, true);
    block_set_size(c, 32);
    block_set_free(c, false);

    CHECK(block_next(a) == b);
    CHECK(block_next(b) == c);

    CHECK(block_prev(b) == a);
    CHECK(block_prev(c) == b);

    CHECK(block_next(block_prev(b)) == b);
    CHECK(block_prev(block_next(a)) == a);
    SECTION_END();
}

int main(void) {
    printf("running block tests\n");
    test_align_up();
    test_block_size_for_payload();
    test_header_get_set();
    test_payload_roundtrip();
    test_navigation();

    printf("\n");
    if (g_total_fail == 0)
        printf("ALL PASSED: %d/%d checks\n", g_total, g_total);
    else
        printf("FAILED: %d/%d checks passed, %d failed\n",
               g_total - g_total_fail, g_total, g_total_fail);

    return g_total_fail == 0 ? 0 : 1;
}
