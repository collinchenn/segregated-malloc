#include "allocator.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_init(void) {

}

static void test_basic_malloc_free(void) {

}

static void test_reuse_after_free(void) {

}

int main(void) {
    test_init();
    test_basic_malloc_free();
    test_reuse_after_free();
    printf("all tests passed\n");
    return 0;
}
