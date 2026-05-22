#include "commands.h"
#include "state.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_REG_COMMANDS 128

static im_command_t g_commands[MAX_REG_COMMANDS];
static int g_command_count = 0;

im_result_t im_command_init() {
    printf("  Command System: Initializing...\n");
    g_command_count = 0;
    return IM_OK;
}

im_result_t im_command_shutdown() {
    printf("  Command System: Shutting down...\n");
    return IM_OK;
}

im_result_t im_command_register(const char* name, const char* description, im_result_t (*handler)(const char* args, bool force)) {
    if (g_command_count >= MAX_REG_COMMANDS) return IM_ERR;
    
    g_commands[g_command_count].name = name;
    g_commands[g_command_count].description = description;
    g_commands[g_command_count].handler = handler;
    g_command_count++;
    
    return IM_OK;
}

im_result_t im_command_execute(const char* cmd_line) {
    if (!cmd_line || strlen(cmd_line) == 0) return IM_OK;

    char full_cmd[MAX_COMMAND_LEN];
    strncpy(full_cmd, cmd_line, MAX_COMMAND_LEN - 1);
    
    // Parse force modifier
    bool force = false;
    char* bang = strchr(full_cmd, '!');
    if (bang) {
        force = true;
        *bang = '\0'; // Split
    }

    char* name = strtok(full_cmd, " ");
    char* args = strtok(NULL, "");

    for (int i = 0; i < g_command_count; i++) {
        if (strcmp(g_commands[i].name, name) == 0) {
            return g_commands[i].handler(args, force);
        }
    }

    printf("  Command: Unknown command '%s'\n", name);
    return IM_ERR_NOT_FOUND;
}
