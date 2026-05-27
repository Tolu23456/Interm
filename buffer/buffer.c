#include "buffer.h"
#include <stdlib.h>
#include <string.h>

static size_t count_lines(const char* text, size_t len) {
    size_t count = 0;
    for (size_t i = 0; i < len; i++) if (text[i] == '\n') count++;
    return count;
}

static void free_piece(im_piece_t* piece) {
    if (!piece) return;
    free(piece->relative_line_offsets);
    free(piece);
}

static im_piece_t* create_piece(im_piece_source_t source, size_t start, size_t length, im_buffer_t* buf) {
    im_piece_t* piece = (im_piece_t*)malloc(sizeof(im_piece_t));
    if (!piece) return NULL;
    piece->source = source;
    piece->start = start;
    piece->length = length;
    const char* data = (source == IM_SOURCE_ORIGINAL) ? buf->original_data : buf->add_data;

    size_t count = count_lines(data + start, length);
    piece->line_count = count;
    if (count > 0) {
        piece->relative_line_offsets = (size_t*)malloc(count * sizeof(size_t));
        if (!piece->relative_line_offsets) { free(piece); return NULL; }
        size_t idx = 0;
        for (size_t i = 0; i < length; i++) {
            if (data[start + i] == '\n') {
                piece->relative_line_offsets[idx++] = i;
            }
        }
    } else {
        piece->relative_line_offsets = NULL;
    }

    piece->next = NULL;
    piece->prev = NULL;
    return piece;
}

static im_piece_t* split_piece(im_piece_t* curr, size_t local_offset) {
    if (local_offset == 0 || local_offset >= curr->length) return NULL;

    im_piece_t* second_half = (im_piece_t*)malloc(sizeof(im_piece_t));
    if (!second_half) return NULL;

    second_half->source = curr->source;
    second_half->start = curr->start + local_offset;
    second_half->length = curr->length - local_offset;
    second_half->next = NULL;
    second_half->prev = NULL;

    size_t lines_before = 0;
    while (lines_before < curr->line_count && curr->relative_line_offsets[lines_before] < local_offset) {
        lines_before++;
    }

    size_t lines_after = curr->line_count - lines_before;
    second_half->line_count = lines_after;
    if (lines_after > 0) {
        second_half->relative_line_offsets = (size_t*)malloc(lines_after * sizeof(size_t));
        if (!second_half->relative_line_offsets) { free(second_half); return NULL; }
        for (size_t i = 0; i < lines_after; i++) {
            second_half->relative_line_offsets[i] = curr->relative_line_offsets[lines_before + i] - local_offset;
        }
    } else {
        second_half->relative_line_offsets = NULL;
    }

    curr->length = local_offset;
    curr->line_count = lines_before;
    if (lines_before == 0) {
        free(curr->relative_line_offsets);
        curr->relative_line_offsets = NULL;
    } else {
        size_t* new_offsets = (size_t*)realloc(curr->relative_line_offsets, lines_before * sizeof(size_t));
        if (new_offsets) curr->relative_line_offsets = new_offsets;
    }

    return second_half;
}

static void update_line_indexing_insert(im_buffer_t* buf, size_t pos, const char* text, size_t len) {
    size_t line_idx = 0;
    if (buf->line_count > 0) {
        size_t low = 0, high = buf->line_count - 1;
        while (low <= high) {
            size_t mid = low + (high - low) / 2;
            if (buf->line_offsets[mid] > pos) {
                line_idx = mid;
                if (mid == 0) break;
                high = mid - 1;
            } else {
                low = mid + 1;
                line_idx = low;
            }
        }
    }

    for (size_t i = line_idx; i < buf->line_count; i++) {
        buf->line_offsets[i] += len;
    }

    size_t new_lines_count = 0;
    for (size_t i = 0; i < len; i++) if (text[i] == '\n') new_lines_count++;

    if (new_lines_count > 0) {
        if (buf->line_count + new_lines_count > buf->line_capacity) {
            size_t new_cap = (buf->line_count + new_lines_count) + 4096;
            size_t* new_offsets = (size_t*)realloc(buf->line_offsets, new_cap * sizeof(size_t));
            if (new_offsets) {
                buf->line_offsets = new_offsets;
                buf->line_capacity = new_cap;
            }
        }

        memmove(&buf->line_offsets[line_idx + new_lines_count], &buf->line_offsets[line_idx], (buf->line_count - line_idx) * sizeof(size_t));

        size_t current_new_idx = line_idx;
        for (size_t i = 0; i < len; i++) {
            if (text[i] == '\n') {
                buf->line_offsets[current_new_idx++] = pos + i + 1;
            }
        }
        buf->line_count += new_lines_count;
    }
}

static void update_line_indexing_delete(im_buffer_t* buf, size_t pos, size_t len) {
    if (buf->line_count <= 1) return;

    size_t start_line_idx = buf->line_count;
    size_t end_line_idx = 0;

    size_t low = 0, high = buf->line_count - 1;
    while (low <= high) {
        size_t mid = low + (high - low) / 2;
        if (buf->line_offsets[mid] > pos) {
            start_line_idx = mid;
            if (mid == 0) break;
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }

    low = 0; high = buf->line_count - 1;
    while (low <= high) {
        size_t mid = low + (high - low) / 2;
        if (buf->line_offsets[mid] <= pos + len) {
            end_line_idx = mid;
            low = mid + 1;
        } else {
            if (mid == 0) break;
            high = mid - 1;
        }
    }

    size_t lines_to_remove = 0;
    if (end_line_idx >= start_line_idx) {
        lines_to_remove = end_line_idx - start_line_idx + 1;
        memmove(&buf->line_offsets[start_line_idx], &buf->line_offsets[end_line_idx + 1], (buf->line_count - (end_line_idx + 1)) * sizeof(size_t));
        buf->line_count -= lines_to_remove;
    }

    for (size_t i = start_line_idx; i < buf->line_count; i++) {
        buf->line_offsets[i] -= len;
    }
}

static void full_rebuild_line_indexing(im_buffer_t* buf) {
    size_t total_lines = 1;
    im_piece_t* curr = buf->head;
    while (curr) {
        total_lines += curr->line_count;
        curr = (im_piece_t*)curr->next;
    }

    if (!buf->line_offsets || total_lines > buf->line_capacity) {
        size_t new_cap = total_lines + 4096;
        size_t* new_offsets = (size_t*)realloc(buf->line_offsets, new_cap * sizeof(size_t));
        if (!new_offsets) return;
        buf->line_offsets = new_offsets;
        buf->line_capacity = new_cap;
    }

    buf->line_count = 0;
    buf->line_offsets[buf->line_count++] = 0;

    curr = buf->head;
    size_t current_abs_offset = 0;
    while (curr) {
        for (size_t i = 0; i < curr->line_count; i++) {
            buf->line_offsets[buf->line_count++] = current_abs_offset + curr->relative_line_offsets[i] + 1;
        }
        current_abs_offset += curr->length;
        curr = (im_piece_t*)curr->next;
    }
}

im_result_t im_buffer_init(im_buffer_t* buf, const char* initial_text, size_t size) {
    memset(buf, 0, sizeof(im_buffer_t));
    pthread_mutex_init(&buf->mutex, NULL);
    if (initial_text && size > 0) {
        buf->original_data = (char*)malloc(size);
        if (!buf->original_data) return IM_ERR_NOMEM;
        memcpy(buf->original_data, initial_text, size);
        buf->original_size = size;
        buf->head = create_piece(IM_SOURCE_ORIGINAL, 0, size, buf);
        if (!buf->head) return IM_ERR_NOMEM;
        buf->total_length = size;
    }
    buf->add_capacity = 4096;
    buf->add_data = (char*)malloc(buf->add_capacity);
    if (!buf->add_data) return IM_ERR_NOMEM;
    full_rebuild_line_indexing(buf);
    return IM_OK;
}

im_result_t im_buffer_destroy(im_buffer_t* buf) {
    if (!buf) return IM_ERR;
    pthread_mutex_lock(&buf->mutex);
    im_piece_t* curr = buf->head;
    while (curr) {
        im_piece_t* next = (im_piece_t*)curr->next;
        free_piece(curr);
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
        size_t new_cap = (buf->add_size + len) * 2;
        char* new_data = (char*)realloc(buf->add_data, new_cap);
        if (!new_data) return 0;
        buf->add_data = new_data;
        buf->add_capacity = new_cap;
    }
    size_t start = buf->add_size;
    memcpy(buf->add_data + start, text, len);
    buf->add_size += len;
    return start;
}

static size_t im_buffer_copy_range_internal(im_buffer_t* buf, size_t pos, size_t len, char* dest) {
    if (pos + len > buf->total_length) return 0;
    im_piece_t* curr = buf->head;
    size_t offset = 0;
    while (curr && offset + curr->length <= pos) {
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
    return bytes_copied;
}

size_t im_buffer_copy_range(im_buffer_t* buf, size_t pos, size_t len, char* dest) {
    pthread_mutex_lock(&buf->mutex);
    size_t res = im_buffer_copy_range_internal(buf, pos, len, dest);
    pthread_mutex_unlock(&buf->mutex);
    return res;
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

static void free_undo_record(im_undo_record_t* rec) {
    if (!rec) return;
    free(rec->text);
    free(rec);
}

static void push_undo(im_buffer_t* buf, im_edit_type_t type, size_t pos, const char* text, size_t len) {
    im_undo_record_t* rec = (im_undo_record_t*)malloc(sizeof(im_undo_record_t));
    if (!rec) return;
    rec->type = type;
    rec->pos = pos;
    rec->len = len;
    rec->text = (char*)malloc(len + 1);
    if (!rec->text) { free(rec); return; }
    memcpy(rec->text, text, len);
    rec->text[len] = '\0';
    rec->next = buf->undo_stack;
    buf->undo_stack = rec;
    while (buf->redo_stack) {
        im_undo_record_t* next = buf->redo_stack->next;
        free_undo_record(buf->redo_stack);
        buf->redo_stack = next;
    }
}

static im_result_t im_buffer_insert_internal(im_buffer_t* buf, size_t pos, const char* text, size_t len) {
    if (pos > buf->total_length) return IM_ERR_INVALID_ARG;
    if (len == 0) return IM_OK;
    size_t add_start = append_to_add_buffer(buf, text, len);

    // Attempt merge if appending to end of buffer and coming from same add sequence
    if (pos == buf->total_length && buf->head) {
        im_piece_t* last = buf->head;
        while (last->next) last = (im_piece_t*)last->next;
        if (last->source == IM_SOURCE_ADD && last->start + last->length == add_start) {
            size_t new_lines = count_lines(text, len);
            if (new_lines > 0) {
                size_t* new_offsets = (size_t*)realloc(last->relative_line_offsets, (last->line_count + new_lines) * sizeof(size_t));
                if (new_offsets) {
                    last->relative_line_offsets = new_offsets;
                    size_t idx = 0;
                    for (size_t i = 0; i < len; i++) {
                        if (text[i] == '\n') {
                            last->relative_line_offsets[last->line_count + idx] = last->length + i;
                            idx++;
                        }
                    }
                    last->line_count += new_lines;
                }
            }
            last->length += len;
            update_line_indexing_insert(buf, pos, text, len);
            buf->total_length += len;
            return IM_OK;
        }
    }

    im_piece_t* new_piece = create_piece(IM_SOURCE_ADD, add_start, len, buf);
    if (!new_piece) return IM_ERR_NOMEM;

    if (buf->head == NULL) {
        buf->head = new_piece;
    } else if (pos == 0) {
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
            im_piece_t* second_half = split_piece(curr, local_offset);
            if (!second_half) { free_piece(new_piece); return IM_ERR_NOMEM; }
            second_half->next = curr->next;
            if (curr->next) ((im_piece_t*)curr->next)->prev = (struct im_piece_t*)second_half;
            curr->next = (struct im_piece_t*)new_piece;
            new_piece->prev = (struct im_piece_t*)curr;
            new_piece->next = (struct im_piece_t*)second_half;
            second_half->prev = (struct im_piece_t*)new_piece;
        }
    }
    update_line_indexing_insert(buf, pos, text, len);
    buf->total_length += len;
    return IM_OK;
}

static im_result_t im_buffer_delete_internal(im_buffer_t* buf, size_t pos, size_t len) {
    if (pos + len > buf->total_length) return IM_ERR_INVALID_ARG;
    if (len == 0) return IM_OK;

    update_line_indexing_delete(buf, pos, len);

    im_piece_t* curr = buf->head;
    size_t offset = 0;
    while (curr && offset + curr->length <= pos) {
        offset += curr->length;
        curr = (im_piece_t*)curr->next;
    }
    size_t remaining_len = len;
    while (curr && remaining_len > 0) {
        size_t local_offset = pos > offset ? pos - offset : 0;
        size_t available = curr->length - local_offset;
        if (available == 0) { curr = (im_piece_t*)curr->next; continue; }
        size_t to_delete = (available < remaining_len) ? available : remaining_len;

        if (local_offset == 0 && to_delete == curr->length) {
            im_piece_t* to_free = curr;
            if (to_free->prev) ((im_piece_t*)to_free->prev)->next = to_free->next;
            else buf->head = (im_piece_t*)to_free->next;
            if (to_free->next) ((im_piece_t*)to_free->next)->prev = to_free->prev;
            curr = (im_piece_t*)to_free->next;
            free_piece(to_free);
        } else if (local_offset == 0) {
            size_t lines_removed = 0;
            while (lines_removed < curr->line_count && curr->relative_line_offsets[lines_removed] < to_delete) {
                lines_removed++;
            }
            size_t lines_remaining = curr->line_count - lines_removed;
            if (lines_remaining > 0) {
                size_t* new_offsets = (size_t*)malloc(lines_remaining * sizeof(size_t));
                for (size_t i = 0; i < lines_remaining; i++) {
                    new_offsets[i] = curr->relative_line_offsets[lines_removed + i] - to_delete;
                }
                free(curr->relative_line_offsets);
                curr->relative_line_offsets = new_offsets;
            } else {
                free(curr->relative_line_offsets);
                curr->relative_line_offsets = NULL;
            }
            curr->line_count = lines_remaining;
            curr->start += to_delete;
            curr->length -= to_delete;
            curr = (im_piece_t*)curr->next;
        } else if (local_offset + to_delete == curr->length) {
            size_t lines_remaining = 0;
            while (lines_remaining < curr->line_count && curr->relative_line_offsets[lines_remaining] < local_offset) {
                lines_remaining++;
            }
            if (lines_remaining > 0) {
                size_t* new_offsets = (size_t*)realloc(curr->relative_line_offsets, lines_remaining * sizeof(size_t));
                if (new_offsets) curr->relative_line_offsets = new_offsets;
            } else {
                free(curr->relative_line_offsets);
                curr->relative_line_offsets = NULL;
            }
            curr->line_count = lines_remaining;
            curr->length -= to_delete;
            curr = (im_piece_t*)curr->next;
        } else {
            im_piece_t* second_half = split_piece(curr, local_offset + to_delete);
            size_t lines_remaining = 0;
            while (lines_remaining < curr->line_count && curr->relative_line_offsets[lines_remaining] < local_offset) {
                lines_remaining++;
            }
            if (lines_remaining > 0) {
                size_t* new_offsets = (size_t*)realloc(curr->relative_line_offsets, lines_remaining * sizeof(size_t));
                if (new_offsets) curr->relative_line_offsets = new_offsets;
            } else {
                free(curr->relative_line_offsets);
                curr->relative_line_offsets = NULL;
            }
            curr->line_count = lines_remaining;
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
    char* deleted_text = (char*)malloc(len + 1);
    if (!deleted_text) { pthread_mutex_unlock(&buf->mutex); return IM_ERR_NOMEM; }
    im_buffer_copy_range_internal(buf, pos, len, deleted_text);
    deleted_text[len] = '\0';
    im_result_t res = im_buffer_delete_internal(buf, pos, len);
    if (res == IM_OK) push_undo(buf, IM_EDIT_DELETE, pos, deleted_text, len);
    free(deleted_text);
    pthread_mutex_unlock(&buf->mutex);
    return res;
}

im_result_t im_buffer_undo(im_buffer_t* buf) {
    pthread_mutex_lock(&buf->mutex);
    if (!buf->undo_stack) { pthread_mutex_unlock(&buf->mutex); return IM_ERR; }
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
    if (!buf->redo_stack) { pthread_mutex_unlock(&buf->mutex); return IM_ERR; }
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
