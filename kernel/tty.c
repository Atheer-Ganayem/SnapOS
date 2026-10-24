#include <tty.h>
#include <drivers/vga.h>
#include <types.h>
#include <paging.h>
#include <vfs.h>
#include <spinlock.h>
#include <sched.h>

#define TTY_BUF_SIZE 512

struct task_struct;
struct tty_struct {
  char buffer[TTY_BUF_SIZE];
  size_t head, tail;
  spinlock_t lock;
  uint32_t fg_pid;
  uint32_t leader_pid;

  struct task_struct* sleeping_task;
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

static inline bool tty_is_empty(struct tty_struct* tty) {
  signed long flags;
  spin_lock_irqsave(tty->lock, flags);

  bool ret = tty->head == tty->tail;
  
  spin_unlock_irqrestore(tty->lock, flags);
  return ret;
}

#include <drivers/vga.h>
void tty_push_char(struct tty_struct* tty, char input) {
  signed long flags;
  spin_lock_irqsave(tty->lock, flags);

  uint32_t tail = (tty->tail + 1 ) % TTY_BUF_SIZE;
  if (tail != tty->head) {
    tty->buffer[tty->tail] = input;
    tty->tail = tail;
  }

  if (tty->sleeping_task) {
    wake_up_task(tty->sleeping_task);
    tty->sleeping_task = NULL;
  }

  spin_unlock_irqrestore(tty->lock, flags);
}

// assumed there is a char to pop
static char tty_pop_char(struct tty_struct* tty) {
  signed long flags;
  spin_lock_irqsave(tty->lock, flags);

  char c = tty->buffer[tty->head];
  tty->head = (tty->head + 1) % TTY_BUF_SIZE;

  spin_unlock_irqrestore(tty->lock, flags);
  return c;
}


#include <sched.h>
ssize_t tty_read(struct file* file, void* buf, size_t count, size_t* offset) {
  vga_print("in tty read\n");
  (void)offset;
  struct tty_struct* tty = (struct tty_struct*)file->dentry->inode->priv_data;

  while (tty_is_empty(tty)) {
    signed long flags;
    spin_lock_irqsave(tty->lock, flags);

    set_current_state(TASK_INTERRUPTIBLE);
    tty->sleeping_task = sched_current();
    vga_print("going to sleep\n");

    spin_unlock_irqrestore(tty->lock, flags);
    schedule();
  }

  vga_print("got a letter\n");
  
  ssize_t available = (tty->head < tty->tail) ?
                      (tty->tail - tty->head) :
                      (TTY_BUF_SIZE - tty->head + tty->tail);
  ssize_t tocopy = size_min(available, count);

  for (ssize_t i = 0; i < tocopy; i++) {
    ((char*)buf)[i] = tty_pop_char(tty);
  }

  return tocopy;
}

