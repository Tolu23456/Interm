#ifndef INTERM_INPUT_H
#define INTERM_INPUT_H

#include "common.h"

typedef enum {
    IM_KEY_UNKNOWN = 0,
    IM_KEY_CHAR,
    IM_KEY_ESC,
    IM_KEY_ENTER,
    IM_KEY_BACKSPACE,
    IM_KEY_TAB,
    IM_KEY_UP,
    IM_KEY_DOWN,
    IM_KEY_LEFT,
    IM_KEY_RIGHT,
    IM_KEY_HOME,
    IM_KEY_END,
    IM_KEY_PAGE_UP,
    IM_KEY_PAGE_DOWN,
    IM_KEY_DEL,
    IM_KEY_F1, IM_KEY_F2, IM_KEY_F3, IM_KEY_F4, IM_KEY_F5,
    IM_KEY_F6, IM_KEY_F7, IM_KEY_F8, IM_KEY_F9, IM_KEY_F10,
    IM_KEY_F11, IM_KEY_F12,
} im_key_code_t;

typedef enum {
    IM_MOD_NONE = 0,
    IM_MOD_SHIFT = 1 << 0,
    IM_MOD_CTRL = 1 << 1,
    IM_MOD_ALT = 1 << 2,
    IM_MOD_META = 1 << 3,
} im_modifier_t;

typedef struct {
    im_key_code_t code;
    uint32_t ch;        // Unicode codepoint for IM_KEY_CHAR
    uint16_t modifiers; // Bitmask of im_modifier_t
    uint64_t timestamp;
} im_key_event_t;

im_result_t im_input_init();
im_result_t im_input_shutdown();

/**
 * Processes raw bytes from terminal and emits key events.
 */
im_result_t im_input_process_raw(const uint8_t* buf, size_t len);

#endif // INTERM_INPUT_H
