#include "boot/bootinfo.h"

#include "kernel/init.h"
#include "kernel/cpu.h"
#include "kernel/printf.h"
#include "kernel/mmu.h"
#include "kernel/kmem.h"

#include "string.h"

void do_initcalls(void) {
    for (initcall_t* call = __initcall_start; call < __initcall_end; call++) {
        if (!call || !*call) continue;
        
        (*call)();
    }
}

void generic_entry() {
    early_init();

    do_initcalls();

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
            fb_ptr[y * pixels_per_line + x] = 0xFFFFFF; // 흰색 점
        }
    }

    kmem_chaos_test();

    for (;;) arch_halt();
}