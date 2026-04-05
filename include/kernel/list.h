#pragma once

#include <stddef.h>

struct list_node {
    struct list_node *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
    struct list_node name = LIST_HEAD_INIT(name)

static inline void list_init(struct list_node *list) {
    list->next = list;
    list->prev = list;
}

static inline void __list_add(struct list_node *new_node,
                              struct list_node *prev,
                              struct list_node *next) {
    next->prev = new_node;
    new_node->next = next;
    new_node->prev = prev;
    prev->next = new_node;
}

static inline void list_add(struct list_node *new_node, struct list_node *head) {
    __list_add(new_node, head, head->next);
}

static inline void list_add_tail(struct list_node *new_node, struct list_node *head) {
    __list_add(new_node, head->prev, head);
}

static inline void __list_del(struct list_node *prev, struct list_node *next) {
    next->prev = prev;
    prev->next = next;
}

static inline void list_del(struct list_node *entry) {
    __list_del(entry->prev, entry->next);
    entry->next = NULL;
    entry->prev = NULL;
}

static inline int list_empty(const struct list_node *head) {
    return head->next == head;
}

#ifndef container_of
#define container_of(ptr, type, member) ({                      \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type,member) );})
#endif

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

#define list_first_entry(ptr, type, member) \
    list_entry((ptr)->next, type, member)

#define list_for_each_entry(pos, head, member)                          \
    for (pos = list_entry((head)->next, typeof(*pos), member);          \
         &pos->member != (head);                                        \
         pos = list_entry(pos->member.next, typeof(*pos), member))