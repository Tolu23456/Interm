#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include "boot.h"
#include "input.h"
#include "event.h"
#include "buffer.h"
#include "state.h"
#include "renderer.h"
#include "commands.h"

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

static im_result_t cmd_save(const char* args, bool force) {
    (void)args; (void)force;
    printf("  Command: Saving buffer... (Not fully implemented)\n");
    // TODO: Implement actual file writing
    return IM_OK;
}

static void on_key_press(im_event_t* event, void* user_data) {
    (void)user_data;
    im_key_event_t* key = (im_key_event_t*)event->payload;
    im_editor_state_t* state = im_state_get();
    im_cursor_t* cursor = &state->cursors[state->primary_cursor_idx];

    if (state->mode == IM_MODE_NORMAL) {
        if (key->code == IM_KEY_CHAR) {
            switch (key->ch) {
                case ':': 
                    im_state_set_mode(IM_MODE_COMMAND);
                    state->command_len = 0;
                    state->command_buffer[0] = '\0';
                    break;
                case 'q': // Keeping 'q' for now but user wants :q
                    // g_keep_running = false; 
                    break;
                case 'i': im_state_set_mode(IM_MODE_INSERT); break;
                case 'h': if (cursor->pos > 0) cursor->pos--; break;
                case 'l': if (cursor->pos < g_main_buffer.total_length) cursor->pos++; break;
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
    im_command_register("s", "Save/Write buffer", cmd_save);

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
