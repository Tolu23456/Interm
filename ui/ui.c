#include "ui.h"
#include "state.h"
#include "terminal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_UI_COMPONENTS 64
static im_ui_component_t* g_components[MAX_UI_COMPONENTS];
static uint32_t g_component_count = 0;
static char* g_line_scratch = NULL;
static size_t g_line_scratch_cap = 0;

static void render_buffer_view(im_ui_component_t* self, im_screen_buffer_t* screen) {
    im_editor_state_t* state = im_state_get();
    if (!state->active_buffer) return;

    if (g_line_scratch_cap < self->width + 1) {
        g_line_scratch_cap = self->width + 1;
        g_line_scratch = realloc(g_line_scratch, g_line_scratch_cap);
    }

    uint32_t start_line = 0;
    for (uint32_t i = 0; i < self->height && (start_line + i) < state->active_buffer->line_count; i++) {
        size_t line_idx = start_line + i;
        size_t start = state->active_buffer->line_offsets[line_idx];
        size_t next_line = (line_idx + 1 < state->active_buffer->line_count) ? state->active_buffer->line_offsets[line_idx+1] : state->active_buffer->total_length;
        size_t len = next_line - start;
        if (len > self->width) len = self->width;

        size_t copied = im_buffer_copy_range(state->active_buffer, start, len, g_line_scratch);
        if (copied > 0 && g_line_scratch[copied-1] == '\n') copied--;
        if (copied > 0 && g_line_scratch[copied-1] == '\r') copied--;

        for (uint32_t x = 0; x < self->width && x < copied; x++) {
            uint32_t idx = (self->y + i) * screen->width + (self->x + x);
            screen->cells[idx].character = g_line_scratch[x];
        }
    }
}

static void render_status_bar(im_ui_component_t* self, im_screen_buffer_t* screen) {
    im_editor_state_t* state = im_state_get();
    char status[256];
    const char* mode_str = "NORMAL";
    if (state->mode == IM_MODE_INSERT) mode_str = "INSERT";
    else if (state->mode == IM_MODE_COMMAND) mode_str = "COMMAND";

    im_cursor_t* cursor = &state->cursors[state->primary_cursor_idx];
    size_t line = 0;
    if (state->active_buffer) {
        while (line < state->active_buffer->line_count - 1 && state->active_buffer->line_offsets[line+1] <= cursor->pos) line++;
    }
    size_t col = state->active_buffer ? cursor->pos - state->active_buffer->line_offsets[line] : 0;

    snprintf(status, sizeof(status), " %s | L:%zu C:%zu ", mode_str, line + 1, col + 1);

    for (uint32_t x = 0; x < self->width; x++) {
        uint32_t idx = (self->y) * screen->width + (self->x + x);
        if (x < strlen(status)) {
            screen->cells[idx].character = status[x];
        } else {
            screen->cells[idx].character = ' ';
        }
        screen->cells[idx].bg_color = 0x444444;
        screen->cells[idx].fg_color = 0xFFFFFF;
    }
}

static void render_command_palette(im_ui_component_t* self, im_screen_buffer_t* screen) {
    im_editor_state_t* state = im_state_get();
    if (state->mode != IM_MODE_COMMAND) return;

    for (uint32_t x = 0; x < self->width; x++) {
        uint32_t idx = (self->y) * screen->width + (self->x + x);
        if (x == 0) {
            screen->cells[idx].character = ':';
        } else if (x - 1 < state->command_len) {
            screen->cells[idx].character = state->command_buffer[x - 1];
        } else {
            screen->cells[idx].character = ' ';
        }
        screen->cells[idx].bg_color = 0x1E1E1E;
        screen->cells[idx].fg_color = 0x00FF00;
    }
}

im_result_t im_ui_init() {
    int w, h;
    im_terminal_get_size(&w, &h);

    im_ui_component_t* bv = malloc(sizeof(im_ui_component_t));
    bv->type = IM_UI_BUFFER_VIEW;
    bv->x = 0; bv->y = 0; bv->width = w; bv->height = h - 2;
    bv->visible = true;
    bv->render = render_buffer_view;
    g_components[g_component_count++] = bv;

    im_ui_component_t* sb = malloc(sizeof(im_ui_component_t));
    sb->type = IM_UI_STATUS_BAR;
    sb->x = 0; sb->y = h - 2; sb->width = w; sb->height = 1;
    sb->visible = true;
    sb->render = render_status_bar;
    g_components[g_component_count++] = sb;

    im_ui_component_t* cp = malloc(sizeof(im_ui_component_t));
    cp->type = IM_UI_COMMAND_PALETTE;
    cp->x = 0; cp->y = h - 1; cp->width = w; cp->height = 1;
    cp->visible = true;
    cp->render = render_command_palette;
    g_components[g_component_count++] = cp;

    return IM_OK;
}

im_result_t im_ui_shutdown() {
    for (uint32_t i = 0; i < g_component_count; i++) {
        free(g_components[i]);
    }
    g_component_count = 0;
    free(g_line_scratch);
    return IM_OK;
}

im_result_t im_ui_render_all(im_screen_buffer_t* screen) {
    for (uint32_t i = 0; i < g_component_count; i++) {
        if (g_components[i]->visible) {
            g_components[i]->render(g_components[i], screen);
        }
    }
    return IM_OK;
}
