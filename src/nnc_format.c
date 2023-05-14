#include "nnc_format.h"

char* nnc_sformat(const char* format, ...) {
    va_list args;
	va_start(args, format);
    nnc_u64 written = 0;
	char formatbuf[FORMAT_BUF_SIZE] = {0};
	if ((written = vsprintf(formatbuf, format, args)) < 0) {
		perror("nnc_sformat:vsprintf"); exit(EXIT_FAILURE);
	}
	va_end(args);	
    char* formatstr = (char*)nnc_alloc(written + 1);
    strcpy(formatstr, formatbuf);
	return formatstr;
}