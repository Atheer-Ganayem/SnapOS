#ifndef ARCH_PCI_H
#define ARCH_PCI_H

#include <stdint.h>

#define CONFIG_ADDR_PORT 0xCF8
#define CONFIG_DATA_PORT 0xCFC

#define PCI_COMMAND_MEMORY_SPACE_BIT  1 << 1
#define PIC_COMMAND_BUS_MASTER_BIT    1 << 2

uint32_t arch_pci_read_dword(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset);
void arch_pci_write_dword(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint32_t val);

#endif