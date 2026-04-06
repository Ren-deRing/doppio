#pragma once

#include <kernel/lock_types.h>

#define SPINLOCK_INITIALIZER { .now_serving = 0, .next_ticket = 0, .holder_cpu = -1 }
#define MUTEX_INITIALIZER(name) { \
    .wait_lock = SPINLOCK_INITIALIZER, \
    .locked = 0, \
    .owner = NULL, \
    .wait_queue = LIST_HEAD_INIT((name).wait_queue); \
}

void spin_lock_init(spinlock_t *lock);
void spin_lock(spinlock_t *lock);
void spin_unlock(spinlock_t *lock);
uint64_t spin_lock_irqsave(spinlock_t *lock);
void spin_unlock_irqrestore(spinlock_t *lock, uint64_t flags);

void mutex_init(mutex_t *m);
void mutex_lock(mutex_t *m);
void mutex_unlock(mutex_t *m);