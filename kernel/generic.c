#include "boot/bootinfo.h"

#include "kernel/init.h"
#include "kernel/cpu.h"
#include "kernel/printf.h"
#include "kernel/mmu.h"
#include "kernel/kmem.h"
#include "kernel/intc.h"
#include "kernel/clock.h"

#include "string.h"

bool ap_release = false;

void do_initcalls(void) {
    for (initcall_t* call = __initcall_start; call < __initcall_end; call++) {
        if (!call || !*call) continue;
        
        (*call)();
    }
}

void do_ap_initcalls(void) {
    for (initcall_t* call = __ap_initcall_start; call < __ap_initcall_end; call++) {
        if (!call || !*call) continue;
        
        (*call)();
    }
}

void generic_entry() {
    early_init(g_boot_info.smp.bsp_hw_id);
    do_initcalls();

    ap_release = true;
    __sync_synchronize();

    arch_irq_enable();

    for (;;) arch_halt();
}

void ap_entry(CoreInfo* info) {
    arch_irq_disable();

    ap_early_init(info->hw_id);
    
    while (!ap_release) {
        arch_pause();
    }

    __sync_synchronize();
    do_ap_initcalls();

    arch_irq_enable();

    // atomic_inc(&initialized_cores);

    // if (info->hw_id == 1) {
    //     volatile int a = 10;
    //     volatile int b = 0;
    //     volatile int c = a / b;
    // }

    for (;;) arch_halt();
}