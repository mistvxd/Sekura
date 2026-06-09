#ifndef XHCI_TRB_H
#define XHCI_TRB_H

#include <stdint.h>

typedef struct {
    uint64_t parameter;
    uint32_t status;
    uint32_t control;
} XHCI_TRB;

/*
 * Command TRBs
 */

#define XHCI_TRB_ENABLE_SLOT      9
#define XHCI_TRB_DISABLE_SLOT     10
#define XHCI_TRB_ADDRESS_DEVICE   11

/*
 * Event TRBs
 */

#define XHCI_TRB_COMMAND_COMPLETION 33

/*
 * Common control bits
 */

#define XHCI_TRB_CYCLE_BIT (1 << 0)

void xhci_trb_clear(
    XHCI_TRB* trb
);

void xhci_trb_set_type(
    XHCI_TRB* trb,
    uint8_t type
);

void xhci_trb_enable_slot(
    XHCI_TRB* trb
);

void xhci_trb_disable_slot(
    XHCI_TRB* trb,
    uint8_t slot_id
);

void xhci_trb_address_device(
    XHCI_TRB* trb,
    uint8_t slot_id,
    uint64_t input_context_phys
);

#endif