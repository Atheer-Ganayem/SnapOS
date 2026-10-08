#include <block.h>
#include <types.h>
#include <string.h>

kstatus_t partition_read(struct block_dev* bdev, uint64_t lba, uint64_t count, struct phys_iovec* iovec, uint16_t iovec_count) {
  struct partition_info* pinfo = (struct partition_info*)bdev->priv_data;
  uint64_t real_lba = pinfo->lba_start + lba;
  return pinfo->parent->read(pinfo->parent, real_lba, count, iovec, iovec_count);
}

int partition_scan(struct block_dev* bdev) {
  uint32_t size = bdev->sector_size > 512 ? bdev->sector_size : 512;
  char* buf = kmalloc(size);
  if (!buf) {
    return -KSTATUS_ERR_NO_MEMORY;
  }

  struct phys_iovec vec[] = {{.addr = (uint64_t)VIRT_TO_PHYS(buf), .length = size}};
  kstatus_t status = bdev->read(bdev, 0, 1, vec, 1);
  if (status != KSTATUS_SUCCESS) {
    kfree(buf);
    return -KSTATUS_ERR_IO;
  }

  int count = 0;
  struct mbr_partition_entry* entry = (struct mbr_partition_entry*)(buf + MBR_OFFSET);
  for (int i = 0; i < MBR_ENTRY_COUNT; i++, entry++) {
    if (entry->sys_id == 0x00) {
      continue;
    }

    struct block_dev* pbdev = kzalloc(sizeof(struct block_dev));
    if (!pbdev) {
      kfree(buf);
      return KSTATUS_ERR_NO_MEMORY;
    }

    struct partition_info* pinfo = kzalloc(sizeof(struct partition_info));
    if (!pinfo) {
      kfree(buf);
      kfree(pbdev);
      return KSTATUS_ERR_NO_MEMORY;
    }

    char suffix[] = "px";
    suffix[1] = '0' + i;
    strcpy(pbdev->name, bdev->name);
    strncat(pbdev->name, suffix, sizeof(pbdev->name) - strlen(bdev->name) - 1);
    pbdev->sector_count = entry->sector_count;
    pbdev->priv_data = (void*)pinfo;
    pbdev->read = partition_read;
    pinfo->lba_start = entry->lba_start;
    pinfo->parent = bdev;

    block_dev_register(pbdev);
    count++;
  }

  kfree(buf);
  return count;
}