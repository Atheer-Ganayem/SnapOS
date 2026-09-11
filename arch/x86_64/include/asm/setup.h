#ifndef SETUP_H
#define SETUP_H

#include <stddef.h>
#include <mm.h>

#define E820_ARR_MAX_SIZE 128
#define PHYS_REGION_MAX_SIZE (E820_ARR_MAX_SIZE * 2) // because regions migh overlap an need splitting

size_t arch_get_memory_map(struct phys_region mem_regions[]);

#endif