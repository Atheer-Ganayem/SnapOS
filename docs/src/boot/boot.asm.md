# src/boot/boot.asm

## Overview

Stage1 is sector 0, the first 512 bytes loaded by BIOS at `0x7C00`.
It operates in 16 bit real mode.
It's Its primary responsibilities are to extract the system memory map from BIOS,
and load stage 2 from the disk using LBA (logical block addressing) (it does that using a BIOS command).

It also sets up esp, cs, and all the other segment registers (ds, ss, es, fs, gs).

## Memory layout

| Address | Size | Purpose |
| :--- | :--- | :--- |
| `0x0500` | 2 bytes | Stores the total number of memory map entries (`COUNTER`) |
| `0x0510` | 3072 bytes | Buffer for the memory map entries (`LIST_BUF`). Max 128 entries at 24 bytes each. |
| `0x7C00` | 512 bytes | Stage 1 bootloader (loaded by BIOS) |
| `0x7C00` | no size, grows downards | Real Mode Stack |
| `0x7E00` | `STAGE2_SECTOR_COUNT*512` (passed to nasm before compilation) | Stage 2 bootloader load address (`0x7C00 + 512`) |

## Execution Sequence

### 1. Environment Setup
Firstly we far jump `jmp 0:start` to force the Code Segment to 0 (because it cannot be manipulated with mov).
Then in `start:` we zero out all segments, set `sp` to `STACK_BASE_ADDR` which is `0x7C00` so it grows away of the first sector.
We then no `mov [drive_no], dl`:
  - The BIOS stores the main drive number (the drive where the bootloader is located) into the `dl`.
  - `drive_no` is just some preserved label rejoin inside the boot sector.
  We use `drive_no` later to read from the disk using LBA.

### 2. Memory Map Detection (E820)
Before leaving real mode, we need to use the BIOS `INT 0x15, EAX=0xE820` function to detect the physical RAM layout. (because BIOS functions are available only in real mode).
We read at max 128 entries (which is more than enough), store them at `LIST_BUF equ 0x510`.
After we finish, we store the number of entries at `COUNTER equ 0x500`. Each entry size is `ENTRY_SIZE equ 24` long.

### 3. Reading and loading stage2 from disk
We use `INT 0x13, AH=0x42` BIOS function, which uses LBA addressing.
This function needs a structure called Disk Address Packet (DAP).
We read starting at sector 1, using the `drive_no` we just stored in the begining. The number of sectors is `STAGE2_SECTOR_COUNT` which is passed to us as a constant in compilation. We read to address `STAGE2_ADDR equ 0x7C00+512`.

#### Format of the Disk Address Packet (from OSDev):
| Offset | Size |	Description |
| :--- | :--- | :--- |
| 0 |	1	| Size of packet (16 bytes, 0x10) |
| 1	| 1	| Always 0 |
| 2	| 2	| Number of sectors to transfer (max 127 on some BIOSes) |
| 4	| 4	| Transfer buffer (16 bit segment:16 bit offset) (see note #1) |
| 8	| 8	| LBA address of sector to read from disk |

### 4. Stage 2 Handoff
Once the disk read succeeds, Stage 1 executes a far jump (`jmp 0:STAGE2_ADDR`) to `0x7E00`, transferring control to Stage2.