#include <stdint.h>
#include <sys/syscall.h>

void _start() {
    // syscall 0: test
    syscall1(0, 0xCACECAFE);

    const char *msg = "Hello from userspace!\n";
    uint64_t len = 0;
    while (msg[len]) len++;

    // syscall 1: sys_write(fd=1, buf=msg, count=len)
    syscall3(1, 1, (uint64_t)msg, len);

    while (1);
}