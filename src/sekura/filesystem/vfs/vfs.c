#include <sekura/filesystem/vfs/vfs.h>

#include <sekura/memory/pmm/pmm.h>
#include <sekura/memory/vmm/vmm.h>

#include <sekura/tools/string.h>
#include <sekura/tools/memset.h>

extern uint64_t hhdm;

static uint64_t fs_next_virtual = FS_BASE;

static VfsNode nodes[MAX_NODES];

static uint64_t node_count;

VfsNode vfs_root;

void vfs_init(void) {
    memset(&vfs_root, 0, sizeof(VfsNode));

    memcpy(vfs_root.name, "/", 2);

    vfs_root.type = NODE_DIRECTORY;
}

static VfsNode* node_alloc(void) {
    if (node_count >= MAX_NODES)
        return NULL;

    return &nodes[node_count++];
}

VfsNode* vfs_find_child(VfsNode* parent, char* name) {
    for (uint64_t i = 0; i < parent->child_count; i++) {
        if (strcmp(parent->children[i]->name, name) == 0)
            return parent->children[i];
    }

    return NULL;
}

static void vfs_normalize(const char* src, char* dst) {
    uint64_t i = 0;
    uint64_t j = 0;

    if (!src || !dst)
        return;

    if (src[0] == '\0') {
        memcpy(dst, "/", 2);
        return;
    }

    while (src[i]) {
        if (src[i] == '/') {
            dst[j++] = '/';

            while (src[i] == '/')
                i++;
        } else {
            dst[j++] = src[i++];
        }
    }

    if (j > 1 && dst[j - 1] == '/')
        j--;

    dst[j] = '\0';
}

VfsNode* vfs_resolve(char* path) {
    if (!path)
        return NULL;

    if (strcmp(path, "/") == 0)
        return &vfs_root;

    if (path[0] != '/')
        return NULL;

    char normalized[256];

    vfs_normalize(path, normalized);

    path = normalized;

    VfsNode* current = &vfs_root;

    char token[64];

    uint64_t len = 0;

    char* p = path + 1;

    while (1) {
        if (*p == '/' || *p == '\0') {
            token[len] = '\0';

            if (len) {
                current = vfs_find_child(current, token);

                if (!current)
                    return NULL;
            }

            len = 0;

            if (*p == '\0')
                break;
        } else {
            if (len < sizeof(token) - 1)
                token[len++] = *p;
        }

        p++;
    }

    return current;
}

VfsNode* mkdir(char* path) {
    char parent_path[256];

    char name[64];

    char* slash = strrchr(path, '/');

    if (!slash)
        return NULL;

    if (slash == path) {
        memcpy(parent_path, "/", 2);
    } else {
        uint64_t len = slash - path;

        memcpy(parent_path, path, len);

        parent_path[len] = '\0';
    }

    memcpy(name, slash + 1, strlen(slash + 1) + 1);

    VfsNode* parent = vfs_resolve(parent_path);

    if (!parent)
        return NULL;

    if (parent->type != NODE_DIRECTORY)
        return NULL;

    if (vfs_find_child(parent, name))
        return NULL;

    VfsNode* node = node_alloc();

    if (!node)
        return NULL;

    memset(node, 0, sizeof(VfsNode));

    memcpy(node->name, name, strlen(name) + 1);

    node->type = NODE_DIRECTORY;

    node->parent = parent;

    parent->children[parent->child_count++] = node;

    return node;
}

VfsNode* create_file(char* path, uint64_t size) {
    char parent_path[256];

    char name[64];

    char* slash = strrchr(path, '/');

    if (!slash)
        return NULL;

    if (slash == path) {
        memcpy(parent_path, "/", 2);
    } else {
        uint64_t len = slash - path;

        memcpy(parent_path, path, len);

        parent_path[len] = '\0';
    }

    memcpy(name, slash + 1, strlen(slash + 1) + 1);

    VfsNode* parent = vfs_resolve(parent_path);

    if (!parent)
        return NULL;

    if (parent->type != NODE_DIRECTORY)
        return NULL;

    if (vfs_find_child(parent, name))
        return NULL;

    VfsNode* node = node_alloc();

    if (!node)
        return NULL;

    memset(node, 0, sizeof(VfsNode));

    memcpy(node->name, name, strlen(name) + 1);

    node->type = NODE_FILE;

    node->parent = parent;

    uint64_t aligned_size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    node->file.size = size;

    node->file.data = (uint8_t*)fs_next_virtual;

    for (uint64_t off = 0; off < aligned_size; off += PAGE_SIZE) {
        uint64_t phys = pmm_alloc_page(0);

        node->file.pages[off / PAGE_SIZE] = phys;

        node->file.page_count++;

        vmm_map_page(
            fs_next_virtual + off,
            phys,
            0x03,
            hhdm
        );
    }

    fs_next_virtual += aligned_size;

    parent->children[parent->child_count++] = node;

    return node;
}

int delete_file(char* path) {
    VfsNode* node = vfs_resolve(path);

    if (!node)
        return -1;

    if (node->type != NODE_FILE)
        return -1;

    for (uint64_t i = 0; i < node->file.page_count; i++) {
        pmm_free_page(node->file.pages[i]);
    }

    VfsNode* parent = node->parent;

    for (uint64_t i = 0; i < parent->child_count; i++) {
        if (parent->children[i] == node) {
            for (uint64_t j = i; j < parent->child_count - 1; j++) {
                parent->children[j] = parent->children[j + 1];
            }

            parent->child_count--;

            break;
        }
    }

    memset(node, 0, sizeof(VfsNode));

    return 0;
}

int read_file(char* path, void* buffer, uint64_t size, uint64_t offset) {
    VfsNode* node = vfs_resolve(path);

    if (!node)
        return -1;

    if (node->type != NODE_FILE)
        return -1;

    if (offset >= node->file.size)
        return -1;

    if (offset + size > node->file.size)
        size = node->file.size - offset;

    memcpy(
        buffer,
        node->file.data + offset,
        size
    );

    return size;
}

int write_file(char* path, void* buffer, uint64_t size, uint64_t offset) {
    VfsNode* node = vfs_resolve(path);

    if (!node)
        return -1;

    if (node->type != NODE_FILE)
        return -1;

    if (offset >= node->file.size)
        return -1;

    if (offset + size > node->file.size)
        size = node->file.size - offset;

    memcpy(
        node->file.data + offset,
        buffer,
        size
    );

    return size;
}

int vfs_read(VfsNode* node, void* buffer, uint64_t size, uint64_t offset) {
    if (!node)
        return -1;

    if (node->type != NODE_FILE)
        return -1;

    if (offset >= node->file.size)
        return -1;

    if (offset + size > node->file.size)
        size = node->file.size - offset;

    memcpy(
        buffer,
        node->file.data + offset,
        size
    );

    return size;
}

int vfs_write(VfsNode* node, void* buffer, uint64_t size, uint64_t offset) {
    if (!node)
        return -1;

    if (node->type != NODE_FILE)
        return -1;

    if (offset >= node->file.size)
        return -1;

    if (offset + size > node->file.size)
        size = node->file.size - offset;

    memcpy(
        node->file.data + offset,
        buffer,
        size
    );

    return size;
}