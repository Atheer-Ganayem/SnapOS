#ifndef SYSCALL_H
#define SYSCALL_H

#include <stddef.h>

long sys_read(unsigned int fd, char* buf, size_t count);
long sys_exit(int error_code);


#endif