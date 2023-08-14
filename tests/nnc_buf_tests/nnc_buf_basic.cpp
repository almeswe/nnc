#include "../nnc_test.hpp"

TEST(nnc_buf_tests, basic) {
    nnc_arena_init(&glob_arena);
    int* intbuf = NULL;
    buf_add(intbuf, 5);
    ASSERT_TRUE(buf_len(intbuf) == 1);
    ASSERT_TRUE(intbuf[0] == 5);
    nnc_arena_fini(&glob_arena);
}