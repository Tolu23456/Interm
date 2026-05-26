#ifndef INTERM_STATE_H
#define INTERM_STATE_H

#include "common.h"
#include "buffer.h"
#include <pthread.h>

typedef enum {
    IM_MODE_NORMAL,
    IM_MODE_INSERT,
    IM_MODE_VISUAL,
    IM_MODE_COMMAND,
} im_mode_t;

typedef struct {
    size_t pos;        // Byte offset in buffer
    size_t anchor;     // Selection anchor offset
    bool has_selection;
    size_t line;       // Cached line number
    size_t col;        // Cached column number
    size_t want_col;   // Desired column for vertical movement
} im_cursor_t;

#define MAX_CURSORS 1024
#define MAX_COMMAND_LEN 256

typedef struct {
    im_mode_t mode;
    im_buffer_t* active_buffer;
    
    im_cursor_t* cursors;
    size_t cursor_count;
    uint32_t cursor_capacity;
    size_t primary_cursor_idx;

    char command_buffer[MAX_COMMAND_LEN];
    size_t command_len;

    size_t scroll_row;
    size_t scroll_col;

    bool dirty;

    pthread_mutex_t mutex;
} im_editor_state_t;

im_result_t im_state_init();
im_result_t im_state_shutdown();

im_editor_state_t* im_state_get();

im_result_t im_state_set_mode(im_mode_t mode);
im_result_t im_state_set_active_buffer(im_buffer_t* buf);
im_result_t im_state_mark_dirty();

#endif // INTERM_STATE_H
