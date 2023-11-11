#ifndef _NNC_3A_PEEPHOLE_OPTIMIZER_
#define _NNC_3A_PEEPHOLE_OPTIMIZER_

#define _NNC_ENABLE_PASS_LOGGING 0
#define _NNC_ENABLE_PEEP_OPTIMIZATIONS 1

#include "nnc_3a.h"

extern void nnc_dump_3a_quads(FILE* to, const nnc_3a_quad* quads);

typedef enum _nnc_3a_peep_pattern {
    /* Redundant cross operator optimizations */
    OPT_CROSS_REF,
    OPT_CROSS_COPY,
    OPT_CROSS_COPY3,
    /* Contant folding optimizations */
    OPT_INDEX_CONST_FOLD,
    OPT_UNARY_CONST_FOLD,
    OPT_BINARY_CONST_FOLD,
    /* Algebraic optimizations */
    OPT_ALG_MUL_ONE,
    OPT_ALG_MUL_ZERO,
    OPT_ALG_ADD_ZERO,
    OPT_ALG_MUL_POW_TWO,
    /* No optimization */
    OPT_NONE,
} nnc_3a_peep_pattern;

_vec_(nnc_3a_quad) nnc_3a_optimize(_vec_(nnc_3a_quad) quads, nnc_3a_opt_stat* stat);
nnc_3a_code nnc_3a_optimize_code(nnc_3a_code code);
nnc_3a_data nnc_3a_optimize_data(nnc_3a_data data);

#endif