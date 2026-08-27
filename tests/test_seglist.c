#include "seglist.h"

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

int main(void) {
    printf("running seglist tests\n");
    test_index_for_size();

    printf("\n");
    if (g_total_fail == 0)
        printf("ALL PASSED: %d/%d checks\n", g_total, g_total);
    else
        printf("FAILED: %d/%d checks passed, %d failed\n",
               g_total - g_total_fail, g_total, g_total_fail);

    return g_total_fail == 0 ? 0 : 1;
}
