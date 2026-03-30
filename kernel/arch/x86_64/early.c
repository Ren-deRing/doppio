#include <stdint.h>
#include <elf.h>

#include "boot/bootinfo.h"

#include "kernel/cpu.h"
#include "kernel/init.h"

static struct cpu cpus[MAX_CPUS];

extern uint8_t __kernel_start[];

void fpu_init(void) {
    uintptr_t cr0, cr4;

    asm volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1 << 2); // Emulation Off
    cr0 |= (1 << 1);  // Monitor Coprocessor On
    cr0 |= (1 << 5);  // Numeric Error On
    asm volatile ("mov %0, %%cr0" : : "r"(cr0));

    asm volatile ("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1 << 9);  // OSFXSR: FXSAVE/FXRSTOR On
    cr4 |= (1 << 10); // OSXMMEXCPT: Unmasked SSE Exception On
    asm volatile ("mov %0, %%cr4" : : "r"(cr4));

    asm volatile ("fninit");
}

void bsp_init(void) {
    int id = 0; 
    struct cpu *c = &cpus[id];

    c->self = c;
    c->id = id;

    // TODO
}

void early_init(void) {
    fpu_init();
    bsp_init();
}

void do_nothing(void) {}   // TODO
arch_initcall(do_nothing); // why???? if remove: boom fuck please don't touch until i patch.