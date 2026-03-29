#include <stdint.h>
#include <elf.h>

#include "boot/bootinfo.h"

#include "init.h"

extern Elf64_Rela __rela_start[];
extern Elf64_Rela __rela_end[];
extern uint8_t __kernel_start[];

void do_rela() {
    uintptr_t delta = g_boot_info.kernel.virt_base - (uintptr_t)__kernel_start;

    if (delta == 0) return;

    for (Elf64_Rela *rela = __rela_start; rela < __rela_end; rela++) {
        if (ELF64_R_TYPE(rela->r_info) == R_X86_64_RELATIVE) {
            uintptr_t *addr = (uintptr_t *)(rela->r_offset + delta);
            *addr = (uintptr_t)(rela->r_addend + delta);
        }
    }
}

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

early_initcall(do_rela);
early_initcall(fpu_init);