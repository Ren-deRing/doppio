#pragma once

#include <stdint.h>

void wrmsr(uint32_t msr, uint64_t val);
uint64_t rdmsr(uint32_t msr);