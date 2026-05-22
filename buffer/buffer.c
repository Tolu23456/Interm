#include "buffer.h"
#include <stdlib.h>
#include <string.h>

// Internal piece creation
static im_piece_t* create_piece(im_piece_source_t source, size_t start, size_t length) {
    im_piece_t* piece = (im_piece_t*)malloc(sizeof(im_piece_t));
    piece->source = source;
    piece->start = start;
    piece->length = length;
    piece->next = NULL;
    piece->prev = NULL;
    return piece;
}

im_result_t im_buffer_init(im_buffer_t* buf, const char* initial_text, size_t size) {
    memset(buf, 0, sizeof(im_buffer_t));
    
    if (initial_text && size > 0) {
        buf->original_data = (char*)malloc(size);
        memcpy(buf->original_data, initial_text, size);
        buf->original_size = size;
        
        buf->head = create_piece(IM_SOURCE_ORIGINAL, 0, size);
        buf->total_length = size;
    } else {
        // Empty buffer
        buf->head = NULL;
        buf->total_length = 0;
    }
    
    buf->add_capacity = 1024;
    buf->add_data = (char*)malloc(buf->add_capacity);
    buf->add_size = 0;
    
    return IM_OK;
}

im_result_t im_buffer_destroy(im_buffer_t* buf) {
    if (!buf) return IM_ERR;
    
    im_piece_t* curr = buf->head;
    while (curr) {
        im_piece_t* next = (im_piece_t*)curr->next;
        free(curr);
        curr = next;
    }
    
    free(buf->original_data);
    free(buf->add_data);
    free(buf->line_offsets);
    
    return IM_OK;
}

// Helper to add text to add_buffer
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

im_result_t im_buffer_insert(im_buffer_t* buf, size_t pos, const char* text, size_t len) {
    if (pos > buf->total_length) return IM_ERR_INVALID_ARG;
    
    size_t add_start = append_to_add_buffer(buf, text, len);
    im_piece_t* new_piece = create_piece(IM_SOURCE_ADD, add_start, len);
    
    if (buf->head == NULL) {
        buf->head = new_piece;
        buf->total_length = len;
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
        // Find piece to split
        im_piece_t* curr = buf->head;
        size_t offset = 0;
        while (curr && offset + curr->length < pos) {
            offset += curr->length;
            curr = (im_piece_t*)curr->next;
        }
        
        size_t local_offset = pos - offset;
        if (local_offset == 0) {
            // Insert before curr
            new_piece->next = (struct im_piece_t*)curr;
            new_piece->prev = curr->prev;
            if (curr->prev) ((im_piece_t*)curr->prev)->next = (struct im_piece_t*)new_piece;
            curr->prev = (struct im_piece_t*)new_piece;
        } else if (local_offset == curr->length) {
            // Insert after curr
            new_piece->next = curr->next;
            new_piece->prev = (struct im_piece_t*)curr;
            if (curr->next) ((im_piece_t*)curr->next)->prev = (struct im_piece_t*)new_piece;
            curr->next = (struct im_piece_t*)new_piece;
        } else {
            // Split piece
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
    return IM_OK;
}

im_result_t im_buffer_delete(im_buffer_t* buf, size_t pos, size_t len) {
    (void)buf; (void)pos; (void)len;
    return IM_ERR_NOT_IMPLEMENTED;
}

char* im_buffer_get_range(im_buffer_t* buf, size_t pos, size_t len) {
    if (pos + len > buf->total_length) return NULL;
    
    char* result = (char*)malloc(len + 1);
    if (!result) return NULL;
    
    im_piece_t* curr = buf->head;
    size_t offset = 0;
    while (curr && offset + curr->length <= pos) {
        if (offset + curr->length == pos && curr->next) {
             // Edge case: pos is exactly at the boundary. 
             // We can either stay at curr and local_offset=length, or go to next and local_offset=0.
             // Our loop logic handles local_offset=0 better if we move to next piece if pos is at its start.
        }
        offset += curr->length;
        curr = (im_piece_t*)curr->next;
    }
    
    // Backtrack one if we went too far or if pos is exactly at the start of a piece
    // Actually the logic above might be slightly flawed. Let's restart finding piece.
    curr = buf->head;
    offset = 0;
    while (curr && offset + curr->length <= pos) {
        if (offset + curr->length == pos) break; 
        offset += curr->length;
        curr = (im_piece_t*)curr->next;
    }

    size_t bytes_copied = 0;
    size_t local_offset = pos - offset;
    
    while (curr && bytes_copied < len) {
        size_t to_copy = curr->length - local_offset;
        if (bytes_copied + to_copy > len) to_copy = len - bytes_copied;
        
        const char* source_data = (curr->source == IM_SOURCE_ORIGINAL) ? buf->original_data : buf->add_data;
        memcpy(result + bytes_copied, source_data + curr->start + local_offset, to_copy);
        
        bytes_copied += to_copy;
        local_offset = 0; 
        curr = (im_piece_t*)curr->next;
    }
    
    result[len] = '\0';
    return result;
}

size_t im_buffer_get_line_count(im_buffer_t* buf) {
    return buf->line_count;
}
