#include "../nnc_test.hpp"

TEST(nnc_literal_tests, i64_b2_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("b1000000000000000000000000000000000000000000000000000000000000000i64");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("b0111111111111111111111111111111111111111111111111111111111111111i64");
        ASSERT_EQ(iliteral->base, 2);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I64);
        ASSERT_EQ(iliteral->exact.d, 9223372036854775807);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("b0i64");
        ASSERT_EQ(iliteral->base, 2);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I64);
        ASSERT_EQ(iliteral->exact.d, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}

TEST(nnc_literal_tests, i64_b8_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("o1000000000000000000000i64");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("o777777777777777777777i64");
        ASSERT_EQ(iliteral->base, 8);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I64);
        ASSERT_EQ(iliteral->exact.d, 9223372036854775807);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("o0i64");
        ASSERT_EQ(iliteral->base, 8);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I64);
        ASSERT_EQ(iliteral->exact.d, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}

TEST(nnc_literal_tests, i64_b10_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("9223372036854775808i64");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("9223372036854775807i64");
        ASSERT_EQ(iliteral->base, 10);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I64);
        ASSERT_EQ(iliteral->exact.d, 9223372036854775807);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("0i64");
        ASSERT_EQ(iliteral->base, 10);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I64);
        ASSERT_EQ(iliteral->exact.d, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}

TEST(nnc_literal_tests, i64_b16_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("x8000000000000000i64");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("x7FFFFFFFFFFFFFFFi64");
        ASSERT_EQ(iliteral->base, 16);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I64);
        ASSERT_EQ(iliteral->exact.d, 9223372036854775807);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("x0i64");
        ASSERT_EQ(iliteral->base, 16);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I64);
        ASSERT_EQ(iliteral->exact.d, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}