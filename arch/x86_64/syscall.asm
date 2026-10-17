[BITS 64]

section .text

extern syscall_table

global syscall_init
; syscall_init(void* sycall_entry_addr)
syscall_init:
  ; enable syscall/sysret
  mov ecx, 0xC0000080 ; EFER MSR
  rdmsr
  or eax, 1
  wrmsr

  ; setting up GDT info
  mov ecx, 0xC0000081 ; STAR MSR
  xor eax, eax
  
  ; funny GPF that took me several hours to fix:
  ; in Intel, on sysret it forces |= 3 to CS and SS.
  ; but in AMD it only does that to the CS.
  ; QEMU behaves like Intel, so i didnt have issues there.
  ; But my machine is AMD, so it kept GPF-ing.
  ; For some F*cking reason, this information is not mentioned in wiki.osdev.org but
  ; mentioned in osdev.wiki, so claude helped me with this.
  ; So what happened is:
  ; - The init task did a syscall
  ; - sysret didnt set RPL=3
  ; - Code executed normally even when CPL != RPL because we are in long mode (the CPU doesnt care about SS when pushing/popping).
  ; - Task descheduled and the CPU pushed the SS with CPL = 0.
  ; - When the task scheduled again, iretq popped the SS with CPL = 0, and iretq forces CPL = RPL,
  ; - it notices they are diffrent, and threw a GPF.
  mov edx, 0x00130008 ; user base gdt 0x13, kernel base 0x08
  wrmsr

  ; syscall entry point
  mov ecx, 0xC0000082 ; LSTAR MSR
  mov rax, rdi        ; Lower 32 bits
  mov rdx, rdi
  shr rdx, 32         ; Upper 32 bits
  wrmsr

  ; setup FMASK
  mov ecx, 0xC0000084 ; FMASK MSR
  xor edx, edx
  mov eax, 0x200      ; mask IF. (if interrupts are enabled this disables them)
  wrmsr

  ret


global syscall_entry
syscall_entry:
  ; on syscall:
  ; - the cpu copies rip into rcx
  ; - the cpu copies rflags into r11
  ; - clears the bit in rflags thar are set in FMASK
  ; - reads STAR and chanegs CS and SS
  ; - jumps to the address in LSTAR (here)

  ; rsp currently is the user stack, we need to change it to the kernel's.
  ; we swapgs, save user rsp, and get the kernel's rsp.
  swapgs
  mov gs:[8], rsp 
  mov rsp, gs:[0]
  add rsp, gs:[24]


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

  ; linux calling convention arguemnts: rdi, rsi, rdx, r10, r8, r9 (and rax for syscall number)
  ; but C excpets the 4th arguemnt to be rcx. so we move r10 to rcx
  mov rcx, r10

  lea r12, [rel syscall_table]
  call [r12 + rax * 8]

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
  add rsp, 8 ; skip rax, its our return value

  cli

  mov rsp, gs:[8]
  swapgs

  o64 sysret