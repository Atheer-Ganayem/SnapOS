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
#include <input.h>

struct tty_struct {
  char buffer[TTY_BUF_SIZE];
  size_t head, tail, canon_lines;
  spinlock_t lock;
  uint32_t fg_pid;
  uint32_t leader_pid;

  struct wait_queue_head read_wait; // used for foreground tasks waiting to read.
  struct wait_queue_head fg_wait; // used for background tasks waiting to be foreground.
};

struct tty_struct* tty0 = NULL;
struct file* tty0_file = NULL;

ssize_t tty_read(struct file* file, void* buf, size_t count, size_t* offset);
ssize_t tty_write(struct file* f, const void* buf, size_t count, size_t* offset);
static struct file_operations tty_fops = {
  .read = tty_read,
  .write = tty_write,
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

  struct file* f = kzalloc(sizeof(struct file));
  if (!f) {
    kfree(tty);
    d_free(d);
    return -ENOMEM;
  }

  f->dentry = d;
  f->fops = &tty_fops;
  f->private_data = tty;
  tty0 = tty;
  tty0_file = f;

  return 0;
}

static inline bool __tty_has_line(struct tty_struct* tty) {
  return tty->canon_lines > 0;
}

// You need to lock to call this.
static inline bool __tty_is_empty(struct tty_struct* tty) {
  return tty->head == tty->tail;
}

// You need to lock to call this.
static bool __is_fg(struct tty_struct* tty, struct task_struct* t) {
  return tty->fg_pid == t->pid;
}

void __handle_backspace(struct tty_struct* tty) {
  if (!__tty_is_empty(tty)) {
    tty->tail = (tty->tail == 0) ? (TTY_BUF_SIZE - 1) : (tty->tail - 1); 
    vga_del_last();
  }
}

void tty_push_char(struct tty_struct* tty, char input) {
  bool new_line = false;
  
  signed long flags;
  spin_lock_irqsave(&tty->lock, flags);

  switch (input) {
  case '\b':
    __handle_backspace(tty);
    break;
  
  default:
    uint32_t tail = (tty->tail + 1 ) % TTY_BUF_SIZE;
    if (tail != tty->head) {
      tty->buffer[tty->tail] = input;
      tty->tail = tail;
    }

    if (input == '\n') {
      tty->canon_lines++;
      new_line = true;
    }

    vga_putchar(input);
  }

  spin_unlock_irqrestore(&tty->lock, flags);

  if (new_line) {
    wake_up(&tty->read_wait);
  }
}

// not safe, no locks.
// buf is kernel buf NOT USER!
size_t __tty_read(struct tty_struct* tty, void* buf, size_t count) {
  if (!__tty_has_line(tty))
    return 0;

  size_t available = (tty->head < tty->tail) ?
    (tty->tail - tty->head) :
    (TTY_BUF_SIZE - tty->head + tty->tail);
  size_t tocopy = size_min(available, count);


  size_t i;
  for (i = 0; i < tocopy; i++) {
    char c = tty->buffer[(tty->head + i) % TTY_BUF_SIZE];

    ((char*)buf)[i] = c;

    if (c == '\n') {
      tty->canon_lines--;
      i++;
      break;
    }
  }

  tty->head = ((tty->head + i) % TTY_BUF_SIZE);

  return i;
}

ssize_t tty_read(struct file* file, void* buf, size_t count, size_t* offset) {
  /* Note:
    If you look and understand the code you would notice that there is a "race condition",
    let's say for example we have 2 lines buffered, both task A and B want to read and issue
    a sys_read to the same TTY.
    We use a bounce buffer, let's assume the first line is bigger than the bounce buffer size and 
    we have to read it in to iterations (bounce twice).
    Task A reads the first half line, then task B reads the rest of the first line, then task A
    reads the SECOND (!!!) line, so now task A has in it's buffer half of the first line and the second line.

    After doing some research, i have found that POSIX doesn't garantuee atomicy for terminal reads,
    so this reall isn't a race condition, it's the user's responsibility to not do concurrent reads if they
    want the data to be 100% correct.

    note: Linux's garantuees atomicy because they user mutexes. 
  */
  (void)offset;
  
  struct tty_struct* tty = (struct tty_struct*)file->private_data;

  struct wait_queue_entry wait_fg;
  struct wait_queue_entry wait_read;
  init_waitqueue_entry(&wait_fg, sched_current());
  init_waitqueue_entry(&wait_read, sched_current());
  wait_fg.func = pid_filter_wake_func;
  
  size_t ret = 0;
  size_t total = 0;

  char bounce[TTY_BOUNCE_BUF_SIZE];

  while (total < count) {
    prepare_to_wait(&tty->fg_wait, &wait_fg, TASK_INTERRUPTIBLE);
    prepare_to_wait(&tty->read_wait, &wait_read, TASK_INTERRUPTIBLE);

    unsigned long flags;
    spin_lock_irqsave(&tty->lock, flags);

    size_t lines_before = tty->canon_lines;

    if (__is_fg(tty, sched_current())) {
      size_t req_count = size_min(count - total, TTY_BOUNCE_BUF_SIZE);
      ret = __tty_read(tty, bounce, req_count);
    }

    size_t lines_after  = tty->canon_lines;

    spin_unlock_irqrestore(&tty->lock, flags);

    // we cant allow ret = 0 and total = 0, because of the line garantuee

    // No data read at all
    if (total == 0 && ret == 0) {
      schedule();
      continue;
    } else if (ret == 0) {
      break;
    }

    unsigned long uncopied = copy_to_user(buf, bounce, ret);
    unsigned long copied = ret - uncopied;

    buf += copied;
    total += copied;

    if (total == 0)
      return -EFAULT;

    if (uncopied)
      break;

    if (lines_after != lines_before)
      break;

    // now if lines_after == lines_before, then our line is still not fully read.
  }
  finish_wait(&tty->fg_wait, &wait_fg);
  finish_wait(&tty->read_wait, &wait_read);

  return (ssize_t)total;
}

// static int tty_set_fg(struct tty_struct* tty, uint32_t new_fg_pid) {
//   int res = 0;

//   unsigned long flags;
//   spin_lock_irqsave(&tty->lock, flags);

//   if (sched_current()->pid != tty->leader_pid) {
//     res = -ENOTTY;
//     goto exit;
//   }

//   tty->fg_pid = new_fg_pid;

//   wake_up(&tty->fg_wait);

// exit:
//   spin_unlock_irqrestore(&tty->lock, flags);
//   return res;
// }

static ssize_t __tty_write(struct tty_struct* tty, const void* buf, size_t count, size_t* offset) {
  (void)offset;

  struct wait_queue_entry wait;
  init_waitqueue_entry(&wait, sched_current());
  wait.func = pid_filter_wake_func;

  while (1) {
    prepare_to_wait(&tty->fg_wait, &wait, TASK_INTERRUPTIBLE);

    unsigned long flags;
    spin_lock_irqsave(&tty->lock, flags);

    if (!__is_fg(tty, sched_current())) {
      spin_unlock_irqrestore(&tty->lock, flags);
      schedule();
      continue;;
    }

    spin_unlock_irqrestore(&tty->lock, flags);
    break;
  }
  finish_wait(&tty->fg_wait, &wait);


  char bounce[TTY_BOUNCE_BUF_SIZE];
  size_t total = 0;

  while (total < count) {
    size_t remaining = count - total;
    size_t to_copy = size_min(remaining, TTY_BOUNCE_BUF_SIZE);

    unsigned long uncopied = copy_from_user(bounce, buf, to_copy);

    size_t copied = to_copy - uncopied;

    for (size_t i = 0; i < copied; i++)
      vga_putchar(((char*)bounce)[i]);

    total += copied;
    buf += copied;

    if (total == 0)
      return -EFAULT;

    if (uncopied)
      break;
  }

  return (ssize_t)total;
}

ssize_t tty_write(struct file* f, const void* buf, size_t count, size_t* offset) {
  struct tty_struct* tty = (struct tty_struct*)f->private_data;
  return __tty_write(tty, buf, count, offset);
}