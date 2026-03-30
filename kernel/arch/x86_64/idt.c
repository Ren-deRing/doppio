#include "kernel/init.h"
#include "kernel/cpu.h"

struct registers {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;

    uint64_t int_no;
    uint64_t err_code;

    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed));

typedef void (*handler_t)(struct registers *regs, void *data);

struct isr_slot {
    handler_t func;
    void *data;
};

typedef struct {
    uint16_t isr_low;      // ISR 주소 하위 16비트
    uint16_t kernel_cs;    // CS
    uint8_t  ist;          // IST
    uint8_t  attributes;   // 권한 & 타입
    uint16_t isr_mid;      // ISR 주소 중간 16비트
    uint32_t isr_high;     // ISR 주소 상위 32비트
    uint32_t reserved;     // 예약됨
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idtr_t;

static volatile idt_entry_t idt[256];
static volatile idtr_t idtr;
static struct isr_slot handlers[256];

extern volatile void* isr_stub_table[];

void register_handler(uint8_t vector, handler_t handler, void *data) {
    handlers[vector].func = handler;
    handlers[vector].data = data;
}

void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags) {
    uintptr_t addr = (uintptr_t)isr;
    idt[vector].isr_low    = addr & 0xFFFF;
    idt[vector].kernel_cs  = 0x08; // GDT CS
    idt[vector].ist        = 0;    // #DF 는 ist index 1로 연결됨 (in idt_install)
    idt[vector].attributes = flags;
    idt[vector].isr_mid    = (addr >> 16) & 0xFFFF;
    idt[vector].isr_high   = (addr >> 32) & 0xFFFFFFFF;
    idt[vector].reserved   = 0;
}

void idt_install(void) {
    for (int i = 0; i < 48; i++) {
        idt_set_descriptor(i, isr_stub_table[i], 0x8E);
    }

    idt[8].ist = 1; // for #DF

    idtr.limit = sizeof(idt) - 1;
    idtr.base  = (uintptr_t)&idt;
    
    asm volatile ("lidt %0" : : "m"(idtr));
}

void ap_idt_install(void) {
	asm volatile ("lidt %0" : : "m"(idtr));
}

#include "boot/bootinfo.h"

static void isr_handler_inner(struct registers *regs) {
    uint8_t v = (uint8_t)regs->int_no;
    struct isr_slot *slot = &handlers[v];

    if (slot->func) {
        slot->func(regs, slot->data);
    } else {
        if (v < 32) {
            uint32_t* fb_ptr = (uint32_t*)g_boot_info.fb.fb_addr;
            uint32_t width = g_boot_info.fb.width;
            uint32_t height = g_boot_info.fb.height;
            uint32_t pixels_per_line = g_boot_info.fb.pitch / 4;

            for (uint32_t y = height/2 - 50; y < height/2 + 50; y++) {
                for (uint32_t x = width/2 - 50; x < width/2 + 50; x++) {
                    fb_ptr[y * pixels_per_line + x] = 0xFF0000;
                }
            }
            // panic("Unhandled Exception", regs, v);
            for (;;) arch_halt();
        } else if (v >= 32 && v < 48) {
            // ack_irq(v - 32); 
        }
    }

    // if (this_core->current == this_core->idle && can_switch()) {
    //     switch_next();
    // }
}

void isr_handler(struct registers *regs) {
    // bool from_user = (regs->cs & 3) != 0;

    // if (from_user && this_core->current) {
    //     this_core->current->time_switch = arch_perf_timer();
    // }

    isr_handler_inner(regs);

    // if (from_user && this_core->current) {
        // check_signals(regs);
        // update_process_times();
    // }
}

arch_initcall(idt_install, PRIO_SECOND);