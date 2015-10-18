#include<iostream>
#include<fstream>
#include<string>

#include<sys/time.h>
#include<sys/types.h>
#include<unistd.h>

#include"strUtil.hpp"

using namespace std;

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



