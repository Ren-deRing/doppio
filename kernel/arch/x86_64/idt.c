#include "kernel/init.h"
#include "kernel/cpu.h"
#include "kernel/printf.h"

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

void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags, uint8_t ist) {
    uintptr_t addr = (uintptr_t)isr;
    idt[vector].isr_low    = addr & 0xFFFF;
    idt[vector].kernel_cs  = 0x08;       // GDT CS
    idt[vector].ist        = ist & 0x07; // 하위 3비트
    idt[vector].attributes = flags;
    idt[vector].isr_mid    = (addr >> 16) & 0xFFFF;
    idt[vector].isr_high   = (addr >> 32) & 0xFFFFFFFF;
    idt[vector].reserved   = 0;
}

void idt_install(void) {
    for (int i = 0; i < 48; i++) {
        uint8_t ist_index = (i == 8) ? 1 : 0; 
        idt_set_descriptor(i, isr_stub_table[i], 0x8E, ist_index);
    }

    idt[8].ist = 1; // TODO: NOT TESTED

    idtr.limit = sizeof(idt) - 1;
    idtr.base  = (uintptr_t)&idt;
    
    asm volatile ("lidt %0" : : "m"(idtr));
}

void ap_idt_install(void) {
	asm volatile ("lidt %0" : : "m"(idtr));
}

const char* exceptions[32] = {
    "Divide-by-zero Error",
    "Debug",                          // 1
    "Non-maskable Interrupt",         // 2
    "Breakpoint",                     // 3
    "Overflow",                       // 4
    "Bound Range Exceeded",           // 5
    "Invalid Opcode",                 // 6
    "Device Not Available",           // 7
    "Double Fault",                   // 8
    "Coprocessor Segment Overrun",    // 9 (Reserved in newer CPUs)
    "Invalid TSS",                    // 10
    "Segment Not Present",            // 11
    "Stack-Segment Fault",            // 12
    "General Protection Fault",       // 13
    "Page Fault",                     // 14
    "Reserved",                       // 15
    "x87 Floating-Point Exception",   // 16
    "Alignment Check",                // 17
    "Machine Check",                  // 18
    "SIMD Floating-Point Exception",  // 19
    "Virtualization Exception",       // 20
    "Control Protection Exception",   // 21
    "Reserved",                       // 22
    "Reserved",                       // 23
    "Reserved",                       // 24
    "Reserved",                       // 25
    "Reserved",                       // 26
    "Reserved",                       // 27
    "Hypervisor Injection Exception", // 28
    "VMM Communication Exception",    // 29
    "Security Exception",             // 30
    "Reserved"                        // 31
};

void panic(const char* description, struct registers *regs, uintptr_t address) {
    dprintf("\n[KERNEL PANIC] %s (Vector: %d)\n", description, regs->int_no);
    dprintf("Error Code: %08x\n", regs->err_code);
    dprintf("RIP: %016llx  RSP: %016llx\n", regs->rip, regs->rsp);
    dprintf("CS: %02x  SS: %02x  RFLAGS: %08x\n", regs->cs, regs->ss, regs->rflags);

    for (;;) arch_halt();
}

static void isr_handler_inner(struct registers *regs) {
    uint8_t vector = (uint8_t)regs->int_no;
    struct isr_slot *slot = &handlers[vector];

    if (slot->func) {
        slot->func(regs, slot->data);
    } else {
        if (vector < 32) {
            panic(exceptions[regs->int_no], regs, regs->rip);
        } else if (vector >= 32 && vector < 48) {
            // ack_irq(vector - 32); 
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