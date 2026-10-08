#ifndef TYPES_H
#define TYPES_H

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

typedef enum {
  KSTATUS_SUCCESS           = 0,
  KSTATUS_GENERAL_ERR       = 1,
  KSTATUS_ERR_NO_MEMORY     = 2,
  KSTATUS_ERR_NO_MEM_MAP    = 3,
  KSTATUS_ERR_INVALID_ARGS  = 4,
  KSTATUS_ERR_NOT_FOUND     = 5,
  KSTATUS_ERR_UNIMPLEMENTED = 6,
  KSTATUS_ERR_IO            = 7
} kstatus_t;

void panic(char* s);

#endif