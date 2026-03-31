#include <stdint.h>

void outb(uint16_t port, uint8_t val) {
    asm volatile (
        "outb %b0, %w1"
        :
        : "a"(val), "Nd"(port)
    );
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__ ("inb %w1, %b0"
                          : "=a" (ret)
                          : "Nd" (port));
    return ret;
}