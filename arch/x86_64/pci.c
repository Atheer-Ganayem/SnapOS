#include <asm/pci.h>
#include <asm/special_insns.h>

uint32_t arch_pci_read_dword(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset) {
  uint32_t addr = (uint32_t)((bus << 16) | (dev << 11) | (func << 8) | (offset & 0xFC) | (uint32_t)0x80000000);
  outl(CONFIG_ADDR_PORT, addr);
  return inl(CONFIG_DATA_PORT);
}

void arch_pci_write_dword(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint32_t val) {
  uint32_t addr = (uint32_t)((bus << 16) | (dev << 11) | (func << 8) | (offset & 0xFC) | (uint32_t)0x80000000);
  outl(CONFIG_ADDR_PORT, addr);
  outl(CONFIG_DATA_PORT, val);
}