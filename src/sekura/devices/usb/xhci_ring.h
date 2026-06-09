#ifndef XHCI_RING_H
#define XHCI_RING_H

#include <stdint.h>

#include <sekura/devices/usb/xhci_trb.h>

#define XHCI_RING_TRBS 256

typedef struct {
    XHCI_TRB* trbs;

    uint64_t phys;

    uint32_t enqueue;

    uint8_t cycle;
} XHCIRing;

void xhci_ring_init(
    XHCIRing* ring
);

XHCI_TRB*
xhci_ring_push(
    XHCIRing* ring
);

#endif