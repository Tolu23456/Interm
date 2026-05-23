#ifndef INTERM_STORAGE_H
#define INTERM_STORAGE_H

#include "common.h"
#include "buffer.h"

im_result_t im_storage_save(im_buffer_t* buf, const char* path);
im_result_t im_storage_load(im_buffer_t* buf, const char* path);

#endif // INTERM_STORAGE_H
