#include <kernel/proc.h>
#include <kernel/cpu.h>
#include <kernel/printf.h>
#include <kernel/sched.h>
#include <kernel/lock.h>
#include <kernel/kmem.h>
#include <kernel/init.h>

#include <string.h>

struct thread *curthread = NULL;

struct {
    spinlock_t lock;
    struct thread *head;
    struct thread *tail;
} ready_queue;

void thread_switch_to(struct thread *next) {
    struct thread *prev = curthread;

    if (prev == next) return;

    if (prev && prev->t_state == THREAD_RUNNING) {
        prev->t_state = THREAD_READY;
    }
    
    next->t_state = THREAD_RUNNING;
    curthread = next;

    context_switch(prev, next);
}

void sched_enqueue(struct thread *t) {
    // if (t->t_tid == 0) return;

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
        if (!ready_queue.head) {
            ready_queue.tail = NULL;
        }
        t->t_sched_next = NULL;
    }
    spin_unlock(&ready_queue.lock);
    return t;
}

void schedule(void) {
    struct thread *prev = curthread;
    struct thread *next = sched_dequeue();

    if (!next) {
        if (prev->t_state == THREAD_RUNNING) return;
        // TODO: SLEEP
        return; 
    }

    if (prev->t_state == THREAD_RUNNING) {
        sched_enqueue(prev);
    }

    thread_switch_to(next);
}

void scheduler_init(void) {
    struct proc *p0 = proc_create(0);
    struct thread *t0 = kmalloc(sizeof(struct thread));
    memset(t0, 0, sizeof(struct thread));

    t0->t_tid = 0;
    t0->t_proc = p0;
    t0->t_state = THREAD_RUNNING;
    t0->t_kstack = (void *)(get_stack_pointer() & ~(KSTACK_SIZE - 1));

    arch_thread_setup(t0, NULL);
    t0->t_context = NULL;

    curthread = t0;
    
    memset(&ready_queue, 0, sizeof(ready_queue));
    spin_lock_init(&ready_queue.lock);
}

subsys_initcall(scheduler_init, PRIO_LAST);