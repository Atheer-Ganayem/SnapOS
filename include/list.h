#ifndef LIST_H
#define LIST_H

#include <container_of.h>
#include <stdbool.h>
#include <stddef.h>

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

static inline void list_add(struct list_head* new, struct list_head* head) {
  new->prev = head;
  new->next = head->next;
  head->next->prev = new;
  head->next = new;
}

static inline void list_add_tail(struct list_head* new, struct list_head* head) {
  new->next = head;
  new->prev = head->prev;
  head->prev->next = new;
  head->prev = new;
}

static inline void list_del(struct list_head* entry) {
  entry->prev->next = entry->next;
  entry->next->prev = entry->prev;
}

static inline bool list_empty(struct list_head* head) {
  return head->next == head;
}

#define list_for_each(pos, head)  \
        for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_entry(pos, head, member)  \
        for (pos = list_entry((head)->next, typeof(*pos), member);  \
            &(pos)->member != (head); \
            pos = list_entry((pos)->member.next, typeof(*pos), member))

// n = next
#define list_for_each_entry_safe(pos, n, head, member)  \
        for (pos = list_entry((head)->next, typeof(*pos), member), n = list_entry(pos->member.next, typeof(*pos), member);  \
            &(pos)->member != (head); \
            pos = n, n = list_entry(n->member.next, typeof(*pos), member))

#endif