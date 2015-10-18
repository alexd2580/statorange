#ifndef __STRUTIL_LOL___
#define __STRUTIL_LOL___

#include<string>
#include<cstddef>

/**
 * Moves the pointer to the next non-whitespace character
 * After this function *string can point to EOS.
 */
void skipWhitespaces(char const*& string);
/**
 * The same inverted.
 */
void skipNonWhitespace(char const*& string);

bool hasInput(int fd, int microsec);

void storeString(char* dst, size_t dst_size, char* src, size_t src_size);

bool loadFile(std::string& name, std::string& content);

#endif
