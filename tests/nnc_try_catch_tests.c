#include "nnc_test.h"

TEST(nest_lvl_1, nnc_try_catch) {
    nnc_i32 counter = 0;
    TRY {
        assert(glob_exception_stack.depth == 1);
        counter++;
        ETRY;
    }
    CATCH(NNC_UNHANDLED) {
        assert(false && "wrong handler called");
    }
    CATCH(NNC_UNINPLEMENTED) {
        assert(glob_exception_stack.depth == 0);
        assert(false && "wrong handler called");
    }
    assert(counter == 1);
    counter = 0;
    TRY {
        assert(glob_exception_stack.depth == 1);
        counter++;
        THROW(NNC_UNINPLEMENTED);
        counter++;
        ETRY;
    }
    CATCH(NNC_UNHANDLED) {
        assert(false && "wrong handler called");
    }
    CATCH(NNC_UNINPLEMENTED) {
        assert(glob_exception_stack.depth == 0);
        counter--;
    }
    assert(counter == 0);
}

TEST(nest_lvl_2, nnc_try_catch) {
    nnc_i32 counter = 0;
    TRY {
        assert(glob_exception_stack.depth == 1);
        counter++;
        TRY {
            assert(glob_exception_stack.depth == 2);
            counter++;
            THROW(NNC_UNINPLEMENTED);
            ETRY;
        }
        CATCH (NNC_UNINPLEMENTED) {
            assert(glob_exception_stack.depth == 1);
            counter--;
            THROW(NNC_UNINPLEMENTED);
        }
        ETRY;
    }
    CATCH (NNC_UNINPLEMENTED) {
        assert(glob_exception_stack.depth == 0);
        counter--;
    }
    assert(counter == 0);
}

TEST(nest_lvl_3, nnc_try_catch) {
    nnc_i32 counter = 0;
    TRY {
        assert(glob_exception_stack.depth == 1);
        counter++;
        TRY {
            assert(glob_exception_stack.depth == 2);
            counter++;
            TRY {
                assert(glob_exception_stack.depth == 3);
                counter++;
                THROW(NNC_UNINPLEMENTED);
                ETRY;
            }
            CATCH (NNC_UNINPLEMENTED) {
                assert(glob_exception_stack.depth == 2);
                counter--;
                THROW(NNC_UNINPLEMENTED);
            }
            ETRY;
        }
        CATCH (NNC_UNINPLEMENTED) {
            assert(glob_exception_stack.depth == 1);
            counter--;
            THROW(NNC_UNINPLEMENTED);
        }
        ETRY;
    }
    CATCH (NNC_UNINPLEMENTED) {
        assert(glob_exception_stack.depth == 0);
        counter--;
    }
    assert(counter == 0);
}