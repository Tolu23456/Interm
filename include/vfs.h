#ifndef INTERM_VFS_H
#define INTERM_VFS_H

#include "common.h"

typedef struct {
    const char* scheme;
    im_result_t (*open)(const char* path, im_handle_t* handle);
    im_result_t (*read)(im_handle_t handle, char* buf, size_t size, size_t* read_bytes);
    im_result_t (*write)(im_handle_t handle, const char* buf, size_t size);
    im_result_t (*close)(im_handle_t handle);
} im_vfs_provider_t;

im_result_t im_vfs_init();
im_result_t im_vfs_register_provider(im_vfs_provider_t* provider);
im_result_t im_vfs_open(const char* path, im_handle_t* handle);

#endif // INTERM_VFS_H
