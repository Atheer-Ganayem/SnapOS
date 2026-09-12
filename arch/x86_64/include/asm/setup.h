#ifndef SETUP_H
#define SETUP_H

#include <stddef.h>
#include <mm.h>

#define E820_ARR_MAX_SIZE 128
#define PHYS_REGION_MAX_SIZE E820_ARR_MAX_SIZE
#define _MIN_USABLE_PADDR 0x100000
#define MAX_MAPPED_PADDR (1024 * 1024 * 1024) // The bootloader idenity maps only the first gig.

size_t arch_get_memory_map(struct phys_region mem_regions[]);

#endif