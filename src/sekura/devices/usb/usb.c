#include <sekura/devices/usb/usb.h>
#include <sekura/serial/serial.h>

const char* usb_class_name(uint8_t class_code) {
    switch (class_code) {

        case 0x00:
            return "Per Interface";

        case 0x02:
            return "Communications";

        case 0x03:
            return "HID";

        case 0x08:
            return "Mass Storage";

        case 0x09:
            return "Hub";

        case 0x0E:
            return "Video";

        case 0xEF:
            return "Composite";

        default:
            return "Unknown";
    }
}

int usb_is_hid_keyboard(
    uint8_t class_code,
    uint8_t subclass,
    uint8_t protocol
) {
    return
        class_code == USB_CLASS_HID &&
        subclass == USB_SUBCLASS_BOOT &&
        protocol == USB_PROTOCOL_KEYBOARD;
}

void usb_dump_device_descriptor(
    USBDeviceDescriptor* desc
) {
    serial_write("\n[USB DEVICE]\n");

    serial_write("VID: ");
    serial_write_hex(desc->vendor_id);

    serial_write("\nPID: ");
    serial_write_hex(desc->product_id);

    serial_write("\nCLASS: ");
    serial_write(
        usb_class_name(
            desc->device_class
        )
    );

    serial_write("\n");
}