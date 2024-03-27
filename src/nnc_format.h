#ifndef __NNC_FORMAT_H__
#define __NNC_FORMAT_H__

#include <stdio.h>
#include <stdarg.h>
#include "nnc_arena.h"

#define FORMAT_BUF_SIZE 2048

/**
 * @brief Macro over `nnc_sformat`.
 *  See `nnc_sformat` for more details.
 */
#define sformat(format, ...) nnc_sformat(format, __VA_ARGS__)

/**
 * @brief Formats `format` string with passed arguments.
 * @param format String to be formatted.
 * @param __VA_ARGS__ Arguments which will be substituted accroding to format variables.
 * @return Arena-allocated formatted string.
 */
char* nnc_sformat(
    const char* format, ...
);

/**
 * @brief Compares two zero terminated strings.
 * @param s1 First string.
 * @param s2 Second string.
 * @return `true` if they are equal and non NULL, otherwise `false` is returned. 
 */
nnc_bool nnc_strcmp(
    const char* s1,
    const char* s2
);

/**
 * @brief Makes duplicate of a string on heap.
 * @param s String to be duplicated.
 * @return Pointer to heap allocated string.
 */
char* nnc_strdup(
    const char* s
);

#endif