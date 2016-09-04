#include <fstream>
#include <iostream>
#include <string>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "util.hpp"

using namespace std;

int const EXIT_RESTART = 4;

string const shell_file_loc = "/bin/sh";
string const terminal_cmd = "x-terminal-emulator -e";

TextPos::TextPos(cchar* begin)
{
  string = begin;
  line = 1;
  column = 1;
}

__attribute__((pure)) char TextPos::operator*(void) const { return *string; }

__attribute__((pure)) cchar* TextPos::ptr(void) const { return string; }

char TextPos::next(void)
{
  char c = *string;
  line += c == '\n' ? 1 : 0; // just to make sure
  column = c == '\n' ? 1 : column + 1;
  string++;
  return *string;
}

void TextPos::skip_whitespace(void)
{
  char c = *string;
  while((c == ' ' || c == '\n' || c == '\t') && c != '\0')
    c = next();
}

void TextPos::skip_nonspace(void)
{
  char c = *string;
  while(c != ' ' && c != '\n' && c != '\t' && c != '\0')
    c = next();
}

string TextPos::to_string(void) const
{
  return ":" + std::to_string(line) + ":" + std::to_string(column) + ":";
}

__attribute__((pure)) unsigned int TextPos::get_line(void) const
{
  return line;
}

__attribute__((pure)) unsigned int TextPos::get_column(void) const
{
  return column;
}

double TextPos::parse_num(void)
{
  char* endptr;
  double n = strtod(string, &endptr);
  if(endptr == string)
    throw(TraceCeption(*this, "Could not convert string to number"));
  column += (unsigned int)(endptr - string);
  string = endptr;
  return n;
}

void TextPos::offset(size_t off)
{
  for(size_t i = 0; i < off; i++)
    (void)next();
}

TextPos& TextPos::operator+=(size_t off)
{
  offset(off);
  return *this;
}

string TextPos::parse_escaped_string(void)
{
  char c = *string;
  if(c != '"')
    throw TraceCeption(*this, std::string("Expected [\"], got: [") + c + "].");

  c = next();
  cchar* start = string;
  std::string res("");

  while(c != '"' && c != '\0')
  {
    if(c == '\\') // if an escaped char is found
    {
      size_t off = (size_t)(string - start);
      res += std::string(start, off); // push current part
      column += off + 1;
      c = next();
      if(c == '\0')
        throw TraceCeption(
            *this,
            std::string("Unexpected EOS when parsing escaped character."));
      switch(c)
      {
      case 'n':
        res += '\n';
        break;
      case 't':
        res += '\t';
        break;
      case 'r':
        res += '\r';
        break;
      default:
        res += c;
        break;
      }
      start = string + 1;
    }
    string++;
    c = *string;
  }

  if(c == '\0')
    throw TraceCeption(*this, std::string("Unexpected EOS"));

  size_t off = (size_t)(string - start);
  res += std::string(start, off);
  column += off + 1;

  // checking " not required here
  string++;
  return res;
}

ostream& operator<<(ostream& out, TextPos const& pos)
{
  return pos.ptr() == nullptr ? out : out << ':' << pos.get_line() << ':'
                                          << pos.get_column() << ':';
}

bool hasInput(int fd, int microsec)
{
  int retval;

  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(fd, &rfds);

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = microsec;

  retval = select(fd + 1, &rfds, nullptr, nullptr, &tv);

  return retval > 0;
}

bool load_file(string& name, string& content)
{
  fstream file(name.c_str(), fstream::in);
  if(file.is_open())
  {
    // We use the standard getline function to read the file into
    // a std::string, stoping only at "\0"
    getline(file, content, '\0');
    bool ret = !file.bad();
    file.close();
    return ret;
  }

  return false;
}

/******************************************************************************/
/******************************************************************************/

std::unique_ptr<LoggerManager> LoggerManager::instance;

LoggerManager::LoggerManager(ostream& ostr) : log_stream(ostr) {}

void LoggerManager::set_stream(ostream& ostr)
{
  instance = std::unique_ptr<LoggerManager>(new LoggerManager(ostr));
}

ostream& LoggerManager::get_stream(void) { return instance->log_stream; }
mutex& LoggerManager::get_mutex(void) { return instance->log_mutex; }

Logger::Logger(string lname)
    : logname(lname), ostream(LoggerManager::get_stream()),
      log_mutex(LoggerManager::get_mutex())
{
}

#include <chrono>  // std::chrono::system_clock
#include <iomanip> // std::put_time

using std::chrono::system_clock;

ostream& print_time(ostream& out, struct tm* ptm, char const* const format)
{
#if __GLIBCXX__ >= 20151204
  return out << put_time(ptm, format);
#else
  char str[256];
  if(strftime(str, 256, format, ptm))
    return out << str;
  else
    return out << "???";
#endif
}

ostream& Logger::log(void)
{
  log_mutex.lock();
  time_t tt = system_clock::to_time_t(system_clock::now());
  struct tm* ptm = localtime(&tt);
  char const* const log_time_format = "%Y-%m-%d %H:%M ";
  auto& res = print_time(ostream, ptm, log_time_format) << logname << ' ';
  log_mutex.unlock();
  return res;
}

#include <cerrno>
#include <cstring>

void Logger::log_errno(void)
{
  log() << "errno = " << errno << endl;
  log() << "Error description is : " << strerror(errno) << endl;
}
