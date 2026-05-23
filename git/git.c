#include "git.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    char* current_branch;
    int dirty_count;
} im_git_status_t;

static im_git_status_t g_git_status;

im_result_t im_git_init() {
    printf("  Git Integration: Initializing repository observer...\n");
    g_git_status.current_branch = NULL;
    g_git_status.dirty_count = 0;
    return IM_OK;
}

im_result_t im_git_shutdown() {
    printf("  Git Integration: Shutting down...\n");
    free(g_git_status.current_branch);
    return IM_OK;
}
