#include <input.h>
#include <drivers/vga.h>

static const char ascii_map[128] = {
  [KEY_A] = 'a', [KEY_B] = 'b', [KEY_C] = 'c', [KEY_D] = 'd', [KEY_E] = 'e',
  [KEY_F] = 'f', [KEY_G] = 'g', [KEY_H] = 'h', [KEY_I] = 'i', [KEY_J] = 'j',
  [KEY_K] = 'k', [KEY_L] = 'l', [KEY_M] = 'm', [KEY_N] = 'n', [KEY_O] = 'o',
  [KEY_P] = 'p', [KEY_Q] = 'q', [KEY_R] = 'r', [KEY_S] = 's', [KEY_T] = 't',
  [KEY_U] = 'u', [KEY_V] = 'v', [KEY_W] = 'w', [KEY_X] = 'x', [KEY_Y] = 'y',
  [KEY_Z] = 'z', 
  [KEY_1] = '1', [KEY_2] = '2', [KEY_3] = '3', [KEY_4] = '4', [KEY_5] = '5',
  [KEY_6] = '6', [KEY_7] = '7', [KEY_8] = '8', [KEY_9] = '9', [KEY_0] = '0',
  [KEY_MINUS] = '-', [KEY_EQUAL] = '=', [KEY_SPACE] = ' ',
  [KEY_ENTER] = '\n', [KEY_BACKSPACE] = '\b',
  [KEY_COMMA] = ',', [KEY_DOT] = '.', [KEY_SLASH] = '/',
  [KEY_SEMICOLON] = ';', [KEY_APOSTROPHE] = '\'', [KEY_GRAVE] = '`',
  [KEY_LEFTBRACE] = '[', [KEY_RIGHTBRACE] = ']', [KEY_BACKSLASH] = '\\'
};

static const char ascii_shift_map[128] = {
  [KEY_A] = 'A', [KEY_B] = 'B', [KEY_C] = 'C', [KEY_D] = 'D', [KEY_E] = 'E',
  [KEY_F] = 'F', [KEY_G] = 'G', [KEY_H] = 'H', [KEY_I] = 'I', [KEY_J] = 'J',
  [KEY_K] = 'K', [KEY_L] = 'L', [KEY_M] = 'M', [KEY_N] = 'N', [KEY_O] = 'O',
  [KEY_P] = 'P', [KEY_Q] = 'Q', [KEY_R] = 'R', [KEY_S] = 'S', [KEY_T] = 'T',
  [KEY_U] = 'U', [KEY_V] = 'V', [KEY_W] = 'W', [KEY_X] = 'X', [KEY_Y] = 'Y',
  [KEY_Z] = 'Z',
  [KEY_1] = '!', [KEY_2] = '@', [KEY_3] = '#', [KEY_4] = '$', [KEY_5] = '%',
  [KEY_6] = '^', [KEY_7] = '&', [KEY_8] = '*', [KEY_9] = '(', [KEY_0] = ')',
  [KEY_MINUS] = '_', [KEY_EQUAL] = '+', [KEY_SPACE] = ' ',
  [KEY_ENTER] = '\n', [KEY_BACKSPACE] = '\b',
  [KEY_COMMA] = '<', [KEY_DOT] = '>', [KEY_SLASH] = '?',
  [KEY_SEMICOLON] = ':', [KEY_APOSTROPHE] = '"', [KEY_GRAVE] = '~',
  [KEY_LEFTBRACE] = '{', [KEY_RIGHTBRACE] = '}', [KEY_BACKSLASH] = '|'
};

static int shift_count = 0;
static bool caps_active = false;
static int ctrl_count = 0;

void input_handle_event(keycode_t key, bool is_release) {
  if (key == KEY_LEFTSHIFT || key == KEY_RIGHTSHIFT) {
    if (is_release) shift_count--;
    else shift_count++;

    // handling edge cases. Maybe a packet dropped.
    if (shift_count < 0) shift_count = 0;
    else if (shift_count > 2) shift_count = 2;
    return;
  } 

  if (key == KEY_LEFTCTRL || key == KEY_RIGHTCTRL) {
    if (is_release) ctrl_count--;
    else ctrl_count++;

    // handling edge cases. Maybe a packet dropped.
    if (ctrl_count < 0) ctrl_count = 0;
    else if (ctrl_count > 2) ctrl_count = 2;
    return;
  }

  if (key == KEY_CAPSLOCK && !is_release) {
    caps_active = !caps_active;
    return;
  }

  if (is_release) return;

  // out of ascii array bounds
  if (key >= sizeof(ascii_map)) return;

  char c = ascii_map[key];

  if (key >= KEY_Q && key <= KEY_M) {
    bool is_upper = caps_active;
    is_upper = shift_count ? !is_upper : is_upper;
    c = is_upper ? ascii_shift_map[key] : c;
  } else {
    c = shift_count ? ascii_shift_map[key] : c;
  }

  if (c) {
    vga_putchar(c);
  }
  // tty later
}