[BITS 64]

section .text

global load_idtr
extern dummy_interrupt_handler

load_idtr:
  lidt [rdi]
  ret

%macro int_err 2
  global int%1
  extern %2
  int%1:
  
  push rax
  push rbx
  push rcx
  push rdx
  push rsi
  push rdi
  push rbp
  push r8
  push r9
  push r10
  push r11
  push r12
  push r13
  push r14
  push r15

  mov rdi, rsp
  mov rsi, qword %1
  mov rdx, qword[rsp + 120]
  cld
  call %2

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
%endmacro

%macro int_no_err 2
  global int%1
  extern %2
  int%1:

  push qword 0
  
  push rax
  push rbx
  push rcx
  push rdx
  push rsi
  push rdi
  push rbp
  push r8
  push r9
  push r10
  push r11
  push r12
  push r13
  push r14
  push r15

  mov rdi, rsp
  mov rsi, qword %1
  mov rdx, qword 0 ; dummy err
  cld
  call %2

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
%endmacro

int_no_err 0, divide_by_zero_handler
int_err 13, general_protection_handler
int_err 14, page_fault_handler