#include "nnc_type.h"

nnc_type* nnc_type_new(const nnc_str repr) {
    nnc_type* ptr = anew(nnc_type);
    ptr->repr = repr;
    ptr->kind = T_INCOMPLETE;
    return ptr;
}

nnc_type* nnc_ptr_type_new(nnc_type* base) {
    nnc_type* ptr = nnc_type_new(base->repr);
    ptr->kind = T_POINTER;
    ptr->base = base;
    ptr->size = sizeof(nnc_heap_ptr);
    return ptr;
}

nnc_type* nnc_arr_type_new(nnc_type* base) {
    nnc_type* ptr = nnc_type_new(base->repr);
    ptr->kind = T_ARRAY;
    ptr->base = base;
    return ptr;
}

nnc_type* nnc_fn_type_new() {
    nnc_type* ptr = nnc_type_new(NULL);
    ptr->kind = T_FUNCTION;
    ptr->size = sizeof(nnc_heap_ptr);
    return ptr;
}

nnc_type* nnc_enum_type_new() {
    nnc_type* ptr = nnc_type_new(NULL);
    ptr->kind = T_ENUM;
    ptr->size = i64_type.size;
    return ptr;
}

nnc_type* nnc_union_type_new() {
    nnc_type* ptr = nnc_type_new(NULL);
    ptr->kind = T_UNION;
    ptr->size = 0;
    return ptr;
}

nnc_type* nnc_alias_type_new() {
    nnc_type* ptr = nnc_type_new(NULL);
    ptr->kind = T_ALIAS;
    return ptr;
}

nnc_type* nnc_struct_type_new() {
    nnc_type* ptr = nnc_type_new(NULL);
    ptr->kind = T_STRUCT;
    ptr->size = 0;
    return ptr;
}

nnc_type* nnc_namespace_type_new() {
    nnc_type* ptr = nnc_type_new(NULL);
    ptr->kind = T_NAMESPACE;
    return ptr;
}

nnc_static nnc_str nnc_fn_type_tostr(const nnc_type* type) {
    nnc_str repr = "fn( ";
    for (nnc_u64 i = 0; i < type->exact.fn.paramc; i++) {
        repr = sformat("%s%s ", repr, nnc_type_tostr(type->exact.fn.params[i]->type));
    }
    return sformat("%s):%s", repr, nnc_type_tostr(type->exact.fn.ret->type));
}

nnc_static nnc_str nnc_enum_type_tostr(const nnc_type* type) {
    nnc_str repr = "{ ";
    nnc_u64 count = type->exact.enumeration.memberc;
    for (nnc_u64 i = 0; i < count; i++) {
        if (i > 2) {
            repr = sformat("%s..%lu more.. ", repr, count-i);
            break;
        }
        repr = sformat("%s%s ", repr, 
            type->exact.enumeration.members[i]->var->name);
    }
    return sformat("%s}", repr);
}

nnc_static nnc_str nnc_alias_type_tostr(const nnc_type* type) {
    nnc_type* alias = type->base;
    while (alias->kind == T_ALIAS) {
        alias = alias->base;
    }
    return nnc_type_tostr(alias);
}

nnc_static nnc_str nnc_struct_or_union_type_tostr(const nnc_type* type) {
    nnc_u64 more = 0;
    nnc_str repr = "{ ";
    nnc_u64 count = type->exact.struct_or_union.memberc;
    nnc_struct_member* member = NULL;
    if (count >= 1) {
        member = type->exact.struct_or_union.members[0];
        if (member->texpr->type->kind == T_ALIAS) {
            repr = sformat("%s%s:%s ", repr, 
                member->var->name, member->texpr->type->repr);
        }
        else {
            repr = sformat("%s%s:%s ", repr, member->var->name, 
                nnc_type_tostr(member->texpr->type));
        }
    }
    if (count > 2) {
        more = count - 2;
        repr = sformat("%s..%lu more.. ", repr, more);
    }
    if (count > 1) {
        member = type->exact.struct_or_union.members[count-1];
        if (member->texpr->type->kind == T_ALIAS) {
            repr = sformat("%s%s:%s ", repr, member->var->name,
                member->texpr->type->repr);
        }
        else {
            repr = sformat("%s%s:%s ", repr, member->var->name,
                nnc_type_tostr(member->texpr->type));
        }
    }
    return sformat("%s}", repr);
}

nnc_str nnc_type_tostr(const nnc_type* type) {
    switch (type->kind) {
        case T_ALIAS:     return sformat("%s (aka %s)", type->repr, nnc_alias_type_tostr(type));
        case T_ENUM:      return sformat("enum %s",   nnc_enum_type_tostr(type));
        case T_UNION:     return sformat("union %s",  nnc_struct_or_union_type_tostr(type));
        case T_STRUCT:    return sformat("struct %s", nnc_struct_or_union_type_tostr(type));
        case T_NAMESPACE: return sformat("namespace %s", type->repr);
        case T_FUNCTION:  return nnc_fn_type_tostr(type);
        case T_ARRAY:     return sformat("%s[]", nnc_type_tostr(type->base));
        case T_POINTER:   return sformat("%s*",  nnc_type_tostr(type->base));
        default:             return type->repr;
    }
}