#include "nnc_literal.h"

/**
 * @brief Parses string representation of float number, and 
 *  determines what suffix was specified within this number.
 * @param repr String representation of float number.
 * @return `nnc_dbl_suffix` value, or SUFFIX_NONE.
 */
nnc_dbl_suffix nnc_get_dbl_suffix(const char* repr) {
    nnc_byte suffix[3] = { 0 };
    nnc_bool met_f = false;
    nnc_u64 size = strlen(repr);
    // suffix will be read from `repr` in reverse order.
    for (nnc_u64 i = 0; i < size; i++) {
        // iterate until we met `f`, `F` or `i` greater
        // than max addressable index of `suffix`. 
        if (met_f || i > 2) {
            break;
        }
        suffix[2-i] = repr[size-1-i];
        switch (suffix[2-i]) {
            case 'f': case 'F':
                met_f = true;
                break;
        }
    }
    // if `f` or `F` was not found, it means that there
    // are no any suffix at all
    if (!met_f) {
        return SUFFIX_NONE;
    }
    // otherwise determine exact suffix.
    // suffix[0] is 'f' or 'F'
    if (suffix[1] == '3' && 
        suffix[2] == '2') {
        return SUFFIX_F32;
    }
    if (suffix[1] == '6' && 
        suffix[2] == '4') {
        return SUFFIX_F64;
    }
    // if it's not f32 or f64, and lexical analysis validator
    // accepted this, it means that bug was detected, report this.
    nnc_abort_no_ctx(sformat("nnc_get_dbl_suffix: "
        "bug detected. suffix: %s\n", suffix));
    return SUFFIX_NONE;
}

/**
 * @brief Checks float number for overflow, in bounds of it's suffix.
 * @param literal Instance of `nnc_dbl_literal`.
 * @return Exact the same literal as it was specified in argument.
 * @throw NNC_OVERFLOW in case of overflow.
 */
nnc_dbl_literal* nnc_dbl_check_overflow(nnc_dbl_literal* literal) {
    //todo: weird thing with f32 overflow (it does not occurs)
    static const nnc_bounds bounds[] = {
        [SUFFIX_F32]  = { .min.f = FLT_MIN, .max.f = FLT_MAX  },
        [SUFFIX_F64]  = { .min.f = DBL_MIN, .max.f = DBL_MAX  },
    };
    nnc_bounds current = bounds[literal->suffix];
    if (current.min.f > literal->exact ||
        current.max.f < literal->exact) {
        THROW(NNC_OVERFLOW, sformat("this value out of bounds "
            "of it's type \'%f\'.\n", literal->exact));
    }
    return literal;
}

/**
 * @brief Allocates & initializes new instance of `nnc_dbl_literal`.
 * @param repr String representation of the float number which will be parsed.
 * @return Allocated & initialized instance of `nnc_dbl_literal`.
 */
nnc_dbl_literal* nnc_dbl_new(const char* repr) {
    nnc_byte repr_buf[512] = { 0 };
    nnc_u64 repr_size = strlen(repr);
    nnc_dbl_literal* ptr = new(nnc_dbl_literal);
    ptr->suffix = nnc_get_dbl_suffix(repr);
    // if there is no suffix, set f64 as default.
    if (ptr->suffix == SUFFIX_NONE) {
        ptr->suffix = SUFFIX_F64;
    }
    // otherwise decrease size of string representatin
    // to remove suffix from copying for correct parsing
    else {
        repr_size -= 3;
    }
    // copy cropped representation to buffer.
    memcpy(repr_buf, repr, repr_size);
    ptr->exact = atof(repr_buf);
    return ptr;
}

/**
 * @brief Parses string representation of integral number, and 
 *  determines what suffix was specified within this number.
 * @param repr String representation of float number.
 * @return `nnc_int_suffix` value, or SUFFIX_NONE.
 */
nnc_int_suffix nnc_get_int_suffix(const char* repr) {
    nnc_byte suffix[3] = { 0 };
    nnc_bool met_i_or_u = false;
    nnc_u64 size = strlen(repr);
    nnc_byte* suffix_ptr = suffix;
    // suffix will be read from `repr` in reverse order.
    for (nnc_u64 i = 0; i < size; i++) {
        if (met_i_or_u || i > 2) {
            break;
        }
        suffix[2-i] = repr[size-1-i];
        switch (suffix[2-i]) {
            case 'i': case 'I':
            case 'u': case 'U':
                met_i_or_u = true;
                break;
        }
    }
    // if 'u' or 'i' was not met, it means that
    // there was no suffix at all.
    if (!met_i_or_u) {
        return SUFFIX_NONE;
    }
    // if first character if `suffix` is `0`,
    // it means that length of the suffix is 2
    // and suffix must be shifted by one character to
    // remove any offset.
    if (suffix[0] == '\0') {
        suffix_ptr++;
    }
    nnc_bool is_unsigned = (suffix_ptr[0] == 'u' || suffix_ptr[0] == 'U');
    // then determine exact suffix.
    if (suffix_ptr[1] == '8') {
        return is_unsigned ? 
            SUFFIX_U8 : SUFFIX_I8;
    }
    if (suffix_ptr[1] == '1' && 
        suffix_ptr[2] == '6') {
        return is_unsigned ? 
            SUFFIX_U16 : SUFFIX_I16;
    }
    if (suffix_ptr[1] == '3' && 
        suffix_ptr[2] == '2') {
        return is_unsigned ?
            SUFFIX_U32 : SUFFIX_I32;
    }
    if (suffix_ptr[1] == '6' && 
        suffix_ptr[2] == '4') {
        return is_unsigned ? 
            SUFFIX_U64 : SUFFIX_I64;
    }
    // if it's not suffix from above, and lexical analysis validator
    // accepted this, it means that bug was detected, report this.
    nnc_abort_no_ctx(sformat("nnc_get_int_suffix: "
        "bug detected. suffix: %s\n", suffix));
    return SUFFIX_NONE;
}

/**
 * @brief Checks integral number for overflow, in bounds of it's suffix.
 * @param literal Instance of `nnc_int_literal`.
 * @return Exact the same literal as it was specified in argument.
 * @throw NNC_OVERFLOW in case of overflow.
 */
nnc_int_literal* nnc_int_check_overflow(nnc_int_literal* literal) {
    static const nnc_bounds bounds[] = {
        [SUFFIX_I8]  = { .min.d = INT8_MIN,  .max.d = INT8_MAX  },
        [SUFFIX_I16] = { .min.d = INT16_MIN, .max.d = INT16_MAX },
        [SUFFIX_I32] = { .min.d = INT32_MIN, .max.d = INT32_MAX },
        [SUFFIX_I64] = { .min.d = INT64_MIN, .max.d = INT64_MAX },
        [SUFFIX_U8]  = { .min.u = 0, .max.u = UINT8_MAX  },
        [SUFFIX_U16] = { .min.u = 0, .max.u = UINT16_MAX },
        [SUFFIX_U32] = { .min.u = 0, .max.u = UINT32_MAX },
        [SUFFIX_U64] = { .min.u = 0, .max.u = UINT64_MAX },
    };
    nnc_bounds current = bounds[literal->suffix];
    if (literal->is_signed) {
        if (current.min.d > literal->exact.d ||
            current.max.d < literal->exact.d) {
            THROW(NNC_OVERFLOW, sformat("this value out of bounds "
                "of it's type \'%ld\'.\n", literal->exact.d));
        }
    }
    else {
        if (current.min.u > literal->exact.u ||
            current.max.u < literal->exact.u) {
            THROW(NNC_OVERFLOW, sformat("this value out of bounds "
                "of it's type \'%lu\'.\n", literal->exact.u));
        }
    }
    return literal;
}

/**
 * @brief Allocates & initializes new instance of `nnc_int_literal`.
 * @param repr String representation of the float number which will be parsed.
 * @return Allocated & initialized instance of `nnc_int_literal`.
 * @throw NNC_OVERFLOW in case of `errno == ENOENT` after call to `strtoll` or `strtoull`.
 */
nnc_int_literal* nnc_int_new(const char* repr) {
    nnc_byte repr_buf[512] = { 0 };
    nnc_int_literal* ptr = new(nnc_int_literal);
    ptr->suffix = nnc_get_int_suffix(repr);
    // determine base by it's shorthand
    switch (repr[0]) {
        case 'x': ptr->base = 16; break;
        case 'o': ptr->base = 8;  break;
        case 'b': ptr->base = 2;  break;
        default:  ptr->base = 10; break;
    }
    // if base is not 10, it means that 
    // representation has 'b', 'x' or 'o' at the beginning
    // it must be skipped for correct parsing.
    if (ptr->base != 10) {
        repr = repr + 1;
    }
    nnc_u64 repr_size = strlen(repr);
    // then suffix must be removed from the end
    // for correct parsing.
    if (ptr->suffix == SUFFIX_I8 ||
        ptr->suffix == SUFFIX_U8) {
        repr_size -= 2;
    }
    else {
        if (ptr->suffix != SUFFIX_NONE) {
            // decrease size only in case when returned
            // suffix is not none
            repr_size -= 3;
        }
    }
    // then copy cropped string representation into buffer.
    memcpy(repr_buf, repr, repr_size);
    // if there was no suffix specified, set i32 as default.
    if (ptr->suffix == SUFFIX_NONE) {
        ptr->suffix = SUFFIX_I32;
    }
    ptr->is_signed = 
        ptr->suffix >= SUFFIX_I8 && 
        ptr->suffix <= SUFFIX_I64;
    if (ptr->is_signed) {
        ptr->exact.d = strtoll(repr_buf, NULL, ptr->base);
    }
    else {
        ptr->exact.u = strtoull(repr_buf, NULL, ptr->base);
    }
    if (errno == ENOENT) {
        THROW(NNC_OVERFLOW, sformat("this value out of bounds "
            "of it's type \'%s\'.\n", repr_buf));
    }
    if (errno != 0) {
        nnc_abort_no_ctx(sformat("nnc_int_new [errno: %d]: cannot"
            " instanciate with \'%s\'\n", errno, repr_buf));
    }
    return nnc_int_check_overflow(ptr);
}

nnc_chr_literal* nnc_chr_new(const char* repr) {
    THROW(NNC_UNINPLEMENTED, "nnc_chr_new\n");
    return NULL;
}

nnc_str_literal* nnc_str_new(const char* repr) {
    THROW(NNC_UNINPLEMENTED, "nnc_str_new\n");
    return NULL;
}