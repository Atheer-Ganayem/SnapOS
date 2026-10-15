#ifndef VFS_H
#define VFS_H

#include <types.h>
#include <block.h>

#define MAX_FS_DRIVERS 16

#define S_IFMT  0xF000
#define S_IFREG 0x8000
#define S_IFDIR 0x4000

#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

struct superblock;
struct fs_probe_info;
struct dentry;
struct file;
struct inode;

typedef ssize_t (*file_read_t)(struct file* file, void* buf, size_t count, size_t* offset);

typedef int (*fs_probe_t)(struct block_dev* bdev, struct fs_probe_info* info);

// Validates that the FS can be mountedm, initializes FS internal data,
// and returns a pointer is struct superblock.
// Must check with IS_ERR();
typedef struct superblock* (*fs_mount_t)(struct block_dev* bdev);

// Looks up child inside dir.
// If found returns 0, and sets child->inode.
// Otherwise, returns an err and doesnt attach a thing.
// The VFS is responisble for attaching the parent to the child.
typedef int (*inode_lookup_t)(struct inode* dir, struct dentry* child);

struct qstr {
  char* str;
  uint32_t len;
  uint32_t hash;
};

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
  uint32_t mode;  
  struct superblock* sb;

  const struct inode_operations* iops;
  const struct file_operations* fops;

  void* priv_data;
};


struct file {
  struct dentry* dentry;
  uint64_t pos;
  uint32_t ref_count;
  const struct file_operations* fops;
};

struct dentry {
  struct qstr name;
  struct inode* inode;
  struct dentry* parent;
  struct dentry* first_child;
  struct dentry* next_sibling;
  struct superblock* sb;
};


void vfs_init();
int vfs_scan_partitions();
int vfs_mount_root();

uint32_t vfs_hash_name(const char* str, uint32_t len);

struct dentry* d_alloc(struct dentry* parent, const struct qstr* name);
void d_free(struct dentry* d);
void d_add(struct dentry* child, struct inode* inode);

struct dentry* vfs_lookup(const char* path);
struct file* vfs_open(const char* path);
ssize_t vfs_read(struct file* f, void* buf, size_t count);
int vfs_close(struct file* f);
ssize_t vfs_seek(struct file* f, ssize_t offset ,int whence);

#endif