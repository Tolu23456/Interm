#ifndef INTERM_TERMINAL_H
#define INTERM_TERMINAL_H

#include "common.h"

im_result_t im_terminal_init();
im_result_t im_terminal_shutdown();
im_result_t im_terminal_get_size(int* cols, int* rows);

#endif // INTERM_TERMINAL_H
