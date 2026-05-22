#pragma once

#include <stdint.h>

typedef struct {
    char name[32];
    uint32_t sector;
    uint8_t* data;
    uint32_t size;
} TempFile;

typedef struct {
    TempFile files[128];
    uint32_t file_count;
} TempFS;

void tmpfs_init(TempFS* fs);
TempFile* tmpfs_find(TempFS* fs, uint32_t sector, uint32_t size);