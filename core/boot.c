#include "boot.h"
#include "terminal.h"
#include "event.h"
#include "task.h"
#include "state.h"
#include "renderer.h"
#include "input.h"
#include "ui.h"
#include "python_bridge.h"
#include <stdio.h>

im_result_t im_boot_init() {
    printf("INTERM - Booting...\n");

    if (im_terminal_init() != IM_OK) return IM_ERR;
    if (im_state_init() != IM_OK) return IM_ERR;
    if (im_event_init() != IM_OK) return IM_ERR;
    if (im_task_init() != IM_OK) return IM_ERR;
    if (im_input_init() != IM_OK) return IM_ERR;
    if (im_render_init() != IM_OK) return IM_ERR;
    if (im_ui_init() != IM_OK) return IM_ERR;
    if (im_python_init() != IM_OK) return IM_ERR;

    return IM_OK;
}

im_result_t im_boot_shutdown() {
    printf("INTERM - Shutting down...\n");

    im_python_shutdown();
    im_ui_shutdown();
    im_render_shutdown();
    im_input_shutdown();
    im_task_shutdown();
    im_event_shutdown();
    im_state_shutdown();
    im_terminal_shutdown();

    return IM_OK;
}
