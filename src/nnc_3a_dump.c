#include "nnc_3a.h"
#include <stdio.h>

static FILE* stream = NULL;
static char buf[128] = {0};

#define dump_indent "   "
#define dump_3a(fmt, ...) fprintf(stream, fmt, __VA_ARGS__)

static const char* op_str[] = {
    /*  binary operators */
    [OP_LT]     = "<", 
    [OP_GT]     = ">", 
    [OP_EQ]     = "==",
    [OP_OR]     = "||",
    [OP_ADD]    = "+",
    [OP_SUB]    = "-",
    [OP_MUL]    = "*",
    [OP_DIV]    = "/",
    [OP_MOD]    = "%", 
    [OP_SHR]    = ">>",
    [OP_SHL]    = "<<",
    [OP_LTE]    = "<=", 
    [OP_GTE]    = ">=",
    [OP_NEQ]    = "!=",
    [OP_AND]    = "&&",
    [OP_BW_AND] = "&",
    [OP_BW_XOR] = "^",
    [OP_BW_OR]  = "|",
    /*  unary operators  */
    [OP_NOT]    = "!",
    [OP_PLUS]   = "+",
    [OP_MINUS]  = "-",
    [OP_BW_NOT] = "~"
    /* ***************** */
};

nnc_static const char* nnc_dump_3a_op(nnc_3a_op_kind op) {
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
    sprintf(buf, "%lu", iconst->iconst);
}

nnc_static void nnc_dump_3a_fconst(const nnc_3a_addr* addr) {
    memset(buf, 0, sizeof buf);
    const nnc_3a_fconst* fconst = &addr->exact.fconst;
    sprintf(buf, "%f", fconst->fconst);
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
    dump_3a(dump_indent "%6s =", nnc_dump_3a_addr(&quad->res));
    dump_3a(" %s", nnc_dump_3a_addr(&quad->arg1));
    dump_3a(" %s", nnc_dump_3a_op(quad->op));
    dump_3a(" %s\n", nnc_dump_3a_addr(&quad->arg2));
}

nnc_static void nnc_dump_3a_unary(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "%6s =", nnc_dump_3a_addr(&quad->res));
    dump_3a(" %s", nnc_dump_3a_op(quad->op));
    dump_3a(" %s\n", nnc_dump_3a_addr(&quad->arg1));
}

nnc_static void nnc_dump_3a_arg(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "    arg %s\n", nnc_dump_3a_addr(&quad->arg1));
}

nnc_static void nnc_dump_3a_copy(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "%6s =", nnc_dump_3a_addr(&quad->res));
    dump_3a(" %s\n", nnc_dump_3a_addr(&quad->arg1));
}

nnc_static void nnc_dump_3a_index(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "%6s =", nnc_dump_3a_addr(&quad->res));
    dump_3a(" %s[", nnc_dump_3a_addr(&quad->arg1));
    dump_3a("%s]\n", nnc_dump_3a_addr(&quad->arg2));
}

nnc_static void nnc_dump_3a_fcall(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "%6s = call", nnc_dump_3a_addr(&quad->res));
    dump_3a(" %s,", nnc_dump_3a_addr(&quad->arg1));
    dump_3a(" %s\n", nnc_dump_3a_addr(&quad->arg2));
}

nnc_static void nnc_dump_3a_pcall(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "    call %s,", nnc_dump_3a_addr(&quad->arg1));
    dump_3a(" %s\n", nnc_dump_3a_addr(&quad->arg2));
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
        case OP_NOT:    case OP_BW_NOT:
        case OP_PLUS:   case OP_MINUS:
            nnc_dump_3a_unary(quad); break;
        case OP_ARG:   nnc_dump_3a_arg(quad);   break;
        case OP_COPY:  nnc_dump_3a_copy(quad);  break;
        case OP_INDEX: nnc_dump_3a_index(quad); break;
        case OP_FCALL: nnc_dump_3a_fcall(quad); break;
        case OP_PCALL: nnc_dump_3a_pcall(quad); break;
        default: nnc_abort_no_ctx("nnc_dump_3a_quad: unimplemented case met.");
    }
}

nnc_static void nnc_dump_3a_quads(const nnc_3a_quad* quads) {
    for (nnc_u64 i = 0; i < buf_len(quads); i++) {
        nnc_dump_3a_quad(&quads[i]);
    }
}

nnc_static void nnc_dump_3a_set(const nnc_3a_quad_set* set) {
    dump_3a("@_%s:\n", set->name);
    nnc_dump_3a_quads(set->quads);
}

void nnc_dump_3a(FILE* to, const nnc_3a_quad_set* sets) {
    stream = to;
    for (nnc_u64 i = 0; i < buf_len(sets); i++) {
        nnc_dump_3a_set(&sets[i]);
    }
}