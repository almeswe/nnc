#ifndef _NNC_TEST_HPP
#define _NNC_TEST_HPP

extern "C" {
    #include "nnc_core.h"
}
#include <vector>
#include <fstream>
#include <iostream>
#include <gtest/gtest.h>

#include "nnc_parse_exported.hpp"
#include "nnc_ast_dump_exported.hpp"

nnc_lex lex = {0};
nnc_parser parser = {0};

nnc_int_literal* iliteral = NULL;
nnc_dbl_literal* fliteral = NULL;

static std::string contents;

inline FILE* nnc_create_test_file(std::string& contents) {
    return fmemopen((void*)(contents.c_str()), contents.length(), "r");
}

inline nnc_parser* nnc_create_parser(std::string& contents) {
    nnc_reset_canarie();
    nnc_arena_init(&glob_arena);
    nnc_deferred_stack_init(&glob_deferred_stack);
    parser = { 0 };
    FILE* fp = nnc_create_test_file(contents);
    nnc_parser_init_with_fp(&parser, fp);
    return &parser;
}

inline nnc_unary_expression* nnc_create_unary(std::string& contents) {
    auto parser = nnc_create_parser(contents);
    auto expr = nnc_parse_expr(parser);
    //nnc_dump_expr(DUMP_DATA(1, expr));
    return (nnc_unary_expression*)expr->exact;
}

inline void nnc_test_fini() {
    nnc_lex_keywords_map_fini();
    nnc_arena_fini(&glob_arena);
}

#endif