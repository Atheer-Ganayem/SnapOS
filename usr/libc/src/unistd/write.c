#include <unistd.h>
#include "../internal/syscall.h"
#include <sys/syscall.h>

ssize_t write(int fd, const void* buf, size_t count) {
  return syscall(SYS_write, fd, buf, count);
}