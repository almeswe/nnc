#include "../nnc_test.hpp"

TEST(nnc_buf_tests, push_n_fetch_1k) {
    nnc_arena_init(&glob_arena);
    int* intbuf = NULL;
    for (int i = 0; i < 1024; i++) {
        buf_add(intbuf, i);
    }
    ASSERT_TRUE(buf_len(intbuf) == 1024);
    for (int i = 0; i < 1024; i++) {
        ASSERT_TRUE(intbuf[i] == i);
    }
    buf_free(intbuf);
    nnc_arena_fini(&glob_arena);
}