#pragma once

#include <stdint.h>

#define SYS_test 0
#define SYS_write 1
#define SYS_brk 2

int64_t syscall0(uint64_t num);
int64_t syscall1(uint64_t num, uint64_t a1);
int64_t syscall3(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3);