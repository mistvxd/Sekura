#include <stdint.h>
#include <surface/surface.h>
#include <render_backend/render_backend.h>

typedef struct {
    SekuraSurface surface;
    SekuraRenderBackend backend;
    SekuraRenderQueue queue;
} SekuraRenderer;