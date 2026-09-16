[BITS 64]

section .text

global read_cr3
global write_cr3
global flush_tlb_single
global check_1gib_pages_support
global enable_no_execute
global load_gdtr

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

enable_no_execute:
  mov ecx, 0xC0000080
  rdmsr
  or eax, (1 << 11)
  wrmsr
  ret

;void load_gdtr(struct gdtr* gdtr_ptr)
load_gdtr:
  lgdt [rdi]

  mov ax, 0x10
  mov ds, ax
  mov es, ax
  mov ss, ax
  mov fs, ax
  mov gs, ax
  
  mov ax, 0x18
  ltr ax

  push 0x08
  lea rax, [rel .done]
  push rax
  retfq
.done:
  ret