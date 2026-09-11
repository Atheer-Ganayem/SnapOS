[BITS 64]

section .text

read_cr3:
  mov rax, cr3
  ret

write_cr3:
  mov cr3, rdi
  ret