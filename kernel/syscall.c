#include <syscall.h>
#include <stdint.h>
#include <stddef.h>
#include <types.h>
#include <drivers/vga.h>

intptr_t sys_not_implemented() {
  vga_print("sys not implemented triggered\n");
  return -ENOSYS;
}

void* syscall_table[256] = {
  [0] = sys_read,
  [1] = sys_write,
  [2 ... 255] = sys_not_implemented,
};