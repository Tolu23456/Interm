#include "vfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static im_result_t local_open(const char* path, im_handle_t* handle) {
    FILE* f = fopen(path, "r+");
    if (!f) f = fopen(path, "w+");
    if (!f) return IM_ERR_IO;
    *handle = (im_handle_t)f;
    return IM_OK;
}

static im_result_t local_read(im_handle_t handle, char* buf, size_t size, size_t* read_bytes) {
    FILE* f = (FILE*)handle;
    *read_bytes = fread(buf, 1, size, f);
    return IM_OK;
}

static im_result_t local_write(im_handle_t handle, const char* buf, size_t size) {
    FILE* f = (FILE*)handle;
    rewind(f);
    if (ftruncate(fileno(f), 0) != 0) return IM_ERR_IO;
    fwrite(buf, 1, size, f);
    return IM_OK;
}

static im_result_t local_close(im_handle_t handle) {
    FILE* f = (FILE*)handle;
    fclose(f);
    return IM_OK;
}

static im_vfs_provider_t g_local_provider = {
    "file://", local_open, local_read, local_write, local_close
};

#define MAX_PROVIDERS 16
static im_vfs_provider_t* g_providers[MAX_PROVIDERS];
static int g_provider_count = 0;

im_result_t im_vfs_init() {
    g_provider_count = 0;
    im_vfs_register_provider(&g_local_provider);
    return IM_OK;
}

im_result_t im_vfs_register_provider(im_vfs_provider_t* provider) {
    if (g_provider_count >= MAX_PROVIDERS) return IM_ERR;
    g_providers[g_provider_count++] = provider;
    return IM_OK;
}

im_result_t im_vfs_open(const char* path, im_handle_t* handle) {
    for (int i = 0; i < g_provider_count; i++) {
        if (strncmp(path, g_providers[i]->scheme, strlen(g_providers[i]->scheme)) == 0) {
            return g_providers[i]->open(path, handle);
        }
    }
    return local_open(path, handle);
}
