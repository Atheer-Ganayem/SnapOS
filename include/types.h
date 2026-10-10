#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

typedef __PTRDIFF_TYPE__ ssize_t;

#define MAX_ERRNO   4095

#define ERR_PTR(err) ((void*)(intptr_t)(err))

#define PTR_ERR(ptr) ((intptr_t)(ptr))

#define IS_ERR(ptr)  ((uintptr_t)(ptr) >= (uintptr_t)-MAX_ERRNO)

#define EPERM            1      // Operation not permitted
#define ENOENT           2      // No entity
#define EIO              5
#define ENOMEM          12
#define EACCES          13      // Permission denied
#define EFAULT          14      // Bad address (Page Fault equivalent)
#define EEXIST          17      // File exists
#define ENODEV          19
#define ENOTDIR         20      // Not a directory
#define EINVAL          22      // Invalid argument

void panic(char* s);

static inline uint32_t u32_min(uint32_t a, uint32_t b) {
  return a < b ? a : b;
}

#endif