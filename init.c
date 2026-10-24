#include <stdint.h>
#include <stddef.h>

long sys_read(unsigned int fd, char* buf, size_t count) {
    long ret;
    
    __asm__ __volatile__(
        "syscall"
        : "=a" (ret)               
        : "a" (0),                
          "D" (fd),                
          "S" (buf),               
          "d" (count)               
        : "rcx", "r11", "memory"      
    );
    
    return ret;
}

void _start() {
  volatile uint64_t counter = 0;

  char buf[128] = {0};
  long ret = 0;
  ret = sys_read(0, buf, 128);
  
  char* ptr = (char*)ret;
  *ptr = 0;

  while (counter < 500000000LL) {
    counter++;
  }

  __asm__ volatile("" ::: "memory");

  while (1) {
    __asm__ __volatile__("nop");
  }
}