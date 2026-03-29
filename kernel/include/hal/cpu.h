#pragma once

#include <stdint.h>

typedef uint64_t cpu_status_t;

#define irq_save(flags)      do { flags = hal_cpu_irq_save(); } while(0)
#define irq_restore(flags)   hal_cpu_irq_restore(flags)

void hal_cpu_init(void);
void hal_cpu_irq_save(cpu_status_t *flags);
void hal_cpu_irq_restore(cpu_status_t flags);
void hal_cpu_halt(void);