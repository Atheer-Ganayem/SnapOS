#include <stdint.h>

void _start() {
  volatile uint64_t counter = 0;

  long ret;
  __asm__ __volatile__(
    "syscall"
    : "=a" (ret)          // Output: RAX goes into 'ret'
    : "a" (0)             // Input: RAX = 0 (your syscall number)
    : "rcx", "r11", "memory" // Clobbers: Hardware overwrites RCX and R11
  );

  while (counter < 500000000LL) {
    counter++;
  }

  __asm__ volatile("" ::: "memory");

  while (1) {
    __asm__ __volatile__("nop");
  }
}