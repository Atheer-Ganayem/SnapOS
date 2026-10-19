[BITS 64]

section .text

%macro int_err 2
  global int%1
  extern %2
  int%1:
  
  ; test CS CPL, if not zero zero (CPL != 0, meaning we are returning to user space) then swapgs.
  test qword [rsp + 16], 3
  jz .skip_swapgs_%1
  swapgs
  
.skip_swapgs_%1:

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

  ; test CS CPL, if not zero zero (CPL != 0, meaning we are returning to user space) then swapgs.
  test qword [rsp + 8], 3
  jz .done_%1
  swapgs

.done_%1:
  iretq
%endmacro

%macro int_no_err 2
  global int%1
  extern %2
  int%1:

  push qword 0

  ; test CS CPL, if not zero zero (CPL != 0, meaning we are returning to user space) then swapgs.
  test qword [rsp + 16], 3
  jz .skip_swapgs_%1
  swapgs
  
.skip_swapgs_%1:

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

  ; test CS CPL, if not zero zero (CPL != 0, meaning we are returning to user space) then swapgs.
  test qword [rsp + 8], 3
  jz .done_%1
  swapgs

.done_%1:
  iretq
%endmacro

extern irq_router

int_no_err 0, divide_by_zero_handler
int_err 13, general_protection_handler
int_err 14, page_fault_handler


%assign v 32
%rep 224 ; 256 - 32 (total vectors - exceptions)
irq_tramp_%+v:
  push qword v
  jmp irq_common
%assign v v+1
%endrep

irq_common:
  test qword [rsp + 16], 3
  jz .from_kernel
  swapgs
.from_kernel:
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
  mov rsi, qword [rsp + 120]   ; vector
  cld
  call irq_router

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
  test qword [rsp + 8], 3
  jz .to_kernel
  swapgs
.to_kernel:
  iretq


section .rodata
global irq_tramp_table
irq_tramp_table:
%assign v 32
%rep 224
  dq irq_tramp_%+v
%assign v v+1
%endrep