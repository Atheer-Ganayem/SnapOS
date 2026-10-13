#ifndef SCHED_H
#define SCHED_H

#include <task.h>

void create_idle_task();

struct task_struct* sched_current();
struct task_struct* sched_next();
void sched_enqueue(struct task_struct* t);

void schedule();

#endif 