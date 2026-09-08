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


### ISSUE DISCOVERED!!!!!!!!!!!!!!!! ####
pm_temp and unreal mode should be under BITS 16
should load 16 bit code seg and 32 data seg

## Execution Sequence

### 1. 

### 2. 

### 3. 

### 4. 

### 5. 

### 6. 

### 7. 