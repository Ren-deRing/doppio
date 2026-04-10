#include <kernel/cpu.h>
#include <kernel/proc.h>
#include <kernel/kmem.h>
#include <kernel/sched.h>
#include <kernel/intc.h>
#include <kernel/mmu.h>

#include <uapi/types.h>
#include <uapi/errno.h>

#include "x86.h"

#include <string.h>

volatile uint64_t g_ticks = 0;

uint8_t g_fpu_preset[4096] __attribute__((aligned(64)));
size_t g_xsave_size = 512;

cpu_status_t arch_irq_save(void) {
    cpu_status_t flags;
    asm volatile ("pushfq; pop %0; cli" : "=rm"(flags) :: "memory");
    return flags;
}

void arch_irq_restore(cpu_status_t flags) {
    asm volatile ("push %0; popfq" : : "rm"(flags) : "memory", "cc");
}

void arch_irq_disable(void) {
    asm volatile ("cli");
}

void arch_irq_enable(void) {
    asm volatile ("sti");
}

void arch_halt(void) {
    asm volatile ("hlt");
}

void arch_pause(void) {
    asm volatile ("pause");
}

struct cpu* get_this_core(void) {
    struct cpu* ptr;
    asm volatile ("mov %%gs:0, %0" : "=r"(ptr));
    return ptr;
}

// x86

void arch_thread_setup(struct thread *t, void (*entry)(void)) {
    uint64_t *sp = (uint64_t *)((uintptr_t)t->t_kstack + KSTACK_SIZE);

    bool is_user = (t->t_flags & THREAD_FLAG_USER);

    if (is_user) {
        *(--sp) = 0x1B; // User
        *(--sp) = t->t_user_stack_top;
    } else {
        *(--sp) = 0x10; // Kernel
        *(--sp) = (uintptr_t)sp + 8;
    }
    *(--sp) = 0x202;               // RFLAGS
    if (is_user) {
        *(--sp) = 0x23; // User
    } else {
        *(--sp) = 0x08; // Kernel
    }
    *(--sp) = (uintptr_t)entry;    // RIP

    *(--sp) = 0; // Error Code
    *(--sp) = 0; // Vector

    *(--sp) = 0;                   // RAX
    *(--sp) = 0;                   // RBX
    *(--sp) = 0;                   // RCX
    *(--sp) = 0;                   // RDX
    *(--sp) = 0;                   // RSI
    *(--sp) = (uintptr_t)t->t_arg; // RDI
    *(--sp) = 0;                   // RBP
    *(--sp) = 0;                   // R8
    *(--sp) = 0;                   // R9
    *(--sp) = 0;                   // R10
    *(--sp) = 0;                   // R11
    *(--sp) = 0;                   // R12
    *(--sp) = 0;                   // R13
    *(--sp) = 0;                   // R14
    *(--sp) = 0;                   // R15

    t->t_context = sp;
}

int arch_proc_init(struct proc *p) {
    struct arch_proc *ap = kmalloc(sizeof(struct arch_proc));
    if (!ap) return -ENOMEM;

    memset(ap, 0, sizeof(struct arch_proc));
    
    p->p_arch = ap;
    
    return 0;
}

int arch_thread_init(struct thread *t, void (*entry)(void)) {
    t->t_arch_data = kmalloc_aligned(g_xsave_size, 64);
    if (!t->t_arch_data) {
        return -ENOMEM;
    }

    memset(t->t_arch_data, 0, g_xsave_size);
    memcpy(t->t_arch_data, g_fpu_preset, g_xsave_size);
    arch_thread_setup(t, entry);

    return 0;
}

void arch_proc_destroy(struct proc *p) {
    if (p->p_arch) {
        if (p->p_arch->io_bitmap) {
            kfree(p->p_arch->io_bitmap);
        }
        kfree(p->p_arch);
        p->p_arch = NULL;
    }
}

void arch_thread_destroy(struct thread *t) {
    if (t->t_arch_data) {
        kfree(t->t_arch_data);
        t->t_arch_data = NULL;
    }
}

extern void update_tss_rsp0(uintptr_t kstack_top);

void arch_switch_context_hardware(struct thread *next) {
    if (!next || !next->t_proc) return;

    if (next->t_proc->p_vm_map) {
        uintptr_t new_cr3 = v2p(next->t_proc->p_vm_map);
        
        uintptr_t old_cr3;
        asm volatile("mov %%cr3, %0" : "=r"(old_cr3));
        
        if (old_cr3 != new_cr3) {
            asm volatile("mov %0, %%cr3" : : "r"(new_cr3) : "memory");
        }
    }

    uintptr_t kstack_top = (uintptr_t)next->t_kstack + KSTACK_SIZE;
    update_tss_rsp0(kstack_top); 

    struct cpu *c = get_this_core();
    struct x86_64_cpu_data *ad = (struct x86_64_cpu_data *)c->arch_cpu_data;
    ad->kernel_stack = kstack_top;
}

uint64_t arch_get_system_ticks(void) {
    return g_ticks;
}

struct thread* arch_get_idle_thread(void) {
    return get_this_core()->idle;
}

void arch_set_current_thread(struct thread *t) {
    struct cpu *c = get_this_core();
    c->current = t;

    asm volatile ("mov %0, %%gs:8" : : "r"(t) : "memory");
}

void arch_request_resched(void) {
    asm volatile ("int $0x40");
}

struct thread* arch_init_first_thread(void) {
    struct proc *p0 = proc_create(0);
    struct thread *t0 = kmalloc_aligned(sizeof(struct thread), 64);
    memset(t0, 0, sizeof(struct thread));

    t0->t_tid = 0;
    t0->t_proc = p0;
    t0->t_state = THREAD_RUNNING;
    
    uint64_t current_sp;
    asm volatile ("mov %%rsp, %0" : "=r"(current_sp));
    t0->t_kstack = (void *)(current_sp & ~(KSTACK_SIZE - 1));

    t0->t_arch_data = kmalloc(g_xsave_size);
    memset(t0->t_arch_data, 0, g_xsave_size);

    struct cpu *c = get_this_core();
    c->idle = t0;
    c->current = t0;
    
    asm volatile ("mov %0, %%gs:8" : : "r"(t0) : "memory");

    return t0;
}

void arch_timer_handler(struct registers *regs, void *data) {
    (void)regs; 
    (void)data;

    g_intc->eoi();

    g_ticks++;
    
    sched_tick(); 
}