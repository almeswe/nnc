#include "../nnc_test.hpp"

TEST(nnc_try_catch_tests, nest_1) {
    nnc_i32 counter = 0;
    TRY {
        ASSERT_TRUE(glob_exception_stack.depth == 1);
        counter++;
        ETRY;
    }
    CATCH(NNC_UNHANDLED) {
        ASSERT_TRUE(false && "wrong handler called");
    }
    CATCH(NNC_UNINPLEMENTED) {
        ASSERT_TRUE(glob_exception_stack.depth == 0);
        ASSERT_TRUE(false && "wrong handler called");
    }
    ASSERT_TRUE(counter == 1);
    counter = 0;
    TRY {
        ASSERT_TRUE(glob_exception_stack.depth == 1);
        counter++;
        THROW(NNC_UNINPLEMENTED);
        counter++;
        ETRY;
    }
    CATCH(NNC_UNHANDLED) {
        ASSERT_TRUE(false && "wrong handler called");
    }
    CATCH(NNC_UNINPLEMENTED) {
        ASSERT_TRUE(glob_exception_stack.depth == 0);
        counter--;
    }
    ASSERT_TRUE(counter == 0);
}