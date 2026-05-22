#include "boot.h"
#include "terminal.h"
#include "event.h"
#include "task.h"
#include "renderer.h"
#include "input.h"
#include "state.h"
#include "commands.h"
#include <stdio.h>

im_result_t im_boot_init() {
    printf("Boot: Initializing subsystems...\n");

    if (im_terminal_init() != IM_OK) return IM_ERR;
    if (im_event_init() != IM_OK) return IM_ERR;
    if (im_state_init() != IM_OK) return IM_ERR;
    if (im_command_init() != IM_OK) return IM_ERR;
    if (im_task_init() != IM_OK) return IM_ERR;
    if (im_input_init() != IM_OK) return IM_ERR;
    if (im_render_init() != IM_OK) return IM_ERR;

    printf("Boot: All systems operational.\n");
    return IM_OK;
}

im_result_t im_boot_shutdown() {
    printf("Boot: Shutting down subsystems...\n");

    im_render_shutdown();
    im_input_shutdown();
    im_task_shutdown();
    im_command_shutdown();
    im_state_shutdown();
    im_event_shutdown();
    im_terminal_shutdown();

    printf("Boot: Shutdown complete.\n");
    return IM_OK;
}
