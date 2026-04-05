#include <kernel/cpu.h>
#include <kernel/proc.h>
#include <kernel/kmem.h>

#include <uapi/types.h>

#include "x86.h"

#include <string.h>

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
    if (t->t_arch_data) memset(t->t_arch_data, 0, g_xsave_size);

    uint32_t *mxcsr_ptr = (uint32_t *)((uint8_t *)t->t_arch_data + 24);
    *mxcsr_ptr = 0x1F80;

    uintptr_t stack_top = (uintptr_t)t->t_kstack + KSTACK_SIZE;
    
    uint64_t *sp = (uint64_t *)(stack_top & ~0xFULL);

    *(--sp) = 0; 

    *(--sp) = (uint64_t)entry;

    for (int i = 0; i < 6; i++) {
        *(--sp) = 0;
    }

    t->t_context = (void *)sp;
}