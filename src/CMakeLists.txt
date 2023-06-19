cmake_minimum_required(VERSION 3.10)
project(nnc)
if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall)
endif()
add_executable(
    nnc 
    nnc_main.c
    nnc_parse.c
    nnc_ast.c
    nnc_ast_dump.c
    nnc_lex.c
    nnc_map.c
    nnc_error.c
    nnc_ctx.c
    nnc_try_catch.c
    nnc_arena.c
    nnc_format.c
    nnc_buf.c
    nnc_literal.c
    nnc_type.c
)