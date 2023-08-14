#include "../nnc_test.hpp"

TEST(nnc_literal_tests, i16_b2_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("b1000000000000000i16");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("b0111111111111111i16");
        ASSERT_EQ(iliteral->base, 2);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I16);
        ASSERT_EQ(iliteral->exact.d, 32767);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("b0i16");
        ASSERT_EQ(iliteral->base, 2);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I16);
        ASSERT_EQ(iliteral->exact.d, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}

TEST(nnc_literal_tests, i16_b8_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("o100000i16");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("o77777i16");
        ASSERT_EQ(iliteral->base, 8);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I16);
        ASSERT_EQ(iliteral->exact.d, 32767);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("o0i16");
        ASSERT_EQ(iliteral->base, 8);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I16);
        ASSERT_EQ(iliteral->exact.d, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}

TEST(nnc_literal_tests, i16_b10_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("32768i16");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("32767i16");
        ASSERT_EQ(iliteral->base, 10);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I16);
        ASSERT_EQ(iliteral->exact.d, 32767);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("0i16");
        ASSERT_EQ(iliteral->base, 10);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I16);
        ASSERT_EQ(iliteral->exact.d, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}

TEST(nnc_literal_tests, i16_b16_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("x8000i16");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("x7FFFi16");
        ASSERT_EQ(iliteral->base, 16);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I16);
        ASSERT_EQ(iliteral->exact.d, 32767);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("x0i16");
        ASSERT_EQ(iliteral->base, 16);
        ASSERT_TRUE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_I16);
        ASSERT_EQ(iliteral->exact.d, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}