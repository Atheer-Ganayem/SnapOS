#include <vfs.h>
#include <string.h>
#include <fs/ext2.h>
#include <block.h>
#include <drivers/vga.h>

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

kstatus_t vfs_mount_root() {
  struct block_dev** bdevs = get_registered_devs();
  int bdevs_count = get_registered_devs_len();

  for (int j = 0; j < bdevs_count; j++) {
    struct block_dev* bdev = bdevs[j];
    for (int i = 0; i < fs_drivers_count; i++) {
      if (fs_drivers[i].probe && fs_drivers[i].mount) {
        struct fs_probe_info info;
        struct superblock* sb;
        vga_putchar_color('-', VGA_COLOR_GREEN);
        vga_print(bdev->name);
        vga_print(": ");
        vga_print(fs_drivers[i].name);
        if (fs_drivers[i].probe(bdev, &info) == KSTATUS_SUCCESS) {
          vga_print(" probing passed ");
          if (fs_drivers[i].mount(bdev, &sb) == KSTATUS_SUCCESS) {
            vga_print(" mounting passed ");
            vfs_root = sb->root;
            return KSTATUS_SUCCESS;
          }
        }
        vga_print_color("-\n", VGA_COLOR_GREEN);
      }
    }
  }

  return KSTATUS_ERR_NO_ROOT;
}

struct dentry* d_alloc(struct dentry* parent, const char* name) {
  struct dentry* d = kzalloc(sizeof(struct dentry));
  if (!d)
    return NULL;

  strncpy(d->name, name, sizeof(d->name) - 1);
  d->parent = parent;

  if (parent) {
    d->next_sibling = parent->first_child;
    parent->first_child = d;
  }

  return d;
}