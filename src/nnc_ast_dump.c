#include "nnc_ast.h"
#include "nnc_misc.h"

#define TREE_BR 	"\\_"
#define TREE_BV 	"| "
#define TREE_BVR    "|_"

#define DUMP_DATA(i, e) (nnc_dump_data) { .indent = i, .exact = (nnc_heap_ptr)(e) }

typedef struct _nnc_dump_data {
    nnc_i64 indent;
    nnc_heap_ptr exact;
} nnc_dump_data;

typedef void (*nnc_dump_fn)(nnc_dump_data);

static void nnc_dump_expr(nnc_dump_data data);

static nnc_bool nnc_is_escape(nnc_byte code) {
    switch (code) {
        case '\0': case '\a':
        case '\b': case '\f':
        case '\t': case '\v':
        case '\r': case '\n':
        case '\\': case '\'':
        case '\"':
            return true;
    }
    return false;
}

static nnc_byte nnc_escape(nnc_byte code) {
    switch (code) {
        case '\0': return '0';
        case '\a': return 'a';
        case '\b': return 'b';
        case '\f': return 'f';
        case '\t': return 't';
        case '\v': return 'v';
        case '\r': return 'r';
        case '\n': return 'n';
        case '\\': return '\\'; 
        case '\'': return '\'';
        case '\"': return '\"';
    }
    nnc_abort_no_ctx("nnc_get_escape_repr: unknown escape character.\n");
    return '\0';
}

static void nnc_dump_indent(nnc_i64 indent) {
    /*nnc_byte buf[128] = { 0 };
    if (indent > sizeof buf - 1) {
        indent = sizeof buf - 1;
    }*/
    for (nnc_i64 i = 0; i < indent; i++) {
        //buf[i] = ' ';
        fprintf(stderr, "%s", "  ");
    }
    //fprintf(stderr, "%s", buf);
}

static void nnc_dump_chr(nnc_dump_data data) {
    nnc_dump_indent(data.indent);
    const nnc_chr_literal* literal = data.exact;
    nnc_byte c_repr[3] = { 0 };
    c_repr[0] = literal->exact;
    if (nnc_is_escape(literal->exact)) {
        c_repr[0] = '\\';
        c_repr[1] = nnc_escape(literal->exact);
    }
    fprintf(stderr, TREE_BR _c(BYEL, " chr") "=\'%s\'<%d>\n", 
        c_repr, (nnc_i32)literal->exact);
}

static void nnc_dump_str(nnc_dump_data data) {
    nnc_dump_indent(data.indent);
    const nnc_str_literal* literal = data.exact;
    fprintf(stderr, TREE_BR _c(BYEL, " str") "=");
    for (nnc_u64 i = 0; i < literal->length; i++) {
        nnc_byte code = literal->exact[i];
        if (!nnc_is_escape(code)) {
            fprintf(stderr, "%c", code);
        }
        else {
            fprintf(stderr, "\\%c", nnc_escape(code));
        }
    }
    fprintf(stderr, "<len %lu>\n", literal->length);
}

static void nnc_dump_int(nnc_dump_data data) {
    nnc_dump_indent(data.indent);
    const nnc_int_literal* literal = data.exact;
    /*
    nnc_i64 d = 0ll;
    nnc_u64 u = 0ull;
    switch (literal->suffix) {
        case SUFFIX_I8:  d = (nnc_i8)literal->exact.d;  break;
        case SUFFIX_I16: d = (nnc_i16)literal->exact.d; break;
        case SUFFIX_I32: d = (nnc_i32)literal->exact.d; break;
        case SUFFIX_I64: d = (nnc_i64)literal->exact.d; break;
        case SUFFIX_U8:  u = (nnc_u8)literal->exact.u;  break;
        case SUFFIX_U16: u = (nnc_u16)literal->exact.u; break;
        case SUFFIX_U32: u = (nnc_u32)literal->exact.u; break;
        case SUFFIX_U64: u = (nnc_u64)literal->exact.u; break;
    }*/
    fprintf(stderr, TREE_BR _c(BYEL, " int") "=");
    if (literal->is_signed) {
        fprintf(stderr, "%ld", literal->exact.d);
        fprintf(stderr, "%s", "<signed>");
    }
    else {
        fprintf(stderr, "%lu", literal->exact.u);
    }
    fprintf(stderr, "<base %d>", literal->base);
    fprintf(stderr, "<suffix %d>\n", literal->suffix);
}

static void nnc_dump_dbl(nnc_dump_data data) {
    nnc_dump_indent(data.indent);
    const nnc_dbl_literal* literal = data.exact;
    fprintf(stderr, TREE_BR _c(BYEL, " float") "=");
    fprintf(stderr, "%f", literal->exact);
    fprintf(stderr, "<suffix %d>\n", literal->suffix);
}

static void nnc_dump_unary(nnc_dump_data data) {
    nnc_dump_indent(data.indent);
    const nnc_unary_expression* unary = data.exact;
    static const char* unary_str[] = {
        [UNARY_PLUS]        = "+ (plus)",
        [UNARY_MINUS]       = "- (minus)",
        [UNARY_BITWISE_NOT] = "~ (bit not)",
        [UNARY_DEREF]       = "* (deref)",
        [UNARY_REF]         = "& (ref)",
        [UNARY_NOT]         = "! (not)",
        [UNARY_SIZEOF]      = "sizeof",
        [UNARY_LENGTHOF]    = "lengthof",
        [UNARY_POSTFIX_AS]  = "as"
    };
    fprintf(stderr, TREE_BR _c(BGRN, " unary-expr") " <%s>\n", unary_str[unary->kind]);
    switch (unary->kind) {
        case UNARY_POSTFIX_AS:
        case UNARY_SIZEOF:
        case UNARY_LENGTHOF:
            break;
        default:
            nnc_dump_expr(DUMP_DATA(data.indent + 1, unary->expr));
    }
}

static void nnc_dump_binary(nnc_dump_data data) {
    nnc_dump_indent(data.indent);
    const nnc_binary_expression* binary = data.exact;
    static const char* binary_str[] = {
        [BINARY_ADD] = "+ (add)",
        [BINARY_SUB] = "- (sub)",
        [BINARY_MUL] = "* (mul)",
        [BINARY_DIV] = "/ (div)",
        [BINARY_MOD] = "%% (mod)",
        [BINARY_SHR] = ">> (shr)",
        [BINARY_SHL] = "<< (shl)",
        [BINARY_LT]  = " < (lt)", 
        [BINARY_GT]  = " > (gt)",
        [BINARY_LTE] = "<= (lte)",
        [BINARY_GTE] = ">= (gte)",
        [BINARY_EQ]  = "== (eq)",
        [BINARY_NEQ] = "== (neq)",
        [BINARY_BW_AND] = "& (bit and)",
        [BINARY_BW_XOR] = "^ (bit xor)",
        [BINARY_BW_OR]  = "| (bit or)",
        [BINARY_AND]    = "&& (and)",
        [BINARY_OR]     = "|| (or)"
    };
    fprintf(stderr, TREE_BR _c(BGRN, " binary-expr") " <%s>\n", binary_str[binary->kind]);
    nnc_dump_expr(DUMP_DATA(data.indent + 1, binary->lexpr));
    nnc_dump_expr(DUMP_DATA(data.indent + 1, binary->rexpr));
}

static void nnc_dump_ternary(nnc_dump_data data) {
    nnc_dump_indent(data.indent);
    const nnc_ternary_expression* ternary = data.exact;
    fprintf(stderr, "%s\n", TREE_BR _c(BGRN, " ternary-expr (? :)"));
    nnc_dump_expr(DUMP_DATA(data.indent + 1, ternary->cexpr));
    nnc_dump_expr(DUMP_DATA(data.indent + 1, ternary->lexpr));
    nnc_dump_expr(DUMP_DATA(data.indent + 1, ternary->rexpr));
}

static void nnc_dump_expr(nnc_dump_data data) {
    const nnc_expression* expr = data.exact;
    static const nnc_dump_fn dumpers[] = {
        [EXPR_DBL_LITERAL] = nnc_dump_dbl,
        [EXPR_INT_LITERAL] = nnc_dump_int,
        [EXPR_CHR_LITERAL] = nnc_dump_chr,
        [EXPR_STR_LITERAL] = nnc_dump_str,
        [EXPR_UNARY]       = nnc_dump_unary,
        [EXPR_BINARY]      = nnc_dump_binary,
        [EXPR_TERNARY]     = nnc_dump_ternary
    };
    dumpers[expr->kind](DUMP_DATA(data.indent, expr->exact));
} 

void nnc_dump_ast(const nnc_ast* ast) {
    fprintf(stderr, _c(BCYN, "%s\n"), "ast");
    nnc_dump_expr(DUMP_DATA(1, ast->expr));
    fflush(stderr);
}