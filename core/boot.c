#include "boot.h"
#include "terminal.h"
#include "event.h"
#include "task.h"
#include "state.h"
#include "renderer.h"
#include "input.h"
#include "ui.h"
#include "commands.h"
#include "python_bridge.h"
#include "vfs.h"
#include "lsp.h"
#include "ai.h"
#include "git.h"
#include "config.h"
#include <stdio.h>

im_result_t im_boot_init() {
    printf("INTERM - Booting...\n");
    if (im_terminal_init() != IM_OK) return IM_ERR;
    if (im_state_init() != IM_OK) return IM_ERR;
    if (im_event_init() != IM_OK) return IM_ERR;
    if (im_task_init() != IM_OK) return IM_ERR;
    if (im_input_init() != IM_OK) return IM_ERR;
    if (im_command_init() != IM_OK) return IM_ERR;
    if (im_vfs_init() != IM_OK) return IM_ERR;
    if (im_config_init() != IM_OK) return IM_ERR;
    if (im_git_init() != IM_OK) return IM_ERR;
    if (im_lsp_init() != IM_OK) return IM_ERR;
    if (im_ai_init() != IM_OK) return IM_ERR;
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
    im_ai_shutdown();
    im_lsp_shutdown();
    im_git_shutdown();
    im_config_shutdown();
    im_command_shutdown();
    im_input_shutdown();
    im_task_shutdown();
    im_event_shutdown();
    im_state_shutdown();
    im_terminal_shutdown();
    return IM_OK;
}
