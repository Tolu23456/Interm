#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include "boot.h"
#include "input.h"
#include "event.h"
#include "buffer.h"
#include "state.h"
#include "renderer.h"
#include "commands.h"
#include "storage.h"

static bool g_keep_running = true;
static im_buffer_t g_main_buffer;

static im_result_t cmd_quit(const char* args, bool force) {
    (void)args;
    // In a real editor, check for unsaved changes unless force is true
    if (!force) {
        // TODO: Check dirty flag
    }
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
    }
    return res;
}

static im_result_t cmd_edit(const char* args, bool force) {
    (void)force;
    if (!args || strlen(args) == 0) return IM_ERR_INVALID_ARG;

    im_buffer_t new_buf;
    if (im_storage_load(&new_buf, args) == IM_OK) {
        im_buffer_destroy(&g_main_buffer);
        g_main_buffer = new_buf;
        im_state_set_active_buffer(&g_main_buffer);
        strncpy(g_active_file, args, sizeof(g_active_file)-1);

        im_editor_state_t* state = im_state_get();
        state->cursors[state->primary_cursor_idx].pos = 0;
        return IM_OK;
    }
    return IM_ERR;
}

static void on_key_press(im_event_t* event, void* user_data) {
    (void)user_data;
    im_key_event_t* key = (im_key_event_t*)event->payload;
    im_editor_state_t* state = im_state_get();
    im_cursor_t* cursor = &state->cursors[state->primary_cursor_idx];

    if (state->mode == IM_MODE_NORMAL) {
        if (key->code == IM_KEY_CHAR) {
            if (key->modifiers & IM_MOD_CTRL) {
                if (key->ch == 'u') im_buffer_undo(&g_main_buffer);
                else if (key->ch == 'r') im_buffer_redo(&g_main_buffer);
            } else {
                switch (key->ch) {
                    case ':':
                        im_state_set_mode(IM_MODE_COMMAND);
                        state->command_len = 0;
                        state->command_buffer[0] = '\0';
                        break;
                    case 'i': im_state_set_mode(IM_MODE_INSERT); break;
                    case 'h': if (cursor->pos > 0) cursor->pos--; break;
                    case 'l': if (cursor->pos < g_main_buffer.total_length) cursor->pos++; break;
                    case 'j': {
                        size_t line = 0;
                        while (line < g_main_buffer.line_count - 1 && g_main_buffer.line_offsets[line+1] <= cursor->pos) line++;
                        if (line < g_main_buffer.line_count - 1) {
                            size_t col = cursor->pos - g_main_buffer.line_offsets[line];
                            size_t next_line_start = g_main_buffer.line_offsets[line+1];
                            size_t next_line_end = (line + 2 < g_main_buffer.line_count) ? g_main_buffer.line_offsets[line+2] - 1 : g_main_buffer.total_length;
                            size_t next_line_len = next_line_end - next_line_start;
                            cursor->pos = next_line_start + (col < next_line_len ? col : next_line_len);
                        }
                        break;
                    }
                    case 'k': {
                        size_t line = 0;
                        while (line < g_main_buffer.line_count && g_main_buffer.line_offsets[line] <= cursor->pos) line++;
                        line--; // Current line
                        if (line > 0) {
                            size_t col = cursor->pos - g_main_buffer.line_offsets[line];
                            size_t prev_line_start = g_main_buffer.line_offsets[line-1];
                            size_t prev_line_end = g_main_buffer.line_offsets[line] - 1;
                            size_t prev_line_len = prev_line_end - prev_line_start;
                            cursor->pos = prev_line_start + (col < prev_line_len ? col : prev_line_len);
                        }
                        break;
                    }
                    case 'x': im_buffer_delete(&g_main_buffer, cursor->pos, 1); break;
                    case 'u': im_buffer_undo(&g_main_buffer); break;
                }
            }
        }
    } else if (state->mode == IM_MODE_INSERT) {
        if (key->code == IM_KEY_ESC) {
            im_state_set_mode(IM_MODE_NORMAL);
        } else if (key->code == IM_KEY_CHAR) {
            char ch = (char)key->ch;
            im_buffer_insert(&g_main_buffer, cursor->pos, &ch, 1);
            cursor->pos++;
        } else if (key->code == IM_KEY_ENTER) {
            char ch = '\n';
            im_buffer_insert(&g_main_buffer, cursor->pos, &ch, 1);
            cursor->pos++;
        }
    } else if (state->mode == IM_MODE_COMMAND) {
        if (key->code == IM_KEY_ESC) {
            im_state_set_mode(IM_MODE_NORMAL);
        } else if (key->code == IM_KEY_ENTER) {
            im_command_execute(state->command_buffer);
            if (g_keep_running) im_state_set_mode(IM_MODE_NORMAL);
        } else if (key->code == IM_KEY_CHAR) {
            if (state->command_len < MAX_COMMAND_LEN - 1) {
                state->command_buffer[state->command_len++] = (char)key->ch;
                state->command_buffer[state->command_len] = '\0';
            }
        } else if (key->code == IM_KEY_BACKSPACE) {
            if (state->command_len > 0) {
                state->command_buffer[--state->command_len] = '\0';
            } else {
                im_state_set_mode(IM_MODE_NORMAL);
            }
        }
    }
}

/**
 * INTERM - In-Terminal Editor
 * Entry Point
 */
int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    if (im_boot_init() != IM_OK) {
        return 1;
    }
    
    im_command_register("q", "Quit editor", cmd_quit);
    im_command_register("w", "Save/Write buffer", cmd_save);
    im_command_register("e", "Edit file", cmd_edit);

    im_buffer_init(&g_main_buffer, "Welcome to INTERM!\nType ':' then 'q' to quit.\n", 48);
    im_state_set_active_buffer(&g_main_buffer);
    
    im_event_subscribe(IM_EVENT_KEY_PRESS, on_key_press, NULL);
    
    uint8_t buf[16];
    while (g_keep_running) {
        ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
        if (n > 0) {
            im_input_process_raw(buf, n);
        }
        
        im_render_frame();
        
        usleep(10000); // 10ms
    }
    
    im_buffer_destroy(&g_main_buffer);
    im_boot_shutdown();
    return 0;
}
