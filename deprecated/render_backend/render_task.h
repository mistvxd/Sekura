#include <stdint.h>

typedef enum {
    SEKURA_RENDER_TASK_PUT_PIXEL,
    SEKURA_RENDER_TASK_FILL_RECT,
    SEKURA_RENDER_TASK_FILL_SURFACE
} SekuraRenderTaskType;

typedef struct {
    int x, y;
    uint32_t color;
} SekuraPutPixelTask;

typedef struct {
    int x, y, w, h;
    uint32_t color;
} SekuraFillRectTask;

typedef struct {
    void* surface;
} SekuraFillSurfaceTask;

typedef union {
    SekuraPutPixelTask put_pixel;
    SekuraFillRectTask fill_rect;
    SekuraFillSurfaceTask fill_surface;
} SekuraRenderTaskData;

typedef struct {
    SekuraRenderTaskType type;
    SekuraRenderTaskData data;
    int complete;
} SekuraRenderTask;