#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define MAX_FILES 64

#define FS_BASE 0x8000000000

typedef struct {
    char name[64];

    uint8_t* data;
    uint64_t size;

    uint64_t page_count;
    uint64_t pages[512];
} File;

extern File files[MAX_FILES];

extern uint64_t file_count;

File* create_file(char* name, uint64_t size);
File* get_file(char* name);
int delete_file(char* name);
int read_file(char* name, void* buffer, uint64_t size, uint64_t offset);
int write_file(char* name, void* buffer, uint64_t size, uint64_t offset);
