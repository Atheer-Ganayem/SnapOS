#include <drivers/ahci.h>
#include <asm/pci.h>
#include <mm.h>
#include <drivers/vga.h>
#include <drivers/pci.h>

volatile struct hba_mem* hba = NULL;



kstatus_t achi_init() {
  vga_print_color("ahci init...\n", VGA_COLOR_GREEN);
  struct pci_device pci_dev = pci_get_by_class(AHCI_CLASS, AHCI_SUB_CLASS);
  if (!pci_dev.found) {
    return KSTATUS_ERR_NOT_FOUND;
  }

  vga_print_color("found bus and dev and func\n", VGA_COLOR_GREEN);

  uint32_t cmd_reg = pci_read_dword(&pci_dev, 0x04);
  cmd_reg |= PIC_COMMAND_BUS_MASTER_BIT | PCI_COMMAND_MEMORY_SPACE_BIT;

  pci_write_dword(&pci_dev, 0x04, cmd_reg);

  uint32_t bar5 =  pci_read_dword(&pci_dev, 0x24);
  uint32_t bar5_phys = bar5 & 0xFFFFFFF0;

  hba = ioremap(bar5_phys, sizeof(struct hba_mem));

  hba->ghc |= AHCI_ENABLE_BIT;

  for (int i = 0; i < 32; i++) {
    if (hba->pi & (1 << i)) {
      volatile struct hba_port* port = &hba->ports[i];

      if ((port->ssts & 0x0F) != AHCI_HBA_PORT_DET_PRESENT || ((port->ssts >> 8) & 0xF) != AHCI_HBA_PORT_IPM_ACTIVE) {
        continue;
      }

      if (port->sig == AHCI_DEV_SATA) {
        vga_print_color("AHCI: found port\n", VGA_COLOR_GREEN);
      }
    }
  }

  return KSTATUS_SUCCESS;
}