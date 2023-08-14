#include "../nnc_test.hpp"

TEST(nnc_try_catch_tests, nest_3) {
    nnc_i32 counter = 0;
    TRY {
        ASSERT_TRUE(glob_exception_stack.depth == 1);
        counter++;
        TRY {
            ASSERT_TRUE(glob_exception_stack.depth == 2);
            counter++;
            TRY {
                ASSERT_TRUE(glob_exception_stack.depth == 3);
                counter++;
                THROW(NNC_UNINPLEMENTED);
                ETRY;
            }
            CATCH (NNC_UNINPLEMENTED) {
                ASSERT_TRUE(glob_exception_stack.depth == 2);
                counter--;
                THROW(NNC_UNINPLEMENTED);
            }
            ETRY;
        }
        CATCH (NNC_UNINPLEMENTED) {
            ASSERT_TRUE(glob_exception_stack.depth == 1);
            counter--;
            THROW(NNC_UNINPLEMENTED);
        }
        ETRY;
    }
    CATCH (NNC_UNINPLEMENTED) {
        ASSERT_TRUE(glob_exception_stack.depth == 0);
        counter--;
    }
    ASSERT_TRUE(counter == 0);
}