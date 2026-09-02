ORG 0x7C00+512
BITS 16

KYB_DATA_PORT equ 0x60
KYB_STATUS_REG equ 0x64
KYB_CMD_REG equ 0x64

start:
  mov si, msg
  call print

  call enable_a20_line
  jc err
  mov si, a20_success_msg
  call print

  call load_gdt
  call switch_to_protected_mode
  call create_and_load_page_table
  call switch_to_long_mode

  jmp $

err:
  mov si, err_msg
  call print
  jmp $


; ax: 0 disabled, 1 enabled
; DS is assumed to be zero.
enable_a20_line:
  call .test_a20
  cmp ax, 1
  je .done

  call .bios_int_0x15
  call .test_a20
  cmp ax, 1
  je .done

  call .enable_a20_kyb_controller
  call .test_a20
  cmp ax, 1
  je .done

  stc ; failed to activate, set carry flag

.done:
  clc
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

.bios_int_0x15:
  ; No need to query int 0x15 support nor the status, because their result isnt reliable.
  ; We already have our own test.
  ; So we just issue the int 0x15 ax=0x2401 and then run our own test.
  mov ax, 0x2401
  int 0x15
  ret

.enable_a20_kyb_controller:
  cli
  
  call .wait_input_empty
  mov al, 0xAD ; diable keyboard
  out KYB_CMD_REG, al

  call .wait_input_empty
  mov al, 0xD0  ; Read Controller Output Port	
  out KYB_CMD_REG, al 

  call .wait_output_arrived
  in al, KYB_DATA_PORT ; save the repsonse
  push ax

  call .wait_input_empty
  mov al, 0xD1  ; write next byte into controller output port
  out KYB_CMD_REG, al 

  call .wait_input_empty
  pop ax
  or al, 2  ; set controller output port second bit to 1 (A20 gate (output))
  out KYB_DATA_PORT, al

  call .wait_input_empty
  mov al, 0xAE ; Enable first PS/2 port	
  out KYB_CMD_REG, al

  call .wait_input_empty
  sti
  ret


.wait_input_empty:  ; wait until input buffer is empty (input is from the cpu to the controller)
  in al, KYB_CMD_REG
  test al, 2
  jnz .wait_input_empty
  ret

.wait_output_arrived: ; wait until output buffer is not empty (out is from the controller to the cpu)
  in al, KYB_CMD_REG
  test al, 1
  jz .wait_output_arrived
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
err_msg db 'An error has occurred', 13, 10, 0
a20_success_msg db 'A20 line activated', 13, 10, 0