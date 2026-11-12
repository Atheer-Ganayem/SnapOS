#include "../internal/syscall.h"
#include <sys/syscall.h>

int open(const char* filename, int flags, int mode) {
  return __syscall3(SYS_OPEN, filename, flags, mode);
}