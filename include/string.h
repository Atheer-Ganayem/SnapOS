#ifndef STRING_H
#define STRING_H

#include <stddef.h>

void* memset(void* s, int c, size_t n);
void* memcpy(void* restrict dest, void* restrict src, size_t count);
int memcmp(const void* s1, const void* s2, size_t n);

size_t strlen(const char* s);
char* strcpy(char* restrict dest, const char* restrict src);
char* strncpy(char* restrict dest, const char* restrict src, size_t n);
int strncmp(const char* s1, const char* s2, size_t n);
char* strncat(char* restrict dest, const char* restrict src, size_t n);

#endif