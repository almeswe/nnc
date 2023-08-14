#include "../nnc_test.hpp"

TEST(nnc_literal_tests, u8_b2_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("b100000000u8");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("b11111111u8");
        ASSERT_EQ(iliteral->base, 2);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U8);
        ASSERT_EQ(iliteral->exact.u, 255);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("b0u8");
        ASSERT_EQ(iliteral->base, 2);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U8);
        ASSERT_EQ(iliteral->exact.u, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}

TEST(nnc_literal_tests, u8_b8_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("o400u8");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("o377u8");
        ASSERT_EQ(iliteral->base, 8);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U8);
        ASSERT_EQ(iliteral->exact.u, 255);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("o0u8");
        ASSERT_EQ(iliteral->base, 8);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U8);
        ASSERT_EQ(iliteral->exact.u, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}

TEST(nnc_literal_tests, u8_b10_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("256u8");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("255u8");
        ASSERT_EQ(iliteral->base, 10);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U8);
        ASSERT_EQ(iliteral->exact.u, 255);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("0u8");
        ASSERT_EQ(iliteral->base, 10);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U8);
        ASSERT_EQ(iliteral->exact.u, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}

TEST(nnc_literal_tests, u8_b16_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("x100u8");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("xffu8");
        ASSERT_EQ(iliteral->base, 16);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U8);
        ASSERT_EQ(iliteral->exact.u, 255);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("x0u8");
        ASSERT_EQ(iliteral->base, 16);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U8);
        ASSERT_EQ(iliteral->exact.u, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}