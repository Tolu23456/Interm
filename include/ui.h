#ifndef INTERM_UI_H
#define INTERM_UI_H

#include "common.h"
#include "renderer.h"

typedef enum {
    IM_UI_BUFFER_VIEW,
    IM_UI_STATUS_BAR,
    IM_UI_COMMAND_PALETTE,
    IM_UI_FILE_EXPLORER,
    IM_UI_NOTIFICATION
} im_ui_type_t;

typedef struct im_ui_component_t {
    im_ui_type_t type;
    uint32_t x, y, width, height;
    bool visible;
    void* data;
    void (*render)(struct im_ui_component_t* self, im_screen_buffer_t* screen);
} im_ui_component_t;

typedef enum {
    IM_LAYOUT_HORIZONTAL,
    IM_LAYOUT_VERTICAL
} im_layout_type_t;

typedef struct im_layout_node_t {
    im_layout_type_t type;
    uint32_t x, y, width, height;
    struct im_layout_node_t* children;
    struct im_layout_node_t* next;
    im_ui_component_t* component;
    float weight;
} im_layout_node_t;

im_result_t im_ui_init();
im_result_t im_ui_shutdown();
im_result_t im_ui_render_all(im_screen_buffer_t* screen);
im_result_t im_ui_layout_recompute();
im_result_t im_ui_notify(const char* message);
im_ui_component_t* im_ui_get_component(im_ui_type_t type);

#endif // INTERM_UI_H
