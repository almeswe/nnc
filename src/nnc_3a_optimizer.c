#include "nnc_3a.h"

nnc_3a_quad_set* target_set = NULL;
nnc_3a_quad_set* target_opt_set = NULL;

typedef enum _nnc_3a_peep_pattern {
    OPT_RED_CROSS_TEMP_COPY,
    OPT_RED_CROSS_TEMP_REF,
    OPT_NONE
} nnc_3a_peep_pattern;

nnc_static nnc_3a_peep_pattern nnc_3a_ref_pattern(nnc_u64 index) {
    if (index >= buf_len(target_set->quads)) {
        return OPT_NONE;
    }  
    const nnc_3a_quad* quad1 = &target_set->quads[index];
    const nnc_3a_quad* quad2 = &target_set->quads[index+1];
    if (quad2->op == OP_DEREF_COPY) {
        if (quad1->res.kind == ADDR_CGT &&
            quad2->res.kind == ADDR_CGT) {
            if (quad1->res.exact.cgt ==
                quad2->res.exact.cgt) {
                return OPT_RED_CROSS_TEMP_REF;
            }
        }
    }
    return OPT_NONE;
}

nnc_static nnc_3a_peep_pattern nnc_3a_copy_pattern(nnc_u64 index) {
    // tX = x
    // tY = tX
    if (index >= buf_len(target_set->quads)) {
        return OPT_NONE;
    }  
    const nnc_3a_quad* quad1 = &target_set->quads[index];
    const nnc_3a_quad* quad2 = &target_set->quads[index+1];
    if (quad2->op == OP_COPY ||
        quad2->op == OP_DEREF_COPY) {
        if (quad1->res.kind == ADDR_CGT &&
            quad2->arg1.kind == ADDR_CGT) {
            if (quad1->res.exact.cgt == 
                quad2->arg1.exact.cgt) {
                return OPT_RED_CROSS_TEMP_COPY;
            }
        }
    }
    return OPT_NONE;
}

nnc_static nnc_3a_peep_pattern nnc_3a_search_peep_pattern(nnc_u64 index) {
    switch (target_set->quads[index].op) {
        case OP_REF:  return nnc_3a_ref_pattern(index);
        case OP_COPY: return nnc_3a_copy_pattern(index);
        default:      return OPT_NONE;
    }
}

nnc_static nnc_u64 nnc_3a_opt_none(nnc_u64 index) {
    const nnc_3a_quad* quad = &target_set->quads[index];
    buf_add(target_opt_set->quads, *quad);
    return 1;
}

nnc_static nnc_u64 nnc_3a_opt_red_cross_temp_ref(nnc_u64 index) {
    //  tX = &x
    // *tX = Y
    // -------
    // x = Y
    const nnc_3a_quad* quad1 = &target_set->quads[index];
    const nnc_3a_quad* quad2 = &target_set->quads[index+1];
    nnc_3a_quad opt_quad = {
        .op = OP_COPY, .res = quad1->arg1, .arg1 = quad2->arg1
    };
    buf_add(target_opt_set->quads, opt_quad);
    return 2;
}

nnc_static nnc_u64 nnc_3a_opt_red_cross_temp_copy(nnc_u64 index) {
    // tX = x
    // tY = tX
    // -------
    // tY = x
    const nnc_3a_quad* quad1 = &target_set->quads[index];
    const nnc_3a_quad* quad2 = &target_set->quads[index+1];
    nnc_3a_quad opt_quad = *quad2;
    opt_quad.arg1 = quad1->arg1;
    buf_add(target_opt_set->quads, opt_quad);
    return 2;
}

nnc_static nnc_u64 nnc_3a_optimize_peep(nnc_u64 index) {
    assert(target_set != NULL);
    switch (nnc_3a_search_peep_pattern(index)) {
        case OPT_NONE:                return nnc_3a_opt_none(index);
        case OPT_RED_CROSS_TEMP_REF:  return nnc_3a_opt_red_cross_temp_ref(index);
        case OPT_RED_CROSS_TEMP_COPY: return nnc_3a_opt_red_cross_temp_copy(index);
    }
    assert(false);
}

/*
@_test:
=========BLOCK[1]=========
     (i32)       t1 = &_x
     (i32)       t2 = 1
     (i32)      *t1 = t2
     (i32)       t3 = t2
     (i32)       t4 = _x
     (i32)       _y = t4
*/

/*
@_test:
=========BLOCK[1]=========
     (i32)       _x = 1
     (i32)       t3 = t2
     (i32)       _y = _x
*/

nnc_static void nnc_3a_optimize_pass() {
    nnc_u64 i = 0;
    nnc_u64 len = buf_len(target_set->quads);
    while (i < len) {
        i += nnc_3a_optimize_peep(i);
    }
}

void nnc_3a_optimize(nnc_3a_quad_set* set, nnc_3a_quad_set* opt_set) {
    opt_set->name = set->name;
    for (nnc_i32 pass = 0; pass < 3; pass++) {
        if (pass == 0) {
            target_set = set;
            target_opt_set = opt_set;
        }
        else {
            nnc_3a_quad_set* bufset = target_set;
            target_set = target_opt_set;
            target_opt_set = bufset;
            target_opt_set->quads = NULL;    
        }
        nnc_3a_optimize_pass();
        nnc_dispose(target_set->quads);
    }
    opt_set = target_opt_set;
}