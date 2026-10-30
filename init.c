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

long sys_write(unsigned int fd, const char* buf, size_t count) {
  long ret;
  
  __asm__ __volatile__(
      "syscall"
      : "=a" (ret)               
      : "a" (1),                
        "D" (fd),                
        "S" (buf),               
        "d" (count)               
      : "rcx", "r11", "memory"      
  );
  
  return ret;
}

void _start() {
  volatile uint64_t counter = 0;

  char buf[128] = "received: ";
  long ret = 0;
  ret = sys_read(0, &buf[10], 128 - 10);
  ret = sys_write(1, buf, 10 + ret);

  // char* ptr = (char*)buf[0];
  // *ptr = 0;

  while (counter < 500000000LL) {
    counter++;
  }

  __asm__ volatile("" ::: "memory");

  while (1) {
    __asm__ __volatile__("nop");
  }
}