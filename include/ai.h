#ifndef INTERM_AI_H
#define INTERM_AI_H

#include "common.h"

im_result_t im_ai_init();
im_result_t im_ai_shutdown();
im_result_t im_ai_register_provider(const char* name, im_result_t (*req)(const char*, char**));
im_result_t im_ai_build_context(char** context);

#endif // INTERM_AI_H
