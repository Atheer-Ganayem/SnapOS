[BITS 64]

section .text

global ret_from_fork
ret_from_fork:
  pop r15
  pop r14
  pop r13
  pop r12
  pop r11
  pop r10
  pop r9
  pop r8
  pop rbp
  pop rdi
  pop rsi
  pop rdx
  pop rcx
  pop rbx
  pop rax

  add rsp, 8
  iretq


global __arch_switch_to
; void arch_switch_to(uint64_t* prev_rsp, uint64_t* next_rsp);
; rdi = pointer to current->ctx.rsp
; rsi = pointer to next->ctx.rsp

__arch_switch_to:
  pushfq
  push rbx
  push rbp
  push r12
  push r13
  push r14
  push r15

  mov [rdi], rsp 
  mov rsp, [rsi]  

  pop r15
  pop r14
  pop r13
  pop r12
  pop rbp
  pop rbx
  popfq

  ret 