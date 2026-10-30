#ifndef SYSCALL_H
#define SYSCALL_H

#include <stddef.h>

long sys_read(unsigned int fd, void* buf, size_t count);
long sys_write(unsigned int fd, const char* buf, size_t count);

long sys_exit(int error_code);


#endif