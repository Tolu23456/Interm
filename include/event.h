#ifndef INTERM_EVENT_H
#define INTERM_EVENT_H

#include "common.h"

typedef enum {
    IM_EVENT_NONE = 0,
    
    // Buffer Events
    IM_EVENT_BUFFER_INSERT,
    IM_EVENT_BUFFER_DELETE,
    IM_EVENT_BUFFER_UPDATE,
    
    // Render Events
    IM_EVENT_RENDER_REQUEST,
    IM_EVENT_RENDER_INVALIDATE,
    
    // Input Events
    IM_EVENT_KEY_PRESS,
    IM_EVENT_MOUSE_EVENT,
    
    // Async Events
    IM_EVENT_TASK_COMPLETED,
    IM_EVENT_TASK_FAILED,
    
    // System Events
    IM_EVENT_SHUTDOWN,
    
    IM_EVENT_MAX
} im_event_type_t;

typedef struct {
    im_event_type_t type;
    im_priority_t priority;
    uint64_t timestamp;
    void* source;
    void* payload;
    size_t payload_size;
} im_event_t;

typedef void (*im_event_callback_t)(im_event_t* event, void* user_data);

// Event System API (Internal Core)
im_result_t im_event_init();
im_result_t im_event_shutdown();
im_result_t im_event_emit(im_event_t event);
im_result_t im_event_subscribe(im_event_type_t type, im_event_callback_t callback, void* user_data);
im_result_t im_event_unsubscribe(im_event_type_t type, im_event_callback_t callback);

#endif // INTERM_EVENT_H
