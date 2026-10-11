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

// only copies the name, and sets ->parent to parent.
// you have to link the parent to the child manually, set sb, siblings, and set inode.
// Note: must be checked with IS_ERR().
struct dentry* d_alloc(struct dentry* parent, const char* name) {
  struct dentry* d = kzalloc(sizeof(struct dentry));
  if (!d)
    return ERR_PTR(-ENOMEM);

  strncpy(d->name, name, sizeof(d->name) - 1);
  d->parent = parent;

  return d;
}

// links the child to parent, child to siblings, and inode.
void d_add(struct dentry* child, struct inode* inode) {
  child->inode = inode;

  if (likely(child->parent)) {
    child->next_sibling = child->parent->first_child;
    child->parent->first_child = child;
  }
}