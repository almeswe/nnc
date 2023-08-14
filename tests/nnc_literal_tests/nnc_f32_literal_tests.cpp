#include "../nnc_test.hpp"

TEST(nnc_literal_tests, f32_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_dbl_new("3.40282346638528859811704183484516925e+39F32");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        nnc_dbl_new("1.17549435082228750796873653722224569e-39F32");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        fliteral = nnc_dbl_new("3.40282346638528859811704183484516925e+38F32");
        ASSERT_EQ(fliteral->suffix, SUFFIX_F32);
        ASSERT_EQ(fliteral->exact, 3.40282346638528859811704183484516925e+38);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        fliteral = nnc_dbl_new("1.17549435082228750796873653722224568e-38F32");
        ASSERT_EQ(fliteral->suffix, SUFFIX_F32);
        ASSERT_EQ(fliteral->exact, 1.17549435082228750796873653722224568e-38);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}