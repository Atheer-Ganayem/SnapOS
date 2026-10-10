#ifndef DRIVERS_AHCI_H
#define DRIVERS_AHCI_H

#include <stdint.h>
#include <stddef.h>
#include <types.h>
#include <mm.h>
#include <block.h>

#define AHCI_CLASS 0x01
#define AHCI_SUB_CLASS 0x06
#define AHCI_ENABLE_BIT (1 << 31)
#define AHCI_DEV_SATA   0x00000101  // Standard SATA Hard Drive

#define AHCI_HBA_PORT_DET_PRESENT 3 // DET = device detection
#define AHCI_HBA_PORT_IPM_ACTIVE  1 // IPM = interface power management
#define AHCI_HBA_PORT_CMD_ST_BIT 0x01 // st = start
#define AHCI_HBA_PORT_CMD_FRE_BIT 0x10  // fre = fis receive enable
#define AHCI_HBA_PORT_CMD_FR_BIT 0x4000 // fr = fis receive running
#define AHCI_HBA_PORT_CMD_CR_BIT 0x8000 // cr = command list running
#define AHCI_HBA_PORT_IS_TFES_BIT (1 << 30) // Task File Error Status (TFES)
#define AHCI_HBA_PORT_TFD_BSY_BIT (1 << 7)
#define AHCI_HBA_PORT_TFD_DRQ_BIT (1 << 3) // Indicates a data transfer is requested

#define AHCI_COMMAND_LIST_MAX_COUNT 32

#define AHCI_PHYSICAL_REGION_MAX_LENGTH 0x400000

#define AHCI_ATA_CMD_READ_DMA_EXT 0x25
#define AHCI_ATA_CMD_IDENTIFY 0xEC

#define AHCI_CMDFIS_DEVICE_LBA_MODE (1 << 6)
#define AHCI_CMDFIS_DEVICE_IDENTIFY 0xEC

#define AHCI_DEFAULT_SECTOR_SIZE 512

// FIS = Frame Inforamtion Structure

typedef enum {
	FIS_TYPE_REG_H2D	= 0x27,	// Register FIS - host to device
	FIS_TYPE_REG_D2H	= 0x34,	// Register FIS - device to host
	FIS_TYPE_DMA_ACT	= 0x39,	// DMA activate FIS - device to host
	FIS_TYPE_DMA_SETUP	= 0x41,	// DMA setup FIS - bidirectional
	FIS_TYPE_DATA		= 0x46,	// Data FIS - bidirectional
	FIS_TYPE_BIST		= 0x58,	// BIST activate FIS - bidirectional
	FIS_TYPE_PIO_SETUP	= 0x5F,	// PIO setup FIS - device to host
	FIS_TYPE_DEV_BITS	= 0xA1,	// Set device bits FIS - device to host
} FIS_TYPE;

struct fis_h2d {
  uint8_t fis_type; // always FIS_TYPE_REG_H2D
  uint8_t pmport:4; // port multiplier
  uint8_t rsv0:3;
  uint8_t c:1;      // 1: command, 0: control
  uint8_t command;
  uint8_t featurel; // lower byte of feature
  uint8_t lba0;
  uint8_t lba1;
  uint8_t lba2;
  uint8_t device;
  uint8_t lba3;
  uint8_t lba4;
  uint8_t lba5;
  uint8_t featureh; // higher byte of feature
  uint8_t countl; // lower byte of count
  uint8_t counth; // higher byte of count
  uint8_t iccl; // Isochronous command completion
  uint8_t control;
  uint8_t rsv1[4];
} __attribute__((packed));

struct fis_d2h {
  uint8_t fis_type; // always FIS_TYPE_REG_D2H
  uint8_t pmport:4; // port multiplier
  uint8_t rsv0:2;
  uint8_t i:1;      // interrupt bit
  uint8_t rsv1:1;
  uint8_t status;
  uint8_t error; // lower byte of feature
  uint8_t lba0;
  uint8_t lba1;
  uint8_t lba2;
  uint8_t device;
  uint8_t lba3;
  uint8_t lba4;
  uint8_t lba5;
  uint8_t rsv2; // higher byte of feature
  uint8_t countl; // lower byte of count
  uint8_t counth; // higher byte of count
  uint8_t rsv3[2];
  uint8_t rsv4[4];
} __attribute__((packed));

struct fis_pio_setup {
	uint8_t  fis_type;	// FIS_TYPE_PIO_SETUP

	uint8_t  pmport:4;
	uint8_t  rsv0:1;
	uint8_t  d:1;		// Data transfer direction, 1 - device to host
	uint8_t  i:1;		// Interrupt bit
	uint8_t  rsv1:1;

	uint8_t  status;
	uint8_t  error;

	uint8_t  lba0;		// LBA 7:0
	uint8_t  lba1;		// LBA 15:8
	uint8_t  lba2;		// LBA 23:16
	uint8_t  device;

	uint8_t  lba3;		// LBA 31:24
	uint8_t  lba4;		// LBA 39:32
	uint8_t  lba5;		// LBA 47:40
	uint8_t  rsv2;

	uint8_t  countl;		// Count 7:0
	uint8_t  counth;		// Count 15:8
	uint8_t  rsv3;
	uint8_t  e_status;	// New value of status register

	uint16_t tc;		// Transfer count
	uint8_t  rsv4[2];
} __attribute__((packed));

struct fis_data {
  uint8_t fis_type; // always FIS_TYPE_DATA
  uint8_t pmport:4;
  uint8_t rsv0:4;
  uint8_t rsv1[2];
  uint32_t data;
} __attribute__((packed));

struct fis_dma_setup {
  uint8_t fis_type; // FIS_TYPE_DMA_SETUP
  uint8_t pmport:4;
  uint8_t rsv0:1;
  uint8_t d:1; // direction, 1: D2H
  uint8_t i:1; // interrupt bit
  uint8_t a:1; // auto-active. specifies if dma activate-fis is needed.
  uint8_t rsv1[2];
  uint64_t dma_buf_id;
  uint32_t rsv2;
  uint32_t dma_buf_offset; // first 2 bits must be zero
  uint32_t transfet_count;
  uint32_t rsv3;
}__attribute__((packed));

struct hba_port {
  uint32_t clb;       // 0x00, Command list base address, 1K-byte aligned
  uint32_t clbu;      // 0x04, Command list base address upper 32 bits
  uint32_t fb;        // 0x08, FIS base address, 256-byte aligned
  uint32_t fbu;       // 0x0C, FIS base address upper 32 bits
  uint32_t is;        // 0x10, Interrupt status
  uint32_t ie;        // 0x14, Interrupt enable
  uint32_t cmd;       // 0x18, Command and status
  uint32_t rsv0;      // 0x1C, Reserved
  uint32_t tfd;       // 0x20, Task file data
  uint32_t sig;       // 0x24, Signature (Hard Drive or CD-ROM)
  uint32_t ssts;      // 0x28, SATA status
  uint32_t sctl;      // 0x2C, SATA control
  uint32_t serr;      // 0x30, SATA error
  uint32_t sact;      // 0x34, SATA active
  uint32_t ci;        // 0x38, Command issue 
  uint32_t sntf;      // 0x3C, SATA notification
  uint32_t fbs;       // 0x40, FIS-based switch control
  uint32_t rsv1[11];  // 0x44 ~ 0x6F, Reserved
  uint32_t vendor[4]; // 0x70 ~ 0x7F, Vendor specific 
} __attribute__((packed));

struct hba_mem {
  uint32_t cap;		// 0x00, Host capability
	uint32_t ghc;		// 0x04, Global host control
	uint32_t is;		// 0x08, Interrupt status
	uint32_t pi;		// 0x0C, Port implemented
	uint32_t vs;		// 0x10, Version
	uint32_t ccc_ctl;	// 0x14, Command completion coalescing control
	uint32_t ccc_pts;	// 0x18, Command completion coalescing ports
	uint32_t em_loc;		// 0x1C, Enclosure management location
	uint32_t em_ctl;		// 0x20, Enclosure management control
	uint32_t cap2;		// 0x24, Host capabilities extended
	uint32_t bohc;		// 0x28, BIOS/OS handoff control and status

	// 0x2C - 0x9F, Reserved
	uint8_t  rsv[0xA0-0x2C];

	// 0xA0 - 0xFF, Vendor specific registers
	uint8_t  vendor[0x100-0xA0];

	// 0x100 - 0x10FF, Port control registers
	struct hba_port	ports[32];	// 1 ~ 32
} __attribute__((packed));



struct hba_fis {
  struct fis_dma_setup dsfis;
  uint8_t pad0[4];

  struct fis_pio_setup psfis;
  uint8_t pad1[12];

  uint8_t sdbfis[8]; // set devise bit fis

  uint8_t ufis[64];

  uint8_t rsv[0x100 - 0xA0];
} __attribute__((packed));

struct hba_cmd_header {
  uint8_t cfl:5; // command fis length in dwords
  uint8_t a:1; // ATAPI
  uint8_t w:1; // write, 0: D2H, 1:H2D
  uint8_t p:1; // prefetchable

  uint8_t r:1; // reset
  uint8_t b:1; // BIST
  uint8_t c:1; // clear busy upon R_OK
  uint8_t rsv0:1;
  uint8_t pmp:4; // port multiplier port

  uint16_t prdtl; // physical region descriptor length in entries

  volatile uint32_t prdbc; // physical region descriptor byte count transfared

  uint32_t ctba; // command table descriptor base address
  uint32_t ctbau; // command table descriptor base address uppper 32 bits

  uint32_t rsv1[4];
} __attribute__((packed));


struct hba_prdt_entry {
  uint32_t dba; // data base address
  uint32_t dbau; // data base address upper 32 bits
  uint32_t rsv0;
  
  uint32_t dbc:22; // byte count, 4MiB max.
  uint32_t rsv1:9;
  uint32_t i:1; // interrupt on completion
} __attribute__((packed));

struct hba_cmd_table {
  uint8_t cfis[64]; // command fis
  uint8_t acmd[16]; // ATAPI command
  uint8_t rsv[48];

  struct hba_prdt_entry prdt_entry[];
} __attribute__((packed));

int ahci_init();

int ahci_read(struct block_dev* bdev, uint64_t lba, uint64_t count, struct phys_iovec* iovec, uint16_t iovec_count);

#endif