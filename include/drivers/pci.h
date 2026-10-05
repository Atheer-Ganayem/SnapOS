#ifndef DRIVERS_PCI_H
#define DRIVERS_PCI_H

#include <stdint.h>
#include <stdbool.h>

#define PCI_MAX_BUS 255
#define PCI_MAX_DEV 31
#define PCI_MAX_FUNC 7

struct pci_device {
  uint8_t bus;
  uint8_t dev;
  uint8_t func;
  bool found;
};

struct pci_device pci_get_by_class(uint8_t target_class, uint8_t target_subclass);
uint32_t pci_read_dword(struct pci_device* pci_dev, uint8_t offset);
void pci_write_dword(struct pci_device* pci_dev, uint8_t offset, uint32_t val);

#endif