ORG 0x7C00
BITS 16

READ_ENTRY_COMMAND equ 0xE820
READ_ENTRY_MAGIC equ 0x534D4150
READ_ENTRY_SECCUSS equ 0x534D4150

STACK_BASE_ADDR 0x7C00
LIST_BUF equ 0x510
COUNTER equ 0x500
ENTRY_SIZE equ 24
MAX_ENTRIES equ 128

jmp 0:start

start:
  xor eax, eax
  mov ds, eax
  mov es, eax
  mov fs, eax
  mov gs, eax
  mov ss, eax

  mov sp, STACK_BASE_ADDR

  call get_memory_map

  mov si, msg
  call print

  jmp $

get_memory_map:
  mov di, LIST_BUF
  xor ebx, ebx
  
  xor bp, bp ; the counter

.read_entry:
  mov edx, READ_ENTRY_MAGIC ; magic number
  mov eax, READ_ENTRY_COMMAND      ; command
  mov ecx, ENTRY_SIZE
  int 0x15

  jc .done

  cmp eax, READ_ENTRY_SECCUSS
  jnz .err

  add di, ENTRY_SIZE

  inc bp
  cmp ebp, MAX_ENTRIES
  je .err

  test ebx, ebx
  je .done

  jmp .read_entry

.done:
  xor eax, eax
  mov ds, eax
  mov [COUNTER], bp
  ret

.err:
  mov si, err
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

msg   db 'Read memory map successfully', 13, 10, 0
err   db 'An error occurred while reading the memory map', 13, 10, 0

times 445-($ - $$) db 0

stage_two_sector_count db 0x00 ; here we are gonna put the size (in sectors) of stage 2 during the build.

times 510-($ - $$) db 0
dw 0xAA55