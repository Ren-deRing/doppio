#pragma once

#include <stdint.h>
#include <stdbool.h>

#define MAX_CPUS 64
#define MAX_ISO 256

#define KMEM_NUM_CLASSES 16

struct kmem_magazine;
typedef struct kmem_magazine kmem_magazine_t; // TODO

struct cpu {
    struct cpu *self;
//    struct task *current;   // running task
//    struct task *idle;      // idle task
    uint32_t id;
    uint32_t hw_id;

    kmem_magazine_t* magazines[KMEM_NUM_CLASSES];
};

typedef uint64_t cpu_status_t;

struct registers;
typedef void (*handler_t)(struct registers *regs, void *data);

extern struct cpu cpus[MAX_CPUS];

cpu_status_t arch_irq_save(void);
void arch_irq_restore(cpu_status_t flags);
void arch_halt(void);
void arch_pause(void);
void arch_irq_disable(void);
void arch_irq_enable(void);
struct cpu* get_this_core(void);

#define this_core          get_this_core()
#define irq_save(flags)    do { flags = arch_irq_save(); } while(0)
#define irq_restore(flags) arch_irq_restore(flags)
#define irq_enable(void)   arch_irq_enable();
#define irq_disable(void)  arch_irq_disable();