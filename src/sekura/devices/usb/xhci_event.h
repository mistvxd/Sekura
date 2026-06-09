#ifndef XHCI_EVENT_H
#define XHCI_EVENT_H

#include <stdint.h>

#include <sekura/devices/usb/xhci_trb.h>

#define XHCI_EVENT_RING_TRBS 256

typedef struct {
    XHCI_TRB* trbs;

    uint64_t phys;

    uint32_t dequeue;

    uint8_t cycle;
} XHCIEventRing;

void xhci_event_init(
    XHCIEventRing* ring
);

XHCI_TRB*
xhci_event_current(
    XHCIEventRing* ring
);

void xhci_event_advance(
    XHCIEventRing* ring
);

#endif