#include "input.h"
#include "event.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

im_result_t im_input_init() {
    printf("  Input System: Initializing...\n");
    return IM_OK;
}

im_result_t im_input_shutdown() {
    printf("  Input System: Shutting down...\n");
    return IM_OK;
}

static uint64_t get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (ts.tv_nsec / 1000000);
}

static void emit_key(im_key_code_t code, uint32_t ch, uint16_t mods) {
    im_key_event_t event;
    event.timestamp = get_time_ms();
    event.code = code;
    event.ch = ch;
    event.modifiers = mods;

    im_event_t ev;
    ev.type = IM_EVENT_KEY_PRESS;
    ev.priority = IM_PRIORITY_CRITICAL;
    ev.timestamp = event.timestamp;
    ev.source = NULL;
    ev.payload = malloc(sizeof(im_key_event_t));
    memcpy(ev.payload, &event, sizeof(im_key_event_t));
    ev.payload_size = sizeof(im_key_event_t);
    im_event_emit(ev);
}

im_result_t im_input_process_raw(const uint8_t* buf, size_t len) {
    if (len == 0) return IM_OK;

    size_t i = 0;
    while (i < len) {
        if (buf[i] == 27) { // ESC
            if (i + 1 == len) {
                emit_key(IM_KEY_ESC, 0, IM_MOD_NONE);
                i++;
            } else if (buf[i+1] == '[') {
                size_t j = i + 2;
                while (j < len && (buf[j] < 64 || buf[j] > 126)) j++;
                if (j < len) {
                    if (buf[i+2] == 'A') emit_key(IM_KEY_UP, 0, IM_MOD_NONE);
                    else if (buf[i+2] == 'B') emit_key(IM_KEY_DOWN, 0, IM_MOD_NONE);
                    else if (buf[i+2] == 'C') emit_key(IM_KEY_RIGHT, 0, IM_MOD_NONE);
                    else if (buf[i+2] == 'D') emit_key(IM_KEY_LEFT, 0, IM_MOD_NONE);
                    else if (buf[i+2] == 'H') emit_key(IM_KEY_HOME, 0, IM_MOD_NONE);
                    else if (buf[i+2] == 'F') emit_key(IM_KEY_END, 0, IM_MOD_NONE);
                    else if (buf[i+2] == '3' && buf[i+3] == '~') emit_key(IM_KEY_DEL, 0, IM_MOD_NONE);
                    else if (buf[i+2] == '5' && buf[i+3] == '~') emit_key(IM_KEY_PAGE_UP, 0, IM_MOD_NONE);
                    else if (buf[i+2] == '6' && buf[i+3] == '~') emit_key(IM_KEY_PAGE_DOWN, 0, IM_MOD_NONE);
                    i = j + 1;
                } else {
                    i = len;
                }
            } else {
                emit_key(IM_KEY_CHAR, buf[i+1], IM_MOD_ALT);
                i += 2;
            }
        } else if (buf[i] == 127 || buf[i] == 8) {
            emit_key(IM_KEY_BACKSPACE, 0, IM_MOD_NONE);
            i++;
        } else if (buf[i] == 13 || buf[i] == 10) {
            emit_key(IM_KEY_ENTER, 0, IM_MOD_NONE);
            i++;
        } else if (buf[i] == 9) {
            emit_key(IM_KEY_TAB, 0, IM_MOD_NONE);
            i++;
        } else if (buf[i] < 32) {
            emit_key(IM_KEY_CHAR, buf[i] + 96, IM_MOD_CTRL);
            i++;
        } else {
            emit_key(IM_KEY_CHAR, buf[i], IM_MOD_NONE);
            i++;
        }
    }

    return IM_OK;
}
