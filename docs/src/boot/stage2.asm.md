# src/boot/stage2.asm

## Overview

Stage2 is sector 1 to `STAGE2_SECTOR_COUNT`, loaded by stage1 at `0x7C00+512`.
It's Its primary responsibilities are:
- Enable A20 line.
- Set up GDT for both Protected and Long Modes.
- Switch to Protected Mode.
- Switch to Unreal Mode, Read the partition table, find the correct partition, read the ext2fs in this partition, read the kernel binary and load it. Then switch back to Protected Mode.
- Create initial PML4, PDPT and PD.
- Switch to Long Mode.

## Execution Sequence

### 1. Enabling A20 line

#### First of all, why do we need to enable the A20 line ?
Because back when the line was 19 bits wide, and registers 16 bits wide, if you set the segment register to 0xFFFF, you could access the last 16 bytes and then it wraps around, and you couold access the first 64kib - 16 bytes, developers used this to gain performance by reducing the overhead of changing segment registers.

#### How to enable
There are many ways of enabling the A20 line, each way might work on some devices and might not on others. Here are the steps:
  - First, check of the A20 line is already enabled, if so, no work to do.
  - Use BIOS `int 0x15 ax = 0x2401` (dont give any attention to the returned values), then check of A20 line is enabled.
  - Enable it using the kerboard controller and check if succeeds. (When they first introduced the A20 line they wired it to the keyboard controller, so if you needed to enable/disable it, you had to talk with the keyboard controller).
  - Last method: use the A20 fast gate and check if succeeds. This is saved for last because it's not supported everywhere, and it might actually do some weird stuff if not, like blanking the screen or worse.
  - If all fail, just hang.

### 2. Loading GDT and switching to temporary PM
After enabling the A20 line, we load the GDT, enable the cr0.pe (protection enabled) bit, and swtich to a temporary Protected Mode routine by far jumping with the 16-bit __code segment__.
In the temporary Protected Mode routine we load a 32-bit base=0 limit=4GiB in the data selector (and all the others), this will allow us to access the wold 4GiB of memory. Then we disable cr0.PE, and far jump to unreal_mode.


### 3. Unreal mode
in Unreal mode we zero out all segment selectors, and even though we did that, we could still access the whole 4GiB because the selector's limit cache is still valid (holding a 4GiB limit).

### 4. Finding a bootable partition
We search the parition table in the MBR (4 entries at the last 64 bytes of sector 9) for a parition markes as bootable and has Linux as it's system ID.
We take it's starting LBA and push it to loadkernel.

### 5. Loadkernel
loadkernel is written in C, it parses the ext2fs for a file in the root directory called `kernel.bin`, and loads it at `0x1000000 (16 MiB)` (this is where almost all operating systems load their kernel, because 1MiB - 16MiB hold some motherboard things).

### 6. Switching to Protected Mode
After loading the kernel, we finally switch to protected mode, we clear interrupts and mask the PIC (slave and master), because not masking them causes me crshes (after searching, it has something to do with an interrupt already arriving to the PIC before the cpu executes `cli`), then we enable cr0.pe, and far jump using the 32-bit code segment to pm_main.

### 7. pm_main and creating a page table
In pm_main we set the selector of all segments (except cs) to the 32-bit data descriptor.
We create a page table:
  - PML4: first entry points to PDPT_LOW, last entry points to PDPT_HIGH.
  - PDPT_LOW: first entry points to PD.
  - PDPT_HIGH: Map the second to last entry to PD.
  - PD: map all the 512 entries (2MiB * 512 = 1GiB) to the first phyiscal 1GiB.

Reminder: 
- Each entry in the PML4 is 512GiB.
- Each entry in the PDPT is 1GiB.
- Each entry in the PD is 2MiB.

### 7. Switching to Long Mode
- Check if CPUID is supported
- Check of CPUID extended function `0x80000001` (query if Long Mode is supported) is supported.
- Call `0x80000001` function and check if Long Mode is supported.
- If any preceding steps fail, we halt.

Then:
  - Load PML4 in cr3.
  - Enable PAE bit in cr4 (physical address extention. Allows us to use more than 4GiB).
  - Enable EFER (extended feature enable register) bit 8 (Long Mode enable).
  - Enable paging (cr0 bit 31).

now we are in compatibility mode, still executing 32-bit instructions, because the code segment is 32-bit code segment.
Finally, far jump to lm_main using 64-bit code selector.

### 8. lm_main
Set all segment selectors (except CS) to 64-bit data descriptor.
jump to kernel code (0xFFFFFFFF_81000000).
