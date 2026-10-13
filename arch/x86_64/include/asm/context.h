#ifndef ARCH_CONTEXT_H
#define ARCH_CONTEXT_H

#include <stdint.h>

struct interrupt_frame {
  uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
  uint64_t ebp, rdi, rsi, rdx, rcx, rbx, rax;

  uint64_t error_code;

  uint64_t rip;
  uint64_t cs;
  uint64_t rflags;
  uint64_t rsp;
  uint64_t ss;
} __attribute__((packed));

struct switch_frame {
  uint64_t r15;
  uint64_t r14;
  uint64_t r13;
  uint64_t r12;
  uint64_t rbp;
  uint64_t rbx;
  uint64_t rflags;
  uint64_t rip;
} __attribute__((packed));

struct task_context {
  uint64_t rsp;
};

struct task_struct;

void arch_init_context(struct task_struct* t, void* entry_point);
void arch_switch_to(struct task_struct* prev, struct task_struct* next);

#endif
