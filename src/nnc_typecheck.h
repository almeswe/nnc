#ifndef _NNC_TYPECHECK_H
#define _NNC_TYPECHECK_H

#include "nnc_ast.h"

nnc_type* nnc_expr_infer_type(nnc_expression* expr);
nnc_type* nnc_expr_get_type(nnc_expression* expr);

#endif