#ifndef __I3IPCINTERFACE__
#define __I3IPCINTERFACE__

#include <cstdint>
#include <unistd.h>
#include <string>

std::string ipc_type_to_string(unsigned int type);
ssize_t readall(int fd, void* buf, size_t count);

int init_socket(char const* path);

char* readMessage(int fd, uint32_t* type);
int sendMessage(int fd, uint32_t type, char* payload);

#endif
