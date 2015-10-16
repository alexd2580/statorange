#ifndef __STRUTIL_LOL___
#define __STRUTIL_LOL___

#include<stddef.h>

/**
 * Moves the pointer to the next non-whitespace character
 * After this function *string can point to EOS.
 */
void skipWhitespaces(char** string);
/**
 * The same inverted.
 */
void skipNonWhitespace(char** string);

int hasInput(int fd, int microsec);

void storeString(char* dst, size_t dst_size, char* src, size_t src_size);

#endif
