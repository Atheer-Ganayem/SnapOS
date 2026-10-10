#ifndef EXT2_H
#define EXT2_H

#include <stdint.h>
#include <types.h>
#include <block.h>
#include <stdbool.h>
#include <vfs.h>

#define EXT2_MODE_DIR 0x4000
#define EXT2_MODE_FILE 0x8000
#define EXT2_ISDIR(m) (m & EXT2_MODE_DIR)
#define EXT2_ISREG(m) (m & EXT2_MODE_FILE)

#define EXT2_SUPERBLOCK_OFFSET 1024
#define EXT2_SIGNATURE 0xEF53

#define EXT2_FEATURE_REQUIRED_COMPRESSION  0x0001
#define EXT2_FEATURE_REQUIRED_FILETYPE     0x0002 // Directory entries store file_type
#define EXT2_FEATURE_REQUIRED_RECOVER      0x0004 // ext3 journal needs replay
#define EXT2_FEATURE_REQUIRED_JOURNAL_DEV  0x0008
#define EXT2_FEATURE_REQUIRED_META_BG      0x0010

#define EXT2_FEATURE_REQUIRED_SUPPORTED (EXT2_FEATURE_REQUIRED_FILETYPE)

#define EXT2_ROOT_DIR_INO 2

struct ext2_superblock {
  // base fields
  uint32_t inode_count;
  uint32_t block_count;
  uint32_t superuser_rsv_block_count;
  uint32_t free_block_count;
  uint32_t free_inode_count;
  uint32_t first_data_block;
  uint32_t log_block_size;
  uint32_t log_frag_size;
  uint32_t blocks_per_blockgroup;
  uint32_t fragments_per_blockgroup;
  uint32_t inodes_per_blockgroup;
  uint32_t last_mount_time;
  uint32_t last_written_time;
  uint16_t mount_count;
  uint16_t max_mount_count;
  uint16_t ext2_signature;
  uint16_t fs_state;
  uint16_t error_action;
  uint16_t version_minor;
  uint32_t last_check_time;
  uint32_t check_interval;
  uint32_t os_id;
  uint32_t version_major;
  uint16_t default_res_uid;
  uint16_t default_res_gid;

  // extended
  uint32_t first_non_rsv_inode;
  uint16_t inode_struct_size;
  uint16_t superblock_blockgroup;
  uint32_t optinal_features;
  uint32_t required_features;
  uint32_t ro_compat_features; // read-only compatible feature flags
  uint8_t fs_id[16];
  uint8_t volume_name[16]; // c style, with null byte.
  uint8_t last_mounted_path[64];
  uint32_t compression_algorithm;
  uint8_t prealloc_blocks;
  uint8_t prealloc_dir_blocks;
  uint16_t unused0;
  uint8_t journal_id[16];
  uint32_t journal_inode;
  uint32_t journal_dev;
  uint32_t orphan_inode_list_head;
  uint8_t unused1[1024 - 236];
} __attribute__((packed));

struct ext2_blockgroup_desc {
  uint32_t block_bitmap_addr; // addr in blocks
  uint32_t inode_bitmap_addr; // addr in blocks
  uint32_t inode_table_addr; // addr in blocks
  uint16_t free_blocks_count;
  uint16_t free_inodes_count;
  uint16_t dir_count;
  uint8_t rsv[14];
} __attribute__((packed));

struct ext2_inode {
  uint16_t mode;
  uint16_t user_id;
  uint32_t sizel; // lower 32 bits of size (size in bytes).
  uint32_t last_access_time;
  uint32_t creation_time;
  uint32_t last_mod_time;
  uint32_t deletion_time;
  uint16_t group_id;
  uint16_t hard_links_count;
  uint32_t sector_count;
  uint32_t flags;
  uint32_t os_specific_val0;
  uint32_t direct_ptrs[12];
  uint32_t singly;
  uint32_t doubly;
  uint32_t triply;
  uint32_t generation_num;
  uint32_t extended_attr_block;
  uint32_t sizeu; // upper 32 bits of size (in bytes).
  uint32_t fragment_block_addr;
  uint8_t os_specific_val1[12];
} __attribute__((packed));

struct ext2_dir_entry {
  uint32_t inode;
  uint16_t entry_size;
  uint8_t name_len_lower;
  uint8_t type;
  char name[];
} __attribute__((packed));

struct ext2_inode_info {
  struct ext2_inode* inode;
  uint64_t size;
  struct superblock* sb;
  uint32_t* singly;
  uint32_t* doubly;
  uint32_t* triply;
};

struct ext2_fs_info {
  struct ext2_superblock superblock;
  uint32_t sectors_per_block;
  uint32_t inodes_per_group;
  uint32_t block_per_group;
  uint32_t block_group_count;
  struct ext2_blockgroup_desc* bgd_table;
};

int ext2_probe(struct block_dev* bdev, struct fs_probe_info* info);

int ext2_init_singly(struct ext2_inode_info* i_info);

struct inode* ext2_read_inode(struct block_dev* bdev, struct superblock* sb, uint32_t ino);

int ext2_mount(struct block_dev* bdev, struct superblock** out_sb);

int ext2_lookup(struct inode* dir, struct dentry* child);

void ext2_register(struct fs_driver* entry);

ssize_t ext2_read_file(struct file* file, void* buf, size_t count, uint64_t offset);

#endif