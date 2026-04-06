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
#include <kernel/lock.h>

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

mutex_t test_mutex;
volatile int shared_counter = 0;

void mutex_test_thread(void* arg) {
    const char* name = (const char*)arg;
    
    for (int i = 0; i < 5; i++) {
        dprintf("\n[%s] Locking mutex...\n", name);
        mutex_lock(&test_mutex);
        
        dprintf("[%s] Counter: %d\n", name, ++shared_counter);
        dprintf("[%s] Anyway, this is critical code\n", name);

        thread_sleep(1000);
        
        dprintf("[%s] Unlocking mutex...\n", name);
        mutex_unlock(&test_mutex);
        
        thread_yield(); 
    }
    
    dprintf("\n[%s] Test Finished!\n", name);
    for(;;) arch_halt();
}

void test_thread(void) {
    double f = 0.0;
    while(1) {
        f += 0.1;
        dprintf("A(%.1f) ", f);

        if (f > 10.0) f = 0.0;
        thread_sleep(500);
    }
}

void generic_entry() {
    arch_irq_disable();

    early_init(g_boot_info.smp.bsp_hw_id);
    do_initcalls();

    ap_release = true;
    __sync_synchronize();

    uint32_t* fb_ptr = (uint32_t*)g_boot_info.fb.fb_addr;
    uint32_t width = g_boot_info.fb.width;
    uint32_t height = g_boot_info.fb.height;
    uint32_t pixels_per_line = g_boot_info.fb.pitch / 4;

    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            fb_ptr[y * pixels_per_line + x] = 0xF3E0ED;
        }
    }

    for (uint32_t y = height/2 - 50; y < height/2 + 50; y++) {
        for (uint32_t x = width/2 - 50; x < width/2 + 50; x++) {
            fb_ptr[y * pixels_per_line + x] = 0xFFFFFF;
        }
    }

    // struct thread* tA = thread_create(curthread->t_proc, 1, test_thread, NULL);
    // sched_enqueue(tA);

    mutex_init(&test_mutex);
    shared_counter = 0;

    struct thread* t1 = thread_create(curthread->t_proc, 1, (void*)mutex_test_thread, "Mika_1");
    struct thread* t2 = thread_create(curthread->t_proc, 1, (void*)mutex_test_thread, "Mika_2");

    sched_enqueue(t1);
    sched_enqueue(t2);

    arch_irq_enable();

    while(1) { // idle
        arch_irq_enable();
        arch_halt();
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