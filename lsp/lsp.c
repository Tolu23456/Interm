#include "lsp.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct {
    char* server_name;
    bool active;
} im_lsp_server_t;

static im_lsp_server_t* g_servers[16];
static int g_server_count = 0;

im_result_t im_lsp_init() {
    printf("  LSP System: Initializing Multiplexer...\n");
    g_server_count = 0;
    return IM_OK;
}

im_result_t im_lsp_shutdown() {
    printf("  LSP System: Shutting down...\n");
    for (int i = 0; i < g_server_count; i++) {
        free(g_servers[i]->server_name);
        free(g_servers[i]);
    }
    return IM_OK;
}
