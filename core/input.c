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

im_result_t im_input_process_raw(const uint8_t* buf, size_t len) {
    if (len == 0) return IM_OK;

    im_key_event_t event;
    event.timestamp = get_time_ms();
    event.modifiers = IM_MOD_NONE;
    event.ch = 0;

    // Very basic translation for now (will need full ANSI sequence parser)
    if (len == 1) {
        if (buf[0] == 27) {
            event.code = IM_KEY_ESC;
        } else if (buf[0] == 13 || buf[0] == 10) {
            event.code = IM_KEY_ENTER;
        } else if (buf[0] == 127 || buf[0] == 8) {
            event.code = IM_KEY_BACKSPACE;
        } else if (buf[0] == 9) {
            event.code = IM_KEY_TAB;
        } else if (buf[0] < 32) {
            // Potential Ctrl + key
            event.code = IM_KEY_CHAR;
            event.ch = buf[0] + 96;
            event.modifiers |= IM_MOD_CTRL;
        } else {
            event.code = IM_KEY_CHAR;
            event.ch = buf[0];
        }
    } else if (len >= 3 && buf[0] == 27 && buf[1] == '[') {
        // Simple escape sequences like arrows
        switch (buf[2]) {
            case 'A': event.code = IM_KEY_UP; break;
            case 'B': event.code = IM_KEY_DOWN; break;
            case 'C': event.code = IM_KEY_RIGHT; break;
            case 'D': event.code = IM_KEY_LEFT; break;
            default: event.code = IM_KEY_UNKNOWN; break;
        }
    } else {
        event.code = IM_KEY_UNKNOWN;
    }

    if (event.code != IM_KEY_UNKNOWN) {
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

    return IM_OK;
}
