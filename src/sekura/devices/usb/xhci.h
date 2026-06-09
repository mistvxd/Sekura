#ifndef SEKURA_XHCI_H
#define SEKURA_XHCI_H

#include <stdint.h>

#include <sekura/devices/usb/xhci_ring.h>
#include <sekura/devices/usb/xhci_event.h>

typedef struct {
    volatile uint8_t  caplength;
    volatile uint8_t  reserved;
    volatile uint16_t hciversion;

    volatile uint32_t hcsparams1;
    volatile uint32_t hcsparams2;
    volatile uint32_t hcsparams3;

    volatile uint32_t hccparams1;

    volatile uint32_t dboff;
    volatile uint32_t rtsoff;

    volatile uint32_t hccparams2;
} XHCICapRegs;

typedef struct {
    uint64_t mmio_phys;
    uint64_t mmio_virt;

    uint64_t op_base;
    uint64_t runtime_base;
    uint64_t doorbell_base;

    XHCICapRegs* cap;

    XHCIRing cmd_ring;
    XHCIEventRing event_ring;

    uint32_t slots;
    uint32_t ports;
} XHCIController;

int xhci_detect(void);

int xhci_init(
    uint64_t mmio_virtual
);

XHCIController*
xhci_controller(void);

void xhci_debug_dump(void);

#endif