#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define KERNEL_FILENAME "kernel.bin"
#define TEMP_BUF_ADDR 0x1110
#define TEMP_BUF_SIZE 4096
#define HEAP_ADDR 0x64000
#define HEAP_SIZE 1024 * 80
#define SECTOR_SIZE 512
#define DIRECT_BLOCK_POINTERS_COUNT 12
#define BLOCK_GROUP_DESCRIPTOR_SIZE 32
#define KERNEL_LOAD_DEST 0x1000000U

extern void issue_read_to_disk(uint32_t lba, uint32_t sectors, void* buf);

void* malloc(size_t size);
void free(size_t size);
size_t strlen(char* s);
bool strncmp(char* s1, char* s2, size_t n);
void* memcpy(void* restrict dest, void* restrict src, size_t size);
size_t min(size_t a, size_t b);
void* memset(void* dest, char c, size_t n);

void read_and_parse_superblock(uint8_t* superblock);
uint8_t* rootdir_inode();
uint8_t* get_inode(uint32_t inode_num);
uint32_t search_dir(uint8_t* dir_inode, char* filename);
uint32_t search_dir_block(uint8_t* block, char* filename);
int load_kernel_binary(uint8_t* inode);
size_t load_data(void* dest, uint8_t* buf, uint32_t* ptr, size_t n, size_t max);

uint8_t* temp_buf = (uint8_t*)TEMP_BUF_ADDR;
uint8_t* brk = (uint8_t*)HEAP_ADDR;
size_t remaining = HEAP_SIZE;

uint32_t pstart;
size_t blocksize, sectors_per_block, blocks_per_bg, inode_size, inodes_per_bg, bgdt_lba;