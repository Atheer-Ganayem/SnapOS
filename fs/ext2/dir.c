#include <fs/ext2.h>
#include <vfs.h>
#include <string.h>

// If found returns a pointer to the entry, if not, returns NULL.
// If it ecounters an error, it would return it (check via IS_ERR).
static struct ext2_dir_entry* search_dir_block(void* buf, uint32_t block_size, const char* name, uint8_t name_len) {
  void* ptr = buf;

  for (struct ext2_dir_entry* entry = (struct ext2_dir_entry*)ptr;
      ptr < buf + block_size;
      ptr += entry->entry_size, entry = (struct ext2_dir_entry*)ptr) {
    
    if (entry->entry_size == 0) {
      return ERR_PTR(-EUCLEAN);
    }

    if (!entry->inode)
      continue;

    if (entry->name_len_lower != name_len)
      continue;

    if (memcmp(entry->name, name, name_len) != 0)
      continue;;

    return entry;
  }

  return NULL;
}


int ext2_lookup(struct inode* dir, struct dentry* child) {
  uint32_t block_size = dir->sb->block_size;
  struct ext2_inode_info* i_info = (struct ext2_inode_info*)dir->priv_data;

  void* buf = ext2_alloc_block(block_size);
  if (!buf)
    return -ENOMEM;

  struct phys_iovec iovec[] = {{
    .addr = (uint64_t)VIRT_TO_PHYS(buf),
    .length = block_size
  }};

  uint32_t addr = i_info->inode->direct_ptrs[0];
  
  struct ext2_dir_entry* entry = NULL;

  for (uint64_t i = 0; i < 12 + (block_size / 4); i++) {
    // locating the block address and reading it.
    if (i < 12) {
      addr = i_info->inode->direct_ptrs[i];
    } else if (i < 12 + (block_size / 4)) {
      int res = ext2_init_singly(i_info);
      if (res < 0) {
        ext2_free_block(buf, block_size);
        return res;
      }

      addr = i_info->singly[i - 12];
    }

    if (!addr) {
      break;
    }

    // reading disk and searching.
    uint64_t lba = addr * dir->sb->sectors_per_block;
    int res = dir->sb->bdev->read(dir->sb->bdev, lba, dir->sb->sectors_per_block, iovec, 1);
    if (res < 0) {
      ext2_free_block(buf, block_size);
      return res;
    }

    entry = search_dir_block(buf, block_size, child->name.str, child->name.len);
    if (IS_ERR(entry)) {
      ext2_free_block(buf, block_size);
      return PTR_ERR(entry);
    }
    
    if (entry) {
      break;
    }
  }

  if (!entry) {
    ext2_free_block(buf, block_size);
    return -ENOENT;
  }

  struct inode* inode = ext2_read_inode(dir->sb->bdev, dir->sb, entry->inode);
  if (IS_ERR(inode)) {
    ext2_free_block(buf, block_size);
    return PTR_ERR(inode);
  }

  d_add(child, inode);
  child->sb = dir->sb;

  ext2_free_block(buf, block_size);
  return 0;
}