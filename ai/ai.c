#include "ai.h"
#include "state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char* provider_name;
    im_result_t (*request)(const char* prompt, char** response);
} im_ai_provider_t;

static im_ai_provider_t* g_providers[8];
static int g_provider_count = 0;

im_result_t im_ai_init() {

    g_provider_count = 0;
    return IM_OK;
}

im_result_t im_ai_register_provider(const char* name, im_result_t (*req)(const char*, char**)) {
    if (g_provider_count >= 8) return IM_ERR;
    im_ai_provider_t* p = malloc(sizeof(im_ai_provider_t));
    p->provider_name = strdup(name);
    p->request = req;
    g_providers[g_provider_count++] = p;
    return IM_OK;
}

im_result_t im_ai_build_context(char** context) {
    im_editor_state_t* state = im_state_get();
    if (!state->active_buffer) return IM_ERR;

    // Simple context: current buffer content around cursor
    pthread_mutex_lock(&state->mutex);
    size_t pos = state->cursors[state->primary_cursor_idx].pos;
    pthread_mutex_unlock(&state->mutex);

    size_t start = (pos > 512) ? pos - 512 : 0;
    size_t len = 1024;
    if (start + len > state->active_buffer->total_length) len = state->active_buffer->total_length - start;

    *context = im_buffer_get_range(state->active_buffer, start, len);
    return IM_OK;
}

im_result_t im_ai_shutdown() {
    for (int i = 0; i < g_provider_count; i++) {
        free(g_providers[i]->provider_name);
        free(g_providers[i]);
    }
    return IM_OK;
}
