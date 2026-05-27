#include "renderer.h"
#include "terminal.h"
#include "state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static im_screen_buffer_t g_curr_screen;
static im_screen_buffer_t g_prev_screen;
static bool g_initialized = false;

static void free_screen(im_screen_buffer_t* screen) {
    free(screen->cells);
    free(screen->dirty_rows);
    screen->cells = NULL;
    screen->dirty_rows = NULL;
}

static void alloc_screen(im_screen_buffer_t* screen, uint32_t w, uint32_t h, bool clear) {
    screen->width = w;
    screen->height = h;
    screen->cells = (im_cell_t*)malloc(w * h * sizeof(im_cell_t));
    screen->dirty_rows = (bool*)malloc(h * sizeof(bool));
    if (clear) {
        for (uint32_t i = 0; i < w * h; i++) {
            screen->cells[i] = (im_cell_t){' ', 0xFFFFFF, 0x000000, 0, 1};
        }
    } else {
        memset(screen->cells, 0, w * h * sizeof(im_cell_t));
    }
    memset(screen->dirty_rows, true, h * sizeof(bool));
}

im_result_t im_render_init() {
    int w, h;
    if (im_terminal_get_size(&w, &h) != IM_OK) {
        w = 80; h = 24;
    }
    
    alloc_screen(&g_curr_screen, (uint32_t)w, (uint32_t)h, true);
    alloc_screen(&g_prev_screen, (uint32_t)w, (uint32_t)h, false);
    
    printf("\033[?1049h");
    printf("\033[2J\033[H");
    printf("\033[?25l"); // Hide cursor during render
    fflush(stdout);

    g_initialized = true;
    return IM_OK;
}

im_result_t im_render_shutdown() {
    if (g_initialized) {
        printf("\033[?25h"); // Show cursor
        printf("\033[?1049l");
        fflush(stdout);
        free_screen(&g_curr_screen);
        free_screen(&g_prev_screen);
        g_initialized = false;
    }
    return IM_OK;
}

static void emit_style(im_cell_t* cell, im_cell_t* last) {
    if (last && cell->fg_color == last->fg_color && cell->bg_color == last->bg_color && cell->style == last->style) return;

    printf("\033[0m"); // Reset first for safety
    printf("\033[38;2;%d;%d;%dm", (cell->fg_color >> 16) & 0xFF, (cell->fg_color >> 8) & 0xFF, cell->fg_color & 0xFF);
    printf("\033[48;2;%d;%d;%dm", (cell->bg_color >> 16) & 0xFF, (cell->bg_color >> 8) & 0xFF, cell->bg_color & 0xFF);
    if (cell->style & IM_STYLE_BOLD) printf("\033[1m");
    if (cell->style & IM_STYLE_DIM) printf("\033[2m");
    if (cell->style & IM_STYLE_ITALIC) printf("\033[3m");
    if (cell->style & IM_STYLE_UNDERLINE) printf("\033[4m");
    if (cell->style & IM_STYLE_REVERSE) printf("\033[7m");
}

im_result_t im_render_frame() {
    if (!g_initialized) return IM_ERR;
    im_editor_state_t* state = im_state_get();
    
    for (uint32_t y = 0; y < g_curr_screen.height; y++) {
        if (!g_curr_screen.dirty_rows[y]) {
            // Check if any cell actually changed (row hashing or just bit comparison)
            bool row_changed = false;
            for (uint32_t x = 0; x < g_curr_screen.width; x++) {
                uint32_t idx = y * g_curr_screen.width + x;
                if (memcmp(&g_curr_screen.cells[idx], &g_prev_screen.cells[idx], sizeof(im_cell_t)) != 0) {
                    row_changed = true;
                    break;
                }
            }
            if (!row_changed) continue;
        }

        bool move_needed = true;
        im_cell_t last_style = {0};
        bool style_initialized = false;

        for (uint32_t x = 0; x < g_curr_screen.width; x++) {
            uint32_t idx = y * g_curr_screen.width + x;
            im_cell_t* curr = &g_curr_screen.cells[idx];
            im_cell_t* prev = &g_prev_screen.cells[idx];
            
            if (memcmp(curr, prev, sizeof(im_cell_t)) != 0) {
                if (move_needed) {
                    printf("\033[%d;%dH", y + 1, x + 1);
                    move_needed = false;
                }
                
                emit_style(curr, style_initialized ? &last_style : NULL);
                last_style = *curr;
                style_initialized = true;

                if (curr->character < 128) putchar((int)curr->character);
                else {
                    // Simple UTF-8 emitter (very basic)
                    if (curr->character <= 0x7F) putchar((int)curr->character);
                    else if (curr->character <= 0x7FF) {
                        putchar(0xC0 | (curr->character >> 6));
                        putchar(0x80 | (curr->character & 0x3F));
                    } else if (curr->character <= 0xFFFF) {
                        putchar(0xE0 | (curr->character >> 12));
                        putchar(0x80 | ((curr->character >> 6) & 0x3F));
                        putchar(0x80 | (curr->character & 0x3F));
                    } else {
                        putchar(0xF0 | (curr->character >> 18));
                        putchar(0x80 | ((curr->character >> 12) & 0x3F));
                        putchar(0x80 | ((curr->character >> 6) & 0x3F));
                        putchar(0x80 | (curr->character & 0x3F));
                    }
                }
            } else {
                move_needed = true;
            }
        }
        g_curr_screen.dirty_rows[y] = false;
    }
    
    // Update cursor
    pthread_mutex_lock(&state->mutex);
    printf("\033[?25h"); // Show cursor
    if (state->mode == IM_MODE_COMMAND) {
        printf("\033[%d;%zuH", g_curr_screen.height, state->command_len + 2);
    } else if (state->cursor_count > 0 && state->active_buffer) {
        im_cursor_t* cursor = &state->cursors[state->primary_cursor_idx];
        size_t line = 0;
        pthread_mutex_lock(&state->active_buffer->mutex);
        while (line < state->active_buffer->line_count - 1 && state->active_buffer->line_offsets[line+1] <= cursor->pos) line++;
        size_t col = cursor->pos - state->active_buffer->line_offsets[line];
        pthread_mutex_unlock(&state->active_buffer->mutex);
        printf("\033[%zu;%zuH", line + 1, col + 1);
    }
    pthread_mutex_unlock(&state->mutex);
    
    fflush(stdout);
    memcpy(g_prev_screen.cells, g_curr_screen.cells, g_curr_screen.width * g_curr_screen.height * sizeof(im_cell_t));
    
    return IM_OK;
}

im_result_t im_render_invalidate_all() {
    if (!g_initialized) return IM_ERR;
    memset(g_curr_screen.dirty_rows, true, g_curr_screen.height * sizeof(bool));
    return IM_OK;
}

im_result_t im_render_invalidate_row(uint32_t y) {
    if (!g_initialized || y >= g_curr_screen.height) return IM_ERR;
    g_curr_screen.dirty_rows[y] = true;
    return IM_OK;
}

im_screen_buffer_t* im_render_get_current_buffer() {
    if (!g_initialized) return NULL;
    // We don't clear everything every time, only what's needed or caller clears.
    return &g_curr_screen;
}
