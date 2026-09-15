#include <mm.h>
#include <paging.h>
#include <stddef.h>

#include <drivers/vga.h>

#define MIN_TIER        32
#define MAX_TIER        1024
#define TOO_BIG_SIZE    -1
#define MAGIC           0xFC6792DA

struct pool_page_header {
  uint32_t magic;
  uint8_t tier_idx;
  uint8_t count;
};

struct free_slot {
  struct free_slot* next;
};

static struct {
  uint16_t slot_size;
  struct free_slot* free_head;
} tiers[] = {
  {32, NULL},
  {64, NULL},
  {128, NULL},
  {256, NULL},
  {512, NULL},
  {1024, NULL},
};

struct big_alloc_node {
  void* vaddr;
  size_t pages_count;
  struct big_alloc_node* next;
};

struct big_alloc_node* big_alloc_head = NULL;

static int size_to_tier_idx(size_t size) {
  size_t n = sizeof(tiers) / sizeof(tiers[0]);
  for (size_t i = 0; i < n; i++) {  
    if (tiers[i].slot_size >= size){
      return (int)i;
    }
  }
  return TOO_BIG_SIZE; 
}

static kstatus_t init_tier(uint8_t tier) {
  void* frame = alloc_page();
  if (!frame) {
    return KSTATUS_ERR_NO_MEMORY;
  }
  
  struct pool_page_header* header = (struct pool_page_header*)frame;

  header->count = 0;
  header->magic = MAGIC;
  header->tier_idx = tier;

  uint16_t slotsize = tiers[tier].slot_size;
  struct free_slot* s = (frame + tiers[tier].slot_size);
  struct free_slot* last_slot = frame + PAGE_SIZE - slotsize;

  for (; s < last_slot; s = s->next) {
    s->next = (void*)s + slotsize;
  }

  last_slot->next = NULL;
  tiers[tier].free_head = (void*)((uintptr_t)frame + slotsize);

  return KSTATUS_SUCCESS;
}

static void* kmalloc_small(uint8_t tier) {
  if (tiers[tier].free_head) {
    struct free_slot* head = tiers[tier].free_head;
    struct pool_page_header* header = (struct pool_page_header*)PAGE_ALIGN_DOWN(((uintptr_t)head));
    header->count++;

    tiers[tier].free_head = head->next;

    return (void*)head;
  }

  kstatus_t res = init_tier(tier);
  if (res != KSTATUS_SUCCESS) {
    return NULL;
  }

  return kmalloc_small(tier);
}

void* kmalloc_big(size_t size) {
  size = PAGE_ALIGN_UP(size);
  size_t npages = size / PAGE_SIZE;

  void* vaddr = alloc_pages(npages);
  if (!vaddr) {
    return NULL;
  }

  struct big_alloc_node* cur_node = kmalloc(sizeof(struct big_alloc_node));
  if (!cur_node) {
    free_pages(vaddr, npages);
  }

  cur_node->vaddr = vaddr;
  cur_node->pages_count = npages;

  if (!big_alloc_head) {
    cur_node->next = NULL;
    big_alloc_head = cur_node;
  } else {
    cur_node->next = big_alloc_head;
    big_alloc_head = cur_node;
  }

  return vaddr;
}

// better used for sizes <= 1024.
// if you use it for bigger sizes it will be slower
// better skip it directly and call alloc_pages(size_t count).
void* kmalloc(size_t size) {
  int tier = size_to_tier_idx(size);
  if (tier == TOO_BIG_SIZE) {
    return kmalloc_big(size);
  }

  void* ptr = kmalloc_small(tier);
  return ptr;
}

static void kfree_big(void* vaddr) {
  struct big_alloc_node* cur = big_alloc_head, *prev = NULL;
  while (cur && cur->vaddr != vaddr) {
    prev = cur;
    cur = cur->next;
  }

  if (!cur) {
    panic("kfree_big: trying to free a non-existing big allocation.");
  }

  if (cur == big_alloc_head) {
    big_alloc_head = big_alloc_head->next;
  } else {
    prev->next = cur->next;
  }
  
  free_pages(vaddr, cur->pages_count);
  kfree(cur);
}

// frees kmalloc pointers.
void kfree(void* vaddr) {
  struct pool_page_header* header = (struct pool_page_header*)PAGE_ALIGN_DOWN((uintptr_t)vaddr);
  if (header->magic != MAGIC) {
    kfree_big(vaddr);
    return;
  }

  if (--(header->count) == 0) {
    free_page(vaddr);
    return;
  }

  struct free_slot* slot = (struct free_slot*)vaddr;
  slot->next = tiers[header->tier_idx].free_head;
  tiers[header->tier_idx].free_head = slot;
}


/////////////////////////////////
////// page allocs/frees ////////
/////////////////////////////////

// returns vaddr of the page.
void* alloc_page() {
  return PHYS_TO_VIRT(pmm_alloc_frame());
}

// returns vaddr of the first page of the contigious pages.
void* alloc_pages(size_t count) {
  return PHYS_TO_VIRT(pmm_alloc_contiguous_frames(count));
}

void free_page(void* vaddr) {
  pmm_free_frame(VIRT_TO_PHYS(vaddr));
}

void free_pages(void* vaddr, size_t count) {
  pmm_free_contiguous_frames(VIRT_TO_PHYS(vaddr), count);
}