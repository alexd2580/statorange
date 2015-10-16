#include"strUtil.h"

void skipWhitespaces(char** string)
{
  char c = **string;
  while((c==' '||c=='\n'||c=='\t') && c!='\0')
  {
    *string = *string+1;
    c = **string;
  }
  return;
}

void skipNonWhitespace(char** string)
{
  char c = **string;
  while(c!=' ' && c!='\n' && c!='\t' && c!='\0')
  {
    *string = *string+1;
    c = **string;
  }
  return;
}


#include<sys/time.h>
#include<sys/types.h>
#include<unistd.h>

int hasInput(int fd, int microsec)
{
  int retval;

  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(fd, &rfds);

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = microsec;
  
  retval = select(fd+1, &rfds, NULL, NULL, &tv);

  switch(retval)
  {
  case -1: // error
    return 0;
  case 0: // timeout
    return 0;
  default: // hasInput
    return 1;
  }
}

#include<string.h>

/**
 * Stores a string to dst. Converts the string to null-terminated.
 * If the string is too long it is shortened.
 * Produces garbage if the buffers are overlapping
 */
void storeString(char* dst, size_t dst_size, char* src, size_t src_size)
{
  if(src_size < dst_size) // fits
  {
    memcpy(dst, src, src_size*sizeof(char));
    dst[src_size] = '\0';
  }
  else //doesn't fit
  {
    memcpy(dst, src, (size_t)(dst_size-6)*sizeof(char));
    memcpy(dst+dst_size-6, "[...]\0", 6);
  }
}



