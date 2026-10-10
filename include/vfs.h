#ifndef VFS_H
#define VFS_H

#include <types.h>
#include <block.h>

#define MAX_FS_DRIVERS 16

struct superblock;
struct fs_probe_info;
struct dentry;
struct file;
struct inode;

typedef ssize_t (*file_read_t)(struct file* file, void* buf, size_t count, uint64_t offset);

typedef kstatus_t (*fs_probe_t)(struct block_dev* bdev, struct fs_probe_info* info);
typedef kstatus_t (*fs_mount_t)(struct block_dev* bdev, struct superblock** sb);

typedef int (*inode_lookup_t)(struct inode* dir, struct dentry* child);
// typedef kstatus_t (*inode_create_t)(struct inode* dir, struct dentry* child);
// typedef kstatus_t (*inode_mkdir_t)(struct inode* dir, struct dentry* child);

struct fs_driver {
  char name[16];
  fs_probe_t probe;
  fs_mount_t mount;
};

struct fs_probe_info {
  char label[32];
};

struct superblock {
  struct block_dev* bdev;
  struct dentry* root;
  uint32_t block_size;
  uint32_t sectors_per_block;
  void* priv_data;
};


struct inode_operations {
  inode_lookup_t lookup;
};

struct file_operations {
  file_read_t read;
};

struct inode {
  uint32_t ino;
  uint64_t size;
  struct superblock* sb;

  const struct inode_operations* iops;
  const struct file_operations* fops;

  void* priv_data;
};


struct file {
  struct dentry* dentry;
  uint64_t pos;
  uint32_t flags;
  const struct file_operations* fops;
};

struct dentry {
  char name[256];
  struct inode* inode;
  struct dentry* parent;
  struct dentry* first_child;
  struct dentry* next_sibling;
  struct superblock* sb;
};

struct dentry* d_alloc(struct dentry* parent, const char* name);

void vfs_init();
int vfs_scan_partitions();
kstatus_t vfs_mount_root();


#endif