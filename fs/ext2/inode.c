#include <fs/ext2.h>
#include <vfs.h>
#include <string.h>

struct file_operations ext2_file_fops = { .read = ext2_read_file };
struct inode_operations ext2_file_iops = { .lookup = NULL };

struct file_operations ext2_dir_fops = { .read = NULL };
struct inode_operations ext2_dir_iops = { .lookup = ext2_lookup };

static inline uint64_t ext2_inode_size(struct ext2_inode* inode) {
  return ((uint64_t)inode->sizel) | ((uint64_t)inode->sizeu << 32);
} 

struct inode* ext2_read_inode(struct block_dev* bdev, struct superblock* sb, uint32_t ino) {
  struct ext2_fs_info* info = (struct ext2_fs_info*)sb->priv_data; 
  uint32_t block_group = (ino - 1) / info->inodes_per_group;
  if (block_group >= info->block_group_count) {
    return NULL;
  }

  uint32_t inode_size = ((struct ext2_fs_info*)sb->priv_data)->superblock.inode_struct_size;
  uint32_t index = (ino - 1) % info->inodes_per_group;
  uint32_t inodes_per_block = sb->block_size / inode_size;
  uint32_t block_offset = index / inodes_per_block;
  uint32_t index_in_block = index % inodes_per_block;

  uint32_t inode_table_base = info->bgd_table[block_group].inode_table_addr;
  uint32_t target_block = inode_table_base + block_offset;
  uint32_t lba = target_block * sb->sectors_per_block;
  
  void* block_buf = kmalloc(sb->block_size);
  struct ext2_inode* ext2_inode = kmalloc(sizeof(struct ext2_inode));
  struct inode* inode = kmalloc(sizeof(struct inode));
  struct ext2_inode_info* i_info = kzalloc(sizeof(struct ext2_inode_info));

  if (!block_buf || !ext2_inode || !inode || !i_info)
    goto out_err;

  struct phys_iovec iovec[] = {{
    .addr = (uint64_t)VIRT_TO_PHYS(block_buf),
    .length = sb->block_size,
  }};

  if (bdev->read(bdev, lba, sb->sectors_per_block, iovec, 1) != KSTATUS_SUCCESS)
    goto out_err;

  memcpy(ext2_inode, block_buf + (inode_size * index_in_block), sizeof(struct ext2_inode));

  i_info->inode = ext2_inode;
  i_info->size = ext2_inode_size(ext2_inode);
  i_info->sb = sb;

  inode->priv_data = i_info;
  inode->sb = sb;
  inode->ino = ino;
  inode->size = i_info->size;
  
  if (EXT2_ISDIR(ext2_inode->mode)) {
    inode->fops = &ext2_dir_fops;
    inode->iops = &ext2_dir_iops;
  } else if (EXT2_ISREG(ext2_inode->mode)) {
    inode->fops = &ext2_file_fops;
    inode->iops = &ext2_file_iops;
  } else {
    inode->fops = NULL;
    inode->iops = NULL;
  }

  kfree(block_buf);
  return inode;

out_err:
  if (block_buf) kfree(block_buf);
  if (ext2_inode) kfree(ext2_inode);
  if (inode) kfree(inode);
  if (!i_info) kfree(i_info);
  return NULL;
}