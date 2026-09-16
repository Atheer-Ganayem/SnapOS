#include <string.h>

void* memset(void* s, int c, size_t n) {
  for (size_t i = 0; i < n; i++) {
    ((char*)s)[i] = c;
  }

  return s;
}

void* memcpy(void* restrict dest, void* restrict src, size_t count) {
  for (size_t i = 0; i < count; i++) {
    ((char*)dest)[i] = ((char*)src)[i];
  }

  return dest;
}

char* strcpy(char* restrict dest, const char* restrict src) {
  char* base_dest = dest;
  while ((*dest++ = *src++)) {}

  return base_dest;
}