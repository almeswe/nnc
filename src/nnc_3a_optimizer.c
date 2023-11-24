#include "nnc_3a.h"

/**
 * @brief Optimized quad set.
*/
nnc_static _vec_(nnc_3a_quad) opt = NULL;

/**
 * @brief Non-optimized quad set.
*/
nnc_static _vec_(nnc_3a_quad) unopt = NULL;

/**
 * @brief Checks if further quads starting from `index` matching
 *  substitution pattern, triggered by `OP_REF` quad. (means first quad is `OP_REF`)
 *  --- This is part of function `nnc_3a_op_copy_pat` moved to separate function.
 *  -------------------------------------
 *  Pattern's Trigger:
 *      tX = &x            (OP_REF)
 *  Following part:    
 *     *tX = tZ            (OP_DEREF_COPY)
 * 
 * @param index Index of first quad with `OP_COPY` (which triggered this function).
 * @return `OPT_NONE` if pattern does not match, `OPT_REF_SUBST` otherwise.
 */
nnc_static nnc_3a_peep_pattern nnc_3a_op_ref_pat(nnc_u64 index) {
    if (index >= buf_len(unopt)) {
        return OPT_NONE;
    }  
    const nnc_3a_quad* quad1 = &unopt[index];
    const nnc_3a_quad* quad2 = &unopt[index+1];
    if (quad2->op == OP_DEREF_COPY) {
        if (quad1->res.kind == ADDR_CGT &&
            quad2->res.kind == ADDR_CGT) {
            if (quad1->res.exact.cgt ==
                quad2->res.exact.cgt) {
                return OPT_REF_SUBST;
            }
        }
    }
    return OPT_NONE;
}

/**
 * @brief Checks if further quads starting from `index` matching
 *  substitution pattern, triggered by `OP_COPY` quad. (means first quad is `OP_COPY`)
 *  --- This is part of function `nnc_3a_op_copy_pat` moved to separate function.
 *  -------------------------------------
 *  Pattern's Trigger:
 *    tX = x                    (OP_COPY)
 *  Following part:    
 *    tY = tX ||                (OP_COPY)
 *   *tY = tX ||                (OP_DEREF_COPY)
 *    
 *    tZ = tX ||                (OP_COPY)
 *   *tZ = tX                   (OP_DEREF_COPY)
 * 
 * @param index Index of first quad with `OP_COPY` (which triggered this function).
 * @return `OPT_NONE` if pattern does not match, `OPT_COPY_UNARY_EX_SUBST` otherwise.
 */
nnc_static nnc_3a_peep_pattern nnc_3a_op_copy_pat_part1(nnc_u64 index) {
    const nnc_3a_quad* quad1 = &unopt[index];
    const nnc_3a_quad* quad2 = &unopt[index+1];
    const nnc_3a_quad* quad3 = NULL;
    if (index+2 < buf_len(unopt)) {
        quad3 = &unopt[index+2];
    }
    if (quad3 == NULL) {
        return OPT_NONE;
    }
    /*
        Both quad1 and quad2 must be ADDR_CGTs
        and quad1's result CGT must be equal 
        quad2's first argument CGT.
    */
    if (quad1->res.kind  == ADDR_CGT && 
        quad2->arg1.kind == ADDR_CGT &&
       (quad1->res.exact.cgt == 
        quad2->arg1.exact.cgt)) {
        switch (quad2->op) {
            case OP_COPY:
            case OP_DEREF_COPY: {
                /*
                    quad3 must be OP_COPY or OP_DEREF_COPY,
                    it's first argument must be ADDR_CGT which
                    equals quad1's result.
                */
                if (quad3->arg1.kind == ADDR_CGT &&
                   (quad3->op == OP_COPY ||
                    quad3->op == OP_DEREF_COPY)) {
                    if (quad1->res.exact.cgt == 
                        quad3->arg1.exact.cgt) {
                        return OPT_COPY_UNARY_EX_SUBST;
                    }
                }
            }
            default: break;
        }
    }
    return OPT_NONE;
}

/**
 * @brief Checks if further quads starting from `index` matching
 *  folding pattern, triggered by `OP_COPY` quad. (means first quad is `OP_COPY`)
 *  --- This is part of function `nnc_3a_op_copy_pat` moved to separate function.
 *  Note: Jump's condition can be easily folded too, this will help CFG to determine
 *  unreachable basic blocks in future.
 *  -------------------------------------
 *  Pattern's Trigger:
 *      tX = x                     (OP_COPY)
 *  Following part: (where x = const)
 *      tZ = unOp tX     ||        (Unary operator: +, -, ~)
 *      tZ = (type) tX   ||        (OP_CAST)
 *      if tX goto L     ||        (OP_CJUMPT) 
 *      if not tX goto L ||        (OP_CJUMPF)
 * 
 * @param index Index of first quad with `OP_COPY` (which triggered this function).
 * @return `OPT_NONE` if pattern does not match, `OPT_UNARY_CONST_FOLD` otherwise.
 */
nnc_static nnc_3a_peep_pattern nnc_3a_op_copy_pat_part2(nnc_u64 index) {
    const nnc_3a_quad* quad1 = &unopt[index];
    const nnc_3a_quad* quad2 = &unopt[index+1];
    if (quad1->res.kind == ADDR_CGT &&
       (quad1->arg1.kind == ADDR_ICONST || 
        quad1->arg1.kind == ADDR_FCONST)) {
        switch (quad2->op) {
            case OP_CAST:
            case OP_CJUMPT:
            case OP_CJUMPF:
            case OP_UNARY: {
                if (quad1->res.exact.cgt == 
                    quad2->arg1.exact.cgt) {
                    return OPT_UNARY_CONST_FOLD;
                }
                break;
            }
            default: break;
        }
    }
    return OPT_NONE;
}

/**
 * @brief Checks if further quads starting from `index` matching
 *  folding pattern, triggered by `OP_COPY` quad. (means first quad is `OP_COPY`)
 *  --- This is part of function `nnc_3a_op_copy_pat` moved to separate function.
 *  Note: Jump's condition can be easily folded too, this will help CFG to determine
 *  unreachable basic blocks in future.
 *  -------------------------------------
 *  Pattern's Trigger:
 *      tX = x                        (OP_COPY)
 *  Following part: (where x = const)
 *                  (where y = const)
 *      tY = y                        (OP_COPY)
 *      tZ = tX binOp tY        ||    (Binary operator: +,-,*,/,%,<<,>>,|,&,^)
 *      tZ = if tX == tY goto L ||    (OP_CJUMPE)
 *      tZ = if tX != tY goto L ||    (OP_CJUMPNE)
 *      tZ = if tX < tY goto L  ||    (OP_CJUMPLT)
 *      tZ = if tX > tY goto L  ||    (OP_CJUMPGT)
 *      tZ = if tX <= tY goto L ||    (OP_CJUMPLTE)
 *      tZ = if tX >= tY goto L       (OP_CJUMPGTE)
 * 
 * @param index Index of first quad with `OP_COPY` (which triggered this function).
 * @return `OPT_NONE` if pattern does not match, `OPT_BINARY_CONST_FOLD` otherwise.
 */
nnc_static nnc_3a_peep_pattern nnc_3a_op_copy_pat_part3(nnc_u64 index) {

    const nnc_3a_quad* quad1 = &unopt[index];
    const nnc_3a_quad* quad2 = &unopt[index+1];
    const nnc_3a_quad* quad3 = NULL;
    if (index + 2 < buf_len(unopt)) {
        quad3 = &unopt[index+2];
    }
    if (quad3 == NULL) {
        return OPT_NONE;
    }
    if (quad1->res.kind == ADDR_CGT &&
        quad2->res.kind == ADDR_CGT &&
       (quad1->arg1.kind == ADDR_ICONST  || 
        quad1->arg1.kind == ADDR_FCONST) &&
       (quad2->arg1.kind == ADDR_ICONST  ||
        quad2->arg1.kind == ADDR_FCONST)) {
        switch (quad3->op) {
            case OP_CJUMPE:
            case OP_CJUMPNE:
            case OP_CJUMPGT:
            case OP_CJUMPLT:
            case OP_CJUMPGTE:
            case OP_CJUMPLTE:
            case OP_BINARY: {
                nnc_3a_cgt q1_res = quad1->res.exact.cgt;
                nnc_3a_cgt q2_res = quad2->res.exact.cgt;
                if (quad3->arg1.exact.cgt == q1_res &&
                    quad3->arg2.exact.cgt == q2_res) {
                    return OPT_BINARY_CONST_FOLD;
                }
            }
            default: break;
        }
    }
    return OPT_NONE;
}

/**
 * @brief Checks if further quads starting from `index` matching
 *  folding pattern, triggered by `OP_COPY` quad. (means first quad is `OP_COPY`)
 *  --- This is part of function `nnc_3a_op_copy_pat` moved to separate function.
 *  -------------------------------------
 *  This optimization targets array's index calculation.
 *  Pattern's Trigger:
 *      tX = x                  (OP_COPY)
 *  Following part: (where x  = const)
 *      tZ = tX binOp const     (Binary operator: +, *)
 * 
 * @param index Index of first quad with `OP_COPY` (which triggered this function).
 * @return `OPT_NONE` if pattern does not match, `OPT_INDEX_CONST_FOLD` otherwise.
 */
nnc_static nnc_3a_peep_pattern nnc_3a_op_copy_pat_part4(nnc_u64 index) {
    const nnc_3a_quad* quad1 = &unopt[index];
    const nnc_3a_quad* quad2 = &unopt[index+1];
    if ((quad1->res.kind  == ADDR_CGT    &&
         quad2->arg1.kind == ADDR_CGT)   &&
        (quad1->arg1.kind == ADDR_ICONST && 
         quad2->arg2.kind == ADDR_ICONST)) {
        switch (quad2->op) {
            case OP_ADD: 
            case OP_MUL: {
                if (quad1->res.exact.cgt ==
                    quad2->arg1.exact.cgt) {
                    return OPT_INDEX_CONST_FOLD;
                }
            }
            default: break;
        }
    }
    return OPT_NONE;
}

/**
 * @brief Checks if further quads starting from `index` matching
 *  algebraic pattern, triggered by `OP_COPY` quad. (means first quad is `OP_COPY`)
 *  --- This is part of function `nnc_3a_op_copy_pat` moved to separate function.
 *  -------------------------------------
 *   This optimization targets redundant algebraic operations.
 *   Pattern's Trigger:
 *       tX = 0  ||                (OP_COPY)
 *       tX = x                    (OP_COPY)
 *   Following part:
 *       tY = tX binOp 0 ||        (Binary operator: *, +)
 *       tY = 0  binOp tX 
 * 
 * @param index Index of first quad with `OP_COPY` (which triggered this function).
 * @return `OPT_NONE` if pattern does not match, `OPT_ALG_MUL_ZERO` | `OPT_ALG_ADD_ZERO` otherwise.
 */
nnc_static nnc_3a_peep_pattern nnc_3a_op_copy_pat_part5(nnc_u64 index) {
    const nnc_3a_quad* quad1 = &unopt[index];
    const nnc_3a_quad* quad2 = &unopt[index+1];
    const nnc_3a_quad* quad3 = NULL;
    if (index + 2 < buf_len(unopt)) {
        quad3 = &unopt[index+2];
    }
    if (quad3 == NULL) {
        return OPT_NONE;
    }
    if (quad1->arg1.kind == ADDR_ICONST ||
        quad2->arg1.kind == ADDR_ICONST) {
        if (quad3->op == OP_ADD || quad3->op == OP_MUL) {
            if (quad1->arg1.exact.iconst.iconst == 0 ||
                quad2->arg1.exact.iconst.iconst == 0) {
                if ((quad1->res.exact.cgt == quad3->arg1.exact.cgt) ||
                    (quad2->res.exact.cgt == quad3->arg2.exact.cgt)) {
                    return quad3->op == OP_MUL ? 
                        OPT_ALG_MUL_ZERO : OPT_ALG_ADD_ZERO;
                }
            }
        }
    }
    return OPT_NONE;
}

/**
 * @brief Checks if further quads starting from `index` matching
 *  algebraic pattern, triggered by `OP_COPY` quad. (means first quad is `OP_COPY`)
 *  --- This is part of function `nnc_3a_op_copy_pat` moved to separate function.
 *  -------------------------------------
 *  This optimization targets redundant algebraic operations.
 *  Pattern's Trigger:
 *      tX = 1  ||                (OP_COPY)
 *      tX = 2^n                  (OP_COPY)
 *      tX = x                    (OP_COPY)
 *  Following part:
 *      tY = tX * 1   ||          (Binary operator: *)
 *      tY = 1 * tX   ||          (Binary operator: *)
 *      tY = 2^n * tX ||          (Binary operator: *)
 *      tY = tX * 2^n ||          (Binary operator: *)
 * 
 * @param index Index of first quad with `OP_COPY` (which triggered this function).
 * @return `OPT_NONE` if pattern does not match, `OPT_ALG_MUL_ONE` | `OPT_ALG_MUL_POW_TWO` otherwise.
 */
nnc_static nnc_3a_peep_pattern nnc_3a_op_copy_pat_part6(nnc_u64 index) {
    const nnc_3a_quad* quad1 = &unopt[index];
    const nnc_3a_quad* quad2 = &unopt[index+1];
    const nnc_3a_quad* quad3 = NULL;
    if (index + 2 < buf_len(unopt)) {
        quad3 = &unopt[index+2];
    }
    if (quad3 == NULL || quad3->op != OP_MUL) {
        return OPT_NONE;
    }
    if ((quad1->arg1.kind == ADDR_ICONST ||
         quad2->arg1.kind == ADDR_ICONST)) {
        if (quad1->arg1.exact.iconst.iconst == 1 ||
            quad2->arg1.exact.iconst.iconst == 1) {
            if ((quad1->res.exact.cgt == quad3->arg1.exact.cgt) ||
                (quad2->res.exact.cgt == quad3->arg2.exact.cgt)) {
                return OPT_ALG_MUL_ONE;
            }
        }
        if (nnc_pow2(quad1->arg1.exact.iconst.iconst) ||
            nnc_pow2(quad2->arg1.exact.iconst.iconst)) {
            if ((quad1->res.exact.cgt == quad3->arg1.exact.cgt) ||
                (quad2->res.exact.cgt == quad3->arg2.exact.cgt)) {
                return OPT_ALG_MUL_POW_TWO;
            }
        }
    }
    return OPT_NONE;
}

/**
 * @brief Checks if further quads starting from `index` matching
 *  substitution pattern, triggered by `OP_COPY` quad. (means first quad is `OP_COPY`)
 *  --- This is part of function `nnc_3a_op_copy_pat` moved to separate function.
 *  -------------------------------------
 *  Pattern's Trigger: (addrX and addrY can be constant, name or CGT)
 *      tX = addrX                      (OP_COPY)
 *  Following part:
 *      tY = addrY                      (OP_COPY)
 *      tZ = tX binOp tY        ||      (Binary operator: +,-,*,/,%,<<,>>,|,&,^)
 *      tZ = if tX == tY goto L ||      (OP_CJUMPE)
 *      tZ = if tX != tY goto L ||      (OP_CJUMPNE)
 *      tZ = if tX < tY goto L  ||      (OP_CJUMPLT)
 *      tZ = if tX > tY goto L  ||      (OP_CJUMPGT)
 *      tZ = if tX <= tY goto L ||      (OP_CJUMPLTE)
 *      tZ = if tX >= tY goto L         (OP_CJUMPGTE)
 * 
 * @param index Index of first quad with `OP_COPY` (which triggered this function).
 * @return `OPT_NONE` if pattern does not match, `OPT_COPY_BINARY_L_SUBST` | 
 *  `OPT_COPY_BINARY_R_SUBST` | `OPT_COPY_BINARY_SUBST` otherwise.
 */
nnc_static nnc_3a_peep_pattern nnc_3a_op_copy_pat_part7(nnc_u64 index) {
    const nnc_3a_quad* quad1 = &unopt[index];
    const nnc_3a_quad* quad2 = &unopt[index+1];
    const nnc_3a_quad* quad3 = NULL;
    if (index + 2 < buf_len(unopt)) {
        quad3 = &unopt[index+2];
    }
    if (quad1->op != OP_COPY &&
        quad2->op != OP_COPY) {
        return OPT_NONE;
    }
    if (quad3 == NULL) {
        return OPT_NONE;
    }
    nnc_3a_peep_pattern pat = OPT_NONE;
    if (quad1->res.kind == ADDR_CGT &&
        quad2->res.kind == ADDR_CGT) {
        switch (quad3->op) {
            case OP_CJUMPE:
            case OP_CJUMPNE:
            case OP_CJUMPGT:
            case OP_CJUMPLT:
            case OP_CJUMPGTE:
            case OP_CJUMPLTE:
            case OP_BINARY: {
                nnc_3a_cgt q1_res = quad1->res.exact.cgt;
                nnc_3a_cgt q2_res = quad2->res.exact.cgt;
                if (quad1->op == OP_COPY &&
                    quad3->arg1.kind == ADDR_CGT &&
                    quad3->arg1.exact.cgt == q1_res) {
                    pat = OPT_COPY_BINARY_L_SUBST;
                }
                if (quad2->op == OP_COPY &&
                    quad3->arg2.kind == ADDR_CGT &&
                    quad3->arg2.exact.cgt == q2_res) {
                    pat = (pat == OPT_COPY_BINARY_L_SUBST ?
                        OPT_COPY_BINARY_SUBST : 
                        OPT_COPY_BINARY_R_SUBST);
                }
                break;
            }
            default: break;
        }
    }
    return pat;
}

/**
 * @brief Checks if further quads starting from `index` matching
 *  substitution pattern, triggered by `OP_COPY` quad. (means first quad is `OP_COPY`)
 *  --- This is part of function `nnc_3a_op_copy_pat` moved to separate function.
 *  -------------------------------------
 *  Pattern's Trigger: (addrX can be constant, name or CGT)
 *      tX = addrX                  (OP_COPY)
 *  Following part:
 *      arg tX           ||        (OP_ARG)
 *      tZ = tX          ||        (OP_COPY)
 *      tZ = (type) tX   ||        (OP_CAST)
 *      ret tX           ||        (OP_RETF)
 *      tZ = *tX         ||        (OP_DEREF)
 *      if tZ goto L     ||        (OP_CJUMPT) 
 *      if not tZ goto L ||        (OP_CJUMPF)
 *     *tZ = tX          ||        (OP_DEREF_COPY)
 *      tZ = unOp tX     ||        (Unary operator: +, -, ~)
 * 
 * @param index Index of first quad with `OP_COPY` (which triggered this function).
 * @return `OPT_NONE` if pattern does not match, `OPT_COPY_UNARY_SUBST` otherwise.
 */
nnc_static nnc_3a_peep_pattern nnc_3a_op_copy_pat_part8(nnc_u64 index) {
    const nnc_3a_quad* quad1 = &unopt[index];
    const nnc_3a_quad* quad2 = &unopt[index+1];
    if (quad1->op != OP_COPY) {
        return OPT_NONE;
    }
    if (quad1->res.kind == ADDR_CGT) {
        switch (quad2->op) {
            case OP_ARG:
            case OP_COPY:
            case OP_CAST:
            case OP_RETF:
            case OP_DEREF:
            case OP_CJUMPE:
            case OP_CJUMPNE:
            case OP_DEREF_COPY:
            case OP_UNARY: {
                nnc_3a_cgt q1_res = quad1->res.exact.cgt;
                if (quad2->arg1.kind == ADDR_CGT &&
                    quad2->arg1.exact.cgt == q1_res) {
                    return OPT_COPY_UNARY_SUBST;
                }
                break;
            }
            default: break;
        }
    }
    return OPT_NONE;
}

/**
 * @brief Checks if further quads starting from `index` matching
 *  substitution pattern, triggered by `OP_COPY` quad. (means first quad is `OP_COPY`)
 * @param index Index of first quad with `OP_COPY` (which triggered this function).
 * @return `OPT_NONE` if pattern does not match, otherwise some patter is returned.
 */
nnc_static nnc_3a_peep_pattern nnc_3a_op_copy_pat(nnc_u64 index) {
    typedef nnc_3a_peep_pattern (pat_fn)(nnc_u64);
    static pat_fn* pat_fns[] = {
        nnc_3a_op_copy_pat_part2,
        nnc_3a_op_copy_pat_part3,
        nnc_3a_op_copy_pat_part4,
        nnc_3a_op_copy_pat_part5,
        nnc_3a_op_copy_pat_part6,
        nnc_3a_op_copy_pat_part1,
        nnc_3a_op_copy_pat_part7,
        nnc_3a_op_copy_pat_part8,
    };
    if (index >= buf_len(unopt)) {
        return OPT_NONE;
    }
    nnc_3a_peep_pattern pat = OPT_NONE;
    const nnc_u64 pat_fns_count = sizeof(pat_fns)/sizeof(pat_fn*);
    for (nnc_u64 i = 0; i < pat_fns_count; i++) {
        pat = pat_fns[i](index);
        if (pat != OPT_NONE) {
            return pat;
        }
    }
    return OPT_NONE;
}

/**
 * @brief Optimizes `OPT_REF_SUBST` pattern.
 *  Accepts:
 *       tX = &x
 *      *tX = y
 *  Optimization:
 *       x = y
 * 
 * @param index Index of first quad that triggered pattern routine. 
 * @return Number of quads (2) skipped by this optimizing function in initial quad set.
 */
nnc_static nnc_u64 nnc_3a_opt_ref_subst(nnc_u64 index) {
    const nnc_3a_quad* quad1 = &unopt[index];
    const nnc_3a_quad* quad2 = &unopt[index+1];
    nnc_3a_quad opt_quad = {
        .op = OP_COPY, .res = quad1->arg1, .arg1 = quad2->arg1
    };
    buf_add(opt, opt_quad);
    return 2;
}

/**
 * @brief Optimizes `OPT_COPY_UNARY_SUBST` pattern.
 *  Accepts:
 *      tX = addrX
 *      arg tX           ||
 *      tZ = tX          ||
 *      tZ = (type) tX   ||
 *      ret tX           ||
 *      tZ = *tX         ||
 *      if tZ goto L     ||
 *      if not tZ goto L ||
 *     *tZ = tX          ||
 *      tZ = unOp tX     ||
 *  Optimization:
 *      arg addrX           ||
 *      tZ = addrX          ||
 *      tZ = (type) addrX   ||
 *      ret addrX           ||
 *      tZ = *addrX         ||
 *      if addrX goto L     ||
 *      if not addrX goto L ||
 *     *tZ = addrX          ||
 *      tZ = unOp addrX     ||
 * 
 * @param index Index of first quad that triggered pattern routine. 
 * @return Number of quads (2) skipped by this optimizing function in initial quad set.
 */
nnc_static nnc_u64 nnc_3a_opt_unary_subst(nnc_u64 index) {
    const nnc_3a_quad* op_quad = &unopt[index];
    const nnc_3a_quad* res_quad = &unopt[index+1];
    nnc_3a_quad opt_res_quad = *res_quad;
    opt_res_quad.arg1 = op_quad->arg1;
    buf_add(opt, opt_res_quad);
    return 2;
}

/**
 * @brief Optimizes `OPT_COPY_UNARY_EX_SUBST` pattern.
 *  Accepts:
 *      tX = x
 *      tY = tX
 *      tZ = tX
 *  Optimization:
 *      tY = x
 *      tZ = x
 * 
 * @param index Index of first quad that triggered pattern routine. 
 * @return Number of quads (2) skipped by this optimizing function in initial quad set.
 */
nnc_static nnc_u64 nnc_3a_opt_unary_ex_subst(nnc_u64 index) {
    const nnc_3a_quad* quad1 = &unopt[index];
    const nnc_3a_quad* quad2 = &unopt[index+1];
    nnc_3a_quad* quad3 = &unopt[index+2];
    nnc_3a_quad opt_quad2 = *quad2;
    opt_quad2.arg1 = quad1->arg1;
    nnc_3a_quad opt_quad3 = *quad3;
    opt_quad3.arg1 = quad1->arg1;
    buf_add(opt, opt_quad2);
    *quad3 = opt_quad3;
    return 2;
}
/**
 * @brief Optimizes `OPT_COPY_BINARY_L_SUBST`, `OPT_COPY_BINARY_R_SUBST` or `OPT_COPY_BINARY_SUBST` pattern.
 *  Accepts (for OPT_COPY_BINARY_L_SUBST):
 *      tX = addrX
 *      tY = ... (not OP_COPY)
 *      tZ = tX (binOp) tY              ||
 *      tZ = if tX (jmpBinOp) tY goto L
 *  Optimization (for OPT_COPY_BINARY_L_SUBST)
 *      tY = ... (not OP_COPY)
 *      tZ = addrX (binOp) tY ||
 *      tZ = if addrX (jmpBinOp) tY goto L
 * 
 *  Accepts (for OPT_COPY_BINARY_R_SUBST):
 *      tX = ... (not OP_COPY)
 *      tY = addrY
 *      tZ = tX (binOp) tY              ||
 *      tZ = if tX (jmpBinOp) tY goto L
 *  Optimization (for OPT_COPY_BINARY_R_SUBST)
 *      tX = ... (not OP_COPY)
 *      tZ = tX (binOp) addrY ||
 *      tZ = if tX (jmpBinOp) addrY goto L
 * 
 *  Accepts (for OPT_COPY_BINARY_SUBST):
 *      tX = addrX
 *      tY = addrY
 *      tZ = tX (binOp) tY              ||
 *      tZ = if tX (jmpBinOp) tY goto L
 *  Optimization (for OPT_COPY_BINARY_SUBST)
 *      tZ = addrX (binOp) addrY              ||
 *      tZ = if addrX (jmpBinOp) addrY goto L
 *
 * @param index Index of first quad that triggered pattern routine. 
 * @param pat Concrete pattern.
 * @return Number of quads (3) skipped by this optimizing function in initial quad set.
 */
nnc_static nnc_u64 nnc_3a_opt_binary_subst(nnc_u64 index, nnc_3a_peep_pattern pat) {
    const nnc_3a_quad* lop_quad = &unopt[index];
    const nnc_3a_quad* rop_quad = &unopt[index+1];
    const nnc_3a_quad* res_quad = &unopt[index+2];
    nnc_3a_quad opt_res_quad = *res_quad;
    switch (pat) {
        case OPT_COPY_BINARY_L_SUBST: {
            opt_res_quad.arg1 = lop_quad->arg1;
            buf_add(opt, *rop_quad);
            break;
        }
        case OPT_COPY_BINARY_R_SUBST: {
            opt_res_quad.arg2 = rop_quad->arg1;
            buf_add(opt, *lop_quad);
            break;
        }
        case OPT_COPY_BINARY_SUBST: {
            opt_res_quad.arg1 = lop_quad->arg1;
            opt_res_quad.arg2 = rop_quad->arg1;
            break;
        }
        default: nnc_abort_no_ctx("nnc_3a_opt_binary_subst: unknown pattern kind."); 
    }
    buf_add(opt, opt_res_quad);
    return 3;
}

/**
 * @brief Optimizes `OPT_UNARY_CONST_FOLD` pattern.
 * This routine is used for case when expression has float type.
 *  Accepts:
 *      tX = x
 *      tZ = unOp tX     ||
 *      tZ = (type) tX   ||
 *  Optimization:
 *      tZ = const
 * 
 * @param index Index of first quad that triggered pattern routine. 
 * @return Number of quads (2) skipped by this optimizing function in initial quad set.
 */
nnc_static nnc_u64 nnc_3a_opt_unary_fconst_fold(nnc_u64 index) {
    const nnc_3a_quad* op_quad = &unopt[index];
    const nnc_3a_quad* un_op_quad = &unopt[index+1];
    const nnc_type* un_op_type = un_op_quad->res.type;
    nnc_f64 val = op_quad->arg1.exact.fconst.fconst;
    if (op_quad->arg1.kind == ADDR_ICONST) {
        val = op_quad->arg1.exact.iconst.iconst;
    }
    switch (un_op_quad->op) {
        case OP_PLUS:  break;
        case OP_MINUS: val = -val; break;
        case OP_CAST: {
            nnc_f64 fval = op_quad->arg1.exact.fconst.fconst;
            if (op_quad->arg1.kind == ADDR_ICONST) {
                fval = op_quad->arg1.exact.iconst.iconst;
            }
            switch (un_op_type->kind) {
                case T_PRIMITIVE_F32: val = (nnc_f32)fval; break;
                case T_PRIMITIVE_F64: val = (nnc_f64)fval; break;
                default: nnc_abort_no_ctx("nnc_3a_opt_unary_fconst_fold: unknown un_op_type."); 
            }
            break;
        }
        default: nnc_abort_no_ctx("nnc_3a_opt_unary_fconst_fold: unknown op.");
    }
    nnc_3a_quad opt_quad = {
        .op = OP_COPY, .res = un_op_quad->res, .arg1 = nnc_3a_mkf2(val, un_op_type)
    };
    buf_add(opt, opt_quad);
    return 2;
}

/**
 * @brief Optimizes `OPT_UNARY_CONST_FOLD` pattern.
 * This routine is used for case of branch operators.
 *  Accepts:
 *      tX = x
 *      if tX goto L     || 
 *      if not tX goto L ||
 * 
 *  Optimization:
 *      goto L || (OP_UJUMP) (in case when constant condition is true)
 *      ...       (OP_NONE)  (sets as OP_NONE in case when constant condition is true)
 * 
 * @param index Index of first quad that triggered pattern routine. 
 * @return Number of quads (2) skipped by this optimizing function in initial quad set.
 */
nnc_static nnc_u64 nnc_3a_opt_unary_branch_fold(nnc_u64 index) {
    const nnc_3a_quad* op_quad = &unopt[index];
    const nnc_3a_quad* res_quad = &unopt[index+1];
    nnc_u64 val = op_quad->arg1.exact.iconst.iconst;
    nnc_3a_quad opt_quad = {
        .op = OP_NONE, .res = res_quad->res
    };
    switch (res_quad->op) {
        case OP_CJUMPT: val = val != 0; break;
        case OP_CJUMPF: val = val == 0; break;
        default: nnc_abort_no_ctx("nnc_3a_opt_unary_branch_fold: unknown res_quad->op."); 
    }
    // if constant value is 1, condition is true,
    // and change conditional jump to unconditional. 
    if (val == 1) {
        opt_quad.op = OP_UJUMP;
        buf_add(opt, opt_quad);
    }
    // if constant value is 0, just remove it,
    // because flow of the program goes further, and
    // will not depend on this condition.
    return 2;
} 

/**
 * @brief Optimizes `OPT_UNARY_CONST_FOLD` pattern.
 * This routine is used for case when expression has integral type.
 * It also handles jump operators by calling `nnc_3a_opt_unary_branch_fold`.  
 * Accepts:
 *      tX = x
 *      tZ = unOp tX     ||
 *      tZ = (type) tX   ||
 * Optimization:
 *      tZ = const
 * 
 * @param index Index of first quad that triggered pattern routine. 
 * @return Number of quads (2) skipped by this optimizing function in initial quad set.
 */
nnc_static nnc_u64 nnc_3a_opt_unary_iconst_fold(nnc_u64 index) {
    const nnc_3a_quad* op_quad = &unopt[index];
    const nnc_3a_quad* res_quad = &unopt[index+1];
    const nnc_type* res_type = res_quad->res.type;
    nnc_u64 val = op_quad->arg1.exact.iconst.iconst;
    switch (res_quad->op) {
        case OP_PLUS:   break;
        case OP_MINUS:  val = -val; break;
        case OP_BW_NOT: val = ~val; break;
        case OP_CAST: {
            nnc_f64 fval = op_quad->arg1.exact.fconst.fconst;
            if (op_quad->arg1.kind == ADDR_ICONST) {
                fval = op_quad->arg1.exact.iconst.iconst;
            }
            switch (res_type->kind) {
                case T_PRIMITIVE_I8:  val = (nnc_i8)fval;  break;
                case T_PRIMITIVE_U8:  val = (nnc_u8)fval;  break;
                case T_PRIMITIVE_U16: val = (nnc_u16)fval; break;
                case T_PRIMITIVE_I16: val = (nnc_i16)fval; break;
                case T_PRIMITIVE_U32: val = (nnc_u32)fval; break;
                case T_PRIMITIVE_I32: val = (nnc_i32)fval; break;
                case T_PRIMITIVE_U64: val = (nnc_u64)fval; break;
                case T_PRIMITIVE_I64: val = (nnc_i64)fval; break;
                default: nnc_abort_no_ctx("nnc_3a_opt_unary_iconst_fold: unknown un_op_type."); 
            }
            break;
        }
        case OP_CJUMPT:
        case OP_CJUMPF: {
            return nnc_3a_opt_unary_branch_fold(index);
        }
        default: nnc_abort_no_ctx("nnc_3a_opt_unary_iconst_fold: unknown op.");
    }
    nnc_3a_quad opt_quad = {
        .op = OP_COPY, .res = res_quad->res, .arg1 = nnc_3a_mki2(val, res_type) 
    };
    buf_add(opt, opt_quad);
    return 2;
}

/**
 * @brief Optimizes `OPT_UNARY_CONST_FOLD` pattern for both integral and float expression types.
 *  For more information about optimization see `nnc_3a_opt_unary_fconst_fold` and
 *  `nnc_3a_opt_unary_iconst_fold` functions.
 * @param index Index of first quad that triggered pattern routine. 
 * @return Number of quads (2) skipped by this optimizing function in initial quad set.
 */
nnc_static nnc_u64 nnc_3a_opt_unary_const_fold(nnc_u64 index) {
    const nnc_3a_quad* un_op_quad = &unopt[index+1];
    const nnc_type* un_op_type = un_op_quad->res.type;
    if (nnc_real_type(un_op_type)) {
        return nnc_3a_opt_unary_fconst_fold(index);
    }
    if (nnc_integral_type(un_op_type)) {
        return nnc_3a_opt_unary_iconst_fold(index);
    }
    nnc_abort_no_ctx("nnc_3a_opt_unary_const_fold: unknown result type.\n");
    return 2;
}

/**
 * @brief Optimizes `OPT_BINARY_CONST_FOLD` pattern.
 * This routine is used for case when expression has float type and jump opetators.
 * Accepts:
 *      tX = x                        (OP_COPY)
 *      tY = y                        (OP_COPY)
 *      tZ = if tX == tY goto L ||    (OP_CJUMPE)
 *      tZ = if tX != tY goto L ||    (OP_CJUMPNE)
 *      tZ = if tX < tY goto L  ||    (OP_CJUMPLT)
 *      tZ = if tX > tY goto L  ||    (OP_CJUMPGT)
 *      tZ = if tX <= tY goto L ||    (OP_CJUMPLTE)
 *      tZ = if tX >= tY goto L       (OP_CJUMPGTE)
 *  Optimization:
 *      goto L || (OP_UJUMP) (in case when constant condition is true)
 *      ...       (OP_NONE)  (sets as OP_NONE in case when constant condition is true)
 * 
 * @param index Index of first quad that triggered pattern routine.
 * @return Number of quads (3) skipped by this optimizing function in initial quad set.
 */
nnc_static nnc_u64 nnc_3a_opt_binary_fconst_branch_fold(nnc_u64 index) {
    const nnc_3a_quad* lop_quad = &unopt[index];
    const nnc_3a_quad* rop_quad = &unopt[index+1];
    const nnc_3a_quad* res_quad = &unopt[index+2];
    nnc_f64 lval = lop_quad->arg1.exact.fconst.fconst;
    if (lop_quad->arg1.kind == ADDR_ICONST) {
        lval = lop_quad->arg1.exact.iconst.iconst;
    }
    nnc_f64 rval = rop_quad->arg1.exact.fconst.fconst;
    if (rop_quad->arg1.kind == ADDR_ICONST) {
        rval = rop_quad->arg1.exact.iconst.iconst;
    }
    nnc_3a_quad opt_quad = {
        .op = OP_NONE, .res = res_quad->res
    };
    switch (res_quad->op) {
        case OP_CJUMPE:     lval = lval == rval; break;
        case OP_CJUMPNE:    lval = lval != rval; break;
        case OP_CJUMPLT:    lval = lval < rval;  break;
        case OP_CJUMPGT:    lval = lval > rval;  break;
        case OP_CJUMPLTE:   lval = lval <= rval; break;
        case OP_CJUMPGTE:   lval = lval >= rval; break;
        default: nnc_abort_no_ctx("nnc_3a_opt_binary_fconst_branch_fold: unknown res_quad->op."); 
    }
    if (lval == 1) {
        opt_quad.op = OP_UJUMP;
        buf_add(opt, opt_quad);
    }
    return 3;
}

/**
 * @brief Optimizes `OPT_BINARY_CONST_FOLD` pattern.
 * This routine is used for case when expression has float type.
 * It also handles jump operators by calling `nnc_3a_opt_binary_fconst_branch_fold`.
 * Accepts:
 *      tX = x                        
 *      tY = y                        
 *      tZ = tX binOp tY
 *  Optimization:
 *      tZ = const
 * 
 * @param index Index of first quad that triggered pattern routine.
 * @return Number of quads (3) skipped by this optimizing function in initial quad set.
 */
nnc_static nnc_u64 nnc_3a_opt_binary_fconst_fold(nnc_u64 index) {
    const nnc_3a_quad* lop_quad = &unopt[index];
    const nnc_3a_quad* rop_quad = &unopt[index+1];
    const nnc_3a_quad* bin_op_quad = &unopt[index+2];
    const nnc_type* bin_op_type = bin_op_quad->res.type;
    nnc_f64 lval = lop_quad->arg1.exact.fconst.fconst;
    if (lop_quad->arg1.kind == ADDR_ICONST) {
        lval = lop_quad->arg1.exact.iconst.iconst;
    }
    nnc_f64 rval = rop_quad->arg1.exact.fconst.fconst;
    if (rop_quad->arg1.kind == ADDR_ICONST) {
        rval = rop_quad->arg1.exact.iconst.iconst;
    }
    switch (bin_op_quad->op) {
        case OP_ADD: lval += rval; break;
        case OP_SUB: lval -= rval; break;
        case OP_MUL: lval *= rval; break;
        case OP_DIV: lval /= rval; break;
        case OP_CJUMPE:
        case OP_CJUMPNE:
        case OP_CJUMPLT:
        case OP_CJUMPGT:
        case OP_CJUMPLTE:
        case OP_CJUMPGTE: {
            return nnc_3a_opt_binary_fconst_branch_fold(index);
        }
        default: nnc_abort_no_ctx("nnc_3a_opt_binary_fconst_fold: unknown op.");
    }
    nnc_3a_quad opt_quad = {
        .op = OP_COPY, .res = bin_op_quad->res, .arg1 = nnc_3a_mkf2(lval, bin_op_type)
    };
    buf_add(opt, opt_quad);
    return 3;
}

/**
 * @brief Optimizes `OPT_BINARY_CONST_FOLD` pattern.
 * This routine is used for case when expression has integral type and jump opetators.
 * Accepts:
 *      tX = x                        (OP_COPY)
 *      tY = y                        (OP_COPY)
 *      tZ = if tX == tY goto L ||    (OP_CJUMPE)
 *      tZ = if tX != tY goto L ||    (OP_CJUMPNE)
 *      tZ = if tX < tY goto L  ||    (OP_CJUMPLT)
 *      tZ = if tX > tY goto L  ||    (OP_CJUMPGT)
 *      tZ = if tX <= tY goto L ||    (OP_CJUMPLTE)
 *      tZ = if tX >= tY goto L       (OP_CJUMPGTE)
 *  Optimization:
 *      goto L || (OP_UJUMP) (in case when constant condition is true)
 *      ...       (OP_NONE)  (sets as OP_NONE in case when constant condition is true)
 * 
 * @param index Index of first quad that triggered pattern routine.
 * @return Number of quads (3) skipped by this optimizing function in initial quad set.
 */
nnc_static nnc_u64 nnc_3a_opt_binary_iconst_branch_fold(nnc_u64 index) {
    const nnc_3a_quad* lop_quad = &unopt[index];
    const nnc_3a_quad* rop_quad = &unopt[index+1];
    const nnc_3a_quad* res_quad = &unopt[index+2];
    nnc_u64 lval = lop_quad->arg1.exact.iconst.iconst;
    nnc_u64 rval = rop_quad->arg1.exact.iconst.iconst;
    nnc_3a_quad opt_quad = {
        .op = OP_NONE, .res = res_quad->res
    };
    switch (res_quad->op) {
        case OP_CJUMPE:     lval = lval == rval; break;
        case OP_CJUMPNE:    lval = lval != rval; break;
        case OP_CJUMPLT:    lval = lval < rval;  break;
        case OP_CJUMPGT:    lval = lval > rval;  break;
        case OP_CJUMPLTE:   lval = lval <= rval; break;
        case OP_CJUMPGTE:   lval = lval >= rval; break;
        default: nnc_abort_no_ctx("nnc_3a_opt_binary_iconst_branch_fold: unknown res_quad->op."); 
    }
    if (lval == 1) {
        opt_quad.op = OP_UJUMP;
        buf_add(opt, opt_quad);
    }
    return 3;
}

/**
 * @brief Optimizes `OPT_BINARY_CONST_FOLD` pattern.
 * This routine is used for case when expression has integral type.
 * It also handles jump operators by calling `nnc_3a_opt_binary_iconst_branch_fold`.
 * Accepts:
 *      tX = x                        
 *      tY = y                        
 *      tZ = tX binOp tY
 *  Optimization:
 *      tZ = const
 * 
 * @param index Index of first quad that triggered pattern routine.
 * @return Number of quads (3) skipped by this optimizing function in initial quad set.
 */
nnc_static nnc_u64 nnc_3a_opt_binary_iconst_fold(nnc_u64 index) {
    const nnc_3a_quad* lop_quad = &unopt[index];
    const nnc_3a_quad* rop_quad = &unopt[index+1];
    const nnc_3a_quad* bin_op_quad = &unopt[index+2];
    const nnc_type* bin_op_type = bin_op_quad->res.type;
    nnc_u64 lval = lop_quad->arg1.exact.iconst.iconst;
    nnc_u64 rval = rop_quad->arg1.exact.iconst.iconst;
    switch (bin_op_quad->op) {
        case OP_ADD:    lval +=  rval; break;
        case OP_SUB:    lval -=  rval; break;
        case OP_MUL:    lval *=  rval; break;
        case OP_DIV:    lval /=  rval; break;
        case OP_MOD:    lval %=  rval; break;
        case OP_SHR:    lval >>= rval; break;
        case OP_SHL:    lval <<= rval; break;
        case OP_BW_OR:  lval |=  rval; break;
        case OP_BW_AND: lval &=  rval; break;
        case OP_BW_XOR: lval ^=  rval; break;
        case OP_CJUMPE:
        case OP_CJUMPNE:
        case OP_CJUMPLT:
        case OP_CJUMPGT:
        case OP_CJUMPLTE:
        case OP_CJUMPGTE: {
            return nnc_3a_opt_binary_iconst_branch_fold(index);
        }
        default: nnc_abort_no_ctx("nnc_3a_opt_binary_iconst_fold: unknown op.");
    }
    nnc_3a_quad opt_quad = {
        .op = OP_COPY, .res = bin_op_quad->res, .arg1 = nnc_3a_mki2(lval, bin_op_type)
    };
    buf_add(opt, opt_quad);
    return 3;
}

/**
 * @brief Optimizes `OPT_BINARY_CONST_FOLD` pattern for both integral and float expression types.
 *  For more information about optimization see `nnc_3a_opt_binary_fconst_fold` and
 *  `nnc_3a_opt_binary_iconst_fold` functions.
 * @param index Index of first quad that triggered pattern routine. 
 * @return Number of quads (2) skipped by this optimizing function in initial quad set.
 */
nnc_static nnc_u64 nnc_3a_opt_binary_const_fold(nnc_u64 index) {
    const nnc_3a_quad* bin_op_quad = &unopt[index+2];
    const nnc_type* bin_op_type = bin_op_quad->res.type;
    if (nnc_real_type(bin_op_type)) {
        return nnc_3a_opt_binary_fconst_fold(index);
    }
    if (nnc_integral_type(bin_op_type)) {
        return nnc_3a_opt_binary_iconst_fold(index);
    }
    nnc_abort_no_ctx("nnc_3a_opt_binary_const_fold: unknown result type.\n");
    return 1;
}

/**
 * @brief Optimizes `OPT_INDEX_CONST_FOLD` pattern.
 * Accepts:
 *      tX = const1
 *      tZ = tX binOp const2
 * Optimization:
 *      tZ = const3
 * @param index Index of first quad that triggered pattern routine. 
 * @return Number of quads (2) skipped by this optimizing function in initial quad set.
 */
nnc_static nnc_u64 nnc_3a_opt_index_const_fold(nnc_u64 index) {
    const nnc_3a_quad* quad1 = &unopt[index];
    const nnc_3a_quad* quad2 = &unopt[index+1];
    nnc_u64 lval = quad1->arg1.exact.iconst.iconst;
    nnc_u64 rval = quad2->arg2.exact.iconst.iconst;
    switch (quad2->op) {
        case OP_ADD: lval += rval; break;
        case OP_MUL: lval *= rval; break;
        default: nnc_abort_no_ctx("nnc_3a_opt_index_const_fold: unknown op.");
    }
    nnc_3a_quad opt_quad = {
        .op = OP_COPY, .res = quad2->res, .arg1 = nnc_3a_mki3(lval)
    };
    buf_add(opt, opt_quad);
    return 2;
}

/**
 * @brief Optimizes `OPT_ALG_MUL_ZERO`, `OPT_ALG_ADD_ZERO`, `OPT_ALG_MUL_ONE` or `OPT_ALG_MUL_POW_TWO` pattern.
 * @param index Index of first quad that triggered pattern routine.
 * Accepts (for OPT_ALG_MUL_ZERO):
 *      tX = x (or 0)
 *      tY = 0 (or x)
 *      tZ = tX * tY
 * Optimization:
 *      tZ = 0
 * 
 * Accepts (for OPT_ALG_ADD_ZERO):
 *      tX = x (or 0)
 *      tY = 0 (or x)
 *      tZ = tX + tY
 * Optimization:
 *      tZ = tX (or tY)
 * 
 * Accepts (for OPT_ALG_MUL_ONE):
 *      tX = x (or 1)
 *      tY = 1 (or x)
 *      tZ = tX * tY
 * Optimization:
 *      tZ = tX (or tY)     
 * 
 * Accepts (for OPT_ALG_MUL_POW_TWO):
 *      tX = x (or 2^n)
 *      tY = 2^n (or x)
 *      tZ = tX * tY
 * Optimization:
 *      tZ = tX (or tY) << n
 * 
 * @param pat Concrete pattern.
 * @return Number of quads (3) skipped by this optimizing function in initial quad set.
 */
nnc_static nnc_u64 nnc_3a_opt_algebraic(nnc_u64 index, nnc_3a_peep_pattern pat) {
    const nnc_3a_quad* quad1 = &unopt[index];
    const nnc_3a_quad* quad2 = &unopt[index+1]; 
    const nnc_3a_quad* quad3 = &unopt[index+2];
    nnc_3a_quad opt_quad = (nnc_3a_quad){
        .op = OP_COPY, .res = quad3->res
    };
    nnc_3a_addr opt_const_arg = quad2->arg1;
    nnc_3a_addr opt_non_const_arg = quad1->arg1;
    if (quad1->arg1.kind == ADDR_ICONST) {
        opt_const_arg = quad1->arg1; 
        opt_non_const_arg = quad2->arg1;
    } 
    switch (pat) {
        case OPT_ALG_MUL_ONE:
        case OPT_ALG_ADD_ZERO: {
            opt_quad.arg1 = opt_non_const_arg;
            break;
        }
        case OPT_ALG_MUL_ZERO: {
            opt_quad.arg1 = nnc_3a_mki3(0); 
            break;
        }
        case OPT_ALG_MUL_POW_TWO: {
            nnc_i32 pow = 0;
            nnc_u64 val = opt_const_arg.exact.iconst.iconst;
            for (; val != 0; pow++) {
                val /= 2;
            }
            opt_quad.op = OP_SHL;
            opt_quad.arg1 = opt_non_const_arg;
            opt_quad.arg2 = nnc_3a_mki3(pow-1);
            break;
        }
        default: nnc_abort_no_ctx("nnc_3a_opt_red_alg: unknown search pattern."); 
    }
    buf_add(opt, opt_quad);
    return 3;
}

/**
 * @brief Performs no optimization.
 * @param index Index of first quad that triggered pattern routine.
 * @return Number of quads (1) skipped by this optimizing function in initial quad set.
 */
nnc_static nnc_u64 nnc_3a_opt_none(nnc_u64 index) {
    const nnc_3a_quad* quad = &unopt[index];
    buf_add(opt, *quad);
    return 1;
}

/**
 * @brief Checks for different pattern triggers.
 * @param index Index of the current quad.
 * @return `OPT_NONE` or some other optimization pattern.
 */
nnc_static nnc_3a_peep_pattern nnc_3a_search_peep_pattern(nnc_u64 index) {
    switch (unopt[index].op) {
        case OP_REF:  return nnc_3a_op_ref_pat(index);
        case OP_COPY: return nnc_3a_op_copy_pat(index);
        default: break;
    }
    return OPT_NONE;
}

/**
 * @brief Optimizes pattern found at `index` quad.
 * @param index Index of the current quad.
 * @return Number of quads skipped by this optimizing function in initial quad set.
 */
nnc_static nnc_u64 nnc_3a_optimize_peep(nnc_u64 index) {
    nnc_3a_peep_pattern pat = nnc_3a_search_peep_pattern(index);
    switch (pat) {
        /* Redundant cross operator optimizations */
        case OPT_REF_SUBST:            return nnc_3a_opt_ref_subst(index);
        case OPT_COPY_UNARY_SUBST:     return nnc_3a_opt_unary_subst(index);
        case OPT_COPY_UNARY_EX_SUBST:  return nnc_3a_opt_unary_ex_subst(index);
        case OPT_COPY_BINARY_SUBST:    return nnc_3a_opt_binary_subst(index, pat);
        case OPT_COPY_BINARY_L_SUBST:  return nnc_3a_opt_binary_subst(index, pat);
        case OPT_COPY_BINARY_R_SUBST:  return nnc_3a_opt_binary_subst(index, pat);
        /* Constant folding optimizations */
        case OPT_UNARY_CONST_FOLD:     return nnc_3a_opt_unary_const_fold(index);
        case OPT_BINARY_CONST_FOLD:    return nnc_3a_opt_binary_const_fold(index);
        case OPT_INDEX_CONST_FOLD:     return nnc_3a_opt_index_const_fold(index);
        /* Algebraic optimizations */
        case OPT_ALG_MUL_ONE:          return nnc_3a_opt_algebraic(index, pat);
        case OPT_ALG_ADD_ZERO:         return nnc_3a_opt_algebraic(index, pat);
        case OPT_ALG_MUL_ZERO:         return nnc_3a_opt_algebraic(index, pat);
        case OPT_ALG_MUL_POW_TWO:      return nnc_3a_opt_algebraic(index, pat);
        /* No optimization */
        case OPT_NONE:                 return nnc_3a_opt_none(index);
        default: nnc_abort_no_ctx("nnc_3a_optimize_peep: unknown search pattern."); 
    }
    return 1;
}

/**
 * @brief Iterator for `nnc_map_iter` function.
 */
nnc_static void nnc_3a_unused_cgts_iter(nnc_map_key key, nnc_map_val val) {
    nnc_3a_quad* unused_quad = val;
    unused_quad->op = OP_NONE;
}

/**
 * @brief Erases unused CGTs from optimized quad set.
 * @return Amount of quads removed.
 */
nnc_static nnc_u64 nnc_3a_erase_dead_cgts() {
    nnc_u64 size = buf_len(opt);
    _map_(nnc_i32, nnc_3a_quad*) unused = map_init_with(size);
    for (nnc_u64 i = 0; i < size; i++) {
        const nnc_3a_quad* quad = &opt[i];
        /*
            If quad's result is CGT and it is
            not stored in `unused` map, put it.
        */
        if (quad->res.kind == ADDR_CGT) {
            if (!map_has(unused, quad->res.exact.cgt)) {
                map_put(unused, quad->res.exact.cgt, quad);
            }
        }
        /* 
            This switch-case sets dependecy
            list for each type of operator,
            and then checks if any dependency is
            stored in `unused` map. If so, removes
            it from the map, othewise leaves it for future.
        */
        nnc_3a_addr deps[2] = { 0 };
        switch (quad->op) {
            /* Ignored operators */
            case OP_REF:
            case OP_NONE:
            case OP_RETP:
            case OP_FCALL:
            case OP_PCALL:
            case OP_UJUMP: {
                break;
            }
            /* Other operators */
            case OP_DEREF_COPY: {
                deps[0] = quad->res;
                deps[1] = quad->arg1;
                break;
            }
            /* Jump operators */
            case OP_CJUMPT:
            case OP_CJUMPF: {
                deps[0] = quad->arg1;
                break;
            }
            case OP_CJUMPE:
            case OP_CJUMPNE:
            case OP_CJUMPGT:
            case OP_CJUMPLT:
            case OP_CJUMPGTE:
            case OP_CJUMPLTE: {
                deps[0] = quad->arg1;
                deps[1] = quad->arg2;
                break;
            }
            /* Unary operators + OP_COPY, OP_DEREF, OP_ARG, OP_RETF */
            case OP_ARG:
            case OP_RETF:
            case OP_COPY:
            case OP_CAST:
            case OP_PLUS:
            case OP_MINUS:
            case OP_DEREF:
            case OP_BW_NOT: {
                deps[0] = quad->arg1; 
                break;
            }
            /* Binary operators */
            case OP_ADD: 
            case OP_SUB:
            case OP_MUL: 
            case OP_DIV:
            case OP_MOD: 
            case OP_SHR:
            case OP_SHL:
            case OP_BW_OR:
            case OP_BW_AND:
            case OP_BW_XOR: {
                deps[0] = quad->arg1;
                deps[1] = quad->arg2;
                break;
            }
            default: break;
        }
        for (nnc_i32 j = 0; j < sizeof(deps)/sizeof(nnc_3a_addr); j++) {
            if (deps[j].kind == ADDR_CGT) {
                nnc_3a_cgt cgt = deps[j].exact.cgt;
                if (map_has(unused, cgt)) {
                    map_pop(unused, cgt);
                }
            }
        }
    }
    /*
        If CGT is left in `unused` map
        after filtering, set their quads
        as OP_NONE, and remove from resulting list.
    */
    nnc_map_iter(unused, nnc_3a_unused_cgts_iter);
    _vec_(nnc_3a_quad) used = NULL;
    for (nnc_u64 i = 0; i < size; i++) {
        if (opt[i].op != OP_NONE || opt[i].label != 0) {
            buf_add(used, opt[i]);
        }
    }
    buf_free(opt);
    opt = used;
    map_fini(unused);
    return size - buf_len(used);
}

/**
 * @brief Zips label quads with next non-label quad.
 * @return Amount of quads zipped.
 */
nnc_static nnc_u64 nnc_3a_zip_quads() {
    nnc_u64 size = buf_len(opt);
    _vec_(nnc_3a_quad) zipped = NULL;
    for (nnc_u64 i = 0; i < size; i++) {
        const nnc_3a_quad* quad = &opt[i];
        if (quad->op == OP_NONE && quad->label != 0) {
            if (i+1 < size && opt[i+1].label == 0) {
                opt[i+1].label = quad->label;
                continue;
            }
        }
        buf_add(zipped, *quad);
    }
    buf_free(opt);
    opt = zipped;
    return size - buf_len(zipped);        
}

/**
 * @brief Represents single peep optimizer pass.
 * @return Amount of quads removed.
 */
nnc_static nnc_u64 nnc_3a_optimization_pass() {
    nnc_u64 i = 0;
    nnc_u64 len = buf_len(unopt);
    while (i < len) {
        i += nnc_3a_optimize_peep(i);
    }
    return len - buf_len(opt);
}

extern void nnc_dump_3a_quads(FILE* to, const nnc_3a_quad* quads);

/**
 * @brief Shows quad set at some optimization pass.
 * @param pass Pass' number.
 * @param quads Quad set at this pass.
 */
nnc_static void nnc_3a_show_pass(nnc_i32 pass, nnc_3a_quad* quads) {
    #if _NNC_ENABLE_PASS_LOGGING
        printf("\n################PASS%d################\n", pass);
        nnc_dump_3a_quads(stderr, quads);
    #endif
}

/**
 * @brief Performs peephole optimization over specified quad set.
 * @param quads Quad set that must be optimized.
 * @param stat Out statistics of this optimization routine.
 * @return Optimized quad set.
 */
_vec_(nnc_3a_quad) nnc_3a_optimize(_vec_(nnc_3a_quad) quads, nnc_3a_opt_stat* stat) {
    opt = NULL, unopt = NULL;
    nnc_i32 pass = 0;
    nnc_u64 reduced = 0;
    nnc_u64 len = buf_len(quads);
    for (; len != 0; pass++) {
        if (unopt != NULL) {
            buf_free(unopt);
        }
        unopt = opt == NULL ? quads : opt;
        opt = NULL;
        nnc_3a_show_pass(pass, unopt);
        nnc_u64 reduced_at_pass = nnc_3a_optimization_pass(); 
        if (reduced_at_pass == 0) {
            break;
        }
        reduced += reduced_at_pass;
    }
    if (unopt != NULL) {
        buf_free(unopt);
    }
    reduced += nnc_3a_erase_dead_cgts();
    reduced += nnc_3a_zip_quads();
    if (stat != NULL) {
        stat->reduced += reduced;
        stat->percent = stat->reduced / (nnc_f32)stat->initial;
        if (stat->initial == 0) {
            stat->percent = 0.0;
        }
    }
    return opt;
}

/**
 * @brief Performs peephole optimization over data segment quad set.
 * @param data Data segment that must be optimized.
 * @return Optimized data segment.
 */
nnc_3a_data nnc_3a_optimize_data(nnc_3a_data data) {
    nnc_3a_opt_stat stat = {0};
    if (buf_len(data.quads) != 0) {
    #if _NNC_ENABLE_OPTIMIZATIONS
        data.quads = nnc_3a_optimize(data.quads, &stat);
        data.stat = stat;
    #endif
        _vec_(nnc_3a_basic) blocks = nnc_3a_get_blocks(&data);
        data.cfg = nnc_3a_get_cfg(blocks);
    }
    return data;
}