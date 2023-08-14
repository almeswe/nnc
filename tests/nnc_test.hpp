#ifndef _NNC_TEST_HPP
#define _NNC_TEST_HPP

extern "C" {
    #include "nnc_core.h"
}
#include <vector>
#include <fstream>
#include <iostream>
#include <gtest/gtest.h>

nnc_lex lex = {0};
nnc_parser parser = {0};

nnc_int_literal* iliteral = NULL;
nnc_dbl_literal* fliteral = NULL;

static std::string contents;

inline FILE* nnc_create_test_file(std::string& contents) {
    return fmemopen((void*)(contents.c_str()), contents.length(), "r");
}

#endif