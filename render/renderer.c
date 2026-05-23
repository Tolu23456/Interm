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
    screen->cells = NULL;
    screen->width = 0;
    screen->height = 0;
}

static void alloc_screen(im_screen_buffer_t* screen, uint32_t w, uint32_t h) {
    screen->width = w;
    screen->height = h;
    screen->cells = (im_cell_t*)malloc(w * h * sizeof(im_cell_t));
    for (uint32_t i = 0; i < w * h; i++) {
        screen->cells[i] = (im_cell_t){' ', 0xFFFFFF, 0x000000, 0};
    }
}

im_result_t im_render_init() {
    int w, h;
    if (im_terminal_get_size(&w, &h) != IM_OK) {
        w = 80; h = 24;
    }
    
    alloc_screen(&g_curr_screen, w, h);
    alloc_screen(&g_prev_screen, w, h);
    
    // Switch to alternate buffer
    printf("\033[?1049h");
    printf("\033[2J\033[H");
    fflush(stdout);

    g_initialized = true;
    return IM_OK;
}

im_result_t im_render_shutdown() {
    if (g_initialized) {
        // Switch back from alternate buffer
        printf("\033[?1049l");
        fflush(stdout);

        free_screen(&g_curr_screen);
        free_screen(&g_prev_screen);
        g_initialized = false;
    }
    return IM_OK;
}

im_result_t im_render_frame() {
    if (!g_initialized) return IM_ERR;
    
    im_editor_state_t* state = im_state_get();
    
    // Diff-based rendering with ANSI optimization
    uint32_t last_fg = 0, last_bg = 0;
    bool first = true;

    for (uint32_t y = 0; y < g_curr_screen.height; y++) {
        bool move_needed = true;
        for (uint32_t x = 0; x < g_curr_screen.width; x++) {
            uint32_t idx = y * g_curr_screen.width + x;
            im_cell_t* curr = &g_curr_screen.cells[idx];
            im_cell_t* prev = &g_prev_screen.cells[idx];
            
            if (curr->character != prev->character || curr->fg_color != prev->fg_color || curr->bg_color != prev->bg_color) {
                if (move_needed) {
                    printf("\033[%d;%dH", y + 1, x + 1);
                    move_needed = false;
                }

                if (first || curr->fg_color != last_fg) {
                    printf("\033[38;2;%d;%d;%dm", (curr->fg_color >> 16) & 0xFF, (curr->fg_color >> 8) & 0xFF, curr->fg_color & 0xFF);
                    last_fg = curr->fg_color;
                }
                if (first || curr->bg_color != last_bg) {
                    printf("\033[48;2;%d;%d;%dm", (curr->bg_color >> 16) & 0xFF, (curr->bg_color >> 8) & 0xFF, curr->bg_color & 0xFF);
                    last_bg = curr->bg_color;
                }
                first = false;
                
                if (curr->character < 128) {
                    putchar(curr->character);
                } else {
                    putchar('?');
                }
                // After putchar, the cursor logically moves to x+1, so we might not need an explicit move for the next cell
                // but we only skip it if the next cell ALSO needs an update.
                // Actually, if we just update move_needed to false, we are good.
            } else {
                move_needed = true;
            }
        }
    }
    
    // Update cursor
    if (state->mode == IM_MODE_COMMAND) {
        printf("\033[%d;%zuH", g_curr_screen.height, state->command_len + 2);
    } else if (state->cursor_count > 0) {
        im_cursor_t* cursor = &state->cursors[state->primary_cursor_idx];
        size_t line = 0;
        while (line < state->active_buffer->line_count - 1 && state->active_buffer->line_offsets[line+1] <= cursor->pos) line++;
        size_t col = cursor->pos - state->active_buffer->line_offsets[line];
        printf("\033[%zu;%zuH", line + 1, col + 1);
    }
    
    fflush(stdout);
    memcpy(g_prev_screen.cells, g_curr_screen.cells, g_curr_screen.width * g_curr_screen.height * sizeof(im_cell_t));
    
    return IM_OK;
}

im_result_t im_render_invalidate_all() {
    if (!g_initialized) return IM_ERR;
    memset(g_prev_screen.cells, 0, g_curr_screen.width * g_curr_screen.height * sizeof(im_cell_t));
    return IM_OK;
}

im_screen_buffer_t* im_render_get_current_buffer() {
    if (!g_initialized) return NULL;
    // Clear it here or in caller? UI system should probably clear.
    for (uint32_t i = 0; i < g_curr_screen.width * g_curr_screen.height; i++) {
        g_curr_screen.cells[i] = (im_cell_t){' ', 0xCCCCCC, 0x1E1E1E, 0};
    }
    return &g_curr_screen;
}
