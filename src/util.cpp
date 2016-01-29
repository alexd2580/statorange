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

__attribute__((pure)) char TextPos::operator*(void) { return *string; }

__attribute__((pure)) cchar* TextPos::ptr(void) const { return string; }

char TextPos::next(void)
{
  string++;
  char c = *string;
  line += c == '\n' ? 1 : 0; // just to make sure
  column = c == '\n' ? 1 : column + 1;
  return c;
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

string TextPos::to_string(void)
{
  const std::string colon(":");
  return colon + std::to_string(line) + colon + std::to_string(column) + colon;
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
    throw TraceCeption(*this, "Could not convert string to number");
  column += (unsigned int)(endptr - string);
  string = endptr;
  return n;
}

void TextPos::offset(size_t off)
{
  for(size_t i = 0; i < off; i++)
    (void)next();
}

std::ostream& operator<<(std::ostream& out, TextPos const& pos)
{
  return pos.ptr() == nullptr ? out : out << ':' << pos.get_line() << ':'
                                          << pos.get_column() << ':';
}

string parse_escaped_string(TextPos& pos)
{
  char c = *pos;
  if(c != '"')
    throw TraceCeption(pos, string("Expected [\"], got: [") + (char)c + "].");

  std::string res;
  c = pos.next();
  cchar* start = pos.ptr();

  while(c != '"' && c != '\0')
  {
    if(c == '\\') // if an escaped char is found
    {
      res += string(start, (size_t)(pos.ptr() - start)); // push current part
      c = pos.next();
      if(c == '\0')
        throw TraceCeption(
            pos, string("Unexpected EOS when parsing escaped character."));
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
      start = pos.ptr() + 1;
    }
    c = pos.next();
  }

  if(c == '\0')
    throw TraceCeption(pos, string("Unexpected EOS"));

  res += std::string(start, (size_t)(pos.ptr() - start));

  // checking " not required here
  (void)pos.next();
  return res;
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

bool execute(string const& command, string& res, bool& die)
{
  int fd[2];
  if(pipe(fd) != 0)
  {
    perror("PIPE failed");
    return false;
  }

  int childpid = fork();
  if(childpid == -1) // Fail
  {
    perror("FORK failed");
    return false;
  }
  else if(childpid == 0) // I am child
  {
    close(1); // stdout
    dup2(fd[1], 1);
    close(fd[0]);
    execlp(shell_file_loc.c_str(),
           shell_file_loc.c_str(),
           "-c",
           command.c_str(),
           nullptr);
    // child has been replaced by shell command
  }

  close(fd[1]);
  wait(nullptr);

  res = "";
  char buffer[500];

  errno = EINTR;
  while((errno == EAGAIN || errno == EINTR) && !die)
  {
    errno = 0;
    ssize_t n = 0;
    n = read(fd[0], buffer, 500);
    if(n == 0)
      break;
    buffer[n] = '\0';
    res += buffer;
  }

  close(fd[0]);
  return true;
}

string mkTerminalCmd(string s) { return terminal_cmd + " " + s + "&"; }

Logger::Logger(string lname, std::ostream& ostr) : logname(lname), ostream(ostr)
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
  time_t tt = system_clock::to_time_t(system_clock::now());
  struct tm* ptm = localtime(&tt);
  char const* const log_time_format = "%Y-%m-%d %H:%M ";
  return print_time(ostream, ptm, log_time_format) << logname << ' ';
}

#include <cerrno>
#include <cstring>
void Logger::log_errno(void)
{
  log() << "errno = " << errno << endl;
  log() << "Error description is : " << strerror(errno) << endl;
}
