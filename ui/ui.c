#include "ui.h"
#include "state.h"
#include "terminal.h"
#include "syntax.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static im_layout_node_t* g_root_node = NULL;
static char* g_line_scratch = NULL;
static size_t g_line_scratch_cap = 0;

static uint32_t get_token_color(im_token_type_t type) {
    switch (type) {
        case IM_TOKEN_KEYWORD: return 0x569CD6;
        case IM_TOKEN_STRING: return 0xCE9178;
        case IM_TOKEN_NUMBER: return 0xB5CEA8;
        case IM_TOKEN_COMMENT: return 0x6A9955;
        case IM_TOKEN_OPERATOR: return 0xD4D4D4;
        case IM_TOKEN_IDENTIFIER: return 0x9CDCFE;
        default: return 0xCCCCCC;
    }
}

static void render_buffer_view(im_ui_component_t* self, im_screen_buffer_t* screen) {
    im_editor_state_t* state = im_state_get();
    if (!state->active_buffer) return;
    if (g_line_scratch_cap < self->width + 1) {
        g_line_scratch_cap = self->width + 1;
        g_line_scratch = realloc(g_line_scratch, g_line_scratch_cap);
    }
    uint32_t start_line = 0;

    pthread_mutex_lock(&state->mutex);
    im_cursor_t* cursor = &state->cursors[state->primary_cursor_idx];
    size_t sel_min = cursor->pos < cursor->anchor ? cursor->pos : cursor->anchor;
    size_t sel_max = cursor->pos > cursor->anchor ? cursor->pos : cursor->anchor;
    bool in_visual = (state->mode == IM_MODE_VISUAL);
    pthread_mutex_unlock(&state->mutex);

    for (uint32_t i = 0; i < self->height && (start_line + i) < state->active_buffer->line_count; i++) {
        size_t line_idx = start_line + i;
        size_t line_start = state->active_buffer->line_offsets[line_idx];
        size_t next_line = (line_idx + 1 < state->active_buffer->line_count) ? state->active_buffer->line_offsets[line_idx+1] : state->active_buffer->total_length;
        size_t len = next_line - line_start;

        size_t copied = im_buffer_copy_range(state->active_buffer, line_start, len, g_line_scratch);
        g_line_scratch[copied] = '\0';
        if (copied > 0 && (g_line_scratch[copied-1] == '\n' || g_line_scratch[copied-1] == '\r')) copied--;

        im_token_t tokens[256];
        size_t token_count = 0;
        im_syntax_tokenize_line(g_line_scratch, copied, tokens, &token_count, 256);

        for (uint32_t x = 0; x < self->width; x++) {
            uint32_t idx = (self->y + i) * screen->width + (self->x + x);
            screen->cells[idx].character = (x < copied) ? g_line_scratch[x] : ' ';
            screen->cells[idx].fg_color = 0xCCCCCC;
            screen->cells[idx].bg_color = 0x1E1E1E;
            screen->cells[idx].style = 0;

            if (in_visual && x < copied) {
                size_t abs_pos = line_start + x;
                if (abs_pos >= sel_min && abs_pos <= sel_max) {
                    screen->cells[idx].bg_color = 0x264F78;
                }
            }
        }

        for (size_t t = 0; t < token_count; t++) {
            uint32_t color = get_token_color(tokens[t].type);
            for (size_t x = tokens[t].start; x < tokens[t].start + tokens[t].length && x < self->width; x++) {
                uint32_t idx = (self->y + i) * screen->width + (self->x + x);
                screen->cells[idx].fg_color = color;
                if (tokens[t].type == IM_TOKEN_KEYWORD) screen->cells[idx].style |= IM_STYLE_BOLD;
            }
        }
    }
}

static void render_status_bar(im_ui_component_t* self, im_screen_buffer_t* screen) {
    im_editor_state_t* state = im_state_get();
    char status[256];
    const char* mode_str = "NORMAL";
    pthread_mutex_lock(&state->mutex);
    if (state->mode == IM_MODE_INSERT) mode_str = "INSERT";
    else if (state->mode == IM_MODE_COMMAND) mode_str = "COMMAND";
    else if (state->mode == IM_MODE_VISUAL) mode_str = "VISUAL";
    im_cursor_t* cursor = &state->cursors[state->primary_cursor_idx];
    pthread_mutex_unlock(&state->mutex);

    size_t line = 0;
    if (state->active_buffer) {
        pthread_mutex_lock(&state->active_buffer->mutex);
        while (line < state->active_buffer->line_count - 1 && state->active_buffer->line_offsets[line+1] <= cursor->pos) line++;
        pthread_mutex_unlock(&state->active_buffer->mutex);
    }
    size_t col = state->active_buffer ? cursor->pos - state->active_buffer->line_offsets[line] : 0;
    snprintf(status, sizeof(status), " %s | L:%zu C:%zu ", mode_str, line + 1, col + 1);
    for (uint32_t x = 0; x < self->width; x++) {
        uint32_t idx = (self->y) * screen->width + (self->x + x);
        screen->cells[idx].character = (x < strlen(status)) ? status[x] : ' ';
        screen->cells[idx].bg_color = 0x444444;
        screen->cells[idx].fg_color = 0xFFFFFF;
    }
}

static void render_command_palette(im_ui_component_t* self, im_screen_buffer_t* screen) {
    im_editor_state_t* state = im_state_get();
    pthread_mutex_lock(&state->mutex);
    bool in_cmd = (state->mode == IM_MODE_COMMAND);
    size_t cmd_len = state->command_len;
    char cmd_buf[MAX_COMMAND_LEN];
    memcpy(cmd_buf, state->command_buffer, MAX_COMMAND_LEN);
    pthread_mutex_unlock(&state->mutex);

    if (!in_cmd) return;
    for (uint32_t x = 0; x < self->width; x++) {
        uint32_t idx = (self->y) * screen->width + (self->x + x);
        if (x == 0) screen->cells[idx].character = ':';
        else if (x - 1 < cmd_len) screen->cells[idx].character = cmd_buf[x - 1];
        else screen->cells[idx].character = ' ';
        screen->cells[idx].bg_color = 0x1E1E1E;
        screen->cells[idx].fg_color = 0x00FF00;
    }
}

static void apply_layout(im_layout_node_t* node, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    node->x = x; node->y = y; node->width = w; node->height = h;
    if (node->component) {
        node->component->x = x; node->component->y = y;
        node->component->width = w; node->component->height = h;
    }
    if (node->children) {
        uint32_t curr_x = x, curr_y = y;
        im_layout_node_t* child = node->children;
        while (child) {
            uint32_t cw = w, ch = h;
            if (node->type == IM_LAYOUT_HORIZONTAL) cw = (uint32_t)(w * child->weight);
            else ch = (uint32_t)(h * child->weight);
            apply_layout(child, curr_x, curr_y, cw, ch);
            if (node->type == IM_LAYOUT_HORIZONTAL) curr_x += cw;
            else curr_y += ch;
            child = child->next;
        }
    }
}

static void render_recursive(im_layout_node_t* node, im_screen_buffer_t* screen) {
    if (node->component && node->component->visible) node->component->render(node->component, screen);
    im_layout_node_t* child = node->children;
    while (child) { render_recursive(child, screen); child = child->next; }
}

im_result_t im_ui_init() {
    int w, h; im_terminal_get_size(&w, &h);
    g_root_node = calloc(1, sizeof(im_layout_node_t));
    g_root_node->type = IM_LAYOUT_VERTICAL;
    g_root_node->weight = 1.0f;
    im_layout_node_t* main_area = calloc(1, sizeof(im_layout_node_t));
    main_area->type = IM_LAYOUT_VERTICAL; main_area->weight = 0.92f;
    g_root_node->children = main_area;
    im_layout_node_t* status_area = calloc(1, sizeof(im_layout_node_t));
    status_area->weight = 0.04f; main_area->next = status_area;
    im_layout_node_t* cmd_area = calloc(1, sizeof(im_layout_node_t));
    cmd_area->weight = 0.04f; status_area->next = cmd_area;
    im_ui_component_t* bv = calloc(1, sizeof(im_ui_component_t));
    bv->type = IM_UI_BUFFER_VIEW; bv->visible = true; bv->render = render_buffer_view;
    main_area->component = bv;
    im_ui_component_t* sb = calloc(1, sizeof(im_ui_component_t));
    sb->type = IM_UI_STATUS_BAR; sb->visible = true; sb->render = render_status_bar;
    status_area->component = sb;
    im_ui_component_t* cp = calloc(1, sizeof(im_ui_component_t));
    cp->type = IM_UI_COMMAND_PALETTE; cp->visible = true; cp->render = render_command_palette;
    cmd_area->component = cp;
    apply_layout(g_root_node, 0, 0, (uint32_t)w, (uint32_t)h);
    return IM_OK;
}

im_result_t im_ui_shutdown() {
    free(g_root_node);
    free(g_line_scratch);
    return IM_OK;
}

im_result_t im_ui_render_all(im_screen_buffer_t* screen) {
    if (!g_root_node) return IM_ERR;
    render_recursive(g_root_node, screen);
    return IM_OK;
}

im_result_t im_ui_layout_recompute() {
    int w, h; im_terminal_get_size(&w, &h);
    if (g_root_node) apply_layout(g_root_node, 0, 0, (uint32_t)w, (uint32_t)h);
    return IM_OK;
}
