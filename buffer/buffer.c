#include "buffer.h"
#include <stdlib.h>
#include <string.h>

static im_piece_t* create_piece(im_piece_source_t source, size_t start, size_t length) {
    im_piece_t* piece = (im_piece_t*)malloc(sizeof(im_piece_t));
    piece->source = source;
    piece->start = start;
    piece->length = length;
    piece->next = NULL;
    piece->prev = NULL;
    return piece;
}

static void update_line_indexing(im_buffer_t* buf) {
    if (!buf->line_offsets) {
        buf->line_capacity = 1024;
        buf->line_offsets = (size_t*)malloc(buf->line_capacity * sizeof(size_t));
    }
    buf->line_count = 0;
    buf->line_offsets[buf->line_count++] = 0;
    im_piece_t* curr = buf->head;
    size_t total_offset = 0;
    while (curr) {
        const char* data = (curr->source == IM_SOURCE_ORIGINAL) ? buf->original_data : buf->add_data;
        for (size_t i = 0; i < curr->length; i++) {
            if (data[curr->start + i] == '\n') {
                if (buf->line_count >= buf->line_capacity) {
                    buf->line_capacity *= 2;
                    buf->line_offsets = (size_t*)realloc(buf->line_offsets, buf->line_capacity * sizeof(size_t));
                }
                buf->line_offsets[buf->line_count++] = total_offset + i + 1;
            }
        }
        total_offset += curr->length;
        curr = (im_piece_t*)curr->next;
    }
}

im_result_t im_buffer_init(im_buffer_t* buf, const char* initial_text, size_t size) {
    memset(buf, 0, sizeof(im_buffer_t));
    pthread_mutex_init(&buf->mutex, NULL);
    if (initial_text && size > 0) {
        buf->original_data = (char*)malloc(size);
        memcpy(buf->original_data, initial_text, size);
        buf->original_size = size;
        buf->head = create_piece(IM_SOURCE_ORIGINAL, 0, size);
        buf->total_length = size;
    }
    buf->add_capacity = 4096;
    buf->add_data = (char*)malloc(buf->add_capacity);
    update_line_indexing(buf);
    return IM_OK;
}

im_result_t im_buffer_destroy(im_buffer_t* buf) {
    if (!buf) return IM_ERR;
    pthread_mutex_lock(&buf->mutex);
    im_piece_t* curr = buf->head;
    while (curr) {
        im_piece_t* next = (im_piece_t*)curr->next;
        free(curr);
        curr = next;
    }
    free(buf->original_data);
    free(buf->add_data);
    free(buf->line_offsets);
    while (buf->undo_stack) {
        im_undo_record_t* next = buf->undo_stack->next;
        free(buf->undo_stack->text);
        free(buf->undo_stack);
        buf->undo_stack = next;
    }
    while (buf->redo_stack) {
        im_undo_record_t* next = buf->redo_stack->next;
        free(buf->redo_stack->text);
        free(buf->redo_stack);
        buf->redo_stack = next;
    }
    pthread_mutex_unlock(&buf->mutex);
    pthread_mutex_destroy(&buf->mutex);
    return IM_OK;
}

static size_t append_to_add_buffer(im_buffer_t* buf, const char* text, size_t len) {
    if (buf->add_size + len > buf->add_capacity) {
        buf->add_capacity = (buf->add_size + len) * 2;
        buf->add_data = (char*)realloc(buf->add_data, buf->add_capacity);
    }
    size_t start = buf->add_size;
    memcpy(buf->add_data + start, text, len);
    buf->add_size += len;
    return start;
}

size_t im_buffer_copy_range(im_buffer_t* buf, size_t pos, size_t len, char* dest) {
    pthread_mutex_lock(&buf->mutex);
    if (pos + len > buf->total_length) {
        pthread_mutex_unlock(&buf->mutex);
        return 0;
    }
    im_piece_t* curr = buf->head;
    size_t offset = 0;
    while (curr && offset + curr->length <= pos) {
        if (offset + curr->length == pos && curr->next) break;
        offset += curr->length;
        curr = (im_piece_t*)curr->next;
    }
    size_t bytes_copied = 0;
    size_t local_offset = pos - offset;
    while (curr && bytes_copied < len) {
        size_t to_copy = curr->length - local_offset;
        if (bytes_copied + to_copy > len) to_copy = len - bytes_copied;
        const char* source_data = (curr->source == IM_SOURCE_ORIGINAL) ? buf->original_data : buf->add_data;
        memcpy(dest + bytes_copied, source_data + curr->start + local_offset, to_copy);
        bytes_copied += to_copy;
        local_offset = 0;
        curr = (im_piece_t*)curr->next;
    }
    pthread_mutex_unlock(&buf->mutex);
    return bytes_copied;
}

char* im_buffer_get_range(im_buffer_t* buf, size_t pos, size_t len) {
    char* result = (char*)malloc(len + 1);
    if (!result) return NULL;
    size_t copied = im_buffer_copy_range(buf, pos, len, result);
    result[copied] = '\0';
    return result;
}

size_t im_buffer_get_line_count(im_buffer_t* buf) {
    pthread_mutex_lock(&buf->mutex);
    size_t count = buf->line_count;
    pthread_mutex_unlock(&buf->mutex);
    return count;
}

static void push_undo(im_buffer_t* buf, im_edit_type_t type, size_t pos, const char* text, size_t len) {
    im_undo_record_t* rec = (im_undo_record_t*)malloc(sizeof(im_undo_record_t));
    rec->type = type;
    rec->pos = pos;
    rec->len = len;
    rec->text = (char*)malloc(len + 1);
    memcpy(rec->text, text, len);
    rec->text[len] = '\0';
    rec->next = buf->undo_stack;
    buf->undo_stack = rec;
    while (buf->redo_stack) {
        im_undo_record_t* next = buf->redo_stack->next;
        free(buf->redo_stack->text);
        free(buf->redo_stack);
        buf->redo_stack = next;
    }
}

static im_result_t im_buffer_insert_internal(im_buffer_t* buf, size_t pos, const char* text, size_t len) {
    if (pos > buf->total_length) return IM_ERR_INVALID_ARG;
    if (len == 0) return IM_OK;
    size_t add_start = append_to_add_buffer(buf, text, len);
    im_piece_t* new_piece = create_piece(IM_SOURCE_ADD, add_start, len);
    if (buf->head == NULL) {
        buf->head = new_piece;
        buf->total_length = len;
        update_line_indexing(buf);
        return IM_OK;
    }
    if (pos == 0) {
        new_piece->next = (struct im_piece_t*)buf->head;
        buf->head->prev = (struct im_piece_t*)new_piece;
        buf->head = new_piece;
    } else if (pos == buf->total_length) {
        im_piece_t* curr = buf->head;
        while (curr->next) curr = (im_piece_t*)curr->next;
        curr->next = (struct im_piece_t*)new_piece;
        new_piece->prev = (struct im_piece_t*)curr;
    } else {
        im_piece_t* curr = buf->head;
        size_t offset = 0;
        while (curr && offset + curr->length < pos) {
            offset += curr->length;
            curr = (im_piece_t*)curr->next;
        }
        size_t local_offset = pos - offset;
        if (local_offset == 0) {
            new_piece->next = (struct im_piece_t*)curr;
            new_piece->prev = curr->prev;
            if (curr->prev) ((im_piece_t*)curr->prev)->next = (struct im_piece_t*)new_piece;
            curr->prev = (struct im_piece_t*)new_piece;
        } else if (local_offset == curr->length) {
            new_piece->next = curr->next;
            new_piece->prev = (struct im_piece_t*)curr;
            if (curr->next) ((im_piece_t*)curr->next)->prev = (struct im_piece_t*)new_piece;
            curr->next = (struct im_piece_t*)new_piece;
        } else {
            im_piece_t* second_half = create_piece(curr->source, curr->start + local_offset, curr->length - local_offset);
            curr->length = local_offset;
            second_half->next = curr->next;
            if (curr->next) ((im_piece_t*)curr->next)->prev = (struct im_piece_t*)second_half;
            curr->next = (struct im_piece_t*)new_piece;
            new_piece->prev = (struct im_piece_t*)curr;
            new_piece->next = (struct im_piece_t*)second_half;
            second_half->prev = (struct im_piece_t*)new_piece;
        }
    }
    buf->total_length += len;
    update_line_indexing(buf);
    return IM_OK;
}

static im_result_t im_buffer_delete_internal(im_buffer_t* buf, size_t pos, size_t len) {
    if (pos + len > buf->total_length) return IM_ERR_INVALID_ARG;
    if (len == 0) return IM_OK;
    im_piece_t* curr = buf->head;
    size_t offset = 0;
    while (curr && offset + curr->length <= pos) {
        if (offset + curr->length == pos && curr->next) break;
        offset += curr->length;
        curr = (im_piece_t*)curr->next;
    }
    size_t remaining_len = len;
    while (curr && remaining_len > 0) {
        size_t local_offset = pos > offset ? pos - offset : 0;
        size_t available = curr->length - local_offset;
        size_t to_delete = (available < remaining_len) ? available : remaining_len;
        if (local_offset == 0 && to_delete == curr->length) {
            im_piece_t* to_free = curr;
            if (to_free->prev) ((im_piece_t*)to_free->prev)->next = to_free->next;
            else buf->head = (im_piece_t*)to_free->next;
            if (to_free->next) ((im_piece_t*)to_free->next)->prev = to_free->prev;
            curr = (im_piece_t*)to_free->next;
            free(to_free);
        } else if (local_offset == 0) {
            curr->start += to_delete;
            curr->length -= to_delete;
            curr = (im_piece_t*)curr->next;
        } else if (local_offset + to_delete == curr->length) {
            curr->length -= to_delete;
            curr = (im_piece_t*)curr->next;
        } else {
            im_piece_t* second_half = create_piece(curr->source, curr->start + local_offset + to_delete, curr->length - (local_offset + to_delete));
            curr->length = local_offset;
            second_half->next = curr->next;
            if (curr->next) ((im_piece_t*)curr->next)->prev = (struct im_piece_t*)second_half;
            curr->next = (struct im_piece_t*)second_half;
            second_half->prev = (struct im_piece_t*)curr;
            curr = (im_piece_t*)second_half->next;
        }
        remaining_len -= to_delete;
        pos = 0; offset = 0;
    }
    buf->total_length -= len;
    update_line_indexing(buf);
    return IM_OK;
}

im_result_t im_buffer_insert(im_buffer_t* buf, size_t pos, const char* text, size_t len) {
    pthread_mutex_lock(&buf->mutex);
    im_result_t res = im_buffer_insert_internal(buf, pos, text, len);
    if (res == IM_OK && len > 0) push_undo(buf, IM_EDIT_INSERT, pos, text, len);
    pthread_mutex_unlock(&buf->mutex);
    return res;
}

im_result_t im_buffer_delete(im_buffer_t* buf, size_t pos, size_t len) {
    if (len == 0) return IM_OK;
    pthread_mutex_lock(&buf->mutex);
    char* deleted_text = im_buffer_get_range(buf, pos, len);
    im_result_t res = im_buffer_delete_internal(buf, pos, len);
    if (res == IM_OK) push_undo(buf, IM_EDIT_DELETE, pos, deleted_text, len);
    free(deleted_text);
    pthread_mutex_unlock(&buf->mutex);
    return res;
}

im_result_t im_buffer_undo(im_buffer_t* buf) {
    pthread_mutex_lock(&buf->mutex);
    if (!buf->undo_stack) {
        pthread_mutex_unlock(&buf->mutex);
        return IM_ERR;
    }
    im_undo_record_t* rec = buf->undo_stack;
    buf->undo_stack = rec->next;
    if (rec->type == IM_EDIT_INSERT) im_buffer_delete_internal(buf, rec->pos, rec->len);
    else im_buffer_insert_internal(buf, rec->pos, rec->text, rec->len);
    rec->next = buf->redo_stack;
    buf->redo_stack = rec;
    pthread_mutex_unlock(&buf->mutex);
    return IM_OK;
}

im_result_t im_buffer_redo(im_buffer_t* buf) {
    pthread_mutex_lock(&buf->mutex);
    if (!buf->redo_stack) {
        pthread_mutex_unlock(&buf->mutex);
        return IM_ERR;
    }
    im_undo_record_t* rec = buf->redo_stack;
    buf->redo_stack = rec->next;
    if (rec->type == IM_EDIT_INSERT) im_buffer_insert_internal(buf, rec->pos, rec->text, rec->len);
    else im_buffer_delete_internal(buf, rec->pos, rec->len);
    rec->next = buf->undo_stack;
    buf->undo_stack = rec;
    pthread_mutex_unlock(&buf->mutex);
    return IM_OK;
}

im_result_t im_buffer_batch_insert(im_buffer_t* buf, im_batch_insert_t* inserts, size_t count) {
    pthread_mutex_lock(&buf->mutex);
    for (size_t i = 0; i < count; i++) im_buffer_insert_internal(buf, inserts[i].pos, inserts[i].text, inserts[i].len);
    pthread_mutex_unlock(&buf->mutex);
    return IM_OK;
}

im_result_t im_buffer_batch_delete(im_buffer_t* buf, im_batch_delete_t* deletes, size_t count) {
    pthread_mutex_lock(&buf->mutex);
    for (size_t i = 0; i < count; i++) im_buffer_delete_internal(buf, deletes[i].pos, deletes[i].len);
    pthread_mutex_unlock(&buf->mutex);
    return IM_OK;
}
