#include <block.h>

struct block_dev* registered_block_devices[BLOCK_DEV_MAX] = {0};
int index = 0;

void block_dev_register(struct block_dev* bdev) {
  if (index >= BLOCK_DEV_MAX) {
    return;
  }

  registered_block_devices[index++] = bdev;
}

struct block_dev** get_registered_devs() {
  return registered_block_devices;
}

int get_registered_devs_len() {
  return index;
}