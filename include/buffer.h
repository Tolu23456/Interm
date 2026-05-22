#ifndef INTERM_BUFFER_H
#define INTERM_BUFFER_H

#include "common.h"

typedef enum {
    IM_SOURCE_ORIGINAL,
    IM_SOURCE_ADD
} im_piece_source_t;

typedef struct {
    im_piece_source_t source;
    size_t start;
    size_t length;
    // For linked list implementation (simplest for piece table)
    struct im_piece_t* next;
    struct im_piece_t* prev;
} im_piece_t;

typedef struct {
    char* original_data;
    size_t original_size;
    
    char* add_data;
    size_t add_size;
    size_t add_capacity;
    
    im_piece_t* head;
    size_t total_length;
    
    // Line indexing (lazy or incremental)
    size_t* line_offsets;
    size_t line_count;
    size_t line_capacity;
} im_buffer_t;

im_result_t im_buffer_init(im_buffer_t* buf, const char* initial_text, size_t size);
im_result_t im_buffer_destroy(im_buffer_t* buf);

im_result_t im_buffer_insert(im_buffer_t* buf, size_t pos, const char* text, size_t len);
im_result_t im_buffer_delete(im_buffer_t* buf, size_t pos, size_t len);

/**
 * Returns a pointer to the text at the given range. 
 * Warning: may require allocation if range spans multiple pieces.
 */
char* im_buffer_get_range(im_buffer_t* buf, size_t pos, size_t len);

/**
 * Returns the line count.
 */
size_t im_buffer_get_line_count(im_buffer_t* buf);

#endif // INTERM_BUFFER_H
