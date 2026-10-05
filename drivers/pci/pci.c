#include <drivers/pci.h>
#include <asm/pci.h>

struct pci_device pci_get_by_class(uint8_t target_class, uint8_t target_subclass) {
  struct pci_device res;
  res.found = false;

  for (uint16_t bus = 0; bus <= PCI_MAX_BUS; bus++) {
    for (uint16_t dev = 0; dev <= PCI_MAX_DEV; dev++) {
      for (uint16_t func = 0; func <= PCI_MAX_FUNC; func++) {
        uint32_t vendor = arch_pci_read_dword(bus, dev, func, 0);
        if ((vendor & 0xFFFF) != 0xFFFF) {
          uint32_t class_info = arch_pci_read_dword(bus, dev, func, 0x08);
          uint8_t class = (uint8_t)(class_info >> 24);
          uint8_t subclass = (uint8_t)(class_info >> 16);

          if (class == target_class && subclass == target_subclass) {
            res.bus = (uint8_t)bus;
            res.dev = (uint8_t)dev;
            res.func = (uint8_t)func;
            res.found = true;
            
            return res;
          }
        }
      }
    }
  }

  return res;
}

uint32_t pci_read_dword(struct pci_device* pci_dev, uint8_t offset) {
  return arch_pci_read_dword(pci_dev->bus, pci_dev->dev, pci_dev->func, offset);
}

void pci_write_dword(struct pci_device* pci_dev, uint8_t offset, uint32_t val) {
  return arch_pci_write_dword(pci_dev->bus, pci_dev->dev, pci_dev->func, offset, val);
}
