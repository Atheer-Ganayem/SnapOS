#include <fs/ext2.h>
#include <vfs.h>
#include <mm.h>
#include <string.h>


ssize_t ext2_read_file(struct file* file, void* buf, size_t count, uint64_t offset) {
  struct ext2_inode_info* i_info = (struct ext2_inode_info*)file->dentry->inode->priv_data;
  struct ext2_inode* inode = i_info->inode;
  uint32_t block_size = file->dentry->sb->block_size;

  if (count == 0) return 0;
  else if (offset >= i_info->size) return 0;
  else if (offset + count > i_info->size) count = i_info->size - offset;
  
  uint32_t start_block = offset / block_size;
  uint32_t end_block = (offset + count + block_size - 1) / block_size;
  uint32_t block_count = end_block - start_block;

  void* block_buf = kmalloc(block_size);
  if (!block_buf) {
    return -KSTATUS_ERR_NO_MEMORY;
  }

  struct phys_iovec iovec[] = {{.addr = (uint64_t)VIRT_TO_PHYS(block_buf), .length = block_size}};
  struct block_dev* bdev = file->dentry->sb->bdev;
  uint32_t sectors_per_block = file->dentry->sb->sectors_per_block;
  uint64_t remaining = count;

  for (uint32_t i = 0; i < block_count; i++) {
    uint32_t block = start_block + i;
    uint64_t lba = 0;

    if (block < 12) {
      lba = inode->direct_ptrs[block] * sectors_per_block;
    } else if (block < 12 + (block_size / 4)) {
      ext2_init_singly(i_info);
      lba = i_info->singly[block-12] * sectors_per_block;
    }

    if (lba && file->dentry->sb->bdev->read(bdev, lba, sectors_per_block, iovec, 1) != KSTATUS_SUCCESS) {
      kstatus_t status = file->dentry->sb->bdev->read(bdev, lba, sectors_per_block, iovec, 1);
      if (status != KSTATUS_SUCCESS) {
        kfree(block_buf);
        return -status;
      }
    }

    uint32_t tocopy = u32_min(remaining, block_size);
    void* src = block_buf;

    if (block == start_block) {
      uint32_t first_offset = offset % block_size; 
      src = block_buf + first_offset;
      tocopy = u32_min(remaining, block_size - first_offset);
    }

    if (lba) {
      memcpy(buf, src, tocopy);
    } else {
      memset(buf, 0x00, tocopy);
    }

    buf += tocopy;
    remaining -= tocopy;
  }

  kfree(block_buf);
  return count;
}