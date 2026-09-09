#include <drivers/vga.h>
#include <stdbool.h>

static volatile uint16_t* vga_buf = (volatile uint16_t*) (0xFFFFFFFF80000000 + 0xB8000);
static size_t x = 0, y = 0;

void vga_init() {
  for (int i = 0; i < VGA_BUF_WIDTH * VGA_BUF_HEIGHT; i++) {
    vga_buf[i] = 0x00; 
  }
}

static bool isfull() {
  return y < VGA_BUF_HEIGHT ? false : x >= VGA_BUF_WIDTH ? false : true;
}

static inline uint16_t makechar(char c, enum vga_color color) {
  return color << 8 | c;
}

void vga_putchar_color(char c, enum vga_color color) {
  if (isfull()) {
    return;
  }

  if (c == '\n') {
    x = 0;
    y++;
    return;
  } 

  vga_buf[y * VGA_BUF_WIDTH + x] = makechar(c, color);
  
  x++;
  if (x % VGA_BUF_WIDTH == 0) {
    x = 0;
    y++;
  }
}

inline void vga_putchar(char c) {
  vga_putchar_color(c, VGA_DEFAULT_TEXT_COLOR);
}

void vga_print_color(char* s, enum vga_color color) {
  while (*s) {
    vga_putchar_color(*s++, color);
  }
}

inline void vga_print(char* s) {
  vga_print_color(s, VGA_DEFAULT_TEXT_COLOR);
}