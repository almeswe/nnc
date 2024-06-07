#ifndef __NNC_x86_64_GEN_STORAGE_ALLOCATOR_H__
#define __NNC_x86_64_GEN_STORAGE_ALLOCATOR_H__

#include "nnc_3a.h"

extern const char* glob_reg_str[];

/**
 * @brief Readable type for describing memory offset.
 */
typedef nnc_i16 nnc_mem;

/**
 * @brief Readable type for describing Data Segment Label.
 */
typedef char* nnc_dsl;

/**
 * @brief Readable type for describing immediate value.
 */
typedef union _nnc_imm {
    nnc_u64 u;
    nnc_i64 d;
} nnc_imm;

/**
 * @brief x86_64 Registers.
 */
typedef enum _nnc_x86_64_reg {
    /* General purpose x86_64 (used) registers */
    R_RAX, R_RBX,
    R_RCX, R_RDX,
    R_RSI, R_RDI,
    R_R8,  R_R9,
    R_R10, R_R11,
    R_R12, R_R13,
    R_R14, R_R15,
    /* SIMD extension registers */
    R_XMM0, R_XMM1, R_XMM2, R_XMM3,
    R_XMM4, R_XMM5, R_XMM6, R_XMM7
} nnc_reg;

/**
 * @brief Location kind. 
 */
typedef enum _nnc_loc_where {
    L_IMM   = 0b00001, // Immediate location, represents 
                       // operand as contant value.
    L_REG   = 0b00010, // Register.
    L_DATA  = 0b00100, // Data segment. Strings and global
                       // variables are stored here.
    L_NONE  = 0b01000, // No location. Indicator for exceptional situation.
    L_STACK = 0b10000, // Stack memory.

    L_MEM   = L_STACK | L_DATA // General memory location.
} nnc_loc_where;

/**
 * @brief Location. Represents placement of the 
 *  quad address as assembly instruction operand.
 */
typedef struct _nnc_loc {
    nnc_bool deref; // Indicates if this location 
                    // should be dereferenced.
    nnc_bool has_offset; // Indicates if this location
                         // has `offset` keyword
    union _nnc_loc_exact {
        nnc_mem mem; // Exact offset in stack memory. 
                     // Can be positive or negative.
        nnc_reg reg; // Exact register.
        nnc_dsl dsl; // Exact data segment label.
        nnc_imm imm; // Exact immediate value.
    } exact;
    nnc_loc_where where;  // where location is presented
    const nnc_type* type; // type of the address
} nnc_loc, nnc_location;

/**
 * @brief Class of the function parameter according
 *  to x86_64 System V ABI.
 */
typedef enum _nnc_pclass {
    C_NONE,
    C_INTEGER,
    C_SSE,
    C_SSEUP,
    C_x87,
    C_x87UP,
    C_COMPLEX_x87,
    C_NO_CLASS,
    C_MEMORY
} nnc_pclass;

/**
 * @brief Structure that holds state for allocating 
 *  parameter classes for function parameters.
 * todo: Only INTEGER and MEMORY classes are supported.
 */
typedef struct _nnc_pclass_state {
    nnc_u8 int_iter;
    nnc_u8 sse_iter;
    nnc_bool was_init;
    // ...
} nnc_pclass_state;

/**
 * @brief Pushes register. It means that from this moment it can be allocated for use.
 * @param reg Register to be pushed.
 */
void nnc_push_reg(
    const nnc_reg reg
);

/**
 * @brief Reserves register. If specified register is in use, pushes it.
 * To unreserve register, call `nnc_unreserve_reg`.
 * @param reg Register to be reserved.
 * @return Register was pushed or not.
 */
nnc_bool nnc_reserve_reg(
    const nnc_reg reg
);

/**
 * @brief Unreserves register.
 * @param reg Register to be unreserved.
 */
void nnc_unreserve_reg(
    const nnc_reg reg
);

/**
 * @brief Gets associated location with some address.
 * @param addr Address for which to search.
 * @return Some location or `NULL`.
 */
const nnc_loc* nnc_get_loc(
    const nnc_3a_addr* addr
);

/**
 * @brief Gets parameter class of the parameter's type.
 *  Can be used for generating function call and storing function
 *  parameters inside callee function. 
 * @param p_type Type of the parameter.
 * @param state State used accross different `nnc_get_pclass` calls.
 * @return Class of the parameter.
 */
nnc_pclass nnc_get_pclass(
    const nnc_type* p_type,
    nnc_pclass_state* state
);

/**
 * @brief Stores any kind of address in some location.
 * @param addr Address to be stored.
 * @return Location. If `.where` is L_NONE, nothing happened.
 *  (but see the implementation, may be this is a bug)
 */
nnc_loc nnc_store(
    const nnc_3a_addr* addr
);

/**
 * @brief Stores function parameter (or argument in a call) 
 * @param addr Address to be stored.
 * @param pclass Class of the parameter. 
 *  Helps to correctly store the address.  
 * @return Location. If `.where` is L_NONE, nothing happened.
 *  (but see the implementation, may be this is a bug)
 */
nnc_loc nnc_store_param(
    const nnc_3a_addr* addr,
    const nnc_pclass pclass
);

#endif