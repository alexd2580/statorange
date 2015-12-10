#define _POSIX_C_SOURCE 200809L

#include<iostream>
#include<fstream>
#include<string>

#include<sys/time.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<unistd.h>

#include"util.hpp"

using namespace std;

int const EXIT_RESTART = 4;

string shell_file_loc = "/bin/sh";
string terminal_cmd = "x-terminal-emulator -e";

void skipWhitespaces(char const*& string)
{
  char c = *string;
  while((c==' '||c=='\n'||c=='\t') && c!='\0')
  {
    string++;
    c = *string;
  }
  return;
}

void skipNonWhitespace(char const*& string)
{
  char c = *string;
  while(c!=' ' && c!='\n' && c!='\t' && c!='\0')
  {
    string++;
    c = *string;
  }
  return;
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
  
  retval = select(fd+1, &rfds, nullptr, nullptr, &tv);

  return retval > 0;
}

bool loadFile(string& name, string& content)
{
    // We create the file object, saying I want to read it
    fstream file(name.c_str(), fstream::in);

    // We verify if the file was successfully opened
    if(file.is_open())
    {
        // We use the standard getline function to read the file into
        // a std::string, stoping only at "\0"
        getline(file, content, '\0');

        // We return the success of the operation
        return !file.bad();
    }

    // The file was not successfully opened, so returning false
    return false;
}

string execute(string const& command)
{
  int fd[2];
  pipe(fd);
  int childpid = fork();
  if(childpid == -1) //Fail
  {
     cerr << "FORK failed" << endl;
     return 0;
  } 
  else if(childpid == 0) //I am child
  {
    close(1); //stdout
    dup2(fd[1], 1);
    close(fd[0]);
    execlp(shell_file_loc.c_str(), shell_file_loc.c_str(), "-c", command.c_str(), nullptr);
    //child has been replaced by shell command
  }

  wait(nullptr);
  
  string res = "";
  char buffer[500];
  
  errno = EINTR;
  while(errno == EAGAIN || errno == EINTR)
  {
    errno = 0;
    ssize_t n = read(fd[0], buffer, 500);
    if(n == 0)
      break;
    buffer[n] = '\0';
    res += buffer;
  }
  
  return res;
}

string mkTerminalCmd(string s)
{
  return terminal_cmd + " " + s + "&";
}

Logger::Logger(string lname, std::ostream& ostr) :
  logname(lname), ostream(ostr)
{
}

Logger::~Logger()
{
}
  
ostream& Logger::log(void)
{
  return ostream << logname << ' ';
}


