#ifndef __STRUTIL_LOL___
#define __STRUTIL_LOL___

#include<string>
#include<cstddef>
#include<ostream>

typedef char const cchar;

/**
 * Moves the pointer to the next non-whitespace character
 * After this function *string can point to EOS.
 */
void skipWhitespaces(cchar*& string);
/**
 * The same inverted.
 */
void skipNonWhitespace(cchar*& string);

bool hasInput(int fd, int microsec);

void storeString(char* dst, size_t dst_size, char* src, size_t src_size);

bool loadFile(std::string& name, std::string& content);

std::string execute(std::string& command);

class Logger
{
private:
  std::string const logname;
  std::ostream& ostream;
public:
  Logger(std::string lname, std::ostream& ostr);
  ~Logger();
  
  std::ostream& log(void);
};

#endif