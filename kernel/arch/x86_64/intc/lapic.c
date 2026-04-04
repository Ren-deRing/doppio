#include "kernel/intc.h"
#include "kernel/printf.h"
#include "kernel/init.h"
#include "kernel/mmu.h"

#include "acpi/acpi.h"

struct interrupt_controller* g_intc = NULL;

#define LAPIC_ID            0x0020
#define LAPIC_VER           0x0030
#define LAPIC_TPR           0x0080
#define LAPIC_EOI           0x00B0
#define LAPIC_SIVR          0x00F0
#define LAPIC_ICR_LOW       0x0300
#define LAPIC_ICR_HIGH      0x0310
#define LAPIC_LVT_TIMER     0x0320
#define LAPIC_LVT_ERROR     0x0370

static inline void lapic_write(uint32_t reg, uint32_t val) {
    *(volatile uint32_t*)(acpi_info.lapic_addr + reg) = val;
}

static inline uint32_t lapic_read(uint32_t reg) {
    return *(volatile uint32_t*)(acpi_info.lapic_addr + reg);
}

/* IPI */
void x86_lapic_send_ipi(uint32_t target_lapic_id, uint8_t vector) {
    while (lapic_read(LAPIC_ICR_LOW) & (1 << 12)) {
        arch_pause();
    }
    dprintf("IPI Debug: Base=%p, Target=%p\n", acpi_info.lapic_addr, acpi_info.lapic_addr + LAPIC_ICR_LOW);

    lapic_write(LAPIC_ICR_HIGH, target_lapic_id << 24);
    lapic_write(LAPIC_ICR_LOW, vector); 
}

/* EOI */
void x86_lapic_eoi(void) {
    lapic_write(LAPIC_EOI, 0);
}

int x86_get_cpu_count(void) {
    return acpi_info.cpu_count;
}

void x86_lapic_init_local(void) {
    if (!acpi_info.lapic_addr) return;

    lapic_write(LAPIC_TPR, 0);

    lapic_write(LAPIC_SIVR, 0xFF | (1 << 8));
    lapic_write(LAPIC_LVT_ERROR, 1 << 16);

    dprintf("LAPIC: Core %d initialized.\n", get_this_core()->id);
}

extern void register_handler(uint8_t vector, handler_t handler, void *data);

void x86_intc_register(uint8_t vector, handler_t handler, void *data) {
    register_handler(vector, handler, data);
}

struct interrupt_controller x86_intc = {
    .name = "x86_LAPIC",
    .get_cpu_count = x86_get_cpu_count,
    .init_local = x86_lapic_init_local,
    .register_handler = x86_intc_register,
    .send_ipi = x86_lapic_send_ipi,
    .eoi = x86_lapic_eoi,
};

void lapic_controller_init(void) {
    g_intc = &x86_intc;

    bool success = mmu_map(
        mmu_get_kernel_map(), 
        acpi_info.lapic_addr, 
        acpi_info.lapic_paddr,
        MMU_FLAGS_READ | MMU_FLAGS_WRITE | MMU_FLAGS_NOCACHE
    );

    if (!success) {
        dprintf("LAPIC: Failed to map memory!\n");
        return;
    }
    
    if (g_intc->init_local) {
        g_intc->init_local();
    }
}

void ap_lapic_init(void) {
    if (g_intc && g_intc->init_local) {
        g_intc->init_local();
    }
}

dev_initcall(lapic_controller_init, PRIO_FIRST);
ap_arch_initcall(ap_lapic_init, PRIO_THIRD);