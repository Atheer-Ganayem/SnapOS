[BITS 64]

section .text

global read_cr3
global write_cr3
global flush_tlb_single
global check_1gib_pages_support

read_cr3:
  mov rax, cr3
  ret

write_cr3:
  mov cr3, rdi
  ret

flush_tlb_single:
  invlpg [rdi]
  ret

check_1gib_pages_support:
  mov eax, 0x80000001
  cpuid
  test edx, 1 << 26
  jz .not_supported
  mov eax, 1
  ret

.not_supported:
  mov eax, 0
  ret