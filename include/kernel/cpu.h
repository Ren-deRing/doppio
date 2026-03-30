#pragma once

#include <stdint.h>
#include <stdbool.h>

#define MAX_CPUS 64

struct cpu {
    struct cpu *self;       
//    struct task *current;   // running task
//    struct task *idle;      // idle task
    uint32_t id;
};

typedef uint64_t cpu_status_t;

cpu_status_t arch_irq_save(void);
void arch_irq_restore(cpu_status_t flags);
void arch_halt(void);
bool arch_irq_enabled(void);
struct cpu* get_this_core(void);

#define this_core          get_this_core()
#define irq_save(flags)    do { flags = arch_irq_save(); } while(0)
#define irq_restore(flags) arch_irq_restore(flags)
#define irq_enabled()      arch_irq_enabled()