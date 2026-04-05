#include <kernel/cpu.h>
#include <kernel/proc.h>
#include <kernel/kmem.h>
#include <kernel/sched.h>
#include <kernel/intc.h>

#include <uapi/types.h>

#include "x86.h"

#include <string.h>

volatile uint64_t g_ticks = 0;

uint8_t g_fpu_preset[4096] __attribute__((aligned(64)));
size_t g_xsave_size = 512;

cpu_status_t arch_irq_save(void) {
    cpu_status_t flags;
    asm volatile ("pushfq; pop %0; cli" : "=rm"(flags) :: "memory");
    return flags;
}

void arch_irq_restore(cpu_status_t flags) {
    asm volatile ("push %0; popfq" : : "rm"(flags) : "memory", "cc");
}

void arch_irq_disable(void) {
    asm volatile ("cli");
}

void arch_irq_enable(void) {
    asm volatile ("sti");
}

void arch_halt(void) {
    asm volatile ("hlt");
}

void arch_pause(void) {
    asm volatile ("pause");
}

struct cpu* get_this_core(void) {
    struct cpu* ptr;
    asm volatile ("mov %%gs:0, %0" : "=r"(ptr));
    return ptr;
}

// x86

void arch_thread_setup(struct thread *t, void (*entry)(void)) {
    t->t_arch_data = kmalloc_aligned(g_xsave_size, 64);
    
    if (t->t_arch_data) {
        memcpy(t->t_arch_data, g_fpu_preset, g_xsave_size);
    }

    uint64_t *sp = (uint64_t *)((uintptr_t)t->t_kstack + KSTACK_SIZE);

    *(--sp) = 0;

    *(--sp) = 0x10;              // SS
    *(--sp) = (uintptr_t)sp + 8; // RSP
    *(--sp) = 0x202;             // RFLAGS
    *(--sp) = 0x08;              // CS
    *(--sp) = (uintptr_t)entry;  // RIP

    *(--sp) = 0; // Error Code
    *(--sp) = 0; // Vector

    for (int i = 0; i < 15; i++) {
        *(--sp) = 0;
    }

    t->t_context = sp;
}

uint64_t arch_get_system_ticks(void) {
    return g_ticks;
}

struct thread* arch_get_idle_thread(void) {
    return get_this_core()->idle;
}

void arch_set_current_thread(struct thread *t) {
    struct cpu *c = get_this_core();
    c->current = t;

    asm volatile ("mov %0, %%gs:8" : : "r"(t) : "memory");
}

void arch_request_resched(void) {
    asm volatile ("int $0x40");
}

struct thread* arch_init_first_thread(void) {
    struct proc *p0 = proc_create(0);
    struct thread *t0 = kmalloc(sizeof(struct thread));
    memset(t0, 0, sizeof(struct thread));

    t0->t_tid = 0;
    t0->t_proc = p0;
    t0->t_state = THREAD_RUNNING;
    
    uint64_t current_sp;
    asm volatile ("mov %%rsp, %0" : "=r"(current_sp));
    t0->t_kstack = (void *)(current_sp & ~(KSTACK_SIZE - 1));

    t0->t_arch_data = kmalloc(g_xsave_size);
    memset(t0->t_arch_data, 0, g_xsave_size);

    struct cpu *c = get_this_core();
    c->idle = t0;
    c->current = t0;
    
    asm volatile ("mov %0, %%gs:8" : : "r"(t0) : "memory");

    return t0;
}

void arch_timer_handler(struct registers *regs, void *data) {
    (void)regs; 
    (void)data;

    g_intc->eoi();

    g_ticks++;
    
    sched_tick(); 
}