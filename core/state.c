#include "state.h"
#include "event.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static im_editor_state_t g_state;
static im_editor_state_t* g_history[1024];
static int g_history_idx = 0;

static void snapshot_state() {
    if (g_history_idx < 1024) {
        im_editor_state_t* snapshot = malloc(sizeof(im_editor_state_t));
        pthread_mutex_lock(&g_state.mutex);
        memcpy(snapshot, &g_state, sizeof(im_editor_state_t));
        snapshot->cursors = malloc(snapshot->cursor_capacity * sizeof(im_cursor_t));
        memcpy(snapshot->cursors, g_state.cursors, snapshot->cursor_count * sizeof(im_cursor_t));
        pthread_mutex_unlock(&g_state.mutex);
        g_history[g_history_idx++] = snapshot;
    }
}

im_result_t im_state_init() {
    memset(&g_state, 0, sizeof(im_editor_state_t));
    pthread_mutex_init(&g_state.mutex, NULL);
    g_state.mode = IM_MODE_NORMAL;
    g_state.cursor_capacity = 32;
    g_state.cursors = (im_cursor_t*)calloc(g_state.cursor_capacity, sizeof(im_cursor_t));
    g_state.cursor_count = 1;
    g_state.primary_cursor_idx = 0;
    g_state.dirty = true;
    return IM_OK;
}

im_result_t im_state_shutdown() {
    pthread_mutex_lock(&g_state.mutex);
    free(g_state.cursors);
    pthread_mutex_unlock(&g_state.mutex);
    pthread_mutex_destroy(&g_state.mutex);
    for (int i = 0; i < g_history_idx; i++) {
        free(g_history[i]->cursors);
        free(g_history[i]);
    }
    return IM_OK;
}

im_editor_state_t* im_state_get() {
    return &g_state;
}

im_result_t im_state_set_mode(im_mode_t mode) {
    snapshot_state();
    pthread_mutex_lock(&g_state.mutex);
    if (g_state.mode == mode) {
        pthread_mutex_unlock(&g_state.mutex);
        return IM_OK;
    }
    g_state.mode = mode;
    g_state.dirty = true;
    pthread_mutex_unlock(&g_state.mutex);
    return IM_OK;
}

im_result_t im_state_set_active_buffer(im_buffer_t* buf) {
    pthread_mutex_lock(&g_state.mutex);
    g_state.active_buffer = buf;
    g_state.dirty = true;
    pthread_mutex_unlock(&g_state.mutex);
    return IM_OK;
}

im_result_t im_state_mark_dirty() {
    pthread_mutex_lock(&g_state.mutex);
    g_state.dirty = true;
    pthread_mutex_unlock(&g_state.mutex);
    return IM_OK;
}
