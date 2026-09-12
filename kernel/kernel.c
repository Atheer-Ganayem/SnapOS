#include <drivers/vga.h>
#include <mm.h>
#include <types.h>

void panic(char* s) {
  if (s) {
    vga_print_color(s, VGA_COLOR_RED);
  }

  while (1) {};
}

extern struct phys_region clean[];
extern size_t clean_count;

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


  uint64_t max_usable = 0;
  for (size_t i = 0; i < clean_count; i++) {
    if (clean[i].type == PHYS_REGION_USABLE) {
      max_usable = clean[i].end;
    }
  }

  max_usable = PHYS_TO_VIRT(max_usable)-4096;

  *(char*)max_usable = 'x';
  vga_putchar(*(char*)max_usable);

  while (1) {}
}