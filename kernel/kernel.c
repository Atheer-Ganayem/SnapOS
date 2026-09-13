#include <drivers/vga.h>
#include <mm.h>
#include <types.h>

void panic(char* s) {
  if (s) {
    vga_print_color(s, VGA_COLOR_RED);
  }

  while (1) {};
}

char* int_to_ascii(int value, char* buf, size_t buf_len) {
    if (buf_len == 0) return buf;
    
    char* p = buf + buf_len - 1;
    *p = '\0'; // Null terminator at the end
    
    if (value == 0) {
        if (buf_len > 1) {
            *--p = '0';
            return p;
        }
        return buf;
    }
    
    int is_negative = 0;
    unsigned int uval;
    
    if (value < 0) {
        is_negative = 1;
        uval = (unsigned int)(-value);
    } else {
        uval = (unsigned int)value;
    }
    
    while (uval > 0 && p > buf) {
        *--p = (char)('0' + (uval % 10));
        uval /= 10;
    }
    
    if (is_negative && p > buf) {
        *--p = '-';
    }
    
    return p;
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

  char* ptr = (char*)pmm_alloc_frame();
  if (!ptr) {
    panic("alloc failed\n");
  }
  ptr = PHYS_TO_VIRT(ptr);
  *ptr = 'A';
  *(ptr+1) = 'B';
  *(ptr + 2) = '\n';
  *(ptr + 3) = 0x00;
  vga_print(ptr);
  pmm_free_frame(VIRT_TO_PHYS(ptr));

  while (1) {}
}