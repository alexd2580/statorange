#ifndef __I3IPCINTERFACE__
#define __I3IPCINTERFACE__

#include<stdint.h>

ssize_t readall(int fd, void* buf, size_t count);

int init_socket(char* path);

char* readMessage(int fd, uint32_t* type);
int sendMessage(int fd, uint32_t type, char* payload);

#endif
