#ifndef SPECIAL_INSNS_H
#define SPECIAL_INSNS_H

#include <stdint.h>

extern uint64_t read_cr3();
extern void write_cr3(uint64_t val);
extern bool check_1gib_pages_support(); 

static inline uint64_t switch_cr3(uint64_t new_val) {
  uint64_t cr3_old = read_cr3();
  write_cr3(new_val);
  return cr3_old;
}


extern void flush_tlb_single(void* vaddr);

extern void enable_no_execute();

#endif