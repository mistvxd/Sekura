#include <sekura/devices/usb/xhci_event.h>

#include <sekura/memory/pmm/pmm.h>
#include <sekura/tools/memset.h>

extern uint64_t hhdm;

void xhci_event_init(
    XHCIEventRing* ring
) {
    uint64_t phys =
        pmm_alloc_page(
            0,
            0
        );

    ring->phys =
        phys;

    ring->trbs =
        (XHCI_TRB*)
        (phys + hhdm);

    memset(
        ring->trbs,
        0,
        4096
    );

    ring->dequeue = 0;

    ring->cycle = 1;
}

XHCI_TRB*
xhci_event_current(
    XHCIEventRing* ring
) {
    return
        &ring->trbs[
            ring->dequeue
        ];
}

void xhci_event_advance(
    XHCIEventRing* ring
) {
    ring->dequeue++;

    if (
        ring->dequeue >=
        XHCI_EVENT_RING_TRBS
    ) {
        ring->dequeue = 0;

        ring->cycle ^= 1;
    }
}