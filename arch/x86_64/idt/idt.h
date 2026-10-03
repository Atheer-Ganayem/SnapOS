#ifndef ARCH_IDT_H
#define ARCH_IDT_H

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

#endif