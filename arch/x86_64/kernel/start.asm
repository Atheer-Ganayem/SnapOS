[BITS 64]

section .bss
align 16
stack_bottom:
  resb (1024 * 16)
stack_top:

section .entry

global _start
extern kmain

_start:
  mov rsp, stack_top
  call kmain
.err:
  cli
  hlt
  jmp $