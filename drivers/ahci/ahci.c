#include <drivers/ahci.h>
#include <asm/pci.h>
#include <drivers/vga.h>
#include <drivers/pci.h>
#include <string.h>
#include <paging.h>

volatile struct hba_mem* hba = NULL;

static inline volatile struct hba_cmd_header* ahci_get_cmd_list(volatile struct hba_port* port) {
  uint64_t cmd_list_phys = ((uintptr_t)port->clb) | (((uintptr_t)port->clbu) << 32);
  if (!cmd_list_phys) {
    return NULL;
  }

  return (volatile struct hba_cmd_header*)PHYS_TO_VIRT(cmd_list_phys);
}

static inline volatile struct hba_cmd_table* ahci_get_cmdtbl(volatile struct hba_cmd_header* header) {
  uint64_t cmdtbl_phys = ((uintptr_t)header->ctba) | (((uintptr_t)header->ctbau) << 32);
  if (!cmdtbl_phys) {
    return NULL;
  }

  return (volatile struct hba_cmd_table*)PHYS_TO_VIRT(cmdtbl_phys);
}

static inline void ahci_set_cmdtbl(volatile struct hba_cmd_header* header, void* phys) {
  header->ctba = (uint32_t)((uint64_t)phys);
  header->ctbau = (uint32_t)(((uint64_t)phys) >> 32);
}

static void stop_port(volatile struct hba_port* port) {
  // disable start bit and FIS receive so the exection engine stops executing commands
  // and stops sending to the FIS receive.
  port->cmd &= ~AHCI_HBA_PORT_CMD_ST_BIT;
  port->cmd &= ~AHCI_HBA_PORT_CMD_FRE_BIT;

  // wait if there's something is already being written/executing
  while (port->cmd & AHCI_HBA_PORT_CMD_FR_BIT);
  while (port->cmd & AHCI_HBA_PORT_CMD_CR_BIT);
}

static void start_port(volatile struct hba_port* port) {
  while (port->cmd & AHCI_HBA_PORT_CMD_CR_BIT);
  port->cmd |= AHCI_HBA_PORT_CMD_FRE_BIT;
  port->cmd |= AHCI_HBA_PORT_CMD_ST_BIT;
}

static kstatus_t ahci_init_port(volatile struct hba_port* port) {
  stop_port(port);

  void* cmd_list = kzalloc(sizeof(struct hba_cmd_header) * AHCI_COMMAND_LIST_MAX_COUNT);
  if (!cmd_list) {
    return KSTATUS_ERR_NO_MEMORY;
  }

  void* fis = kzalloc(sizeof(struct hba_fis));
  if (!fis) {
    return KSTATUS_ERR_NO_MEMORY;
  }

  void* cmd_list_phys = VIRT_TO_PHYS(cmd_list);
  void* fis_phys = VIRT_TO_PHYS(fis);

  port->clb = (uint32_t)((uintptr_t)cmd_list_phys);
  port->clbu = (uint32_t)(((uint64_t)cmd_list_phys) >> 32);
  port->fb = (uint32_t)((uintptr_t)fis_phys);
  port->fbu = (uint32_t)(((uint64_t)fis_phys) >> 32);

  start_port(port);

  return KSTATUS_SUCCESS;
}

static kstatus_t ahci_probe_ports(volatile struct hba_mem* hba) {
  for (int i = 0; i < 32; i++) {
    if (hba->pi & (1 << i)) {
      volatile struct hba_port* port = &hba->ports[i];

      uint8_t det = port->ssts & 0x0F; // device detection
      uint8_t ipm = (port->ssts >> 8) & 0x0F; // interface power management

      if (det != AHCI_HBA_PORT_DET_PRESENT || ipm != AHCI_HBA_PORT_IPM_ACTIVE)
        continue;
      if (port->sig != AHCI_DEV_SATA)
        continue;

      vga_print_color("AHCI: found port\n", VGA_COLOR_GREEN);

      kstatus_t status = ahci_init_port(port);
      if (status != KSTATUS_SUCCESS) {
        return status;
      }
      vga_print_color("AHCI: init port success\n", VGA_COLOR_GREEN);
    }
  }

  return KSTATUS_SUCCESS;
}

kstatus_t ahci_init() {
  struct pci_device pci_dev = pci_get_by_class(AHCI_CLASS, AHCI_SUB_CLASS);
  if (!pci_dev.found) {
    return KSTATUS_ERR_NOT_FOUND;
  }

  uint32_t cmd_reg = pci_read_dword(&pci_dev, 0x04);
  cmd_reg |= PIC_COMMAND_BUS_MASTER_BIT | PCI_COMMAND_MEMORY_SPACE_BIT;
  pci_write_dword(&pci_dev, 0x04, cmd_reg);

  uint32_t bar5 =  pci_read_dword(&pci_dev, 0x24);
  uint32_t bar5_phys = bar5 & 0xFFFFFFF0;

  hba = ioremap(bar5_phys, sizeof(struct hba_mem));

  hba->ghc |= AHCI_ENABLE_BIT;

  kstatus_t status = ahci_probe_ports(hba);
  if (status != KSTATUS_SUCCESS) {
    return status;
  }

  return KSTATUS_SUCCESS;
}

static int ahci_build_prdt(
  volatile struct hba_cmd_table* cmdtbl,
  uint16_t max_entries,
  struct phys_iovec* iovec,
  uint16_t iovec_count) {

  uint16_t index = 0;

  for (uint32_t i = 0; i < iovec_count; i++) {
    uint64_t addr = iovec[i].addr;
    uint64_t bytes_left = iovec[i].length;

    while (bytes_left) {
      if (index >= max_entries) {
        return KSTATUS_ERR_NO_MEMORY;
      }

      uint32_t chunck_size = bytes_left > AHCI_PHYSICAL_REGION_MAX_LENGTH ? AHCI_PHYSICAL_REGION_MAX_LENGTH : bytes_left;
      
      cmdtbl->prdt_entry[index].dba = (uint32_t)addr;
      cmdtbl->prdt_entry[index].dbau = (uint32_t)(addr >> 32);
      cmdtbl->prdt_entry[index].dbc = chunck_size-1;

      addr += chunck_size;
      bytes_left -= chunck_size;
      index++;
    }
  }

  return index;
}

static void ahci_cmdtbl_build_fis(volatile struct hba_cmd_table* cmdtbl, uint64_t lba, uint16_t count, uint8_t cmd) {
  volatile struct fis_h2d* cfis = (volatile struct fis_h2d*)(&cmdtbl->cfis);
  cfis->fis_type = FIS_TYPE_REG_H2D;
  cfis->c = 1;
  cfis->command = cmd;

  cfis->lba0 = (uint8_t)lba;
  cfis->lba1 = (uint8_t)(lba >> 8);
  cfis->lba2 = (uint8_t)(lba >> 16);
  cfis->lba3 = (uint8_t)(lba >> 24);
  cfis->lba4 = (uint8_t)(lba >> 32);
  cfis->lba5 = (uint8_t)(lba >> 40);

  cfis->device = 1 << 6;

  cfis->countl = (uint8_t)(count);
  cfis->counth = (uint8_t)(count >> 8);
}

static bool ahci_issue_cmd(volatile struct hba_port* port, uint8_t slot) {
  uint32_t spin = 1000000;
  while (spin-- > 0 && (port->tfd & AHCI_HBA_PORT_TFD_BSY_BIT || port->tfd & AHCI_HBA_PORT_TFD_DRQ_BIT));

  if (spin == 0) {
    return false;
  }

  port->ci |= 1 << slot;

  while (1) {
    if ((port->ci & (1 << slot)) == 0)
      break;
    if (port->is & AHCI_HBA_PORT_IS_TFES_BIT)
      return false;
  }

  if (port->is & AHCI_HBA_PORT_IS_TFES_BIT)
    return false;

  return true;
}

static struct hba_cmd_table* ahci_alloc_cmdtbl() {
  struct hba_cmd_table* cmdtbl = alloc_page();
  if (!cmdtbl) {
    return NULL;
  }

  return memset(cmdtbl, 0x00, PAGE_SIZE);
}

kstatus_t ahci_read(volatile struct hba_port* port, uint64_t lba, uint64_t count, struct phys_iovec* iovec, uint16_t iovec_count) {
  volatile struct hba_cmd_header* cmd_list = ahci_get_cmd_list(port);
  volatile struct hba_cmd_table* cmdtbl = ahci_get_cmdtbl(&cmd_list[0]);

  if (!cmdtbl) {
    cmdtbl = ahci_alloc_cmdtbl();
    if (!cmdtbl) {
      return KSTATUS_ERR_NO_MEMORY;
    }
    ahci_set_cmdtbl(cmd_list, VIRT_TO_PHYS((uintptr_t)cmdtbl));
  }

  ahci_cmdtbl_build_fis(cmdtbl, lba, count, AHCI_ATA_CMD_READ_DMA_EXT);


  uint16_t max_ptrd_entries = (PAGE_SIZE - offsetof(struct hba_cmd_table, prdt_entry))/sizeof(struct hba_prdt_entry);
  int ptrdl = ahci_build_prdt(cmdtbl, max_ptrd_entries, iovec, iovec_count);
  if (ptrdl < 0) {
    return KSTATUS_GENERAL_ERR;
  }

  cmd_list[0].prdtl = ptrdl;
  cmd_list[0].w = 0;
  cmd_list[0].cfl = sizeof(struct fis_h2d) / 4; // length in dword
  
  
  
  if (!ahci_issue_cmd(port, 0)) 
    return KSTATUS_GENERAL_ERR;
  
  return KSTATUS_SUCCESS;
}

volatile struct hba_port* __ahci_get_port(int i) {
  return &hba->ports[i];
}