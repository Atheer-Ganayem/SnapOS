#include <drivers/vga.h>
#include <mm.h>

void panic(char* s) {
  if (s) {
    vga_print_color(s, VGA_COLOR_RED);
  }

  while (1) {};
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

  char* ptr = (char*)kmalloc(1024);
  if (!ptr) {
    panic("alloc failed\n");
  }
  *ptr = 'A';
  *(ptr+1) = 'B';
  *(ptr + 2) = '\n';
  *(ptr + 3) = 0x00;
  vga_print(ptr);

  char* ptr2 = (char*)kmalloc(1024);
  char* ptr3 = (char*)kmalloc(1024);
  char* ptr4 = (char*)kmalloc(1024);
  if (!ptr2 || !ptr3 || !ptr4) {
    panic("alloc failed\n");
  }
  
  kfree(ptr);
  kfree(ptr4);
  kfree(ptr2);
  kfree(ptr3);

  while (1) {}
}