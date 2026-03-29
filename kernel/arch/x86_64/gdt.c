#include <string.h>
#include <stdint.h>

#include "init.h"

typedef struct tss_entry {
	uint32_t reserved_0;
	uint64_t rsp[3];
	uint64_t reserved_1;
	uint64_t ist[7];
	uint64_t reserved_2;
	uint16_t reserved_3;
	uint16_t iomap_offset;
} __attribute__ ((packed)) tss_entry_t;

typedef struct {
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_mid;
	uint8_t access;
	uint8_t flags;
	uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct {
	uint32_t base_highest;
	uint32_t reserved0;
} __attribute__((packed)) gdt_entry_high_t;

typedef struct {
	uint16_t limit;
	uintptr_t base;
} __attribute__((packed)) gdt_pointer_t;

typedef struct {
    gdt_entry_t entries[7];          // 0:NULL, 1:Kernel Code, 2:Kernel Data, 3:User Data, 4:User Code, 5:User Data, 6:TSS(Low)
    gdt_entry_high_t tss_extra;      // TSS(High)
    gdt_pointer_t pointer;           // Pointer for LGDT
    tss_entry_t tss;                 // TSS
    uint8_t df_stack[4096] __attribute__((aligned(16)));
} __attribute__((packed)) __attribute__((aligned(0x10))) GDT;

GDT gdt[32];

static void set_gdt_entry(gdt_entry_t* entry, uint8_t access, uint8_t flags) {
    entry->limit_low = 0xFFFF;
    entry->base_low = 0;
    entry->base_mid = 0;
    entry->access = access;
    entry->flags = flags;
    entry->base_high = 0;
}

void gdt_install(void) {
    for (int i = 0; i < 32; i++) {
        // Index 1: Kernel Code
        set_gdt_entry(&gdt[i].entries[1], 0x9A, (1 << 5) | (1 << 7) | 0x0F);
        // Index 2: Kernel Data
        set_gdt_entry(&gdt[i].entries[2], 0x92, (1 << 5) | (1 << 7) | 0x0F);
        // Index 3: User Data
        set_gdt_entry(&gdt[i].entries[3], 0xF2, (1 << 5) | (1 << 7) | 0x0F);
        // Index 4: User Code
        set_gdt_entry(&gdt[i].entries[4], 0xFA, (1 << 5) | (1 << 7) | 0x0F);

        // TSS
        uintptr_t tss_offset = (uintptr_t)&gdt[i].tss;
        gdt[i].entries[6].limit_low = sizeof(tss_entry_t) - 1;
        gdt[i].entries[6].base_low = tss_offset & 0xFFFF;
        gdt[i].entries[6].base_mid = (tss_offset >> 16) & 0xFF;
        gdt[i].entries[6].base_high = (tss_offset >> 24) & 0xFF;
        gdt[i].entries[6].access = 0x89; // Present, TSS 타입
        gdt[i].tss_extra.base_highest = (tss_offset >> 32) & 0xFFFFFFFF;

        // IST
        gdt[i].tss.ist[0] = (uintptr_t)gdt[i].df_stack + sizeof(gdt[i].df_stack);

        // LGDT
        gdt[i].pointer.limit = sizeof(gdt[i].entries) + sizeof(gdt[i].tss_extra) - 1;
        gdt[i].pointer.base = (uintptr_t)&gdt[i].entries;
    }

    // BSP 적용
    asm volatile (
        "lgdt %0\n"
        "push $0x08\n"
        "lea 1f(%%rip), %%rax\n"
        "push %%rax\n"
        "lretq\n"
        "1:\n"
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%ss\n"
        "mov $0x30, %%ax\n"        // TSS Index
        "ltr %%ax\n"
        : : "m"(gdt[0].pointer) : "rax", "memory"
    );
}

arch_initcall(gdt_install);