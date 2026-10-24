#ifndef SPINLOCK_H
#define SPINLOCK_H


#include <asm/irq.h>

// empty for now
// TODO: make a real spin lock.
typedef struct {

} spinlock_t;


static inline void spin_lock_init(spinlock_t* lock) {
  (void)lock;
};

#define spin_lock_irqsave(lock, flags) \
  do {  \
    (void)(lock); \
    (flags) = local_irq_save(); \
  } while (0)

#define spin_unlock_irqrestore(lock, flags) \
  do {  \
    (void)(lock); \
    local_irq_restore(flags);\
  } while (0)


static inline void spin_lock(spinlock_t* lock) {
  (void)lock;
};

static inline void spin_unlock(spinlock_t* lock) {
  (void)lock;
};

#endif