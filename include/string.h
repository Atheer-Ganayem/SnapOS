#ifndef STRING_H
#define STRING_H

#include <stddef.h>

void* memset(void* s, int c, size_t n);
void* memcpy(void* restrict dest, void* restrict src, size_t count);
char* strcpy(char* restrict dest, const char* restrict src);

#endif