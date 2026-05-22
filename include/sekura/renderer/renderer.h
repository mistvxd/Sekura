#include <stdint.h>
#include <sekura/surface/surface.h>
#include <sekura/render_backend/render_backend.h>

typedef struct {
    SekuraSurface surface;
    SekuraRenderBackend backend;
    SekuraRenderQueue queue;
} SekuraRenderer;