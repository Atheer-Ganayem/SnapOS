## Initial implementation

These describe the initial core features/mechanism/policies of SnapOS.
What do I mean by "initial" ? scheduler, threads, memory, FS, simple Port IO, etc...
for exaple, at this stage we wont implement a vidoe buffer, we'll just use the VGA text buffer.


## Ideas (general, no szpecific title)

### Boatloader

- Loading sector 0.
  - Stack pointer directly under sector 0 (**0x7C0**).
- Loading stages 2 directly after sector 0 address.
  - In stage 2, we will extract the memory map out of the BIOS, we will load at max 256 entries, at the maximum avaialbe address in the free range.
  - Setting GDT and switching to protected mode.
  - Setting paging.
  - switching to long mode.
  - Loading the kernel at 1MB (for now im assuming the kernel is a static binary. I might change it to elf).

### Memory

- Paging (assuming 48bit virtuall addresses):
  - High virtual adresses will always be for the kernel (48th bit set to 1. i.e 0xFFFF8000_00000000 to 0xFFFFFFFF_FFFFFFFF) and it will be mapped linerally. i.e 0xFFFF8000_00000000 = physical address 0x00.
  - Low virtual addresses (0x00 to 0x00007FFF_FFFFFFFF) will be mapped to user space.
  - How to manage it? I'll decide later.

- Physical memory:
  - We extracted the memory map using bios before we switched to protected mode, we use it to strack out free physical memory.
  - We'll use a buddy allocator.

### Scheduler And processes

- We'll intially implement a Round-Robin scheduler.
- Binary loader
- ELF loader


### Filesystem

- We'll implement EXT2.
- Read/Write.