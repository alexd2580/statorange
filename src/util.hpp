#ifndef __STRUTIL_LOL___
#define __STRUTIL_LOL___

#include <cstddef>
#include <iostream>
#include <ostream>
#include <stack>
#include <string>
#include <memory>
#include <mutex>

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

  TextPos(TextPos const& other) = default;
  TextPos(TextPos&& other) = default;
  TextPos& operator=(TextPos const& other) = default;
  TextPos& operator=(TextPos&& other) = default;

  virtual ~TextPos(void) = default;

  /**
   * Returns the byte (character) at the current position.
   */
  char operator*(void) const;

  /**
   * Returns the string ptr to the current position.
   */
  cchar* ptr(void) const;

  /**
   * Returns the next BYTE of the string.
   */
  char next(void);

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
  std::string to_string(void) const;

  unsigned int get_line(void) const;
  unsigned int get_column(void) const;

  /**
   * Moves the pointer forward
   */
  void offset(size_t);
  TextPos& operator+=(size_t off);

  /**
   * Parses a number at the current position and sets the pposition
   * to the first byte after the number.
   */
  double parse_num(void);

  /**
   * Given a pointer to the starting " of the string, returns a C++ string
   * and offsets the pointer to the next character after the "
   */
  std::string parse_escaped_string(void);
};

std::ostream& operator<<(std::ostream&, TextPos const& pos);

bool hasInput(int fd, int microsec);

void store_string(char* dst, size_t dst_size, char* src, size_t src_size);

bool load_file(std::string& name, std::string& content);

class LoggerManager
{
private:
  static std::unique_ptr<LoggerManager> instance;

  std::ostream& log_stream;
  std::mutex log_mutex;
  LoggerManager(std::ostream& ostr);

public:
  static void set_stream(std::ostream& ostr);
  static std::ostream& get_stream(void);
  static std::mutex& get_mutex(void);
};

std::ostream&
print_time(std::ostream& out, struct tm* ptm, char const* const format);

class Logger
{
private:
  std::string const logname;
  std::ostream& ostream;
  std::mutex& log_mutex;

public:
  Logger(std::string lname);
  virtual ~Logger() = default;

  std::ostream& log(void);
  void log_errno(void);
};

#include<vector>
#include<functional>
void try_all(std::vector<std::function<void(void)>>);

#endif
