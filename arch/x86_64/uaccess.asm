[BITS 64]

section .text

global copy_to_user
global copy_from_user
global strlen_user


; unsigned long copy_to_user(void* dest, const void* src, unsigned long n);
; rdi, rsi, rdx
copy_to_user:
  mov rcx, rdx

.faulting_instr:
  rep movsb

  xor rax, rax
  ret

.fix_up:
  mov rax, rcx
  ret


; unsigned long copy_from_user(void* dest, const void* src, unsigned long n);
; rdi, rsi, rdx
copy_from_user:
  mov rcx, rdx

.faulting_instr:
  rep movsb

  xor rax, rax
  ret

; here we zero out the rest for security reasons.
.fix_up:
  mov r8, rcx
  xor al, al
  rep stosb

  mov rax, r8
  ret


; long strlen_user(const char* str);
strlen_user:
  xor rax, rax

.faulting_instr:
  cmp byte[rdi + rax], 0
  je .done
  inc rax
  jmp .faulting_instr

.done:
  inc rax
  ret

.fix_up:
  xor rax, rax
  ret


section .ex_table
  align 8
  dq copy_to_user.faulting_instr
  dq copy_to_user.fix_up

  dq copy_from_user.faulting_instr
  dq copy_from_user.fix_up

  dq strlen_user.faulting_instr
  dq strlen_user.fix_up

