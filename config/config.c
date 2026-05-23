#include "config.h"
#include <stdio.h>
im_result_t im_config_init() { printf("  Config System: Initializing...\n"); return IM_OK; }
im_result_t im_config_shutdown() { printf("  Config System: Shutting down...\n"); return IM_OK; }
