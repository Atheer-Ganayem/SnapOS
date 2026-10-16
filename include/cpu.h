#ifndef CPU_H
#define CPU_H

struct cpu_local_data {
  void* kernel_stack;
  void* user_rsp;
  struct task_struct* current;
  uint64_t stack_size;
};

extern struct cpu_local_data cpu0_data;

#endif