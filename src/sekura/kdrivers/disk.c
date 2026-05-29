#include <stdint.h>

#define ATA_PRIMARY_IO      0x1F0
#define ATA_PRIMARY_CTRL    0x3F6
#define ATA_CMD_IDENTIFY    0xEC
#define ATA_STATUS_BSY      0x80
#define ATA_STATUS_DRQ      0x08
#define ATA_STATUS_ERR      0x01
#define ATA_CMD_READ_28     0x20
#define ATA_CMD_READ_48     0x24

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static int ata_wait_data(void) {
    while (1) {
        uint8_t status = inb(ATA_PRIMARY_IO + 7);
        if (status & ATA_STATUS_ERR) {
            return -1;
        }
        if (!(status & ATA_STATUS_BSY) && (status & ATA_STATUS_DRQ)) {
            return 0;
        }
    }
}

typedef struct {
    uint8_t  exists;
    uint8_t  supports_lba48;
    uint64_t total_sectors;
} AtaDisk;

AtaDisk g_main_disk = {0};

int ata_disk_init(void) {
    outb(ATA_PRIMARY_CTRL, 0x04);
    for (volatile int i = 0; i < 1000; i++);
    outb(ATA_PRIMARY_CTRL, 0x00);

    outb(ATA_PRIMARY_IO + 6, 0xA0); 
    
    inb(ATA_PRIMARY_CTRL); inb(ATA_PRIMARY_CTRL);
    inb(ATA_PRIMARY_CTRL); inb(ATA_PRIMARY_CTRL);

    uint8_t status = inb(ATA_PRIMARY_IO + 7);
    if (status == 0xFF) {
        return -1;
    }
    outb(ATA_PRIMARY_IO + 2, 0);
    outb(ATA_PRIMARY_IO + 3, 0);
    outb(ATA_PRIMARY_IO + 4, 0);
    outb(ATA_PRIMARY_IO + 5, 0);
    outb(ATA_PRIMARY_IO + 7, ATA_CMD_IDENTIFY);

    status = inb(ATA_PRIMARY_IO + 7);
    if (status == 0x00) {
        return -2;
    }
    while (inb(ATA_PRIMARY_IO + 7) & ATA_STATUS_BSY);
    uint8_t cl = inb(ATA_PRIMARY_IO + 4);
    uint8_t ch = inb(ATA_PRIMARY_IO + 5);
    if (cl != 0x00 || ch != 0x00) {
        return -3;
    }
    while (1) {
        status = inb(ATA_PRIMARY_IO + 7);
        if (status & ATA_STATUS_ERR) {
            return -4;
        }
        if (status & ATA_STATUS_DRQ) {
            break;
        }
    }
    uint16_t identify_buf[256];
    for (int i = 0; i < 256; i++) {
        identify_buf[i] = inw(ATA_PRIMARY_IO);
    }
    g_main_disk.exists = 1;
    if (identify_buf[83] & (1 << 10)) {
        g_main_disk.supports_lba48 = 1;
        
        uint64_t sectors = 0;
        sectors |= (uint64_t)identify_buf[100];
        sectors |= (uint64_t)identify_buf[101] << 16;
        sectors |= (uint64_t)identify_buf[102] << 32;
        sectors |= (uint64_t)identify_buf[103] << 48;
        g_main_disk.total_sectors = sectors;
    } else {
        g_main_disk.supports_lba48 = 0;

        uint32_t sectors = 0;
        sectors |= (uint32_t)identify_buf[60];
        sectors |= (uint32_t)identify_buf[61] << 16;
        g_main_disk.total_sectors = sectors;
    }
    return 0;
}

int ata_read_sector(uint64_t lba, uint16_t *buffer) {
    if (!g_main_disk.exists) {
        return -1;
    }
    if (lba >= g_main_disk.total_sectors) {
        return -2;
    }

    while (inb(ATA_PRIMARY_IO + 7) & ATA_STATUS_BSY);

    if (g_main_disk.supports_lba48 && (lba > 0x0FFFFFFF || 1)) { 

        outb(ATA_PRIMARY_IO + 6, 0x40); 

        outb(ATA_PRIMARY_IO + 2, 0);
        outb(ATA_PRIMARY_IO + 2, 1);
        
        outb(ATA_PRIMARY_IO + 3, (uint8_t)(lba >> 24));
        outb(ATA_PRIMARY_IO + 3, (uint8_t)lba);
        
        outb(ATA_PRIMARY_IO + 4, (uint8_t)(lba >> 32));
        outb(ATA_PRIMARY_IO + 4, (uint8_t)(lba >> 8));
        
        outb(ATA_PRIMARY_IO + 5, (uint8_t)(lba >> 40));
        outb(ATA_PRIMARY_IO + 5, (uint8_t)(lba >> 16));

        outb(ATA_PRIMARY_IO + 7, ATA_CMD_READ_48);
        
    } 
    else {
        outb(ATA_PRIMARY_IO + 6, 0xE0 | ((lba >> 24) & 0x0F));
        
        outb(ATA_PRIMARY_IO + 2, 1);
        
        outb(ATA_PRIMARY_IO + 3, (uint8_t)lba);
        outb(ATA_PRIMARY_IO + 4, (uint8_t)(lba >> 8));
        outb(ATA_PRIMARY_IO + 5, (uint8_t)(lba >> 16));
        outb(ATA_PRIMARY_IO + 7, ATA_CMD_READ_28);
    }

    inb(ATA_PRIMARY_CTRL); inb(ATA_PRIMARY_CTRL);

    if (ata_wait_data() < 0) {
        return -3;
    }

    for (int i = 0; i < 256; i++) {
        buffer[i] = inw(ATA_PRIMARY_IO);
    }
    return 0;
}

int ata_read(
    uint64_t lba,
    uint32_t sectors,
    void* buffer
) {
    uint16_t* ptr = buffer;

    for (
        uint32_t s = 0;
        s < sectors;
        s++
    ) {
        int r =
            ata_read_sector(
                lba + s,
                ptr + (s * 256)
            );

        if (r < 0)
            return r;
    }

    return 0;
}