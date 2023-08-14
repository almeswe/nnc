#include "../nnc_test.hpp"

TEST(nnc_buf_tests, push_n_fetch_50k) {
    nnc_arena_init(&glob_arena);
    int* intbuf = NULL;
    for (int i = 0; i < 50000; i++) {
        buf_add(intbuf, i);
    }
    ASSERT_TRUE(buf_len(intbuf) == 50000);
    for (int i = 0; i < 50000; i++) {
        ASSERT_TRUE(intbuf[i] == i);
    }
    buf_free(intbuf);
    nnc_arena_fini(&glob_arena);
}