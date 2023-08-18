#ifndef _NNC_FORMAT_H
#define _NNC_FORMAT_H

#include <stdio.h>
#include <stdarg.h>
#include "nnc_arena.h"

#define sformat(format, ...) nnc_sformat(format, __VA_ARGS__)

#define FORMAT_BUF_SIZE 2048

char* nnc_sformat(const char* format, ...);
nnc_bool nnc_sequal(const char* s1, const char* s2);
char* nnc_sdup(const char* s);

#endif