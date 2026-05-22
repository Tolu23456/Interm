#include "state.h"
#include "event.h"
#include <stdio.h>
#include <string.h>

static im_editor_state_t g_state;

im_result_t im_state_init() {
    printf("  State Machine: Initializing...\n");
    memset(&g_state, 0, sizeof(im_editor_state_t));
    g_state.mode = IM_MODE_NORMAL;
    g_state.cursor_count = 1;
    g_state.primary_cursor_idx = 0;
    return IM_OK;
}

im_result_t im_state_shutdown() {
    printf("  State Machine: Shutting down...\n");
    return IM_OK;
}

im_editor_state_t* im_state_get() {
    return &g_state;
}

im_result_t im_state_set_mode(im_mode_t mode) {
    if (g_state.mode == mode) return IM_OK;
    
    im_mode_t old_mode = g_state.mode;
    g_state.mode = mode;
    
    // Emit event
    // TODO: Create state_mode_changed event
    printf("  State: Mode changed %d -> %d\n", old_mode, mode);
    
    return IM_OK;
}

im_result_t im_state_set_active_buffer(im_buffer_t* buf) {
    g_state.active_buffer = buf;
    return IM_OK;
}
