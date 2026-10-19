#include <drivers/vga.h>
#include <mm.h>
#include <string.h>
#include <drivers/ahci.h>
#include <types.h>
#include <fs/ext2.h>
#include <sched.h>
#include <asm/irq.h>
#include <irq.h>

#include <cpu.h>

void panic(char* s) {
  if (s) {
    vga_print_color(s, VGA_COLOR_RED);
  }

  while (1) {
    __asm__ volatile("cli; hlt");
  };
}

extern struct dentry* vfs_root;

struct cpu_local_data cpu0_data = {.stack_size = TASK_KERNEL_STACK_SIZE};



void kmain() {
  int res = early_pmm_init(); 
  if (res < 0) {
    panic(NULL);
  }
  
  res = vmm_init();
  if (res < 0) {
    panic(NULL);
  }

  vga_init();
  vga_print("vga_init ");


  res = pmm_init();
  if (res != 0) {
    panic("pmm_init() failed.");
  }

  vga_print("pmm_init ");

  setup_stage2();

  vga_print("stage2_init ");

  if (ahci_init() < 0)
    panic("failed to init ACHI.\n");

  vga_print("ahci_init ");


  vfs_init();
  if (vfs_scan_partitions() < 0)
    panic("partition_scan failed.\n");

  vga_print("vfs_init ");

  res = vfs_mount_root();
  if (res < 0)
    panic("couldn't mount root,\n");

  vga_print_color("\nmounted root! (", VGA_COLOR_GREEN);
  vga_print_color((char*)((struct ext2_fs_info*)vfs_root->sb->priv_data)->superblock.volume_name, VGA_COLOR_GREEN);
  vga_putchar_color('0' + ((struct ext2_inode_info*)vfs_root->inode->priv_data)->inode->hard_links_count, VGA_COLOR_RED);
  vga_print(")\n");


  create_idle_task();

  res = kthread_run(init_task_init, "/init");
  if (res < 0) {
    panic("kthread run err\n");
  }

  if (timer_irq_init() != 0)
    panic("failed to init timer irq.\n");

  local_irq_enable();
  schedule();


  /*
    - protection against passing syscall number out of range.
    - some inconsitincies (idk the spelling) in ssize_t and size_t in vfs functions and INODE struct.
    - Bug in kmalloc, check 'TODO' comment there.
    - 'TODO' comments in ahci.
    - IRQ routing for PCI devices
  */

  while (1) {
    __asm__ volatile("hlt");
  }
}