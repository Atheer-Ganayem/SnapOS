#include <vfs.h>
#include <string.h>
#include <fs/ext2.h>
#include <block.h>

struct dentry* vfs_root = NULL;

static struct fs_driver fs_drivers[MAX_FS_DRIVERS];
static int fs_drivers_count = 0;

void vfs_init() {
  ext2_register(&fs_drivers[fs_drivers_count++]);
}

int vfs_scan_partitions() {
  int count = 0;
  struct block_dev** reg_devs = get_registered_devs();
  int len = get_registered_devs_len();
  
  for (int i = 0; i < len; i++) {
    int res = partition_scan(reg_devs[i]);
    if (res < 0) return res;
    count += res;
  }

  return count;
}

int vfs_mount_root() {
  struct block_dev** bdevs = get_registered_devs();
  int bdevs_count = get_registered_devs_len();

  for (int j = 0; j < bdevs_count; j++) {
    struct block_dev* bdev = bdevs[j];
    for (int i = 0; i < fs_drivers_count; i++) {
      if (fs_drivers[i].probe && fs_drivers[i].mount) {
        struct fs_probe_info info;
        if (fs_drivers[i].probe(bdev, &info) == 0) {
          struct superblock* sb = fs_drivers[i].mount(bdev);
          if (IS_ERR(sb)) continue;

          vfs_root = sb->root;
          return 0;
        }
      }
    }
  }

  return -ENOENT;
}

// DJB2 hashing algorithm.
uint32_t vfs_hash_name(const char* str, uint32_t len) {
  uint32_t hash = 5381; 
  
  for (uint32_t i = 0; i < len; i++) {
    hash = ((hash << 5) + hash) + str[i];
  }

  return hash;
}

// only copies the name, and sets ->parent to parent.
// you have to link the parent to the child manually, set sb, siblings, and set inode.
// doesnt assume `qstr` will survive after function calls.
// Note: must be checked with IS_ERR().
struct dentry* d_alloc(struct dentry* parent, const struct qstr* qstr) {
  struct dentry* d = kzalloc(sizeof(struct dentry));
  if (!d)
    return ERR_PTR(-ENOMEM);

  d->name.str = kmalloc(qstr->len + 1);
  if (!d->name.str) {
    kfree(d);
    return ERR_PTR(-ENOMEM);
  }

  memcpy(d->name.str, qstr->str, qstr->len);
  d->name.str[qstr->len] = '\0';
  d->name.len = qstr->len;
  d->name.hash = qstr->hash;

  d->parent = parent;

  return d;
}

void d_free(struct dentry* d) {
  if (d && d->name.str) kfree(d->name.str);
  if (d) kfree(d);
}

// links the child to parent, child to siblings, and inode.
void d_add(struct dentry* child, struct inode* inode) {
  child->inode = inode;

  if (likely(child->parent)) {
    child->next_sibling = child->parent->first_child;
    child->parent->first_child = child;
  }
}

struct dentry* vfs_lookup_direct(struct dentry* parent, const struct qstr* name) {
  struct dentry* cur = parent->first_child;

  while (cur) {
    if (cur->name.hash == name->hash &&
        cur->name.len == name->len &&
        memcmp(cur->name.str, name->str, name->len) == 0)
          return cur;

    cur = cur->next_sibling;
  }

  struct dentry* child = d_alloc(parent, name);
  if (IS_ERR(child)) {
    return child;
  }

  int res = parent->inode->iops->lookup(parent->inode, child);
  if (res < 0) {
    d_free(child);
    return ERR_PTR(res);
  }

  return child;
}

// @param path: garantueed null terminated string.
struct dentry* vfs_lookup(const char* path) {
  struct dentry* cur = vfs_root;
  
  while (*path == '/') path++;

  while (*path) {
    if (!S_ISDIR(cur->inode->mode))
      return ERR_PTR(-ENOTDIR);
    
    const char* delim = path;
    while (*delim && *delim != '/') delim++;

    struct qstr token = {
      .str = (char*)path,
      .len = delim - path,
      .hash = vfs_hash_name(path, delim - path),
    };

    struct dentry* child = vfs_lookup_direct(cur, &token);
    if (IS_ERR(child)) {
      return child;
    }

    cur = child;
    path = delim;

    while (*path == '/') path++;
  }

  return cur;
}

struct file* vfs_open(const char* path) {
  struct dentry* d = vfs_lookup(path);
  if (IS_ERR(d))
    return (struct file*)d;

  if (!S_ISREG(d->inode->mode))
    return ERR_PTR(-ENOTDIR);

  struct file* f = kzalloc(sizeof(struct file));
  if (!f)
    return ERR_PTR(-ENOMEM);

  f->dentry = d;
  // f->flags = later
  f->ref_count = 1;
  f->pos = 0;
  f->fops = d->inode->fops;

  return f;
}

ssize_t vfs_read(struct file* f, void* buf, size_t count) {
  if (!S_ISREG(f->dentry->inode->mode))
    return -EINVAL;

  if (unlikely(f->ref_count == 0))
    return -EINVAL;

  if (count > SSIZE_MAX)
    count = SSIZE_MAX;

  return f->fops->read(f, buf, count, &f->pos);
}

int vfs_close(struct file* f) {
  if (!f)
    return -EINVAL;

  f->ref_count--;

  if (f->ref_count == 0) {
    kfree(f);
  }

  return 0;
}

ssize_t vfs_seek(struct file* f, ssize_t offset ,int whence) {
  if (!S_ISREG(f->dentry->inode->mode))
    return -EINVAL;

  ssize_t new_pos;

  switch (whence) {
  case SEEK_SET:
    new_pos = offset;
    break;

  case SEEK_CUR:
    new_pos = f->pos + offset;
    break;

  case SEEK_END:
    // Currently now supported
    return -EINVAL;
  
  default:
    return -EINVAL;
  }

  if (new_pos < 0)
    return -EINVAL;

  new_pos = new_pos > (ssize_t)f->dentry->inode->size ? (ssize_t)f->dentry->inode->size : new_pos;

  f->pos = new_pos;

  return new_pos;
}

#include <sched.h>
#include <drivers/vga.h>
long sys_read(unsigned int fd, char* buf, size_t count) {
  if (fd >= OPEN_FILE_TABLE_COUNT)
  return -EBADF;
  
  
  struct task_struct* cur = sched_current();
  struct file* f = cur->fd[fd];
  if (!f)
    return -EBADF;
  // vga_print("in sysread");

  return f->fops->read(f, buf, count, &f->pos);
}