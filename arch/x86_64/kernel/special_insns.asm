[BITS 64]

section .text

global read_cr3
global write_cr3
global flush_tlb_single

read_cr3:
  mov rax, cr3
  ret

write_cr3:
  mov cr3, rdi
  ret

flush_tlb_single:
  invlpg [rdi]
  ret
