#include "../nnc_test.hpp"

TEST(nnc_literal_tests, u16_b2_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("b10000000000000000u16");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("b1111111111111111u16");
        ASSERT_EQ(iliteral->base, 2);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U16);
        ASSERT_EQ(iliteral->exact.u, 65535);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("b0u16");
        ASSERT_EQ(iliteral->base, 2);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U16);
        ASSERT_EQ(iliteral->exact.u, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}

TEST(nnc_literal_tests, u16_b8_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("o200000u16");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("o177777u16");
        ASSERT_EQ(iliteral->base, 8);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U16);
        ASSERT_EQ(iliteral->exact.u, 65535);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("o0u16");
        ASSERT_EQ(iliteral->base, 8);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U16);
        ASSERT_EQ(iliteral->exact.u, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}

TEST(nnc_literal_tests, u16_b10_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("65536u16");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("65535u16");
        ASSERT_EQ(iliteral->base, 10);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U16);
        ASSERT_EQ(iliteral->exact.u, 65535);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("0u16");
        ASSERT_EQ(iliteral->base, 10);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U16);
        ASSERT_EQ(iliteral->exact.u, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}

TEST(nnc_literal_tests, u16_b16_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("x10000u16");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("xffffu16");
        ASSERT_EQ(iliteral->base, 16);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U16);
        ASSERT_EQ(iliteral->exact.u, 65535);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("x0u16");
        ASSERT_EQ(iliteral->base, 16);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U16);
        ASSERT_EQ(iliteral->exact.u, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}