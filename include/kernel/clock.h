#pragma once

#include <stdint.h>

struct clock_source {
    const char *name;
    uint64_t (*read)(void);
    uint64_t (*ticks_to_ns)(uint64_t ticks);
    uint32_t rating; // prio (HPET > PIT)
};

void clock_register_source(struct clock_source *source);
uint64_t get_uptime_ns(void);
void udelay(uint64_t us);