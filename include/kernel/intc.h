#pragma once

#include <stdint.h>

struct interrupt_controller {
    const char* name;
    
    int (*get_cpu_count)(void);
    uint32_t (*get_cpu_id)(int index);

    void (*eoi)(void);
    void (*send_ipi)(uint32_t target_cpu, uint8_t vector);
    
    // TODO: MSI
};

extern struct interrupt_controller* g_intc;