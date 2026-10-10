#include <drivers/ahci.h>
#include <asm/pci.h>
#include <drivers/pci.h>
#include <string.h>
#include <paging.h>
#include <asm/io.h>

volatile struct hba_mem* hba = NULL;

int ahci_identify(volatile struct hba_port* port, void* buf);

static inline volatile struct hba_cmd_header* ahci_get_cmd_list(volatile struct hba_port* port) {
  uintptr_t clbl = (uintptr_t)readl_relaxed(&port->clb);
  uintptr_t clbu = (uintptr_t)readl_relaxed(&port->clbu);
  uint64_t cmd_list_phys = clbl | (clbu << 32);
  if (!cmd_list_phys) {
    return NULL;
  }

  return (volatile struct hba_cmd_header*)PHYS_TO_VIRT(cmd_list_phys);
}

static inline volatile struct hba_cmd_table* ahci_get_cmdtbl(volatile struct hba_cmd_header* header) {
  uintptr_t ctbal = (uintptr_t)readl_relaxed(&header->ctba);
  uintptr_t ctbau = (uintptr_t)readl_relaxed(&header->ctbau);
  uint64_t cmdtbl_phys = ctbal | (ctbau << 32);
  if (!cmdtbl_phys) {
    return NULL;
  }

  return (volatile struct hba_cmd_table*)PHYS_TO_VIRT(cmdtbl_phys);
}

static inline void ahci_set_cmdtbl(volatile struct hba_cmd_header* header, void* phys) {
  writel_relaxed((uint32_t)((uint64_t)phys), &header->ctba);
  writel_relaxed((uint32_t)(((uint64_t)phys) >> 32), &header->ctbau);
}

static void stop_port(volatile struct hba_port* port) {
  // disable start bit and FIS receive so the exection engine stops executing commands
  // and stops sending to the FIS receive.
  uint32_t cmd = readl_relaxed(&port->cmd);
  cmd &= ~AHCI_HBA_PORT_CMD_ST_BIT;
  cmd &= ~AHCI_HBA_PORT_CMD_FRE_BIT;
  writel(cmd, &port->cmd);

  // wait if there's something is already being written/executing
  while (readl_relaxed(&port->cmd) & AHCI_HBA_PORT_CMD_FR_BIT);
  while (readl_relaxed(&port->cmd) & AHCI_HBA_PORT_CMD_CR_BIT);
}

static void start_port(volatile struct hba_port* port) {
  while (readl_relaxed(&port->cmd) & AHCI_HBA_PORT_CMD_CR_BIT);
  
  uint32_t cmd = readl_relaxed(&port->cmd);
  cmd |= AHCI_HBA_PORT_CMD_FRE_BIT | AHCI_HBA_PORT_CMD_ST_BIT;
  writel_relaxed(cmd, &port->cmd);
}

static int ahci_init_port(volatile struct hba_port* port) {
  writel(0, &port->ie);
  writel(0xFFFFFFFF, &port->serr);
  writel(0xFFFFFFFF, &port->is);

  stop_port(port);

  void* cmd_list = kzalloc(sizeof(struct hba_cmd_header) * AHCI_COMMAND_LIST_MAX_COUNT);
  if (!cmd_list) {
    return -ENOMEM;
  }

  void* fis = kzalloc(sizeof(struct hba_fis));
  if (!fis) {
    return -ENOMEM;
  }


  wmb();

  void* cmd_list_phys = VIRT_TO_PHYS(cmd_list);
  void* fis_phys = VIRT_TO_PHYS(fis);

  uint32_t clb = (uint32_t)((uintptr_t)cmd_list_phys);
  uint32_t clbu = (uint32_t)(((uint64_t)cmd_list_phys) >> 32);
  uint32_t fb = (uint32_t)((uintptr_t)fis_phys);
  uint32_t fbu = (uint32_t)(((uint64_t)fis_phys) >> 32);

  writel_relaxed(clb, &port->clb);
  writel_relaxed(clbu, &port->clbu);
  writel_relaxed(fb, &port->fb);
  writel_relaxed(fbu, &port->fbu);

  start_port(port);

  return 0;
}

static uint64_t __id_get_sector_count_(uint16_t* id) {
  // bit 15 must be 1, bit 14 must be 1.
  // bit 10 is set if "48-bit Address feature set supported 
  // if not (bit 10 set), sector count is in words 60-61.
  // if set, sector count in words 100-103.
  uint64_t sector_count = (uint64_t)id[60] | ((uint64_t)id[61] << 16);
  if (((id[83] & 0xC000) == 0x4000) && (id[83] & (1 << 10))) {
    sector_count =  (uint64_t)id[100] |
                    ((uint64_t)id[101] << 16) |
                    ((uint64_t)id[102] << 32) |
                    ((uint64_t)id[103] << 48);
  }

  return sector_count;
}

static uint64_t __id_get_sector_size(uint16_t* id) {
  // bit 15 must be 1, bit 14 must be 1.
  // bit 12 set if "Device Logical Sector Longer than 256 Words".
  uint32_t sector_size = AHCI_DEFAULT_SECTOR_SIZE;
  if ((id[106] & 0xC000) == 0x4000) {
    if (id[106] & (1 << 12)) {
      uint32_t words_per_sector = id[117] | ((uint32_t)id[118] << 16);
      if (words_per_sector > 0) { // just in case we get trash value.
        sector_size =  words_per_sector * 2;
      } 
    }
  }
  return sector_size;
}

static int ahci_probe_ports(volatile struct hba_mem* hba) {
  for (int i = 0; i < 32; i++) {
    if (hba->pi & (1 << i)) {
      volatile struct hba_port* port = &hba->ports[i];

      uint32_t ssts = readl_relaxed(&port->ssts);
      uint8_t det = ssts & 0x0F; // device detection
      uint8_t ipm = (ssts >> 8) & 0x0F; // interface power management

      if (det != AHCI_HBA_PORT_DET_PRESENT || ipm != AHCI_HBA_PORT_IPM_ACTIVE)
        continue;
      if (readl_relaxed(&port->sig) != AHCI_DEV_SATA)
        continue;

      int status = ahci_init_port(port);
      if (status < 0) {
        return status;
      }

      uint16_t* id = kmalloc(512);
      if (!id) {
        return -ENOMEM;
      }

      status = ahci_identify(port, id);
      if (status < 0) {
        return -EIO;
      } 

      struct block_dev* bdev = kzalloc(sizeof(struct block_dev));
      if (!bdev) {
        return -ENOMEM;
      }

      char name[] = "satax";
      name[4] = '0' + i;
      strcpy(bdev->name, name);
      bdev->priv_data = (void*)&hba->ports[i];
      bdev->read = ahci_read;
      bdev->sector_size = __id_get_sector_size(id); 
      bdev->sector_count = __id_get_sector_count_(id);

      block_dev_register(bdev);
    }
  }

  return 0;
}

int ahci_init() {
  struct pci_device pci_dev = pci_get_by_class(AHCI_CLASS, AHCI_SUB_CLASS);
  if (!pci_dev.found) {
    return -ENODEV;
  }

  uint32_t cmd_reg = pci_read_dword(&pci_dev, 0x04);
  cmd_reg |= PIC_COMMAND_BUS_MASTER_BIT | PCI_COMMAND_MEMORY_SPACE_BIT;
  pci_write_dword(&pci_dev, 0x04, cmd_reg);

  uint32_t bar5 =  pci_read_dword(&pci_dev, 0x24);
  uint32_t bar5_phys = bar5 & 0xFFFFFFF0;

  hba = ioremap(bar5_phys, sizeof(struct hba_mem));

  uint32_t ghc = readl_relaxed(&hba->ghc);
  ghc |= AHCI_ENABLE_BIT;
  writel_relaxed(ghc, &hba->ghc);

  int status = ahci_probe_ports(hba);
  if (status < 0) {
    return status;
  }

  return 0;
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
        return -ENOMEM;
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

static void ahci_cmdtbl_build_fis(volatile struct hba_cmd_table* cmdtbl, uint64_t lba, uint16_t count, uint8_t cmd, uint8_t device) {
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

  cfis->device = device;

  cfis->countl = (uint8_t)(count);
  cfis->counth = (uint8_t)(count >> 8);
}

static bool ahci_issue_cmd(volatile struct hba_port* port, uint8_t slot) {
  uint32_t spin = 1000000;
  while (spin > 0 && 
    (readl_relaxed(&port->tfd) & AHCI_HBA_PORT_TFD_BSY_BIT || 
      readl_relaxed(&port->tfd) & AHCI_HBA_PORT_TFD_DRQ_BIT)) spin--;

  if (spin == 0)
    return false;


  writel(0xFFFFFFFF, &port->is);
  writel(1U << slot, &port->ci);

  while (1) {
    if ((readl_relaxed(&port->ci) & (1U << slot)) == 0)
      break;
    if (readl_relaxed(&port->is) & AHCI_HBA_PORT_IS_TFES_BIT)
      return false;
  }

  if (readl_relaxed(&port->is) & AHCI_HBA_PORT_IS_TFES_BIT)
    return false;

  return true;
}

static struct hba_cmd_table* ahci_alloc_cmdtbl() {
  /////////////////////////////////////////////////
  ///// TODO: for those, we need a function that allocs a page, flushed cache, ioremap the page.
  ///////////////////////////////////////////
  struct hba_cmd_table* cmdtbl = alloc_page();
  if (!cmdtbl) {
    return NULL;
  }


  return memset(cmdtbl, 0x00, PAGE_SIZE);
}

int __ahci_read(volatile struct hba_port* port, uint64_t lba, uint64_t count, struct phys_iovec* iovec, uint16_t iovec_count) {
  volatile struct hba_cmd_header* cmd_list = ahci_get_cmd_list(port);
  volatile struct hba_cmd_table* cmdtbl = ahci_get_cmdtbl(&cmd_list[0]);

  if (!cmdtbl) {
    cmdtbl = ahci_alloc_cmdtbl();
    if (!cmdtbl) {
      return -ENOMEM;
    }
    ahci_set_cmdtbl(cmd_list, VIRT_TO_PHYS((uintptr_t)cmdtbl));
  }

  ahci_cmdtbl_build_fis(cmdtbl, lba, count, AHCI_ATA_CMD_READ_DMA_EXT, AHCI_CMDFIS_DEVICE_LBA_MODE);

  uint16_t max_ptrd_entries = (PAGE_SIZE - offsetof(struct hba_cmd_table, prdt_entry))/sizeof(struct hba_prdt_entry);
  int ptrdl = ahci_build_prdt(cmdtbl, max_ptrd_entries, iovec, iovec_count);
  if (ptrdl < 0) {
    return ptrdl;
  }

  cmd_list[0].prdtl = ptrdl;
  cmd_list[0].w = 0;
  cmd_list[0].cfl = sizeof(struct fis_h2d) / 4; // length in dword

  wmb();  
  
  ////////////////////////////////////
  //// TODO: invalidate cache lines. (BEFORE issuing)
  ////////////////////////////////////

  if (!ahci_issue_cmd(port, 0)) 
    return -EIO;
  
  
  return 0;
}

int ahci_read(struct block_dev* bdev, uint64_t lba, uint64_t count, struct phys_iovec* iovec, uint16_t iovec_count) {
  volatile struct hba_port* port = (volatile struct hba_port*)bdev->priv_data;
  return __ahci_read(port, lba, count, iovec, iovec_count);
}

int ahci_identify(volatile struct hba_port* port, void* buf) {
  volatile struct hba_cmd_header* cmd_list = ahci_get_cmd_list(port);
  volatile struct hba_cmd_table* cmdtbl = ahci_get_cmdtbl(&cmd_list[0]);

  if (!cmdtbl) {
    cmdtbl = ahci_alloc_cmdtbl();
    if (!cmdtbl) {
      return -ENOMEM;
    }
    ahci_set_cmdtbl(cmd_list, VIRT_TO_PHYS((uintptr_t)cmdtbl));
  }

  memset((void*)cmdtbl->cfis, 0x00, sizeof(cmdtbl->cfis));
  ahci_cmdtbl_build_fis(cmdtbl, 0, 0, AHCI_ATA_CMD_IDENTIFY, 0x00);

  struct phys_iovec iovec[] = {{.addr = (uint64_t)VIRT_TO_PHYS(buf), .length = 512}};
  int ptrdl = ahci_build_prdt(cmdtbl, 1, iovec, 1);
  if (ptrdl < 0) {
    return -ptrdl;
  }

  cmd_list[0].prdtl = ptrdl;
  cmd_list[0].w = 0;
  cmd_list[0].cfl = sizeof(struct fis_h2d) / 4; // length in dword
  
  wmb();  
  
  ////////////////////////////////////
  //// TODO: invalidate cache lines. (BEFORE issuing)
  ////////////////////////////////////

  if (!ahci_issue_cmd(port, 0)) 
    return -EIO;
  

  return 0;
}