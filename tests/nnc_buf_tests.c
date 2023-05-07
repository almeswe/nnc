#include "nnc_test.h"

TEST(basic, nncbuf) {
    nnc_arena_init(&glob_arena);
    int* intbuf = NULL;
    buf_add(intbuf, 5);
    assert(buf_len(intbuf) == 1);
    assert(intbuf[0] == 5);
    nnc_arena_fini(&glob_arena);
}

TEST(free, nncbuf) {
    nnc_arena_init(&glob_arena);
    int* intbuf = NULL;
    for (int i = 0; i < 32; i++) {
        buf_add(intbuf, i);
    }
    assert(buf_len(intbuf) == 32);
    buf_free(intbuf);
    assert(glob_arena.alloc_bytes == 0);
    nnc_arena_fini(&glob_arena);
}

TEST(add_n_get_1k, nncbuf) {
    nnc_arena_init(&glob_arena);
    int* intbuf = NULL;
    for (int i = 0; i < 1024; i++) {
        buf_add(intbuf, i);
    }
    assert(buf_len(intbuf) == 1024);
    for (int i = 0; i < 1024; i++) {
        assert(intbuf[i] == i);
    }
    TEST_PRINTF("cap: %lu\n", buf_cap(intbuf));
    buf_free(intbuf);
    nnc_arena_fini(&glob_arena);
}

TEST(add_n_get_50k, nncbuf) {
    nnc_arena_init(&glob_arena);
    int* intbuf = NULL;
    for (int i = 0; i < 50000; i++) {
        buf_add(intbuf, i);
    }
    assert(buf_len(intbuf) == 50000);
    for (int i = 0; i < 50000; i++) {
        assert(intbuf[i] == i);
    }
    TEST_PRINTF("cap: %lu\n", buf_cap(intbuf));
    buf_free(intbuf);
    nnc_arena_fini(&glob_arena);
}