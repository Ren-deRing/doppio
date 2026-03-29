#include "boot/bootinfo.h"
#include "init.h"

extern initcall_t __initcall_start;
extern initcall_t __initcall_end;

void do_initcalls(void) {
    for (initcall_t* call = &__initcall_start; call < &__initcall_end; call++) {
        if (*call) {
            (*call)();
        }
    }
}

void generic_entry() {
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

    // 중앙에 작은 사각형 하나 그려서 "나 살아있음" 표시
    for (uint32_t y = height/2 - 50; y < height/2 + 50; y++) {
        for (uint32_t x = width/2 - 50; x < width/2 + 50; x++) {
            fb_ptr[y * pixels_per_line + x] = 0xFFFFFF; // 흰색 점
        }
    }

    for (;;);
}