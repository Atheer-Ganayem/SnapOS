void* syscall_table[] = {};

void syscall_registers(void* func) {
  syscall_table[0] = func;
}