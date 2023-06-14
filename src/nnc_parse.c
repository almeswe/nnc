#include "nnc_parse.h"

void nnc_parser_init(nnc_parser* out_parser, const char* file) {
    nnc_lex_init(&out_parser->lex, file);
}

void nnc_parser_fini(nnc_parser* parser) {
    if (parser != NULL) {
        nnc_lex_fini(&parser->lex);
    }
}

nnc_tok* nnc_parser_get(nnc_parser* parser) {
    return &parser->lex.ctok;
}

nnc_tok_kind nnc_parser_peek(nnc_parser* parser) {
    return parser->lex.ctok.kind;
}

nnc_bool nnc_parser_match(nnc_parser* parser, nnc_tok_kind kind) {
    return nnc_parser_peek(parser) == kind;
}

nnc_tok_kind nnc_parser_next(nnc_parser* parser) {
    return nnc_lex_next(&parser->lex);
}

nnc_tok_kind nnc_parser_expect(nnc_parser* parser, nnc_tok_kind kind) {
    nnc_tok_kind current = nnc_parser_peek(parser);
    if (current != kind) {
        THROW(NNC_SYNTAX, sformat("expected <%s>, but met <%s>.", 
            nnc_tok_str(kind), nnc_tok_str(current)));
    }
    nnc_parser_next(parser);
    return nnc_parser_peek(parser);
}

static nnc_expression* nnc_parse_dbl(nnc_parser* parser) {
    const nnc_tok* tok = nnc_parser_get(parser);
    nnc_heap_ptr exact = nnc_dbl_new(tok->lexeme);
    nnc_parser_next(parser);
    return nnc_expr_new(EXPR_DBL_LITERAL, exact);
}

static nnc_expression* nnc_parse_int(nnc_parser* parser) {
    const nnc_tok* tok = nnc_parser_get(parser);
    nnc_heap_ptr exact = nnc_int_new(tok->lexeme);
    nnc_parser_next(parser);
    return nnc_expr_new(EXPR_INT_LITERAL, exact);
}

static nnc_expression* nnc_parse_number(nnc_parser* parser) {
    const nnc_tok* tok = nnc_parser_get(parser);
    if (strchr(tok->lexeme, '.') != NULL) {
        return nnc_parse_dbl(parser);
    }
    return nnc_parse_int(parser);
}

static nnc_expression* nnc_parse_chr(nnc_parser* parser) {
    const nnc_tok* tok = nnc_parser_get(parser);
    nnc_heap_ptr exact = nnc_chr_new(tok->lexeme);
    nnc_parser_next(parser);
    return nnc_expr_new(EXPR_CHR_LITERAL, exact);
}

static nnc_expression* nnc_parse_str(nnc_parser* parser) {
    const nnc_tok* tok = nnc_parser_get(parser);
    nnc_heap_ptr exact = nnc_str_new(tok->lexeme);
    nnc_parser_next(parser);
    return nnc_expr_new(EXPR_STR_LITERAL, exact);
}

static nnc_expression* nnc_parse_parens(nnc_parser* parser) {
    nnc_parser_next(parser);
    nnc_expression* expr = nnc_parse_expr(parser);
    nnc_parser_expect(parser, TOK_CPAREN);
    return expr;
}

static nnc_expression* nnc_parse_primary(nnc_parser* parser) {
    switch (nnc_parser_peek(parser)) {
        case TOK_STR:    return nnc_parse_str(parser);
        case TOK_CHR:    return nnc_parse_chr(parser);
        case TOK_NUMBER: return nnc_parse_number(parser);
        case TOK_OPAREN: return nnc_parse_parens(parser);
        default:
            THROW(NNC_UNINPLEMENTED, sformat("nnc_parse_expr -> %s.\n", 
                nnc_tok_str(nnc_parser_peek(parser))));
    }
    return NULL;
}

static nnc_expression* nnc_parse_unary(nnc_parser* parser);

static nnc_expression* nnc_parse_plus(nnc_parser* parser) {
    nnc_parser_next(parser);
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_PLUS);
    expr->expr = nnc_parse_unary(parser);
    return nnc_expr_new(EXPR_UNARY, expr);
}

static nnc_expression* nnc_parse_minus(nnc_parser* parser) {
    nnc_parser_next(parser);
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_MINUS);
    expr->expr = nnc_parse_unary(parser);
    return nnc_expr_new(EXPR_UNARY, expr);
}

static nnc_expression* nnc_parse_bitwise_not(nnc_parser* parser) {
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_BITWISE_NOT);
    nnc_parser_next(parser);
    expr->expr = nnc_parse_unary(parser);
    return nnc_expr_new(EXPR_UNARY, expr); 
}

static nnc_expression* nnc_parse_dereference(nnc_parser* parser) {
    nnc_parser_next(parser);
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_DEREF);
    expr->expr = nnc_parse_unary(parser);
    return nnc_expr_new(EXPR_UNARY, expr); 
}

static nnc_expression* nnc_parse_reference(nnc_parser* parser) {
    nnc_parser_next(parser);
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_REF);
    expr->expr = nnc_parse_unary(parser);
    return nnc_expr_new(EXPR_UNARY, expr); 
}

static nnc_expression* nnc_parse_not(nnc_parser* parser) {
    nnc_parser_next(parser);
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_NOT);
    expr->expr = nnc_parse_unary(parser);
    return nnc_expr_new(EXPR_UNARY, expr); 
}

static nnc_expression* nnc_parse_sizeof(nnc_parser* parser) {
    nnc_parser_next(parser);
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_SIZEOF);
    nnc_parser_expect(parser, TOK_OPAREN);
    //todo: insert type parsing here. 
    nnc_parser_next(parser);    
    expr->exact.size.of = (nnc_heap_ptr)0xBAD;
    //-------------------------------
    nnc_parser_expect(parser, TOK_CPAREN);
    return nnc_expr_new(EXPR_UNARY, expr);
}

static nnc_expression* nnc_parse_lengthof(nnc_parser* parser) {
    nnc_parser_next(parser);
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_SIZEOF);
    expr->exact.length.of = nnc_parse_expr(parser);
    return nnc_expr_new(EXPR_UNARY, expr);
}

static nnc_expression* nnc_parse_as(nnc_parser* parser) {
    nnc_parser_next(parser);    
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_POSTFIX_AS);
    //todo: insert type parsing here.
    fprintf(stderr, "type repr: %s\n", nnc_parser_get(parser)->lexeme);
    nnc_parser_next(parser);
    expr->exact.cast.to = (nnc_heap_ptr)0xBAD;
    //-------------------------------
    return nnc_expr_new(EXPR_UNARY, expr);
}

static nnc_expression* nnc_parse_postfix(nnc_parser* parser) {
    nnc_expression* expr = nnc_parse_primary(parser);
    //nnc_unresolved_stack_push_stage_1(UNRESOLVED_TYPE, "usertype", CONTEXT_AS, as_expr);
    const nnc_tok* tok = nnc_parser_get(parser);
    switch (tok->kind) {
        case TOK_AS:    return nnc_parse_as(parser);
        default:
            return expr;
    }
    //THROW(NNC_UNINPLEMENTED, "nnc_parse_postfix");
}

static nnc_expression* nnc_parse_unary(nnc_parser* parser) {
    const nnc_tok* tok = nnc_parser_get(parser);
    switch (tok->kind) {
        case TOK_PLUS:      return nnc_parse_plus(parser);
        case TOK_MINUS:     return nnc_parse_minus(parser);
        case TOK_TILDE:     return nnc_parse_bitwise_not(parser);
        case TOK_ASTERISK:  return nnc_parse_dereference(parser);
        case TOK_AMPERSAND: return nnc_parse_reference(parser);
        case TOK_EXCMARK:   return nnc_parse_not(parser);
        case TOK_SIZEOF:    return nnc_parse_sizeof(parser);
        case TOK_LENGTHOF:  return nnc_parse_lengthof(parser);
        default: 
            return nnc_parse_postfix(parser);
    }
}

static nnc_expression* nnc_parse_arith_multiplication(nnc_parser* parser) {
    nnc_expression* nether_expr = nnc_parse_unary(parser);
    nnc_binary_expression* temp = NULL;
    nnc_binary_expression* expr = NULL;
    while (nnc_parser_match(parser, TOK_SLASH)   ||
           nnc_parser_match(parser, TOK_PERCENT) ||
           nnc_parser_match(parser, TOK_ASTERISK)) {
        const nnc_tok* tok = nnc_parser_get(parser);
        nnc_binary_expression_kind kind = BINARY_DIV;
        switch (tok->kind) {
            case TOK_SLASH:    kind = BINARY_DIV; break;
            case TOK_PERCENT:  kind = BINARY_MOD; break;
            case TOK_ASTERISK: kind = BINARY_MUL; break;
            default:
                nnc_abort_no_ctx("nnc_parse_arith_multiplication: bug detected.\n");
        }
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(kind);
        temp->lexpr = nether_expr;
        temp->rexpr = nnc_parse_unary(parser);
        if (expr != NULL) {
            temp->lexpr = nnc_expr_new(EXPR_BINARY, expr);
        }
        expr = temp;
    }
    if (expr != NULL) {
        return nnc_expr_new(EXPR_BINARY, expr);
    }
    return nether_expr;
}

static nnc_expression* nnc_parse_arith_addition(nnc_parser* parser) {
    nnc_expression* nether_expr = nnc_parse_arith_multiplication(parser);
    nnc_binary_expression* temp = NULL;
    nnc_binary_expression* expr = NULL;
    while (nnc_parser_match(parser, TOK_PLUS) ||
           nnc_parser_match(parser, TOK_MINUS)) {
        nnc_binary_expression_kind kind = BINARY_ADD;
        if (nnc_parser_match(parser, TOK_MINUS)) {
            kind = BINARY_SUB;
        }
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(kind);
        temp->lexpr = nether_expr;
        temp->rexpr = nnc_parse_arith_multiplication(parser);
        if (expr != NULL) {
            temp->lexpr = nnc_expr_new(EXPR_BINARY, expr);
        }
        expr = temp;
    }
    if (expr != NULL) {
        return nnc_expr_new(EXPR_BINARY, expr);
    }
    return nether_expr;
}

static nnc_expression* nnc_parse_arith_shift(nnc_parser* parser) {
    nnc_expression* nether_expr = nnc_parse_arith_addition(parser);
    nnc_binary_expression* temp = NULL;
    nnc_binary_expression* expr = NULL;
    while (nnc_parser_match(parser, TOK_LSHIFT) ||
           nnc_parser_match(parser, TOK_RSHIFT)) {
        nnc_binary_expression_kind kind = BINARY_SHL;
        if (nnc_parser_match(parser, TOK_RSHIFT)) {
            kind = BINARY_SHR;
        }
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(kind);
        temp->lexpr = nether_expr;
        temp->rexpr = nnc_parse_arith_addition(parser);
        if (expr != NULL) {
            temp->lexpr = nnc_expr_new(EXPR_BINARY, expr);
        }
        expr = temp;
    }
    if (expr != NULL) {
        return nnc_expr_new(EXPR_BINARY, expr);
    }
    return nether_expr;
}

static nnc_expression* nnc_parse_relation(nnc_parser* parser) {
    nnc_expression* nether_expr = nnc_parse_arith_shift(parser);
    nnc_binary_expression* temp = NULL;
    nnc_binary_expression* expr = NULL;
    while (nnc_parser_match(parser, TOK_LT)  ||
           nnc_parser_match(parser, TOK_GT)  ||
           nnc_parser_match(parser, TOK_LTE) ||
           nnc_parser_match(parser, TOK_GTE)) {
        const nnc_tok* tok = nnc_parser_get(parser); 
        nnc_binary_expression_kind kind = BINARY_LT;
        switch (tok->kind) {
            case TOK_LT:  kind = BINARY_LT;  break;
            case TOK_GT:  kind = BINARY_GT;  break;
            case TOK_LTE: kind = BINARY_LTE; break;
            case TOK_GTE: kind = BINARY_GTE; break;
            default:
                nnc_abort_no_ctx("nnc_parse_relation: bug detected.\n");
        }
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(kind);
        temp->lexpr = nether_expr;
        temp->rexpr = nnc_parse_arith_shift(parser);
        if (expr != NULL) {
            temp->lexpr = nnc_expr_new(EXPR_BINARY, expr);
        }
        expr = temp;
    }
    if (expr != NULL) {
        return nnc_expr_new(EXPR_BINARY, expr);
    }
    return nether_expr;
}

static nnc_expression* nnc_parse_equality(nnc_parser* parser) {
    nnc_expression* nether_expr = nnc_parse_relation(parser);
    nnc_binary_expression* temp = NULL;
    nnc_binary_expression* expr = NULL;
    while (nnc_parser_match(parser, TOK_EQ)  ||
           nnc_parser_match(parser, TOK_NEQ)) {
        nnc_binary_expression_kind kind = BINARY_EQ;
        if (nnc_parser_match(parser, TOK_NEQ)) {
            kind = BINARY_NEQ;
        }
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(kind);
        temp->lexpr = nether_expr;
        temp->rexpr = nnc_parse_relation(parser);
        if (expr != NULL) {
            temp->lexpr = nnc_expr_new(EXPR_BINARY, expr);
        }
        expr = temp;
    }
    if (expr != NULL) {
        return nnc_expr_new(EXPR_BINARY, expr);
    }
    return nether_expr;
}

static nnc_expression* nnc_parse_bitwise_and(nnc_parser* parser) {
    nnc_expression* nether_expr = nnc_parse_equality(parser);
    nnc_binary_expression* temp = NULL;
    nnc_binary_expression* expr = NULL;
    while (nnc_parser_match(parser, TOK_AMPERSAND)) {
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(BINARY_BW_AND);
        temp->lexpr = nether_expr;
        temp->rexpr = nnc_parse_equality(parser);
        if (expr != NULL) {
            temp->lexpr = nnc_expr_new(EXPR_BINARY, expr);
        }
        expr = temp;
    }
    if (expr != NULL) {
        return nnc_expr_new(EXPR_BINARY, expr);
    }
    return nether_expr;
}

static nnc_expression* nnc_parse_bitwise_xor(nnc_parser* parser) {
    nnc_expression* nether_expr = nnc_parse_bitwise_and(parser);
    nnc_binary_expression* temp = NULL;
    nnc_binary_expression* expr = NULL;
    while (nnc_parser_match(parser, TOK_CIRCUMFLEX)) {
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(BINARY_BW_XOR);
        temp->lexpr = nether_expr;
        temp->rexpr = nnc_parse_bitwise_and(parser);
        if (expr != NULL) {
            temp->lexpr = nnc_expr_new(EXPR_BINARY, expr);
        }
        expr = temp;
    }
    if (expr != NULL) {
        return nnc_expr_new(EXPR_BINARY, expr);
    }
    return nether_expr;
}

static nnc_expression* nnc_parse_bitwise_or(nnc_parser* parser) {
    nnc_expression* nether_expr = nnc_parse_bitwise_xor(parser);
    nnc_binary_expression* temp = NULL;
    nnc_binary_expression* expr = NULL;
    while (nnc_parser_match(parser, TOK_VLINE)) {
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(BINARY_BW_OR);
        temp->lexpr = nether_expr;
        temp->rexpr = nnc_parse_bitwise_xor(parser);
        if (expr != NULL) {
            temp->lexpr = nnc_expr_new(EXPR_BINARY, expr);
        }
        expr = temp;
    }
    if (expr != NULL) {
        return nnc_expr_new(EXPR_BINARY, expr);
    }
    return nether_expr;
}

static nnc_expression* nnc_parse_and(nnc_parser* parser) {
    nnc_expression* nether_expr = nnc_parse_bitwise_or(parser);
    nnc_binary_expression* temp = NULL;
    nnc_binary_expression* expr = NULL;
    while (nnc_parser_match(parser, TOK_AND)) {
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(BINARY_AND);
        temp->lexpr = nether_expr;
        temp->rexpr = nnc_parse_bitwise_or(parser);
        if (expr != NULL) {
            temp->lexpr = nnc_expr_new(EXPR_BINARY, expr);
        }
        expr = temp;
    }
    if (expr != NULL) {
        return nnc_expr_new(EXPR_BINARY, expr);
    }
    return nether_expr;
}

static nnc_expression* nnc_parse_or(nnc_parser* parser) {
    nnc_expression* nether_expr = nnc_parse_and(parser);
    nnc_binary_expression* temp = NULL;
    nnc_binary_expression* expr = NULL;
    while (nnc_parser_match(parser, TOK_OR)) {
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(BINARY_OR);
        temp->lexpr = nether_expr;
        temp->rexpr = nnc_parse_and(parser);
        if (expr != NULL) {
            temp->lexpr = nnc_expr_new(EXPR_BINARY, expr);
        }
        expr = temp;
    }
    if (expr != NULL) {
        return nnc_expr_new(EXPR_BINARY, expr);
    }
    return nether_expr;
}

static nnc_expression* nnc_parse_ternary(nnc_parser* parser) {
    nnc_expression* nether_expr = nnc_parse_or(parser);
    nnc_ternary_expression* expr = NULL;
    if (nnc_parser_match(parser, TOK_QUESTION)) {
        nnc_parser_next(parser);
        expr = nnc_ternary_expr_new();
        expr->lexpr = nnc_parse_expr(parser);
        nnc_parser_expect(parser, TOK_COLON);
        expr->rexpr = nnc_parse_expr(parser);
        expr->cexpr = nether_expr;   
    }
    if (expr != NULL) {
        return nnc_expr_new(EXPR_TERNARY, expr);
    }
    return nether_expr;
}

nnc_expression* nnc_parse_expr(nnc_parser* parser) {
    return nnc_parse_ternary(parser);
}

nnc_ast* nnc_parse(const char* file) {
    nnc_parser parser = { 0 };
    nnc_parser_init(&parser, file);
    nnc_parser_next(&parser);
    nnc_ast* ast = nnc_ast_new(file);
    ast->expr = nnc_parse_expr(&parser);
    nnc_parser_fini(&parser);
    return ast;
}