__asm__(".code16gcc\n");

#include "loadkernel.h"

int loadkernel(uint32_t parition_start_lba) {
  pstart = parition_start_lba;

  uint8_t* superblock = malloc(1024);
  if (!superblock) {
    return 1;
  }
  read_and_parse_superblock(superblock);

  uint8_t* rootdir = rootdir_inode();
  if (!rootdir) {
    return 1;
  } 

  uint32_t kernel_inode_num = search_dir(rootdir, KERNEL_FILENAME);
  if (!kernel_inode_num) {
    return 1;
  }

  uint8_t* kernel_inode = get_inode(kernel_inode_num);
  if (!kernel_inode) {
    return 1;
  }

  int res = load_kernel_binary(kernel_inode);
  if (res) {
    return 1;
  }

  return 0;
}

void read(uint32_t lba, uint32_t sectors, void* buf) {
  while (sectors > 0) {
    size_t sectors_toread = min(TEMP_BUF_SIZE / SECTOR_SIZE, sectors); 
    
    issue_read_to_disk(lba, sectors_toread, temp_buf);
    memcpy(buf, temp_buf, sectors_toread * SECTOR_SIZE);

    lba += sectors_toread;
    buf = (uint8_t*)buf + (sectors_toread * SECTOR_SIZE);
    
    sectors -= sectors_toread; 
  }
}


void read_and_parse_superblock(uint8_t* superblock) {
  read(pstart + 2, 2, superblock);
  blocksize =  1024 << (*(uint32_t*)(superblock+24));
  inode_size = *(uint16_t*)(superblock+88);
  sectors_per_block = blocksize/SECTOR_SIZE;
  blocks_per_bg = *(uint32_t*)(superblock+32);
  inodes_per_bg = *(uint32_t*)(superblock+40);
}

uint8_t* rootdir_inode() {
  size_t bgdt_first_block = ((4 - 1)/sectors_per_block) + 1; 
  bgdt_lba = pstart + (bgdt_first_block * sectors_per_block);
  uint8_t* bgdt = malloc(SECTOR_SIZE);
  if (!bgdt) {
    return NULL;
  }
  
  read(bgdt_lba, 1, bgdt);
  
  // read first 2 inodes to find inode 2 (root dir) inode.
  size_t inode_table_starting_block = *(uint32_t*)(bgdt + 8);
  free(SECTOR_SIZE);
  
  uint8_t* inode_table = malloc(SECTOR_SIZE);
  if (!inode_table) {
    return NULL;
  }

  read(pstart + inode_table_starting_block * sectors_per_block, 1, inode_table);

  return inode_table + inode_size;
}

// returns inode number of filename if it exists, 
// if not, returns zero.
// this works under the assumption that the file is in the direct pointers of the dir.
uint32_t search_dir(uint8_t* dir_inode, char* filename) {
  uint32_t* direct_ptr = (uint32_t*)(dir_inode + 40);
  uint32_t inode_num = 0;
  
  uint8_t* buf = malloc(blocksize);
  if (!buf) {
    return 0;
  } 

  for (int i = 0; i < DIRECT_BLOCK_POINTERS_COUNT; i++, direct_ptr++) {
    if (!*direct_ptr) {
      continue;
    }

    size_t block_lba = pstart + (*direct_ptr * sectors_per_block);
    read(block_lba, sectors_per_block, buf);

    inode_num = search_dir_block(buf, filename);
    if (inode_num) {
      break;
    }
  }

  free(blocksize);
  return inode_num;
}

uint32_t search_dir_block(uint8_t* block, char* filename) {
  uint8_t* entry = block;
  size_t filename_len = strlen(filename);

  while (entry + 6 < block + blocksize) {
    uint16_t entry_size = *(uint16_t*)(entry+4);
    if (entry + entry_size > block + blocksize) {
      return 0;
    }

    uint8_t name_len = *(uint8_t*)(entry+6);
    if (name_len != filename_len) {
      entry += entry_size;
      continue;
    }

    if (strncmp(filename, (char*)(entry+8), filename_len)) {
      return *(uint32_t*)entry;
    }

    entry += entry_size;
  }

  return 0;
}

uint8_t* get_inode(uint32_t inode_num) {
  // retrieve the block number of the inode table
  size_t bg_num = (inode_num - 1) / inodes_per_bg;
  size_t bg_desc_offset = bg_num * BLOCK_GROUP_DESCRIPTOR_SIZE;

  uint8_t* buf = malloc(SECTOR_SIZE);
  if (!buf) {
    return NULL;
  }

  read(bgdt_lba + (bg_desc_offset/SECTOR_SIZE), 1, buf);

  uint8_t* bg_desc = buf + bg_desc_offset % SECTOR_SIZE;
  size_t inode_table_block = *(uint32_t*)(bg_desc+8);

  // find and read the inode
  size_t index = (inode_num - 1) % inodes_per_bg;
  size_t lba_to_read = pstart + inode_table_block * sectors_per_block + (index * inode_size)/SECTOR_SIZE;
  read(lba_to_read, 1, buf);

  return buf + (index * inode_size)%SECTOR_SIZE;
}

int load_kernel_binary(uint8_t* inode) {
  size_t filesize = *(uint32_t*)(inode+4);
  size_t offset = 0;
  uint32_t* direct_ptr = (uint32_t*)(inode + 40);

  // read direct blocks
  uint8_t* buf = malloc(blocksize);
  if (!buf) {
    return -1;
  }

  size_t n = load_data((void*)KERNEL_LOAD_DEST, buf, direct_ptr, DIRECT_BLOCK_POINTERS_COUNT, filesize - offset);
  offset += n;

  if (offset >= filesize) {
    free(blocksize);
    return 0;
  }

  // read singly indirect block
  uint32_t* singly_indirect_ptr = malloc(blocksize); 
  if (!singly_indirect_ptr) {
    free(blocksize);
    return -1;
  }
  uint32_t singly_indirect_ptr_lba = pstart + (*(uint32_t*)(inode + 88)) * sectors_per_block;
  read(singly_indirect_ptr_lba, sectors_per_block, (void*)singly_indirect_ptr);

  n = load_data((void*)KERNEL_LOAD_DEST + offset, buf, singly_indirect_ptr, blocksize / 4, filesize - offset);
  offset += n;

  if (offset >= filesize) {
    free(2 * blocksize);
    return 0;
  }

  // read doubly indirect block
  uint32_t* doubly_indirect_ptr = malloc(blocksize); 
  if (!doubly_indirect_ptr) {
    free(2 * blocksize);
    return -1;
  }
  uint32_t doubly_indirect_lba = pstart + (*(uint32_t*)(inode + 92)) * sectors_per_block;
  read(doubly_indirect_lba, sectors_per_block, (void*)doubly_indirect_ptr);

  for (uint32_t* ptr = doubly_indirect_ptr; (void*)ptr < ((void*)doubly_indirect_ptr) + blocksize; ptr++) {
    if (offset >= filesize) {
      free(3 * blocksize);
      return 0;
    }

    if (!*ptr) {
      size_t tocopy = min((blocksize/4) * blocksize, filesize - offset);
      memset((void*)KERNEL_LOAD_DEST + offset, 0x00, tocopy);
      offset += tocopy;
      continue;
    }

    read(pstart + ((*ptr) * sectors_per_block), sectors_per_block, singly_indirect_ptr);

    n = load_data((void*)KERNEL_LOAD_DEST + offset, buf, singly_indirect_ptr, blocksize / 4, filesize - offset);
    offset += n;
  }

  free(3 * blocksize);
  return 0;
}

size_t load_data(void* dest, uint8_t* buf, uint32_t* ptr, size_t n, size_t max) {
  size_t offset = 0;

  for (int i = 0; i < n; i++, ptr++) {
    if (offset >= max) {
      return offset;
    }

    if (!*ptr) {
      memset(dest + offset, 0x00, blocksize);
      offset += blocksize;
      continue;
    }

    size_t block_lba = pstart + (*ptr * sectors_per_block);
    read(block_lba, sectors_per_block, buf);
    size_t tocopy = min(blocksize, max - offset);
    memcpy(dest + offset, buf, tocopy);

    offset += tocopy;
  }

  return offset;
}


/// some utils ////

void* malloc(size_t size) {
  if (size <= remaining) {
    brk += size;
    remaining -= size;
    return brk - size;
  }
  return NULL;
}

void free(size_t size) {
  remaining += size;
  brk -= size;
}

size_t strlen(char* s) {
  size_t len = 0;
  while (s[len]) len++;
  return len;
} 

bool strncmp(char* s1, char* s2, size_t n) {
  int i = 0;
  for (; i < n && s1[i] && s2[i] && s1[i] == s2[i]; i++) {}
  return i == n;
} 

void* memcpy(void* restrict dest, void* restrict src, size_t size) {
  for (int i = 0; i < size; i++) {
    ((char*)dest)[i] = ((char*)src)[i];
  }

  return dest;
} 

size_t min(size_t a, size_t b) {
  return a < b ? a : b;
}

void* memset(void* dest, char c, size_t n) {
  for (int i = 0; i < n; i++) {
    ((char*)dest)[i] = c;
  }
  return dest;
}