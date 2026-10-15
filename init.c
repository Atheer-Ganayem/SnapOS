#include <stdint.h>

void _start() {
  volatile uint64_t counter = 0;
  while (counter < 1500000000LL) {
    counter++;
  }

  __asm__ volatile("" ::: "memory");

  __asm__ __volatile__("cli");


  while (1) {
    __asm__ __volatile__("nop");
  }
}