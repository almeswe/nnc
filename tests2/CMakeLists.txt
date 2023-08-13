CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
PROJECT(nnc_run_tests)

SET(CMAKE_CXX_STANDARD 14)
SET(INSTALL_GTEST OFF)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../src)
INCLUDE_DIRECTORIES(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

ADD_SUBDIRECTORY(googletest)

ADD_LIBRARY(
    nnc_src SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/nnc_buf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/nnc_map.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/nnc_ast.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/nnc_lex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/nnc_ctx.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/nnc_type.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/nnc_parse.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/nnc_arena.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/nnc_error.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/nnc_format.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/nnc_resolve.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/nnc_literal.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/nnc_symtable.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/nnc_ast_dump.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/nnc_typecheck.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/nnc_try_catch.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/nnc_deferred_stack.c
)

ADD_EXECUTABLE(${PROJECT_NAME} nnc_test_main.cpp)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} gtest gtest_main)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} nnc_src)

SET_TARGET_PROPERTIES(nnc_src PROPERTIES LINKER_LANGUAGE C)