#pragma once

#include "kernel/cpu.h"

#include "acpi_types.h"
#include "fadt.h"
#include "hpet.h"

struct rsdp_descriptor {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__((packed));

struct xsdt {
    struct acpi_sdt_header header;
    uint64_t tables[];
} __attribute__((packed));

struct madt {
    struct acpi_sdt_header header;
    uint32_t lapic_addr;
    uint32_t flags;
    uint8_t  entries[];
} __attribute__((packed));

struct madt_entry_header {
    uint8_t type;
    uint8_t length;
} __attribute__((packed));

struct acpi_info {
    uintptr_t lapic_addr;
    uintptr_t lapic_paddr;

    uintptr_t ioapic_addr;
    uintptr_t hpet_addr;

    struct madt* madt; 
    struct fadt* fadt;
    struct hpet* hpet;

    struct acpi_sdt_header* timer_table; // HPET

    uint32_t cpu_ids[MAX_CPUS]; // APIC ID
    int cpu_count;

    struct {
        uint32_t source;
        uint32_t target_gsi;
        uint16_t flags;
    } int_overrides[MAX_ISO];
    int override_count;
};

void acpi_init();
void madt_init(struct madt* m);

extern struct acpi_info acpi_info;