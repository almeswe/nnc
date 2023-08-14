#include "../nnc_test.hpp"

TEST(nnc_literal_tests, i32_b2_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("b10000000000000000000000000000000i32");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("b1111111111111111111111111111111i32");
        ASSERT_EQ(iliteral->base, 2);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I32);
        ASSERT_EQ(iliteral->exact.d, 2147483647);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("b0i32");
        ASSERT_EQ(iliteral->base, 2);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I32);
        ASSERT_EQ(iliteral->exact.d, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}

TEST(nnc_literal_tests, i32_b8_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("o20000000000i32");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("o17777777777i32");
        ASSERT_EQ(iliteral->base, 8);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I32);
        ASSERT_EQ(iliteral->exact.d, 2147483647);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("o0i32");
        ASSERT_EQ(iliteral->base, 8);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I32);
        ASSERT_EQ(iliteral->exact.d, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}

TEST(nnc_literal_tests, i32_b10_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("2147483648i32");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("2147483647i32");
        ASSERT_EQ(iliteral->base, 10);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I32);
        ASSERT_EQ(iliteral->exact.d, 2147483647);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("0i32");
        ASSERT_EQ(iliteral->base, 10);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I32);
        ASSERT_EQ(iliteral->exact.d, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}

TEST(nnc_literal_tests, i32_b16_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("x80000000i32");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("x7FFFFFFFi32");
        ASSERT_EQ(iliteral->base, 16);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I32);
        ASSERT_EQ(iliteral->exact.d, 2147483647);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("x0i32");
        ASSERT_EQ(iliteral->base, 16);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I32);
        ASSERT_EQ(iliteral->exact.d, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}