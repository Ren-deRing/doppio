#include <string.h>
#include <stdint.h>

#include <kernel/init.h>
#include <kernel/cpu.h>

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
    gdt_entry_t entries[6]; // 0:NULL, 1:K-Code, 2:K-Data, 3:U-Code, 4:U-Data, 5:U-Data(Spare)
    uint16_t tss_limit_low;
    uint16_t tss_base_low;
    uint8_t  tss_base_mid;
    uint8_t  tss_access;
    uint8_t  tss_limit_high;
    uint8_t  tss_base_high;
    uint32_t tss_base_highest;
    uint32_t tss_reserved;
} __attribute__((packed)) gdt_table_t;

typedef struct {
    gdt_table_t   table;
    gdt_pointer_t pointer;
    tss_entry_t   tss;
    uint8_t df_stack[8192] __attribute__((aligned(16)));
} __attribute__((packed)) GDT;

GDT gdt[MAX_CPUS];

void update_tss_rsp0(uintptr_t kstack_top) {
    struct cpu* c = get_this_core();
    gdt[c->id].tss.rsp[0] = kstack_top;
}

static void set_gdt_entry(gdt_entry_t* entry, uint8_t access, uint8_t flags) {
    entry->limit_low = 0xFFFF;
    entry->base_low = 0;
    entry->base_mid = 0;
    entry->access = access;
    entry->flags = flags;
    entry->base_high = 0;
}

static inline void gdt_load(gdt_pointer_t* ptr) {
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
        "mov $0x30, %%ax\n"
        "ltr %%ax\n"
        : : "m"(*ptr) : "rax", "memory"
    );
}

void gdt_install(void) {
    for (int i = 0; i < MAX_CPUS; i++) {
        // Index 1: Kernel Code
        set_gdt_entry(&gdt[i].table.entries[1], 0x9A, (1 << 5) | (1 << 7) | 0x0F);
        // Index 2: Kernel Data
        set_gdt_entry(&gdt[i].table.entries[2], 0x92, (1 << 5) | (1 << 7) | 0x0F);
        // Index 3: User Data
        set_gdt_entry(&gdt[i].table.entries[3], 0xF2, (1 << 5) | (1 << 7) | 0x0F);
        // Index 4: User Code
        set_gdt_entry(&gdt[i].table.entries[4], 0xFA, (1 << 5) | (1 << 7) | 0x0F);
        set_gdt_entry(&gdt[i].table.entries[5], 0xF2, 0xCF);

        // TSS
        uintptr_t tss_addr = (uintptr_t)&gdt[i].tss;
        gdt[i].table.tss_limit_low = sizeof(tss_entry_t) - 1;
        gdt[i].table.tss_base_low  = tss_addr & 0xFFFF;
        gdt[i].table.tss_base_mid  = (tss_addr >> 16) & 0xFF;
        gdt[i].table.tss_base_high = (tss_addr >> 24) & 0xFF;
        gdt[i].table.tss_base_highest = (tss_addr >> 32) & 0xFFFFFFFF;
        gdt[i].table.tss_access = 0x89; // Present, Type: 64-bit TSS (Available)

        // IST
        gdt[i].tss.ist[0] = (uintptr_t)gdt[i].df_stack + sizeof(gdt[i].df_stack);

        // LGDT
        gdt[i].pointer.limit = sizeof(gdt_table_t) - 1;
        gdt[i].pointer.base  = (uintptr_t)&gdt[i].table;
    }

    // BSP 적용
    gdt_load(&gdt[0].pointer);
}

void ap_gdt_install(void) {
    struct cpu* c = get_this_core();
    gdt_load(&gdt[c->id].pointer);
}

arch_initcall(gdt_install, PRIO_FIRST);

ap_arch_initcall(ap_gdt_install, PRIO_FIRST);