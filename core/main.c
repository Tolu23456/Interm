#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <poll.h>
#include <signal.h>
#include "boot.h"
#include "input.h"
#include "event.h"
#include "buffer.h"
#include "state.h"
#include "renderer.h"
#include "commands.h"
#include "storage.h"
#include "ui.h"
#include "python_bridge.h"
#include "config.h"

static bool g_keep_running = true;
static im_buffer_t g_main_buffer;
static volatile sig_atomic_t g_resize_pending = 0;

static void handle_sigwinch(int sig) {
    (void)sig;
    g_resize_pending = 1;
}

static im_result_t cmd_quit(const char* args, bool force) {
    (void)args; (void)force;
    g_keep_running = false;
    return IM_OK;
}

static char g_active_file[256] = "untitled.txt";

static im_result_t cmd_save(const char* args, bool force) {
    (void)force;
    const char* path = (args && strlen(args) > 0) ? args : g_active_file;
    im_result_t res = im_storage_save(&g_main_buffer, path);
    if (res == IM_OK) {
        if (args && strlen(args) > 0) strncpy(g_active_file, args, sizeof(g_active_file)-1);
        im_ui_notify("File saved successfully");
    }
    return res;
}

static im_result_t cmd_edit(const char* args, bool force) {
    (void)force;
    if (!args || strlen(args) == 0) return IM_ERR_INVALID_ARG;
    im_buffer_t new_buf;
    if (im_storage_load(&new_buf, args) == IM_OK) {
        pthread_mutex_lock(&g_main_buffer.mutex);
        im_buffer_destroy(&g_main_buffer);
        g_main_buffer = new_buf;
        pthread_mutex_unlock(&g_main_buffer.mutex);
        im_state_set_active_buffer(&g_main_buffer);
        strncpy(g_active_file, args, sizeof(g_active_file)-1);
        im_editor_state_t* state = im_state_get();
        pthread_mutex_lock(&state->mutex);
        state->cursors[state->primary_cursor_idx].pos = 0;
        state->cursors[state->primary_cursor_idx].anchor = 0;
        state->cursors[state->primary_cursor_idx].has_selection = false;
        state->scroll_row = 0; state->scroll_col = 0;
        pthread_mutex_unlock(&state->mutex);
        im_ui_notify("File loaded");
        return IM_OK;
    }
    return IM_ERR;
}

static void update_scrolling(im_editor_state_t* state) {
    if (!state->active_buffer) return;
    im_cursor_t* cursor = &state->cursors[state->primary_cursor_idx];
    size_t line = 0;
    pthread_mutex_lock(&state->active_buffer->mutex);
    while (line < state->active_buffer->line_count - 1 && state->active_buffer->line_offsets[line+1] <= cursor->pos) line++;
    size_t col = cursor->pos - state->active_buffer->line_offsets[line];
    pthread_mutex_unlock(&state->active_buffer->mutex);
    int vh = 22;
    if (line < state->scroll_row) state->scroll_row = line;
    else if (line >= state->scroll_row + vh) state->scroll_row = line - vh + 1;
    int vw = 60;
    if (col < state->scroll_col) state->scroll_col = col;
    else if (col >= state->scroll_col + vw) state->scroll_col = col - vw + 1;
}

static void handle_motion(im_key_code_t code, im_cursor_t* cursor, im_buffer_t* buf) {
    if (code == IM_KEY_LEFT) { if (cursor->pos > 0) cursor->pos--; }
    else if (code == IM_KEY_RIGHT) { if (cursor->pos < buf->total_length) cursor->pos++; }
    else if (code == IM_KEY_DOWN) {
        size_t line = 0; pthread_mutex_lock(&buf->mutex);
        while (line < buf->line_count - 1 && buf->line_offsets[line+1] <= cursor->pos) line++;
        if (line < buf->line_count - 1) {
            size_t col = cursor->pos - buf->line_offsets[line];
            size_t ns = buf->line_offsets[line+1];
            size_t ne = (line + 2 < buf->line_count) ? buf->line_offsets[line+2] - 1 : buf->total_length;
            size_t nl = ne - ns; cursor->pos = ns + (col < nl ? col : nl);
        }
        pthread_mutex_unlock(&buf->mutex);
    } else if (code == IM_KEY_UP) {
        size_t line = 0; pthread_mutex_lock(&buf->mutex);
        while (line < buf->line_count && buf->line_offsets[line] <= cursor->pos) line++;
        line--;
        if (line > 0) {
            size_t col = cursor->pos - buf->line_offsets[line];
            size_t ps = buf->line_offsets[line-1];
            size_t pe = buf->line_offsets[line] - 1;
            size_t pl = pe - ps; cursor->pos = ps + (col < pl ? col : pl);
        }
        pthread_mutex_unlock(&buf->mutex);
    }
}

static void on_key_press(im_event_t* event, void* user_data) {
    (void)user_data;
    im_key_event_t* key = (im_key_event_t*)event->payload;
    im_editor_state_t* state = im_state_get();
    pthread_mutex_lock(&state->mutex);
    im_cursor_t* cursor = &state->cursors[state->primary_cursor_idx];
    if (state->mode == IM_MODE_NORMAL || state->mode == IM_MODE_VISUAL) {
        if (key->code == IM_KEY_ESC) { state->mode = IM_MODE_NORMAL; cursor->has_selection = false; cursor->anchor = cursor->pos; }
        else if (key->code == IM_KEY_UP || key->code == IM_KEY_DOWN || key->code == IM_KEY_LEFT || key->code == IM_KEY_RIGHT) { handle_motion(key->code, cursor, &g_main_buffer); }
        else if (key->code == IM_KEY_CHAR) {
            if (key->modifiers & IM_MOD_CTRL) {
                if (key->ch == 'u') im_buffer_undo(&g_main_buffer);
                else if (key->ch == 'r') im_buffer_redo(&g_main_buffer);
            } else {
                switch (key->ch) {
                    case ':': state->mode = IM_MODE_COMMAND; state->command_len = 0; state->command_buffer[0] = '\0'; break;
                    case 'i': state->mode = IM_MODE_INSERT; break;
                    case 'a': cursor->pos++; if(cursor->pos > g_main_buffer.total_length) cursor->pos = g_main_buffer.total_length; state->mode = IM_MODE_INSERT; break;
                    case 'v': if (state->mode == IM_MODE_VISUAL) state->mode = IM_MODE_NORMAL; else { state->mode = IM_MODE_VISUAL; cursor->anchor = cursor->pos; cursor->has_selection = true; } break;
                    case 'h': handle_motion(IM_KEY_LEFT, cursor, &g_main_buffer); break;
                    case 'l': handle_motion(IM_KEY_RIGHT, cursor, &g_main_buffer); break;
                    case 'j': handle_motion(IM_KEY_DOWN, cursor, &g_main_buffer); break;
                    case 'k': handle_motion(IM_KEY_UP, cursor, &g_main_buffer); break;
                    case 'w': {
                        char b[64]; size_t offset = cursor->pos; bool f = false;
                        while (offset < g_main_buffer.total_length && !f) {
                            size_t tr = (g_main_buffer.total_length - offset < 64) ? g_main_buffer.total_length - offset : 64;
                            im_buffer_copy_range(&g_main_buffer, offset, tr, b);
                            size_t i = 0; if (offset == cursor->pos) { while (i < tr && isalnum(b[i])) i++; }
                            while (i < tr && !isalnum(b[i])) i++;
                            if (i < tr) { cursor->pos = offset + i; f = true; }
                            offset += tr;
                        }
                        if (!f) cursor->pos = g_main_buffer.total_length;
                        break;
                    }
                    case 'b': {
                        if (cursor->pos == 0) break;
                        char b[64]; long off = (long)cursor->pos; bool f = false;
                        while (off > 0 && !f) {
                            size_t tr = (off < 64) ? off : 64; long start = off - (long)tr;
                            im_buffer_copy_range(&g_main_buffer, (size_t)start, tr, b);
                            int i = (int)tr - 1; if (off == (long)cursor->pos) { while (i >= 0 && !isalnum(b[i])) i--; }
                            while (i >= 0 && isalnum(b[i])) i--;
                            if (i >= -1) { cursor->pos = (size_t)(start + i + 1); f = true; }
                            off -= (long)tr;
                        }
                        break;
                    }
                    case 'x': {
                        if (state->mode == IM_MODE_VISUAL) {
                            size_t sel_min = cursor->pos < cursor->anchor ? cursor->pos : cursor->anchor;
                            size_t sel_max = cursor->pos > cursor->anchor ? cursor->pos : cursor->anchor;
                            im_buffer_delete(&g_main_buffer, sel_min, sel_max - sel_min + 1);
                            cursor->pos = sel_min; cursor->anchor = sel_min; state->mode = IM_MODE_NORMAL;
                        } else {
                            pthread_mutex_lock(&g_main_buffer.mutex);
                            size_t line = 0; while (line < g_main_buffer.line_count - 1 && g_main_buffer.line_offsets[line+1] <= cursor->pos) line++;
                            size_t s = g_main_buffer.line_offsets[line];
                            size_t e = (line + 1 < g_main_buffer.line_count) ? g_main_buffer.line_offsets[line+1] : g_main_buffer.total_length;
                            pthread_mutex_unlock(&g_main_buffer.mutex);
                            im_buffer_delete(&g_main_buffer, s, e - s);
                        }
                        break;
                    }
                    case 'u': im_buffer_undo(&g_main_buffer); break;
                    case 'U': im_buffer_redo(&g_main_buffer); break;
                    case 'd':
                        if (state->mode == IM_MODE_VISUAL) {
                            size_t sel_min = cursor->pos < cursor->anchor ? cursor->pos : cursor->anchor;
                            size_t sel_max = cursor->pos > cursor->anchor ? cursor->pos : cursor->anchor;
                            im_buffer_delete(&g_main_buffer, sel_min, sel_max - sel_min + 1);
                            cursor->pos = sel_min; cursor->anchor = sel_min; state->mode = IM_MODE_NORMAL;
                        }
                        break;
                }
                if (state->mode != IM_MODE_VISUAL && state->mode != IM_MODE_INSERT) { cursor->anchor = cursor->pos; cursor->has_selection = false; }
            }
        }
    } else if (state->mode == IM_MODE_INSERT) {
        if (key->code == IM_KEY_ESC) { state->mode = IM_MODE_NORMAL; cursor->anchor = cursor->pos; cursor->has_selection = false; }
        else if (key->code == IM_KEY_UP || key->code == IM_KEY_DOWN || key->code == IM_KEY_LEFT || key->code == IM_KEY_RIGHT) { handle_motion(key->code, cursor, &g_main_buffer); }
        else if (key->code == IM_KEY_CHAR) { char ch = (char)key->ch; im_buffer_insert(&g_main_buffer, cursor->pos, &ch, 1); cursor->pos++; cursor->anchor = cursor->pos; }
        else if (key->code == IM_KEY_ENTER) { char ch = '\n'; im_buffer_insert(&g_main_buffer, cursor->pos, &ch, 1); cursor->pos++; cursor->anchor = cursor->pos; }
        else if (key->code == IM_KEY_BACKSPACE) { if (cursor->pos > 0) { im_buffer_delete(&g_main_buffer, cursor->pos - 1, 1); cursor->pos--; cursor->anchor = cursor->pos; } }
    } else if (state->mode == IM_MODE_COMMAND) {
        if (key->code == IM_KEY_ESC) state->mode = IM_MODE_NORMAL;
        else if (key->code == IM_KEY_ENTER) {
            char cmd[MAX_COMMAND_LEN]; strncpy(cmd, state->command_buffer, MAX_COMMAND_LEN);
            pthread_mutex_unlock(&state->mutex); im_command_execute(cmd); pthread_mutex_lock(&state->mutex);
            if (g_keep_running) state->mode = IM_MODE_NORMAL;
        } else if (key->code == IM_KEY_CHAR) {
            if (state->command_len < MAX_COMMAND_LEN - 1) { state->command_buffer[state->command_len++] = (char)key->ch; state->command_buffer[state->command_len] = '\0'; }
        } else if (key->code == IM_KEY_BACKSPACE) {
            if (state->command_len > 0) state->command_buffer[--state->command_len] = '\0';
            else state->mode = IM_MODE_NORMAL;
        }
    }
    update_scrolling(state);
    pthread_mutex_unlock(&state->mutex);
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    signal(SIGWINCH, handle_sigwinch);
    if (im_boot_init() != IM_OK) return 1;
    im_config_load("interm.conf");
    im_command_register("q", "Quit editor", cmd_quit);
    im_command_register("w", "Save/Write buffer", cmd_save);
    im_command_register("e", "Edit file", cmd_edit);
    const char* welcome = "INTERM - Helix-style editing enabled.\nhjkl - move\nw,b,e - word motions\nv - visual mode\nd,x - delete\nu,U - undo/redo\na - append\n";
    im_buffer_init(&g_main_buffer, welcome, strlen(welcome));
    im_state_set_active_buffer(&g_main_buffer);
    im_event_subscribe(IM_EVENT_KEY_PRESS, on_key_press, NULL);
    struct pollfd pfd = { STDIN_FILENO, POLLIN, 0 };
    uint8_t buf[64];
    while (g_keep_running) {
        if (g_resize_pending) {
            g_resize_pending = 0;
            im_ui_layout_recompute();
            im_render_invalidate_all();
        }
        int res = poll(&pfd, 1, 10);
        if (res > 0 && (pfd.revents & POLLIN)) {
            ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
            if (n > 0) im_input_process_raw(buf, (size_t)n);
        }
        im_editor_state_t* state = im_state_get();
        pthread_mutex_lock(&state->mutex);
        im_screen_buffer_t* screen = im_render_get_current_buffer();
        im_ui_render_all(screen);
        pthread_mutex_unlock(&state->mutex);
        im_render_frame();
    }
    im_buffer_destroy(&g_main_buffer);
    im_boot_shutdown();
    return 0;
}
