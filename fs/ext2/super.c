#include <fs/ext2.h>
#include <string.h>
#include <mm.h>
#include <vfs.h>
#include <paging.h>

int ext2_read_superblock(struct block_dev* bdev, struct ext2_superblock* out_sb) {
  uint32_t start_sector = EXT2_SUPERBLOCK_OFFSET / bdev->sector_size;
  uint32_t end_byte = EXT2_SUPERBLOCK_OFFSET + sizeof(struct ext2_superblock);
  uint32_t end_sector = (end_byte + bdev->sector_size - 1) / bdev->sector_size;
  uint32_t sector_count = end_sector - start_sector;
  uint32_t total_bytes = sector_count * bdev->sector_size;

  void* buf = kmalloc(total_bytes);
  if (!buf)
    return -ENOMEM;
  

  struct phys_iovec iovec[] = {{.addr = (uint64_t)VIRT_TO_PHYS(buf), .length = total_bytes}};
  int status = bdev->read(bdev, start_sector, sector_count, iovec, 1);
  if (status < 0) {
    kfree(buf);
    return status;
  }
  
  uint32_t offset = EXT2_SUPERBLOCK_OFFSET - (start_sector * bdev->sector_size);
  memcpy(out_sb, (struct ext2_superblock*)((uintptr_t)buf + offset), sizeof(struct ext2_superblock));

  kfree(buf);
  return 0;
}

int ext2_probe(struct block_dev* bdev, struct fs_probe_info* info) {
  struct ext2_superblock* super = kmalloc(sizeof(struct ext2_superblock));
  if (!super)
    return -ENOMEM;
  
  int status = ext2_read_superblock(bdev, super);
  if (status < 0) {
    kfree(super);
    return status;
  }

  uint32_t block_size = 1024 << super->log_block_size;

  if (super->ext2_signature != EXT2_SIGNATURE ||
      super->version_major != 1 ||
      (super->required_features & ~EXT2_FEATURE_REQUIRED_SUPPORTED) ||
      PAGE_SIZE < block_size) {
    kfree(super);
    return -EINVAL;
  }

  if (info) {
    strncpy(info->label, (char*)super->volume_name, sizeof(info->label));
    info->label[sizeof(info->label)-1] = 0x00;
  }

  kfree(super);
  return 0;
};

// Reads the block group descriptor table and assigns the appropriate in 'info'.
// If fails, it is garantueed that info->bgd_table is NULL.
static int ext2_fill_bgd_table_info(struct block_dev* bdev, struct ext2_fs_info* info) {
  info->inodes_per_group = info->superblock.inodes_per_blockgroup;
  info->block_per_group = info->superblock.blocks_per_blockgroup;
  info->block_group_count = (info->superblock.inode_count + info->inodes_per_group - 1) / info->inodes_per_group;

  uint32_t block_size = 1024 << info->superblock.log_block_size;

  uint32_t bgd_table_start_block = info->superblock.first_data_block + 1;
  uint32_t bgd_tbale_bytes = sizeof(struct ext2_blockgroup_desc) * info->block_group_count;
  uint32_t block_count = (bgd_tbale_bytes + block_size - 1) / block_size;

  struct ext2_blockgroup_desc* bgd_table = kmalloc(block_count * block_size);
  if (!bgd_table) {
    return -ENOMEM;
  }

  uint32_t sectors_per_block = block_size / bdev->sector_size;
  uint64_t lba = bgd_table_start_block * sectors_per_block;
  uint32_t lba_count = block_count * sectors_per_block;

  struct phys_iovec iovec[] = {{
    .addr = (uint64_t)VIRT_TO_PHYS(bgd_table), 
    .length = block_count * block_size
  }};

  int status = bdev->read(bdev, lba, lba_count, iovec, 1);
  if (status < 0) {
    kfree(bgd_table);
    return status;
  }

  info->bgd_table = bgd_table;

  return 0;
}

// must be checked with IS_ERR
static struct dentry* ext2_get_root(struct block_dev* bdev, struct superblock* sb) {
  struct inode* inode = ext2_read_inode(bdev, sb, EXT2_ROOT_DIR_INO);
  if (!inode)
    return ERR_PTR(-ENOMEM);

  struct qstr root_name = {
    .str = "/",
    .len = 1,
    .hash = vfs_hash_name("/", 1)
  };

  struct dentry* d = d_alloc(NULL, &root_name);
  if (IS_ERR(d)) {
    kfree(inode);
    return ERR_PTR(-ENOMEM);
  }

  d->sb = sb;
  d_add(d, inode);

  return d;
}

struct superblock* ext2_mount(struct block_dev* bdev) {
  void* res = 0;
  int status = 0;
  struct superblock* sb = NULL;
  struct ext2_fs_info* info = NULL;
  struct ext2_superblock* super = NULL;
  struct dentry* root = NULL;

  sb = kzalloc(sizeof(struct superblock));
  if (!sb) {
    res = ERR_PTR(-ENOMEM);
    goto exit_err;
  }
    
  info = kmalloc(sizeof(struct ext2_fs_info));
  if (!info) {
    res = ERR_PTR(-ENOMEM);
    goto exit_err;
  }
    

  super = &info->superblock;
  status = ext2_read_superblock(bdev, super);
  if (status < 0) {
    res = ERR_PTR(status);
    goto exit_err;
  }

  status = ext2_fill_bgd_table_info(bdev, info);
  if (status < 0) {
    res = ERR_PTR(status);
    goto exit_err;
  }

  sb->bdev = bdev;
  sb->block_size = 1024 << super->log_block_size;
  sb->sectors_per_block = sb->block_size / bdev->sector_size;
  sb->priv_data = info;

  root = ext2_get_root(bdev, sb);
  if (IS_ERR(root)) {
    res = root;
    goto exit_err;
  }

  sb->root = root;

  return sb;

exit_err:
  if (sb && sb->root) kfree(sb->root);
  if (sb) kfree(sb);
  if (info && info->bgd_table) kfree(info->bgd_table);
  if (info) kfree(info);

  return res;
}