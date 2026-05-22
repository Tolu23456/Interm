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
    printf("  Renderer: Initializing...\n");
    
    int w, h;
    if (im_terminal_get_size(&w, &h) != IM_OK) {
        w = 80; h = 24; // Fallback
    }
    
    alloc_screen(&g_curr_screen, w, h);
    alloc_screen(&g_prev_screen, w, h);
    
    g_initialized = true;
    return IM_OK;
}

im_result_t im_render_shutdown() {
    if (g_initialized) {
        printf("  Renderer: Shutting down...\n");
        free_screen(&g_curr_screen);
        free_screen(&g_prev_screen);
        g_initialized = false;
    }
    return IM_OK;
}

im_result_t im_render_frame() {
    if (!g_initialized) return IM_ERR;
    
    im_editor_state_t* state = im_state_get();
    if (!state->active_buffer) return IM_OK; // Nothing to render
    
    // 1. Clear current screen buffer (logically)
    for (uint32_t i = 0; i < g_curr_screen.width * g_curr_screen.height; i++) {
        g_curr_screen.cells[i] = (im_cell_t){' ', 0xFFFFFF, 0x000000, 0};
    }
    
    // 2. Fill with buffer content (visible area is height-1)
    char* text = im_buffer_get_range(state->active_buffer, 0, g_curr_screen.width * (g_curr_screen.height - 1));
    if (text) {
        for (uint32_t i = 0; text[i] != '\0' && i < g_curr_screen.width * (g_curr_screen.height - 1); i++) {
            g_curr_screen.cells[i].character = text[i];
        }
        free(text);
    }

    // 2.1 Render command line at the bottom
    if (state->mode == IM_MODE_COMMAND) {
        uint32_t start_idx = (g_curr_screen.height - 1) * g_curr_screen.width;
        g_curr_screen.cells[start_idx].character = ':';
        for (size_t i = 0; i < state->command_len && i < g_curr_screen.width - 1; i++) {
            g_curr_screen.cells[start_idx + 1 + i].character = state->command_buffer[i];
        }
    }
    
    // 3. Diff and emit ANSI
    for (uint32_t y = 0; y < g_curr_screen.height; y++) {
        for (uint32_t x = 0; x < g_curr_screen.width; x++) {
            uint32_t idx = y * g_curr_screen.width + x;
            im_cell_t* curr = &g_curr_screen.cells[idx];
            im_cell_t* prev = &g_prev_screen.cells[idx];
            
            if (curr->character != prev->character || curr->fg_color != prev->fg_color || curr->bg_color != prev->bg_color) {
                printf("\033[%d;%dH", y + 1, x + 1);
                // Simple color output (always true color)
                printf("\033[38;2;%d;%d;%dm", (curr->fg_color >> 16) & 0xFF, (curr->fg_color >> 8) & 0xFF, curr->fg_color & 0xFF);
                printf("\033[48;2;%d;%d;%dm", (curr->bg_color >> 16) & 0xFF, (curr->bg_color >> 8) & 0xFF, curr->bg_color & 0xFF);
                
                if (curr->character < 128) {
                    putchar(curr->character);
                } else {
                    putchar('?');
                }
            }
        }
    }
    
    // 4. Update cursor position
    if (state->mode == IM_MODE_COMMAND) {
        printf("\033[%d;%zuH", g_curr_screen.height, state->command_len + 2);
    } else if (state->cursor_count > 0) {
        im_cursor_t* cursor = &state->cursors[state->primary_cursor_idx];
        int cy = cursor->pos / g_curr_screen.width;
        int cx = cursor->pos % g_curr_screen.width;
        printf("\033[%d;%dH", cy + 1, cx + 1);
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
