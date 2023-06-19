#include "nnc_type.h"

nnc_type* nnc_type_new(const nnc_str repr) {
    nnc_type* ptr = new(nnc_type);
    ptr->repr = repr;
    ptr->kind = TYPE_INCOMPLETE;
    return ptr;
}

nnc_type* nnc_ptr_type_new(nnc_type* base) {
    nnc_type* ptr = nnc_type_new(base->repr);
    ptr->kind = TYPE_POINTER;
    ptr->base = base;
    ptr->size = sizeof(nnc_heap_ptr);
    return ptr;
}

nnc_type* nnc_arr_type_new(nnc_type* base) {
    nnc_type* ptr = nnc_type_new(base->repr);
    ptr->kind = TYPE_ARRAY;
    ptr->base = base;
    return ptr;
}

nnc_type* nnc_fn_type_new() {
    nnc_type* ptr = nnc_type_new(NULL);
    ptr->kind = TYPE_FUNCTION;
    ptr->size = sizeof(nnc_heap_ptr);
    return ptr;
}

nnc_str nnc_type_tostr(const nnc_type* type) {
    if (type->kind == TYPE_FUNCTION) {
        nnc_str repr = sformat("fn( %s )( ", nnc_type_tostr(type->exact.fn.ret));
        for (nnc_u64 i = 0; i < type->exact.fn.paramc; i++) {
            repr = sformat("%s%s ", repr, nnc_type_tostr(type->exact.fn.params[i]));
        }
        return sformat("%s)", repr);
    }
    switch (type->kind) {
        case TYPE_ENUM:    return sformat("enum %s",   type->repr);
        case TYPE_UNION:   return sformat("union %s",  type->repr);
        case TYPE_STRUCT:  return sformat("struct %s", type->repr);
        case TYPE_ARRAY:   return sformat("%s[]", nnc_type_tostr(type->base));
        case TYPE_POINTER: return sformat("%s*",  nnc_type_tostr(type->base));
        default:
            return type->repr;
    }
}