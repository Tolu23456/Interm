#include "buffer.h"
#include <stdio.h>
#include <stdlib.h>

im_result_t im_storage_save(im_buffer_t* buf, const char* path) {
    FILE* f = fopen(path, "w");
    if (!f) return IM_ERR_IO;

    im_piece_t* curr = buf->head;
    while (curr) {
        const char* data = (curr->source == IM_SOURCE_ORIGINAL) ? buf->original_data : buf->add_data;
        fwrite(data + curr->start, 1, curr->length, f);
        curr = (im_piece_t*)curr->next;
    }

    fclose(f);
    return IM_OK;
}

im_result_t im_storage_load(im_buffer_t* buf, const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return IM_ERR_IO;

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
