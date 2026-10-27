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
#include <uaccess.h>

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

// not safe, no locks.
ssize_t __tty_read(struct tty_struct* tty, void* buf, size_t count) {
  if (!__tty_is_empty(tty)) {
    ssize_t available = (tty->head < tty->tail) ?
      (tty->tail - tty->head) :
      (TTY_BUF_SIZE - tty->head + tty->tail);
    size_t tocopy = size_min(available, count);

    // the tty buf is a circular buffer.
    // so the data we want to copy could be in two chunks, at the end and begining.
    // if its 2 chunks:
    // chunk1: from head to tail or the end of the buffer.
    // check2: from the start of the buf to the tail.

    size_t chunk1_size = size_min(tocopy, TTY_BUF_SIZE - tty->head);
    size_t chunk2_size = tocopy - chunk1_size;

    unsigned long uncopied1 = copy_to_user(buf, tty->buffer + tty->head, chunk1_size);
    unsigned long copied1 = chunk1_size - uncopied1;

    tty->head = (tty->head + copied1) % TTY_BUF_SIZE;

    if (uncopied1 > 0) {
      return copied1 == 0 ? -EFAULT : (ssize_t)copied1;
    }

    if (chunk2_size > 0) {
      unsigned long uncopied2 = copy_to_user(buf + chunk1_size, tty->buffer, chunk2_size);
      unsigned long copied2 = chunk2_size - uncopied2;

      tty->head = (tty->head + copied2) % TTY_BUF_SIZE;

      return copied1 + copied2;
    }

    return copied1;
  }

  return 0;
}

ssize_t tty_read(struct file* file, void* buf, size_t count, size_t* offset) {
  (void)offset;
  struct tty_struct* tty = (struct tty_struct*)file->dentry->inode->priv_data;

  struct wait_queue_entry wait_fg;
  struct wait_queue_entry wait_read;
  init_waitqueue_entry(&wait_fg, sched_current());
  init_waitqueue_entry(&wait_read, sched_current());
  wait_fg.func = pid_filter_wake_func;
  
  ssize_t ret = 0;
  while (!ret) {
    prepare_to_wait(&tty->fg_wait, &wait_fg, TASK_INTERRUPTIBLE);
    prepare_to_wait(&tty->read_wait, &wait_read, TASK_INTERRUPTIBLE);

    unsigned long flags;
    spin_lock_irqsave(&tty->lock, flags);

    if (__is_fg(tty, sched_current())) {
      ret = __tty_read(tty, buf, count);
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

int tty_set_fg(struct tty_struct* tty, uint32_t new_fg_pid) {
  int res = 0;

  unsigned long flags;
  spin_lock_irqsave(&tty->lock, flags);

  if (sched_current()->pid != tty->leader_pid) {
    res = -ENOTTY;
    goto exit;
  }

  tty->fg_pid = new_fg_pid;

  wake_up(&tty->fg_wait);

exit:
  spin_unlock_irqrestore(&tty->lock, flags);
  return res;
}