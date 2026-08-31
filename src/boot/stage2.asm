ORG 0x7C00+512
BITS 16

start:
  mov si, msg
  call print

  jmp $


print:
  nop
.print_loop: 
  lodsb
  test al, al
  jz .done_print

  mov ah, 0x0e
  mov bx, 0
  int 0x10
  jmp .print_loop

.done_print:
  ret

msg db 'Hello from stage2', 13, 10, 0