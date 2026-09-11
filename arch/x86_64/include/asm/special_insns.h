#ifndef SPECIAL_INSNS_H
#define SPECIAL_INSNS_H

#include <stdint.h>

extern uint64_t read_cr3();
extern void write_cr3(uint64_t val);

#endif