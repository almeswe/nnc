#include "../nnc_test.hpp"

TEST(nnc_literal_tests, i8_b2_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("b10000000i8");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("b01111111i8");
        ASSERT_EQ(iliteral->base, 2);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I8);
        ASSERT_EQ(iliteral->exact.d, 127);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("b00000i8");
        ASSERT_EQ(iliteral->base, 2);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I8);
        ASSERT_EQ(iliteral->exact.d, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}

TEST(nnc_literal_tests, i8_b8_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("o200i8");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("o177i8");
        ASSERT_EQ(iliteral->base, 8);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I8);
        ASSERT_EQ(iliteral->exact.d, 127);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("o0i8");
        ASSERT_EQ(iliteral->base, 8);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I8);
        ASSERT_EQ(iliteral->exact.d, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}

TEST(nnc_literal_tests, i8_b10_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("128i8");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("127i8");
        ASSERT_EQ(iliteral->base, 10);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I8);
        ASSERT_EQ(iliteral->exact.d, 127);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("0i8");
        ASSERT_EQ(iliteral->base, 10);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I8);
        ASSERT_EQ(iliteral->exact.d, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}

TEST(nnc_literal_tests, i8_b16_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("x80i8");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("x7fi8");
        ASSERT_EQ(iliteral->base, 16);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I8);
        ASSERT_EQ(iliteral->exact.d, 127);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("x00i8");
        ASSERT_EQ(iliteral->base, 16);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I8);
        ASSERT_EQ(iliteral->exact.d, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}