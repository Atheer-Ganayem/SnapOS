[BITS 64]

section .text

global ret_from_fork
ret_from_fork:
  test r12, r12 ; do we have a function pointer ?
  jz .ireturn   ; no ? just iretq

  mov rdi, r13  ; this hold void* arg
  call r12


.ireturn
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

  ; ; test CS CPL, if not zero zero (CPL != 0, meaning we are returning to user space) then swapgs.
  test qword [rsp + 8], 3
  jz .done
  swapgs

.done:
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