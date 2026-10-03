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

int memcmp(const void* s1, const void* s2, size_t n) {
  for (; n > 0; n--, s1++, s2++) {
    if (*(const unsigned char*)s1 != *(const unsigned char*)s2) {
      return *(const unsigned char*)s1 - *(const unsigned char*)s2;
    }
  }

  return 0;
}

char* strcpy(char* restrict dest, const char* restrict src) {
  char* base_dest = dest;
  while ((*dest++ = *src++)) {}

  return base_dest;
}

int strncmp(const char* s1, const char* s2, size_t n) {
  for (; n > 0; n--, s1++, s2++) {
    if (*s1 != *s2) {
      return (unsigned char)*s1 - (unsigned char)*s2;
    }

    if (*s1 == 0x00) break;
  }

  return 0;
}