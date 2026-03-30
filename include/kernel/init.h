#pragma once

#include <stdint.h>
#include <stdbool.h>

#define PRIO_FIRST      A
#define PRIO_SECOND     B
#define PRIO_THIRD      C
#define PRIO_FOURTH     D
#define PRIO_FIFTH      E
#define PRIO_LAST       Z

typedef void (*initcall_t)(void);

#define __INITCALL_SELECT(_1, _2, NAME, ...) NAME

#define __initcall_with_sub(level, fn, sub) \
    static initcall_t __initcall_##level##sub##_##fn __attribute__((used, \
    section(".initcall." #level "." #sub), aligned(8))) = fn

#define __initcall_no_sub(level, fn) \
    static initcall_t __initcall_##level##5_##fn __attribute__((used, \
    section(".initcall." #level ".5"), aligned(8))) = fn

#define arch_initcall(...)   __INITCALL_SELECT(__VA_ARGS__, __initcall_with_sub, __initcall_no_sub)(0, __VA_ARGS__)
#define mem_initcall(...)    __INITCALL_SELECT(__VA_ARGS__, __initcall_with_sub, __initcall_no_sub)(1, __VA_ARGS__)
#define dev_initcall(...)    __INITCALL_SELECT(__VA_ARGS__, __initcall_with_sub, __initcall_no_sub)(2, __VA_ARGS__)
#define subsys_initcall(...) __INITCALL_SELECT(__VA_ARGS__, __initcall_with_sub, __initcall_no_sub)(3, __VA_ARGS__)
#define late_initcall(...)   __INITCALL_SELECT(__VA_ARGS__, __initcall_with_sub, __initcall_no_sub)(4, __VA_ARGS__)

extern initcall_t __initcall_start[];
extern initcall_t __initcall_end[];

void early_init(void);