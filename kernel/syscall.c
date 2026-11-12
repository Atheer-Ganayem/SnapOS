#include <syscall.h>
#include <types.h>
#include <drivers/vga.h>

long sys_not_implemented() {
  vga_print("sys not implemented triggered\n");
  return -ENOSYS;
}

void* syscall_table[256] = {
  [0] = sys_read,
  [1] = sys_write,
  [2] = sys_open,
  [3 ... 255] = sys_not_implemented,
};