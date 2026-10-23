#ifndef LIST_H
#define LIST_H

#include <container_of.h>

#define list_entry(ptr, type, member) \
        container_of(ptr, type, member)


// used for static allocs.
// (init at compile time).
#define LIST_HEAD_INIT(name) {&(name), &(name)}

struct list_head {
  struct list_head* next;
  struct list_head* prev;
};

// for dynamic allocs.
// (init at run time).
static inline void INIT_LIST_HEAD(struct list_head* list) {
  list->next = list;
  list->prev = list;
}

static inline void list_add(struct list_head* _new, struct list_head* head) {
  _new->prev = head;
  _new->next = head->next;
  head->next->prev = _new;
  head->next = _new;
}

static inline void list_add_tail(struct list_head* _new, struct list_head* head) {
  _new->next = head;
  _new->prev = head->prev;
  head->prev->next = _new;
  head->prev = _new;
}

static inline void list_del(struct list_head* entry) {
  entry->prev->next = entry->next;
  entry->next->prev = entry->prev;
}

static inline bool list_empty(struct list_head* head) {
  return head->next == head;
}

#endif