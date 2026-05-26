#include "input.h"
#include "event.h"
#include "commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static im_key_node_t* g_root_node = NULL;
static im_key_node_t* g_current_match = NULL;

static void free_key_node(im_key_node_t* node) {
    if (!node) return;
    im_key_node_t* child = node->children;
    while (child) {
        im_key_node_t* next = child->next;
        free_key_node(child);
        child = next;
    }
    if (node->command) free((void*)node->command);
    free(node);
}

im_result_t im_input_init() {
    g_root_node = calloc(1, sizeof(im_key_node_t));
    g_current_match = g_root_node;
    return IM_OK;
}

im_result_t im_input_shutdown() {
    free_key_node(g_root_node);
    g_root_node = NULL;
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

    im_key_node_t* child = g_current_match->children;
    bool found = false;
    while (child) {
        if (child->key.code == code && child->key.ch == ch && child->key.modifiers == mods) {
            if (child->command) {
                im_command_execute(child->command);
                g_current_match = g_root_node;
            } else {
                g_current_match = child;
            }
            found = true;
            break;
        }
        child = child->next;
    }

    if (!found) {
        g_current_match = g_root_node;
        im_event_t ev = {0};
        ev.type = IM_EVENT_KEY_PRESS;
        ev.priority = IM_PRIORITY_CRITICAL;
        ev.timestamp = event.timestamp;
        ev.payload = malloc(sizeof(im_key_event_t));
        if (ev.payload) {
            memcpy(ev.payload, &event, sizeof(im_key_event_t));
            ev.payload_size = sizeof(im_key_event_t);
            im_event_emit(ev);
        }
    }
}

im_result_t im_input_process_raw(const uint8_t* buf, size_t len) {
    size_t i = 0;
    while (i < len) {
        if (buf[i] == 27) {
            if (i + 1 == len) { emit_key(IM_KEY_ESC, 0, IM_MOD_NONE); i++; }
            else if (buf[i+1] == '[') {
                size_t j = i + 2;
                while (j < len && (buf[j] < 64 || buf[j] > 126)) j++;
                if (j < len) {
                    if (buf[i+2] == 'A') emit_key(IM_KEY_UP, 0, IM_MOD_NONE);
                    else if (buf[i+2] == 'B') emit_key(IM_KEY_DOWN, 0, IM_MOD_NONE);
                    else if (buf[i+2] == 'C') emit_key(IM_KEY_RIGHT, 0, IM_MOD_NONE);
                    else if (buf[i+2] == 'D') emit_key(IM_KEY_LEFT, 0, IM_MOD_NONE);
                    i = j + 1;
                } else i = len;
            } else { emit_key(IM_KEY_CHAR, buf[i+1], IM_MOD_ALT); i += 2; }
        } else if (buf[i] == 127 || buf[i] == 8) { emit_key(IM_KEY_BACKSPACE, 0, IM_MOD_NONE); i++; }
        else if (buf[i] == 13 || buf[i] == 10) { emit_key(IM_KEY_ENTER, 0, IM_MOD_NONE); i++; }
        else if (buf[i] == 9) { emit_key(IM_KEY_TAB, 0, IM_MOD_NONE); i++; }
        else if (buf[i] < 32) { emit_key(IM_KEY_CHAR, buf[i] + 96, IM_MOD_CTRL); i++; }
        else { emit_key(IM_KEY_CHAR, buf[i], IM_MOD_NONE); i++; }
    }
    return IM_OK;
}

im_result_t im_input_bind(const char* sequence, const char* command) {
    im_key_node_t* curr = g_root_node;
    im_key_node_t* node = calloc(1, sizeof(im_key_node_t));
    if (!node) return IM_ERR_NOMEM;
    node->key.code = IM_KEY_CHAR;
    node->key.ch = sequence[0];
    node->command = strdup(command);
    node->next = curr->children;
    curr->children = node;
    return IM_OK;
}
