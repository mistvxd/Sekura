#ifndef XHCI_CMD_H
#define XHCI_CMD_H

#include <stdint.h>

#include <sekura/devices/usb/xhci_ring.h>
#include <sekura/devices/usb/xhci.h>

void xhci_cmd_init(
    XHCIController* xhci
);

void xhci_cmd_ring_doorbell(
    XHCIController* xhci
);

void xhci_cmd_enable_slot(
    XHCIController* xhci
);

#endif