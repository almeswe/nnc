#include "nnc_3a.h"

nnc_3a_quad_set* target_set = NULL;
nnc_3a_quad_set* target_opt_set = NULL;

typedef enum _nnc_3a_peep_pattern {
    OPT_RED_CROSS_TEMP_COPY,
    OPT_RED_CROSS_TEMP_REF,
    OPT_UNARY_CONST_FOLD,
    OPT_BINARY_CONST_FOLD,
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

nnc_static nnc_bool nnc_3a_unary_op(nnc_3a_op_kind op) {
    switch (op) {
        case OP_PLUS:
        case OP_MINUS:
        case OP_BW_NOT:
            return true;
        default: break;
    }
    return false;    
}

nnc_static nnc_bool nnc_3a_binary_op(nnc_3a_op_kind op) {
    switch (op) {
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
        case OP_MOD:
        case OP_SHR:
        case OP_SHL:
        case OP_BW_OR:
        case OP_BW_AND:
        case OP_BW_XOR:
            return true;
        default: break;
    }
    return false;
}

nnc_static nnc_3a_peep_pattern nnc_3a_copy_pattern(nnc_u64 index) {
    if (index >= buf_len(target_set->quads)) {
        return OPT_NONE;
    }
    const nnc_3a_quad* quad1 = &target_set->quads[index];
    const nnc_3a_quad* quad2 = &target_set->quads[index+1];
    //  tX = x
    //  tY = tX || 
    // *tY = tX
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
    // tX = const1
    // tZ = unOp   tX ||
    // tz = (type) tX
    if (quad1->res.kind == ADDR_CGT) {
        if (quad1->arg1.kind == ADDR_ICONST || 
            quad1->arg1.kind == ADDR_FCONST) {
            if (nnc_3a_unary_op(quad2->op) || quad2->op == OP_CAST) {
                if (quad1->res.exact.cgt == quad2->arg1.exact.cgt) {
                    return OPT_UNARY_CONST_FOLD;
                }
            }
        }
    }
    // tX = const1
    // tY = const2
    // tZ = tX binOp tY
    if (quad1->res.kind == ADDR_CGT &&
        quad2->res.kind == ADDR_CGT) {
        if ((quad1->arg1.kind == ADDR_ICONST || 
             quad1->arg1.kind == ADDR_FCONST) &&
            (quad2->arg1.kind == ADDR_ICONST ||
             quad2->arg1.kind == ADDR_FCONST)) {
            const nnc_3a_quad* quad3 = NULL;
            if (index + 2 < buf_len(target_set->quads)) {
                quad3 = &target_set->quads[index+2];
                if (nnc_3a_binary_op(quad3->op)) {
                    if (quad3->arg1.exact.cgt == quad1->res.exact.cgt &&
                        quad3->arg2.exact.cgt == quad2->res.exact.cgt) {
                        return OPT_BINARY_CONST_FOLD;
                    }
                }
            }
        }
    }
    return OPT_NONE;
}

nnc_static nnc_3a_peep_pattern nnc_3a_search_peep_pattern(nnc_u64 index) {
    switch (target_set->quads[index].op) {
        case OP_REF:  return nnc_3a_ref_pattern(index);
        case OP_COPY: return nnc_3a_copy_pattern(index);
        default: break;
    }
    return OPT_NONE;
}

nnc_static nnc_u64 nnc_3a_opt_none(nnc_u64 index) {
    const nnc_3a_quad* quad = &target_set->quads[index];
    buf_add(target_opt_set->quads, *quad);
    return 1;
}

nnc_static nnc_u64 nnc_3a_opt_unary_fconst_fold(nnc_u64 index) {
    const nnc_3a_quad* op_quad = &target_set->quads[index];
    const nnc_3a_quad* un_op_quad = &target_set->quads[index+1];
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
    buf_add(target_opt_set->quads, opt_quad);
    return 2;
}

nnc_static nnc_u64 nnc_3a_opt_unary_iconst_fold(nnc_u64 index) {
    const nnc_3a_quad* op_quad = &target_set->quads[index];
    const nnc_3a_quad* un_op_quad = &target_set->quads[index+1];
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
    buf_add(target_opt_set->quads, opt_quad);
    return 2;
}

nnc_static nnc_u64 nnc_3a_opt_unary_const_fold(nnc_u64 index) {
    const nnc_3a_quad* un_op_quad = &target_set->quads[index+1];
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
    const nnc_3a_quad* lop_quad = &target_set->quads[index];
    const nnc_3a_quad* rop_quad = &target_set->quads[index+1];
    const nnc_3a_quad* bin_op_quad = &target_set->quads[index+2];
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
    buf_add(target_opt_set->quads, opt_quad);
    return 3;
}

nnc_static nnc_u64 nnc_3a_opt_binary_iconst_fold(nnc_u64 index) {
    const nnc_3a_quad* lop_quad = &target_set->quads[index];
    const nnc_3a_quad* rop_quad = &target_set->quads[index+1];
    const nnc_3a_quad* bin_op_quad = &target_set->quads[index+2];
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
    buf_add(target_opt_set->quads, opt_quad);
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
    const nnc_3a_quad* bin_op_quad = &target_set->quads[index+2];
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
        case OPT_UNARY_CONST_FOLD:    return nnc_3a_opt_unary_const_fold(index);
        case OPT_BINARY_CONST_FOLD:   return nnc_3a_opt_binary_const_fold(index);
        case OPT_RED_CROSS_TEMP_REF:  return nnc_3a_opt_red_cross_temp_ref(index);
        case OPT_RED_CROSS_TEMP_COPY: return nnc_3a_opt_red_cross_temp_copy(index);
    }
    assert(false);
}

nnc_static nnc_u64 nnc_3a_optimize_pass() {
    nnc_u64 i = 0;
    nnc_u64 len = buf_len(target_set->quads);
    while (i < len) {
        i += nnc_3a_optimize_peep(i);
    }
    return len - buf_len(target_opt_set->quads);
}

void nnc_3a_optimize(nnc_3a_quad_set* set, nnc_3a_quad_set* opt_set) {
    nnc_u64 reduced = -1;
    opt_set->name = set->name;
    for (nnc_i32 pass = 0; reduced != 0; pass++) {
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
        reduced = nnc_3a_optimize_pass();
        nnc_dispose(target_set->quads);
    }
    opt_set = target_opt_set;
}