#include <sekura/devices/usb/xhci_trb.h>

void xhci_trb_clear(
    XHCI_TRB* trb
) {
    trb->parameter = 0;
    trb->status = 0;
    trb->control = 0;
}

void xhci_trb_set_type(
    XHCI_TRB* trb,
    uint8_t type
) {
    trb->control |=
        ((uint32_t)type << 10);
}

void xhci_trb_enable_slot(
    XHCI_TRB* trb
) {
    xhci_trb_clear(trb);

    xhci_trb_set_type(
        trb,
        XHCI_TRB_ENABLE_SLOT
    );
}

void xhci_trb_disable_slot(
    XHCI_TRB* trb,
    uint8_t slot_id
) {
    xhci_trb_clear(trb);

    trb->control |=
        ((uint32_t)slot_id << 24);

    xhci_trb_set_type(
        trb,
        XHCI_TRB_DISABLE_SLOT
    );
}

void xhci_trb_address_device(
    XHCI_TRB* trb,
    uint8_t slot_id,
    uint64_t input_context_phys
) {
    xhci_trb_clear(trb);

    trb->parameter =
        input_context_phys;

    trb->control |=
        ((uint32_t)slot_id << 24);

    xhci_trb_set_type(
        trb,
        XHCI_TRB_ADDRESS_DEVICE
    );
}