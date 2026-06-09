#ifndef SEKURA_PCI_H
#define SEKURA_PCI_H

#include <stdint.h>

typedef struct {
    uint8_t bus;
    uint8_t slot;
    uint8_t function;

    uint16_t vendor_id;
    uint16_t device_id;

    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
} PCI_Device;

uint32_t pci_read(
    uint8_t bus,
    uint8_t slot,
    uint8_t function,
    uint8_t offset
);

uint16_t pci_read16(
    uint8_t bus,
    uint8_t slot,
    uint8_t function,
    uint8_t offset
);

uint8_t pci_read8(
    uint8_t bus,
    uint8_t slot,
    uint8_t function,
    uint8_t offset
);

int pci_find_class(
    uint8_t class_code,
    uint8_t subclass,
    uint8_t prog_if,
    PCI_Device* out
);

uint64_t pci_get_bar0(
    PCI_Device* dev
);

#endif