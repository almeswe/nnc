#ifndef _NNC_TYPE_H
#define _NNC_TYPE_H

#include "nnc_arena.h"
#include "nnc_format.h"

typedef struct _nnc_type       nnc_type;
typedef struct _nnc_expression nnc_expression;

typedef enum _nnc_ident_ctx {
    IDENT_DEFAULT,
    IDENT_NAMESPACE,
    IDENT_ENUMERATOR
} nnc_ident_ctx;

typedef struct _nnc_ident {
    nnc_u64 size;
    nnc_str name;
    nnc_type* type;
    nnc_ident_ctx ctx;
} nnc_ident;

typedef struct _nnc_enumerator {
    nnc_ident* var;
    nnc_expression* init;
    union _nnc_enumerator_value {
        nnc_u64 u;
        nnc_i64 d;
    } init_const;
    nnc_type* in_enum;
} nnc_enumerator;

typedef struct _nnc_struct_member {
    nnc_ident* var;
    nnc_type* type;
} nnc_struct_member;

typedef enum _nnc_type_kind {
	TYPE_ARRAY,
	TYPE_POINTER,
	TYPE_PRIMITIVE_I8,
	TYPE_PRIMITIVE_U8,
	TYPE_PRIMITIVE_I16,
	TYPE_PRIMITIVE_U16,
	TYPE_PRIMITIVE_I32,
	TYPE_PRIMITIVE_U32,
	TYPE_PRIMITIVE_I64,
	TYPE_PRIMITIVE_U64,
	TYPE_PRIMITIVE_F32,
	TYPE_PRIMITIVE_F64,
	TYPE_ENUM,
	TYPE_UNION,
	TYPE_STRUCT,
	TYPE_FUNCTION,
	TYPE_INCOMPLETE,
	TYPE_VOID,
	TYPE_UNKNOWN
} nnc_type_kind;

typedef struct _nnc_type {
    nnc_u64 size;
    nnc_type_kind kind;
    nnc_str repr;
    union {
    struct _nnc_fn_type {
        nnc_type*  ret;
        nnc_type** params;
        nnc_u64 paramc;
    } fn;
    struct _nnc_array_type {
        nnc_expression* dim;
    } array;
    struct _nnc_enumeration_type {
        nnc_u64 memberc;
        nnc_enumerator** members;
    } enumeration;
    struct _nnc_struct_or_union_type {
        nnc_u64 memberc;
        nnc_struct_member** members;
    } struct_or_union;
    } exact;
    nnc_type* base;
} nnc_type;

static nnc_type i8_type   __attribute__((unused)) = { .size=sizeof(nnc_i8),  .kind=TYPE_PRIMITIVE_I8,  .repr="i8"   };
static nnc_type u8_type   __attribute__((unused)) = { .size=sizeof(nnc_u8),  .kind=TYPE_PRIMITIVE_U8,  .repr="u8"   };
static nnc_type i16_type  __attribute__((unused)) = { .size=sizeof(nnc_i16), .kind=TYPE_PRIMITIVE_I16, .repr="i16"  };
static nnc_type u16_type  __attribute__((unused)) = { .size=sizeof(nnc_u16), .kind=TYPE_PRIMITIVE_U16, .repr="u16"  };
static nnc_type i32_type  __attribute__((unused)) = { .size=sizeof(nnc_i32), .kind=TYPE_PRIMITIVE_I32, .repr="i32"  };
static nnc_type u32_type  __attribute__((unused)) = { .size=sizeof(nnc_u32), .kind=TYPE_PRIMITIVE_U32, .repr="u32"  };
static nnc_type f32_type  __attribute__((unused)) = { .size=sizeof(nnc_f32), .kind=TYPE_PRIMITIVE_F32, .repr="f32"  };
static nnc_type i64_type  __attribute__((unused)) = { .size=sizeof(nnc_i64), .kind=TYPE_PRIMITIVE_I64, .repr="i64"  };
static nnc_type u64_type  __attribute__((unused)) = { .size=sizeof(nnc_u64), .kind=TYPE_PRIMITIVE_U64, .repr="u64"  };
static nnc_type f64_type  __attribute__((unused)) = { .size=sizeof(nnc_f64), .kind=TYPE_PRIMITIVE_F64, .repr="f64"  };
static nnc_type void_type __attribute__((unused)) = { .size=0,               .kind=TYPE_VOID,      .repr="void" };
static nnc_type unknown_type __attribute__((unused)) = { .size=0,            .kind=TYPE_UNKNOWN,   .repr="unknown" };

nnc_type* nnc_type_new(const nnc_str repr);
nnc_type* nnc_ptr_type_new(nnc_type* base);
nnc_type* nnc_arr_type_new(nnc_type* base);
nnc_type* nnc_fn_type_new();
nnc_type* nnc_enum_type_new(); 
nnc_type* nnc_union_type_new();
nnc_type* nnc_struct_type_new();

nnc_str nnc_type_tostr(const nnc_type* type);

#endif