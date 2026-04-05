#include <boot/bootinfo.h>

#include <kernel/init.h>
#include <kernel/cpu.h>
#include <kernel/printf.h>
#include <kernel/mmu.h>
#include <kernel/kmem.h>
#include <kernel/intc.h>
#include <kernel/clock.h>
#include <kernel/proc.h>
#include <kernel/sched.h>

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

void test_entry(void) {
    double f = 0.0;
    while(1) {
        f += 0.1;
        dprintf("A(%.1f) ", f);
        
        if (f > 10.0) f = 0.0;
        schedule();
    }
}

#include "arch/x86_64/x86.h"

void generic_entry() {
    early_init(g_boot_info.smp.bsp_hw_id);
    do_initcalls();

    dprintf("x86: XSAVE/AVX enabled. Context size: %u bytes\n", g_xsave_size);

    ap_release = true;
    __sync_synchronize();

    uint32_t* fb_ptr = (uint32_t*)g_boot_info.fb.fb_addr;
    uint32_t width = g_boot_info.fb.width;
    uint32_t height = g_boot_info.fb.height;
    uint32_t pixels_per_line = g_boot_info.fb.pitch / 4;

    uint32_t test_color = 0x0077BE; 

    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            fb_ptr[y * pixels_per_line + x] = test_color;
        }
    }

    for (uint32_t y = height/2 - 50; y < height/2 + 50; y++) {
        for (uint32_t x = width/2 - 50; x < width/2 + 50; x++) {
            fb_ptr[y * pixels_per_line + x] = 0xFFFFFF;
        }
    }

    thread_create(curthread->t_proc, 1, test_entry);
    sched_enqueue(curthread);

    arch_irq_enable();

    double g = 100.0;
    
    while(1) {
        g += 0.5;
        dprintf("B(%.1f) ", g);

        if (g > 110.0) g = 100.0;
        schedule();
    }
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