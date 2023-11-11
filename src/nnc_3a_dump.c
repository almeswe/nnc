#include "nnc_3a.h"
#include <stdio.h>

static FILE* stream = NULL;
static char buf[128] = {0};
static int pad = 10;

#define dump_indent "   "
#define dump_3a(fmt, ...) fprintf(stream, fmt, __VA_ARGS__)

static const char* op_str[] = {
    /*  binary operators */
    [OP_ADD]    = "+",
    [OP_SUB]    = "-",
    [OP_MUL]    = "*",
    [OP_DIV]    = "/",
    [OP_MOD]    = "%", 
    [OP_SHR]    = ">>",
    [OP_SHL]    = "<<",
    [OP_BW_AND] = "&",
    [OP_BW_XOR] = "^",
    [OP_BW_OR]  = "|",
    /*  unary operators  */
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
    sprintf(buf, "_%s", addr->exact.name.name);
}

nnc_static void nnc_dump_3a_sconst(const nnc_3a_addr* addr) {
    memset(buf, 0, sizeof buf);
    const nnc_3a_sconst* sconst = &addr->exact.sconst;
    sprintf(buf, "\"%s\"", sconst->sconst);
}

nnc_static void nnc_dump_3a_iconst(const nnc_3a_addr* addr) {
    memset(buf, 0, sizeof buf);
    const nnc_3a_iconst* iconst = &addr->exact.iconst;
    if (nnc_signed_type(addr->type)) {
        sprintf(buf, "%ld", iconst->sconst);
    }
    else {
        sprintf(buf, "%lu", iconst->iconst);
    }
}

nnc_static void nnc_dump_3a_fconst(const nnc_3a_addr* addr) {
    memset(buf, 0, sizeof buf);
    const nnc_3a_fconst* fconst = &addr->exact.fconst;
    sprintf(buf, "%f", fconst->fconst);
}

nnc_static char* nnc_dump_3a_type(const nnc_type* type) {
    memset(buf, 0, sizeof buf);
    sprintf(buf, "(%s)", nnc_type_tostr(type));
    return buf;
}

nnc_static const char* nnc_dump_3a_addr(const nnc_3a_addr* addr) {
    switch (addr->kind) {
        case ADDR_CGT:    nnc_dump_3a_cgt(addr);    break;
        case ADDR_NONE:   nnc_dump_3a_none(addr);   break;
        case ADDR_NAME:   nnc_dump_3a_name(addr);   break;
        case ADDR_SCONST: nnc_dump_3a_sconst(addr); break;
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

nnc_static void nnc_dump_3a_ref(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "%6s =", nnc_dump_3a_addr(&quad->res));
    dump_3a(" &%s\n", nnc_dump_3a_addr(&quad->arg1));
}

nnc_static void nnc_dump_3a_copy(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "%6s =", nnc_dump_3a_addr(&quad->res));
    dump_3a(" %s\n", nnc_dump_3a_addr(&quad->arg1));
}

nnc_static void nnc_dump_3a_cast(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "%6s =", nnc_dump_3a_addr(&quad->res));
    dump_3a(" %s", nnc_dump_3a_type(quad->res.type));
    dump_3a("%s\n", nnc_dump_3a_addr(&quad->arg1));
}

nnc_static void nnc_dump_3a_retf(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "%7s %s\n", "ret", nnc_dump_3a_addr(&quad->arg1));
}

nnc_static void nnc_dump_3a_retp(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "%7s\n", "ret");
}

nnc_static void nnc_dump_3a_deref(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "%6s =", nnc_dump_3a_addr(&quad->res));
    dump_3a(" *%s\n", nnc_dump_3a_addr(&quad->arg1));
}

nnc_static void nnc_dump_3a_fcall(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "%6s = call", nnc_dump_3a_addr(&quad->res));
    dump_3a(" @%s,", nnc_dump_3a_addr(&quad->arg1));
    dump_3a(" %s\n", nnc_dump_3a_addr(&quad->arg2));
}

nnc_static void nnc_dump_3a_pcall(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "    call @%s,", nnc_dump_3a_addr(&quad->arg1));
    dump_3a(" %s\n", nnc_dump_3a_addr(&quad->arg2));
}

nnc_static void nnc_dump_3a_ujump(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "    goto @%s\n", nnc_dump_3a_addr(&quad->res));
}

nnc_static void nnc_dump_3a_cjumpt(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "    if %s", nnc_dump_3a_addr(&quad->arg1));
    dump_3a(" goto @%s\n", nnc_dump_3a_addr(&quad->res));
}

nnc_static void nnc_dump_3a_cjumpf(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "    if not %s", nnc_dump_3a_addr(&quad->arg1));
    dump_3a(" goto @%s\n", nnc_dump_3a_addr(&quad->res));
}

nnc_static void nnc_dump_3a_cjumpe(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "    if %s ==", nnc_dump_3a_addr(&quad->arg1));
    dump_3a(" %s", nnc_dump_3a_addr(&quad->arg2));
    dump_3a(" goto @%s\n", nnc_dump_3a_addr(&quad->res));
}

nnc_static void nnc_dump_3a_cjumplt(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "    if %s <", nnc_dump_3a_addr(&quad->arg1));
    dump_3a(" %s", nnc_dump_3a_addr(&quad->arg2));
    dump_3a(" goto @%s\n", nnc_dump_3a_addr(&quad->res));
}

nnc_static void nnc_dump_3a_cjumpgt(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "    if %s >", nnc_dump_3a_addr(&quad->arg1));
    dump_3a(" %s", nnc_dump_3a_addr(&quad->arg2));
    dump_3a(" goto @%s\n", nnc_dump_3a_addr(&quad->res));
}

nnc_static void nnc_dump_3a_cjumpne(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "    if %s !=", nnc_dump_3a_addr(&quad->arg1));
    dump_3a(" %s", nnc_dump_3a_addr(&quad->arg2));
    dump_3a(" goto @%s\n", nnc_dump_3a_addr(&quad->res));
}

nnc_static void nnc_dump_3a_cjumplte(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "    if %s <=", nnc_dump_3a_addr(&quad->arg1));
    dump_3a(" %s", nnc_dump_3a_addr(&quad->arg2));
    dump_3a(" goto @%s\n", nnc_dump_3a_addr(&quad->res));
}

nnc_static void nnc_dump_3a_cjumpgte(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "    if %s >=", nnc_dump_3a_addr(&quad->arg1));
    dump_3a(" %s", nnc_dump_3a_addr(&quad->arg2));
    dump_3a(" goto @%s\n", nnc_dump_3a_addr(&quad->res));
}

nnc_static void nnc_dump_3a_deref_copy(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "%4s%s =", "*", nnc_dump_3a_addr(&quad->res));
    dump_3a(" %s\n", nnc_dump_3a_addr(&quad->arg1));
}

nnc_static void nnc_dump_3a_quad_type(const nnc_3a_quad* quad) {
    const char* repr = "";
    if (quad->op < OP_UJUMP || quad->op > OP_CJUMPNE) {
        repr = quad->res.type == NULL ? 
            "" : nnc_dump_3a_type(quad->res.type); 
    }
    pad = nnc_max((int)strlen(repr), pad);
    dump_3a("%*s", pad, repr);
}

nnc_static void nnc_dump_3a_quad(const nnc_3a_quad* quad) {
    if (quad->label > 0) {
        dump_3a(" @%u:\n", quad->label);
        return;
    }
    nnc_dump_3a_quad_type(quad);
    switch (quad->op) {
        case OP_ADD:    
        case OP_SUB:    case OP_MUL:
        case OP_DIV:    case OP_MOD:    
        case OP_SHR:    case OP_SHL:    
        case OP_BW_AND: case OP_BW_XOR: case OP_BW_OR:
            nnc_dump_3a_binary(quad); break;
        case OP_PLUS:
        case OP_MINUS:
        case OP_BW_NOT:
            nnc_dump_3a_unary(quad); break;
        case OP_ARG:        nnc_dump_3a_arg(quad);        break;
        case OP_REF:        nnc_dump_3a_ref(quad);        break;
        case OP_COPY:       nnc_dump_3a_copy(quad);       break;
        case OP_CAST:       nnc_dump_3a_cast(quad);       break;
        case OP_RETF:       nnc_dump_3a_retf(quad);       break;
        case OP_RETP:       nnc_dump_3a_retp(quad);       break;
        case OP_DEREF:      nnc_dump_3a_deref(quad);      break;
        case OP_FCALL:      nnc_dump_3a_fcall(quad);      break;
        case OP_PCALL:      nnc_dump_3a_pcall(quad);      break;
        case OP_UJUMP:      nnc_dump_3a_ujump(quad);      break;
        case OP_CJUMPT:     nnc_dump_3a_cjumpt(quad);     break;
        case OP_CJUMPF:     nnc_dump_3a_cjumpf(quad);     break;
        case OP_CJUMPE:     nnc_dump_3a_cjumpe(quad);     break;
        case OP_CJUMPLT:    nnc_dump_3a_cjumplt(quad);    break;
        case OP_CJUMPGT:    nnc_dump_3a_cjumpgt(quad);    break;
        case OP_CJUMPNE:    nnc_dump_3a_cjumpne(quad);    break;
        case OP_CJUMPLTE:   nnc_dump_3a_cjumplte(quad);   break;
        case OP_CJUMPGTE:   nnc_dump_3a_cjumpgte(quad);   break;
        case OP_DEREF_COPY: nnc_dump_3a_deref_copy(quad); break;
        default: nnc_abort_no_ctx("nnc_dump_3a_quad: unimplemented case met.");
    }
}

void nnc_dump_3a_quads(FILE* to, const nnc_3a_quad* quads) {
    stream = to;
    for (nnc_u64 i = 0; i < buf_len(quads); i++) {
        nnc_dump_3a_quad(&quads[i]);
    }
}

void nnc_dump_3a_set(FILE* to, const nnc_3a_quad_set* set) {
    stream = to;
    dump_3a("@_%s: (%lu=%d%% after %d passes)\n", set->name,
        set->stat.reduced, set->stat.percent, set->stat.passes);
    //dump_3a("@_%s:\n", set->name);
    nnc_dump_3a_quads(to, set->quads);
    //for (nnc_u64 i = 0; i < buf_len(set->blocks); i++) {
    //    dump_3a("=========BLOCK[%u]=========\n", set->blocks[i].id);
    //    nnc_dump_3a_quads(to, set->blocks[i].quads);
    //}
}

void nnc_dump_3a_code(FILE* to, const nnc_3a_code code) {
    stream = to;
    for (nnc_u64 i = 0; i < buf_len(code); i++) {
        nnc_dump_3a_set(to, &code[i]);
    }
}

void nnc_dump_3a_data(FILE* to, const nnc_3a_data data) {
    stream = to;
    if (buf_len(data.quads) != 0) {
        nnc_dump_3a_set(to, &data);
    }
}