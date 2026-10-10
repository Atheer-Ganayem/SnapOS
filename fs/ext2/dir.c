#include <fs/ext2.h>
#include <vfs.h>
#include <string.h>

int ext2_lookup(struct inode* dir, struct dentry* child) {
  (void)dir; (void)child;
  return 0;
}