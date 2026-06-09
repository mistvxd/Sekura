#include <stdint.h>

#include <sekura/devices/pci/pci.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

static inline void outl(
    uint16_t port,
    uint32_t value
) {
    __asm__ volatile (
        "outl %0, %1"
        :
        : "a"(value),
          "Nd"(port)
    );
}

static inline uint32_t inl(
    uint16_t port
) {
    uint32_t value;

    __asm__ volatile (
        "inl %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

uint32_t pci_read(
    uint8_t bus,
    uint8_t slot,
    uint8_t function,
    uint8_t offset
) {
    uint32_t address =
        (1U << 31) |
        ((uint32_t)bus << 16) |
        ((uint32_t)slot << 11) |
        ((uint32_t)function << 8) |
        (offset & 0xFC);

    outl(
        PCI_CONFIG_ADDRESS,
        address
    );

    return inl(
        PCI_CONFIG_DATA
    );
}

uint16_t pci_read16(
    uint8_t bus,
    uint8_t slot,
    uint8_t function,
    uint8_t offset
) {
    uint32_t value =
        pci_read(
            bus,
            slot,
            function,
            offset
        );

    return
        (value >> ((offset & 2) * 8))
        & 0xFFFF;
}

uint8_t pci_read8(
    uint8_t bus,
    uint8_t slot,
    uint8_t function,
    uint8_t offset
) {
    uint32_t value =
        pci_read(
            bus,
            slot,
            function,
            offset
        );

    return
        (value >> ((offset & 3) * 8))
        & 0xFF;
}

int pci_find_class(
    uint8_t class_code,
    uint8_t subclass,
    uint8_t prog_if,
    PCI_Device* out
) {
    for (
        uint16_t bus = 0;
        bus < 256;
        bus++
    ) {
        for (
            uint16_t slot = 0;
            slot < 32;
            slot++
        ) {
            for (
                uint16_t func = 0;
                func < 8;
                func++
            ) {
                uint16_t vendor =
                    pci_read16(
                        bus,
                        slot,
                        func,
                        0x00
                    );

                if (
                    vendor == 0xFFFF
                )
                    continue;

                uint8_t cls =
                    pci_read8(
                        bus,
                        slot,
                        func,
                        0x0B
                    );

                uint8_t sub =
                    pci_read8(
                        bus,
                        slot,
                        func,
                        0x0A
                    );

                uint8_t pi =
                    pci_read8(
                        bus,
                        slot,
                        func,
                        0x09
                    );

                if (
                    cls == class_code &&
                    sub == subclass &&
                    pi == prog_if
                ) {
                    out->bus =
                        bus;

                    out->slot =
                        slot;

                    out->function =
                        func;

                    out->vendor_id =
                        vendor;

                    out->device_id =
                        pci_read16(
                            bus,
                            slot,
                            func,
                            0x02
                        );

                    out->class_code =
                        cls;

                    out->subclass =
                        sub;

                    out->prog_if =
                        pi;

                    return 1;
                }
            }
        }
    }

    return 0;
}

uint64_t pci_get_bar0(
    PCI_Device* dev
) {
    uint32_t bar =
        pci_read(
            dev->bus,
            dev->slot,
            dev->function,
            0x10
        );

    return
        (uint64_t)
        (bar & ~0xF);
}