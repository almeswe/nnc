#include "nnc_3a.h"

nnc_static _vec_(nnc_3a_quad) opt = NULL;
nnc_static _vec_(nnc_3a_quad) unopt = NULL;

extern void nnc_dump_3a_quads(FILE* to, const nnc_3a_quad* quads);

nnc_static nnc_3a_peep_pattern nnc_3a_ref_pattern(nnc_u64 index) {
    /*
    Pattern's Trigger:
        tX = &x            (OP_REF)

    Following part:    
       *tX = tZ            (OP_DEREF_COPY)
    */
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
                return OPT_CROSS_REF;
            }
        }
    }
    return OPT_NONE;
}

nnc_static nnc_3a_peep_pattern nnc_3a_op_copy_pat_part1(nnc_u64 index) {
    /*
    Pattern's Trigger:
        tX = x                   (OP_COPY)

    Following part:    
        tY = tX          ||   [1] (OP_COPY)
       *tY = tX          ||   [2] (OP_DEREF_COPY)
        if tX goto L     ||   [3] (OP_CJUMPT)
        if not tX goto L ||   [4] (OP_CJUMPF)
        arg tX           ||   [5] (OP_ARG)
        ret tX           ||   [6] (OP_RETF)

    Variable part (for cases [1] or [2])
        tZ = tX || (?)           (OP_COPY)
       *tZ = tX    (?)           (OP_DEREF_COPY)
    */
    const nnc_3a_quad* quad1 = &unopt[index];
    const nnc_3a_quad* quad2 = &unopt[index+1];
    const nnc_3a_quad* quad3 = NULL;
    if (index+2 < buf_len(unopt)) {
        quad3 = &unopt[index+2];
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
                    equals quad1's first argument.
                    This is `Variable part` from pattern above.
                */
                if (quad3 != NULL &&
                    quad3->arg1.kind == ADDR_CGT &&
                   (quad3->op == OP_COPY ||
                    quad3->op == OP_DEREF_COPY)) {
                    if (quad1->res.exact.cgt == 
                        quad3->arg1.exact.cgt) {
                        return OPT_CROSS_COPY3;
                    }
                }
            }
            case OP_ARG:
            case OP_RETF:
            case OP_CJUMPT:
            case OP_CJUMPF: {
                /*
                    This is `Following part` from pattern above.
                */
                return OPT_CROSS_COPY;              
            }
            default: break;
        }
    }
    return OPT_NONE;
}

nnc_static nnc_3a_peep_pattern nnc_3a_op_copy_pat_part2(nnc_u64 index) {
    /*
    Pattern's Trigger:
        tX = x                   (OP_COPY)

    Following part: (where x = const)
        tZ = unOp tX   ||        (Unary operator)
        tZ = (type) tX           (OP_CAST)
    */
    const nnc_3a_quad* quad1 = &unopt[index];
    const nnc_3a_quad* quad2 = &unopt[index+1];
    /*
        quad1's result must be CGT,
        and it's first argument must be constant.
    */
    if (quad1->res.kind == ADDR_CGT &&
       (quad1->arg1.kind == ADDR_ICONST || 
        quad1->arg1.kind == ADDR_FCONST)) {
        /*
            quad2 must be unary operator or OP_CAST
            This is `Following part` of pattern above.
        */
        switch (quad2->op) {
            case OP_CAST:
            case OP_PLUS:
            case OP_MINUS:
            case OP_BW_NOT: {
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

nnc_static nnc_3a_peep_pattern nnc_3a_op_copy_pat_part3(nnc_u64 index) {
    /*
    Pattern's Trigger:
        tX = x                  (OP_COPY)

    Following part: (where x = const)
                    (where y = const)
        tY = y                  (OP_COPY)
        tZ = tX binOp tY        (Binary operator)
    */
    const nnc_3a_quad* quad1 = &unopt[index];
    const nnc_3a_quad* quad2 = &unopt[index+1];
    const nnc_3a_quad* quad3 = NULL;
    if (index + 2 < buf_len(unopt)) {
        quad3 = &unopt[index+2];
    }
    if (quad3 == NULL) {
        return OPT_NONE;
    }
    /*
        Both quad1 and quad2 results must be ADDR_CGTs
        and their first arguments must be constant.
    */
    if (quad1->res.kind == ADDR_CGT &&
        quad2->res.kind == ADDR_CGT &&
       (quad1->arg1.kind == ADDR_ICONST  || 
        quad1->arg1.kind == ADDR_FCONST) &&
       (quad2->arg1.kind == ADDR_ICONST  ||
        quad2->arg1.kind == ADDR_FCONST)) {
        /*
            quad3 must be binary operator
            This is `Following part` of pattern above.
        */
        switch (quad3->op) {
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

nnc_static nnc_3a_peep_pattern nnc_3a_op_copy_pat_part4(nnc_u64 index) {
    /*
    This optimization targets index calculation.
    Pattern's Trigger:
        tX = x                  (OP_COPY)

    Following part: (where x  = const)
        tZ = tX binOp const     (Binary operator: OP_ADD, OP_MUL)
    */
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

nnc_static nnc_3a_peep_pattern nnc_3a_op_copy_pat_part5(nnc_u64 index) {
    /*
    This optimization targets redundant algebraic operations.
    Pattern's Trigger:
        tX = 0  ||                (OP_COPY)
        tX = x                    (OP_COPY)

    Following part:
        tY = tX binOp 0 ||        (Binary operator: *, +)
        tY = 0 binOp tX 
    */
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
        switch (quad3->op) {
            case OP_ADD:
            case OP_MUL: {
                if (quad1->arg1.exact.iconst.iconst == 0 ||
                    quad2->arg1.exact.iconst.iconst == 0) {
                    if ((quad1->res.exact.cgt == quad3->arg1.exact.cgt) ||
                        (quad2->res.exact.cgt == quad3->arg2.exact.cgt)) {
                        return quad3->op == OP_MUL ? 
                            OPT_ALG_MUL_ZERO : OPT_ALG_ADD_ZERO;
                    }
                }
                break;
            }
            default: break;
        }
    }
    return OPT_NONE;
}

nnc_static nnc_3a_peep_pattern nnc_3a_op_copy_pat_part6(nnc_u64 index) {
    /*
    This optimization targets redundant algebraic operations.
    Pattern's Trigger:
        tX = 1  ||                (OP_COPY)
        tX = 2^n                  (OP_COPY)
        tX = x                    (OP_COPY)

    Following part:
        tY = tX * 1   ||          (Binary operator: *)
        tY = 1 * tX   ||          (Binary operator: *)
        tY = 2^n * tX ||          (Binary operator: *)
        tY = tX * 2^n ||          (Binary operator: *)
    */
    const nnc_3a_quad* quad1 = &unopt[index];
    const nnc_3a_quad* quad2 = &unopt[index+1];
    const nnc_3a_quad* quad3 = NULL;
    if (index + 2 < buf_len(unopt)) {
        quad3 = &unopt[index+2];
    }
    if (quad3 == NULL) {
        return OPT_NONE;
    }
    if (quad3->op == OP_MUL &&
       (quad1->arg1.kind == ADDR_ICONST ||
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

nnc_static nnc_3a_peep_pattern nnc_3a_op_copy_pat(nnc_u64 index) {
    typedef nnc_3a_peep_pattern (pat_fn)(nnc_u64);
    static pat_fn* pat_fns[] = {
        nnc_3a_op_copy_pat_part1,
        nnc_3a_op_copy_pat_part2,
        nnc_3a_op_copy_pat_part3,
        nnc_3a_op_copy_pat_part4,
        nnc_3a_op_copy_pat_part5,
        nnc_3a_op_copy_pat_part6,
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

nnc_static nnc_u64 nnc_3a_opt_cross_ref(nnc_u64 index) {
    /*
        tX = &x
        *tX = Y
        --------
        x = Y
    */
    const nnc_3a_quad* quad1 = &unopt[index];
    const nnc_3a_quad* quad2 = &unopt[index+1];
    nnc_3a_quad opt_quad = {
        .op = OP_COPY, .res = quad1->arg1, .arg1 = quad2->arg1
    };
    buf_add(opt, opt_quad);
    return 2;
}

nnc_static nnc_u64 nnc_3a_opt_cross_copy(nnc_u64 index) {
    /*
        tX = x
        tY = tX
        -------
        tY = x
    */
    const nnc_3a_quad* quad1 = &unopt[index];
    const nnc_3a_quad* quad2 = &unopt[index+1];
    nnc_3a_quad opt_quad = *quad2;
    opt_quad.arg1 = quad1->arg1;
    buf_add(opt, opt_quad);
    return 2;
}

nnc_static nnc_u64 nnc_3a_opt_cross_copy3(nnc_u64 index) {
    /*
        tX = x
        tY = tX
        tZ = tX
        -------
        tY = x
        tZ = x
    */
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

nnc_static nnc_u64 nnc_3a_opt_unary_iconst_fold(nnc_u64 index) {
    const nnc_3a_quad* op_quad = &unopt[index];
    const nnc_3a_quad* un_op_quad = &unopt[index+1];
    const nnc_type* un_op_type = un_op_quad->res.type;
    nnc_u64 val = op_quad->arg1.exact.iconst.iconst;
    switch (un_op_quad->op) {
        case OP_PLUS:   break;
        case OP_MINUS:  val = -val; break;
        case OP_BW_NOT: val = ~val; break;
        case OP_CAST: {
            nnc_f64 fval = op_quad->arg1.exact.fconst.fconst;
            if (op_quad->arg1.kind == ADDR_ICONST) {
                fval = op_quad->arg1.exact.iconst.iconst;
            }
            switch (un_op_type->kind) {
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
        default: nnc_abort_no_ctx("nnc_3a_opt_unary_iconst_fold: unknown op.");
    }
    nnc_3a_quad opt_quad = {
        .op = OP_COPY, .res = un_op_quad->res, .arg1 = nnc_3a_mki2(val, un_op_type)
    };
    buf_add(opt, opt_quad);
    return 2;
}

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
        default: nnc_abort_no_ctx("nnc_3a_opt_binary_fconst_fold: unknown op.");
    }
    nnc_3a_quad opt_quad = {
        .op = OP_COPY, .res = bin_op_quad->res, .arg1 = nnc_3a_mkf2(lval, bin_op_type)
    };
    buf_add(opt, opt_quad);
    return 3;
}

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
        default: nnc_abort_no_ctx("nnc_3a_opt_binary_iconst_fold: unknown op.");
    }
    nnc_3a_quad opt_quad = {
        .op = OP_COPY, .res = bin_op_quad->res, .arg1 = nnc_3a_mki2(lval, bin_op_type)
    };
    buf_add(opt, opt_quad);
    return 3;
}

nnc_static nnc_u64 nnc_3a_opt_binary_const_fold(nnc_u64 index) {
    /*
        tX = const1
        tY = const2
        tZ = tX binOp tY
        ----------------
        tZ = const3
    */
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

nnc_static nnc_u64 nnc_3a_opt_index_const_fold(nnc_u64 index) {
    /*
        tX = const1
        tZ = tX binOp const2
        --------------------
        tZ = const3
    */
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

nnc_static nnc_u64 nnc_3a_opt_algebraic(nnc_u64 index, nnc_3a_peep_pattern pat) {
    /*
    1) OPT_ALG_MUL_ZERO
        tX = x (or 0)
        tY = 0 (or x)
        tZ = tX * tY
        -------------
        tZ = 0

    2) OPT_ALG_ADD_ZERO
        tX = x (or 0)
        tY = 0 (or x)
        tZ = tX + tY
        -------------
        tZ = tX (or tY)

    3) OPT_ALG_MUL_ONE
        tX = x (or 1)
        tY = 1 (or x)
        tZ = tX * tY
        -------------
        tZ = tX (or tY)

    4) OPT_ALG_MUL_POW_TWO
        tX = x (or 2^n)
        tY = 2^n (or x)
        tZ = tX * tY
        -------------
        tZ = tX (or tY) << n
    */
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

nnc_static nnc_u64 nnc_3a_opt_none(nnc_u64 index) {
    const nnc_3a_quad* quad = &unopt[index];
    buf_add(opt, *quad);
    return 1;
}

nnc_static nnc_3a_peep_pattern nnc_3a_search_peep_pattern(nnc_u64 index) {
    switch (unopt[index].op) {
        case OP_REF:  return nnc_3a_ref_pattern(index);
        case OP_COPY: return nnc_3a_op_copy_pat(index);
        default: break;
    }
    return OPT_NONE;
}

nnc_static nnc_u64 nnc_3a_optimize_peep(nnc_u64 index) {
    nnc_3a_peep_pattern pat = nnc_3a_search_peep_pattern(index);
    switch (pat) {
        /* Redundant cross operator optimizations */
        case OPT_CROSS_REF:            return nnc_3a_opt_cross_ref(index);
        case OPT_CROSS_COPY:           return nnc_3a_opt_cross_copy(index);
        case OPT_CROSS_COPY3:          return nnc_3a_opt_cross_copy3(index);
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

nnc_static void nnc_3a_unused_cgts_iter(nnc_map_key key, nnc_map_val val) {
    nnc_3a_quad* unused_quad = val;
    unused_quad->op = OP_NONE;
}

nnc_static nnc_u64 nnc_3a_clean_cgts() {
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

nnc_static nnc_u64 nnc_3a_merge_labels() {
    nnc_u64 size = buf_len(opt);
    _vec_(nnc_3a_quad) merged = NULL;
    for (nnc_u64 i = 0; i < size; i++) {
        const nnc_3a_quad* quad = &opt[i];
        if (quad->op == OP_NONE && quad->label != 0) {
            if (i+1 < size && opt[i+1].label == 0) {
                opt[i+1].label = quad->label;
                continue;
            }
        }
        buf_add(merged, *quad);
    }
    buf_free(opt);
    opt = merged;
    return size - buf_len(merged);
}

nnc_static nnc_u64 nnc_3a_optimization_pass() {
    nnc_u64 i = 0;
    nnc_u64 len = buf_len(unopt);
    while (i < len) {
        i += nnc_3a_optimize_peep(i);
    }
    return len - buf_len(opt);
}

nnc_static void nnc_3a_show_pass(nnc_i32 pass, nnc_3a_quad* quads) {
    #if _NNC_ENABLE_PASS_LOGGING
        printf("\n################PASS%d################\n", pass);
        nnc_dump_3a_quads(stderr, quads);
        //printf("pass %d reduced %lu\n", pass, reduced);
    #endif
}

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
    reduced += nnc_3a_clean_cgts();
    reduced += nnc_3a_merge_labels();
    if (stat != NULL) {
        *stat = (nnc_3a_opt_stat) {
            .passes = pass,
            .reduced = reduced,
            .percent = (nnc_i32)(((len == 0 ? 0 : reduced / (nnc_f32)len)) * 100)
        };
    }
    return opt;
}

nnc_3a_code nnc_3a_optimize_code(nnc_3a_code code) {
    for (nnc_u64 i = 0; i < buf_len(code); i++) {
        nnc_3a_opt_stat stat = {0};
        #if _NNC_ENABLE_PEEP_OPTIMIZATIONS
        code[i].quads = nnc_3a_optimize(code[i].quads, &stat);
        code[i].stat = stat;
        #endif
        code[i].blocks = nnc_3a_get_blocks(&code[i]);
    }
    return code;
}

nnc_3a_data nnc_3a_optimize_data(nnc_3a_data data) {
    nnc_3a_opt_stat stat = {0};
    #if _NNC_ENABLE_PEEP_OPTIMIZATIONS
    data.quads = nnc_3a_optimize(data.quads, &stat);
    data.stat = stat;
    #endif
    data.blocks = nnc_3a_get_blocks(&data);
    return data;
}