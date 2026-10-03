#ifndef ARCH_PIC_H
#define ARCH_PIC_H

#include <stdint.h>

void picEIO(uint8_t irq);
void remap_pic(uint8_t offset1, uint8_t offset2);
void disable_pic();
void enable_pic();

#endif