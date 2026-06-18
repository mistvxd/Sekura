#pragma once

#include <stdint.h>

#define MAX_NODES 256
#define MAX_CHILDREN 64
#define FS_BASE 0xFF000000

typedef enum {
    NODE_FILE,
    NODE_DIRECTORY
} NodeType;

typedef struct {
    uint8_t* data;
    uint64_t size;

    uint64_t pages[256];
    uint64_t page_count;
} File;

typedef struct VfsNode VfsNode;

struct VfsNode {
    char name[64];

    NodeType type;

    VfsNode* parent;

    VfsNode* children[MAX_CHILDREN];
    uint64_t child_count;

    File file;
};

extern VfsNode vfs_root;

void vfs_init(void);

VfsNode* vfs_resolve(char* path);
VfsNode* vfs_find_child(VfsNode* parent, char* name);

VfsNode* mkdir(char* path);

VfsNode* create_file(char* path, uint64_t size);

int delete_file(char* path);

int read_file(char* path, void* buffer, uint64_t size, uint64_t offset);

int write_file(char* path, void* buffer, uint64_t size, uint64_t offset);

int vfs_read(VfsNode* node, void* buffer, uint64_t size, uint64_t offset);

int vfs_write(VfsNode* node, void* buffer, uint64_t size, uint64_t offset);