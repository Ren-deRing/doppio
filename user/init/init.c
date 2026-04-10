#include <stdint.h>

static inline int64_t syscall1(uint64_t num, uint64_t a1) {
    int64_t ret;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(num), "D"(a1)
        : "rcx", "r11", "memory"
    );
    return ret;
}

void _start() {
    syscall1(1, 0x1234);

    for (;;) {
        __asm__ volatile ("pause");
    }
}