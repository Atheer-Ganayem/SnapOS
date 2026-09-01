ORG 0x7C00
BITS 16

READ_ENTRY_COMMAND equ 0xE820
READ_ENTRY_MAGIC equ 0x534D4150
READ_ENTRY_SECCUSS equ 0x534D4150

STACK_BASE_ADDR equ 0x7C00
STAGE2_ADDR equ 0x7C00+512
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

  mov [drive_no], dl

  call get_memory_map

  mov si, msg
  call print

  call read_stage2

  jmp 0:STAGE2_ADDR

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

DAP: ; Disk Address Packet
db 0x10               ; peacket size
db 0x00               ; always 0
dw STAGE2_SECTOR_COUNT
dw STAGE2_ADDR        ; buffer offset
dw 0                  ; buffer segment
dq 1                  ; LBA number

drive_no: db 0x00 ; we overwrite this in start

read_stage2:
  mov si, DAP
  mov ah, 0x42 ; command num
  mov dl, 0x80 ; typically for drive zero
  int 0x13

  jc .err

  test ah, ah
  jnz .err

  ret

.err:
  mov si, err
  call print
  jmp $

times 510-($ - $$) db 0
dw 0xAA55