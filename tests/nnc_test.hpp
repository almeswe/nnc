#ifndef _NNC_TEST_H
#define _NNC_TEST_H

extern "C" {
    #include "nnc_core.h"
}
#include <vector>
#include <fstream>
#include <iostream>
#include <gtest/gtest.h>

static std::string contents;

inline FILE* nnc_create_test_file(std::string& contents) {
    return fmemopen((void*)(contents.c_str()), contents.length(), "r");
}

#endif