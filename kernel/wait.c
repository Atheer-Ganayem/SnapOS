#include <wait.h>
#include <sched.h>

int default_wake_function(struct wait_queue_entry* wq_entry, unsigned mode, int flags, void* key) {
  (void)mode, (void)flags, (void)key;
  struct task_struct* t = (struct task_struct*)wq_entry->private;
  return wake_up_task(t);
}

void prepare_to_wait(struct wait_queue_head* wq_head, struct wait_queue_entry* wq_entry, int state) {
  signed long flags;
  spin_lock_irqsave(&wq_head->lock, flags);

  if (list_empty(&wq_entry->entry)) {
  list_add_tail(&wq_entry->entry, &wq_head->head);
  }

  task_set_state((struct task_struct*)wq_entry->private, state);
  
  spin_unlock_irqrestore(&wq_head->lock, flags);
}

void finish_wait(struct wait_queue_head* wq_head, struct wait_queue_entry* wq_entry) {
  task_set_state((struct task_struct*)wq_entry->private, TASK_RUNNING);
  
  signed long flags;
  spin_lock_irqsave(&wq_head->lock, flags);

  if (!list_empty(&wq_entry->entry)) {
    list_del(&wq_entry->entry);
    INIT_LIST_HEAD(&wq_entry->entry);
  }

  spin_unlock_irqrestore(&wq_head->lock, flags);

}

void __wakeup(struct wait_queue_head* wq_head, unsigned int mode, int nr_exclusive, void* key) {
  signed long flags;
  spin_lock_irqsave(&wq_head->lock, flags);

  struct wait_queue_entry *cur;
  list_for_each_entry(cur, &wq_head->head, entry) {
    if (!mode || cur->flags & mode) {
      int woke_up = cur->func(cur, mode, 0, key);
      
      if (woke_up && (cur->flags & WQ_FLAG_EXCLUSIVE) && !--nr_exclusive) {
        break;
      }
    }
  }

  spin_unlock_irqrestore(&wq_head->lock, flags);
}

int pid_filter_wake_func(struct wait_queue_entry* wq_entry, unsigned mode, int flags, void* key) {
  (void)mode, (void)flags;
  uint32_t pid = *(uint32_t*)key;
  struct task_struct* t = (struct task_struct*)wq_entry->private;

  if (t->pid == pid) {
    return wake_up_task(t);
  }

  return 0;
}