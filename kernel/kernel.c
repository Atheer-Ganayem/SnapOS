#include <drivers/vga.h>
#include <mm.h>
#include <string.h>
#include <drivers/ahci.h>
#include <types.h>
#include <fs/ext2.h>
#include <sched.h>
#include <asm/irq.h>

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

uint64_t dummy_syscall() {
  vga_print("dummy syscall");
  return 5;
}

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

  res = pmm_init();
  if (res != 0) {
    panic("pmm_init() failed.");
  }

  setup_stage2();

  res = ahci_init();
  if (res < 0) {
    panic("failed to init ACHI.\n");
  }


  vfs_init();
  res = vfs_scan_partitions();
  if (res < 0) {
    panic("partition_scan failed.\n");
  }

  res = vfs_mount_root();
  if (res < 0) {
    panic("couldn't mount root,\n");
  }

  vga_print_color("\nmounted root! (", VGA_COLOR_GREEN);
  vga_print_color((char*)((struct ext2_fs_info*)vfs_root->sb->priv_data)->superblock.volume_name, VGA_COLOR_GREEN);
  vga_putchar_color('0' + ((struct ext2_inode_info*)vfs_root->inode->priv_data)->inode->hard_links_count, VGA_COLOR_RED);
  vga_print(")\n");


  struct file* f = vfs_open("/test_dir/level2.txt");
  if (IS_ERR(f)) {
    panic("failed to open /test_dir/level2.txt\n");
  }

  char* buf = kmalloc(512);
  if (!buf) {
    panic("couldn't alloc buf\n");
  }

  ssize_t n = vfs_read(f, buf, 512 - 2);
  if (n < 0) {
    panic("readign err\n");
  }

  buf[n] = 0x00;

  vga_print(buf);
  vga_putchar('\n');

  vfs_close(f);

  create_idle_task();

  res = kthread_run(init_task_init, "/init");
  if (res < 0) {
    panic("kthread run err\n");
  }

  syscall_registers(dummy_syscall);

  local_irq_enable();
  schedule();


  /*
    - Idle task should only run when runqueue is empty.
    - some inconsitincies (idk the spelling) in ssize_t and size_t in vfs functions and INODE struct.
    - I should read more about the CPL and the required privlage level.
    - the trick of putting snapos img on a parition doesnt actually work.
    - Bug in kmalloc, check 'TODO' comment there.
    - 'TODO' comments in ahci.
  */

  while (1) {
    __asm__ volatile("hlt");
  }
}