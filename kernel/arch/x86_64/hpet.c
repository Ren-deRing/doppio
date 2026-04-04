#include "intc/acpi/hpet.h"
#include "intc/acpi/acpi.h"

#include "kernel/mmu.h"
#include "kernel/init.h"
#include "kernel/clock.h"

static volatile uint64_t* hpet_regs;
static uint32_t hpet_period_fs;

static inline uint64_t hpet_read(uint32_t reg) {
    return hpet_regs[reg / 8];
}

static inline void hpet_write(uint32_t reg, uint64_t val) {
    hpet_regs[reg / 8] = val;
}

static uint64_t hpet_read_raw(void) {
    return hpet_read(HPET_MAIN_COUNT);
}

static uint64_t hpet_ticks_to_ns(uint64_t ticks) {
    return (ticks * hpet_period_fs) / 1000000ULL;
}

static struct clock_source hpet_src = {
    .name = "hpet",
    .read = hpet_read_raw,
    .ticks_to_ns = hpet_ticks_to_ns,
    .rating = 100
};

void hpet_init() {
    mmu_map(mmu_get_kernel_map(), acpi_info.hpet_addr, acpi_info.hpet_paddr,
            MMU_FLAGS_READ | MMU_FLAGS_WRITE | MMU_FLAGS_NOCACHE);
    
    hpet_regs = (volatile uint64_t*)acpi_info.hpet_addr;

    uint64_t caps = hpet_read(HPET_GEN_CAPS);
    hpet_period_fs = (uint32_t)(caps >> 32);

    uint64_t conf = hpet_read(HPET_GEN_CONF);
    conf |= 0x01; 
    hpet_write(HPET_GEN_CONF, conf);

    clock_register_source(&hpet_src);
}

dev_initcall(hpet_init, PRIO_FIRST);