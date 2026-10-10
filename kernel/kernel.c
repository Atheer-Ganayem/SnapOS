#include <drivers/vga.h>
#include <mm.h>
#include <string.h>
#include <drivers/ahci.h>
#include <types.h>
#include <fs/ext2.h>

void panic(char* s) {
  if (s) {
    vga_print_color(s, VGA_COLOR_RED);
  }

  while (1) {
    __asm__ volatile("cli; hlt");
  };
}
extern struct dentry* vfs_root;
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
  

  /*
    - the trick of putting snapos img on a parition doesnt actually work.
    - Bug in kmalloc, check 'TODO' comment there.
    - 'TODO' comments in ahci.
  */

  while (1) {
    __asm__ volatile("cli; hlt");
  }
}