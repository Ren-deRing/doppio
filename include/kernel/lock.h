#ifndef __KERNEL_LOCK_H
#define __KERNEL_LOCK_H

#include <stdint.h>
#include "kernel/cpu.h"

typedef struct {
    volatile int locked;
} spinlock_t;

static inline void spin_lock_init(spinlock_t *lock) {
    lock->locked = 0;
}

static inline void spin_lock(spinlock_t *lock) {
    while (__sync_lock_test_and_set(&lock->locked, 1)) {
        arch_pause();
    }
}

static inline void spin_unlock(spinlock_t *lock) {
    __sync_lock_release(&lock->locked);
}

static inline uint64_t spin_lock_irqsave(spinlock_t *lock) {
    uint64_t flags = arch_irq_save();
    spin_lock(lock);
    return flags;
}

static inline void spin_unlock_irqrestore(spinlock_t *lock, uint64_t flags) {
    spin_unlock(lock);
    arch_irq_restore(flags);
}

#endif