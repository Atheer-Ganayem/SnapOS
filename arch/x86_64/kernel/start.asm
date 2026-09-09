[BITS 64]

section .entry

global _start
extern kmain

_start:
  jmp kmain
