#ifndef __STRUTIL_LOL___
#define __STRUTIL_LOL___

#include <cstddef>
#include <iostream>
#include <ostream>
#include <stack>
#include <string>

typedef char const cchar;

extern int const EXIT_RESTART;

class TextPos
{
private:
  cchar* string;
  unsigned int line;
  unsigned int column;

public:
  explicit TextPos(cchar* begin);

  /**
   * Returns the byte (character) at the current position.
   */
  char operator*(void);

  /**
   * Returns the string ptr to the current position.
   */
  cchar* ptr(void) const;
  /**
   * Returns the next BYTE of the string.
   */
  char next();

  /**
   * Moves the Text_pos to the next non-whitespace character
   * After this function *string can point to EOS.
   */
  void skip_whitespace(void);
  /**
   * The same inverted.
   */
  void skip_nonspace(void);

  /**
   * Returns the string representation of the position -> ":line:column:"
   */
  std::string to_string(void);

  unsigned int get_line(void) const;
  unsigned int get_column(void) const;

  /**
   * Moves the pointer forward
   */
  void offset(size_t);

  /**
   * Parses a number at the current position and sets the pposition
   * to the first byte after the number.
   */
  double parse_num(void);
};

std::ostream& operator<<(std::ostream&, TextPos const& pos);

class TraceCeption
{
protected:
  TextPos const position;
  std::stack<std::string> stacktrace;

public:
  TraceCeption(TextPos pos, std::string msg) : position(pos)
  {
    stacktrace.push(msg);
  }

  // TODO copy constr

  virtual ~TraceCeption() {}

  void push_stack(std::string msg) { stacktrace.push(msg); }

  void printStackTrace(std::ostream& out = std::cerr)
  {
    while(!stacktrace.empty())
    {
      out << position << stacktrace.top() << std::endl;
      stacktrace.pop();
    }
  }
};

/**
 * Given a pointer to the starting " of the string, returns a C++ string
 * and offsets the pointer to the next character after the
 */
std::string parse_escaped_string(TextPos& pos);

bool hasInput(int fd, int microsec);

void store_string(char* dst, size_t dst_size, char* src, size_t src_size);

bool load_file(std::string& name, std::string& content);

std::string mkTerminalCmd(std::string);

std::ostream&
print_time(std::ostream& out, struct tm* ptm, char const* const format);

class Logger
{
private:
  std::string const logname;
  std::ostream& ostream;

public:
  Logger(std::string lname, std::ostream& ostr);
  virtual ~Logger() {}

  std::ostream& log(void);
  void log_errno(void);
};

#endif
