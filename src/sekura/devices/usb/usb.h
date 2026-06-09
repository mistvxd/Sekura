#ifndef SEKURA_USB_H
#define SEKURA_USB_H

#include <stdint.h>

#define USB_CLASS_HID        0x03

#define USB_SUBCLASS_BOOT    0x01

#define USB_PROTOCOL_KEYBOARD 0x01
#define USB_PROTOCOL_MOUSE    0x02

#define USB_DESC_DEVICE        0x01
#define USB_DESC_CONFIG        0x02
#define USB_DESC_STRING        0x03
#define USB_DESC_INTERFACE     0x04
#define USB_DESC_ENDPOINT      0x05
#define USB_DESC_HID           0x21
#define USB_DESC_REPORT        0x22

#define USB_DIR_OUT 0x00
#define USB_DIR_IN  0x80

#define USB_REQ_GET_STATUS        0x00
#define USB_REQ_CLEAR_FEATURE     0x01
#define USB_REQ_SET_FEATURE       0x03
#define USB_REQ_SET_ADDRESS       0x05
#define USB_REQ_GET_DESCRIPTOR    0x06
#define USB_REQ_SET_DESCRIPTOR    0x07
#define USB_REQ_GET_CONFIGURATION 0x08
#define USB_REQ_SET_CONFIGURATION 0x09

typedef struct {
    uint8_t length;
    uint8_t type;
} USBDescriptorHeader;

typedef struct {
    uint8_t length;
    uint8_t type;

    uint16_t usb_version;

    uint8_t device_class;
    uint8_t device_subclass;
    uint8_t device_protocol;

    uint8_t max_packet_size;

    uint16_t vendor_id;
    uint16_t product_id;

    uint16_t device_version;

    uint8_t manufacturer;
    uint8_t product;
    uint8_t serial;

    uint8_t configurations;
} USBDeviceDescriptor;

typedef struct {
    uint8_t request_type;
    uint8_t request;

    uint16_t value;
    uint16_t index;
    uint16_t length;
} USBSetupPacket;

const char* usb_class_name(uint8_t class_code);

int usb_is_hid_keyboard(
    uint8_t class_code,
    uint8_t subclass,
    uint8_t protocol
);

void usb_dump_device_descriptor(
    USBDeviceDescriptor* desc
);

#endif