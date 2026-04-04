#include <stdint.h>
#include <elf.h>

#include "x86.h"

#include <boot/bootinfo.h>

#include <kernel/kmem.h>
#include <kernel/cpu.h>
#include <kernel/init.h>
#include <kernel/printf.h>

#define MSR_GS_BASE 0xC0000101

#define SERIAL_DEVICE 0x3F8

struct cpu cpus[MAX_CPUS];

extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t val);

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

void cpu_init(uint32_t logic_id, uint32_t hw_id) {
    struct cpu *c = &cpus[logic_id];

    c->self = c;
    c->id = logic_id;
    c->hw_id = hw_id;

    for (int i = 0; i < KMEM_NUM_CLASSES; i++) {
        c->magazines[i] = NULL; 
    }

    wrmsr(MSR_GS_BASE, (uintptr_t)c);
}

static char* log_write(const char* buffer, void* user, int size) {
    (void)user;

    for (int i = 0; i < size; ++i) {
        outb(SERIAL_DEVICE, buffer[i]);
    }
    return (char*)buffer;
}

static void log_init(void) {
	outb(SERIAL_DEVICE + 3, 0x03);
    set_output_sink(&log_write);
}

void early_init(uint32_t hw_id) {
    fpu_init();
    cpu_init(0, hw_id);
    log_init();
}

static volatile int next_id = 1;

void ap_early_init(uint32_t hw_id) {
    int id = __sync_fetch_and_add(&next_id, 1); 

    fpu_init();
    cpu_init(id, hw_id);
}

/* Fixed: do_nothing(void)'s former kingdom... RIP 2026-2026 */