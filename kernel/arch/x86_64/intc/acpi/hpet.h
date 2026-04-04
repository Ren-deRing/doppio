#pragma once

#include "acpi_types.h"

#define HPET_GEN_CAPS    0x000
#define HPET_GEN_CONF    0x010
#define HPET_MAIN_COUNT  0x0F0

struct hpet {
    struct acpi_sdt_header header;
    uint8_t hardware_rev_id;
    uint8_t event_timer_block_id;
    uint16_t pci_vendor_id;
    struct address_structure address;
    uint8_t hpet_number;
    uint16_t minimum_tick;
    uint8_t page_protection;
} __attribute__((packed));

#define HPET_GET_COMPARATOR_COUNT(id) ((id) & 0x1F)
#define HPET_GET_COUNTER_SIZE(id)     (((id) >> 5) & 0x01)
#define HPET_GET_LEGACY_REPLACEMENT(id) (((id) >> 7) & 0x01)