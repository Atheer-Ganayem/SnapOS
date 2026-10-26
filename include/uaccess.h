#ifndef UACCESS_H
#define UACCESS_H

// these are implemented in arch specific code.

unsigned long copy_to_user(void* dest, const void* src, unsigned long n);
unsigned long copy_from_user(void* dest, const void* src, unsigned long n);

#endif