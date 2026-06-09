#include <stdint.h>

#include <sekura/devices/pci/pci.h>
#include <sekura/devices/usb/xhci.h>

#include <sekura/devices/usb/xhci_cmd.h>

#include <sekura/serial/serial.h>

static XHCIController controller;

XHCIController*
xhci_controller(void)
{
    return &controller;
}

int xhci_detect(void)
{
    PCI_Device dev;

    if (
        !pci_find_class(
            0x0C,
            0x03,
            0x30,
            &dev
        )
    ) {
        serial_write(
            "[xHCI not found]\n"
        );

        return 0;
    }

    controller.mmio_phys =
        pci_get_bar0(&dev);

    serial_write(
        "[xHCI found]\n"
    );

    serial_write(
        "MMIO: "
    );

    serial_write_hex(
        controller.mmio_phys
    );

    serial_write(
        "\n"
    );

    return 1;
}

void xhci_debug_dump(void)
{
    serial_write(
        "\n========== XHCI ==========\n"
    );

    serial_write(
        "CAPLENGTH="
    );

    serial_write_hex(
        controller.cap->caplength
    );

    serial_write(
        "\nHCIVERSION="
    );

    serial_write_hex(
        controller.cap->hciversion
    );

    serial_write(
        "\nHCS1="
    );

    serial_write_hex(
        controller.cap->hcsparams1
    );

    serial_write(
        "\nHCS2="
    );

    serial_write_hex(
        controller.cap->hcsparams2
    );

    serial_write(
        "\nHCS3="
    );

    serial_write_hex(
        controller.cap->hcsparams3
    );

    serial_write(
        "\nSlots="
    );

    serial_write_int(
        controller.slots
    );

    serial_write(
        "\nPorts="
    );

    serial_write_int(
        controller.ports
    );

    serial_write(
        "\nCommand Ring="
    );

    serial_write_hex(
        controller.cmd_ring.phys
    );

    serial_write(
        "\nEvent Ring="
    );

    serial_write_hex(
        controller.event_ring.phys
    );

    serial_write(
        "\n==========================\n"
    );
}

int xhci_init(
    uint64_t mmio_virtual
)
{
    if (!xhci_detect())
        return 0;

    controller.mmio_virt =
        mmio_virtual;

    controller.cap =
        (XHCICapRegs*)
        mmio_virtual;

    controller.slots =
        controller.cap->hcsparams1 &
        0xFF;

    controller.ports =
        (
            controller.cap->hcsparams1
            >> 24
        ) &
        0xFF;

    controller.op_base =
        mmio_virtual +
        controller.cap->caplength;

    controller.runtime_base =
        mmio_virtual +
        controller.cap->rtsoff;

    controller.doorbell_base =
        mmio_virtual +
        controller.cap->dboff;

    xhci_ring_init(
        &controller.cmd_ring
    );

    xhci_event_init(
        &controller.event_ring
    );

    xhci_cmd_init(
        &controller
    );

    xhci_debug_dump();

    return 1;
}