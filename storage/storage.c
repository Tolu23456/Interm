#include "buffer.h"
#include "vfs.h"
#include <stdio.h>
#include <stdlib.h>

im_result_t im_storage_save(im_buffer_t* buf, const char* path) {
    im_handle_t handle;
    if (im_vfs_open(path, &handle) != IM_OK) return IM_ERR_IO;

    // We need to implement write in vfs.c or use a higher level.
    // Assuming we stick to simple for now but using the handle.
    FILE* f = (FILE*)handle;

    pthread_mutex_lock(&buf->mutex);
    im_piece_t* curr = buf->head;
    while (curr) {
        const char* data = (curr->source == IM_SOURCE_ORIGINAL) ? buf->original_data : buf->add_data;
        fwrite(data + curr->start, 1, curr->length, f);
        curr = (im_piece_t*)curr->next;
    }
    pthread_mutex_unlock(&buf->mutex);

    fclose(f);
    return IM_OK;
}

im_result_t im_storage_load(im_buffer_t* buf, const char* path) {
    im_handle_t handle;
    if (im_vfs_open(path, &handle) != IM_OK) return IM_ERR_IO;
    FILE* f = (FILE*)handle;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* data = (char*)malloc(size);
    if (fread(data, 1, size, f) != (size_t)size) {
        free(data);
        fclose(f);
        return IM_ERR_IO;
    }
    fclose(f);

    im_result_t res = im_buffer_init(buf, data, size);
    free(data);
    return res;
}
