#ifndef INTERM_TASK_H
#define INTERM_TASK_H

#include "common.h"

typedef enum {
    IM_TASK_STATE_QUEUED,
    IM_TASK_STATE_RUNNING,
    IM_TASK_STATE_COMPLETED,
    IM_TASK_STATE_FAILED,
    IM_TASK_STATE_CANCELLED
} im_task_state_t;

typedef struct im_task im_task_t;

typedef void* (*im_task_func_t)(void* data, bool* cancelled);

struct im_task {
    uint64_t id;
    im_priority_t priority;
    im_task_state_t state;
    im_task_func_t func;
    void* input_data;
    void* result;
    bool cancelled;
};

// Async Runtime API
im_result_t im_task_init();
im_result_t im_task_shutdown();
im_result_t im_task_submit(im_task_t* task);
im_result_t im_task_cancel(uint64_t task_id);
im_task_state_t im_task_get_status(uint64_t task_id);

#endif // INTERM_TASK_H
