#include <drivers/ps2.h>
#include <asm/io.h>
#include <irq.h>
#include <asm/irq.h>
#include <stddef.h>
#include <types.h>


void ps2_irq(unsigned irq, void* dev);

static inline void ps2_wait_read() {
  while ((inb(PS2_STATUS_REG) & 0x01) == 0);
}

static inline void ps2_wait_write() {
  while ((inb(PS2_STATUS_REG) & 0x02) != 0);
}

static inline void disable_ports() {
  ps2_wait_write();
  outb(PS2_CMD_REG, PS2_DISABLE_FIRST_PORT_CMD);
  ps2_wait_write();
  outb(PS2_CMD_REG, PS2_DISABLE_SECOND_PORT_CMD);
}

static void enable_first_port() {
  ps2_wait_write();
  outb(PS2_CMD_REG, PS2_READ_CTRL_CONFIG_BYTE_CMD);
  ps2_wait_read();
  uint8_t config_byte = inb(PS2_DATA_PORT);

  config_byte |= 0x01; // First PS/2 port interrupt
  config_byte |= (1 << 6); // First PS/2 port translation
  config_byte &= ~(1 << 4); // First PS/2 port clock 

  ps2_wait_write();
  outb(PS2_CMD_REG, PS2_WRITE_CONFIG_BYTE_CMD);
  ps2_wait_write();
  outb(PS2_DATA_PORT, config_byte);

  ps2_wait_write();
  outb(PS2_CMD_REG, PS2_ENABLE_FIRST_PORT_CMD);
}

static int controller_self_test() {
  ps2_wait_write();
  outb(PS2_CMD_REG, PS2_SELF_TEST_CMD);
  
  ps2_wait_read();
  if (inb(PS2_DATA_PORT) != 0x55)
    return -EFAULT;

  return 0;
}

static int first_port_test() {
  ps2_wait_write();
  outb(PS2_CMD_REG, PS2_FIRST_PORT_TEST_CMD);

  ps2_wait_read();
  if (inb(PS2_DATA_PORT) != 0x00)
    return -EFAULT;

  return 0;
}

static int reset_devices() {
  ps2_wait_write();
  outb(PS2_DATA_PORT, 0xFF);

  ps2_wait_read();
  if (inb(PS2_DATA_PORT) != 0xFA)
    return -EFAULT;

  ps2_wait_read();
  if (inb(PS2_DATA_PORT) != 0xAA)
    return -EFAULT;

  return 0;
}

int ps2_kyb_init() {
  disable_ports();

  // flush garbage data.
  while (inb(PS2_STATUS_REG) & 0x01) {
    inb(PS2_DATA_PORT);
  }

  if (controller_self_test() != 0) return -EFAULT;
  if (first_port_test() != 0) return -EFAULT;
  
  enable_first_port();
  
  if (reset_devices() != 0) return -EFAULT;

  if (request_irq(IRQ_PS2, ps2_irq, "ps2-kyb", NULL) != 0)
    return -EFAULT;

  return 0;
}



void ps2_irq(unsigned irq, void* dev) {
  (void)irq; (void)dev;
  static bool is_extended = false;

  uint8_t scancode = inb(0x60);
  bool is_release = (scancode & 0x80);
  uint8_t raw_key = scancode & (~0x80);

  if (is_extended) {
    // we will handle this later.
    is_extended = false;
    return;
  }

  if (scancode == 0xE0) {
    is_extended = true;
    return;
  }

  if (raw_key < (sizeof(ps2_scancode_base) / sizeof(ps2_scancode_base[0]))) {
    keycode_t key = ps2_scancode_base[raw_key];
    if (key) {
      input_handle_event(raw_key, is_release);
    }
  }
}