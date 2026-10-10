#include <fs/ext2.h>
#include <string.h>
#include <mm.h>
#include <vfs.h>

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

  if (super->ext2_signature != EXT2_SIGNATURE ||
      super->version_major != 1 ||
      (super->required_features & ~EXT2_FEATURE_REQUIRED_SUPPORTED)) {
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

static struct dentry* ext2_get_root(struct block_dev* bdev, struct superblock* sb) {
  struct inode* inode = ext2_read_inode(bdev, sb, EXT2_ROOT_DIR_INO);
  if (!inode)
    return NULL;

  struct dentry* d = d_alloc(NULL, "/");
  if (!d) {
    kfree(inode);
    return NULL;
  }

  d->sb = sb;
  d_add(d, inode);

  return d;
}

int ext2_mount(struct block_dev* bdev, struct superblock** out_sb) {
  struct superblock* sb = kzalloc(sizeof(struct superblock));
  if (!sb)
    return -ENOMEM;
    
  struct ext2_fs_info* info = kmalloc(sizeof(struct ext2_fs_info));
  if (!info) {
    kfree(sb);
    return -ENOMEM;
  }
    

  struct ext2_superblock* super = &info->superblock;
  int status = ext2_read_superblock(bdev, super);
  if (status < 0) {
    kfree(sb);
    kfree(info);
    return status;
  }

  status = ext2_fill_bgd_table_info(bdev, info);
  if (status < 0) {
    kfree(sb);
    kfree(info);
    return status;
  }

  sb->bdev = bdev;
  sb->block_size = 1024 << super->log_block_size;
  sb->sectors_per_block = sb->block_size / bdev->sector_size;
  sb->priv_data = info;

  struct dentry* root = ext2_get_root(bdev, sb);
  if (!root) {
    kfree(sb);
    kfree(info);
    return -ENOENT;
  }

  sb->root = root;

  *out_sb = sb;

  return 0;
}