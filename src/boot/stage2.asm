ORG 0x7C00+512
BITS 16

start:
  mov si, msg
  call print

  call enable_a20_line
  call load_gdt
  call switch_to_protected_mode
  call create_and_load_page_table
  call switch_to_long_mode

  jmp $


; ax: 0 disabled, 1 enabled
; DS is assumed to be zero.
enable_a20_line:
  call .test_a20
  cmp ax, 1
  je .done



.done:
  ret

.test_a20:
  ; we first push the values of 0x0000:0x500 and 0xffff:0x510
  ; then set 0xffff:0x510 to 0xff, then 0x0000:0x500 to 0xx
  ; if A20 is disabled, then setting 0x0000:0x500 to 0x00 will wrap around and overrite 0xffff:0x510 (which is 0xff)
  ; check if 0xffff:0x510 is 0xff, if not, ax = 0, else ax = 1
  ; restore values, reset es = 0

  ; set es to 0xffff
  xor ax, ax
  not ax
  mov es, ax

  mov di, 0x510
  mov si, 0x500

  ; es:di 0xffff:0x510
  ; ds:si 0x0000:0x500

  push word[es:di]
  push word[si]

  mov byte[es:di], 0xff ; now 0xffff:0x510 is 0xff
  mov byte[si], 0x00 ; now 0x0000:0x500 is 0x00


  ; check if overwritten
  xor ax, ax
  mov al, byte[es:di] ; if disabled this would be 0x00, else (if enabled) 0xff
  test ax, ax
  
  jz .test_a20_done

  mov ax, 1

.test_a20_done:
  pop word[si]
  pop word[es:di]

  xor bx, bx
  mov es, bx

  ret

load_gdt:
  jmp $

switch_to_protected_mode:
  jmp $

create_and_load_page_table:
  jmp $

switch_to_long_mode:
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