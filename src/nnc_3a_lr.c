#include "nnc_3a.h"

nnc_static void nnc_3a_lr_put_cgt(nnc_map* lr_cgt, const nnc_3a_addr* cgt, nnc_i32 pos) {
    assert(cgt->kind == ADDR_CGT);
    nnc_3a_lr* lr = NULL;
    nnc_map_key key = cgt->exact.cgt;
    if (!map_has(lr_cgt, key)) {
        lr = nnc_alloc(sizeof(nnc_3a_lr));
        lr->starts = lr->ends = pos;
        map_put(lr_cgt, key, lr);
    }
    else {
        lr = (nnc_3a_lr*)map_get(lr_cgt, key);
        assert(lr != NULL);
        lr->ends = pos;
    }
}

nnc_static void nnc_3a_lr_put_var(nnc_map* lr_var, const nnc_3a_addr* var, nnc_i32 pos) {
    assert(var->kind == ADDR_NAME);
    nnc_3a_lr* lr = NULL;
    //todo: maybe i need take to account nesting?
    if (!map_has_s(lr_var, var->exact.name.name)) {
        lr = nnc_alloc(sizeof(nnc_3a_lr));
        lr->starts = lr->ends = pos;
        map_put_s(lr_var, var->exact.name.name, lr);
    }
    else {
        lr = (nnc_3a_lr*)map_get_s(lr_var, var->exact.name.name);
        assert(lr != NULL);
        lr->ends = pos;
    }
}

nnc_static void nnc_3a_lr_put_addr(nnc_3a_unit* unit, const nnc_3a_addr* addr, nnc_i32 pos) {
    switch (addr->kind) {
        case ADDR_CGT:  nnc_3a_lr_put_cgt(unit->lr_cgt, addr, pos); break;
        case ADDR_NAME: nnc_3a_lr_put_var(unit->lr_var, addr, pos); break;
        default: break;
    }
}

void nnc_3a_lr_process_quad(nnc_3a_unit* unit, nnc_i32 pos) {
    const nnc_3a_quad* quad = &unit->quads[pos];
    nnc_3a_lr_put_addr(unit, &quad->res, pos);
    // process liverange of certain arguments, depending on quad operator.
    switch (quad->op) {
        /* do not consider */
        case OP_NONE:
        case OP_RETP:
        case OP_UJUMP:
        case OP_FCALL:
        case OP_PCALL: {
            break;
        }
        /* unary operators */
        case OP_UNARY:
        /* unary conditional operators */
        case OP_UNARY_JUMP:
        /* other operators, that also use only `arg1` */
        case OP_ARG:
        case OP_REF:
        case OP_RETF:
        case OP_COPY:
        case OP_CAST:
        case OP_DEREF:
        case OP_DEREF_COPY: {
            nnc_3a_lr_put_addr(unit, &quad->arg1, pos);
            break;
        }
        /* binary operators */
        case OP_BINARY:
        /* binary conditional operators */
        case OP_BINARY_JUMP: {
            nnc_3a_lr_put_addr(unit, &quad->arg1, pos);
            nnc_3a_lr_put_addr(unit, &quad->arg2, pos);
            break;
        }
        default: nnc_abort_no_ctx("nnc_3a_lr_process_quad: unknown `block->quads[pos].op`");
    }
}

void nnc_3a_make_lr_for_unit(nnc_3a_unit* unit) {
    for (nnc_u64 i = 0; i < buf_len(unit->quads); i++) {
        nnc_3a_lr_process_quad(unit, i);
    }
}