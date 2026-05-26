#include "git.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char current_branch[64];
    int dirty_count;
} im_git_status_t;

static im_git_status_t g_git_status;

im_result_t im_git_init() {

    g_git_status.current_branch[0] = '\0';
    g_git_status.dirty_count = 0;
    return IM_OK;
}

im_result_t im_git_refresh_status() {
    FILE* fp = popen("git rev-parse --abbrev-ref HEAD 2>/dev/null", "r");
    if (fp) {
        if (fgets(g_git_status.current_branch, 63, fp)) {
            char* nl = strchr(g_git_status.current_branch, '\n');
            if (nl) *nl = '\0';
        }
        pclose(fp);
    }
    return IM_OK;
}

im_result_t im_git_shutdown() {
    return IM_OK;
}
