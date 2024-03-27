#ifndef __NNC_AST_EXPRESSION_H__
#define __NNC_AST_EXPRESSION_H__

#include "nnc_buf.h"
#include "nnc_type.h"
#include "nnc_literal.h"

typedef enum _nnc_expression_kind {
    EXPR_DBL_LITERAL,
    EXPR_INT_LITERAL,
    EXPR_CHR_LITERAL,
    EXPR_STR_LITERAL,
    EXPR_IDENT,
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_TERNARY,
} nnc_expression_kind;

typedef struct _nnc_expression {
    nnc_heap_ptr exact;
    nnc_expression_kind kind;
} nnc_expression;

typedef enum _nnc_unary_expression_kind {
    UNARY_REF,
    UNARY_NOT,
    UNARY_CAST,
    UNARY_PLUS,
    UNARY_MINUS,
    UNARY_DEREF,
    UNARY_SIZEOF,
    UNARY_LENGTHOF,
    UNARY_BITWISE_NOT,
    UNARY_POSTFIX_AS,
    UNARY_POSTFIX_DOT,
    UNARY_POSTFIX_CALL,
    UNARY_POSTFIX_INDEX
} nnc_unary_expression_kind;

typedef struct _nnc_unary_expression {
    nnc_ctx ctx;
    nnc_type* type;
    nnc_expression* expr;
    nnc_unary_expression_kind kind;
    union {
        struct _nnc_unary_sizeof {
            nnc_type_expression* of;    
        } size;
        struct _nnc_unary_cast {
            nnc_type_expression* to;
        } cast;
        struct _nnc_unary_postfix_dot {
            nnc_expression* member;
        } dot;
        struct _nnc_unary_postfix_call {
            nnc_u64 argc;
            nnc_expression** args;
        } call;
        struct _nnc_unary_postfix_scope {
            nnc_expression* member;
        } scope;
        struct _nnc_unary_postfix_index {
            nnc_expression* expr;
        } index;
    } exact;
} nnc_unary_expression;

typedef enum _nnc_binary_expression_kind {
    BINARY_ADD,
    BINARY_SUB,
    BINARY_MUL,
    BINARY_DIV,
    BINARY_MOD,
    BINARY_SHR,
    BINARY_SHL,
    BINARY_LT,
    BINARY_GT,
    BINARY_LTE,
    BINARY_GTE,
    BINARY_EQ,
    BINARY_NEQ,
    BINARY_BW_AND,
    BINARY_BW_XOR,
    BINARY_BW_OR,
    BINARY_AND,
    BINARY_OR,
    BINARY_ASSIGN,
    BINARY_COMMA
} nnc_binary_expression_kind;

typedef struct _nnc_binary_expression {
    nnc_ctx ctx;
    nnc_type* type;
    nnc_expression* lexpr;
    nnc_expression* rexpr;
    nnc_binary_expression_kind kind;
} nnc_binary_expression;

typedef struct _nnc_ternary_expression {
    nnc_ctx ctx;
    nnc_type* type;
    nnc_expression* cexpr;
    nnc_expression* rexpr;
    nnc_expression* lexpr;
} nnc_ternary_expression;

typedef struct _nnc_init_expression {
    nnc_expression** exprs;
    enum _nnc_init_expression_kind {
        INIT_KIND_UNTYPED,
        INIT_KIND_ZERO,
        INIT_KIND_UNION,
        INIT_KIND_ARRAY,
        INIT_KIND_STRUCT
    } kind;
    nnc_heap_ptr inits;
} nnc_init_expression;

nnc_ident* nnc_ident_new(const nnc_byte* from);
nnc_ctx* nnc_expr_get_ctx(const nnc_expression* expr);

nnc_unary_expression* nnc_unary_expr_new(nnc_unary_expression_kind kind);
nnc_binary_expression* nnc_binary_expr_new(nnc_binary_expression_kind kind);
nnc_ternary_expression* nnc_ternary_expr_new();

nnc_expression* nnc_expr_new(nnc_expression_kind kind, nnc_heap_ptr exact);

#endif