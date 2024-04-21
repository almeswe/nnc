#ifndef __NNC_CMD_ARGUMENTS_H__
#define __NNC_CMD_ARGUMENTS_H__

#define _GNU_SOURCE

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "nnc_buf.h"
#include "nnc_format.h"
#include "nnc_arena.h"

#define EXT_OBJ         ".o"
#define EXT_SHARED_LIB  ".so"
#define EXT_STATIC_LIB  ".a"
#define EXT_NNC_SOURCE1 ".nnc"
#define EXT_NNC_SOURCE2 ".source"

#define NNC_ARGV_DEF_OUT "out"
#define NNC_ARGV_DEF_FILE stderr

#define NNC_OPT_OUTPUT    "o"
#define NNC_OPT_COMPILE   "c"
#define NNC_OPT_GEN_DEBUG "g"

#define NNC_OPT_HELP_LONG            "help"
#define NNC_OPT_VERBOSE_LONG         "verbose"
#define NNC_OPT_OUTPUT_LONG          "output"
#define NNC_OPT_GEN_DEBUG_LONG       "gen-debug"
#define NNC_OPT_COMPILE_LONG         "compile"

#define NNC_OPT_DUMP_DEST_LONG       "dump-dest"
#define NNC_OPT_DUMP_IR_LONG         "dump-ir"
#define NNC_OPT_DUMP_AST_LONG        "dump-ast"
#define NNC_OPT_DUMP_WITH_COLOR_LONG "dump-with-color"

typedef struct _nnc_argv {
    const char* output;
    vector(const char*) sources;
    vector(const char*) objects;
    vector(const char*) static_libs;
    vector(const char*) shared_libs;
    nnc_i32 help;
    nnc_i32 verbose;
    nnc_i32 gen_debug;
    nnc_i32 compile;
    nnc_i32 dump_ir;
    nnc_i32 dump_ast;
    nnc_i32 dump_with_color;
    const char* dump_ir_pat;
    const char* dump_ast_pat;
    FILE* dump_dest;
} nnc_argv;

extern nnc_argv glob_nnc_argv;

const nnc_argv* nnc_parse_argv(
    nnc_i32 argc,
    char* const* argv
);

#endif