#ifndef TYPES_H
#define TYPES_H

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

typedef enum {
  KSTATUS_SUCCESS           = 0,
  KSTATUS_ERR_NO_MEMORY     = 1,
  KSTATUS_ERR_NO_MEM_MAP    = 2,
  KSTATUS_ERR_INVALID_ARGS  = 3,
  KSTATUS_ERR_NOT_FOUND     = 4,
  KSTATUS_ERR_UNIMPLEMENTED = 5
} kstatus_t;

void panic(char* s);

#endif