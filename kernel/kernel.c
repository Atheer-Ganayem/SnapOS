#include <drivers/vga.h>
#include <mm.h>
#include <string.h>
#include <drivers/ahci.h>
#include <types.h>

void panic(char* s) {
  if (s) {
    vga_print_color(s, VGA_COLOR_RED);
  }

  while (1) {
    __asm__ volatile("cli; hlt");
  };
}

void kmain() {
  int res = early_pmm_init(); 
  if (res != KSTATUS_SUCCESS) {
    panic(NULL);
  }

  res = vmm_init();
  if (res != KSTATUS_SUCCESS) {
    panic(NULL);
  }

  vga_init();
  vga_print("early pmm initalized.\n");
  vga_print("vmm initalized.\n");
  vga_print("VGA text mode initialized.\n");

  res = pmm_init();
  if (res != KSTATUS_SUCCESS) {
    panic("pmm_init() failed.");
  }
  vga_print("pmm initialized.\n");

  setup_stage2();

  kstatus_t status = ahci_init();
  if (status != KSTATUS_SUCCESS) {
    panic("failed to init ACHI.\n");
  }

  vga_print("AHCI initialized\n");

  // volatile struct hba_port* port = __ahci_get_port(1);
  // if (!port) {
  //   panic("NO PORT\n");
  // }

  // char* buf = (char*)alloc_page();
  // memset(buf, 0x00, 4096);

  // struct phys_iovec vec[] = {{.addr = (uint64_t)VIRT_TO_PHYS(buf), .length = 512}};
  // extern struct block_dev* registered_block_devices[];
  // struct block_dev* bdev = registered_block_devices[1];
  // status = bdev->read(bdev, 0, 1, vec, 1);
  // status = __ahci_read(port, 0, 1, vec, 1);
  // if (status) {
  //   panic("coudln't read\n");
  // }

  struct block_dev** reg_devs = get_registered_devs();
  int len = get_registered_devs_len();
  for (int i = 0; i < len; i++) {
    int res = partition_scan(reg_devs[i]);
    if (res < 0) panic("partition_scan failed.\n");
  }

  for (int i = 0; i < get_registered_devs_len(); i++) {
    vga_print(reg_devs[i]->name);
    vga_putchar('\n');
    if (strncmp(reg_devs[i]->name, "sata4p2", 8) == 0) {
      vga_print("BINGO!\n");
      char* buf = kzalloc(512);
      if (!buf) {
        panic("al failed\n");
      }

      struct phys_iovec vec[] = {{.addr = (uint64_t)VIRT_TO_PHYS(buf), .length = 512}};
      status = reg_devs[i]->read(reg_devs[i], 0, 1, vec, 1);
      if (status != KSTATUS_SUCCESS) {
        panic("read failed\n");
      }
      vga_print_color("result: ", VGA_COLOR_GREEN);
      vga_print(buf);
    }
  }

  // vga_print_color("result: ", VGA_COLOR_GREEN);
  // vga_print(buf);


  /*
    - the trick of putting snapos img on a parition doesnt actually work.
    - struct block_dev should hold sector size.
    - i should query sector count and size using AHCI.
    - Bug in kmalloc, check 'TODO' comment there.
    - 'TODO' comments in ahci.
    - partition_scan(). unwind what allocated if failed.
  */









  while (1) {
    __asm__ volatile("cli; hlt");
  }
}