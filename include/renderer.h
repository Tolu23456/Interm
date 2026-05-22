#ifndef INTERM_RENDERER_H
#define INTERM_RENDERER_H

#include "common.h"

typedef struct {
    uint32_t character; // UTF-32
    uint32_t fg_color;   // 0xRRGGBB
    uint32_t bg_color;   // 0xRRGGBB
    uint16_t style;      // Bitmask: Bold, Italic, Underline, etc.
} im_cell_t;

typedef struct {
    uint32_t width;
    uint32_t height;
    im_cell_t* cells;
} im_screen_buffer_t;

// Renderer API
im_result_t im_render_init();
im_result_t im_render_shutdown();
im_result_t im_render_frame();
im_result_t im_render_invalidate_all();

#endif // INTERM_RENDERER_H
