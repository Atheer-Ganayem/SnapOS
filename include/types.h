#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

typedef __PTRDIFF_TYPE__ ssize_t;

typedef enum {
  KSTATUS_SUCCESS           = 0,
  KSTATUS_GENERAL_ERR       = 1,
  KSTATUS_ERR_NO_MEMORY     = 2,
  KSTATUS_ERR_NO_MEM_MAP    = 3,
  KSTATUS_ERR_INVALID_ARGS  = 4,
  KSTATUS_ERR_NOT_FOUND     = 5,
  KSTATUS_ERR_UNIMPLEMENTED = 6,
  KSTATUS_ERR_IO            = 7,
  KSTATUS_FS_PROBE_FAIL     = 8,
  KSTATUS_ERR_NO_ROOT       = 9
} kstatus_t;

void panic(char* s);

static inline uint32_t u32_min(uint32_t a, uint32_t b) {
  return a < b ? a : b;
}

#endif