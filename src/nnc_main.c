#include "nnc_core.h"

nnc_arena glob_arena;

static nnc_i32 nnc_main(nnc_i32 argc, char** argv) {
    return EXIT_SUCCESS;
}

nnc_i32 main(nnc_i32 argc, char** argv) {
    nnc_arena_init(&glob_arena);
    nnc_i32 status = nnc_main(argc, argv);
    nnc_arena_fini(&glob_arena);
    return status;
}