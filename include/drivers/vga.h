#ifndef VGA_H
#define VGA_H

#include <stdint.h>
#include <stddef.h>

enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE,
    VGA_COLOR_GREEN,
    VGA_COLOR_CYAN,
    VGA_COLOR_RED,
    VGA_COLOR_MAGENTA,
    VGA_COLOR_BROWN,
    VGA_COLOR_LIGHT_GRAY,
    VGA_COLOR_DARK_GRAY,
    VGA_COLOR_LIGHT_BLUE,
    VGA_COLOR_LIGHT_GREEN,
    VGA_COLOR_LIGHT_CYAN,
    VGA_COLOR_LIGHT_RED,
    VGA_COLOR_LIGHT_MAGENTA,
    VGA_COLOR_YELLOW,
    VGA_COLOR_WHITE
};

#define VGA_DEFAULT_TEXT_COLOR  VGA_COLOR_WHITE
#define VGA_BUF_WIDTH           80
#define VGA_BUF_HEIGHT          25


void vga_init();
void vga_putchar(char c);
void vga_print(char* s);
void vga_print_color(char* s, enum vga_color color);
void vga_putchar_color(char c, enum vga_color color);

#endif