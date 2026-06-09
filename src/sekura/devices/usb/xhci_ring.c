#include <sekura/devices/usb/xhci_ring.h>
#include <sekura/devices/usb/xhci_trb.h>

#include <sekura/memory/pmm/pmm.h>
#include <sekura/tools/memset.h>

extern uint64_t hhdm;

void xhci_ring_init(
    XHCIRing* ring
) {
    uint64_t phys =
        pmm_alloc_page(
            0,
            0
        );

    ring->phys = phys;

    ring->trbs =
        (XHCI_TRB*)
        (phys + hhdm);

    memset(
        ring->trbs,
        0,
        4096
    );

    ring->enqueue = 0;

    ring->cycle = 1;
}

XHCI_TRB*
xhci_ring_push(
    XHCIRing* ring
) {
    XHCI_TRB* trb =
        &ring->trbs[
            ring->enqueue
        ];

    ring->enqueue++;

    if (
        ring->enqueue >=
        XHCI_RING_TRBS
    ) {
        ring->enqueue = 0;
    }

    return trb;
}