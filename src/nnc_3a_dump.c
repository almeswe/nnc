#include "nnc_3a.h"
#include <stdio.h>

static FILE* stream = NULL;
static char buf[128] = {0};

#define dump_3a(fmt, ...) fprintf(stream, fmt, __VA_ARGS__)

nnc_static const char* nnc_dump_3a_op(nnc_3a_op_kind op) {
    static const char* op_str[] = {
        [OP_ADD]    = "+",
        [OP_SUB]    = "-",
        [OP_MUL]    = "*",
        [OP_DIV]    = "/",
        [OP_MOD]    = "%", 
        [OP_SHR]    = ">>",
        [OP_SHL]    = "<<",
        [OP_LT]     = "<", 
        [OP_GT]     = ">", 
        [OP_LTE]    = "<=", 
        [OP_GTE]    = ">=",
        [OP_EQ]     = "==",
        [OP_NEQ]    = "!=",
        [OP_BW_AND] = "&",
        [OP_BW_XOR] = "^",
        [OP_BW_OR]  = "|",
        [OP_AND]    = "&&",
        [OP_OR]     = "||",
        [OP_MINUS]  = "-"
    };
    return op_str[op];
}

nnc_static void nnc_dump_3a_cgt(const nnc_3a_addr* addr) {
    memset(buf, 0, sizeof buf);
    sprintf(buf, "t%lu", addr->exact.cgt);
}

nnc_static void nnc_dump_3a_none(const nnc_3a_addr* addr) {
    memset(buf, 0, sizeof buf);
}

nnc_static void nnc_dump_3a_name(const nnc_3a_addr* addr) {
    memset(buf, 0, sizeof buf);
    sprintf(buf, "%s", addr->exact.name.name);
}

nnc_static void nnc_dump_3a_iconst(const nnc_3a_addr* addr) {
    memset(buf, 0, sizeof buf);
    const nnc_3a_iconst* iconst = &addr->exact.iconst;
    if (iconst->is_signed) {
        sprintf(buf, "%ld", iconst->exact.d);
    }
    else {
        sprintf(buf, "%lu", iconst->exact.u);
    }
}

nnc_static void nnc_dump_3a_fconst(const nnc_3a_addr* addr) {
    memset(buf, 0, sizeof buf);
    const nnc_3a_fconst* fconst = &addr->exact.fconst;
    sprintf(buf, "%f", fconst->exact);
}

nnc_static const char* nnc_dump_3a_addr(const nnc_3a_addr* addr) {
    switch (addr->kind) {
        case ADDR_CGT:    nnc_dump_3a_cgt(addr); break;
        case ADDR_NONE:   nnc_dump_3a_none(addr); break;
        case ADDR_NAME:   nnc_dump_3a_name(addr); break;
        case ADDR_ICONST: nnc_dump_3a_iconst(addr); break;
        case ADDR_FCONST: nnc_dump_3a_fconst(addr); break;
    }
    return buf;
}

nnc_static void nnc_dump_3a_binary(const nnc_3a_quad* quad) {
    dump_3a("   %6s =", nnc_dump_3a_addr(&quad->res));
    dump_3a(" %s", nnc_dump_3a_addr(&quad->arg1));
    dump_3a(" %s", nnc_dump_3a_op(quad->op));
    dump_3a(" %s\n", nnc_dump_3a_addr(&quad->arg2));
}

nnc_static void nnc_dump_3a_unary(const nnc_3a_quad* quad) {
    dump_3a("   %6s =", nnc_dump_3a_addr(&quad->res));
    dump_3a(" %s", nnc_dump_3a_op(quad->op));
    dump_3a(" %s\n", nnc_dump_3a_addr(&quad->arg1));
}

nnc_static void nnc_dump_3a_copy(const nnc_3a_quad* quad) {
    dump_3a("   %6s =", nnc_dump_3a_addr(&quad->res));
    dump_3a(" %s\n", nnc_dump_3a_addr(&quad->arg1));
}

nnc_static void nnc_dump_3a_quad(const nnc_3a_quad* quad) {
    switch (quad->op) {
        case OP_ADD:    case OP_SUB:    case OP_MUL:
        case OP_DIV:    case OP_MOD:    case OP_SHR:
        case OP_SHL:    case OP_LTE:    case OP_GTE:
        case OP_LT:     case OP_GT:     case OP_EQ:
        case OP_OR:     case OP_NEQ:    case OP_AND:
        case OP_BW_AND: case OP_BW_XOR: case OP_BW_OR:
            nnc_dump_3a_binary(quad); break;
        case OP_MINUS: nnc_dump_3a_unary(quad);  break;
        case OP_COPY:  nnc_dump_3a_copy(quad);   break;
        default: nnc_abort_no_ctx("nnc_dump_3a_quad: unimplemented case met.");
    }
}

void nnc_dump_3a(FILE* to, const nnc_3a_quad* quads) {
    stream = to;
    for (nnc_u64 i = 0; i < buf_len(quads); i++) {
        nnc_dump_3a_quad(&quads[i]);
    }
}