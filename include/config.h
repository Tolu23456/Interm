#ifndef INTERM_CONFIG_H
#define INTERM_CONFIG_H

#include "common.h"

typedef struct {
    int tab_width;
    bool line_numbers;
    bool word_wrap;
    char theme_name[64];
} im_config_t;

im_result_t im_config_init();
im_result_t im_config_shutdown();
im_config_t* im_config_get();
im_result_t im_config_load(const char* path);

#endif // INTERM_CONFIG_H
