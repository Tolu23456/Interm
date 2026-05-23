#ifndef INTERM_PYTHON_H
#define INTERM_PYTHON_H

#include "common.h"

im_result_t im_python_init();
im_result_t im_python_shutdown();
im_result_t im_python_execute_file(const char* path);
im_result_t im_python_load_plugins(const char* dir);

#endif // INTERM_PYTHON_H
