#include "../nnc_test.hpp"

TEST(nnc_buf_tests, free) {
    nnc_arena_init(&glob_arena);
    int* intbuf = NULL;
    for (int i = 0; i < 32; i++) {
        buf_add(intbuf, i);
    }
    ASSERT_TRUE(buf_len(intbuf) == 32);
    buf_free(intbuf);
    ASSERT_TRUE(glob_arena.alloc_bytes == 0);
    nnc_arena_fini(&glob_arena);
}