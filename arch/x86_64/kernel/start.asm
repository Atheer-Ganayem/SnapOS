[BITS 64]

section .bss
align 16
stack_bottom:
  resb (1024 * 16)
stack_top:

section .entry

global _start
extern kmain
extern init


extern _bss_start
extern _bss_end

_start:
  mov rsp, stack_top


  ; This costed me few ours of debuging.
  ; because .bss isn't included on disk.
  ; i have to zero it myself.
  lea rdi, [rel _bss_start]
  lea rcx, [rel _bss_end]
  sub rcx, rdi
  xor eax, eax
  rep stosb


  call init
  call kmain
.err:
  cli
  hlt
  jmp $
