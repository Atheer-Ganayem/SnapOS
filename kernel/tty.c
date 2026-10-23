#include <tty.h>
#include <drivers/vga.h>
#include <types.h>
#include <paging.h>
#include <vfs.h>

#define TTY_BUF_SIZE PAGE_SIZE

struct task_struct;
struct tty_struct {
  char buffer[TTY_BUF_SIZE];
  uint32_t fg_pid;
  uint32_t leader_pid;
  size_t head, tail;
};

static inline bool tty_is_empty(struct tty_struct* tty) {
  return tty->head == tty->tail;
}

// static void tty_push_char(struct tty_struct* tty, char input) {
//   vga_putchar(input);
  
//   uint32_t tail = (tty->tail + 1 ) % TTY_BUF_SIZE;
//   if (tail == tty->head) {
//     return;
//   }
  
//   tty->buffer[tty->tail] = input;

//   tty->tail = tail;
// }

// assumed there is a char to pop
static char tty_pop_char(struct tty_struct* tty) {
  char c = tty->buffer[tty->head];
  tty->head = (tty->head + 1) % TTY_BUF_SIZE;
  return c;
}

ssize_t tty_read(struct file* file, void* buf, size_t count, size_t* offset) {
  (void)offset;
  // TODO: F*CK, i didnt do that. 
  struct tty_struct* tty = (struct tty_struct*)file->dentry->inode->priv_data;

  if (tty_is_empty(tty)) {
    // sleep, later.
  }
  
  ssize_t available = (tty->head < tty->tail) ? (tty->tail - tty->head) : (tty->head - tty->tail);
  ssize_t tocopy = size_min(available, count);

  for (ssize_t i = 0; i < tocopy; i++) {
    ((char*)buf)[i] = tty_pop_char(tty);
  }

  return tocopy;
}

