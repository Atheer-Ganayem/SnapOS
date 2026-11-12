#ifndef UACCESS_H
#define UACCESS_H

// these are implemented in arch specific code.

unsigned long copy_to_user(void* dest, const void* src, unsigned long n);
unsigned long copy_from_user(void* dest, const void* src, unsigned long n);


// returns length including the null byte.
// on error it returns 0
long strlen_user(const char* str);
long strnlen_user(const char* str, long count);

#endif