#include "allocator.h"
#include "block.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

#define ALIGNED16(p) (((uintptr_t)(p) & 15u) == 0)

static void test_init(void) {
    SECTION("init");
    CHECK(seg_malloc_init() == 0);
    CHECK(seg_heap_check() == 0);
    SECTION_END();
}

static void test_basic_malloc_free(void) {
    SECTION("basic malloc/free");
    seg_malloc_init();

    void *p = seg_malloc(100);
    CHECK(p != NULL);
    CHECK(ALIGNED16(p));
    CHECK(seg_heap_check() == 0);

    memset(p, 0xAB, 100);
    CHECK(seg_heap_check() == 0);

    seg_free(p);
    CHECK(seg_heap_check() == 0);
    SECTION_END();
}

static void test_alignment(void) {
    SECTION("alignment");
    seg_malloc_init();
    size_t sizes[] = {1, 8, 16, 17, 100, 4096};
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        void *p = seg_malloc(sizes[i]);
        CHECK(p != NULL);
        CHECK(ALIGNED16(p));
    }
    CHECK(seg_heap_check() == 0);
    SECTION_END();
}

static void test_reuse_after_free(void) {
    SECTION("reuse after free");
    seg_malloc_init();

    void *p1 = seg_malloc(100);
    seg_free(p1);
    void *p2 = seg_malloc(100);
    CHECK(p2 == p1);
    CHECK(seg_heap_check() == 0);
    SECTION_END();
}

static void test_split(void) {
    SECTION("split");
    seg_malloc_init();

    void *big = seg_malloc(1000);
    seg_free(big);

    void *a = seg_malloc(16);
    void *b = seg_malloc(16);
    CHECK(a != NULL && b != NULL);
    CHECK(a == big);
    CHECK((char *)b - (char *)a == (long)block_size_for_payload(16));
    CHECK(seg_heap_check() == 0);
    SECTION_END();
}

static void test_coalesce(void) {
    SECTION("coalesce both sides");
    seg_malloc_init();

    void *a = seg_malloc(64);
    void *b = seg_malloc(64);
    void *c = seg_malloc(64);
    CHECK((char *)b - (char *)a == (long)block_size_for_payload(64));
    CHECK((char *)c - (char *)b == (long)block_size_for_payload(64));

    seg_free(a);
    seg_free(c);
    CHECK(seg_heap_check() == 0);

    seg_free(b);
    CHECK(seg_heap_check() == 0);

    void *big = seg_malloc(180);
    CHECK(big == a);
    CHECK(seg_heap_check() == 0);
    SECTION_END();
}

static void test_calloc(void) {
    SECTION("calloc");
    seg_malloc_init();

    unsigned char *p = seg_calloc(4, 25);
    CHECK(p != NULL);
    bool all_zero = p != NULL;
    for (int i = 0; p != NULL && i < 100; i++)
        if (p[i] != 0) all_zero = false;
    CHECK(all_zero);
    CHECK(seg_heap_check() == 0);

    CHECK(seg_calloc((size_t)-1, 2) == NULL);
    SECTION_END();
}

static void test_realloc(void) {
    SECTION("realloc");
    seg_malloc_init();

    CHECK(seg_realloc(NULL, 100) != NULL);

    char *p = seg_malloc(100);
    for (int i = 0; i < 100; i++) p[i] = (char)i;

    char *q = seg_realloc(p, 200);
    CHECK(q != NULL);
    bool preserved = q != NULL;
    for (int i = 0; q != NULL && i < 100; i++)
        if (q[i] != (char)i) preserved = false;
    CHECK(preserved);
    CHECK(seg_heap_check() == 0);

    char *r = seg_malloc(200);
    char *s = seg_realloc(r, 50);
    CHECK(s == r);
    CHECK(seg_heap_check() == 0);

    CHECK(seg_realloc(q, 0) == NULL);
    CHECK(seg_heap_check() == 0);
    SECTION_END();
}

static void test_stress(void) {
    SECTION("stress");
    seg_malloc_init();

    enum { N = 64 };
    void *ptrs[N];
    for (int i = 0; i < N; i++) {
        ptrs[i] = seg_malloc((size_t)(i * 8 + 1));
        CHECK(ptrs[i] != NULL);
        if (ptrs[i] != NULL)
            memset(ptrs[i], i & 0xFF, (size_t)(i * 8 + 1));
    }
    CHECK(seg_heap_check() == 0);

    for (int i = 0; i < N; i += 2)
        seg_free(ptrs[i]);
    CHECK(seg_heap_check() == 0);

    for (int i = 1; i < N; i += 2)
        seg_free(ptrs[i]);
    CHECK(seg_heap_check() == 0);
    SECTION_END();
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("running allocator tests\n");
    test_init();
    test_basic_malloc_free();
    test_alignment();
    test_reuse_after_free();
    test_split();
    test_coalesce();
    test_calloc();
    test_realloc();
    test_stress();

    printf("\n");
    if (g_total_fail == 0)
        printf("ALL PASSED: %d/%d checks\n", g_total, g_total);
    else
        printf("FAILED: %d/%d checks passed, %d failed\n",
               g_total - g_total_fail, g_total, g_total_fail);

    return g_total_fail == 0 ? 0 : 1;
}
