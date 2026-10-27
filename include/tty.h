#ifndef TTY_H
#define TTY_H

#define TTY_BUF_SIZE 512

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

struct tty_operations {
  
};

struct file;
struct tty_struct;
extern struct file* tty0_file;
extern struct tty_struct* tty0;

void tty_push_char(struct tty_struct* tty, char input);

int tty_init();


#endif