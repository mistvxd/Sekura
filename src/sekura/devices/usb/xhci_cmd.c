#include <sekura/devices/usb/xhci_cmd.h>
#include <sekura/devices/usb/xhci_trb.h>

#include <sekura/serial/serial.h>

#define XHCI_DBOFF 0x2000

void xhci_cmd_init(
    XHCIController* xhci
) {
    xhci_ring_init(
        &xhci->cmd_ring
    );
}

void xhci_cmd_ring_doorbell(
    XHCIController* xhci
) {
    volatile uint32_t* db0 =
        (volatile uint32_t*)
        (
            xhci->mmio_base +
            XHCI_DBOFF
        );

    *db0 = 0;
}

void xhci_cmd_enable_slot(
    XHCIController* xhci
) {
    XHCI_TRB* trb =
        xhci_ring_push(
            &xhci->cmd_ring
        );

    xhci_trb_enable_slot(
        trb
    );

    trb->control |=
        XHCI_TRB_CYCLE_BIT;

    serial_write(
        "Enable Slot TRB\n"
    );

    serial_write_hex(
        trb->control
    );

    serial_write(
        "\n"
    );

    xhci_cmd_ring_doorbell(
        xhci
    );
}