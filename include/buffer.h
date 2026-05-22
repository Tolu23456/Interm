#ifndef INTERM_BUFFER_H
#define INTERM_BUFFER_H

#include "common.h"
#include <pthread.h>

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

    // Undo system
    struct im_undo_record_t* undo_stack;
    struct im_undo_record_t* redo_stack;

    pthread_mutex_t mutex;
} im_buffer_t;

typedef enum {
    IM_EDIT_INSERT,
    IM_EDIT_DELETE
} im_edit_type_t;

typedef struct im_undo_record_t {
    im_edit_type_t type;
    size_t pos;
    char* text;
    size_t len;
    struct im_undo_record_t* next;
} im_undo_record_t;

im_result_t im_buffer_init(im_buffer_t* buf, const char* initial_text, size_t size);
im_result_t im_buffer_destroy(im_buffer_t* buf);

im_result_t im_buffer_insert(im_buffer_t* buf, size_t pos, const char* text, size_t len);
im_result_t im_buffer_delete(im_buffer_t* buf, size_t pos, size_t len);

/**
 * Returns a pointer to the text at the given range. 
 * Warning: requires allocation. Use im_buffer_copy_range for no-alloc.
 */
char* im_buffer_get_range(im_buffer_t* buf, size_t pos, size_t len);

/**
 * Copies text into the provided buffer. Returns number of bytes copied.
 */
size_t im_buffer_copy_range(im_buffer_t* buf, size_t pos, size_t len, char* dest);

/**
 * Returns the line count.
 */
size_t im_buffer_get_line_count(im_buffer_t* buf);

im_result_t im_buffer_undo(im_buffer_t* buf);
im_result_t im_buffer_redo(im_buffer_t* buf);

// Multicursor Batching
typedef struct {
    size_t pos;
    const char* text;
    size_t len;
} im_batch_insert_t;

typedef struct {
    size_t pos;
    size_t len;
} im_batch_delete_t;

im_result_t im_buffer_batch_insert(im_buffer_t* buf, im_batch_insert_t* inserts, size_t count);
im_result_t im_buffer_batch_delete(im_buffer_t* buf, im_batch_delete_t* deletes, size_t count);

#endif // INTERM_BUFFER_H
