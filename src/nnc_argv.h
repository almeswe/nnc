#ifndef __NNC_ARGV_H__
#define __NNC_ARGV_H__

#include <sys/time.h>

#include "nnc_buf.h"
#include "nnc_format.h"

#define EXT_OBJ         ".o"
#define EXT_SHARED_LIB  ".so"
#define EXT_STATIC_LIB  ".a"
#define EXT_NNC_SOURCE1 ".nnc"
#define EXT_NNC_SOURCE2 ".source"

#define O_NO_ARG  0
#define O_ONE_ARG 1
#define O_OUT_DEF "out"
#define O_HELP_STR \
    "nnc:   Not Named x86_64 Compiler (see more: https://github.com/almeswe/nnc)\n"                             \
    "usage: ./nnc [options] [sources] [objects] [libs] \n\n"                                                    \
    "sources:                  `.nnc` or `.source` files\n"                                                     \
    "objects:                  `.o` compiled & assembled files\n"                                               \
    "libs:                     `.a` for nnc_static libs\n"                                                      \
    "                          `.so` for shared libs \n"                                                        \
    "options (short):\n"                                                                                        \
    "  -o <file>               Set path for output file\n"                                                      \
    "  -c                      Compile only; do not assemble or link \n"                                        \
    "  -g                      Generates debug symbols when assembling.\n"                                      \
    "options (long):\n"                                                                                         \
    "  --help                  Display this information\n"                                                      \
    "  --gen-debug             Generates debug symbols when assembling.\n"                                      \
    "  --compile               Compile only; do not assemble or link \n"                                        \
    "  --no-opt                Disable peephole optimizations.\n"                                               \
    "  --output <file>         Set path for output file\n"                                                      \
    "  --dump-ast              Display abstract syntax tree of the program.\n"                                  \
    "  --dump-ir               Display intermediate representation of the program.\n"

typedef enum _nnc_option_id {
    O_NONE       = 0,
    O_HELP       = 1,
    O_VERBOSE    = 2,
    O_OPTIMIZE   = 3,
    O_DEBUG      = 4,
    O_COMPILE    = 5,
    O_OUT        = 6,
    O_OUT_IR     = 7,
    O_OUT_AST    = 8
} nnc_option_id;

typedef struct _nnc_option {
    nnc_option_id id;
    const char* opt;
    nnc_u8 opt_arg;
    nnc_i32* flag;
    nnc_u8 flag_value;
    const char* arg;
} nnc_option;

typedef struct _nnc_argv {
    const char* output;
    vector(const char*) sources;
    vector(const char*) objects;
    vector(const char*) static_libs;
    vector(const char*) shared_libs;
    nnc_i32 help;
    nnc_i32 verbose;
    nnc_i32 optimize;
    nnc_i32 gen_debug;
    nnc_i32 compile;
    nnc_i32 dump_ir;
    nnc_i32 dump_ast;
} nnc_argv;

#define tv_us(x) (glob_verbose.x.tv_sec * 1000000 + glob_verbose.x.tv_usec)
#define tv_ms(x) (tv_us(x) / 1000) 

typedef struct _nnc_verbose {
    struct timeval tv_start;
    struct timeval tv_link;
    struct timeval tv_compile;
    struct timeval tv_assemble;
    struct timeval tv_overall;
    nnc_i32 as_code;
    nnc_i32 ld_code;
    char* ld_params;
    char* as_params;
} nnc_verbose;

extern nnc_argv glob_argv;
extern nnc_verbose glob_verbose;

void nnc_glob_argv_init(
    nnc_i32 argc,
    char* const* argv
);

void nnc_glob_argv_fini();

#endif