#include <kernel/sched.h>
#include <kernel/proc.h>
#include <kernel/cpu.h>
#include <kernel/lock.h>
#include <kernel/init.h>
#include <kernel/intc.h>
#include <kernel/kmem.h>
#include <string.h>

struct thread *curthread = NULL;

static struct {
    spinlock_t lock;
    struct thread *head;
    struct thread *tail;
} ready_queue;

static struct {
    spinlock_t lock;
    struct thread *head;
} sleep_queue;

void sched_enqueue(struct thread *t) {
    if (!t || t->t_tid == 0) return;

    spin_lock(&ready_queue.lock);
    t->t_state = THREAD_READY;
    t->t_sched_next = NULL;

    if (ready_queue.tail) {
        ready_queue.tail->t_sched_next = t;
    } else {
        ready_queue.head = t;
    }
    ready_queue.tail = t;
    spin_unlock(&ready_queue.lock);
}

struct thread* sched_dequeue(void) {
    spin_lock(&ready_queue.lock);
    struct thread *t = ready_queue.head;
    if (t) {
        ready_queue.head = t->t_sched_next;
        if (!ready_queue.head) ready_queue.tail = NULL;
        t->t_sched_next = NULL;
    }
    spin_unlock(&ready_queue.lock);
    return t;
}

struct thread* pick_next_thread(void) {
    struct thread *prev = curthread;

    if (prev && prev->t_state == THREAD_RUNNING && prev->t_tid != 0) {
        sched_enqueue(prev);
    }

    struct thread *next = sched_dequeue();
    if (!next) {
        next = arch_get_idle_thread();
    }

    next->t_state = THREAD_RUNNING;
    curthread = next;
    
    arch_set_current_thread(next);

    return next;
}

void schedule(void) {
    if (curthread) {
        curthread->t_need_resched = true;
        arch_request_resched();
    }
}

void thread_yield(void) {
    schedule();
}

void thread_sleep(uint64_t ms) {
    uint64_t flags = arch_irq_save();
    
    spin_lock(&sleep_queue.lock);
    curthread->t_sleep_until = arch_get_system_ticks() + ms;
    curthread->t_state = THREAD_SLEEP;
    curthread->t_sched_next = sleep_queue.head;
    sleep_queue.head = curthread;
    spin_unlock(&sleep_queue.lock);
    
    arch_irq_restore(flags);

    schedule();
}

void sched_tick(void) {
    uint64_t now = arch_get_system_ticks();
    
    spin_lock(&sleep_queue.lock);
    struct thread *curr = sleep_queue.head;
    struct thread *prev = NULL;
    while (curr) {
        struct thread *next = curr->t_sched_next;
        if (now >= curr->t_sleep_until) {
            if (prev) prev->t_sched_next = next;
            else sleep_queue.head = next;
            
            sched_enqueue(curr);
        } else {
            prev = curr;
        }
        curr = next;
    }
    spin_unlock(&sleep_queue.lock);

    if (curthread) {
        curthread->t_ticks++;
        if (curthread->t_ticks >= 10) {
            curthread->t_ticks = 0;
            curthread->t_need_resched = true;
        }
    }
}

void scheduler_init(void) {
    spin_lock_init(&ready_queue.lock);
    spin_lock_init(&sleep_queue.lock);

    curthread = arch_init_first_thread();

    g_intc->register_handler(0x40, arch_timer_handler, NULL);
    g_intc->start_timer(1, 0x40);
}

subsys_initcall(scheduler_init, PRIO_LAST);