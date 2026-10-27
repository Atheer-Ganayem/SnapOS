#ifndef WAIT_H
#define WAIT_H

#include <list.h>
#include <spinlock.h>
#include <sched.h>

// This is inspired by the implementation of linux.
// So some fields my not be used yet, they are just there to match Linux's API.

// wait_queue_entry::flags
#define WQ_FLAG_EXCLUSIVE	0x01
#define WQ_FLAG_WOKEN		0x02
#define WQ_FLAG_CUSTOM		0x04
#define WQ_FLAG_DONE		0x08
#define WQ_FLAG_PRIORITY	0x10

struct wait_queue_entry;
struct wait_queue_head;

typedef int (*wait_queue_func_t)(struct wait_queue_entry* wq_entry, unsigned mode, int flags, void* key);

struct wait_queue_entry {
  unsigned int flags;
  void* private;
  wait_queue_func_t func;
  struct list_head entry;
};

struct wait_queue_head {
  spinlock_t lock;
  struct list_head head;
};

#define init_waitqueue_head(q) do { \
    spin_lock_init(&(q)->lock); \
    INIT_LIST_HEAD(&(q)->head); \
} while (0)

struct task_struct;

int default_wake_function(struct wait_queue_entry* wq_entry, unsigned mode, int flags, void* key);
int pid_filter_wake_func(struct wait_queue_entry* wq_entry, unsigned mode, int flags, void* key);

static inline void init_waitqueue_entry(struct wait_queue_entry* wq_entry, struct task_struct* t) {
  wq_entry->flags = 0;
  wq_entry->func = default_wake_function;
  wq_entry->private = t;  
  INIT_LIST_HEAD(&wq_entry->entry);
}

void prepare_to_wait(struct wait_queue_head* wq_head, struct wait_queue_entry* wq_entry, int state);

void __wakeup(struct wait_queue_head* wq_head, unsigned int mode, int nr_exclusive, void* key);
#define wake_up(x) __wakeup(x, 0, 1, NULL)
#define wake_up_all(x) __wakeup(x, 0, 0, NULL)

void finish_wait(struct wait_queue_head* wq_head, struct wait_queue_entry* wq_entry);

#endif