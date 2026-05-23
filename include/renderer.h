#ifndef INTERM_RENDERER_H
#define INTERM_RENDERER_H

#include "common.h"

#define IM_STYLE_BOLD      (1 << 0)
#define IM_STYLE_ITALIC    (1 << 1)
#define IM_STYLE_UNDERLINE (1 << 2)
#define IM_STYLE_DIM       (1 << 3)
#define IM_STYLE_REVERSE   (1 << 4)

typedef struct {
    uint32_t character; // UTF-32
    uint32_t fg_color;   // 0xRRGGBB
    uint32_t bg_color;   // 0xRRGGBB
    uint16_t style;      // Bitmask
    uint8_t width;       // Display width (usually 1 or 2)
} im_cell_t;

typedef struct {
    uint32_t width;
    uint32_t height;
    im_cell_t* cells;
    bool* dirty_rows;
} im_screen_buffer_t;

// Renderer API
im_result_t im_render_init();
im_result_t im_render_shutdown();
im_result_t im_render_frame();
im_result_t im_render_invalidate_all();
im_result_t im_render_invalidate_row(uint32_t y);
im_screen_buffer_t* im_render_get_current_buffer();

#endif // INTERM_RENDERER_H
