#include <block.h>
#include <types.h>
#include <string.h>

int partition_read(struct block_dev* bdev, uint64_t lba, uint64_t count, struct phys_iovec* iovec, uint16_t iovec_count) {
  struct partition_info* pinfo = (struct partition_info*)bdev->priv_data;
  uint64_t real_lba = pinfo->lba_start + lba;
  return pinfo->parent->read(pinfo->parent, real_lba, count, iovec, iovec_count);
}

int partition_scan(struct block_dev* bdev) {
  int res = 0;
  int count = 0;
  struct block_dev* pbdevs[MBR_ENTRY_COUNT] = {NULL};

  uint32_t size = bdev->sector_size > 512 ? bdev->sector_size : 512;
  char* buf = kmalloc(size);
  if (!buf) {
    return -ENOMEM;
  }

  struct phys_iovec vec[] = {{.addr = (uint64_t)VIRT_TO_PHYS(buf), .length = size}};
  int status = bdev->read(bdev, 0, 1, vec, 1);
  if (status != 0) {
    res = -EIO;
    goto exit;
  }

  struct mbr_partition_entry* entry = (struct mbr_partition_entry*)(buf + MBR_OFFSET);
  for (int i = 0; i < MBR_ENTRY_COUNT; i++, entry++) {
    if (entry->sys_id == 0x00) {
      continue;
    }

    struct block_dev* pbdev = kzalloc(sizeof(struct block_dev));
    if (!pbdev) {
      res = -ENOMEM;
      goto exit;
    }

    pbdevs[count++] = pbdev;

    struct partition_info* pinfo = kzalloc(sizeof(struct partition_info));
    if (!pinfo) {
      res = -ENOMEM;
      goto exit;
    }

    char suffix[] = "px";
    suffix[1] = '0' + i;
    strcpy(pbdev->name, bdev->name);
    strncat(pbdev->name, suffix, sizeof(pbdev->name) - strlen(bdev->name) - 1);
    pbdev->sector_count = entry->sector_count;
    pbdev->sector_size = bdev->sector_size;
    pbdev->priv_data = (void*)pinfo;
    pbdev->read = partition_read;
    pinfo->lba_start = entry->lba_start;
    pinfo->parent = bdev;
  }

exit:
  for (int i = 0; i < count; i++) {
    if (pbdevs[i] && res != 0) {
      if (pbdevs[i]->priv_data) {
        kfree(pbdevs[i]->priv_data);
      }
      kfree(pbdevs[i]);
    } else if (pbdevs[i] && res == 0) {
      block_dev_register(pbdevs[i]);
    }
  }

  kfree(buf);
  return res == 0 ? count : res;
}