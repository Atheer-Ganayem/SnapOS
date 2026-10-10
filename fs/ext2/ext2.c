#include <fs/ext2.h>
#include <vfs.h>
#include <string.h>

kstatus_t ext2_init_singly(struct ext2_inode_info* i_info) {
  if (i_info->singly) {
    return KSTATUS_SUCCESS;
  }

  uint32_t block_size = i_info->sb->block_size;
  uint32_t* buf = kmalloc(block_size);
  if (!buf) {
    return KSTATUS_ERR_NO_MEMORY;
  }

  uint64_t lba = (i_info->inode->singly) * i_info->sb->sectors_per_block;
  struct phys_iovec iovec[] = {{.addr = (uint64_t)VIRT_TO_PHYS(buf), .length = block_size}};
  kstatus_t status = i_info->sb->bdev->read(i_info->sb->bdev, lba, i_info->sb->sectors_per_block, iovec, 1);
  if (status != KSTATUS_SUCCESS) {
    kfree(buf);
    return status;
  }  

  i_info->singly = buf;

  return KSTATUS_SUCCESS;
}

int ext2_lookup(struct inode* dir, struct dentry* child) {
  (void)dir; (void)child;
  return 0;
}

void ext2_register(struct fs_driver* entry) {
  memcpy(entry->name, "ext2fs", sizeof(entry->name));
  entry->mount = ext2_mount;
  entry->probe = ext2_probe;
}