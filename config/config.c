#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static im_config_t g_config;

im_result_t im_config_init() {
    printf("  Config System: Initializing...\n");
    // Defaults
    g_config.tab_width = 4;
    g_config.line_numbers = true;
    g_config.word_wrap = false;
    strcpy(g_config.theme_name, "default");
    return IM_OK;
}

im_result_t im_config_shutdown() {
    return IM_OK;
}

im_config_t* im_config_get() {
    return &g_config;
}

static char* trim(char* str) {
    char* end;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

im_result_t im_config_load(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return IM_ERR_IO;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char* l = trim(line);
        if (l[0] == '#' || l[0] == '[' || l[0] == '\0') continue;

        char* key = strtok(l, "=");
        char* val = strtok(NULL, "=");
        if (key && val) {
            key = trim(key);
            val = trim(val);
            if (strcmp(key, "tab_width") == 0) g_config.tab_width = atoi(val);
            else if (strcmp(key, "line_numbers") == 0) g_config.line_numbers = (strcmp(val, "true") == 0);
            else if (strcmp(key, "theme") == 0) strncpy(g_config.theme_name, val, 63);
        }
    }
    fclose(f);
    return IM_OK;
}
