#include <sys/syscall.h>
#include <stdarg.h>
#include <stdint.h>

int64_t syscall0(uint64_t num) {
    int64_t ret;
    asm volatile ("syscall" : "=a"(ret) : "a"(num) : "rcx", "r11", "memory");
    return ret;
}

int64_t syscall1(uint64_t num, uint64_t a1) {
    int64_t ret;
    asm volatile ("syscall" : "=a"(ret) : "a"(num), "D"(a1) : "rcx", "r11", "memory");
    return ret;
}

int64_t syscall3(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3) {
    int64_t ret;
    asm volatile (
        "syscall"
        : "=a"(ret)
        : "a"(num), "D"(a1), "S"(a2), "d"(a3)
        : "rcx", "r11", "memory"
    );
    return ret;
}