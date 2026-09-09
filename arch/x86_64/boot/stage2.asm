; ORG 0x7C00+512
BITS 16

global _start

KYB_DATA_PORT equ 0x60
KYB_STATUS_REG equ 0x64
KYB_CMD_REG equ 0x64

FAST_A20_GATE_REG equ 0x92

_start:
  mov [drive_no], dl
  call enable_a20_line
  jc .err
  call load_gdt_and_switch_to_pm_temp

  ; this shouldn't be reached. load_gdt_and_switch_to_pm_temp will jump to 32 bit code
  jmp $

.err:
  jmp $


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

  call .fast_a20_gate
  call .test_a20
  cmp ax, 1
  je .done

  stc ; failed to activate, set carry flag
  ret

.done:
  clc
  ret

; ax: 0 disabled, 1 enabled
; DS is assumed to be zero.
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

.fast_a20_gate:
  in al, FAST_A20_GATE_REG
  test al, 2
  jnz .fast_a20_gate_done ; already enabled
  or al, 2

  ; if the first bit is set to 1 it will trigger a restart.
  ; after some research i found out it might be set to 1 because it's a stail value from the last run,
    ; or because the hardware just handed us garbage value.
    ; someone might wonder how would it be 1 and didn't cause a restart ?
    ; because reading the port might be trigger only when writing.
  and al, 0b11111110
  
  out FAST_A20_GATE_REG, al

.fast_a20_gate_done:
  ret


null_seg: dq 0

kernel_code_seg_16:
dw 0xFFFF
dw 0
db 0
db 0b10011011
db 0b00000000
db 0

kernel_data_seg_32:
dw 0xFFFF
dw 0
db 0
db 0b10010011
db 0b11001111
db 0

kernel_code_seg_32:
dw 0xFFFF
dw 0
db 0
db 0b10011011
db 0b11001111
db 0

kernel_code_seg_64:
dw 0xFFFF
dw 0
db 0
db 0b10011011
db 0b10101111
db 0

kernel_data_seg_64:
dw 0xFFFF
dw 0
db 0
db 0b10010011
db 0b11001111
db 0


gdtr:
dw 8*6 - 1
dd null_seg

load_gdt_and_switch_to_pm_temp:
  cli
  lgdt [gdtr]
  mov eax, cr0
  or al, 1
  mov cr0, eax

  jmp dword (kernel_code_seg_16 - null_seg):pm_temp


PARTITION_TABLE_START equ 0x7c00 + 446
PARTITION_TABLE_ENTRY_SIZE equ 16

DAP: ; Disk Address Packet
db 0x10               ; packet size
db 0x00               ; always 0
DAP_SECTOR_COUNT dw 0x00
DAP_BUF_ADDR dw 0x00        ; buffer offset
dw 0                  ; buffer segment
DAP_LBA_NUMBER dq 0

drive_no: db 0x00 

global issue_read_to_disk
extern loadkernel

unreal_mode:
  xor ax, ax
  mov ds, ax
  mov es, ax
  mov ss, ax
  mov gs, ax
  mov fs, ax
  mov esp, 0x7c00

  call .find_bootable_partition
  jc .err
  cmp byte[si+4], 0x83 ; Linux system ID
  jne .err
  mov ebp, dword[si+8] ; starting LBA of the partition

  sti

  push ebp
  call dword loadkernel
  add esp, 4
  test eax, eax ; return code
  jnz .err

  call .switch_to_pm_main

.err:
  jmp $

; if carry is set then no partition was found.
; if carry was clear, then si will hold a pointer to the begining of the partition entry.
.find_bootable_partition:
  mov si, PARTITION_TABLE_START
.find_bootable_partition_loop:
  cmp byte[si], 0x80
  je .find_bootable_partition_done

  add si, PARTITION_TABLE_ENTRY_SIZE
  cmp si, PARTITION_TABLE_START + 4 * PARTITION_TABLE_ENTRY_SIZE
  jne .find_bootable_partition_loop
  
  stc
  ret
.find_bootable_partition_done:
  clc
  ret


.switch_to_pm_main:
  cli

  ; mask slave and master interrupts
  ; not masking them caused me issues
  mov al, 0xFF
  out 0xA1, al  
  out 0x21, al 

  mov eax, cr0
  or al, 1
  mov cr0, eax

  jmp dword (kernel_code_seg_32 - null_seg):pm_main


issue_read_to_disk:
  push ebp
  mov ebp, esp

  push esi

  mov eax, [ebp + 16] ; buf addr
  mov word[DAP_BUF_ADDR], ax
  mov eax, [ebp + 12] ; number of sectors
  mov word[DAP_SECTOR_COUNT], ax
  mov eax, [ebp + 8] ; lba
  mov dword[DAP_LBA_NUMBER], eax

  mov si, DAP
  mov ah, 0x42 ; command num
  mov dl, byte[drive_no]
  int 0x13

  jc .err

  test ah, ah
  jnz .err

  pop esi
  pop ebp

  ret
.err:
  jmp $


pm_temp:
  mov eax, kernel_data_seg_32 - null_seg
  mov ds, eax
  mov es, eax
  mov ss, eax
  mov gs, eax
  mov fs, eax
  ; reset stack pointer
  mov esp, 0x7c00

  mov eax, cr0
  and eax, ~1
  mov cr0, eax

  jmp word 0x00:unreal_mode


[BITS 32]

pm_main:
  mov eax, kernel_data_seg_32 - null_seg
  mov ds, eax
  mov es, eax
  mov ss, eax
  mov gs, eax
  mov fs, eax
  ; reset stack pointer
  mov esp, 0x7c00

  call create_page_table

  call switch_to_long_mode
  jc .err

  jmp (kernel_code_seg_64 - null_seg):lm_main ; entering to 64 submode
.err:
  jmp $


PML4_ADDR equ 0x70000 ; 448KiB

create_page_table:
  call .create_PML4
  call .create_PDPT_low
  call .create_PDPT_high
  call .create_PD

  ret

.create_PML4:
  mov edi, PML4_ADDR
  call .zero_table
  
  ; first entry
  mov eax, PML4_ADDR + 4096
  or eax, 0x03
  mov dword[PML4_ADDR], eax
  mov dword[PML4_ADDR+4], 0

  ; last entry
  mov eax, PML4_ADDR + (4096*2)
  or eax, 0x03
  mov dword[PML4_ADDR+4088], eax
  mov dword[PML4_ADDR+4092], 0

  ret

.create_PDPT_low:
  mov edi, PML4_ADDR+4096
  call .zero_table

  mov eax, PML4_ADDR + (4096*3)
  or eax, 0x03
  mov dword[PML4_ADDR+4096], eax
  mov dword[PML4_ADDR+4096+4], 0

  ret

.create_PDPT_high:
  mov edi, PML4_ADDR+(4096*2)
  call .zero_table

  mov eax, PML4_ADDR + (4096*3)
  or eax, 0x03
  mov dword[PML4_ADDR + 4096*3 - 16], eax
  mov dword[PML4_ADDR + 4096*3 - 12], 0

  ret

.create_PD: ; this table maps the full table as huge 2MiB pages
  mov edi, PML4_ADDR+(4096*3)
  call .zero_table

  mov edi, PML4_ADDR+(4096*3)
  mov ecx, 0x00
.fill_PD_entry:
  mov eax, ecx
  or eax, 0x83
  mov dword[edi], eax
  mov dword[edi+4], 0
  add edi, 8
  add ecx, 0x200000 ; 2MiB

  cmp ecx, 0x200000 * 512
  jne .fill_PD_entry
  
  ret

; address assumed at edi
.zero_table:
  xor eax, eax
  mov ecx, 1024
  rep stosd
  ret


ERFLAGS_ID equ 1 << 21
CPUID_EXTENDED_FUNCTIONS equ  0x80000000 ; This is the cpuid command that returns to us flags about the highest extended function supported.
CPUID_EXTENDED_INFO equ       0x80000001 ; This function returns info about the extedned features.
CPUID_EDX_EXT_FEAT_LM equ 1 << 29
EFER_MSR equ 0xC0000080
EFER_LM_ENABLE equ 1 << 8
CR4_PAE_ENABLE equ 1 << 5
CR0_PM_ENABLE equ 1 << 0
CR0_PG_ENABLE equ 1 << 31

switch_to_long_mode:
  call .check_CPUID_and_long_mode_support
  jc .not_supported

  mov eax, PML4_ADDR
  mov cr3, eax

  mov eax, cr4
  or eax, CR4_PAE_ENABLE
  mov cr4, eax

  mov ecx, EFER_MSR
  rdmsr

  or eax, EFER_LM_ENABLE
  wrmsr

  mov eax, cr0
  or eax, CR0_PM_ENABLE | CR0_PG_ENABLE
  mov cr0, eax

  ; now we are in compatibility mode

  clc
  ret

.check_CPUID_and_long_mode_support:
  ; check if CPUID is supported
  pushfd
  pop eax
  mov ecx, eax ; save original copy

  xor eax, ERFLAGS_ID
  push eax
  popfd

  pushfd
  pop eax

  push ecx ; resotre original value
  popfd

  xor eax, ecx
  jz .not_supported

  ; restore ERFLAGS original value
  push ecx
  popfd

  ; check if cpuid function is supported and if so, call it and check for long mode support.
  mov eax, CPUID_EXTENDED_FUNCTIONS
  cpuid
  cmp eax, CPUID_EXTENDED_INFO
  jb .not_supported

  mov eax, CPUID_EXTENDED_INFO
  cpuid
  test edx, CPUID_EDX_EXT_FEAT_LM
  jz .not_supported

  clc
  ret
.not_supported:
  stc
  ret


[BITS 64]

lm_main:
  mov ax, (kernel_data_seg_64 - null_seg)
  mov ds, ax
  mov es, ax
  mov ss, ax
  mov gs, ax
  mov fs, ax

  jmp 0xFFFFFFFF81000000