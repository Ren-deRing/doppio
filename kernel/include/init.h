#pragma once

typedef void (*initcall_t)(void);

#define __initcall(fn, level) \
    static initcall_t __initcall_##fn##level __attribute__((used, \
    section(".initcall." #level), aligned(8))) = fn

#define early_initcall(fn)     __initcall(fn, 0) // early
#define arch_initcall(fn)      __initcall(fn, 1) // L0
#define mem_initcall(fn)       __initcall(fn, 2) // L1
#define dev_initcall(fn)       __initcall(fn, 3) // L2
#define subsys_initcall(fn)    __initcall(fn, 4) // L3
#define late_initcall(fn)      __initcall(fn, 5) // L4