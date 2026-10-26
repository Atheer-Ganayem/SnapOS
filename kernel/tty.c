#include <tty.h>
#include <drivers/vga.h>
#include <types.h>
#include <paging.h>
#include <vfs.h>
#include <spinlock.h>
#include <sched.h>
#include <wait.h>
#include <sched.h>
#include <string.h>


#define TTY_BUF_SIZE 512
#define TMP_BUF_SIZE 64

struct task_struct;
struct tty_struct {
  char buffer[TTY_BUF_SIZE];
  size_t head, tail;
  spinlock_t lock;
  uint32_t fg_pid;
  uint32_t leader_pid;

  struct wait_queue_head read_wait; // used for foreground tasks waiting to read.
  struct wait_queue_head fg_wait; // used for background tasks waiting to be foreground.
};

struct tty_struct* tty0 = NULL;
struct file* tty0_file = NULL;

ssize_t tty_read(struct file* file, void* buf, size_t count, size_t* offset);
static struct file_operations tty_fops = {
  .read = tty_read
};

int tty_init() {
  struct tty_struct* tty = kzalloc(sizeof(struct tty_struct));
  if (!tty)
    return -ENOMEM;

  spin_lock_init(&tty->lock);
  init_waitqueue_head(&tty->read_wait);
  init_waitqueue_head(&tty->fg_wait);
  tty->leader_pid = 1;
  tty->fg_pid = 1;

  struct qstr str = {.str = "tty0", .len = 4, .hash = vfs_hash_name("tty0", 4)}; 
  struct dentry* d = d_alloc(NULL, &str);
  if (IS_ERR(d)) {
    kfree(tty);
    return PTR_ERR(d);
  }

  struct inode* inode = kzalloc(sizeof(struct inode));
  if (!inode) {
    kfree(tty);
    d_free(d);
    return -ENOMEM;
  }

  inode->priv_data = (void*)(tty);
  d->inode = inode;
  
  struct file* f = kzalloc(sizeof(struct file));
  if (!f) {
    kfree(tty);
    d_free(d);
    kfree(inode);
    return -ENOMEM;
  }

  f->dentry = d;
  f->fops = &tty_fops;
  tty0 = tty;
  tty0_file = f;

  return 0;
}

// You need to lock to call this.
static inline bool __tty_is_empty(struct tty_struct* tty) {
  return tty->head == tty->tail;
}

// You need to lock to call this.
static bool __is_fg(struct tty_struct* tty, struct task_struct* t) {
  return tty->fg_pid == t->pid;
}

// This holds the lock.
static inline bool tty_is_empty(struct tty_struct* tty) {
  signed long flags;
  spin_lock_irqsave(&tty->lock, flags);

  bool ret = __tty_is_empty(tty);
  
  spin_unlock_irqrestore(&tty->lock, flags);
  return ret;
}

void tty_push_char(struct tty_struct* tty, char input) {
  signed long flags;
  spin_lock_irqsave(&tty->lock, flags);

  uint32_t tail = (tty->tail + 1 ) % TTY_BUF_SIZE;
  if (tail != tty->head) {
    tty->buffer[tty->tail] = input;
    tty->tail = tail;
  }
  
  spin_unlock_irqrestore(&tty->lock, flags);
  
  wake_up(&tty->read_wait);
}

#include <drivers/vga.h>

// assumed there is a char to pop.
static char __tty_pop_char(struct tty_struct* tty) {
  char c = tty->buffer[tty->head];
  tty->head = (tty->head + 1) % TTY_BUF_SIZE;

  vga_print("popping: ");
  vga_putchar(c);
  vga_print("\n");

  return c;
}

// not safe, no locks.
ssize_t __tty_read(struct tty_struct* tty, void* buf, size_t count) {
  ssize_t tocopy = 0;

  if (!__tty_is_empty(tty)) {
    ssize_t available = (tty->head < tty->tail) ?
      (tty->tail - tty->head) :
      (TTY_BUF_SIZE - tty->head + tty->tail);
    tocopy = size_min(available, count);

    for (ssize_t i = 0; i < tocopy; i++) {
      ((char*)buf)[i] = __tty_pop_char(tty);
    }
  }

  return tocopy;
}

ssize_t tty_read(struct file* file, void* buf, size_t count, size_t* offset) {
  (void)offset;
  struct tty_struct* tty = (struct tty_struct*)file->dentry->inode->priv_data;

  struct wait_queue_entry wait_fg;
  struct wait_queue_entry wait_read;
  init_waitqueue_entry(&wait_fg, sched_current());
  init_waitqueue_entry(&wait_read, sched_current());
  
  ssize_t ret = 0;
  while (!ret) {
    prepare_to_wait(&tty->fg_wait, &wait_fg, TASK_INTERRUPTIBLE);
    prepare_to_wait(&tty->read_wait, &wait_read, TASK_INTERRUPTIBLE);

    ssize_t to_read = size_min(TMP_BUF_SIZE, count);
    unsigned long flags;

    spin_lock_irqsave(&tty->lock, flags);
    if (__is_fg(tty, sched_current())) {
      ret = __tty_read(tty, buf, to_read);
    }
    spin_unlock_irqrestore(&tty->lock, flags);

    if (ret == 0) {
      schedule();
      continue;
    }
  }
  finish_wait(&tty->fg_wait, &wait_fg);
  finish_wait(&tty->read_wait, &wait_read);

  return ret;
}