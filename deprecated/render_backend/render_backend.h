#include <stdint.h>
#include <render_backend/render_task.h>
#include <surface/surface.h>

#define MAX_TASKS 1024

typedef struct {
    const char* vendor;

    void (*submit)(SekuraRenderQueue* queue, SekuraSurface* surface);
} SekuraRenderBackend;

typedef struct {
    SekuraRenderTask tasks[MAX_TASKS];
    uint32_t count;
} SekuraRenderQueue;