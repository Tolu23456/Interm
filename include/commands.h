#ifndef INTERM_COMMANDS_H
#define INTERM_COMMANDS_H

#include "common.h"

typedef struct {
    const char* name;
    const char* description;
    bool force;
    // Callback: returns IM_OK on success
    im_result_t (*handler)(const char* args, bool force);
} im_command_t;

im_result_t im_command_init();
im_result_t im_command_shutdown();

/**
 * Registers a new command.
 */
im_result_t im_command_register(const char* name, const char* description, im_result_t (*handler)(const char* args, bool force));

/**
 * Executes a raw command string (e.g. "q!", "w filename").
 */
im_result_t im_command_execute(const char* cmd_line);

#endif // INTERM_COMMANDS_H
