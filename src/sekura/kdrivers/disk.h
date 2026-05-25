#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#pragma once

int ata_disk_init(void);
int ata_read_sector(uint64_t lba, uint16_t *buffer);