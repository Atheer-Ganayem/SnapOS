#ifndef BLOCK_H
#define BLOCK_H

#include <types.h>
#include <stdint.h>
#include <mm.h>

#define BLOCK_DEV_NAME_MAX  32
#define BLOCK_DEV_MAX       32

#define MBR_OFFSET 446
#define MBR_ENTRY_COUNT 4

struct block_dev;

typedef kstatus_t (*bdev_read_t)(struct block_dev* bdev, uint64_t lba, uint64_t count, struct phys_iovec* iovec, uint16_t iovec_count);

struct block_dev {
  char name[BLOCK_DEV_NAME_MAX];
  uint32_t sector_size;
  uint64_t sector_count;
  void* priv_data;
  bdev_read_t read;
};

struct partition_info {
  struct block_dev* parent;
  uint64_t lba_start;
};

struct mbr_partition_entry {
  uint8_t status;
  uint8_t chs0[3]; // legacy CHS stuff
  uint8_t sys_id;
  uint8_t chs1[3]; // legacy CHS stuff
  uint32_t lba_start;
  uint32_t sector_count;
} __attribute__((packed));

void block_dev_register(struct block_dev* bdev);

struct block_dev** get_registered_devs();
int get_registered_devs_len();

int partition_scan(struct block_dev* bdev);

#endif