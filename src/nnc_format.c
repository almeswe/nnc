#include "nnc_format.h"

#define FORMAT_BUF_SIZE 2048

/**
 * @brief Formats `format` string with passed arguments.
 * @param format String to be formatted.
 * @param __VA_ARGS__ Arguments which will be substituted accroding to format variables.
 * @return Arena-allocated formatted string.
 */
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

/**
 * @brief Compares two zero terminated strings.
 * @param s1 First string.
 * @param s2 Second string.
 * @return `true` if they are equal and non NULL, otherwise `false` is returned. 
 */
nnc_bool nnc_strcmp(const char* s1, const char* s2) {
	if (s1 == NULL || s2 == NULL) {
		return false;
	}
	return strcmp(s1, s2) == 0;
}

/**
 * @brief Makes duplicate of a string on heap.
 * @param s String to be duplicated.
 * @return Pointer to heap allocated string.
 */
char* nnc_strdup(const char* s) {
	nnc_u64 size = strlen(s);
	char* dup = (char*)nnc_cnew(char, size+1);
	strcpy(dup, s);
	return dup;
}