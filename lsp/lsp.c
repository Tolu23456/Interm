#include "lsp.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

typedef struct {
    char* server_name;
    int stdin_pipe[2];
    int stdout_pipe[2];
    pid_t pid;
    bool active;
} im_lsp_server_t;

static im_lsp_server_t* g_servers[16];
static int g_server_count = 0;

im_result_t im_lsp_init() {

    g_server_count = 0;
    return IM_OK;
}

im_result_t im_lsp_start_server(const char* name, const char* cmd) {
    (void)cmd;
    if (g_server_count >= 16) return IM_ERR;
    im_lsp_server_t* s = calloc(1, sizeof(im_lsp_server_t));
    if (!s) return IM_ERR_NOMEM;
    s->server_name = strdup(name);
    if (pipe(s->stdin_pipe) == -1 || pipe(s->stdout_pipe) == -1) {
        free(s->server_name); free(s); return IM_ERR_IO;
    }
    s->pid = fork();
    if (s->pid == 0) {
        dup2(s->stdin_pipe[0], STDIN_FILENO);
        dup2(s->stdout_pipe[1], STDOUT_FILENO);
        close(s->stdin_pipe[1]); close(s->stdout_pipe[0]);
        char* argv[] = {"/usr/bin/python3", "-m", "pylsp", NULL};
        execvp(argv[0], argv);
        exit(1);
    }
    close(s->stdin_pipe[0]); close(s->stdout_pipe[1]);
    s->active = true;
    g_servers[g_server_count++] = s;
    return IM_OK;
}

im_result_t im_lsp_shutdown() {
    for (int i = 0; i < g_server_count; i++) {
        kill(g_servers[i]->pid, SIGTERM);
        waitpid(g_servers[i]->pid, NULL, 0);
        free(g_servers[i]->server_name);
        free(g_servers[i]);
    }
    return IM_OK;
}
