#include "kernel/cpu.h"

#define MSR_GS_BASE 0xC0000101

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