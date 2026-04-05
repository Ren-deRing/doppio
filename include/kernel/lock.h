#pragma once

#include <stdint.h>

typedef struct {
    volatile int locked;
} spinlock_t;

#define SPINLOCK_INITIALIZER { .locked = 0 }

void spin_lock_init(spinlock_t *lock);
void spin_lock(spinlock_t *lock);
void spin_unlock(spinlock_t *lock);
uint64_t spin_lock_irqsave(spinlock_t *lock);
void spin_unlock_irqrestore(spinlock_t *lock, uint64_t flags);